#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../error.h"
#include "../module.h"
#include "../read_buffer.h"
#include "../string_utils.h"

#ifndef MODULE_CPU_MULTIPLIER
#define MODULE_CPU_MULTIPLIER 100
#endif

#define FIELDS 10
#define FIELDS_SHIFT_ALL 16
static const uint32_t F_USER = 1 << 0, F_NICE = 1 << 1, F_SYSTEM = 1 << 2, F_IDLE = 1 << 3,
                      F_IO = 1 << 4, F_IRQ = 1 << 5, F_SOFTIRQ = 1 << 6, F_STEAL = 1 << 7,
                      F_GUEST = 1 << 8, F_GUEST_NICE = 1 << 9;

static const uint32_t F_TOTAL_USAGE = 1 << 0, F_USAGE = 1 << 1;

static const int IDLE = 3;

struct cpu_stats {
    int sum;
    uint64_t data[FIELDS];
};
typedef struct cpu_stats cpu_stats_t;

struct cpu_data {
    int fd;
    int cpus;
    uint32_t flags;
    cpu_stats_t cpu_stats[];
};

typedef struct cpu_data cpu_data_t;

static int handle_stat_line(int fd, uint32_t field_flags, cpu_stats_t *cpu_stats, uint64_t *fields_diff, uint64_t *sum_diff) {
    skip_next(fd);
    uint64_t sum = 0;
    for (int field = 0; field < FIELDS; field++) {
        uint64_t value;
        read_next_uint64(&value);
        sum += value;
        if (field_flags & 1) {
            fields_diff[field] = value - cpu_stats->data[field];
            cpu_stats->data[field] = value;
        }
        field_flags >>= 1;
    }
    *sum_diff = sum - cpu_stats->sum;
    cpu_stats->sum = sum;
}

static int write_data(module_data data, writter_t *wr) {
    cpu_data_t *cpu_data = data;
    int fd = cpu_data->fd;
    lseek(fd, 0, SEEK_SET);
    read_start(fd);
    int lines = cpu_data->cpus + 1;

    for (int line = 0; line < lines; line++) {
        uint32_t field_flags =
            cpu_data->flags >> (line ? FIELDS_SHIFT_ALL : 0) & 0xFFFF;
        if (field_flags) {
            uint64_t diffs[FIELDS];
            uint64_t diff_sum;
            handle_stat_line(fd, field_flags, &cpu_data->cpu_stats[line], diffs, &diff_sum);
            uint32_t flags = field_flags;
            int field = 0;
            while (flags) {
                if (flags & 1) {
                    if (diff_sum == 0)
                        diff_sum = 1;
                    uint32_t tenth_of_percent = (diffs[field] * MODULE_CPU_MULTIPLIER + (diff_sum >> 1)) / diff_sum;
                    write_uint(wr, tenth_of_percent);
                    write_char(wr, ' ');
                }
                flags >>= 1;
                field++;
            }
        }
        next_line();
    }
    return 0;
}

static int preload_data(module_data data) {
    char waste_buffer[256];
    writter_t wt = {.buffer = waste_buffer, .len = 256, .pos = 0};
    write_data(data, &wt);
}


static int get_number_of_cores(int fd) {
    lseek(fd, 0, SEEK_SET);
    read_start(fd);
    char buf[32];
    if (read_next_string(buf, 32) != read_result_ok)
        return 0;

    if (strcmp(buf, "cpu"))
        return 0;

    int cores = 0;
    while (true) {
        next_line();
        if (read_next_string(buf, 32) != read_result_ok)
            return 0;
        if (!string_starts_with(buf, "cpu"))
            return cores;
        cores++;
    }
}

int bit_count_slow(uint32_t x) {
    int count = 0;
    while(x) {
        if (x & 1)
            count++;
        x >>= 1;
    }
    return count;
}

module_config_t module_init_cpu(const char *args) {
    int fd = open("/proc/stat", O_RDONLY);

    if (fd == -1)
        exit_with_perror("cpu usage module init failed");

    if (args == NULL || string_is_empty(args))
        args = "total_idle";

    uint32_t fields_bitset = 0;
    char arg_buffer[64];
    int n = 0, idx = 0;

    while (sscanf(args + (idx += n), "%63s%n", arg_buffer, &n) == 1) {
        char * arg = arg_buffer;
        uint32_t flag_shift = FIELDS_SHIFT_ALL;
        if (string_starts_with(arg_buffer, "total")) {
            flag_shift = 0;
            arg = arg_buffer + 6;
        }

        if (string_starts_with("user", arg))
            fields_bitset |= F_USER << flag_shift;
        else if (string_starts_with("nice", arg))
            fields_bitset |= F_NICE << flag_shift;
        else if (string_starts_with("system", arg))
            fields_bitset |= F_SYSTEM << flag_shift;
        else if (string_starts_with("idle", arg))
            fields_bitset |= F_IDLE << flag_shift;
        else if (string_starts_with("io", arg))
            fields_bitset |= F_IO << flag_shift;
        else if (string_starts_with("irq", arg))
            fields_bitset |= F_IRQ << flag_shift;
        else if (string_starts_with("softirq", arg))
            fields_bitset |= F_SOFTIRQ << flag_shift;
        else if (string_starts_with("steal", arg))
            fields_bitset |= F_STEAL << flag_shift;
        else if (string_starts_with("guest", arg))
            fields_bitset |= F_GUEST << flag_shift;
        else if (string_starts_with("guest_nice", arg))
            fields_bitset |= F_GUEST_NICE << flag_shift;
        else
            exit_with_error("cpu module init failed: invalid argument: %s\n", arg_buffer);
    }

    int cpus = 0;
    if (fields_bitset >> FIELDS_SHIFT_ALL) {
        cpus = get_number_of_cores(fd);
        if (cpus == 0)
            exit_with_error("cpu module init failed, can't determine number of cores");
    }

    module_config_t config;
    cpu_data_t *cpu_data = config.data = calloc(1,
        sizeof(cpu_data_t) + sizeof(cpu_stats_t) * (cpus + ((fields_bitset & 0xFFFF) ? 1 : 0)));
    cpu_data->fd = fd;
    cpu_data->cpus = cpus;
    cpu_data->flags = fields_bitset;
    config.write_data = write_data;
    preload_data(config.data);
    return config;
}

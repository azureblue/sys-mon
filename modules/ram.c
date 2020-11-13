#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../error.h"
#include "../module.h"
#include "../read_buffer.h"
#include "../writter.h"
#include "../string_utils.h"

struct ram_data {
    int fd;
    int flags;
};

typedef struct ram_data ram_data;

enum mem_info_key {
    mem_info_key_total = 0,
    mem_info_key_free,
    mem_info_key_available,
    mem_info_key_buffers,
    mem_info_key_cached
};

typedef enum mem_info_key mem_info_key_t;

#define mem_info_key_size 5
int line_numbers[mem_info_key_size] = {-1};

static int write_data(module_data data, writter_t *wr) {
    ram_data *mem_data = data;
    int fd = mem_data->fd;
    int flags = mem_data->flags;
    lseek(fd, 0, SEEK_SET);
    read_init(fd);
    while (flags) {
        if (flags & 1) {
            int value;
            skip_next(fd);
            read_next_uint(fd, &value);
            write_uint(wr, value / 1000);
            write_char(wr, ' ');
        }
        flags >>= 1;
        next_line(fd);
    }
}

int check_key(mem_info_key_t key, const char *info) {
    if (line_numbers[key] == -1)
        exit_with_error("unsupported key: %s\n", info);
    return line_numbers[key];
}

module_config_t module_init_ram(const char *args) {
    if (string_is_empty(args))
        args = "available";

    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd == -1)
        exit_with_perror("mem usage module init failed");

    char arg[64];
    int line = 0;
    read_init(fd);
    while (true) {
        if (read_next_string(fd, arg, 64) == read_result_eof)
            break;
        if (string_starts_with(arg, "MemTotal"))
            line_numbers[mem_info_key_total] = line;
        else if (string_starts_with(arg, "MemFree"))
            line_numbers[mem_info_key_free] = line;
        else if (string_starts_with(arg, "MemAvailable"))
            line_numbers[mem_info_key_available] = line;
        else if (string_starts_with(arg, "Buffers"))
            line_numbers[mem_info_key_buffers] = line;
        else if (string_starts_with(arg, "Cached"))
            line_numbers[mem_info_key_cached] = line;

        next_line(fd);
        line++;
    }

    int flags = 0;
    int n = 0, idx = 0, count = 0;

    while(sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
        if (string_starts_with("total", arg))
            flags |= 1 << check_key(mem_info_key_total, "total");
        else if (string_starts_with("free", arg))
            flags |= 1 << check_key(mem_info_key_free, "free");
        else if (string_starts_with("available", arg))
            flags |= 1 << check_key(mem_info_key_available, "available");
        else if (string_starts_with("buffers", arg))
            flags |= 1 << check_key(mem_info_key_buffers, "buffers");
        else if (string_starts_with("cached", arg))
            flags |= 1 << check_key(mem_info_key_cached, "cached");
        else
            exit_with_error("disk module init failed: invalid argument: %s\n", arg);
    }
    module_config_t config;
    config.data = calloc(1, sizeof(ram_data));
    ((ram_data *)config.data)->fd = fd;
    ((ram_data *)config.data)->flags = flags;
    config.write_data = write_data;
    return config;
}

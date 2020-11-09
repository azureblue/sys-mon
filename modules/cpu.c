#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <inttypes.h>

#include "../module.h"
#include "../read_buffer.h"

bool startsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

inline static int is_number(char c) {
    return c >= '0' && c <= '9';
}

const int TOTAL_SHIFT = 48;

struct core_data {
    int idle;
    int sum;
    int diff_idle;
    int diff_sum;
};
typedef struct core_data core_data_t;

struct cpu_data {
    int fd;
    int cores;
    core_data_t core_data[];
};

typedef struct cpu_data cpu_data_t;

static int update_data(cpu_data_t *data) {
    int fd = data->fd;
    lseek(fd, 0, SEEK_SET);
    read_init(fd);

    for (int i = 0; i < data->cores; i++) {
        int user, nice, sys, idle;
        skip_next(fd);
        read_next_uint(fd, &user);
        read_next_uint(fd, &nice);
        read_next_uint(fd, &sys);
        read_next_uint(fd, &idle);

        int sum = user + nice + sys + idle;
        data->core_data[i].diff_sum = sum - data->core_data[i].sum;
        data->core_data[i].diff_idle = idle - data->core_data[i].idle;
        data->core_data[i].sum = sum;
        data->core_data[i].idle = idle;
        next_line(fd);
    }
}

static int write_data(module_data data, writter_t *wr) {
    cpu_data_t *cpu_data = data;
    update_data(cpu_data);
    for (int i = 0; i < cpu_data->cores; i++) {
        int diff = cpu_data->core_data[i].diff_sum;
        if (diff != 0) {
            int percent_idle = (cpu_data->core_data[i].diff_idle * 100 + diff / 2) / diff;
            if (percent_idle > 100)
                percent_idle = 100;
            write_uint(wr, 100 - percent_idle);
        } else
            write_uint(wr, 0);

        write_char(wr, ' ');
    }
    return 0;
}

struct module_config module_init_cpu(char *args) {
    struct module_config config;
    int fd = open("/proc/stat", O_RDONLY);
    if (fd == -1) {
        perror("cpu usage module init failed");
        exit(-1);
    }
    if (args == NULL) {
       fprintf(stderr, "cpu module init failed: need argument\n");
       exit(-1);
    }

    int cores;
    sscanf(args, "%d", &cores);
    cores++;

    config.data = calloc(1, sizeof(cpu_data_t) + sizeof(core_data_t) * cores);
    ((cpu_data_t *)config.data)->fd = fd;
    ((cpu_data_t *)config.data)->cores = cores;
    config.write_data = write_data;
    update_data(config.data);
    return config;
}

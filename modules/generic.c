#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/fcntl.h>
#include "../module.h"
#include "../read_buffer.h"
#include "../error.h"

struct gen_data {
    int fd;
    int64_t prev;
    bool diff;
};

typedef struct gen_data gen_data_t;

static int preload_data(gen_data_t * gen_data) {
    read_start(gen_data->fd);
    read_next_int64(&gen_data->prev);
}

static int write_data(module_data data, writter_t *wr) {
    char buf[32];
    gen_data_t * gen_data = data;
    lseek(gen_data->fd, 0, SEEK_SET);
    read_start(gen_data->fd);
    if (!gen_data->diff) {
        read_next_string(buf, 32);
        return write_string(wr, buf);
    } else {
        int64_t prev = gen_data->prev;
        read_next_int64(&gen_data->prev);
        write_int64(wr, gen_data->prev - prev);
    }
}

static void unload(module_data data) {
    gen_data_t *gen_data = data;
    close(gen_data->fd);
    free(data);
}

module_config_t sys_mon_module_init_generic(const char *args) {
  uint32_t lines_bitset = 0;
    bool diff = false;
    char path[128];
    char arg[64];
    int n = 0, idx = 0, count = 0;

    if (sscanf(args + (idx += n), "%127s%n", path, &n) != 1)
        exit_with_error("generic module: too few args");

    while (sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
        if (!strcmp(arg, "diff"))
            diff = true;
        else
            exit_with_error("generic module init failed: invalid argument: %s\n", arg);
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1)
        exit_with_perror("[generic module (%s)] init failed", path);

    module_config_t config;
    config.write_data = write_data;
    config.data = calloc(1, sizeof (gen_data_t));
    ((gen_data_t *)config.data)->fd = fd;
    ((gen_data_t *)config.data)->diff = diff;
    if (diff)
        preload_data(config.data);
    config.unload = unload;
    return config;
 }

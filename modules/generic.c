#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "../module.h"
#include "../read_buffer.h"
#include "../error.h"

struct gen_data {
    int fd;
    char source_desc[];
};

typedef struct gen_data gen_data_t;

static int write_data(module_data data, writter_t *wr) {
    char buf[32];
    gen_data_t * gen_data = data;
    lseek(gen_data->fd, 0, SEEK_SET);
    read_init(gen_data->fd);
    read_next_string(gen_data->fd, buf, 32);
    return write_string(wr, buf);
}

module_config_t module_init_generic(const char *args) {
    char path[128];

    if (sscanf(args, "%s", path) != 1)
        exit_with_perror("generic module: too few arguments");

    int fd = open(path, O_RDONLY);
    if (fd == -1)
        exit_with_perror("generic module: init failed");

    module_config_t config;
    config.write_data = write_data;
    config.data = calloc(1, sizeof (gen_data_t) + strlen(path) + 1);
    strcpy(((gen_data_t *)config.data)->source_desc, path);
    ((gen_data_t *)config.data)->fd = fd;
    return config;
 }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "../module.h"
#include "../read_buffer.h"

#define REQUIRED_BUFFER_SIZE 64
#if REQUIRED_BUFFER_SIZE > BUFFER_SIZE
#error BUFFER_SIZE too small;
#endif

struct gen_data {
    int fd;
    int div;
    char source_desc[];
};

typedef struct gen_data gen_data_t;

static int write_data(module_data data, writter_t *wr) {
    gen_data_t * gen_data = data;
    lseek(gen_data->fd, 0, SEEK_SET);
    if (read_to_buffer(gen_data->fd, REQUIRED_BUFFER_SIZE) == -1)
        return -1;
    int div = gen_data->div;
    int value = (read_next_uint() + div / 2) / div;
    return write_uint(wr, value);
}

struct module_config module_init_generic(char *args) {
    char path[128];
    int div;
    if (sscanf(args, "%s %d", path, &div) != 2) {
        perror("generic module: too few arguments");
        exit(-1);
    }
    struct module_config config;
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("generic module: init failed");
        exit(-1);
    }
    config.write_data = write_data;
    config.data = calloc(1, sizeof (struct gen_data) + strlen(path) + 1);
    strcpy(((gen_data_t *)config.data)->source_desc, path);
    ((gen_data_t *)config.data)->fd = fd;
    ((gen_data_t *)config.data)->div = div;
    return config;
 }

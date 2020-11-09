#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../module.h"
#include "../read_buffer.h"
#include "../writter.h"

struct ram_data {
    int fd;
};

typedef struct ram_data ram_data;

static int write_data(module_data data, writter_t *wr) {
    ram_data *mem_data = data;
    int fd = mem_data->fd;
    lseek(fd, 0, SEEK_SET);
    read_init(fd);
    int max, free;
    skip_next(fd);
    read_next_uint(fd, &max);
    skip_next(fd);

    skip_next(fd);
    skip_next(fd);
    skip_next(fd);

    skip_next(fd);
    read_next_uint(fd, &free);
    skip_next(fd);

    write_uint(wr, max / 1000);
    write_char(wr, ' ');
    write_uint(wr, free / 1000);
}

struct module_config module_init_ram(char *args) {
    struct module_config config;
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd == -1) {
        perror("mem usage module init failed");
        exit(-1);
    }
    config.data = calloc(1, sizeof(ram_data));
    ((ram_data *)config.data)->fd = fd;
    config.write_data = write_data;
    return config;
}

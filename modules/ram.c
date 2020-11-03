#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "../module.h"
#include "../read_buffer.h"
#include "../writter.h"

#define REQUIRED_BUFFER_SIZE 128
#if REQUIRED_BUFFER_SIZE > BUFFER_SIZE
    #error BUFFER_SIZE too small;
#endif

struct ram_data {
    int fd;
};
typedef struct ram_data ram_data;

static int write_data(module_data data, writter_t *wr) {
    ram_data * mem_data = data;
    lseek(mem_data->fd, 0, SEEK_SET);
    if (read_to_buffer(mem_data->fd, REQUIRED_BUFFER_SIZE) == -1)
        return -1;
    int max = next_uint() / 1000;
    skip_int(1);
    int free = next_uint() / 1000;
    write_uint(wr, max);
    write_char(wr, ' ');
    write_uint(wr, free);
}

struct module_config module_init_ram(char *args) {
    struct module_config config;
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd == -1) {
        perror("mem usage module init failed");
        exit(-1);
    }
    config.data = calloc(1, sizeof(ram_data));
    ((ram_data*)config.data)->fd = fd;
    config.write_data = write_data;
    return config;
 }

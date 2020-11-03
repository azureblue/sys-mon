#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "../read_buffer.h"
#include "../writter.h"
#include "../module.h"

#define REQUIRED_BUFFER_SIZE 128
#if REQUIRED_BUFFER_SIZE > BUFFER_SIZE
#error BUFFER_SIZE too small;
#endif

struct disk_usage_data {
    int fd;
    int io_w;
    int io_r;
    int bytes_r;
    int bytes_w;
    char device_info[];
};

typedef struct disk_usage_data disk_usage_data;

static int update_data(disk_usage_data *disk_data) {
    lseek(disk_data->fd, 0, SEEK_SET);
    if (read_to_buffer(disk_data->fd, REQUIRED_BUFFER_SIZE) == -1)
        return -1;
    int readIO = next_uint();
    skip_int(1);
    int readBytes = next_uint() * 512;
    skip_int(1);
    int writeIO = next_uint();
    skip_int(1);
    int writeBytes = next_uint() * 512;
    disk_data->bytes_r = readBytes;
    disk_data->bytes_w = writeBytes;
    disk_data->io_r = readIO;
    disk_data->io_w = writeIO;
}

static int write_data(module_data data, writter_t *wr) {
    struct disk_usage_data *disk_data = data;
    struct disk_usage_data old = *disk_data;
    update_data(disk_data);
    write_uint(wr, disk_data->io_r - old.io_r);
    write_char(wr, ' ');
    write_uint(wr, disk_data->io_w - old.io_w);
    write_char(wr, ' ');
    write_uint(wr, disk_data->bytes_r - old.bytes_r);
    write_char(wr, ' ');
    write_uint(wr, disk_data->bytes_w - old.bytes_w);
    return 0;
}

struct module_config module_init_disk(char *args) {
    char path[128];
    sprintf(path, "/sys/block/%s/stat", args);
    struct module_config config;
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("disk usage module init failed");
        exit(-1);
    }
    config.data = calloc(1, sizeof(struct disk_usage_data) + 5);
    strncpy(((struct disk_usage_data *) config.data)->device_info, args, 4);
    ((struct disk_usage_data *) config.data)->fd = fd;
    update_data(config.data);
    config.write_data = write_data;
    return config;
}

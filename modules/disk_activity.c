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

static const int R_RQS = 1 << 0, R_MERGES = 1 << 1, R_SECTORS = 1 << 2, R_TICKS = 1 << 3,
                 W_RQS = 1 << 4, W_MERGES = 1 << 5, W_SECTORS = 1 << 6, W_TICKS = 1 << 7;

struct disk_activity_data {
    int fd;
    int flags;
    int data[8];
};

typedef struct disk_activity_data disk_activity_data;

static int update_data(disk_activity_data *disk_data) {
    lseek(disk_data->fd, 0, SEEK_SET);
    if (read_to_buffer(disk_data->fd, REQUIRED_BUFFER_SIZE) == -1)
        return -1;
    unsigned int flags = disk_data->flags;
    for (int i = 0; i < 8; i++) {
        if (flags & 1)
            disk_data->data[i] = next_uint();
        else
            skip_int(1);
        flags >>= 1;
    }
}

static int write_data(module_data data, writter_t *wr) {
    disk_activity_data *disk_data = data;
    disk_activity_data old = *disk_data;
    update_data(disk_data);
    unsigned int flags = disk_data->flags;
    for (int i = 0; i < 8; i++) {
        if (flags & 1) {
            write_uint(wr, disk_data->data[i] - old.data[i]);
            write_char(wr, ' ');
        }
        flags >>= 1;
    }
    return 0;
}

struct module_config module_init_disk_activity(char *args) {
    int flags = 0;
    char path[128];
    char arg[64];
    int n = 0, idx = 0, count = 0;
    if (sscanf(args + (idx += n), "%63s%n", arg, &n) != 1) {
        fprintf(stderr, "disk module init failed: too few args\n");
        exit(-1);
    }

    sprintf(path, "/sys/block/%s/stat", arg);
    while(sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
        if (!strcmp(arg, "r_sectors"))
            flags |= R_SECTORS;
        else if (!strcmp(arg, "w_sectors"))
            flags |= W_SECTORS;
        else if (!strcmp(arg, "r_reqs"))
            flags |= R_RQS;
        else if (!strcmp(arg, "w_reqs"))
            flags |= W_RQS;
        else if (!strcmp(arg, "r_merges"))
            flags |= R_MERGES;
        else if (!strcmp(arg, "w_merges"))
            flags |= W_MERGES;
        else if (!strcmp(arg, "r_ticks"))
            flags |= R_TICKS;
        else if (!strcmp(arg, "w_ticks"))
            flags |= W_TICKS;
        else {
            fprintf(stderr, "disk module init failed: invalid argument: %s\n", arg);
            exit(-1);
        }
    }

    if (flags == 0) {
        fprintf(stderr, "disk module init failed: too few args\n");
        exit(-1);
    }
    struct module_config config;
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("disk module init failed");
        exit(-1);
    }
    config.data = calloc(1, sizeof(disk_activity_data) + 5);
    ((disk_activity_data *) config.data)->fd = fd;
    ((disk_activity_data *) config.data)->flags = flags;
    update_data(config.data);
    config.write_data = write_data;
    return config;
}

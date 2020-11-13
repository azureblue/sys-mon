#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../error.h"
#include "../module.h"
#include "../read_buffer.h"
#include "../writter.h"

static const int R_RQS = 1 << 0, R_MERGES = 1 << 1, R_SECTORS = 1 << 2, R_TICKS = 1 << 3,
                 W_RQS = 1 << 4, W_MERGES = 1 << 5, W_SECTORS = 1 << 6, W_TICKS = 1 << 7;

struct disk_activity_data {
    int fd;
    int flags;
    unsigned int data[8];
};

typedef struct disk_activity_data disk_activity_data_t;

static int update_data(disk_activity_data_t *disk_data) {
    int fd = disk_data->fd;
    lseek(fd, 0, SEEK_SET);
    read_init(fd);
    unsigned int flags = disk_data->flags;
    for (int i = 0; i < 8; i++) {
        if (flags & 1)
            read_next_uint(fd, &disk_data->data[i]);
        else
            skip_next(fd);
        flags >>= 1;
    }
}

static int write_data(module_data data, writter_t *wr) {
    disk_activity_data_t *disk_data = data;
    disk_activity_data_t old = *disk_data;
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

module_config_t module_init_disk_activity(const char *args) {
    int flags = 0;
    char path[128];
    char arg[64];
    int n = 0, idx = 0, count = 0;

    if (sscanf(args + (idx += n), "%63s%n", arg, &n) != 1)
        exit_with_error("disk module init failed: too few args");

    sprintf(path, "/sys/block/%s/stat", arg);
    while (sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
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
        else
            exit_with_error("disk module init failed: invalid argument: %s\n", arg);
    }

    if (!flags)
        exit_with_error("disk module init failed: too few args");

    int fd = open(path, O_RDONLY);
    if (fd == -1)
        exit_with_perror("disk module init failed");

    module_config_t config;
    config.data = calloc(1, sizeof(disk_activity_data_t) + 5);
    ((disk_activity_data_t *)config.data)->fd = fd;
    ((disk_activity_data_t *)config.data)->flags = flags;
    update_data(config.data);
    config.write_data = write_data;
    return config;
}

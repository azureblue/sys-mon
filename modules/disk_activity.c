#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../error.h"
#include "../module.h"
#include "../read_buffer.h"
#include "../writter.h"

static const uint32_t R_RQS = 1 << 0, R_MERGES = 1 << 1, R_SECTORS = 1 << 2, R_TICKS = 1 << 3,
                 W_RQS = 1 << 4, W_MERGES = 1 << 5, W_SECTORS = 1 << 6, W_TICKS = 1 << 7,
                 F_CHANGE = 1 << 31;

struct disk_activity_data {
    int fd;
    uint32_t lines_bitset;
    bool change_only;
    uint32_t data[8];
};

typedef struct disk_activity_data disk_activity_data_t;

static int update_data(disk_activity_data_t *disk_data) {
    int fd = disk_data->fd;
    lseek(fd, 0, SEEK_SET);
    read_init(fd);
    uint32_t lines_bitset = disk_data->lines_bitset;
    for (int i = 0; i < 8; i++) {
        if (lines_bitset & 1)
            read_next_uint(fd, &disk_data->data[i]);
        else
            skip_next(fd);
        lines_bitset >>= 1;
    }
}

static int write_data(module_data data, writter_t *wr) {
    disk_activity_data_t *disk_data = data;
    uint32_t lines_bitset = disk_data->lines_bitset;
    disk_activity_data_t old = *disk_data;
    update_data(disk_data);
    bool relative = disk_data->change_only;

    for (int i = 0; i < 8; i++) {
        if (lines_bitset & 1) {
            if (relative)
                write_uint(wr, disk_data->data[i] - old.data[i]);
            else
                write_uint(wr, disk_data->data[i]);
            write_char(wr, ' ');
        }
        lines_bitset >>= 1;
    }
    return 0;
}

module_config_t module_init_disk_activity(const char *args) {
    uint32_t lines_bitset = 0;
    bool change_only = true;
    char path[128];
    char arg[64];
    int n = 0, idx = 0, count = 0;

    if (sscanf(args + (idx += n), "%63s%n", arg, &n) != 1)
        exit_with_error("disk module init failed: too few args");

    sprintf(path, "/sys/block/%s/stat", arg);
    while (sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
        if (!strcmp(arg, "total"))
            change_only = false;
        else if (!strcmp(arg, "r_sectors"))
            lines_bitset |= R_SECTORS;
        else if (!strcmp(arg, "w_sectors"))
            lines_bitset |= W_SECTORS;
        else if (!strcmp(arg, "r_reqs"))
            lines_bitset |= R_RQS;
        else if (!strcmp(arg, "w_reqs"))
            lines_bitset |= W_RQS;
        else if (!strcmp(arg, "r_merges"))
            lines_bitset |= R_MERGES;
        else if (!strcmp(arg, "w_merges"))
            lines_bitset |= W_MERGES;
        else if (!strcmp(arg, "r_ticks"))
            lines_bitset |= R_TICKS;
        else if (!strcmp(arg, "w_ticks"))
            lines_bitset |= W_TICKS;
        else
            exit_with_error("disk module init failed: invalid argument: %s\n", arg);
    }

    if (!lines_bitset)
        exit_with_error("disk module init failed: too few args");

    int fd = open(path, O_RDONLY);
    if (fd == -1)
        exit_with_perror("disk module init failed");

    module_config_t config;
    config.data = calloc(1, sizeof(disk_activity_data_t));
    ((disk_activity_data_t *)config.data)->fd = fd;
    ((disk_activity_data_t *)config.data)->lines_bitset = lines_bitset;
    ((disk_activity_data_t *)config.data)->change_only = change_only;
    update_data(config.data);
    config.write_data = write_data;
    return config;
}

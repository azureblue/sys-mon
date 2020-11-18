#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/fcntl.h>
#include <time.h>
#include "../module.h"
#include "../string_utils.h"
#include "../error.h"

struct time_data {
    int64_t milis;
    uint32_t flags;
};

typedef struct time_data time_data_t;
typedef struct timespec timespec_t;

const uint32_t F_CURRENT = 1 << 0, F_DIFF = 1 << 1;

static int load_data(time_data_t * time_data) {
    timespec_t ts;
    timespec_get(&ts, TIME_UTC);
    time_data->milis = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int write_data(module_data data, writter_t *wr) {
    char buf[32];
    time_data_t * time_data = data;
    int64_t prev = time_data->milis;
    load_data(data);
    bool space = false;
    if (time_data->flags & F_CURRENT) {
        write_int64(wr, time_data->milis);
        space = true;
    }

    if (time_data->flags & F_DIFF) {
        if (space)
            write_char(wr, ' ');
        write_int64(wr, time_data->milis - prev);
        space = true;
    }
}

module_config_t module_init_time(const char *args) {
    if (string_is_empty(args))
        args = "current";

    uint32_t flags = 0;
    char arg[64];
    int n = 0, idx = 0, count = 0;

    while (sscanf(args + (idx += n), "%63s%n", arg, &n) == 1) {
        if (string_starts_with("diff", arg))
            flags |= F_DIFF;
        else if (string_starts_with("current", arg))
            flags |= F_CURRENT;
        else
            exit_with_error("time module init failed: invalid argument: %s\n", arg);
    }


    module_config_t config;
    config.write_data = write_data;
    config.data = calloc(1, sizeof (time_data_t));
    ((time_data_t *)config.data)->flags = flags;
    load_data(config.data);
    return config;
 }

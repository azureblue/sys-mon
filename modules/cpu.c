#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "../module.h"
#include "../read_buffer.h"

#define REQUIRED_BUFFER_SIZE 128
#if REQUIRED_BUFFER_SIZE > BUFFER_SIZE
#error BUFFER_SIZE too small;
#endif

struct cpu_data {
    int fd;
    int idle;
    int sum;
};

typedef struct cpu_data cpu_data_t;

static int update_data(cpu_data_t *data) {
    lseek(data->fd, 0, SEEK_SET);
    if (read_to_buffer(data->fd, REQUIRED_BUFFER_SIZE) == -1)
        return -1;
    int user = next_uint();
    int nice = next_uint();
    int sys = next_uint();
    int idle = next_uint();
    int sum = user + nice + sys + idle;
    data->sum = sum;
    data->idle = idle;
}

static int write_data(module_data data, writter_t *wr) {
    cpu_data_t *cpu_data = data;
    int old_sum = cpu_data->sum;
    int old_idle = cpu_data->idle;
    update_data(cpu_data);
    int diff = cpu_data->sum - old_sum;
    if (diff != 0) {
        int percent_idle = ((cpu_data->idle - old_idle) * 100 + diff / 2) / diff;
        if (percent_idle > 100)
            percent_idle = 100;
        write_uint(wr, 100 - percent_idle);
    } else
        write_uint(wr, 0);
    return 0;
}

static int write_desc(module_data data, writter_t *wt) {
    return write_string(wt, "total cpu usage, format: [percentage_cpu_usage]:uint");
}

struct module_config module_init_cpu(char *args) {
    struct module_config spec;
    int fd = open("/proc/stat", O_RDONLY);
    if (fd == -1) {
        perror("cpu usage module init failed");
        exit(-1);
    }
    spec.data = malloc(sizeof(cpu_data_t));
    ((cpu_data_t *)spec.data)->fd = fd;
    spec.write_description = write_desc;
    spec.write_data = write_data;
    update_data(spec.data);
    return spec;
}

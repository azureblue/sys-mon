#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "../module.h"
#include "../writter.h"

struct system_info_output {
    unsigned int core_usage[8];
    unsigned int core_freq[8];
    unsigned int temp[8];
};

typedef struct system_info_output system_info_output_t;

system_info_output_t info_out;

struct system_info_data {
    module_config_t cpu;
    module_config_t temp[8];
    module_config_t freq[8];
};

typedef struct system_info_data system_info_data_t;

static writter_t wr;

static void append_char(char c) {
    write_char(&wr, c);
}

static void append_text(char const *txt) {
    write_string(&wr, txt);
}

static void append_uint(unsigned int x) {
    write_uint(&wr, x);
}


__attribute__((visibility("default")))
system_info_data_t *
system_info_init() {
    system_info_data_t *handle = malloc(sizeof(system_info_data_t));
    if (handle == NULL)
        exit(-1);

    handle->cpu = sys_mon_load_module("cpu total_idle idle");
    handle->temp[0] = sys_mon_load_module("generic /tmp/.sys-mon/temp2");
    handle->temp[1] = sys_mon_load_module("generic /tmp/.sys-mon/temp3");
    handle->temp[2] = sys_mon_load_module("generic /tmp/.sys-mon/temp4");
    handle->temp[3] = sys_mon_load_module("generic /tmp/.sys-mon/temp5");
    handle->freq[0] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu0/cpufreq/scaling_cur_freq");
    handle->freq[1] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu1/cpufreq/scaling_cur_freq");
    handle->freq[2] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu2/cpufreq/scaling_cur_freq");
    handle->freq[3] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu3/cpufreq/scaling_cur_freq");
    return handle;
}

__attribute__((visibility("default"))) void system_info_close(system_info_data_t *handle) {
    sys_mon_unload_module(&handle->cpu);
    for (int i = 0; i < 4; i++) {
        sys_mon_unload_module(&handle->temp[i]);
        sys_mon_unload_module(&handle->freq[i]);
    }
    free(handle);
}

static inline int append_module_output(module_config_t *conf, writter_t *wt) {
    sys_mon_module_write_data(conf, wt);
    return write_char(wt, '\n');
}

__attribute__((visibility("default"))) system_info_output_t * system_info_get_output(system_info_data_t *handle) {
    return &info_out;
}

__attribute__((visibility("default"))) int system_info_update(system_info_data_t *handle) {

    char buffer[512];
    writter_t module_output_writter = (writter_t){.buffer = buffer, .pos = 0, .len = 512};

    append_module_output(&handle->cpu, &module_output_writter);
    append_module_output(&handle->temp[0], &module_output_writter);
    append_module_output(&handle->temp[1], &module_output_writter);
    append_module_output(&handle->temp[2], &module_output_writter);
    append_module_output(&handle->temp[3], &module_output_writter);
    append_module_output(&handle->freq[0], &module_output_writter);
    append_module_output(&handle->freq[1], &module_output_writter);
    append_module_output(&handle->freq[2], &module_output_writter);
    append_module_output(&handle->freq[3], &module_output_writter);
    write_char(&module_output_writter, 0);

//	printf("%s", buffer);

    sscanf(buffer, "%d%d%d%d %*d%*d%*d%*d  %d%d%d%d%d%d%d%d%d",
           &info_out.core_usage[0], &info_out.core_usage[0], &info_out.core_usage[1], &info_out.core_usage[2], &info_out.core_usage[3],
           &info_out.temp[0], &info_out.temp[1], &info_out.temp[2], &info_out.temp[3],
           &info_out.core_freq[0], &info_out.core_freq[1], &info_out.core_freq[2], &info_out.core_freq[3]
           );



    for (int i = 0; i < 4; i++) {

        info_out.core_freq[i] /= 1000;
        info_out.temp[i] /= 1000;
        info_out.core_usage[i] = 100 - info_out.core_usage[i];
    }


    return 0;
}

struct timespec ts;


int main(int argc, char ** argv) {
    system_info_data_t * handle = system_info_init();
    int interval = 1000;
    if (argc > 1)
        sscanf(argv[1], "%d", &interval);
    ts.tv_sec = interval / 1000;
    ts.tv_nsec =  (interval % 1000) * 1000000;
    while(1) {
        system_info_update(handle);
        printf("\x1b[2J");
        printf("\n");

        for (int i = 0; i < 4; i++)
            printf("%d %d %d\n", info_out.core_freq[i], info_out.temp[i], info_out.core_usage[i]);
        nanosleep(&ts, &ts);
    }

    system_info_close(handle);

}


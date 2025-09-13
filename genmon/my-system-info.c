#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../module.h"
#include "../writter.h"

#define RX_MAX_MBITS_PER_SEC 60
#define TX_MAX_MBITS_PER_SEC 10

struct system_info_output {
    unsigned int cpu_usage;
    unsigned int core_usage[4];
    unsigned int core_freq[4];
    unsigned int free_mem;
    unsigned int temp;
    unsigned int update_time;
};

typedef struct system_info_output system_info_output_t;

system_info_output_t info_out;

struct system_info_data {
    module_config_t cpu;
    module_config_t ram;
    module_config_t temp[4];
    module_config_t freq[4];
    module_config_t time;
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

static inline void run_and_ignore(const char *command) {
    int x = system(command);
}

__attribute__((visibility("default")))
system_info_data_t *
system_info_init() {
    system_info_data_t *handle = malloc(sizeof(system_info_data_t));
    if (handle == NULL)
        exit(-1);

    handle->cpu = sys_mon_load_module("cpu total_idle idle");
    handle->ram = sys_mon_load_module("ram available");
//    handle->temp[0] = sys_mon_load_module("generic /tmp/.sys-mon/temp2");
//    handle->temp[1] = sys_mon_load_module("generic /tmp/.sys-mon/temp3");
//    handle->temp[2] = sys_mon_load_module("generic /tmp/.sys-mon/temp4");
//    handle->temp[3] = sys_mon_load_module("generic /tmp/.sys-mon/temp5");

    handle->temp[0] = sys_mon_load_module("generic /sys/class/hwmon/hwmon1/temp1_input");
    handle->temp[1] = sys_mon_load_module("generic /sys/class/hwmon/hwmon1/temp3_input");
    handle->temp[2] = sys_mon_load_module("generic /sys/class/hwmon/hwmon1/temp4_input");
    handle->temp[3] = sys_mon_load_module("generic /sys/class/hwmon/hwmon1/temp5_input");
    handle->freq[0] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu0/cpufreq/scaling_cur_freq");
    handle->freq[1] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu1/cpufreq/scaling_cur_freq");
    handle->freq[2] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu2/cpufreq/scaling_cur_freq");
    handle->freq[3] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu3/cpufreq/scaling_cur_freq");
    handle->time = sys_mon_load_module("time diff");
    return handle;
}

__attribute__((visibility("default"))) void system_info_close(system_info_data_t *handle) {
    sys_mon_unload_module(&handle->cpu);
    sys_mon_unload_module(&handle->ram);
    for (int i = 0; i < 4; i++) {
        sys_mon_unload_module(&handle->temp[i]);
        sys_mon_unload_module(&handle->freq[i]);
    }
    sys_mon_unload_module(&handle->time);
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
    append_module_output(&handle->ram, &module_output_writter);
    append_module_output(&handle->temp[0], &module_output_writter);
    append_module_output(&handle->temp[1], &module_output_writter);
    append_module_output(&handle->temp[2], &module_output_writter);
    append_module_output(&handle->temp[3], &module_output_writter);
    append_module_output(&handle->freq[0], &module_output_writter);
    append_module_output(&handle->freq[1], &module_output_writter);
    append_module_output(&handle->freq[2], &module_output_writter);
    append_module_output(&handle->freq[3], &module_output_writter);
    append_module_output(&handle->time, &module_output_writter);

    unsigned int temp[4], sda_r_time, sda_w_time, sdb_r_time, sdb_w_time;

    sscanf(buffer, "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
           &info_out.cpu_usage,
           &info_out.core_usage[0], &info_out.core_usage[1], &info_out.core_usage[2], &info_out.core_usage[3],
           &info_out.free_mem,
           &temp[0], &temp[1], &temp[2], &temp[3],
           &info_out.core_freq[0], &info_out.core_freq[1], &info_out.core_freq[2], &info_out.core_freq[3],
           &info_out.update_time);


    info_out.temp = 0;
    for (int i = 0; i < 4; i++) {
        if (temp[i] > info_out.temp)
            info_out.temp = temp[i];
        // if (io[i + 1] > max_io)
        //     max_io = io[i + 1];
        info_out.core_freq[i] /= 1000;
        info_out.core_usage[i] = 100 - info_out.core_usage[i];
    }

    info_out.cpu_usage = 100 - info_out.cpu_usage;

    info_out.temp /= 1000;
    info_out.free_mem /= 100000;

    if (info_out.free_mem < 2) {
        run_and_ignore("pkill chrom");
        run_and_ignore("pkill firefox");
        run_and_ignore("beep");
    } else if (info_out.free_mem < 5) {
        run_and_ignore("beep");
    }

    return 0;
}

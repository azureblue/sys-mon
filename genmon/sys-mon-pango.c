#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sys-mon-pango.h"
#include "../module.h"
#include "../writter.h"

#define RX_MAX_MBITS_PER_SEC 60
#define TX_MAX_MBITS_PER_SEC 10

struct sys_mon_pango {
    module_config_t cpu;
    module_config_t ram;
    module_config_t temp[4];
    module_config_t freq[4];
    module_config_t sda;
    module_config_t sdb;
    module_config_t time;
};

static const char* sys_mon_name = "sys-mon-genmon";
static const char* color_temp = "#db7632";
static const char* color_ram = "#50b9ff";
static const char* color_gpu_usage = "#bf9cff";
static const char* color_freq[] = {"#2a72f7", "#a147f5", "#f53387"};
static const char* color_disk_read = "#2a72f7";
static const char* color_disk_write = "#d94aae";
static const char* color_rx = "#5197b3";
static const char* color_tx = "#8751b3";

static writter_t wr;

static int choose(int n, int min, int max, int val) {
    if (max == 0)
        return 0;

    if (val < min)
        val = min;
    if (val > max)
        val = max;
    val -= min;
    max -= min;

    int idx = (val * n) / max;
    if (idx == n)
        idx--;

    return idx;
}

static const char bar_char(int width, int level) {
    static const char level_start[] = {"0alA"};
    return level_start[width] + level;
}

static const char space(int width) {
    return bar_char(width, 0);
}

static const char* bar_string(unsigned int max, unsigned int value, bool show_nonzero_value, int width) {
    static char char_str[3] = " ";
    int idx = choose(11, 0, max, value);
    if (idx == 0 && value > 0 && show_nonzero_value)
        idx = 1;

    char_str[0] = bar_char(width, idx);
    return char_str;
}

static void append_char(char c) {
    write_char(&wr, c);
}

static void append_text(char const* txt) {
    write_string(&wr, txt);
}

static void append_colored_text(const char* text, const char* color) {
    write_string(&wr, "<span fgcolor=\"");
    write_string(&wr, color);
    write_string(&wr, "\">");
    write_string(&wr, text);
    write_string(&wr, "</span>");
}

static void append_start_color(const char* color) {
    write_string(&wr, "<span fgcolor=\"");
    write_string(&wr, color);
    write_string(&wr, "\">");
}

static void append_section_end() {
    write_string(&wr, "</span>");
}

static void append_bars_start() {
    append_text("<span font_desc=\"BlockBarsGaps 12\">");
}

static void append_uint(unsigned int x) {
    write_uint(&wr, x);
}

static inline void run_and_ignore(const char * command) {
    int x = system(command);
}

__attribute__ ((visibility ("default")))
sys_mon_pango_t * sys_mon_pango_init() {
    sys_mon_pango_t *handle = malloc(sizeof(sys_mon_pango_t));
    if (handle == NULL)
        exit(-1);

    handle->cpu = sys_mon_load_module("cpu total_idle idle");
    handle->ram = sys_mon_load_module("ram available");
    handle->temp[0] = sys_mon_load_module("generic /sys/class/hwmon/hwmon0/temp2_input");
    handle->temp[1] = sys_mon_load_module("generic /sys/class/hwmon/hwmon0/temp3_input");
    handle->temp[2] = sys_mon_load_module("generic /sys/class/hwmon/hwmon0/temp4_input");
    handle->temp[3] = sys_mon_load_module("generic /sys/class/hwmon/hwmon0/temp5_input");
    handle->freq[0] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq");
    handle->freq[1] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu1/cpufreq/cpuinfo_cur_freq");
    handle->freq[2] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu2/cpufreq/cpuinfo_cur_freq");
    handle->freq[3] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu3/cpufreq/cpuinfo_cur_freq");
    handle->sda = sys_mon_load_module("disk_activity sda r_ticks w_ticks");
    handle->sdb = sys_mon_load_module("disk_activity sdb r_ticks w_ticks");
    handle->time = sys_mon_load_module("time diff");
    return handle;
}

__attribute__ ((visibility ("default")))
void sys_mon_pango_close(sys_mon_pango_t *handle) {
    sys_mon_unload_module(&handle->cpu);
    sys_mon_unload_module(&handle->ram);
    for (int i = 0; i < 4; i++) {
        sys_mon_unload_module(&handle->temp[i]);
        sys_mon_unload_module(&handle->freq[i]);
    }
    sys_mon_unload_module(&handle->sda);
    sys_mon_unload_module(&handle->sdb);
    sys_mon_unload_module(&handle->time);
    free(handle);
}

static inline int append_module_output(module_config_t * conf, writter_t * wt) {
    sys_mon_module_write_data(conf, wt);
    return write_char(wt, '\n');
}
__attribute__ ((visibility ("default")))
int sys_mon_plugin_write_pango_string(sys_mon_pango_t *handle, char *buf, int len) {
    wr = (writter_t){.buffer = buf, .pos = 0, .len = len};

    char buffer[512];
    writter_t module_output_writter =  (writter_t){.buffer = buffer, .pos = 0, .len = 512};

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
    append_module_output(&handle->sda, &module_output_writter);
    append_module_output(&handle->sdb, &module_output_writter);
    append_module_output(&handle->time, &module_output_writter);

    char char_str[2] = " ";

    unsigned int cpu_usage,
        usage[5],
        // io[5], nothing,
        mem,
        temp[4],
        freq[4],
        sda_r_time, sda_w_time, sdb_r_time, sdb_w_time,
        rx, tx,
        update_time_ms;

    sscanf(buffer, "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
           &usage[0],
           &usage[1], &usage[2], &usage[3], &usage[4],
           &mem,
           &temp[0], &temp[1], &temp[2], &temp[3],
           &freq[0], &freq[1], &freq[2], &freq[3],
           &sda_r_time, &sda_w_time, &sdb_r_time, &sdb_w_time,
        //    &rx, &tx,
            &update_time_ms);


    int max_temp = 0;
    // int max_io = 0;
    for (int i = 0; i < 4; i++) {
        if (temp[i] > max_temp)
            max_temp = temp[i];
        // if (io[i + 1] > max_io)
        //     max_io = io[i + 1];
        freq[i] /= 1000;
        usage[i] = 100 - usage[i];
    }

    usage[4] = 100 - usage[4];

    max_temp /= 1000;
    mem /= 100000;

    if (mem < 2) {
        run_and_ignore("pkill chrom");
        run_and_ignore("pkill firefox");
        run_and_ignore("beep");
    } else if (mem < 5) {
        run_and_ignore("beep");
    }

    char disk_usage_str[256];
    disk_usage_str[0] = 0;

    append_start_color(color_temp);
    append_uint(max_temp);
    append_text("°C");
    append_section_end();

    append_bars_start();
    append_char(space(0));

    append_colored_text(bar_string(100, usage[1], false, 2), color_freq[choose(3, 2000, 2833, freq[0])]);
    append_colored_text(bar_string(100, usage[2], false, 2), color_freq[choose(3, 2000, 2833, freq[1])]);
    append_colored_text(bar_string(100, usage[3], false, 2), color_freq[choose(3, 2000, 2833, freq[2])]);
    append_colored_text(bar_string(100, usage[4], false, 2), color_freq[choose(3, 2000, 2833, freq[3])]);
    append_char(space(0));
    append_section_end();

    append_start_color(color_gpu_usage);

    if (usage[0] == 100)
        append_text("##");
    else {
        if (usage[0] < 10)
            append_text(" ");
        append_uint(usage[0]);
    }

    append_text("% ");
    append_section_end();
    append_start_color(color_ram);
    append_uint(mem / 10);
    append_text(".");
    append_uint(mem % 10);
    append_text("G");
    append_section_end();

    append_bars_start();
    append_char(space(0));

    unsigned int sda_time = sda_r_time > sda_w_time ? sda_r_time : sda_w_time;
    unsigned int sdb_time = sdb_r_time > sdb_w_time ? sdb_r_time : sdb_w_time;
    append_colored_text(bar_string(update_time_ms, sda_time, true, 2), sda_r_time > sda_w_time ? color_disk_read : color_disk_write);
    append_colored_text(bar_string(update_time_ms, sdb_time, true, 2), sdb_r_time > sdb_w_time ? color_disk_read : color_disk_write);
    // append_colored_text(bar_string(update_time_ms, sda_r_time, true, 2), color_disk_read);
    // append_colored_text(bar_string(update_time_ms, sda_w_time, true, 2), color_disk_write);
    // append_colored_text(bar_string(update_time_ms, sdb_r_time, true, 2), color_disk_read);
    // append_colored_text(bar_string(update_time_ms, sdb_w_time, true, 2), color_disk_write);

    // append_colored_text(bar_string(RX_MAX_MBITS_PER_SEC * update_time_ms * 1000 / 8, rx, true, 2), color_rx);
    // append_colored_text(bar_string(TX_MAX_MBITS_PER_SEC * update_time_ms * 1000 / 8, tx, true, 2), color_tx);
    append_char(space(0));
    append_section_end();

    if (write_char(&wr, 0) == -1) {
        return -1;
    }

    return 0;
}

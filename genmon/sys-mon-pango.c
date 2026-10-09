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

struct system_info_data {
    module_config_t cpu;
    module_config_t ram;
    module_config_t temp[4];
    module_config_t freq[4];
    // module_config_t sda;
    //module_config_t sdb;
    module_config_t gpu_rc6;
    module_config_t time;
};

#define cpu_usage_color_even "#a62cb3"
#define cpu_usage_color_odd "#4900f4"
#define gpu_usage_color "#307899"

static const char* sys_mon_name = "sys-mon-genmon";
static const char* color_temp = "#db7632";
static const char* color_ram = "#50b9ff";
static const char* color_cpu_usage = "#bf9cff";
static const char* color_freq[] = {"#2a72f7", "#a147f5", "#f53387"};
static const char* color_disk_read = "#46a2f2";
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

static inline void sort4_u32(uint32_t a[4]) {
    uint32_t t;
    if (a[0] > a[1]) { t=a[0]; a[0]=a[1]; a[1]=t; }
    if (a[2] > a[3]) { t=a[2]; a[2]=a[3]; a[3]=t; }
    if (a[0] > a[2]) { t=a[0]; a[0]=a[2]; a[2]=t; }
    if (a[1] > a[3]) { t=a[1]; a[1]=a[3]; a[3]=t; }
    if (a[1] > a[2]) { t=a[1]; a[1]=a[2]; a[2]=t; }
}

static inline void swap_pairs_if_greater_u32(uint32_t a[8]) {
    for (int i = 0; i < 4; ++i) {
        uint32_t x = a[i];
        uint32_t y = a[i + 4];
        uint32_t mask = 0u - (x > y);     // 0xFFFFFFFF if x>y, else 0
        uint32_t t = (x ^ y) & mask;
        a[i]      = x ^ t;                // min(x,y)
        a[i + 4]  = y ^ t;                // max(x,y)
    }
}

static inline void cswap_pair_u32(uint32_t a[8], int i, int j)
{
    uint32_t x0 = a[i];
    uint32_t y0 = a[j];
    uint32_t x1 = a[i + 4];
    uint32_t y1 = a[j + 4];

    uint32_t mask = 0u - (x0 > y0);      // 0xFFFFFFFF if x0>y0 else 0
    uint32_t t;

    // swap keys
    t = (x0 ^ y0) & mask;
    x0 ^= t; y0 ^= t;

    // swap attached values in the same way
    t = (x1 ^ y1) & mask;
    x1 ^= t; y1 ^= t;

    a[i]     = x0;
    a[j]     = y0;
    a[i + 4] = x1;
    a[j + 4] = y1;
}

static inline void sort4_pairs_u32(uint32_t a[8])
{
    cswap_pair_u32(a, 0, 1);
    cswap_pair_u32(a, 2, 3);
    cswap_pair_u32(a, 0, 2);
    cswap_pair_u32(a, 1, 3);
    cswap_pair_u32(a, 1, 2);
}

static inline void sort4_u8(uint32_t a[4]) {
    uint32_t t;
    if (a[0] > a[1]) { t=a[0]; a[0]=a[1]; a[1]=t; }
    if (a[2] > a[3]) { t=a[2]; a[2]=a[3]; a[3]=t; }
    if (a[0] > a[2]) { t=a[0]; a[0]=a[2]; a[2]=t; }
    if (a[1] > a[3]) { t=a[1]; a[1]=a[3]; a[3]=t; }
    if (a[1] > a[2]) { t=a[1]; a[1]=a[2]; a[2]=t; }
}

__attribute__ ((visibility ("default")))
sys_mon_pango_t * sys_mon_pango_init() {
    sys_mon_pango_t *handle = malloc(sizeof(sys_mon_pango_t));
    if (handle == NULL)
        exit(-1);

    handle->cpu = sys_mon_load_module("cpu total_idle idle");
    handle->ram = sys_mon_load_module("ram free");
    handle->temp[0] = sys_mon_load_module("generic /tmp/.sys-mon/temp2");
    handle->temp[1] = sys_mon_load_module("generic /tmp/.sys-mon/temp3");
    handle->temp[2] = sys_mon_load_module("generic /tmp/.sys-mon/temp4");
    handle->temp[3] = sys_mon_load_module("generic /tmp/.sys-mon/temp5");

    handle->freq[0] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu0/cpufreq/scaling_cur_freq");
    handle->freq[1] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu1/cpufreq/scaling_cur_freq");
    handle->freq[2] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu2/cpufreq/scaling_cur_freq");
    handle->freq[3] = sys_mon_load_module("generic /sys/bus/cpu/devices/cpu3/cpufreq/scaling_cur_freq");

    handle->time = sys_mon_load_module("time diff");
    handle->gpu_rc6 = sys_mon_load_module("generic /tmp/.sys-mon/gpu_r6_ms diff");
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
    // sys_mon_unload_module(&handle->sda);
    //sys_mon_unload_module(&handle->sdb);
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
    // append_module_output(&handle->sda, &module_output_writter);
  //  append_module_output(&handle->sdb, &module_output_writter);
    append_module_output(&handle->time, &module_output_writter);
    append_module_output(&handle->gpu_rc6, &module_output_writter);

    char char_str[2] = " ";

    uint32_t cpu_usage,
        total_usage,
        usage[8],
        usage_sums[4],
        // io[5], nothing,
        mem,
        temp[4],
        freq[4],
        sda_r_time, sda_w_time, sdb_r_time, sdb_w_time,
        rx, tx,
        update_time_ms,
        gpu_rc6;

    sscanf(buffer, "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
           &total_usage,
           &usage[0], &usage[1], &usage[2], &usage[3],
           &usage[4], &usage[5], &usage[6], &usage[7],
           &mem,
           &temp[0], &temp[1], &temp[2], &temp[3],
           &freq[0], &freq[1], &freq[2], &freq[3],
        //    &sda_r_time, &sda_w_time, &sdb_r_time, &sdb_w_time,
        //    &rx, &tx,
            &update_time_ms,
            &gpu_rc6);

    swap_pairs_if_greater_u32(usage);
    sort4_pairs_u32(usage);

    int max_temp = 0;
    // int max_io = 0;
    total_usage = 100 - total_usage;
    for (int i = 0; i < 8; i++) {
        usage[i] = 100 - usage[i];
    }

    double gpu_usage = 1.0 - ((double) gpu_rc6 / (double) update_time_ms);
    gpu_usage = gpu_usage * gpu_usage * gpu_usage;

    // usage_sums[0] = usage[0] + usage[4];
    // usage_sums[1] = usage[1] + usage[5];
    // usage_sums[2] = usage[2] + usage[6];
    // usage_sums[3] = usage[3] + usage[7];

    // sort4_u32(usage_sums);

    for (int i = 0; i < 4; i++) {
         if (temp[i] > max_temp)
            max_temp = temp[i];
        // if (io[i + 1] > max_io)
        //     max_io = io[i + 1];
        freq[i] /= 1000;
    }

    // usage[8] = 100 - usage[8];

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

// int col0 = 2; if (freq[0] < 3700) col0 = 1; if (freq[0] < 3000) col0 = 0;
// int col1 = 2; if (freq[1] < 3700) col1 = 1; if (freq[1] < 3000) col1 = 0;
// int col2 = 2; if (freq[2] < 3700) col2 = 1; if (freq[2] < 3000) col2 = 0;
// int col3 = 2; if (freq[3] < 3700) col3 = 1; if (freq[3] < 3000) col3 = 0;
//    append_colored_text(bar_string(100, usage[1], false, 2), color_freq[choose(3, 800, 4000, freq[0])]);
  //  append_colored_text(bar_string(100, usage[2], false, 2), color_freq[choose(3, 800, 4000, freq[1])]);
  //  append_colored_text(bar_string(100, usage[3], false, 2), color_freq[choose(3, 800, 4000, freq[2])]);
   // append_colored_text(bar_string(100, usage[4], false, 2), color_freq[choose(3, 800, 4000, freq[3])]);

// append_start_color(cpu_usage_color_even);
//     append_text(bar_string(100, usage[0], false, 2));
//     append_text(bar_string(100, usage[4], false, 2));
// append_section_end();
// append_start_color(cpu_usage_color_odd);
//     append_text(bar_string(100, usage[1], false, 2));
//     append_text(bar_string(100, usage[5], false, 2));
// append_section_end();
// append_start_color(cpu_usage_color_even);
//     append_text(bar_string(100, usage[2], false, 2));
//     append_text(bar_string(100, usage[6], false, 2));
// append_section_end();
// append_start_color(cpu_usage_color_odd);
//     append_text(bar_string(100, usage[3], false, 2));
//     append_text(bar_string(100, usage[7], false, 2));
    // append_text(bar_string(200, usage_sums[0], false, 2));
    // append_text(bar_string(200, usage_sums[1], false, 2));
    // append_text(bar_string(200, usage_sums[2], false, 2));
    // append_text(bar_string(200, usage_sums[3], false, 2));
// append_section_end();
append_colored_text(bar_string(100, usage[0], false, 2), cpu_usage_color_even);
append_colored_text(bar_string(100, usage[4], false, 2), cpu_usage_color_odd);
append_colored_text(bar_string(100, usage[1], false, 2), cpu_usage_color_even);
append_colored_text(bar_string(100, usage[5], false, 2), cpu_usage_color_odd);
append_colored_text(bar_string(100, usage[2], false, 2), cpu_usage_color_even);
append_colored_text(bar_string(100, usage[6], false, 2), cpu_usage_color_odd);
append_colored_text(bar_string(100, usage[3], false, 2), cpu_usage_color_even);
append_colored_text(bar_string(100, usage[7], false, 2), cpu_usage_color_odd);
append_colored_text(bar_string(100, (int)(gpu_usage * 100), false, 1), gpu_usage_color);

append_char(space(0));
    append_section_end();

    append_start_color(color_cpu_usage);

    if (total_usage == 100)
        append_text("##");
    else {
        if (total_usage < 10)
            append_text(" ");
        append_uint(total_usage);
    }

    append_text("% ");
    append_section_end();
    append_start_color(color_ram);
    int mem_gb = mem / 10;
    if (mem_gb < 10)
	append_text(" ");

    append_uint(mem_gb);
    append_text(".");
    append_uint(mem % 10);
    append_text("G");
    append_section_end();

    // append_bars_start();
    // append_char(space(0));

    // append_colored_text(bar_string(update_time_ms, sda_r_time, true, 1), color_disk_read);
    // append_colored_text(bar_string(update_time_ms, sda_w_time, true, 1), color_disk_write);
//    append_colored_text(bar_string(update_time_ms, sdb_r_time, true, 1), color_disk_read);
//    append_colored_text(bar_string(update_time_ms, sdb_w_time, true, 1), color_disk_write);
    // append_colored_text(bar_string(update_time_ms, sda_r_time, true, 2), color_disk_read);
    // append_colored_text(bar_string(update_time_ms, sda_w_time, true, 2), color_disk_write);
    // append_colored_text(bar_string(update_time_ms, sdb_r_time, true, 2), color_disk_read);
    // append_colored_text(bar_string(update_time_ms, sdb_w_time, true, 2), color_disk_write);

    // append_colored_text(bar_string(RX_MAX_MBITS_PER_SEC * update_time_ms * 1000 / 8, rx, true, 2), color_rx);
    // append_colored_text(bar_string(TX_MAX_MBITS_PER_SEC * update_time_ms * 1000 / 8, tx, true, 2), color_tx);
    // append_char(space(0));
    // append_section_end();

    append_text(" ");
    if (write_char(&wr, 0) == -1) {
        return -1;
    }

    return 0;
}

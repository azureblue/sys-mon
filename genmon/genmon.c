#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../client/client.h"
#include "../read_buffer.h"
#include "../shared_buf.h"
#include "../writter.h"

#define GENMON_STR_SIZE 512

const char* sys_mon_name = "sys-mon-genmon";
const char* color_temp = "#db7632";
const char* color_ram = "#50b9ff";
const char* color_cpu = "#bf9cff";
const char* color_freq[] = {"#4355fa", "#8241f2", "#c92cc4", "#f53387"};
const char* color_disk_read = "#8b64b3";
const char* color_disk_write = "#c671eb";

const char* progress_bar_blocks[] = {" ", "\xE2\x96\x81", "\xE2\x96\x82", "\xE2\x96\x83", "\xE2\x96\x84", "\xE2\x96\x85", "\xE2\x96\x86", "\xE2\x96\x87", "\xE2\x96\x87"};
const int progress_bar_blocks_len = 9;

char genmon_str[GENMON_STR_SIZE];
writter_t wr = {.buffer = genmon_str, .pos = 0, .len = GENMON_STR_SIZE};

int choose(int n, int min, int max, int val) {
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

const char* bar_symbol(unsigned int max, unsigned int value, bool show_nonzero_value) {
    int idx = choose(progress_bar_blocks_len, 0, max, value);
    if (idx == 0 && value > 0 && show_nonzero_value)
        idx = 1;
    return progress_bar_blocks[idx];
}

void append_colored_text(const char* text, const char* color) {
    write_string(&wr, "<span fgcolor=\"");
    write_string(&wr, color);
    write_string(&wr, "\">");
    write_string(&wr, text);
    write_string(&wr, "</span>");
}

void append_start_color(const char* color) {
    write_string(&wr, "<span fgcolor=\"");
    write_string(&wr, color);
    write_string(&wr, "\">");
}

void append_color_end() {
    write_string(&wr, "</span>");
}

void append_text(char const* txt) {
    write_string(&wr, txt);
}

void append_uint(unsigned int x) {
    write_uint(&wr, x);
}

int main() {
    sys_mon_handle_t* sys_mon = sys_mon_open(sys_mon_name);

    char buffer[512];
    sys_mon_read_data(sys_mon, buffer, 512);
    sys_mon_close(sys_mon);

    unsigned int cpu_usage, core_0, core_1, mem_mb, t1, t2, freq_0, freq_1, sda_rb, sda_wb, sdb_rb, sdb_wb;
    sscanf(buffer, "%d%d%d%d%d%d%d%d%d%d%d%d",
           &cpu_usage, &core_0, &core_1, &mem_mb, &t1, &t2, &freq_0, &freq_1, &sda_rb, &sda_wb, &sdb_rb, &sdb_wb);

    freq_0 /= 1000;
    freq_0 /= 1000;
    int max_temp = t1 > t2 ? t1 : t2;
    max_temp /= 1000;
    mem_mb /= 1000;


    if (mem_mb < 200) {
        system("pkill chrom");
        system("pkill firefox");
        system("beep");
    } else if (mem_mb < 500) {
        system("beep");
    }

    char disk_usage_str[256];
    disk_usage_str[0] = 0;

    append_text("<txt>");
    append_start_color(color_temp);
    append_uint(max_temp);
    append_text("°C ");
    append_color_end();

    append_colored_text(bar_symbol(100, core_0, false), color_freq[choose(4, 2000, 3000, freq_0)]);
    append_colored_text(bar_symbol(100, core_1, false), color_freq[choose(4, 2000, 3000, freq_1)]);

    append_start_color(color_cpu);
    append_text(" ");

    if (cpu_usage == 100)
        append_text("##");
    else {
        if (cpu_usage < 10)
            append_text(" ");
        append_uint(cpu_usage);
    }

    append_text("% ");
    append_color_end();
    append_start_color(color_ram);
    append_uint(mem_mb / 1000);
    append_text(".");
    append_uint((mem_mb % 1000) / 100);
    append_text("G ");
    append_color_end();
    append_colored_text(bar_symbol(5000000, sda_rb, true), color_disk_read);
    append_colored_text(bar_symbol(5000000, sda_wb, true), color_disk_write);
    append_colored_text(bar_symbol(5000000, sdb_rb, true), color_disk_read);
    append_colored_text(bar_symbol(5000000, sdb_wb, true), color_disk_write);
    append_text(" </txt>");

    if (write_char(&wr, 0) == -1){
        fprintf(stderr, "string writting error. probably buffer length not enough");
        exit(-1);
    }

    puts(genmon_str);
    return 0;
}

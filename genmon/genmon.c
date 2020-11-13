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

const char* progress_bar_blocks[] = {" ", "\xE2\x96\x81", "\xE2\x96\x82", "\xE2\x96\x83", "\xE2\x96\x84", "\xE2\x96\x85", "\xE2\x96\x86", "\xE2\x96\x87", "\xE2\x96\x87"};
const int progress_bar_blocks_len = 9;

char genmon_str[GENMON_STR_SIZE];
writter_t wr = {.buffer = genmon_str, .pos = 0, .len = GENMON_STR_SIZE};

const char* bar_symbol(unsigned int max, unsigned int value, bool show_nonzero_value) {
    if (value > max)
        value = max;
    int n = progress_bar_blocks_len;
    int idx = (value * n + (max / n / 2)) / max;
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

    unsigned int cpu_usage, core_0, core_1, mem_total, mem_mb, t1, t2, freq_0, freq_1, sda_rb, sda_wb, sdb_rb, sdb_wb;
    sscanf(buffer, "%d%d%d%d%d%d%d%d%d%d%d%d%d",
           &cpu_usage, &core_0, &core_1, &mem_total, &mem_mb, &t1, &t2, &freq_0, &freq_1, &sda_rb, &sda_wb, &sdb_rb, &sdb_wb);

    int max_temp = t1 > t2 ? t1 : t2;

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
    append_start_color("#db7632");
    append_uint(max_temp);
    append_text("°C ");
    append_color_end();

    freq_0 /= 100;
    freq_1 /= 100;

    char* freq_colors[] = {"#4355fa", "#8241f2", "#c92cc4", "#f53387"};
    char *core_0_color = freq_colors[0], *core_1_color = freq_colors[0];

    if (freq_0 == 23)
        core_0_color = freq_colors[1];
    else if (freq_0 == 26)
        core_0_color = freq_colors[2];
    else if (freq_0 == 30)
        core_0_color = freq_colors[3];

    if (freq_1 == 23)
        core_1_color = freq_colors[1];
    else if (freq_1 == 26)
        core_1_color = freq_colors[2];
    else if (freq_1 == 30)
        core_1_color = freq_colors[3];

    append_colored_text(bar_symbol(100, core_0, false), core_0_color);
    append_colored_text(bar_symbol(100, core_1, false), core_1_color);

    append_start_color("#bf9cff");
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
    append_start_color("#50b9ff");
    append_uint(mem_mb / 1000);
    append_text(".");
    append_uint((mem_mb % 1000) / 100);
    append_text("G ");
    append_color_end();
    append_colored_text(bar_symbol(5000000, sda_rb, true), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sda_wb, true), "#c671eb");
    append_colored_text(bar_symbol(5000000, sdb_rb, true), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sdb_wb, true), "#c671eb");
    append_text(" </txt>");
    if (write_char(&wr, 0) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
    }
    puts(genmon_str);
    return 0;
}

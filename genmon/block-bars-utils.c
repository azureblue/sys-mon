#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "block-bars-utils.h"

int choose(int n, int min, int max, int val) {
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

char bar_char(int width, int level) {
    static const char level_start[] = {"0alA"};
    return level_start[width] + level;
}

char pie_char(int level) {
    static const char level_start[] = {"'"};
    return level_start[0] + level;
}

char space(int width) {
    return bar_char(width, 0);
}

const char* bar_string(unsigned int max, unsigned int value, bool show_nonzero_value, int width) {
    static char char_str[3] = " ";
    int idx = choose(11, 0, max, value);
    if (idx == 0 && value > 0 && show_nonzero_value)
        idx = 1;

    char_str[0] = bar_char(width, idx);
    return char_str;
}

const char* pie_string(unsigned int max, unsigned int value, bool show_nonzero_value) {
    static char char_str[3] = " ";
    int idx = choose(9, 0, max, value);
    if (idx == 0 && value > 0 && show_nonzero_value)
        idx = 1;

    char_str[0] = pie_char(idx);
    return char_str;
}

void write_colored_text(writter_t* wr, const char* text, const char* color) {
    write_string(wr, "<span fgcolor=\"");
    write_string(wr, color);
    write_string(wr, "\">");
    write_string(wr, text);
    write_string(wr, "</span>");
}

void write_start_color(writter_t* wr, const char* color) {
    write_string(wr, "<span fgcolor=\"");
    write_string(wr, color);
    write_string(wr, "\">");
}

void write_section_end(writter_t* wr) {
    write_string(wr, "</span>");
}

void write_bars_start(writter_t* wr) {
    write_string(wr, "<span font_desc=\"BlockBarsGaps 12\">");
}

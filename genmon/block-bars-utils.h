#ifndef BLOCK_BARS_UTILS_H
#define BLOCK_BARS_UTILS_H

#include "../writter.h"

int choose(int n, int min, int max, int val);
char bar_char(int width, int level);
char pie_char(int level);
char space(int width);
const char* bar_string(unsigned int max, unsigned int value, bool show_nonzero_value, int width);
const char* pie_string(unsigned int max, unsigned int value, bool show_nonzero_value);
void write_colored_text(writter_t* wr, const char* text, const char* color);
void write_start_color(writter_t* wr, const char* color);
void write_section_end(writter_t* wr);
void write_bars_start(writter_t* wr);

#endif /* BLOCK_BARS_UTILS_H */

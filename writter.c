#include "writter.h"
#include <stdlib.h>

int writter_get_size(writter_t *wr) {
    return wr->pos;
}
void *close_writter(writter_t *wr) {
    free(wr);
}

writter_t *create_writter(char *buf, int len) {
    writter_t * wr = malloc(sizeof(writter_t));
    wr->buffer = buf;
    wr->len = len;
    wr->pos = 0;
    return wr;
}

static inline char * current_buf(writter_t *wr) {
    return wr->buffer + wr->pos;
}

inline int write_char(writter_t *wr, char c) {
    if (wr->pos == wr->len)
        return -1;
    wr->buffer[wr->pos++] = c;
}

int write_string(writter_t *wr, const char *str) {
    int res_code = 0;
    while (*str)
        if (res_code = write_char(wr, *str++) != 0)
            return res_code;
    return res_code;
}

int write_uint(writter_t *wr, unsigned int i) {
    int res_code = 0;
    char buf[11];
    int len = 0;
    do {
        buf[len++] = '0' + (i % 10);
        i /= 10;
    } while (i);

    for (i = 0; i < len; i++)
        if (res_code = write_char(wr, buf[len - i - 1]) != 0)
            return res_code;

    return res_code;
}


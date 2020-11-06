#include "writter.h"
#include <stdlib.h>
#include <string.h>

int writter_get_size(writter_t *wr) {
    return wr->pos;
}

static inline char * current_buf(writter_t *wr) {
    return wr->buffer + wr->pos;
}

inline int write_char(writter_t *wr, char c) {
    if (wr->pos == wr->len)
        return -1;
    wr->buffer[wr->pos++] = c;
    return 0;
}

int write_string(writter_t *wr, const char *str) {
    int len = strlen(str);
    if (len + wr->pos >= wr->len)
        return -1;
    memcpy(current_buf(wr), str, len);
    wr->pos += len;
    return 0;
}

int write_uint(writter_t *wr, unsigned int i) {
    int res_code = 0;
    char buf[11];
    int len = 0;
    do {
        buf[len++] = '0' + (i % 10);
        i /= 10;
    } while (i);
    if (wr->pos + len >= wr->len)
        return -1;

    char *wr_buf = current_buf(wr);
    for (i = 0; i < len; i++)
        *(wr_buf++) = buf[len - i - 1];

    wr->pos += len;
    return 0;
}

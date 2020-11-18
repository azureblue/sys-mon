#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "writter.h"

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

int write_int64(writter_t *wr, int64_t x) {
    int res_code = 0;
    char buf[20];
    int len = 0;
    bool negative = false;
    if (x < 0) {
        if (x == INT64_MIN)
            return write_string(wr, "-9223372036854775808");
        x = -x;
        negative = true;
    }

    do {
        buf[len++] = '0' + (x % 10);
        x /= 10;
    } while (x);

    if (negative)
        buf[len++] = '-';

    if (wr->pos + len >= wr->len)
        return -1;

    char *wr_buf = current_buf(wr);
    for (x = 0; x < len; x++)
        *(wr_buf++) = buf[len - x - 1];

    wr->pos += len;
    return 0;
}

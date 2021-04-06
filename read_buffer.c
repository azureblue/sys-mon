#include "read_buffer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define BUFFER_SIZE 64

static char buffer[BUFFER_SIZE];
static int pos;
static int filled;
static int current_fd = -1;
static read_result_t last_error;

read_result_t fill_buffer() {
    filled = read(current_fd, buffer, BUFFER_SIZE);
    if (unlikely(filled < 0)) {
        return read_result_error;
    }

    if (filled == 0) {
        return read_result_eof;
    }

    pos = 0;
    return read_result_ok;
}

inline static int is_number(char c) {
    return c >= '0' && c <= '9';
}

inline static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline static char get() {
    return buffer[pos];
}

inline static read_result_t next() {
    pos++;
    if (pos >= filled)
        return last_error = fill_buffer();

    return last_error = read_result_ok;
}

inline static char next_char() {
    pos++;
    return get();
}

inline static read_result_t skip_whitespaces() {
    while (true) {
        if (!is_whitespace(get()))
            return read_result_ok;

        if (next() != read_result_ok)
            return last_error;
    };

    return read_result_ok;
}

read_result_t read_start(int fd) {
    current_fd = fd;
    return fill_buffer();
}

read_result_t read_next_uint32(uint32_t *out) {
    if (skip_whitespaces() != read_result_ok)
        return last_error;

    unsigned int res = 0;
    while (true) {
        if (!is_number(get())) {
            *out = res;
            return read_result_ok;
        }
        res *= 10;
        res += buffer[pos] - '0';

        if (next() != read_result_ok) {
            if (last_error == read_result_eof) {
                *out = res;
                return read_result_ok;
            }
            return last_error;
        }
    }
}

read_result_t read_next_uint64(uint64_t *out) {
    if (skip_whitespaces() != read_result_ok)
        return last_error;

    uint64_t res = 0;
    while (true) {
        if (!is_number(get())) {
            *out = res;
            return read_result_ok;
        }
        res *= 10;
        res += buffer[pos] - '0';

        if (next() != read_result_ok) {
            if (last_error == read_result_eof) {
                *out = res;
                return read_result_ok;
            }
            return last_error;
        }
    }
}

read_result_t read_next_int64(int64_t *out) {
    if (skip_whitespaces() != read_result_ok)
        return last_error;

    uint64_t res = 0;
    bool neg = (get() == '-');

    while (true) {
        if (!is_number(get())) {
            *out = res;
            return read_result_ok;
        }
        res *= 10;
        res += buffer[pos] - '0';

        if (next() != read_result_ok) {
            if (last_error == read_result_eof) {
                *out = res;
                return read_result_ok;
            }
            return last_error;
        }
    }
}

read_result_t next_line() {
    while (true) {
        if (get() == '\n')
            return next();
        if (next() != read_result_ok)
            return last_error;
    }
}

read_result_t read_next_string(char *dst, int n) {
    if (skip_whitespaces() != read_result_ok)
        return last_error;

    if (n == 0)
        return read_result_error;
    n--;

    while (true) {
        if (is_whitespace(get()) || !(n--)) {
            *dst = 0;
            return read_result_ok;
        }
        *dst++ = buffer[pos];
        if (next() != read_result_ok)
            return last_error;
    }
}

read_result_t skip_next() {
    if (skip_whitespaces() != read_result_ok)
        return last_error;

    while (true) {
        if (is_whitespace(get()))
            return read_result_ok;
        if (next() != read_result_ok)
            return last_error;
    }
}

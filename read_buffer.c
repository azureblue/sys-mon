#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "read_buffer.h"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define BUFFER_SIZE 128

static char buffer[BUFFER_SIZE];
static int pos;
static int filled;
static int current_fd = -1;

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

char peek_next_char() {
    if (pos >= filled) {
        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok)
            return fill_result;
    }
    return buffer[pos];
}

inline static read_result_t skip_whitespaces() {
    while (true) {
        for (; pos < filled; pos++) {
            if (!is_whitespace(buffer[pos]))
                return read_result_ok;
        }

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok)
            return fill_result;
    };
}

read_result_t read_start(int fd) {
    current_fd = fd;
    return fill_buffer();
}

read_result_t read_next_uint(unsigned int *out) {
    read_result_t res_code =  skip_whitespaces();
    if (res_code != read_result_ok)
        return res_code;

    unsigned int res = 0;
    while (true) {
        for (; pos < filled; pos++) {
            if (!is_number(buffer[pos])) {
                *out = res;
                return read_result_ok;
            }
            res *= 10;
            res += buffer[pos] - '0';
        }

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok) {
            if (fill_result == read_result_eof) {
                *out = res;
                return read_result_ok;
            }

            return fill_result;
        }
    }
}

read_result_t read_next_int64(uint64_t *out) {
    read_result_t res_code =  skip_whitespaces();
    if (res_code != read_result_ok)
        return res_code;

    int64_t res = 0;
    bool neg = (peek_next_char() == '-');

    while (true) {
        for (; pos < filled; pos++) {
            if (!is_number(buffer[pos])) {
                *out = neg ? res : -res;
                return read_result_ok;
            }
            res *= 10;
            res -= buffer[pos] - '0';
        }

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok) {
            if (fill_result == read_result_eof){
                *out = neg ? res : -res;
                return read_result_ok;
            }

            return fill_result;
        }
    }
}

read_result_t next_line() {
    while(true) {
          while (pos < filled)
            if (buffer[pos++] == '\n')
                return read_result_ok;

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok)
            return fill_result;
    }
}

read_result_t read_next_string(char *dst, int n) {
    read_result_t res_code = skip_whitespaces();

   if (res_code != read_result_ok)
        return res_code;

    if (n == 0)
        return read_result_error;
    n--;

    while (true) {
        for (; pos < filled; pos++) {
            if (is_whitespace(buffer[pos]) || !(n--)) {
                *dst = 0;
                return read_result_ok;
            }
            *dst++ = buffer[pos];
        }

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok) {
            if (fill_result == read_result_eof) {
                *dst = 0;
                return read_result_ok;
            }
            return fill_result;
        }
    }
}

read_result_t skip_next() {
   read_result_t res_code = skip_whitespaces();
   if (res_code != read_result_ok)
        return res_code;

    while (true) {
        while (pos < filled)
            if (is_whitespace(buffer[pos++]))
                return read_result_ok;

        read_result_t fill_result = fill_buffer();
        if (fill_result != read_result_ok)
            return fill_result;
    }
}

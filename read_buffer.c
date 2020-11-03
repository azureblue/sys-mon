#include <stdbool.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "read_buffer.h"

char read_buffer[BUFFER_SIZE];

static int read_buffer_pos = 0;

int read_to_buffer(int fd, int size) {
    if (size > BUFFER_SIZE)
        size = BUFFER_SIZE;
    int n = 0, off = 0;
    while ((n = read(fd, read_buffer + off, size - off - 1))) {
        if (n < 0) {
           return -1;
        }
        if (n == 0)
            break;
        off += n;
        if (off == size - 1)
            break;
    }
    read_buffer[off] = 0;
    read_buffer_pos = 0;
    return off;
}

inline static int is_number(char c) {
    return c >= '0' && c <= '9';
}

inline static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int next_uint() {
    int res = 0;
    while (read_buffer_pos < BUFFER_SIZE) {
        if (is_number(read_buffer[read_buffer_pos]))
            break;
        read_buffer_pos++;
    }
    while (read_buffer_pos < BUFFER_SIZE) {
        if (!is_number(read_buffer[read_buffer_pos]))
            break;
        res *= 10;
        res += read_buffer[read_buffer_pos] - '0';
        read_buffer_pos++;
    }
    return res;
}

int skip_int(int n) {
    while (n--) {
        while (read_buffer_pos < BUFFER_SIZE) {
            if (is_number(read_buffer[read_buffer_pos]))
                break;
            read_buffer_pos++;
        }
        while (read_buffer_pos < BUFFER_SIZE) {
            if (!is_number(read_buffer[read_buffer_pos]))
                break;
            read_buffer_pos++;
        }
    }
}

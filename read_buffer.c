#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define BUFFER_SIZE 128

static char buffer[BUFFER_SIZE];
static int pos;
static int filled;
static int last_fd = -1;

int fill_buffer(int fd) {
    int off = 0, n;
    while ((n = read(fd, buffer + off, BUFFER_SIZE - off))) {
        if (unlikely(n < 0))
            return -1;
        off += n;
    }
    pos = 0;
    filled = off;
    buffer[filled] = 0;
    return off;
}


inline static int is_number(char c) {
    return c >= '0' && c <= '9';
}

inline static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int read_init(int fd) {
    if (fill_buffer(fd) == -1)
        return -1;
    return 0;
}

int read_next_uint(int fd, unsigned int *out) {
    unsigned int res = 0;
    do {
        for (; pos < filled; pos++) {
            if (is_number(buffer[pos]))
                break;
        }

        if (pos < filled)
            break;
    } while (fill_buffer(fd) > 0);

    if (!filled)
        return -1;

    do {
        for (; pos < filled; pos++) {
            if (!is_number(buffer[pos]))
                break;
            res *= 10;
            res += buffer[pos] - '0';
        }
        if (pos < filled)
            break;
    } while (fill_buffer(fd) > 0);

    *out = res;
    return 0;
}

int next_line(int fd) {
    do {
        for (; pos < filled; pos++) {
            if (buffer[pos] == '\n')
                break;
        }

        if (pos < filled)
            break;
    } while (fill_buffer(fd) > 0);

    if (!filled)
        return -1;

    pos++;
    return 0;
}

int skip_next(int fd) {
    do {
        for (; pos < filled; pos++) {
            if (!is_whitespace(buffer[pos]))
                break;
        }

        if (pos < filled)
            break;
    } while (fill_buffer(fd) > 0);

    if (!filled)
        return -1;

    do {
        for (; pos < filled; pos++) {
            if (is_whitespace(buffer[pos]))
                break;
        }
        if (pos < filled)
            break;
    } while (fill_buffer(fd) > 0);

    if (!filled)
        return -1;

    pos++;
    return 0;
}

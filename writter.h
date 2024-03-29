#ifndef WRITTER_H
#define WRITTER_H

#include <inttypes.h>

struct writter_t {
    char *buffer;
    int pos;
    int len;
};

typedef struct writter_t writter_t;

int writter_get_size(writter_t *wr);

int write_string(writter_t *wr, const char *str);
int write_char(writter_t *wr, char c);
int write_uint(writter_t *wr, unsigned int i);
int write_int64(writter_t *wr, int64_t i);

#endif /* WRITTER_H */

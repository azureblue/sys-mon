#ifndef WRITTER_H
#define WRITTER_H
struct writter_t {
    char *buffer;
    int pos;
    int len;
};

typedef struct writter_t writter_t;


int writter_get_size(writter_t *wr);
void *close_writter(writter_t *wr);

writter_t *create_writter(char *buf, int len);
int write_string(writter_t *wr, const char *str);
int write_char(writter_t *wr, char c);
int write_uint(writter_t *wr, unsigned int i);

#endif

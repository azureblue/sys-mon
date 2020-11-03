#include <stddef.h>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 512
#endif

extern char read_buffer[];

int read_to_buffer(int fd, int size);
int next_uint();
int skip_int(int n);


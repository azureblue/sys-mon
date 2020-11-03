#include<semaphore.h>

#define SHARED_SIZE 512
struct shared_buf {
    char data[SHARED_SIZE];
    sem_t in;
    sem_t out;
};

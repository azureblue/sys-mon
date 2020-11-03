#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../shared_buf.h"

int main() {
    int shm_fd = shm_open("/sys-mon", O_RDWR, 0);
    if (shm_fd == -1) {
        perror("opening shm [sys-mon] failed");
        exit(-1);
    }

    struct shared_buf* sh_buf = mmap(NULL, sizeof(struct shared_buf),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (sh_buf == MAP_FAILED) {
        perror("shm map failed");
        exit(-1);
    }

    sem_post(&sh_buf->in);
    sem_wait(&sh_buf->out);
    printf("%s", sh_buf->data);
    munmap(sh_buf, sizeof(struct shared_buf));
    close(shm_fd);
    return 0;
}

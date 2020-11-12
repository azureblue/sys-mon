#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "client.h"
#include "../shared_buf.h"


static const int cleanup_rp;

struct sys_mon_handle {
    int shm_fd;
    shared_buf_t* sh_buf;
};


sys_mon_handle_t* sys_mon_open(const char* name) {
    sys_mon_handle_t* handle = malloc(sizeof(sys_mon_handle_t));
    int shm_fd = shm_open(name, O_RDWR, 0);
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

    handle->sh_buf = sh_buf;
    handle->shm_fd = shm_fd;
    return handle;
}

int sys_mon_read_data(sys_mon_handle_t *handle, char* buffer, int buffer_size) {
    const int sync = handle->sh_buf->sync_method;
    switch (sync) {
        case SYS_MON_SYNC_IP_WO_R:
            sem_post(&handle->sh_buf->in);
            sem_wait(&handle->sh_buf->out);
            strncpy(buffer, handle->sh_buf->data, buffer_size - 1);
            buffer[buffer_size - 1] = 0;
            break;
        case SYS_MON_SYNC_WP_R_OP:
            sem_wait(&handle->sh_buf->in);
            strncpy(buffer, handle->sh_buf->data, buffer_size - 1);
            sem_post(&handle->sh_buf->in);
            buffer[buffer_size - 1] = 0;
        break;
        default:
            fprintf(stderr, "unknown sync method\n");
            exit(-1);
    }
}

void sys_mon_close(sys_mon_handle_t *handle) {
    munmap(handle->sh_buf, sizeof(shared_buf_t));
    close(handle->shm_fd);
}

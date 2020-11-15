#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include "client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

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
        perror("opening shm sys-mon failed");
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

int sys_mon_read_data(sys_mon_handle_t* handle, char* buffer, int buffer_size) {
    const int sync = handle->sh_buf->sync_method;
    switch (sync) {
        case SYS_MON_SYNC_IP_WO_R: {
            int lock_res = pthread_mutex_lock(&handle->sh_buf->mutex);
            if (lock_res == EOWNERDEAD)
                pthread_mutex_consistent(&handle->sh_buf->mutex);
            int res;
            sem_getvalue(&handle->sh_buf->out, &res);
            while (res-- > 0)
                sem_wait(&handle->sh_buf->out);
            sem_post(&handle->sh_buf->in);
            sem_wait(&handle->sh_buf->out);
            strncpy(buffer, handle->sh_buf->data, buffer_size - 1);
            buffer[buffer_size - 1] = 0;
            pthread_mutex_unlock(&handle->sh_buf->mutex);
            break;
        }
        case SYS_MON_SYNC_WP_R_OP: {
            int lock_res = pthread_mutex_lock(&handle->sh_buf->mutex);
            if (lock_res == EOWNERDEAD)
                pthread_mutex_consistent(&handle->sh_buf->mutex);
            strncpy(buffer, handle->sh_buf->data, buffer_size - 1);
            pthread_mutex_unlock(&handle->sh_buf->mutex);
            buffer[buffer_size - 1] = 0;
            break;
        }
        default:
            fprintf(stderr, "unknown sync method\n");
            exit(-1);
    }
}

void sys_mon_close(sys_mon_handle_t* handle) {
    munmap(handle->sh_buf, sizeof(shared_buf_t));
    close(handle->shm_fd);
}

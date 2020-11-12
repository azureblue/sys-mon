#include <pthread.h>
#include <semaphore.h>

enum sys_mon_sync_method {
    SYS_MON_SYNC_IP_WO_R,
    SYS_MON_SYNC_WP_R_OP
};

typedef enum sys_mon_sync_method sys_mon_sync_method_t;

#define SHARED_SIZE 512

struct shared_buf {
    char data[SHARED_SIZE];
    pthread_mutex_t mutex;
    sem_t in;
    sem_t out;
    sys_mon_sync_method_t sync_method;
};

typedef struct shared_buf shared_buf_t;

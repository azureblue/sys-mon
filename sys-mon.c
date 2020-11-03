#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "read_buffer.h"
#include "writter.h"
#include "module.h"
#include "shared_buf.h"

#define MODS 8

struct module_config modules[MODS];

void init_modules() {
    modules[0] = module_init_cpu(NULL);
    modules[1] = module_init_ram(NULL);
    modules[2] = module_init_generic("/sys/class/hwmon/hwmon0/temp2_input 1000");
    modules[3] = module_init_generic("/sys/class/hwmon/hwmon0/temp3_input 1000");
    modules[4] = module_init_generic("/sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq 1000");
    modules[5] = module_init_generic("/sys/bus/cpu/devices/cpu1/cpufreq/cpuinfo_cur_freq 1000");
    modules[6] = module_init_disk("sda");
    modules[7] = module_init_disk("sdb");
}

static const char* shm_name = "/sys-mon";
static int shm_fd;

static void handle_signal(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            shm_unlink(shm_name);
            close(shm_fd);
            exit(0);
            break;
    }
}

int main() {
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    init_modules();
    mode_t old_umask = umask(0);
    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    umask(old_umask);

    if (shm_fd == -1) {
        perror("shm create failed");
        exit(-1);
    }

    if (ftruncate(shm_fd, sizeof(struct shared_buf)) != 0) {
        perror("ftruncate failed");
        exit(-1);
    }
    struct shared_buf* sh_buf = mmap(NULL, sizeof(struct shared_buf),
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (sh_buf == MAP_FAILED) {
        perror("map failed");
        exit(-1);
    }

    sem_init(&sh_buf->in, 1, 0);
    sem_init(&sh_buf->out, 1, 0);

    writter_t wr = {.buffer = sh_buf->data, .len = SHARED_SIZE, .pos = 0};
    while (true) {
        sem_wait(&sh_buf->in);
        wr.pos = 0;
        for (int i = 0; i < MODS; i++) {
            int written = modules[i].write_data(modules[i].data, &wr);
            write_char(&wr, '\n');
        }
        write_char(&wr, 0);
        sem_post(&sh_buf->out);
    }
}

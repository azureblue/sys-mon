#include <dlfcn.h>
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

#define MAX_CONF_LINE_SIZE 256
#ifndef MAX_MODULES
#define MAX_MODULES 32
#endif
module_config modules[MAX_MODULES];
static int modules_n = 0;

void init_modules(const char* config_path) {
    memptr_t dl = dlopen(NULL, RTLD_LAZY);
    FILE * conf = fopen(config_path, "r");
    char line[MAX_CONF_LINE_SIZE + 1];
    while (fgets(line, MAX_CONF_LINE_SIZE + 1, conf) != NULL) {
        if (modules_n == MAX_MODULES) {
            fprintf(stderr, "too many modules in config\n");
            exit(-1);
        }
        if (strlen(line) > MAX_CONF_LINE_SIZE) {
            fprintf(stderr, "invalid conf file, line too long\n");
            exit(-1);
        }
        char module_name[32];
        if (sscanf(line, "%31s", module_name) != 1) {
            fprintf(stderr, "invalid config line: %d\n", modules_n + 1);
            exit(-1);
        }
        char module_init_name[50] = "module_init_";
        strcat(module_init_name, module_name);
        module_init init = dlsym(dl, module_init_name);
        if (init == NULL) {
            fprintf(stderr, "can't load module %s: %s\n", module_name, dlerror());
            exit(-1);
        }
        modules[modules_n] = init(line + strlen(module_name));
        modules_n++;
    }
    fclose(conf);
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

int main(int argc, char *argv[]) {
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    if (argc < 2) {
            fprintf(stderr, "missing file path\n");
            exit(-1);
    }
    init_modules(argv[1]);
    mode_t old_umask = umask(0);
    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
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
        for (int i = 0; i < modules_n; i++) {
            modules[i].write_data(modules[i].data, &wr);
            write_char(&wr, '\n');
        }
        if (write_char(&wr, 0) == -1) {
            fprintf(stderr, "not enough shm space\n");
            raise(SIGTERM);
        }

        sem_post(&sh_buf->out);
    }
}

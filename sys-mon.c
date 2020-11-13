#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "error.h"
#include "module.h"
#include "read_buffer.h"
#include "shared_buf.h"
#include "string_utils.h"
#include "writter.h"

#define MAX_CONF_LINE_SIZE 256
#ifndef MAX_MODULES
#define MAX_MODULES 32
#endif

static timer_t sys_mon_timer;
static module_config_t modules[MAX_MODULES];
static int modules_n = 0;
static struct shared_buf* sh_buf;
writter_t wr;

struct module_config_spec {
    const char* module_name;
    const char* module_args;
};

typedef struct module_config_spec module_config_spec_t;

struct sys_mon_config {
    bool auto_update;
    int update_ms;
    const char* shm_name;
};

typedef struct sys_mon_config sys_mon_config_t;

char shm_name_buff[32] = "sys-mon";

sys_mon_config_t sys_mon_config = {
    .auto_update = false,
    .shm_name = shm_name_buff};

void section_config(FILE* conf, const char* section_line);

void parse_modules_config(FILE* conf) {
    memptr_t dl = dlopen(NULL, RTLD_LAZY);
    char line[MAX_CONF_LINE_SIZE + 1];
    while (fgets(line, MAX_CONF_LINE_SIZE + 1, conf) != NULL) {
        if (string_is_empty(line))
            continue;
        if (line[0] == '[') {
            section_config(conf, line);
            return;
        }

        if (modules_n == MAX_MODULES)
            exit_with_error("too many modules in config");

        if (strlen(line) > MAX_CONF_LINE_SIZE)
            exit_with_error("invalid conf file, line too long");

        char module_name[32];
        if (sscanf(line, "%31s", module_name) != 1)
            exit_with_error("invalid config line: %d", modules_n + 1);

        char module_init_name[50] = "module_init_";
        strcat(module_init_name, module_name);

        module_init init = dlsym(dl, module_init_name);

        if (init == NULL)
            exit_with_error("can't load module %s: %s", module_name, dlerror());

        modules[modules_n] = init(line + strlen(module_name));
        modules_n++;
    }
    fclose(conf);
}

void parse_sys_mon_config(FILE* conf) {
    char line[MAX_CONF_LINE_SIZE + 1];
    while (fgets(line, MAX_CONF_LINE_SIZE + 1, conf) != NULL) {
        if (string_is_empty(line))
            continue;
        if (line[0] == '[') {
            section_config(conf, line);
            return;
        }
        char* asign = strchr(line, '=');
        if (!asign)
            exit_with_error("invalid conf line:%s\n", line);
        *asign = ' ';

        char param[16], value[128];

        if (sscanf(line, "%16s %128s", param, value) != 2)
            exit_with_error("invalid conf line:%s\n", line);

        if (!strcmp(param, "auto_update")) {
            if (!strcmp(value, "true"))
                sys_mon_config.auto_update = true;
            else if (!strcmp(value, "false"))
                sys_mon_config.auto_update = false;
            else
                exit_with_error("invalid conf line:%s\n", line);
        } else if (!strcmp(param, "update_ms")) {
            sys_mon_config.update_ms = atoi(value);
        } else if (!strcmp(param, "name")) {
            if (strlen(value) > sizeof(shm_name_buff) - 1)
                exit_with_error("invalid conf line:%s\n", line);
            strcpy(shm_name_buff, value);
        } else {
            exit_with_error("invalid conf line: %s\n", line);
        }
    }
}

void section_config(FILE* conf, const char* section_line) {
    static const int max_section_name_len = 15;
    const char* start = strchr(section_line, '[');
    const char* end = strchr(section_line, ']');
    if (start == NULL || end == NULL)
        exit_with_error("invalid line: %s", section_line);

    start++;
    int name_len = end - start;
    if (name_len > max_section_name_len)
        exit_with_error("invalid line: %s", section_line);

    char section_name[16];
    memcpy(section_name, start, name_len);
    section_name[name_len] = 0;

    if (!strcmp(section_name, "sys-mon")) {
        parse_sys_mon_config(conf);
        return;
    } else if (!strcmp(section_name, "modules")) {
        parse_modules_config(conf);
        return;
    } else
        exit_with_error("invalid line: %s", section_line);
}

static int shm_fd;

static void handle_signal(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            shm_unlink(sys_mon_config.shm_name);
            close(shm_fd);
            exit(0);
            break;
    }
}

void parse_config(const char* path) {
    FILE* conf = fopen(path, "r");
    if (!conf)
        exit_with_error("unable to open config");
    parse_sys_mon_config(conf);
}

void update() {
    wr.pos = 0;
    for (int i = 0; i < modules_n; i++) {
        modules[i].write_data(modules[i].data, &wr);
        write_char(&wr, '\n');
    }
    if (write_char(&wr, 0) == -1) {
        fprintf(stderr, "not enough shm space\n");
        raise(SIGTERM);
    }
}

int main(int argc, char* args[]) {
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    char *config_path = argc < 2 ? "./default.conf" : args[1];
    parse_config(config_path);

    mode_t old_umask = umask(0);
    int shm_fd = shm_open(sys_mon_config.shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IROTH);
    umask(old_umask);

    if (shm_fd == -1) {
        perror("shm create failed");
        exit(-1);
    }

    if (ftruncate(shm_fd, sizeof(struct shared_buf)) != 0) {
        perror("ftruncate failed");
        exit(-1);
    }
    sh_buf = mmap(NULL, sizeof(struct shared_buf),
                  PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (sh_buf == MAP_FAILED) {
        perror("map failed");
        exit(-1);
    }

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&sh_buf->mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);
    // pthread_condattr_t cattr;
    // pthread_condattr_init(&cattr);
    // pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    // pthread_cond_init(&sh_buf->cond, &cattr);
    // pthread_condattr_destroy(&cattr);

    wr = (writter_t){.buffer = sh_buf->data, .len = SHARED_SIZE, .pos = 0};

    if (sys_mon_config.auto_update == false) {
        sh_buf->sync_method = SYS_MON_SYNC_IP_WO_R;

        sem_init(&sh_buf->in, 1, 0);
        sem_init(&sh_buf->out, 1, 0);

        while (true) {
            sem_wait(&sh_buf->in);
            update();
            sem_post(&sh_buf->out);
        }
    } else {
        sh_buf->sync_method = SYS_MON_SYNC_WP_R_OP;
        sem_init(&sh_buf->in, 1, 1);
        timer_create(CLOCK_MONOTONIC, NULL, &sys_mon_timer);

        long nanos = (sys_mon_config.update_ms % 1000) * 1000 * 1000;
        struct itimerspec time_spec = {
            .it_interval = {.tv_sec = sys_mon_config.update_ms / 1000, .tv_nsec = nanos},
            .it_value = {.tv_sec = sys_mon_config.update_ms / 1000, .tv_nsec = nanos}};

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigprocmask(SIG_BLOCK, &sigset, NULL);

        timer_settime(sys_mon_timer, 0, &time_spec, NULL);

        while (true) {
            int sig;
            sigwait(&sigset, &sig);
            int lock_res = pthread_mutex_lock(&sh_buf->mutex);
            if (lock_res == EOWNERDEAD)
                pthread_mutex_consistent(&sh_buf->mutex);
            update();
            pthread_mutex_unlock(&sh_buf->mutex);
        }
    }
}

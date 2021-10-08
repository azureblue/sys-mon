#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GENMON_STR_SIZE 1024

struct sys_mon_pango;

int main() {
    char buf[GENMON_STR_SIZE];
    void *dl = dlopen("/home/konrad/sys-mon/genmon/sys-mon-pango.so", RTLD_LAZY);

    int (*sys_mon_plugin_write_pango_string)(struct sys_mon_pango *handle, char *buf, int len) = dlsym(dl, "sys_mon_plugin_write_pango_string");
    struct sys_mon_pango *(*sys_mon_pango_init)()  = dlsym(dl, "sys_mon_pango_init");
    void (*sys_mon_pango_close)(struct sys_mon_pango *handle) = dlsym(dl, "sys_mon_pango_close");

    struct sys_mon_pango *handle = sys_mon_pango_init();
    if (sys_mon_plugin_write_pango_string(handle, buf, GENMON_STR_SIZE) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
        printf("string writting error. probably buffer length not enough");
        exit(-1);
    }
    sys_mon_pango_close(handle);
    printf("<txt>%s</txt>\n", buf);
    return 0;
}

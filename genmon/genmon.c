#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GENMON_STR_SIZE 1024

struct system_info_data;

int main() {
    char buf[GENMON_STR_SIZE];
    void *dl = dlopen("/home/konrad/sys-mon/genmon/sys-mon-pango.so", RTLD_LAZY);

    int (*sys_mon_plugin_write_pango_string)(struct system_info_data *handle, char *buf, int len) = dlsym(dl, "sys_mon_plugin_write_pango_string");
    struct system_info_data *(*sys_mon_pango_init)()  = dlsym(dl, "sys_mon_pango_init");
    void (*sys_mon_pango_close)(struct system_info_data *handle) = dlsym(dl, "sys_mon_pango_close");

    struct system_info_data *handle = sys_mon_pango_init();
    if (sys_mon_plugin_write_pango_string(handle, buf, GENMON_STR_SIZE) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
        printf("string writting error. probably buffer length not enough");
        exit(-1);
    }
    sys_mon_pango_close(handle);
    printf("<txt>%s</txt>\n", buf);

    dlclose(dl);
    dl = dlopen("/home/konrad/sys-mon/genmon/sys-mon-pango-tooltip.so", RTLD_LAZY);
    static char buf2[2048] = {0};
    int (*sys_mon_pango_tooltip_print)(char *, int) = dlsym(dl, "sys_mon_plugin_write_pango_tooltip_string");
    sys_mon_pango_tooltip_print(buf2, 2048);
    printf("%s", buf2);
    return 0;
}

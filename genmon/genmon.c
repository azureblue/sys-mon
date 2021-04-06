#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sys-mon-pango.h"

#define GENMON_STR_SIZE 1024

int main() {
    char buf[GENMON_STR_SIZE];
    if (sys_mon_plugin_write_pango_string(buf, GENMON_STR_SIZE) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
        printf("string writting error. probably buffer length not enough");
        exit(-1);
    }
    printf("<txt>%s</txt>\n", buf);
    return 0;
}

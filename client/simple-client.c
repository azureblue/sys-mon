#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "client.h"

int main() {
    char buffer[512];
    sys_mon_handle_t *sys_mon = sys_mon_open("sys-mon-0");

    sys_mon_read_data(sys_mon, buffer, 512);
    printf("%s", buffer);
    sys_mon_close(sys_mon);

    return 0;
}

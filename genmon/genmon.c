#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../read_buffer.h"
#include "../writter.h"
#include "../shared_buf.h"

#define GENMON_STR_SIZE 1024

const char* progress_bar_blocks[] = {"\xE2\x96\x81", "\xE2\x96\x82", "\xE2\x96\x83", "\xE2\x96\x84", "\xE2\x96\x85", "\xE2\x96\x86", "\xE2\x96\x87", "\xE2\x96\x87"};
const int progress_bar_blocks_len = 8;

char genmon_str[GENMON_STR_SIZE];
writter_t wr = {.buffer = genmon_str, .pos = 0, .len = GENMON_STR_SIZE};


const char* bar_symbol(unsigned int max, unsigned int value) {
    if (value > max)
        value = max;
    int n = progress_bar_blocks_len - 1;
    int idx = (value * n + (max / n / 2)) / max;
    if (idx == 0 && value > 0)
         idx = 1;
    return progress_bar_blocks[idx];
}

void append_colored_text( const char * text, const char *color) {
    write_string(&wr, "<span fgcolor=\"");
    write_string(&wr, color);
    write_string(&wr, "\">");
    write_string(&wr, text);
    write_string(&wr, "</span>");
}

int main() {
    int shm_fd = shm_open("/sys-mon", O_RDWR, 0);
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

    sem_post(&sh_buf->in);
    sem_wait(&sh_buf->out);
    read_to_buffer(shm_fd, 128);
    munmap(sh_buf, sizeof(struct shared_buf));
    close(shm_fd);

    int cpu_usage = read_next_uint();
    int mem_total = read_next_uint();
    int mem_mb = read_next_uint();
    int t1 = read_next_uint();
    int t2 = read_next_uint();
    int freq1 = read_next_uint();
    int freq2 = read_next_uint();
    int sda_rb = read_next_uint() * 512;
    int sda_wb = read_next_uint() * 512;
    int sdb_rb = read_next_uint() * 512;
    int sdb_wb = read_next_uint() * 512;

    int max_temp = t1 > t2 ? t1 : t2;

    if (mem_mb < 200) {
        system("pkill chrom");
        system("pkill firefox");
        system("./beep");
    } else if (mem_mb < 500) {
        system("./beep");
    }

    char disk_usage_str[256];
    disk_usage_str[0] = 0;

    write_string(&wr, "<txt>");
    write_uint(&wr, max_temp);
    write_string(&wr, "°C");

    append_colored_text(bar_symbol(1000, freq1 - 2000), "#8b64b3");
    append_colored_text(bar_symbol(1000, freq2 - 2000), "#8b64b3");

    if (cpu_usage == 100)
        write_string(&wr, "##");
    else {
        if (cpu_usage < 10)
            write_char(&wr, ' ');
        write_uint(&wr, cpu_usage);
    }
    write_string(&wr, "%|");
    write_uint(&wr, mem_mb / 1000);
    write_char(&wr, '.');
    write_uint(&wr, (mem_mb % 1000) / 100);
    write_char(&wr, 'G');
    append_colored_text(bar_symbol(5000000, sda_rb), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sda_wb), "#c671eb");
    append_colored_text(bar_symbol(5000000, sdb_rb), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sdb_wb), "#c671eb");
    write_string(&wr, "</txt>");
    if (write_char(&wr, 0) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
    }
    puts(genmon_str);
}

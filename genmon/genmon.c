#include <stdbool.h>
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

const char* progress_bar_blocks[] = {" ", "\xE2\x96\x81", "\xE2\x96\x82", "\xE2\x96\x83", "\xE2\x96\x84", "\xE2\x96\x85", "\xE2\x96\x86", "\xE2\x96\x87", "\xE2\x96\x87"};
const int progress_bar_blocks_len = 9;

char genmon_str[GENMON_STR_SIZE];
writter_t wr = {.buffer = genmon_str, .pos = 0, .len = GENMON_STR_SIZE};


const char* bar_symbol(unsigned int max, unsigned int value, bool show_nonzero_value) {
    if (value > max)
        value = max;
    int n = progress_bar_blocks_len;
    int idx = (value * n + (max / n / 2)) / max;
    if (idx == 0 && value > 0 && show_nonzero_value)
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
    munmap(sh_buf, sizeof(struct shared_buf));


    unsigned int cpu_usage, core_0, core_1, mem_total, mem_mb, t1, t2, freq_0, freq_1, sda_rb, sda_wb, sdb_rb, sdb_wb;
    read_next_uint(shm_fd, &cpu_usage);
    read_next_uint(shm_fd, &core_0);
    read_next_uint(shm_fd, &core_1);
    //next_line(shm_fd);
    read_next_uint(shm_fd, &mem_total);
    read_next_uint(shm_fd, &mem_mb);
    read_next_uint(shm_fd, &t1);
    read_next_uint(shm_fd, &t2);
    read_next_uint(shm_fd, &freq_0);
    read_next_uint(shm_fd, &freq_1);
    read_next_uint(shm_fd, &sda_rb);
    read_next_uint(shm_fd, &sda_wb);
    read_next_uint(shm_fd, &sdb_rb);
    read_next_uint(shm_fd, &sdb_wb);
    close(shm_fd);
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

    freq_0 /= 100;
    freq_1 /= 100;

    char * freq_colors[] = {"#4355fa", "#8241f2", "#c92cc4", "#f53387"};

    char * core_0_color = freq_colors[0], * core_1_color = freq_colors[0];


    if (freq_0 == 23)
        core_0_color = freq_colors[1];
    else if (freq_0 == 26)
         core_0_color = freq_colors[2];
    else if (freq_0 == 30)
         core_0_color = freq_colors[3];

    if (freq_1 == 23)
        core_1_color = freq_colors[1];
    else if (freq_1 == 26)
         core_1_color = freq_colors[2];
    else if (freq_1 == 30)
         core_1_color = freq_colors[3];

     append_colored_text(bar_symbol(100, core_0, false), core_0_color);
     append_colored_text(bar_symbol(100, core_1, false), core_1_color);
    // append_colored_text(bar_symbol(1000, freq_0 - 2000), "#8b64b3");
    // append_colored_text(bar_symbol(1000, freq_1 - 2000), "#8b64b3");

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
    append_colored_text(bar_symbol(5000000, sda_rb, true), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sda_wb, true), "#c671eb");
    append_colored_text(bar_symbol(5000000, sdb_rb, true), "#8b64b3");
    append_colored_text(bar_symbol(5000000, sdb_wb, true), "#c671eb");
    write_string(&wr, "</txt>");
    if (write_char(&wr, 0) == -1) {
        fprintf(stderr, "string writting error. probably buffer length not enough");
    }
    puts(genmon_str);
}

#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "block-bars-utils.h"

const char *DF = "bash -c \"df --output=avail,pcent,source,target -h | grep -vP 'run|/dev$|cgroup' | sed -E '1d;s/^ //g;s/,/./g;s/  +/ /g;s/%//g;s/\\S*\\/([^\\/]+)/\\1/g;s/(sd.)[0-9]+/\\1/g'\"";
const char *NVIDIA_GPU_INFO = "bash -c \"nvidia-settings -t -q [gpu:0]/GPUUtilization -q [gpu:0]/TotalDedicatedGPUMemory -q [gpu:0]/UsedDedicatedGPUMemory -q [gpu:0]/GPUCurrentClockFreqs -q [gpu:0]/GPUCoreTemp | sed 's/\\S*=//g;s/,/ /g'\"";

static const char* color_ram = "#50b9ff";
static const char* color_gpu_usage = "#bf9cff";
static const char* color_temp = "#db7632";
static const char* color_freq[] = {"#2a72f7", "#a147f5", "#d94aae"};
static const char* color_sda = "#cccccc";
static const char* color_sdb = "#e187bf";
static const char* color_tmpfs = "#5d9ede";

__attribute__ ((visibility ("default")))
int sys_mon_plugin_write_pango_tooltip_string(char *buf, int len) {
    writter_t wr = (writter_t){.buffer = buf, .pos = 0, .len = len};
    write_string(&wr, "<span font_desc=\"Ubuntu Mono Condense d 12\">");
    FILE *fp = popen(NVIDIA_GPU_INFO, "r");
    if (fp) {
        int gpu_gpu, gpu_mem, gpu_video, gpu_pci, gpu_mem_total, gpu_mem_used, gpu_freq, gpu_mem_freq, gpu_temp;
        fscanf(fp, "%d%d%d%d%d%d%d%d%d",
            &gpu_gpu,
            &gpu_mem,
            &gpu_video,
            &gpu_pci,
            &gpu_mem_total,
            &gpu_mem_used,
            &gpu_freq,
            &gpu_mem_freq,
            &gpu_temp
            );

        pclose(fp);
        write_start_color(&wr, "#a19f9c");
        write_string(&wr, "gpu:\n");
        write_start_color(&wr, color_temp);
        write_uint(&wr, gpu_temp);
        write_string(&wr, "°C");
        write_section_end(&wr);
        write_char(&wr, ' ');
        write_start_color(&wr, color_gpu_usage);
        write_uint(&wr, gpu_gpu);
        write_char(&wr, '%');
        write_char(&wr, ' ');
        write_uint(&wr, gpu_pci);
        write_char(&wr, '%');
        write_section_end(&wr);

        write_char(&wr, ' ');

        write_start_color(&wr, color_ram);
        write_uint(&wr, gpu_mem_used);
        write_char(&wr, 'M');
        write_char(&wr, '/');
        write_uint(&wr, gpu_mem_total);
        write_char(&wr, 'M');
        write_section_end(&wr);
        write_char(&wr, '\n');
        write_start_color(&wr, color_freq[choose(3, 500, 1200, gpu_freq)]);
        write_uint(&wr,gpu_freq);
        write_string(&wr, " MHz");
        write_section_end(&wr);
        write_string(&wr, " | ");
        write_start_color(&wr, color_freq[choose(2, 400, 1000, gpu_mem_freq)]);
        write_uint(&wr,gpu_mem_freq);
        write_string(&wr, " MHz");
        write_section_end(&wr);

        write_bars_start(&wr);
        write_section_end(&wr);
        write_char(&wr, '\n');
        write_char(&wr, '\n');
    }
    fp = popen(DF, "r");
    if (fp) {
        write_string(&wr, "filesystems:\n");
        char tokens[3][32];
        int percentage;
            while(EOF != fscanf(fp, "%s%d%s%[^\n]", tokens[0], &percentage, tokens[1], tokens[2])) {
                if (!strcmp(tokens[1], "sda"))
                    write_start_color(&wr, color_sda);
                else if (!strcmp(tokens[1], "sdb"))
                    write_start_color(&wr, color_sdb);
                else if (!strcmp(tokens[1], "tmpfs"))
                    write_start_color(&wr, color_tmpfs);
                else
                    write_start_color(&wr, "#a19f9c");
                int firstLen = strlen(tokens[0]);
                while (firstLen++ < 4) {
                    write_char(&wr, ' ');
                }
                if(strstr(tokens[0], "."))
                    write_string(&wr, "¼");

                write_string(&wr, tokens[0]);
                write_string(&wr, " ");

                write_bars_start(&wr);
                write_string(&wr, pie_string(100, percentage, false));
                write_section_end(&wr);
                write_string(&wr, " ");
                // write_char(&wr, ' ');
                write_string(&wr, tokens[2] + 1);
                write_section_end(&wr);
                write_char(&wr, '\n');
        }

        pclose(fp);

    }
    if (wr.buffer[wr.pos - 1] == '\n')
        wr.pos--;
    write_section_end(&wr);
    write_section_end(&wr);
    write_char(&wr, 0);
}

int main() {
    char out[2048];
    sys_mon_plugin_write_pango_tooltip_string(out, 2048);
    puts(out);
    return 0;

}
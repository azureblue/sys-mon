#ifndef MY_SYSTEM_INFO_H
#define MY_SYSTEM_INFO_H

struct system_info_output {
    unsigned int cpu_usage;
    unsigned int core_usage[4];
    unsigned int core_freq[4];
    unsigned int free_mem;
    unsigned int temp;
    int sda_r_time;
    int sda_w_time;
    int sdb_r_time;
    int sdb_w_time;
    unsigned int update_time;
};

typedef struct system_info_output system_info_output_t;

#endif /* MY_SYSTEM_INFO_H */

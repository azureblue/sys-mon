#include <inttypes.h>

enum read_result {
    read_result_ok = 0,
    read_result_error = -1,
    read_result_eof = -2
};

typedef enum read_result read_result_t;

read_result_t read_start();
read_result_t read_next_uint32(uint32_t *out);
read_result_t read_next_uint64(uint64_t *out);
read_result_t read_next_int64(int64_t *out);
read_result_t next_line();
read_result_t skip_next();
read_result_t read_next_string(char *dst, int n);

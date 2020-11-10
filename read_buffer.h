
enum read_result {
    read_result_ok = 0,
    read_result_error = -1,
    read_result_eof = -2
};

typedef enum read_result read_result_t;

read_result_t read_init(int fd);
read_result_t read_next_uint(int fd, unsigned int *out);
read_result_t next_line(int fd);
read_result_t skip_next(int fd);

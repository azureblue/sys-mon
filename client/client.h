struct sys_mon_handle;

typedef struct sys_mon_handle sys_mon_handle_t;

sys_mon_handle_t* sys_mon_open(const char* name);
int sys_mon_read_data(sys_mon_handle_t *handle, char* buffer, int buffer_size);
void sys_mon_close(sys_mon_handle_t *handle);

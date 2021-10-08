#ifndef MODULE_H
#define MODULE_H
#include "writter.h"

#define MAX_CONF_LINE_SIZE 256

typedef void* memptr_t;

typedef memptr_t module_data;

typedef int (*module_write_data)(module_data data, writter_t *wt);
typedef void (*module_unload)(module_data data);

struct module_config {
    module_write_data write_data;
    module_unload unload;
    module_data data;
};

typedef struct module_config module_config_t;
typedef module_config_t (*module_init)(const char * module_args);

module_config_t sys_mon_load_module(const char * module_config_line);
int sys_mon_module_write_data(module_config_t * module, writter_t *wt);
void sys_mon_unload_module(module_config_t * module);

#endif /* MODULE_H */

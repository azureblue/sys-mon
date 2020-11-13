#ifndef MODULE_H
#define MODULE_H
#include "writter.h"

typedef void* memptr_t;

typedef memptr_t module_data;

typedef int (*module_write_data_callback)(module_data data, writter_t *wt);

struct module_config {
    module_write_data_callback write_data;
    module_data data;
};

typedef struct module_config module_config_t;
typedef module_config_t (*module_init)(const char * module_args);

#endif

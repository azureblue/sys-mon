#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "module.h"
#include "module_list.h"


module_config_t sys_mon_load_module(const char* module_config_line) {

    char module_name[32];
    if (sscanf(module_config_line, "%31s", module_name) != 1)
        exit_with_error("invalid config line: %s", module_config_line);

    char module_init_name[50] = "module_init_";
    strcat(module_init_name, module_name);

    module_init init;
    for (int i = 0;; i++) {
        if (MODULES[i].name == 0)
            exit_with_error("can't load module %s", module_name);

        if (!strcmp(module_name, MODULES[i].name)) {
            init = MODULES[i].init;
            break;
        }
    }

    return init(module_config_line + strlen(module_name));
}

int sys_mon_module_write_data(module_config_t * module, writter_t *wt) {
    return module->write_data(module->data, wt);
}

void sys_mon_unload_module(module_config_t * module) {
    module->unload(module->data);
}
#ifndef MODULE_LIST_H
#define MODULE_LIST_H

#include "module.h"
#define MODULE_INIT_NAME(module_name) sys_mon_module_init_##module_name
#define MODULE_INIT_DECL(module_name) module_config_t MODULE_INIT_NAME(module_name)(const char *args)

MODULE_INIT_DECL(cpu);
MODULE_INIT_DECL(ram);
MODULE_INIT_DECL(generic);
MODULE_INIT_DECL(time);
MODULE_INIT_DECL(disk_activity);

struct module_init_entry {
    const char * name;
    memptr_t init;
};

typedef struct module_init_entry module_init_entry_t;

module_init_entry_t MODULES[] = {
    {.name = "cpu", .init = MODULE_INIT_NAME(cpu)},
    {.name = "ram", .init = MODULE_INIT_NAME(ram)},
    {.name = "generic", .init = MODULE_INIT_NAME(generic)},
    {.name = "time", .init = MODULE_INIT_NAME(time)},
    {.name = "disk_activity", .init = MODULE_INIT_NAME(disk_activity)},
    {.name = 0}
};

#endif /* MODULE_LIST_H */

#ifndef SYS_MON_PANGO_H
#define SYS_MON_PANGO_H

struct sys_mon_pango;
typedef struct sys_mon_pango sys_mon_pango_t;

sys_mon_pango_t * sys_mon_pango_init();

void sys_mon_pango_close(sys_mon_pango_t *handle);

int sys_mon_plugin_write_pango_string(sys_mon_pango_t *handle, char *buf, int len);

#endif /* SYS_MON_PANGO_H */

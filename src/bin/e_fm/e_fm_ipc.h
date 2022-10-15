#ifndef E_FM_IPC_H
#define E_FM_IPC_H

#include "e_fm_main.h"
#include "e_fm_shared_types.h"

int _e_fm_ipc_init(void);
Eina_Bool _e_fm_ipc_slave_data_cb(void *data, int type, void *event);
Eina_Bool _e_fm_ipc_slave_error_cb(void *data, int type, void *event);
Eina_Bool _e_fm_ipc_slave_del_cb(void *data, int type, void *event);
void e_fm_ipc_volume_add(E_Volume *v);

E_API E_Storage *e_storage_add(const char *udi);
E_API void       e_storage_del(const char *udi);
E_API E_Storage *e_storage_find(const char *udi);

E_API E_Volume *e_volume_add(const char *udi, char first_time);
E_API void      e_volume_del(const char *udi);
E_API E_Volume *e_volume_find(const char *udi);

E_API void      e_volume_mount(E_Volume *v);
E_API void      e_volume_unmount(E_Volume *v);
E_API void      e_volume_eject(E_Volume *v);

#endif

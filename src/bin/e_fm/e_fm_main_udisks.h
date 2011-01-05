#ifndef E_FM_MAIN_UDISKS_H
#define E_FM_MAIN_UDISKS_H

#include <E_DBus.h>
#include <E_Ukit.h>

#include "e_fm_shared_types.h"

E_Volume *_e_fm_main_udisks_volume_add(const char *udi, Eina_Bool first_time);
void _e_fm_main_udisks_volume_del(const char *udi);
E_Volume *_e_fm_main_udisks_volume_find(const char *udi);

void _e_fm_main_udisks_volume_eject(E_Volume *v);
void _e_fm_main_udisks_volume_unmount(E_Volume *v);
void _e_fm_main_udisks_volume_mount(E_Volume *v);

E_Storage *_e_fm_main_udisks_storage_add(const char *udi);
void _e_fm_main_udisks_storage_del(const char *udi);
E_Storage *_e_fm_main_udisks_storage_find(const char *udi);


void _e_fm_main_udisks_init(void);
void _e_fm_main_udisks_shutdown(void);
void _e_fm_main_udisks_catch(Eina_Bool usable);

#endif

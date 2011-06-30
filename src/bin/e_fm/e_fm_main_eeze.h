#ifndef E_FM_MAIN_EEZE_H
#define E_FM_MAIN_EEZE_H

#include <Eeze.h>
#include <Eeze_Disk.h>

#include "e_fm_shared_types.h"

E_Volume *_e_fm_main_eeze_volume_add(const char *syspath, Eina_Bool first_time);
void _e_fm_main_eeze_volume_del(const char *syspath);
E_Volume *_e_fm_main_eeze_volume_find(const char *syspath);

void _e_fm_main_eeze_volume_eject(E_Volume *v);
void _e_fm_main_eeze_volume_unmount(E_Volume *v);
void _e_fm_main_eeze_volume_mount(E_Volume *v);

E_Storage *_e_fm_main_eeze_storage_add(const char *syspath);
void _e_fm_main_eeze_storage_del(const char *syspath);
E_Storage *_e_fm_main_eeze_storage_find(const char *syspath);


void _e_fm_main_eeze_init(void);
void _e_fm_main_eeze_shutdown(void);

#endif

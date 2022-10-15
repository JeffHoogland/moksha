#ifndef E_FM_MAIN_UDISKS2_H
#define E_FM_MAIN_UDISKS2_H

E_Volume *_e_fm_main_udisks2_volume_add(const char *udi, Eina_Bool first_time);
void _e_fm_main_udisks2_volume_del(const char *udi);
E_Volume *_e_fm_main_udisks2_volume_find(const char *udi);

void _e_fm_main_udisks2_volume_eject(E_Volume *v);
void _e_fm_main_udisks2_volume_unmount(E_Volume *v);
void _e_fm_main_udisks2_volume_mount(E_Volume *v);

E_Storage *_e_fm_main_udisks2_storage_add(const char *udi);
void _e_fm_main_udisks2_storage_del(const char *udi);
E_Storage *_e_fm_main_udisks2_storage_find(const char *udi);


void _e_fm_main_udisks2_init(void);
void _e_fm_main_udisks2_shutdown(void);
void _e_fm_main_udisks2_catch(Eina_Bool usable);

#endif

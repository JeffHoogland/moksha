/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_FM_HAL_H
#define E_FM_HAL_H

#include "e.h"
#include "e_fm.h"

EAPI void         e_fm2_hal_storage_add(E_Storage *s);
EAPI void         e_fm2_hal_storage_del(E_Storage *s);
EAPI E_Storage   *e_fm2_hal_storage_find(const char *udi);

EAPI void         e_fm2_hal_volume_add(E_Volume *s);
EAPI void         e_fm2_hal_volume_del(E_Volume *s);
EAPI E_Volume    *e_fm2_hal_volume_find(const char *udi);
EAPI char        *e_fm2_hal_volume_mountpoint_get(E_Volume *v);

EAPI void         e_fm2_hal_mount_add(E_Volume *v, const char *mountpoint);
EAPI void         e_fm2_hal_mount_del(E_Fm2_Mount *m);
EAPI E_Fm2_Mount *e_fm2_hal_mount_find(const char *path);
EAPI E_Fm2_Mount *e_fm2_hal_mount(E_Volume *v,
                                  void (*mount_ok) (void *data), void (*mount_fail) (void *data), 
				  void (*unmount_ok) (void *data), void (*unmount_fail) (void *data), 
				  void *data);
EAPI void         e_fm2_hal_unmount(E_Fm2_Mount *m);

EAPI void         e_fm2_hal_show_desktop_icons(void);
EAPI void         e_fm2_hal_hide_desktop_icons(void);

#endif

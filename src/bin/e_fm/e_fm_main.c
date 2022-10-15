#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#define HAVE_UDISKS_MOUNT 1
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#ifdef __linux__
#include <features.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <utime.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <glob.h>
#include <errno.h>
#include <signal.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>
#include <Ecore_File.h>
#include <Eet.h>

#include "e_fm_main.h"

#define E_TYPEDEFS
#include "e_config_data.h"
#include "e_fm_op.h"
#undef E_TYPEDEFS
#include "e_config_data.h"
#include "e_fm_op.h"

#include "e_fm_shared_device.h"
#ifdef HAVE_UDISKS_MOUNT
# include "e_fm_main_udisks.h"
# include "e_fm_main_udisks2.h"
#endif
#ifdef HAVE_EEZE_MOUNT
# include "e_fm_main_eeze.h"
#endif


/* if using udisks, functions will point to _e_fm_main_udisks_X
 * if using eeze, functions will point to _e_fm_main_eeze_X
 */

static Efm_Mode mode = EFM_MODE_USING_RASTER_MOUNT;

/* FIXME: things to add to the slave enlightenment_fm process and ipc to e:
 *
 * * reporting results of fop's (current status - what has been don, what failed etc.)
 * * dbus removable device monitoring (in e17 itself now via eldbus - move to enlightenment_fm and ipc removable device add/del and anything else)
 * * mount/umount of removable devices (to go along with removable device support - put it in here and message back mount success/failure and where it is now mounted - remove from e17 itself)
 *
 */

#ifndef E_API
#define E_API
#endif

#include "e_fm_main.h"
#include "e_fm_shared_types.h"
#include "e_fm_shared_codec.h"
#include "e_fm_ipc.h"

int efm_log_dom = -1;

static void
_e_fm_init(void)
{
# ifdef HAVE_UDISKS_MOUNT
   _e_fm_main_udisks2_init();
# else
#  ifdef HAVE_EEZE_MOUNT
   _e_fm_main_eeze_init();
   mode = EFM_MODE_USING_EEZE_MOUNT; /* this gets overridden if the others are available */
#  endif
# endif
}

static void
_e_fm_shutdown(void)
{
# ifdef HAVE_UDISKS_MOUNT
   _e_fm_main_udisks2_shutdown();
   _e_fm_main_udisks_shutdown();
# else
#  ifdef HAVE_EEZE_MOUNT
   _e_fm_main_eeze_shutdown();
#  endif
# endif
}

/* externally accessible functions */
int
main(int argc, char **argv)
{
   if (argc > 1)
     {
        printf(
        "This is an internal tool for Enlightenment.\n"
        "do not use it.\n"
        );
        exit(0);
     }

   eina_init();
   eet_init();
   ecore_app_no_system_modules();
   ecore_init();
   ecore_app_args_set(argc, (const char **)argv);

   ecore_file_init();
   ecore_ipc_init();
   _e_storage_volume_edd_init();
   if (!_e_fm_ipc_init()) return -1;
   efm_log_dom = eina_log_domain_register("efm", EINA_COLOR_GREEN);
   _e_fm_init();

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _e_fm_ipc_slave_data_cb, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _e_fm_ipc_slave_error_cb, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_fm_ipc_slave_del_cb, NULL);

   ecore_main_loop_begin();

   if (_e_fm_ipc_server)
     {
        ecore_ipc_server_del(_e_fm_ipc_server);
        _e_fm_ipc_server = NULL;
     }

   _e_fm_shutdown();
   _e_storage_volume_edd_shutdown();
   ecore_ipc_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();
   eina_shutdown();
   return 0;
}

void
_e_fm_main_catch(unsigned int val)
{
   char buf[64];

   snprintf(buf, sizeof(buf), "%u", val);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_INIT,
                         0, 0, 0, buf, strlen(buf) + 1);
}

#ifdef HAVE_UDISKS_MOUNT
void
_e_fm_main_udisks_catch(Eina_Bool usable)
{
   if (usable)
     {
        mode = EFM_MODE_USING_UDISKS_MOUNT;
        _e_fm_main_catch(mode);
        return;
     }
# ifdef HAVE_EEZE_MOUNT
   _e_fm_main_eeze_init();
   mode = EFM_MODE_USING_EEZE_MOUNT;
# endif
}

void
_e_fm_main_udisks2_catch(Eina_Bool usable)
{
   if (usable)
     {
        mode = EFM_MODE_USING_UDISKS2_MOUNT;
        _e_fm_main_catch(mode);
        return;
     }
   _e_fm_main_udisks_init();
   mode = EFM_MODE_USING_UDISKS_MOUNT;
}
#endif

void
_e_storage_free(E_Storage *s)
{
   _e_fm_shared_device_storage_free(s);
}

void
_e_volume_free(E_Volume *v)
{
   _e_fm_shared_device_volume_free(v);
}

/* API functions */

E_API void
e_volume_mount(E_Volume *v)
{
  switch (mode)
    {
#ifdef HAVE_UDISKS_MOUNT
     case EFM_MODE_USING_UDISKS_MOUNT:
       _e_fm_main_udisks_volume_mount(v);
       break;
     case EFM_MODE_USING_UDISKS2_MOUNT:
       _e_fm_main_udisks2_volume_mount(v);
       break;
#endif
#ifdef HAVE_EEZE_MOUNT
     case EFM_MODE_USING_EEZE_MOUNT:
       _e_fm_main_eeze_volume_mount(v);
       break;
#endif
     default:
       printf("raster can't mount disks by himself!\n");
       (void)v;
    }
}


E_API void
e_volume_unmount(E_Volume *v)
{
  switch (mode)
    {
#ifdef HAVE_UDISKS_MOUNT
     case EFM_MODE_USING_UDISKS_MOUNT:
       _e_fm_main_udisks_volume_unmount(v);
       break;
     case EFM_MODE_USING_UDISKS2_MOUNT:
       _e_fm_main_udisks2_volume_unmount(v);
       break;
#endif
#ifdef HAVE_EEZE_MOUNT
     case EFM_MODE_USING_EEZE_MOUNT:
       _e_fm_main_eeze_volume_unmount(v);
       break;
#endif
     default:
       printf("raster can't unmount disks by himself!\n");
       (void)v;
    }
}

E_API void
e_volume_eject(E_Volume *v)
{
  switch (mode)
    {
#ifdef HAVE_UDISKS_MOUNT
     case EFM_MODE_USING_UDISKS_MOUNT:
       _e_fm_main_udisks_volume_eject(v);
       break;
     case EFM_MODE_USING_UDISKS2_MOUNT:
       _e_fm_main_udisks2_volume_eject(v);
       break;
#endif
#ifdef HAVE_EEZE_MOUNT
     case EFM_MODE_USING_EEZE_MOUNT:
       _e_fm_main_eeze_volume_eject(v);
       break;
#endif
     default:
       printf("raster can't eject disks by himself!\n");
       (void)v;
    }
}

E_API E_Volume *
e_volume_find(const char *udi)
{
   switch (mode)
     {
#ifdef HAVE_UDISKS_MOUNT
      case EFM_MODE_USING_UDISKS_MOUNT:
        return _e_fm_main_udisks_volume_find(udi);
      case EFM_MODE_USING_UDISKS2_MOUNT:
        return _e_fm_main_udisks2_volume_find(udi);
#endif
#ifdef HAVE_EEZE_MOUNT
      case EFM_MODE_USING_EEZE_MOUNT:
        return _e_fm_main_eeze_volume_find(udi);
#endif
      default:
        printf("raster can't find disks by himself!\n");
        (void)udi;
     }
   return NULL;
}

E_API void
e_storage_del(const char *udi)
{
  switch (mode)
    {
#ifdef HAVE_UDISKS_MOUNT
     case EFM_MODE_USING_UDISKS_MOUNT:
       _e_fm_main_udisks_storage_del(udi);
       break;
     case EFM_MODE_USING_UDISKS2_MOUNT:
       _e_fm_main_udisks2_storage_del(udi);
       break;
#endif
#ifdef HAVE_EEZE_MOUNT
     case EFM_MODE_USING_EEZE_MOUNT:
       _e_fm_main_eeze_storage_del(udi);
       break;
#endif
     default:
       printf("raster can't delete disks by himself!\n");
       (void)udi;
    }
}

E_API E_Storage *
e_storage_find(const char *udi)
{
  switch (mode)
    {
#ifdef HAVE_UDISKS_MOUNT
     case EFM_MODE_USING_UDISKS_MOUNT:
       return _e_fm_main_udisks_storage_find(udi);
     case EFM_MODE_USING_UDISKS2_MOUNT:
       return _e_fm_main_udisks2_storage_find(udi);
#endif
#ifdef HAVE_EEZE_MOUNT
     case EFM_MODE_USING_EEZE_MOUNT:
       return _e_fm_main_eeze_storage_find(udi);
#endif
     default:
       printf("raster can't find disks by himself!\n");
       (void)udi;
    }
  return NULL;
}

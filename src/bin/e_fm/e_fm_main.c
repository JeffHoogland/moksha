#include "config.h"

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
#include <fnmatch.h>
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
#include <Evas.h>

#define E_TYPEDEFS
#include "e_config_data.h"
#include "e_fm_op.h"
#undef E_TYPEDEFS
#include "e_config_data.h"
#include "e_fm_op.h"

/* if using ehal, functions will point to _e_fm_main_dbus_X
 * if using eeze, functions will point to _e_fm_main_eeze_X
 */
#ifndef HAVE_EEZE_MOUNT
#include "e_fm_main_dbus.h"
#include "e_fm_shared_dbus.h"
#define _E_FM(FUNC) _e_fm_main_dbus_##FUNC
#define _E_FM_SHARED(FUNC) _e_fm_shared_dbus_##FUNC
#else
#include "e_fm_main_eeze.h"
#define _E_FM(FUNC) _e_fm_main_eeze_##FUNC
#define _E_FM_SHARED(FUNC) _e_fm_shared_eeze_##FUNC
#endif

/* FIXME: things to add to the slave enlightenment_fm process and ipc to e:
 * 
 * * reporting results of fop's (current status - what has been don, what failed etc.)
 * * dbus removable device monitoring (in e17 itself now via e_dbus - move to enlightenment_fm and ipc removable device add/del and anything else)
 * * mount/umount of removable devices (to go along with removable device support - put it in here and message back mount success/failure and where it is now mounted - remove from e17 itself)
 * 
 */

#ifndef EAPI
#define EAPI
#endif

#include "e_fm_main.h"
#include "e_fm_shared_types.h"
#include "e_fm_shared_codec.h"
#include "e_fm_ipc.h"


static void _e_fm_init(void);
static void _e_fm_shutdown(void);

/* contains:
 * _e_volume_edd
 * _e_storage_edd
 * _e_volume_free()
 * _e_storage_free()
 * _e_volume_edd_new()
 * _e_storage_edd_new()
 * _e_storage_volume_edd_init()
 * _e_storage_volume_edd_shutdown()
 */

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;

   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-h")) ||
	    (!strcmp(argv[i], "-help")) ||
	    (!strcmp(argv[i], "--help")))
	  {
	     printf(
		    "This is an internal tool for Enlightenment.\n"
		    "do not use it.\n"
		    );
	     exit(0);
	  }
     }

   eina_init();
   ecore_init();
   ecore_app_args_set(argc, (const char **)argv);

   _e_fm_init();

   ecore_file_init();
   ecore_ipc_init();
   _e_storage_volume_edd_init();
   _e_fm_ipc_init();
   
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
}

static void
_e_fm_init(void)
{
   _E_FM(init)();
}

static void
_e_fm_shutdown(void)
{
   _E_FM(shutdown)();
}

EAPI void
e_volume_mount(E_Volume *v)
{
   _E_FM(volume_mount)(v);
}


EAPI void
e_volume_unmount(E_Volume *v)
{
   _E_FM(volume_unmount)(v);
}

EAPI void
e_volume_eject(E_Volume *v)
{
   _E_FM(volume_eject)(v);
}

EAPI E_Volume *
e_volume_find(const char *udi)
{
	  return _E_FM(volume_find)(udi);
}

EAPI void
e_storage_del(const char *udi)
{
	  _E_FM(storage_del)(udi);
}

EAPI E_Storage *
e_storage_find(const char *udi)
{
	  return _E_FM(storage_find)(udi);
}

void
_e_storage_free(E_Storage *s)
{
	  _E_FM_SHARED(storage_free)(s);
}

void
_e_volume_free(E_Volume *v)
{
	  _E_FM_SHARED(volume_free)(v);
}

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
void *alloca(size_t);
#endif

#ifdef __linux__
#include <features.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Eet.h>
#include <Eeze.h>
#include <Eeze_Disk.h>

#include "e_fm_shared_device.h"
#include "e_fm_shared_codec.h"
#include "e_fm_ipc.h"
#include "e_fm_device.h"
#include <eeze_scanner.h>

#include "e_fm_main.h"
#include "e_fm_main_eeze.h"

static void _e_fm_main_eeze_storage_rescan(const char *syspath);
static void _e_fm_main_eeze_volume_rescan(const char *syspath);
static E_Volume *_e_fm_main_eeze_volume_find_fast(const char *syspath);
static E_Storage *_e_fm_main_eeze_storage_find_fast(const char *syspath);
static void _e_fm_main_eeze_volume_del(const char *syspath);

static Eina_List *_e_stores = NULL;
static Eina_List *_e_vols = NULL;
static Eina_Bool _e_fm_eeze_init = EINA_FALSE;

static Ecore_Exe *scanner = NULL;
static Ecore_Con_Server *svr = NULL;
static Eet_Data_Descriptor *es_edd = NULL;
static Eet_Connection *es_con = NULL;
static Eina_Prefix *pfx = NULL;

void
e_util_env_set(const char *var, const char *val)
{
   if (val)
     {
#ifdef HAVE_SETENV
	setenv(var, val, 1);
#else
	char buf[8192];

	snprintf(buf, sizeof(buf), "%s=%s", var, val);
	if (getenv(var))
	  putenv(buf);
	else
	  putenv(strdup(buf));
#endif
     }
   else
     {
#ifdef HAVE_UNSETENV
	unsetenv(var);
#else
	if (getenv(var)) putenv(var);
#endif
     }
}

static const char *
_mount_point_escape(const char *path)
{
   const char *p;
   if (!path) return NULL;

   /* Most app doesn't handle the hostname in the uri so it's put to NULL */
   for (p = path; *p; p++)
     if ((!isalnum(*p)) && (*p != '/') && (*p != '_')) return NULL;

   return eina_stringshare_add(path);
}

static void
_e_fm_main_eeze_mount_point_set(E_Volume *v)
{
   const char *str;
   char path[PATH_MAX];

   str = strrchr(v->udi, '/');
   if ((!str) || (!str[1]))
     {
        CRI("DEV CHANGED, ABORTING");
        eina_stringshare_replace(&v->mount_point, NULL);
        eeze_disk_mount_point_set(v->disk, NULL);
        return;
     }
   /* here we arbitrarily mount everything to /media/$devnode regardless of fstab */
   eina_stringshare_del(v->mount_point);
   snprintf(path, sizeof(path), "/media%s", str);
   v->mount_point = _mount_point_escape(path);
   eeze_disk_mount_point_set(v->disk, v->mount_point);
}

static int
_e_fm_main_eeze_format_error_msg(char     **buf,
                                 E_Volume  *v,
                                 const char *name,
                                 const char *msg)
{
   int size, vu, vm, en;
   char *tmp;

   vu = strlen(v->udi) + 1;
   vm = strlen(v->mount_point) + 1;
   en = strlen(name) + 1;
   size = vu + vm + en + strlen(msg) + 1;
   tmp = *buf = malloc(size);

   strcpy(tmp, v->udi);
   tmp += vu;
   strcpy(tmp, v->mount_point);
   tmp += vm;
   strcpy(tmp, name);
   tmp += en;
   strcpy(tmp, msg);

   return size;
}

static Eina_Bool
_e_fm_main_eeze_vol_mount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   eeze_disk_cancel(v->disk);
   size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.MountTimeout", "Unable to mount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_fm_main_eeze_cb_vol_mounted(void *user_data __UNUSED__,
                               int type __UNUSED__,
                               Eeze_Event_Disk_Mount *ev)
{
   E_Volume *v;
   char *buf;
   int size;

   v = eeze_disk_data_get(ev->disk);
   if (!v) return ECORE_CALLBACK_RENEW;
   if (!eina_list_data_find(_e_vols, v)) return ECORE_CALLBACK_RENEW;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (!eeze_disk_mounted_get(ev->disk))
     {
        ERR("Mount of '%s' failed!", v->udi);
        return ECORE_CALLBACK_RENEW;
     }
   v->mounted = EINA_TRUE;
   INF("MOUNT: %s from %s", v->udi, v->mount_point);
   size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   strcpy(buf + strlen(buf) + 1, v->mount_point);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_MOUNT_DONE,
                         0, 0, 0, buf, size);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_fm_main_eeze_vol_unmount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   eeze_disk_cancel(v->disk);
   size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.UnmountTimeout", "Unable to unmount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_fm_main_eeze_cb_vol_error(void *user_data __UNUSED__,
                             int type __UNUSED__,
                             Eeze_Event_Disk_Error *ev)
{
   E_Volume *v;
   char *buf;
   int size;

   v = eeze_disk_data_get(ev->disk);
   if (!v) return ECORE_CALLBACK_RENEW;
   if (!eina_list_data_find(_e_vols, v)) return ECORE_CALLBACK_RENEW;
   if (v->mounted)
     {
        size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.UnmountError", ev->message);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                              0, 0, 0, buf, size);
     }
   else
     {
        size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.MountError", ev->message);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                              0, 0, 0, buf, size);
     }
   free(buf);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_fm_main_eeze_cb_vol_unmounted(void *user_data __UNUSED__,
                                 int type __UNUSED__,
                                 Eeze_Event_Disk_Unmount *ev)
{
   char *buf;
   int size;
   E_Volume *v;

   v = eeze_disk_data_get(ev->disk);
   if (!v) return ECORE_CALLBACK_RENEW;
   if (!eina_list_data_find(_e_vols, v)) return ECORE_CALLBACK_RENEW;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   v->mounted = EINA_FALSE;
   INF("UNMOUNT: %s from %s", v->udi, v->mount_point);
   if (!strncmp(v->mount_point, e_user_dir_get(), strlen(e_user_dir_get())))
     unlink(v->mount_point);
   size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   strcpy(buf + strlen(buf) + 1, v->mount_point);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_UNMOUNT_DONE,
                         0, 0, 0, buf, size);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_fm_main_eeze_vol_eject_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   eeze_disk_cancel(v->disk);
   size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.EjectTimeout", "Unable to eject the media with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_fm_main_eeze_cb_vol_ejected(void *user_data __UNUSED__,
                               int type __UNUSED__,
                               Eeze_Event_Disk_Eject *ev)
{
   E_Volume *v;
   char *buf;
   int size;

   v = eeze_disk_data_get(ev->disk);
   if (!v) return ECORE_CALLBACK_RENEW;
   if (!eina_list_data_find(_e_vols, v)) return ECORE_CALLBACK_RENEW;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   v->mounted = EINA_FALSE;
   if (!v->mount_point) _e_fm_main_eeze_mount_point_set(v);
   if (!v->mount_point)
     {
        /* we're aborting here, so just fail */
        return ECORE_CALLBACK_RENEW;
     }
   INF("EJECT: %s from %s", v->udi, v->mount_point);
   size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   strcpy(buf + strlen(buf) + 1, v->mount_point);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_EJECT_DONE,
                         0, 0, 0, buf, size);
   return ECORE_CALLBACK_RENEW;
}

void
_e_fm_main_eeze_volume_eject(E_Volume *v)
{
   if (!v || v->guard) return;
   if (!eeze_disk_mount_wrapper_get(v->disk))
     {
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "%s/enlightenment/utils/enlightenment_sys", eina_prefix_lib_get(pfx));
        eeze_disk_mount_wrapper_set(v->disk, buf);
     }
   v->guard = ecore_timer_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_eeze_vol_eject_timeout, v);
   eeze_disk_eject(v->disk);
}

E_Volume *
_e_fm_main_eeze_volume_add(const char *syspath,
                           Eina_Bool   first_time)
{
   E_Volume *v;
   const char *str;
   Eeze_Disk_Type type;

   if (!syspath) return NULL;
   if (_e_fm_main_eeze_volume_find_fast(syspath)) return NULL;
   v = calloc(1, sizeof(E_Volume));
   if (!v) return NULL;
   v->disk = eeze_disk_new(syspath);
   if (!v->disk)
     {
        free(v);
        return NULL;
     }
   INF("VOL+ %s", syspath);
   eeze_disk_scan(v->disk);
   type = eeze_disk_type_get(v->disk);
   if ((type == EEZE_DISK_TYPE_INTERNAL) || (type == EEZE_DISK_TYPE_UNKNOWN))
     {
        str = eeze_disk_mount_point_get(v->disk);
        if ((type != EEZE_DISK_TYPE_INTERNAL) || (str && (strncmp(str, "/media/", 7))))
          {
             INF("VOL is %s, ignoring", (type == EEZE_DISK_TYPE_INTERNAL) ? "internal" : "unknown");
             eeze_disk_free(v->disk);
             free(v);
             return NULL;
          }
     }
   eeze_disk_data_set(v->disk, v);
   v->udi = eina_stringshare_add(syspath);
   v->icon = NULL;
   v->first_time = first_time;
   v->efm_mode = EFM_MODE_USING_EEZE_MOUNT;
   _e_vols = eina_list_append(_e_vols, v);
   v->uuid = eeze_disk_uuid_get(v->disk);
   v->label = eeze_disk_label_get(v->disk);
   v->fstype = eeze_disk_fstype_get(v->disk);
   v->parent = eeze_disk_udev_get_parent(v->disk);
   {
      /* check for device being its own parent: this is the case
       * for devices such as cdroms
       */
      str = eeze_udev_syspath_get_subsystem(v->parent);
      if (strcmp(str, "block"))
        eina_stringshare_replace(&v->parent, v->udi);
   }
   str = eeze_disk_udev_get_sysattr(v->disk, "queue/hw_sector_size");
   if (!str)
     str = eeze_udev_syspath_get_sysattr(v->parent, "queue/hw_sector_size");
   if (str)
     {
        int64_t sector_size, size;
        sector_size = strtoll(str, NULL, 10);
        eina_stringshare_del(str);
        str = eeze_disk_udev_get_sysattr(v->disk, "size");
        if (str)
          {
             size = strtoll(str, NULL, 10);
             v->size = size * sector_size;
          }
        eina_stringshare_del(str);
     }
   v->mounted = eeze_disk_mounted_get(v->disk);
   str = eeze_disk_udev_get_property(v->disk, "DEVTYPE");
   if (str && (!strcmp(str, "partition")))
     v->partition = EINA_TRUE;
   eina_stringshare_del(str);
   if (v->mounted)
     v->mount_point = eina_stringshare_add(eeze_disk_mount_point_get(v->disk));

   if (v->partition)
     {
        str = eeze_disk_udev_get_sysattr(v->disk, "partition");
        if (str)
           v->partition_number = strtol(str, NULL, 10);
        eina_stringshare_del(str);
        v->partition_label = eeze_disk_udev_get_property(v->disk, "ID_FS_LABEL");
     }

   /* if we have dev/sda ANd dev/sda1 - ia a parent vol - del the parent vol
    * since we actually have real child partition volumes to mount */
   if ((v->partition != 0) && (v->parent))
     {
        E_Volume *v2 = _e_fm_main_eeze_volume_find_fast(v->parent);
        
        if ((v2) && (v2->partition == 0))
          _e_fm_main_eeze_volume_del(v2->udi);
     }
   
   {
      E_Storage *s;
      s = e_storage_find(v->parent);
      INF("++VOL:\n  syspath: %s\n  uuid: %s\n  fstype: %s\n  size: %llu\n label: %s\n"
          "  partition: %d\n  partition_number: %d\n partition_label: %s\n  mounted: %d\n  mount_point: %s",
          v->udi, v->uuid, v->fstype, v->size, v->label, v->partition, v->partition_number,
          v->partition ? v->partition_label : "(not a partition)", v->mounted, v->mount_point);
      if (s)
        v->storage = s;
      else if (v->parent)
        s = v->storage = _e_fm_main_eeze_storage_add(v->parent);

      if (s)
        {
           s->volumes = eina_list_append(s->volumes, v);
           INF("  for storage: %s", s->udi);
        }
      else INF("  storage unknown");
   }

   v->validated = EINA_TRUE;
   {
      void *msg_data;
      int msg_size;

      msg_data = _e_fm_shared_codec_volume_encode(v, &msg_size);
      if (msg_data)
        {
           ecore_ipc_server_send(_e_fm_ipc_server,
                                 6 /*E_IPC_DOMAIN_FM*/,
                                 E_FM_OP_VOLUME_ADD,
                                 0, 0, 0, msg_data, msg_size);
           free(msg_data);
        }
   }
   return v;
}

static void
_e_fm_main_eeze_volume_del(const char *syspath)
{
   E_Volume *v;

   v = _e_fm_main_eeze_volume_find_fast(syspath);
   if (!v) return;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }
   if (v->validated)
     {
        INF("--VOL %s\n", v->udi);
        /* FIXME: send event of storage volume (disk) removed */
        ecore_ipc_server_send(_e_fm_ipc_server,
                              6 /*E_IPC_DOMAIN_FM*/,
                              E_FM_OP_VOLUME_DEL,
                              0, 0, 0, v->udi, eina_stringshare_strlen(v->udi) + 1);
     }
   _e_vols = eina_list_remove(_e_vols, v);
   v->uuid = NULL;
   v->label = NULL;
   v->fstype = NULL;
   _e_fm_shared_device_volume_free(v);
}

static E_Volume *
_e_fm_main_eeze_volume_find_fast(const char *syspath)
{
   Eina_List *l;
   E_Volume *v;

   if (!syspath) return NULL;
   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (syspath == v->udi) return v;
     }
   return NULL;
}

E_Volume *
_e_fm_main_eeze_volume_find(const char *syspath)
{
   Eina_List *l;
   E_Volume *v;

   if (!syspath) return NULL;
   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (!v->udi) continue;
        if (!strcmp(syspath, v->udi)) return v;
     }
   return NULL;
}

void
_e_fm_main_eeze_volume_unmount(E_Volume *v)
{
   if (!v || v->guard) return;

   INF("unmount %s %s", v->udi, v->mount_point);

   if (!eeze_disk_mount_wrapper_get(v->disk))
     {
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "%s/enlightenment/utils/enlightenment_sys", eina_prefix_lib_get(pfx));
        eeze_disk_mount_wrapper_set(v->disk, buf);
     }
   v->guard = ecore_timer_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_eeze_vol_unmount_timeout, v);
   eeze_disk_unmount(v->disk);
}

void
_e_fm_main_eeze_volume_mount(E_Volume *v)
{
   int size, opts = 0;
   char *buf;

   if (!v || v->guard)
     return;

   if (v->fstype)
     {
        if ((!strcmp(v->fstype, "vfat")) ||
            (!strcmp(v->fstype, "ntfs")) ||
            (!strcmp(v->fstype, "iso9660")) ||
            (!strcmp(v->fstype, "jfs")))
          {
             opts |= EEZE_DISK_MOUNTOPT_UTF8;
          }
     }
   opts |= EEZE_DISK_MOUNTOPT_UID | EEZE_DISK_MOUNTOPT_NOSUID | EEZE_DISK_MOUNTOPT_NODEV | EEZE_DISK_MOUNTOPT_NOEXEC;

   _e_fm_main_eeze_mount_point_set(v);
   if (!v->mount_point) goto error;
   INF("mount %s %s [fs type = %s]", v->udi, v->mount_point, v->fstype);   
   eeze_disk_mountopts_set(v->disk, opts);
   if (!eeze_disk_mount_wrapper_get(v->disk))
     {
        char buf2[PATH_MAX];

        snprintf(buf2, sizeof(buf2), "%s/enlightenment/utils/enlightenment_sys", eina_prefix_lib_get(pfx));
        eeze_disk_mount_wrapper_set(v->disk, buf2);
     }
   v->guard = ecore_timer_add(E_FM_MOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_eeze_vol_mount_timeout, v);
   INF("MOUNT: %s", v->udi);
   if (!eeze_disk_mount(v->disk)) goto error;
   return;
error:
   size = _e_fm_main_eeze_format_error_msg(&buf, v, "org.enlightenment.fm2.MountError", "Critical error occurred during mounting!");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);
   CRI("ERROR WHEN ATTEMPTING TO MOUNT!");
}

static void
eet_setup(void)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), "eeze_scanner_event", sizeof(Eeze_Scanner_Event)))
     {
        ERR("Could not create eet data descriptor!");
        exit(1);
     }

   es_edd = eet_data_descriptor_stream_new(&eddc);
   EEZE_SCANNER_EDD_SETUP(es_edd);
}
static Eina_Bool
_scanner_poll(void *data __UNUSED__)
{
   if (svr) return EINA_FALSE;
   svr = ecore_con_server_connect(ECORE_CON_LOCAL_SYSTEM, "eeze_scanner", 0, NULL);
   if (!svr) return EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
_scanner_add(void *data, int type __UNUSED__, Ecore_Exe_Event_Add *ev)
{
   if (data != ecore_exe_data_get(ev->exe)) return ECORE_CALLBACK_PASS_ON;
   INF("Scanner started");
   if (_scanner_poll(NULL))
     ecore_poller_add(ECORE_POLLER_CORE, 8, (Ecore_Task_Cb)_scanner_poll, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_scanner_del(void *data, int type __UNUSED__, Ecore_Exe_Event_Del *ev)
{
   if (data != ecore_exe_data_get(ev->exe)) return ECORE_CALLBACK_PASS_ON;
   if (!svr)
     {
        INF("scanner connection dead, exiting");
        exit(1);
     }
   INF("lost connection to scanner");
   scanner = NULL;
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_scanner_read(const void *data, size_t size, void *d __UNUSED__)
{
   Eeze_Scanner_Event *ev = NULL;

   ev = eet_data_descriptor_decode(es_edd, data, size);
   INF("Received %s '%s'", ev->volume ? "volume" : "storage", ev->device);
   switch (ev->type)
     {
      case EEZE_SCANNER_EVENT_TYPE_ADD:
        if (ev->volume) _e_fm_main_eeze_volume_add(ev->device, EINA_TRUE);
        else _e_fm_main_eeze_storage_add(ev->device);
        break;
      case EEZE_SCANNER_EVENT_TYPE_REMOVE:
        if (ev->volume) _e_fm_main_eeze_volume_del(ev->device);
        else _e_fm_main_eeze_storage_del(ev->device);
        break;
      default:
        if (ev->volume) _e_fm_main_eeze_volume_rescan(ev->device);
        else _e_fm_main_eeze_storage_rescan(ev->device);
        break;
     }
   eina_stringshare_del(ev->device);
   free(ev);
   return EINA_TRUE;
}

static Eina_Bool
_scanner_write(const void *eet_data __UNUSED__, size_t size __UNUSED__, void *user_data __UNUSED__)
{
   return EINA_TRUE;
}

static void
_scanner_run(void)
{
   scanner = ecore_exe_pipe_run("eeze_scanner", ECORE_EXE_NOT_LEADER, pfx);
}


static Eina_Bool
_scanner_con(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Del *ev __UNUSED__)
{
   INF("Scanner connected");
   es_con = eet_connection_new(_scanner_read, _scanner_write, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_scanner_disc(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Del *ev __UNUSED__)
{
   INF("Scanner disconnected");
   if (_scanner_poll(NULL))
     _scanner_run();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_scanner_err(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Error *ev __UNUSED__)
{
   INF("Scanner connection error");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_scanner_data(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev)
{
   eet_connection_received(es_con, ev->data, ev->size);
   return ECORE_CALLBACK_RENEW;
}

void
_e_fm_main_eeze_init(void)
{
   char **argv;
   int argc;

   if (_e_fm_eeze_init) return;
   ecore_app_args_get(&argc, &argv);
   pfx = eina_prefix_new(argv[0], e_storage_find,
                         "E", "enlightenment", "AUTHORS",
                         PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR, LOCALE_DIR);
   if (!pfx)
     {
        ERR("Could not determine prefix!");
        exit(1);
     }
   _e_fm_eeze_init = EINA_TRUE;
   ecore_con_init();
   eeze_init();
   eina_log_domain_level_set("eeze_disk", EINA_LOG_LEVEL_DBG);
   eeze_mount_tabs_watch();
   ecore_event_handler_add(EEZE_EVENT_DISK_MOUNT, 
                           (Ecore_Event_Handler_Cb)_e_fm_main_eeze_cb_vol_mounted, NULL);
   ecore_event_handler_add(EEZE_EVENT_DISK_UNMOUNT, 
                           (Ecore_Event_Handler_Cb)_e_fm_main_eeze_cb_vol_unmounted, NULL);
   ecore_event_handler_add(EEZE_EVENT_DISK_EJECT, 
                           (Ecore_Event_Handler_Cb)_e_fm_main_eeze_cb_vol_ejected, NULL);
   ecore_event_handler_add(EEZE_EVENT_DISK_ERROR, 
                           (Ecore_Event_Handler_Cb)_e_fm_main_eeze_cb_vol_error, NULL);

   ecore_event_handler_add(ECORE_EXE_EVENT_ADD, 
                           (Ecore_Event_Handler_Cb)_scanner_add, pfx);
   ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
                           (Ecore_Event_Handler_Cb)_scanner_del, pfx);

   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, 
                           (Ecore_Event_Handler_Cb)_scanner_con, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, 
                           (Ecore_Event_Handler_Cb)_scanner_disc, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, 
                           (Ecore_Event_Handler_Cb)_scanner_data, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ERROR, 
                           (Ecore_Event_Handler_Cb)_scanner_err, NULL);

   eet_setup();
   if (!_scanner_poll(NULL))
     _scanner_run();
}

void
_e_fm_main_eeze_shutdown(void)
{
   if (scanner) ecore_exe_free(scanner);
   eeze_mount_tabs_unwatch();
   eeze_shutdown();
   ecore_con_shutdown();
}


static void
_e_fm_main_eeze_storage_rescan(const char *syspath)
{
   E_Storage *s;

   s = _e_fm_main_eeze_storage_find_fast(syspath);
   if (s) _e_fm_main_eeze_storage_del(syspath);
   _e_fm_main_eeze_storage_add(syspath);
}

static void
_e_fm_main_eeze_volume_rescan(const char *syspath)
{
   E_Volume *v;

   v = _e_fm_main_eeze_volume_find_fast(syspath);
   if (v) _e_fm_main_eeze_volume_del(syspath);
   _e_fm_main_eeze_volume_add(syspath, EINA_FALSE);
}

E_Storage *
_e_fm_main_eeze_storage_add(const char *syspath)
{
   E_Storage *s;
   const char *str;

   if (!syspath) return NULL;
   if (_e_fm_main_eeze_storage_find_fast(syspath)) return NULL;

   str = eeze_udev_syspath_get_property(syspath, "ID_TYPE");
   if (!str)
     {
        const char *p, *subsystem;

        p = eeze_udev_syspath_get_parent(syspath);
        if (!p) return NULL;
        subsystem = eeze_udev_syspath_get_subsystem(p);
        if ((!subsystem) || strcmp(subsystem, "mmc"))
          {
             eina_stringshare_del(subsystem);
             return NULL;
          }
        eina_stringshare_del(subsystem);
     }
   eina_stringshare_del(str);
   s = calloc(1, sizeof(E_Storage));
   if (!s) return NULL;
   s->disk = eeze_disk_new(syspath);
   if (!s->disk) goto error;
   _e_stores = eina_list_append(_e_stores, s);
   eeze_disk_scan(s->disk); /* performs all necessary udev lookups at once */
   s->udi = eina_stringshare_add(eeze_disk_syspath_get(s->disk));
   switch (eeze_disk_type_get(s->disk))
     {
      case EEZE_DISK_TYPE_CDROM:
        s->drive_type = eina_stringshare_add("cdrom");
        break;
      case EEZE_DISK_TYPE_USB:
      case EEZE_DISK_TYPE_INTERNAL:
        s->drive_type = eina_stringshare_add("disk");
        break;
      case EEZE_DISK_TYPE_FLASH:
        s->drive_type = eina_stringshare_add("compact_flash");
        break;
      default:
        s->drive_type = eina_stringshare_add("floppy");
        break;
     }
   /* FIXME: this may not be congruent with the return types of other mounters,
    * but we don't use it anyway, so it's probably fine
    */
   s->bus = eeze_disk_udev_get_property(s->disk, "ID_BUS");
   s->model = eina_stringshare_add(eeze_disk_model_get(s->disk));
   s->vendor = eina_stringshare_add(eeze_disk_vendor_get(s->disk));
   s->serial = eina_stringshare_add(eeze_disk_serial_get(s->disk));
   s->removable = !!eeze_disk_removable_get(s->disk);

   if (s->removable)
     {
        int64_t size, sector_size;

        s->media_available = EINA_TRUE; /* all storage with removable=1 in udev has media available */
        str = eeze_disk_udev_get_sysattr(s->disk, "queue/hw_sector_size");
        if (str)
          {
             sector_size = strtoll(str, NULL, 10);
             eina_stringshare_del(str);
             str = eeze_disk_udev_get_sysattr(s->disk, "size");
             if (str)
               {
                  size = strtoll(str, NULL, 10);
                  s->media_size = size * sector_size;
                  eina_stringshare_del(str);
               }
          }
        s->hotpluggable = EINA_TRUE;
     }

   if (eeze_disk_type_get(s->disk) == EEZE_DISK_TYPE_CDROM)
     s->requires_eject = EINA_TRUE; /* only true on cdroms */
   s->media_check_enabled = s->removable;

   INF("++STO:\n  syspath: %s\n  bus: %s\n  drive_type: %s\n  model: %s\n  vendor: %s\n  serial: %s\n  icon.drive: %s\n  icon.volume: %s",
       s->udi, s->bus, s->drive_type, s->model, s->vendor, s->serial, s->icon.drive, s->icon.volume);
   s->validated = EINA_TRUE;
   {
      void *msg_data;
      int msg_size;

      msg_data = _e_fm_shared_codec_storage_encode(s, &msg_size);
      if (msg_data)
        {
           ecore_ipc_server_send(_e_fm_ipc_server,
                                 6 /*E_IPC_DOMAIN_FM*/,
                                 E_FM_OP_STORAGE_ADD,
                                 0, 0, 0, msg_data, msg_size);
           free(msg_data);
        }
   }
   return s;

error:
   ERR("ERR %s", s->udi);
   _e_fm_shared_device_storage_free(s);
   return NULL;
}

void
_e_fm_main_eeze_storage_del(const char *syspath)
{
   E_Storage *s;

   s = e_storage_find(syspath);
   if (!s) return;
   if (s->validated)
     {
        INF("--STO %s", s->udi);
        ecore_ipc_server_send(_e_fm_ipc_server,
                              6 /*E_IPC_DOMAIN_FM*/,
                              E_FM_OP_STORAGE_DEL,
                              0, 0, 0, s->udi, eina_stringshare_strlen(s->udi) + 1);
     }
   _e_stores = eina_list_remove(_e_stores, s);
   _e_fm_shared_device_storage_free(s);
}

static E_Storage *
_e_fm_main_eeze_storage_find_fast(const char *syspath)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (syspath == s->udi) return s;
     }
   return NULL;
}

E_Storage *
_e_fm_main_eeze_storage_find(const char *syspath)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (!strcmp(syspath, s->udi)) return s;
     }
   return NULL;
}

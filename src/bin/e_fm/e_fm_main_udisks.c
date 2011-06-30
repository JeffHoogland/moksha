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
#include <E_DBus.h>
#include <E_Ukit.h>

#include "e_fm_main.h"
#include "e_fm_main_udisks.h"

#include "e_fm_shared_codec.h"
#include "e_fm_shared_device.h"
#include "e_fm_ipc.h"
#include "e_fm_device.h"

static E_DBus_Signal_Handler *_udisks_poll = NULL;
static E_DBus_Connection *_e_fm_main_udisks_conn = NULL;
static Eina_List *_e_stores = NULL;
static Eina_List *_e_vols = NULL;

static void _e_fm_main_udisks_cb_dev_all(void *user_data __UNUSED__,
                             E_Ukit_String_List_Return *ret,
                             DBusError      *error);
static void _e_fm_main_udisks_cb_dev_verify(const char *udi,
                                            E_Ukit_Property *ret,
                                            DBusError      *error);
static void _e_fm_main_udisks_cb_dev_add_verify(const char *udi,
                             E_Ukit_Property *ret,
                             DBusError      *error);
static void _e_fm_main_udisks_cb_dev_add(void        *data,
                                         DBusMessage *msg);
static void _e_fm_main_udisks_cb_dev_del(void        *data,
                                         DBusMessage *msg);
static void _e_fm_main_udisks_cb_dev_chg(void        *data,
                                         DBusMessage *msg);
static void _e_fm_main_udisks_cb_prop_modified(E_Volume    *v,
                                   DBusMessage *msg);
static void _e_fm_main_udisks_cb_store_prop(E_Storage *s,
                                E_Ukit_Properties *ret,
                                DBusError *error);
static void _e_fm_main_udisks_cb_vol_prop(E_Volume      *v,
                              E_Ukit_Properties      *ret,
                              DBusError *error);
static void _e_fm_main_udisks_cb_vol_mounted(E_Volume               *v,
                                 DBusError          *error);
static void _e_fm_main_udisks_cb_vol_unmounted(E_Volume               *v,
                                   DBusError          *error);
static void _e_fm_main_udisks_cb_vol_unmounted_before_eject(E_Volume      *v,
                                                void      *method_return __UNUSED__,
                                                DBusError *error);

static Eina_Bool _e_fm_main_udisks_cb_vol_ejecting_after_unmount(E_Volume *v);
static void      _e_fm_main_udisks_cb_vol_ejected(E_Volume               *v,
                                 void *method_return __UNUSED__,
                                 DBusError          *error);
static int _e_fm_main_udisks_format_error_msg(char     **buf,
                                              E_Volume  *v,
                                              DBusError *error);
static void _e_fm_main_udisks_test(void        *data,
                                   DBusMessage *msg,
                                   DBusError   *error);
static void _e_fm_main_udisks_poll(void        *data,
                                   DBusMessage *msg);

static Eina_Bool _e_fm_main_udisks_vol_mount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks_vol_unmount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks_vol_eject_timeout(E_Volume *v);

static void
_e_fm_main_udisks_poll(void *data   __UNUSED__,
                       DBusMessage *msg)
{
   DBusError err;
   const char *name, *from, *to;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &from,
                              DBUS_TYPE_STRING, &to,
                              DBUS_TYPE_INVALID))
     dbus_error_free(&err);

   //printf("name: %s\nfrom: %s\nto: %s\n", name, from, to);
   if ((name) && !strcmp(name, E_UDISKS_BUS))
     _e_fm_main_udisks_test(NULL, NULL, NULL);
}

static void
_e_fm_main_udisks_test(void *data       __UNUSED__,
                       DBusMessage *msg __UNUSED__,
                       DBusError       *error)
{
   if ((error) && (dbus_error_is_set(error)))
     {
        dbus_error_free(error);
        _e_fm_main_udisks_catch(EINA_FALSE);
        return;
     }
   if (_udisks_poll)
     e_dbus_signal_handler_del(_e_fm_main_udisks_conn, _udisks_poll);

   e_udisks_get_all_devices(_e_fm_main_udisks_conn, (E_DBus_Callback_Func)_e_fm_main_udisks_cb_dev_all, NULL);

   e_dbus_signal_handler_add(_e_fm_main_udisks_conn, E_UDISKS_BUS,
                             E_UDISKS_PATH,
                             E_UDISKS_BUS,
                             "DeviceAdded", (E_DBus_Signal_Cb)_e_fm_main_udisks_cb_dev_add, NULL);
   e_dbus_signal_handler_add(_e_fm_main_udisks_conn, E_UDISKS_BUS,
                             E_UDISKS_PATH,
                             E_UDISKS_BUS,
                             "DeviceRemoved", (E_DBus_Signal_Cb)_e_fm_main_udisks_cb_dev_del, NULL);
   e_dbus_signal_handler_add(_e_fm_main_udisks_conn, E_UDISKS_BUS,
                             E_UDISKS_PATH,
                             E_UDISKS_BUS,
                             "DeviceChanged", (E_DBus_Signal_Cb)_e_fm_main_udisks_cb_dev_chg, NULL);
   _e_fm_main_udisks_catch(EINA_TRUE); /* signal usage of udisks for mounting */
}

static void
_e_fm_main_udisks_cb_dev_all(void *user_data __UNUSED__,
                             E_Ukit_String_List_Return *ret,
                             DBusError      *error)
{
   Eina_List *l;
   const char *udi;

   if (!ret || !ret->strings) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   EINA_LIST_FOREACH(ret->strings, l, udi)
     {
	printf("DB INIT DEV+: %s\n", udi);
          e_udisks_get_property(_e_fm_main_udisks_conn, udi, "IdUsage",
                                (E_DBus_Callback_Func)_e_fm_main_udisks_cb_dev_verify, (void*)eina_stringshare_add(udi));
     }
}


static void
_e_fm_main_udisks_cb_dev_verify(const char *udi,
                             E_Ukit_Property *ret,
                             DBusError      *error)
{
   if (!ret) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   if ((!ret->val.s) || (!ret->val.s[0])) /* storage */
     _e_fm_main_udisks_storage_add(udi);
   else if (!strcmp(ret->val.s, "filesystem"))
     {
	printf("DB VOL+: %s\n", udi);
          _e_fm_main_udisks_volume_add(udi, EINA_TRUE);
     }
   //eina_stringshare_del(udi);
}

static void
_e_fm_main_udisks_cb_dev_add_verify(const char *udi,
                             E_Ukit_Property *ret,
                             DBusError      *error)
{
   if (!ret) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   if ((!ret->val.s) || (!ret->val.s[0])) /* storage */
     _e_fm_main_udisks_storage_add(udi);
   else if (!strcmp(ret->val.s, "filesystem"))
     {
	printf("DB VOL+: %s\n", udi);
          _e_fm_main_udisks_volume_add(udi, EINA_FALSE);
     }
   //eina_stringshare_del(udi);
}

static void
_e_fm_main_udisks_cb_dev_add(void *data   __UNUSED__,
                             DBusMessage *msg)
{
   DBusError err;
   char *udi = NULL;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_OBJECT_PATH, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   e_udisks_get_property(_e_fm_main_udisks_conn, udi, "IdUsage",
                         (E_DBus_Callback_Func)_e_fm_main_udisks_cb_dev_add_verify, (void*)eina_stringshare_add(udi));
}

static void
_e_fm_main_udisks_cb_dev_del(void *data   __UNUSED__,
                             DBusMessage *msg)
{
   DBusError err;
   char *udi;
   E_Volume *v;

   dbus_error_init(&err);

   dbus_message_get_args(msg,
                         &err, DBUS_TYPE_OBJECT_PATH,
                         &udi, DBUS_TYPE_INVALID);
   printf("DB DEV-: %s\n", udi);
   if ((v = _e_fm_main_udisks_volume_find(udi)))
     {
        if (v->optype == E_VOLUME_OP_TYPE_EJECT)
          _e_fm_main_udisks_cb_vol_ejected(v, msg, &err);  
     }
   _e_fm_main_udisks_volume_del(udi);
   _e_fm_main_udisks_storage_del(udi);
}

static void
_e_fm_main_udisks_cb_dev_chg(void *data   __UNUSED__,
                             DBusMessage *msg)
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);

   dbus_message_get_args(msg, &err,
                         DBUS_TYPE_OBJECT_PATH, &udi,
                         DBUS_TYPE_INVALID);

        printf("DB STORE CAP+: %s\n", udi);
   e_udisks_get_property(_e_fm_main_udisks_conn, udi, "IdUsage",
                         (E_DBus_Callback_Func)_e_fm_main_udisks_cb_dev_add_verify, (void*)eina_stringshare_add(udi));
}

static void
_e_fm_main_udisks_cb_prop_modified(E_Volume    *v,
                                   DBusMessage *msg)
{
   if (!v) return;

   if (dbus_message_get_error_name(msg))
     {
        printf("DBUS ERROR: %s\n", dbus_message_get_error_name(msg));
        return;
     }
   e_udisks_get_all_properties(_e_fm_main_udisks_conn, v->udi,
                                      (E_DBus_Callback_Func)_e_fm_main_udisks_cb_vol_prop, v);
}

static void
_e_fm_main_udisks_cb_store_prop(E_Storage *s,
                                E_Ukit_Properties *ret,
                                DBusError *error)
{
   const char *str;
   int err = 0;

   if (!ret) goto error;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   s->bus = e_ukit_property_string_get(ret, "DriveConnectionInterface", &err);
   if (err) goto error;
   s->bus = eina_stringshare_add(s->bus);
   {
      const Eina_List *l;

      l = e_ukit_property_strlist_get(ret, "DriveMediaCompatibility", &err);
      if (err) goto error;
      if (l) s->drive_type = eina_stringshare_add(l->data);
   }
   s->model = e_ukit_property_string_get(ret, "DriveModel", &err);
   if (err) goto error;
   s->model = eina_stringshare_add(s->model);
   s->vendor = e_ukit_property_string_get(ret, "DriveVendor", &err);
   if (err) goto error;
   s->vendor = eina_stringshare_add(s->vendor);
   s->serial = e_ukit_property_string_get(ret, "DriveSerial", &err);
//   if (err) goto error;
   if (err) printf("Error getting serial for %s\n", s->udi);
   s->serial = eina_stringshare_add(s->serial);

   s->removable = e_ukit_property_bool_get(ret, "DeviceIsRemovable", &err);
   s->system_internal = e_ukit_property_bool_get(ret, "DeviceIsSystemInternal", &err);
   if (s->system_internal) goto error; /* only track non internal */
   str = e_ukit_property_string_get(ret, "IdUsage", &err);
   /* if not of filesystem usage type - skip it  - testing on ubuntu 10.04 */
   if (!((str) && (!strcmp(str, "filesystem")))) goto error;
   /* force it to be removable if it passed the above tests */
   s->removable = EINA_TRUE;
   
// ubuntu 10.04 - only dvd is reported as removable. usb storage and mmcblk
// is not - but its not "system internal".   
//   if (!s->removable) goto error; /* only track removable media */
   s->media_available = e_ukit_property_bool_get(ret, "DeviceIsMediaAvailable", &err);
   s->media_size = e_ukit_property_uint64_get(ret, "DeviceSize", &err);

   s->requires_eject = e_ukit_property_bool_get(ret, "DriveIsMediaEjectable", &err);
   s->hotpluggable = e_ukit_property_bool_get(ret, "DriveCanDetach", &err);
   s->media_check_enabled = !e_ukit_property_bool_get(ret, "DeviceIsMediaChangeDetectionInhibited", &err);

   s->icon.drive = e_ukit_property_string_get(ret, "DevicePresentationIconName", &err);
   if (s->icon.drive && s->icon.drive[0]) s->icon.drive = eina_stringshare_add(s->icon.drive);
   else s->icon.drive = NULL;
   s->icon.volume = e_ukit_property_string_get(ret, "DevicePresentationIconName", &err);
   if (s->icon.volume && s->icon.volume[0]) s->icon.volume = eina_stringshare_add(s->icon.volume);
   else s->icon.volume = NULL;

//   printf("++STO:\n  udi: %s\n  bus: %s\n  drive_type: %s\n  model: %s\n  vendor: %s\n  serial: %s\n  icon.drive: %s\n  icon.volume: %s\n\n", s->udi, s->bus, s->drive_type, s->model, s->vendor, s->serial, s->icon.drive, s->icon.volume);
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
   return;

error:
//   printf("ERR %s\n", s->udi);
   _e_fm_main_udisks_storage_del(s->udi);
}

static void
_e_fm_main_udisks_cb_vol_prop(E_Volume      *v,
                              E_Ukit_Properties      *ret,
                              DBusError *error)
{
   E_Storage *s = NULL;
   int err = 0;
   const char *str = NULL;

   if ((!v) || (!ret)) goto error;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   if (e_ukit_property_bool_get(ret, "DeviceIsSystemInternal", &err) || err) goto error;

   /* skip volumes with volume.ignore set */
   if (e_ukit_property_bool_get(ret, "DeviceIsMediaChangeDetectionInhibited", &err) || err)
     goto error;

   /* skip volumes that aren't filesystems */
   str = e_ukit_property_string_get(ret, "IdUsage", &err);
   if (err || !str) goto error;
   if (strcmp(str, "filesystem"))
     {
        if (strcmp(str, "crypto"))
          v->encrypted = e_ukit_property_bool_get(ret, "DeviceIsLuks", &err);

        if (!v->encrypted)
          goto error;
     }
   str = NULL;

   v->uuid = e_ukit_property_string_get(ret, "IdUuid", &err);
   if (err) goto error;
   v->uuid = eina_stringshare_add(v->uuid);

   v->label = e_ukit_property_string_get(ret, "IdLabel", &err);
   if (!v->label) v->label = e_ukit_property_string_get(ret, "DeviceFile", &err); /* avoid having blank labels */
   if (!v->label) v->label = v->uuid; /* last resort */
   v->label = eina_stringshare_add(v->label);

   if (!v->encrypted)
     {
        const Eina_List *l;   

        l = e_ukit_property_strlist_get(ret, "DeviceMountPaths", &err);
        if (err) goto error;
        if (l) v->mount_point = eina_stringshare_add(l->data);

        v->fstype = e_ukit_property_string_get(ret, "IdType", &err);
        v->fstype = eina_stringshare_add(v->fstype);

        v->size = e_ukit_property_uint64_get(ret, "DeviceSize", &err);

        v->mounted = e_ukit_property_bool_get(ret, "DeviceIsMounted", &err);
        if (err) goto error;
     }
   else
     v->unlocked = e_ukit_property_bool_get(ret, "DeviceIsLuksCleartext", &err);

   v->partition = e_ukit_property_bool_get(ret, "DeviceIsPartition", &err);
   if (err) goto error;

   if (v->partition)
     {
        v->partition_number = e_ukit_property_int_get(ret, "PartitionNumber", NULL);
        v->partition_label = e_ukit_property_string_get(ret, "PartitionLabel", NULL);
        v->partition_label = eina_stringshare_add(v->partition_label);
     }

   if (v->unlocked)
     {
        const char *enc;
        E_Volume *venc;

        enc = e_ukit_property_string_get(ret, "LuksCleartextSlave", &err);
        venc = _e_fm_main_udisks_volume_find(enc);
        v->parent = venc->parent;
        v->storage = venc->storage;
        v->storage->volumes = eina_list_append(v->storage->volumes, v);
     }
   else
     {
        v->parent = e_ukit_property_string_get(ret, "PartitionSlave", &err);
          
        if (!err)
          {
             if (v->parent)
               {
                  s = e_storage_find(v->parent);
                  if (s)
                    {
                       v->storage = s;
                       if (!eina_list_data_find_list(s->volumes, v))
                         s->volumes = eina_list_append(s->volumes, v);
                    }
               }
             else
               {
                  v->storage = _e_fm_main_udisks_storage_add(v->udi); /* disk is both storage and volume */
                  if (v->storage) v->storage->volumes = eina_list_append(v->storage->volumes, v);
                  v->parent = v->udi;
               }
          }
     }
   v->parent = eina_stringshare_add(v->parent);

   switch (v->optype)
     {
      case E_VOLUME_OP_TYPE_MOUNT:
        _e_fm_main_udisks_cb_vol_mounted(v, error);
        return;
      case E_VOLUME_OP_TYPE_UNMOUNT:
        _e_fm_main_udisks_cb_vol_unmounted(v, error);
        return;
      case E_VOLUME_OP_TYPE_EJECT:
        _e_fm_main_udisks_cb_vol_unmounted_before_eject(v, NULL, error);
        return;
      default:
        break;
     }

//   printf("++VOL:\n  udi: %s\n  uuid: %s\n  fstype: %s\n  size: %llu\n label: %s\n  partition: %d\n  partition_number: %d\n partition_label: %s\n  mounted: %d\n  mount_point: %s\n", v->udi, v->uuid, v->fstype, v->size, v->label, v->partition, v->partition_number, v->partition ? v->partition_label : "(not a partition)", v->mounted, v->mount_point);
//   if (s) printf("  for storage: %s\n", s->udi);
//   else printf("  storage unknown\n");
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
   return;

error:
   if (v) _e_fm_main_udisks_volume_del(v->udi);
   return;
}

static int
_e_fm_main_udisks_format_error_msg(char     **buf,
                                   E_Volume  *v,
                                   DBusError *error)
{
   int size, vu, vm, en;
   char *tmp;

   vu = strlen(v->udi) + 1;
   vm = strlen(v->mount_point) + 1;
   en = strlen(error->name) + 1;
   size = vu + vm + en + strlen(error->message) + 1;
   tmp = *buf = malloc(size);

   strcpy(tmp, v->udi);
   tmp += vu;
   strcpy(tmp, v->mount_point);
   tmp += vm;
   strcpy(tmp, error->name);
   tmp += en;
   strcpy(tmp, error->message);

   return size;
}

static Eina_Bool
_e_fm_main_udisks_vol_mount_timeout(E_Volume *v)
{
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   error.name = "org.enlightenment.fm2.MountTimeout";
   error.message = "Unable to mount the volume with specified time-out.";
   size = _e_fm_main_udisks_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_mounted(E_Volume               *v,
                                 DBusError          *error)
{
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (error && dbus_error_is_set(error))
     {
        size = _e_fm_main_udisks_format_error_msg(&buf, v, error);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                              0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        v->optype = E_VOLUME_OP_TYPE_NONE;
        return;
     }
   if (!v->mount_point) return; /* come back later */

   v->optype = E_VOLUME_OP_TYPE_NONE;
   v->op = NULL;
   v->mounted = EINA_TRUE;
//   printf("MOUNT: %s from %s\n", v->udi, v->mount_point);
   size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   strcpy(buf + strlen(buf) + 1, v->mount_point);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_MOUNT_DONE,
                         0, 0, 0, buf, size);
}

static Eina_Bool
_e_fm_main_udisks_vol_unmount_timeout(E_Volume *v)
{
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   error.name = "org.enlightenment.fm2.UnmountTimeout";
   error.message = "Unable to unmount the volume with specified time-out.";
   size = _e_fm_main_udisks_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_unmounted(E_Volume               *v,
                                   DBusError          *error)
{
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (v->optype != E_VOLUME_OP_TYPE_EJECT)
     {
        v->optype = E_VOLUME_OP_TYPE_NONE;
        v->op = NULL;
     }
   if (dbus_error_is_set(error))
     {
        size = _e_fm_main_udisks_format_error_msg(&buf, v, error);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                              0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        return;
     }

   v->mounted = EINA_FALSE;
//   printf("UNMOUNT: %s from %s\n", v->udi, v->mount_point);
   size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   strcpy(buf + strlen(buf) + 1, v->mount_point);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_UNMOUNT_DONE,
                         0, 0, 0, buf, size);
}

static Eina_Bool
_e_fm_main_udisks_vol_eject_timeout(E_Volume *v)
{
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   error.name = "org.enlightenment.fm2.EjectTimeout";
   error.message = "Unable to eject the media with specified time-out.";
   size = _e_fm_main_udisks_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_fm_main_udisks_cb_vol_ejecting_after_unmount(E_Volume *v)
{
   v->guard = ecore_timer_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_eject_timeout, v);
   v->op = e_udisks_volume_eject(_e_fm_main_udisks_conn, v->udi, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_unmounted_before_eject(E_Volume      *v,
                                                void      *method_return __UNUSED__,
                                                DBusError *error)
{
   Eina_Bool err;

   err = !!dbus_error_is_set(error);
   _e_fm_main_udisks_cb_vol_unmounted(v, error);

   // delay is required for all message handlers were executed after unmount
   if (!err)
     ecore_timer_add(1.0, (Ecore_Task_Cb)_e_fm_main_udisks_cb_vol_ejecting_after_unmount, v);
}

static void
_e_fm_main_udisks_cb_vol_ejected(E_Volume               *v,
                                 void *method_return __UNUSED__,
                                 DBusError          *error)
{
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   v->optype = E_VOLUME_OP_TYPE_NONE;
   v->op = NULL;
   if (dbus_error_is_set(error))
     {
        size = _e_fm_main_udisks_format_error_msg(&buf, v, error);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                              0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        return;
     }

   size = strlen(v->udi) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_EJECT_DONE,
                         0, 0, 0, buf, size);
}

E_Volume *
_e_fm_main_udisks_volume_add(const char *udi,
                             Eina_Bool   first_time)
{
   E_Volume *v;

   if (!udi) return NULL;
   if (e_volume_find(udi)) return NULL;
   v = calloc(1, sizeof(E_Volume));
   if (!v) return NULL;
//   printf("VOL+ %s\n", udi);
   v->efm_mode = EFM_MODE_USING_UDISKS_MOUNT;
   v->udi = eina_stringshare_add(udi);
   v->icon = NULL;
   v->first_time = first_time;
   _e_vols = eina_list_append(_e_vols, v);
   e_udisks_get_all_properties(_e_fm_main_udisks_conn, v->udi,
                                      (E_DBus_Callback_Func)_e_fm_main_udisks_cb_vol_prop, v);
   v->prop_handler = e_dbus_signal_handler_add(_e_fm_main_udisks_conn,
                                               E_UDISKS_BUS,
                                               udi,
                                               E_UDISKS_INTERFACE,
                                               "Changed",
                                               (E_DBus_Signal_Cb)_e_fm_main_udisks_cb_prop_modified, v);
   v->guard = NULL;

   return v;
}

void
_e_fm_main_udisks_volume_del(const char *udi)
{
   E_Volume *v;

   v = e_volume_find(udi);
   if (!v) return;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }
   if (v->prop_handler) e_dbus_signal_handler_del(_e_fm_main_udisks_conn, v->prop_handler);
   if (v->validated)
     {
        //	printf("--VOL %s\n", v->udi);
        /* FIXME: send event of storage volume (disk) removed */
           ecore_ipc_server_send(_e_fm_ipc_server,
                                 6 /*E_IPC_DOMAIN_FM*/,
                                 E_FM_OP_VOLUME_DEL,
                                 0, 0, 0, v->udi, eina_stringshare_strlen(v->udi) + 1);
     }
   _e_vols = eina_list_remove(_e_vols, v);
   _e_fm_shared_device_volume_free(v);
}

E_Volume *
_e_fm_main_udisks_volume_find(const char *udi)
{
   Eina_List *l;
   E_Volume *v;

   if (!udi) return NULL;
   EINA_LIST_FOREACH(_e_vols, l, v)
     {
        if (!v->udi) continue;
        if (!strcmp(udi, v->udi)) return v;
     }
   return NULL;
}

void
_e_fm_main_udisks_volume_eject(E_Volume *v)
{
   if (!v || v->guard) return;
   if (v->mounted)
     {
        v->guard = ecore_timer_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_unmount_timeout, v);
        v->op = e_udisks_volume_unmount(_e_fm_main_udisks_conn, v->udi, NULL);
     }
   else
     {
        v->guard = ecore_timer_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_eject_timeout, v);
        v->op = e_udisks_volume_eject(_e_fm_main_udisks_conn, v->udi, NULL);
     }
   v->optype = E_VOLUME_OP_TYPE_EJECT;
}

void
_e_fm_main_udisks_volume_unmount(E_Volume *v)
{
//   printf("unmount %s %s\n", v->udi, v->mount_point);
     if (!v || v->guard) return;

     v->guard = ecore_timer_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_unmount_timeout, v);
     v->op = e_udisks_volume_unmount(_e_fm_main_udisks_conn, v->udi, NULL);
     v->optype = E_VOLUME_OP_TYPE_UNMOUNT;
}

void
_e_fm_main_udisks_volume_mount(E_Volume *v)
{
   char buf[256];
   char buf2[256];
   Eina_List *opt = NULL;

   if ((!v) || (v->guard))
     return;

//   printf("mount %s %s [fs type = %s]\n", v->udi, v->mount_point, v->fstype);

   // for vfat and ntfs we want the uid mapped to the user mounting, if we can
   if ((!strcmp(v->fstype, "vfat")) ||
       (!strcmp(v->fstype, "ntfs"))
       )
     {
        snprintf(buf, sizeof(buf), "uid=%i", (int)getuid());
        opt = eina_list_append(opt, buf);
     }

   // force utf8 as the encoding - e only likes/handles utf8. its the
   // pseudo-standard these days anyway for "linux" for intl text to work
   // everywhere. problem is some fs's use differing options
   if ((!strcmp(v->fstype, "vfat")) ||
       (!strcmp(v->fstype, "ntfs")) ||
       (!strcmp(v->fstype, "iso9660"))
       )
     {
        snprintf(buf2, sizeof(buf2), "utf8");
        opt = eina_list_append(opt, buf2);
     }
   else if ((!strcmp(v->fstype, "fat")) ||
            (!strcmp(v->fstype, "jfs")) ||
            (!strcmp(v->fstype, "udf"))
            )
     {
        snprintf(buf2, sizeof(buf2), "iocharset=utf8");
        opt = eina_list_append(opt, buf2);
     }

   v->guard = ecore_timer_add(E_FM_MOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_mount_timeout, v);
// on ubuntu 10.04 if we request mount with opt - it fails. unknown why right
// now, but lets try without and maybe we need to try 2 mounts - one with
// opts and one without?   
//   v->op = e_udisks_volume_mount(_e_fm_main_udisks_conn, v->udi,
//                                        v->fstype, opt);
   v->op = e_udisks_volume_mount(_e_fm_main_udisks_conn, v->udi,
                                        v->fstype, NULL);
   eina_list_free(opt);
   v->optype = E_VOLUME_OP_TYPE_MOUNT;
}

void
_e_fm_main_udisks_init(void)
{
   DBusMessage *msg;

   e_dbus_init();
   e_ukit_init();
   _e_fm_main_udisks_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!_e_fm_main_udisks_conn) return;
   if (!_udisks_poll)
     _udisks_poll = e_dbus_signal_handler_add(_e_fm_main_udisks_conn,
                                              E_DBUS_FDO_BUS, E_DBUS_FDO_PATH,
                                              E_DBUS_FDO_INTERFACE,
                                              "NameOwnerChanged", _e_fm_main_udisks_poll, NULL);

   e_dbus_get_name_owner(_e_fm_main_udisks_conn, E_UDISKS_BUS, _e_fm_main_udisks_test, NULL); /* test for already running udisks */
   msg = dbus_message_new_method_call(E_UDISKS_BUS, E_UDISKS_PATH, E_UDISKS_BUS, "suuuuuup");
   e_dbus_method_call_send(_e_fm_main_udisks_conn, msg, NULL, (E_DBus_Callback_Func)_e_fm_main_udisks_test, NULL, -1, NULL); /* test for not running udisks */
   dbus_message_unref(msg);
}

void
_e_fm_main_udisks_shutdown(void)
{
   if (_e_fm_main_udisks_conn)
     e_dbus_connection_close(_e_fm_main_udisks_conn);
   e_ukit_shutdown();
   e_dbus_shutdown();
}

E_Storage *
_e_fm_main_udisks_storage_add(const char *udi)
{
   E_Storage *s;

   if (!udi) return NULL;
   if (e_storage_find(udi)) return NULL;
   s = calloc(1, sizeof(E_Storage));
   if (!s) return NULL;
   s->udi = eina_stringshare_add(udi);
   _e_stores = eina_list_append(_e_stores, s);
   e_udisks_get_all_properties(_e_fm_main_udisks_conn, s->udi,
                                      (E_DBus_Callback_Func)_e_fm_main_udisks_cb_store_prop, s);
   return s;
}

void
_e_fm_main_udisks_storage_del(const char *udi)
{
   E_Storage *s;

   s = e_storage_find(udi);
   if (!s) return;
   if (s->validated)
     {
//        printf("--STO %s\n", s->udi);
          ecore_ipc_server_send(_e_fm_ipc_server,
                                6 /*E_IPC_DOMAIN_FM*/,
                                E_FM_OP_STORAGE_DEL,
                                0, 0, 0, s->udi, strlen(s->udi) + 1);
     }
   _e_stores = eina_list_remove(_e_stores, s);
   _e_fm_shared_device_storage_free(s);
}

E_Storage *
_e_fm_main_udisks_storage_find(const char *udi)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (!strcmp(udi, s->udi)) return s;
     }
   return NULL;
}


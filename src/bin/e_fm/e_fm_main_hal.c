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
#include <E_Hal.h>

#include "e_fm_main.h"
#include "e_fm_main_hal.h"
#ifdef HAVE_UDISKS_MOUNT
# include "e_fm_main_udisks.h"
#endif

#include "e_fm_shared_codec.h"
#include "e_fm_shared_device.h"
#include "e_fm_ipc.h"
#include "e_fm_device.h"

static E_DBus_Signal_Handler *_hal_poll = NULL;
static E_DBus_Connection *_e_fm_main_hal_conn = NULL;
static Eina_List *_e_stores = NULL;

static void _e_fm_main_hal_cb_dev_all(void      *user_data,
                                      void      *reply_data,
                                      DBusError *error);
static void _e_fm_main_hal_cb_dev_store(void      *user_data,
                                        void      *reply_data,
                                        DBusError *error);
static void _e_fm_main_hal_cb_dev_vol(void      *user_data,
                                      void      *reply_data,
                                      DBusError *error);
static void _e_fm_main_hal_cb_store_is(void      *user_data,
                                       void      *reply_data,
                                       DBusError *error);
static void _e_fm_main_hal_cb_vol_is(void      *user_data,
                                     void      *reply_data,
                                     DBusError *error);
static void _e_fm_main_hal_cb_dev_add(void        *data,
                                      DBusMessage *msg);
static void _e_fm_main_hal_cb_dev_del(void        *data,
                                      DBusMessage *msg);
static void _e_fm_main_hal_cb_cap_add(void        *data,
                                      DBusMessage *msg);
static void _e_fm_main_hal_cb_prop_modified(void        *data,
                                            DBusMessage *msg);
static void _e_fm_main_hal_cb_store_prop(void      *data,
                                         void      *reply_data,
                                         DBusError *error);
static void _e_fm_main_hal_cb_vol_prop(void      *data,
                                       void      *reply_data,
                                       DBusError *error);
static void _e_fm_main_hal_cb_vol_prop_mount_modified(void      *data,
                                                      void      *reply_data,
                                                      DBusError *error);
static void _e_fm_main_hal_cb_vol_mounted(void      *user_data,
                                          void      *method_return,
                                          DBusError *error);
static void _e_fm_main_hal_cb_vol_unmounted(void      *user_data,
                                            void      *method_return,
                                            DBusError *error);
static void _e_fm_main_hal_cb_vol_unmounted_before_eject(void      *user_data,
                                                         void      *method_return,
                                                         DBusError *error);

static Eina_Bool _e_fm_main_hal_vb_vol_ejecting_after_unmount(void *data);
static void      _e_fm_main_hal_cb_vol_ejected(void      *user_data,
                                               void      *method_return,
                                               DBusError *error);
static int _e_fm_main_hal_format_error_msg(char     **buf,
                                           E_Volume  *v,
                                           DBusError *error);
static void _e_fm_main_hal_test(void        *data,
                                DBusMessage *msg,
                                DBusError   *error);
static void _e_fm_main_hal_poll(void        *data,
                                DBusMessage *msg);

static Eina_Bool _e_fm_main_hal_vol_mount_timeout(void *data);
static Eina_Bool _e_fm_main_hal_vol_unmount_timeout(void *data);
static Eina_Bool _e_fm_main_hal_vol_eject_timeout(void *data);

static void
_e_fm_main_hal_poll(void *data   __UNUSED__,
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
   if ((name) && !strcmp(name, E_HAL_SENDER))
     _e_fm_main_hal_test(NULL, NULL, NULL);
}

static void
_e_fm_main_hal_test(void *data       __UNUSED__,
                    DBusMessage *msg __UNUSED__,
                    DBusError       *error)
{
   if ((error) && (dbus_error_is_set(error)))
     {
        dbus_error_free(error);
        if (!_hal_poll)
          _hal_poll = e_dbus_signal_handler_add(_e_fm_main_hal_conn,
                                                E_DBUS_FDO_BUS, E_DBUS_FDO_PATH,
                                                E_DBUS_FDO_INTERFACE,
                                                "NameOwnerChanged", _e_fm_main_hal_poll, NULL);
#ifdef HAVE_UDISKS_MOUNT
        _e_fm_main_udisks_init(); /* check for udisks while listening for hal */
#else
        _e_fm_main_hal_catch(EINA_FALSE);
#endif
        return;
     }
   if (_hal_poll)
     e_dbus_signal_handler_del(_e_fm_main_hal_conn, _hal_poll);

   e_hal_manager_get_all_devices(_e_fm_main_hal_conn, _e_fm_main_hal_cb_dev_all, NULL);
   e_hal_manager_find_device_by_capability(_e_fm_main_hal_conn, "storage",
                                           _e_fm_main_hal_cb_dev_store, NULL);
   e_hal_manager_find_device_by_capability(_e_fm_main_hal_conn, "volume",
                                           _e_fm_main_hal_cb_dev_vol, NULL);

   e_dbus_signal_handler_add(_e_fm_main_hal_conn, E_HAL_SENDER,
                             E_HAL_MANAGER_PATH,
                             E_HAL_MANAGER_INTERFACE,
                             "DeviceAdded", _e_fm_main_hal_cb_dev_add, NULL);
   e_dbus_signal_handler_add(_e_fm_main_hal_conn, E_HAL_SENDER,
                             E_HAL_MANAGER_PATH,
                             E_HAL_MANAGER_INTERFACE,
                             "DeviceRemoved", _e_fm_main_hal_cb_dev_del, NULL);
   e_dbus_signal_handler_add(_e_fm_main_hal_conn, E_HAL_SENDER,
                             E_HAL_MANAGER_PATH,
                             E_HAL_MANAGER_INTERFACE,
                             "NewCapability", _e_fm_main_hal_cb_cap_add, NULL);
   _e_fm_main_hal_catch(EINA_TRUE); /* signal usage of HAL for mounting */
}

static void
_e_fm_main_hal_cb_dev_all(void *user_data __UNUSED__,
                          void           *reply_data,
                          DBusError      *error)
{
   E_Hal_Manager_Get_All_Devices_Return *ret = reply_data;
   Eina_List *l;
   char *udi;

   if (!ret || !ret->strings) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   EINA_LIST_FOREACH(ret->strings, l, udi)
     {
//	printf("DB INIT DEV+: %s\n", udi);
          e_hal_device_query_capability(_e_fm_main_hal_conn, udi, "storage",
                                        _e_fm_main_hal_cb_store_is, (void *)eina_stringshare_add(udi));
          e_hal_device_query_capability(_e_fm_main_hal_conn, udi, "volume",
                                        _e_fm_main_hal_cb_vol_is, (void *)eina_stringshare_add(udi));
     }
}

static void
_e_fm_main_hal_cb_dev_store(void *user_data __UNUSED__,
                            void           *reply_data,
                            DBusError      *error)
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
   Eina_List *l;
   char *device;

   if (!ret || !ret->strings) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   EINA_LIST_FOREACH(ret->strings, l, device)
     {
//	printf("DB STORE+: %s\n", device);
          _e_fm_main_hal_storage_add(device);
     }
}

static void
_e_fm_main_hal_cb_dev_vol(void *user_data __UNUSED__,
                          void           *reply_data,
                          DBusError      *error)
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
   Eina_List *l;
   char *device;

   if (!ret || !ret->strings) return;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }

   EINA_LIST_FOREACH(ret->strings, l, device)
     {
//	printf("DB VOL+: %s\n", device);
          _e_fm_main_hal_volume_add(device, 1);
     }
}

static void
_e_fm_main_hal_cb_store_is(void      *user_data,
                           void      *reply_data,
                           DBusError *error)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret = reply_data;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   if (ret && ret->boolean)
     {
//	printf("DB STORE IS+: %s\n", udi);
          _e_fm_main_hal_storage_add(udi);
     }

error:
   eina_stringshare_del(udi);
}

static void
_e_fm_main_hal_cb_vol_is(void      *user_data,
                         void      *reply_data,
                         DBusError *error)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret = reply_data;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   if (ret && ret->boolean)
     {
//	printf("DB VOL IS+: %s\n", udi);
          _e_fm_main_hal_volume_add(udi, 0);
     }

error:
   eina_stringshare_del(udi);
}

static void
_e_fm_main_hal_cb_dev_add(void *data   __UNUSED__,
                          DBusMessage *msg)
{
   DBusError err;
   char *udi = NULL;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   e_hal_device_query_capability(_e_fm_main_hal_conn, udi, "storage",
                                 _e_fm_main_hal_cb_store_is, (void *)eina_stringshare_add(udi));
   e_hal_device_query_capability(_e_fm_main_hal_conn, udi, "volume",
                                 _e_fm_main_hal_cb_vol_is, (void *)eina_stringshare_add(udi));
}

static void
_e_fm_main_hal_cb_dev_del(void *data   __UNUSED__,
                          DBusMessage *msg)
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);

   dbus_message_get_args(msg,
                         &err, DBUS_TYPE_STRING,
                         &udi, DBUS_TYPE_INVALID);
//   printf("DB DEV-: %s\n", udi);
   _e_fm_main_hal_storage_del(udi);
   _e_fm_main_hal_volume_del(udi);
}

static void
_e_fm_main_hal_cb_cap_add(void *data   __UNUSED__,
                          DBusMessage *msg)
{
   DBusError err;
   char *udi, *capability;

   dbus_error_init(&err);

   dbus_message_get_args(msg,
                         &err, DBUS_TYPE_STRING,
                         &udi, DBUS_TYPE_STRING,
                         &capability, DBUS_TYPE_INVALID);
   if (!strcmp(capability, "storage"))
     {
//        printf("DB STORE CAP+: %s\n", udi);
          _e_fm_main_hal_storage_add(udi);
     }
}

static void
_e_fm_main_hal_cb_prop_modified(void        *data,
                                DBusMessage *msg)
{
   E_Volume *v;
   DBusMessageIter iter, sub, subsub;
   struct
   {
      const char *name;
      int         added;
      int         removed;
   } prop;
   int num_changes = 0, i;

   if (!(v = data)) return;

   if (dbus_message_get_error_name(msg))
     {
        printf("DBUS ERROR: %s\n", dbus_message_get_error_name(msg));
        return;
     }
   if (!dbus_message_iter_init(msg, &iter)) return;

   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) return;
   dbus_message_iter_get_basic(&iter, &num_changes);
   if (num_changes == 0) return;

   dbus_message_iter_next(&iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) return;
   dbus_message_iter_recurse(&iter, &sub);

   for (i = 0; i < num_changes; i++, dbus_message_iter_next(&sub))
     {
        dbus_message_iter_recurse(&sub, &subsub);

        if (dbus_message_iter_get_arg_type(&subsub) != DBUS_TYPE_STRING) break;
        dbus_message_iter_get_basic(&subsub, &(prop.name));
        if (!strcmp(prop.name, "volume.mount_point"))
          {
             e_hal_device_get_all_properties(_e_fm_main_hal_conn, v->udi,
                                             _e_fm_main_hal_cb_vol_prop_mount_modified,
                                             v);
             return;
          }

        dbus_message_iter_next(&subsub);
        dbus_message_iter_next(&subsub);
     }
}

static void
_e_fm_main_hal_cb_store_prop(void      *data,
                             void      *reply_data,
                             DBusError *error)
{
   E_Storage *s = data;
   E_Hal_Properties *ret = reply_data;
   int err = 0;

   if (!ret) goto error;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   s->bus = e_hal_property_string_get(ret, "storage.bus", &err);
   if (err) goto error;
   s->bus = eina_stringshare_add(s->bus);
   s->drive_type = e_hal_property_string_get(ret, "storage.drive_type", &err);
   if (err) goto error;
   s->drive_type = eina_stringshare_add(s->drive_type);
   s->model = e_hal_property_string_get(ret, "storage.model", &err);
   if (err) goto error;
   s->model = eina_stringshare_add(s->model);
   s->vendor = e_hal_property_string_get(ret, "storage.vendor", &err);
   if (err) goto error;
   s->vendor = eina_stringshare_add(s->vendor);
   s->serial = e_hal_property_string_get(ret, "storage.serial", &err);
//   if (err) goto error;
   if (err) printf("Error getting serial for %s\n", s->udi);
   s->serial = eina_stringshare_add(s->serial);

   s->removable = e_hal_property_bool_get(ret, "storage.removable", &err);

   if (s->removable)
     {
        s->media_available = e_hal_property_bool_get(ret, "storage.removable.media_available", &err);
        s->media_size = e_hal_property_uint64_get(ret, "storage.removable.media_size", &err);
     }

   s->requires_eject = e_hal_property_bool_get(ret, "storage.requires_eject", &err);
   s->hotpluggable = e_hal_property_bool_get(ret, "storage.hotpluggable", &err);
   s->media_check_enabled = e_hal_property_bool_get(ret, "storage.media_check_enabled", &err);

   s->icon.drive = e_hal_property_string_get(ret, "storage.icon.drive", &err);
   s->icon.drive = eina_stringshare_add(s->icon.drive);
   s->icon.volume = e_hal_property_string_get(ret, "storage.icon.volume", &err);
   s->icon.volume = eina_stringshare_add(s->icon.volume);

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
   _e_fm_main_hal_storage_del(s->udi);
}

static void
_e_fm_main_hal_cb_vol_prop(void      *data,
                           void      *reply_data,
                           DBusError *error)
{
   E_Volume *v = data;
   E_Storage *s = NULL;
   E_Hal_Device_Get_All_Properties_Return *ret = reply_data;
   int err = 0;
   const char *str = NULL;

   if (!ret) goto error;
   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        goto error;
     }

   /* skip volumes with volume.ignore set */
   if (e_hal_property_bool_get(ret, "volume.ignore", &err) || err)
     goto error;

   /* skip volumes that aren't filesystems */
   str = e_hal_property_string_get(ret, "volume.fsusage", &err);
   if (err || !str) goto error;
   if (strcmp(str, "filesystem")) goto error;
   str = NULL;

   v->uuid = e_hal_property_string_get(ret, "volume.uuid", &err);
   if (err) goto error;
   v->uuid = eina_stringshare_add(v->uuid);

   v->label = e_hal_property_string_get(ret, "volume.label", &err);
//   if (err) goto error;
   v->label = eina_stringshare_add(v->label);

   v->fstype = e_hal_property_string_get(ret, "volume.fstype", &err);
//   if (err) goto error;
   v->fstype = eina_stringshare_add(v->fstype);

   v->size = e_hal_property_uint64_get(ret, "volume.size", &err);

   v->mounted = e_hal_property_bool_get(ret, "volume.is_mounted", &err);
   if (err) goto error;

   v->partition = e_hal_property_bool_get(ret, "volume.is_partition", &err);
   if (err) goto error;

   v->mount_point = e_hal_property_string_get(ret, "volume.mount_point", &err);
   if (err) goto error;
   v->mount_point = eina_stringshare_add(v->mount_point);

   if (v->partition)
     {
        v->partition_number = e_hal_property_int_get(ret, "volume.partition.number", NULL);
        v->partition_label = e_hal_property_string_get(ret, "volume.partition.label", NULL);
        v->partition_label = eina_stringshare_add(v->partition_label);
     }

   v->parent = e_hal_property_string_get(ret, "info.parent", &err);
   if ((!err) && (v->parent))
     {
        s = e_storage_find(v->parent);
        if (s)
          {
             v->storage = s;
             s->volumes = eina_list_append(s->volumes, v);
          }
     }
   v->parent = eina_stringshare_add(v->parent);

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
   _e_fm_main_hal_volume_del(v->udi);
   return;
}

static int
_e_fm_main_hal_format_error_msg(char     **buf,
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

static void
_e_fm_main_hal_cb_vol_prop_mount_modified(void      *data,
                                          void      *reply_data,
                                          DBusError *error)
{
   E_Volume *v = data;
   E_Hal_Device_Get_All_Properties_Return *ret = reply_data;
   int err = 0;

   if (!ret) return;
   if (dbus_error_is_set(error))
     {
        char *buf;
        int size;

        size = _e_fm_main_hal_format_error_msg(&buf, v, error);
        if (v->mounted)
          ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                                0, 0, 0, buf, size);
        else
          ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                                0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        return;
     }

   v->mounted = e_hal_property_bool_get(ret, "volume.is_mounted", &err);
   if (err) printf("HAL Error : can't get volume.is_mounted property");

   if (v->mount_point) eina_stringshare_del(v->mount_point);
   v->mount_point = e_hal_property_string_get(ret, "volume.mount_point", &err);
   if (err) printf("HAL Error : can't get volume.is_mount_point property");
   v->mount_point = eina_stringshare_add(v->mount_point);

//   printf("**VOL udi: %s mount_point: %s mounted: %d\n", v->udi, v->mount_point, v->mounted);
   {
      char *buf;
      int size;

      size = strlen(v->udi) + 1 + strlen(v->mount_point) + 1;
      buf = alloca(size);
      strcpy(buf, v->udi);
      strcpy(buf + strlen(buf) + 1, v->mount_point);
      if (v->mounted)
        ecore_ipc_server_send(_e_fm_ipc_server,
                              6 /*E_IPC_DOMAIN_FM*/,
                              E_FM_OP_MOUNT_DONE,
                              0, 0, 0, buf, size);
      else
        ecore_ipc_server_send(_e_fm_ipc_server,
                              6 /*E_IPC_DOMAIN_FM*/,
                              E_FM_OP_UNMOUNT_DONE,
                              0, 0, 0, buf, size);
   }
   return;
}

static Eina_Bool
_e_fm_main_hal_vol_mount_timeout(void *data)
{
   E_Volume *v = data;
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   error.name = "org.enlightenment.fm2.MountTimeout";
   error.message = "Unable to mount the volume with specified time-out.";
   size = _e_fm_main_hal_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_hal_cb_vol_mounted(void               *user_data,
                              void *method_return __UNUSED__,
                              DBusError          *error)
{
   E_Volume *v = user_data;
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (dbus_error_is_set(error))
     {
        size = _e_fm_main_hal_format_error_msg(&buf, v, error);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                              0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        return;
     }

#if 0
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
#endif
}

static Eina_Bool
_e_fm_main_hal_vol_unmount_timeout(void *data)
{
   E_Volume *v = data;
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   error.name = "org.enlightenment.fm2.UnmountTimeout";
   error.message = "Unable to unmount the volume with specified time-out.";
   size = _e_fm_main_hal_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_hal_cb_vol_unmounted(void               *user_data,
                                void *method_return __UNUSED__,
                                DBusError          *error)
{
   E_Volume *v = user_data;
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (dbus_error_is_set(error))
     {
        size = _e_fm_main_hal_format_error_msg(&buf, v, error);
        ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                              0, 0, 0, buf, size);
        dbus_error_free(error);
        free(buf);
        return;
     }

#if 0
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
#endif
}

static Eina_Bool
_e_fm_main_hal_vol_eject_timeout(void *data)
{
   E_Volume *v = data;
   DBusError error;
   char *buf;
   int size;

   v->guard = NULL;
   dbus_pending_call_cancel(v->op);
   error.name = "org.enlightenment.fm2.EjectTimeout";
   error.message = "Unable to eject the media with specified time-out.";
   size = _e_fm_main_hal_format_error_msg(&buf, v, &error);
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_fm_main_hal_vb_vol_ejecting_after_unmount(void *data)
{
   E_Volume *v = data;

   v->guard = ecore_timer_add(E_FM_EJECT_TIMEOUT, _e_fm_main_hal_vol_eject_timeout, v);
   v->op = e_hal_device_volume_eject(_e_fm_main_hal_conn, v->udi, NULL,
                                     _e_fm_main_hal_cb_vol_ejected, v);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_hal_cb_vol_unmounted_before_eject(void      *user_data,
                                             void      *method_return,
                                             DBusError *error)
{
   E_Volume *v = user_data;
   char err;

   err = dbus_error_is_set(error) ? 1 : 0;
   _e_fm_main_hal_cb_vol_unmounted(user_data, method_return, error);

   // delay is required for all message handlers were executed after unmount
   if (!err)
     ecore_timer_add(1.0, _e_fm_main_hal_vb_vol_ejecting_after_unmount, v);
}

static void
_e_fm_main_hal_cb_vol_ejected(void               *user_data,
                              void *method_return __UNUSED__,
                              DBusError          *error)
{
   E_Volume *v = user_data;
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (dbus_error_is_set(error))
     {
        size = _e_fm_main_hal_format_error_msg(&buf, v, error);
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

static Eina_List *_e_vols = NULL;

E_Volume *
_e_fm_main_hal_volume_add(const char *udi,
                          Eina_Bool   first_time)
{
   E_Volume *v;

   if (!udi) return NULL;
   if (e_volume_find(udi)) return NULL;
   v = calloc(1, sizeof(E_Volume));
   if (!v) return NULL;
//   printf("VOL+ %s\n", udi);
   v->udi = eina_stringshare_add(udi);
   v->efm_mode = EFM_MODE_USING_HAL_MOUNT;
   v->icon = NULL;
   v->first_time = first_time;
   _e_vols = eina_list_append(_e_vols, v);
   e_hal_device_get_all_properties(_e_fm_main_hal_conn, v->udi,
                                   _e_fm_main_hal_cb_vol_prop, v);
   v->prop_handler = e_dbus_signal_handler_add(_e_fm_main_hal_conn,
                                               E_HAL_SENDER,
                                               udi,
                                               E_HAL_DEVICE_INTERFACE,
                                               "PropertyModified",
                                               _e_fm_main_hal_cb_prop_modified, v);
   v->guard = NULL;

   return v;
}

void
_e_fm_main_hal_volume_del(const char *udi)
{
   E_Volume *v;

   v = e_volume_find(udi);
   if (!v) return;
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }
   if (v->prop_handler) e_dbus_signal_handler_del(_e_fm_main_hal_conn, v->prop_handler);
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
_e_fm_main_hal_volume_find(const char *udi)
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
_e_fm_main_hal_volume_eject(E_Volume *v)
{
   if (!v || v->guard) return;
   if (v->mounted)
     {
        v->guard = ecore_timer_add(E_FM_UNMOUNT_TIMEOUT, _e_fm_main_hal_vol_unmount_timeout, v);
        v->op = e_hal_device_volume_unmount(_e_fm_main_hal_conn, v->udi, NULL,
                                            _e_fm_main_hal_cb_vol_unmounted_before_eject, v);
     }
   else
     {
        v->guard = ecore_timer_add(E_FM_EJECT_TIMEOUT, _e_fm_main_hal_vol_eject_timeout, v);
        v->op = e_hal_device_volume_eject(_e_fm_main_hal_conn, v->udi, NULL,
                                          _e_fm_main_hal_cb_vol_ejected, v);
     }
}

void
_e_fm_main_hal_volume_unmount(E_Volume *v)
{
//   printf("unmount %s %s\n", v->udi, v->mount_point);
     if (!v || v->guard) return;

     v->guard = ecore_timer_add(E_FM_UNMOUNT_TIMEOUT, _e_fm_main_hal_vol_unmount_timeout, v);
     v->op = e_hal_device_volume_unmount(_e_fm_main_hal_conn, v->udi, NULL,
                                         _e_fm_main_hal_cb_vol_unmounted, v);
}

void
_e_fm_main_hal_volume_mount(E_Volume *v)
{
   char buf[256];
   char buf2[256];
   const char *mount_point;
   Eina_List *opt = NULL;

   if ((!v) || (v->guard) || (!v->mount_point) ||
       (strncmp(v->mount_point, "/media/", 7)))
     return;

   mount_point = v->mount_point + 7;
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

   v->guard = ecore_timer_add(E_FM_MOUNT_TIMEOUT,
                              _e_fm_main_hal_vol_mount_timeout, v);
   v->op = e_hal_device_volume_mount(_e_fm_main_hal_conn, v->udi, mount_point,
                                     v->fstype, opt,
                                     _e_fm_main_hal_cb_vol_mounted, v);
   eina_list_free(opt);
}

void
_e_fm_main_hal_init(void)
{
   e_dbus_init();
   e_hal_init();
   _e_fm_main_hal_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   /* previously, this assumed that if dbus was running, hal was running. */
   if (_e_fm_main_hal_conn)
     e_dbus_get_name_owner(_e_fm_main_hal_conn, E_HAL_SENDER, _e_fm_main_hal_test, NULL);
}

void
_e_fm_main_hal_shutdown(void)
{
   if (_e_fm_main_hal_conn)
     e_dbus_connection_close(_e_fm_main_hal_conn);
   e_hal_shutdown();
   e_dbus_shutdown();
}

E_Storage *
_e_fm_main_hal_storage_add(const char *udi)
{
   E_Storage *s;

   if (!udi) return NULL;
   if (e_storage_find(udi)) return NULL;
   s = calloc(1, sizeof(E_Storage));
   if (!s) return NULL;
   s->udi = eina_stringshare_add(udi);
   _e_stores = eina_list_append(_e_stores, s);
   e_hal_device_get_all_properties(_e_fm_main_hal_conn, s->udi,
                                   _e_fm_main_hal_cb_store_prop, s);
   return s;
}

void
_e_fm_main_hal_storage_del(const char *udi)
{
   E_Storage *s;

   s = e_storage_find(udi);
   if (!s) return;
   if (s->validated)
     {
        //	printf("--STO %s\n", s->udi);
          ecore_ipc_server_send(_e_fm_ipc_server,
                                6 /*E_IPC_DOMAIN_FM*/,
                                E_FM_OP_STORAGE_DEL,
                                0, 0, 0, s->udi, strlen(s->udi) + 1);
     }
   _e_stores = eina_list_remove(_e_stores, s);
   _e_fm_shared_device_storage_free(s);
}

E_Storage *
_e_fm_main_hal_storage_find(const char *udi)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (!strcmp(udi, s->udi)) return s;
     }
   return NULL;
}


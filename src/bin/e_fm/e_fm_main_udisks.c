#include "config.h"

#include "e_fm_shared_device.h"
#include "e_fm_shared_codec.h"
#include "e_fm_ipc.h"

#include "e_macros.h"
#include "e_fm_main_udisks.h"
#include "e_fm_main.h"

#define E_TYPEDEFS
#include "e_fm_op.h"

#define UDISKS_BUS "org.freedesktop.UDisks"
#define UDISKS_PATH "/org/freedesktop/UDisks"
#define UDISKS_INTERFACE "org.freedesktop.UDisks"
#define UDISKS_DEVICE_INTERFACE "org.freedesktop.UDisks.Device"

static Eldbus_Connection *_e_fm_main_udisks_conn = NULL;
static Eldbus_Proxy *_e_fm_main_udisks_proxy = NULL;
static Eina_List *_e_stores = NULL;
static Eina_List *_e_vols = NULL;

static void _e_fm_main_udisks_cb_dev_all(void *data, const Eldbus_Message *msg,
                                         Eldbus_Pending *pending);
static void _e_fm_main_udisks_cb_dev_verify(void *data, const Eldbus_Message *msg,
                                            Eldbus_Pending *pending);
static void _e_fm_main_udisks_cb_dev_add(void *data, const Eldbus_Message *msg);
static void _e_fm_main_udisks_cb_dev_del(void *data, const Eldbus_Message *msg);
static void _e_fm_main_udisks_cb_prop_modified(void *data, const Eldbus_Message *msg);
static void _e_fm_main_udisks_cb_store_prop(void *data, const Eldbus_Message *msg,
                                            Eldbus_Pending *pending);
static void _e_fm_main_udisks_cb_vol_prop(void *data, const Eldbus_Message *msg,
                                          Eldbus_Pending *pending);
static void _e_fm_main_udisks_cb_vol_mounted(E_Volume *v);
static void _e_fm_main_udisks_cb_vol_unmounted(E_Volume *v);
static void _e_fm_main_udisks_cb_vol_unmounted_before_eject(E_Volume *v);

static Eina_Bool _e_fm_main_udisks_cb_vol_ejecting_after_unmount(E_Volume *v);
static void      _e_fm_main_udisks_cb_vol_ejected(E_Volume *v);
static int _e_fm_main_udisks_format_error_msg(char     **buf,
                                              E_Volume  *v,
                                              const char *name,
                                              const char *message);

static Eina_Bool _e_fm_main_udisks_vol_mount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks_vol_unmount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks_vol_eject_timeout(E_Volume *v);
static E_Storage *_storage_find_by_dbus_path(const char *path);
static E_Volume *_volume_find_by_dbus_path(const char *path);
static void _volume_del(E_Volume *v);

static void
_e_fm_main_udisks_name_start(void *data EINA_UNUSED, const Eldbus_Message *msg,
                             Eldbus_Pending *pending EINA_UNUSED)
{
   unsigned flag = 0;
   Eldbus_Object *obj;

   if (!eldbus_message_arguments_get(msg, "u", &flag) || !flag)
     {
        _e_fm_main_udisks_catch(EINA_FALSE);
        return;
     }
   obj = eldbus_object_get(_e_fm_main_udisks_conn, UDISKS_BUS, UDISKS_PATH);
   _e_fm_main_udisks_proxy = eldbus_proxy_get(obj, UDISKS_INTERFACE);

   eldbus_proxy_call(_e_fm_main_udisks_proxy, "EnumerateDevices",
                    _e_fm_main_udisks_cb_dev_all, NULL, -1, "");

   eldbus_proxy_signal_handler_add(_e_fm_main_udisks_proxy, "DeviceAdded",
                                  _e_fm_main_udisks_cb_dev_add, NULL);
   eldbus_proxy_signal_handler_add(_e_fm_main_udisks_proxy, "DeviceRemoved",
                                  _e_fm_main_udisks_cb_dev_del, NULL);
   _e_fm_main_udisks_catch(EINA_TRUE); /* signal usage of udisks for mounting */
}

static void
_e_fm_main_udisks_cb_dev_all(void *data EINA_UNUSED, const Eldbus_Message *msg,
                             Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *txt, *path;
   Eldbus_Message_Iter *array;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("Error %s %s.", name, txt);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "ao", &array))
     {
        ERR("Error getting arguments.");
        return;
     }

   while (eldbus_message_iter_get_and_next(array, 'o', &path))
     {
        Eldbus_Message *new_msg;
        new_msg = eldbus_message_method_call_new(UDISKS_BUS, path,
                                                ELDBUS_FDO_INTERFACE_PROPERTIES, "Get");
        eldbus_message_arguments_append(new_msg, "ss", UDISKS_DEVICE_INTERFACE, "IdUsage");
        eldbus_connection_send(_e_fm_main_udisks_conn, new_msg,
                              _e_fm_main_udisks_cb_dev_verify,
                              eina_stringshare_add(path), -1);
        INF("DB INIT DEV+: %s", path);
     }
}


static void
_e_fm_main_udisks_cb_dev_verify(void *data, const Eldbus_Message *msg,
                                Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *txt, *id_usage, *path = data;
   Eldbus_Message_Iter *variant;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("Error %s %s.", name, txt);
        goto error;
     }

   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        goto error;
     }

   if (!eldbus_message_iter_arguments_get(variant, "s", &id_usage))
     {
        ERR("Type of variant not expected");
        goto error;
     }

   if (!id_usage[0])
     _e_fm_main_udisks_storage_add(path);
   else if(!strcmp(id_usage, "filesystem"))
     _e_fm_main_udisks_volume_add(path, EINA_TRUE);
   else
     eina_stringshare_del(path);
   return;
error:
   eina_stringshare_del(path);
}

static void
_e_fm_main_udisks_cb_dev_verify_added(void *data, const Eldbus_Message *msg,
                                Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *txt, *id_usage, *path = data;
   Eldbus_Message_Iter *variant;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("Error %s %s.", name, txt);
        goto error;
     }

   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        goto error;
     }

   if (!eldbus_message_iter_arguments_get(variant, "s", &id_usage))
     {
        ERR("Type of variant not expected");
        goto error;
     }

   DBG("%s usage=%s", path, id_usage);
   if (!id_usage[0])
     {
        E_Storage *s;
        s = _storage_find_by_dbus_path(path);
        if (!s)
          _e_fm_main_udisks_storage_add(path);
        else
          eldbus_proxy_property_get_all(s->proxy,
                                       _e_fm_main_udisks_cb_store_prop, s);
     }
   else if(!strcmp(id_usage, "filesystem"))
     {
        E_Volume *v;
        v = _volume_find_by_dbus_path(path);
        if (!v)
          _e_fm_main_udisks_volume_add(path, EINA_TRUE);
        else
          eldbus_proxy_property_get_all(v->proxy,
                                       _e_fm_main_udisks_cb_vol_prop, v);
     }
   else
     eina_stringshare_del(path);
   return;
error:
   eina_stringshare_del(path);
}

static void
_e_fm_main_udisks_cb_dev_add(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
   Eldbus_Message *new;
   E_Volume *v;
   char *path;

   if (!eldbus_message_arguments_get(msg, "o", &path))
     return;
   DBG("DB DEV+: %s", path);

   v = _volume_find_by_dbus_path(path);
   if (v)
     {
        eldbus_proxy_property_get_all(v->proxy, _e_fm_main_udisks_cb_vol_prop, v);
        return;
     }

   new = eldbus_message_method_call_new(UDISKS_BUS, path, ELDBUS_FDO_INTERFACE_PROPERTIES, "Get");
   eldbus_message_arguments_append(new, "ss", UDISKS_DEVICE_INTERFACE, "IdUsage");
   eldbus_connection_send(_e_fm_main_udisks_conn, new,
                         _e_fm_main_udisks_cb_dev_verify_added,
                         eina_stringshare_add(path), -1);
}

static void
_e_fm_main_udisks_cb_dev_del(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
   char *path;
   E_Volume *v;

   if (!eldbus_message_arguments_get(msg, "o", &path))
     return;
   DBG("DB DEV-: %s", path);
   if ((v = _volume_find_by_dbus_path(path)))
     {
        if (v->optype == E_VOLUME_OP_TYPE_EJECT)
          _e_fm_main_udisks_cb_vol_ejected(v);
     }
   _e_fm_main_udisks_volume_del(path);
   _e_fm_main_udisks_storage_del(path);
}

static void
_e_fm_main_udisks_cb_prop_modified(void *data,
                                   const Eldbus_Message *msg EINA_UNUSED)
{
   E_Volume *v = data;
   eldbus_proxy_property_get_all(v->proxy, _e_fm_main_udisks_cb_vol_prop, v);
}

static Eina_Bool
_storage_del(void *data)
{
   const char *path = data;
   _e_fm_main_udisks_storage_del(path);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_store_prop(void *data, const Eldbus_Message *msg,
                                Eldbus_Pending *pending EINA_UNUSED)
{
   E_Storage *s = data;
   const char *name, *txt;
   Eldbus_Message_Iter *dict, *entry;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("Error %s %s.", name, txt);
        return;
     }
   if (!eldbus_message_arguments_get(msg, "a{sv}", &dict))
     {
        ERR("Error getting arguments.");
        return;
     }

   while (eldbus_message_iter_get_and_next(dict, 'e', &entry))
     {
        char *key;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &var))
          continue;
        if (!strcmp(key, "DeviceFile"))
          {
             const char *udi;
             if (eldbus_message_iter_arguments_get(var, "s", &udi))
               eina_stringshare_replace(&s->udi, udi);
          }
        else if (!strcmp(key, "DriveConnectionInterface"))
          {
             const char *bus;
             if (eldbus_message_iter_arguments_get(var, "s", &bus))
               {
                  if (s->bus)
                    eina_stringshare_del(bus);
                  s->bus = eina_stringshare_add(bus);
               }
          }
        else if (!strcmp(key, "DriveMediaCompatibility"))
          {
             Eldbus_Message_Iter *inner_array;
             const char *media;

             if (!eldbus_message_iter_arguments_get(var, "as", &inner_array))
               continue;

             while(eldbus_message_iter_get_and_next(inner_array, 's', &media))
               {
                  eina_stringshare_replace(&s->drive_type, media);
                  break;
               }
          }
        else if (!strcmp(key, "DriveModel"))
          {
             const char *model;
             if (eldbus_message_iter_arguments_get(var, "s", &model))
               eina_stringshare_replace(&s->model, model);
          }
        else if (!strcmp(key, "DriveVendor"))
           {
              const char *vendor;
              if (eldbus_message_iter_arguments_get(var, "s", &vendor))
                {
                   if (s->vendor)
                     eina_stringshare_del(s->vendor);
                   s->vendor = eina_stringshare_add(vendor);
                }
           }
        else if (!strcmp(key, "DriveSerial"))
           {
              const char *serial;
              if (eldbus_message_iter_arguments_get(var, "s", &serial))
                {
                   if (s->serial)
                     eina_stringshare_del(s->serial);
                   s->serial = eina_stringshare_add(serial);
                }
           }
        else if (!strcmp(key, "DeviceIsSystemInternal"))
          eldbus_message_iter_arguments_get(var, "b", &s->system_internal);
        else if (!strcmp(key, "DeviceIsMediaAvailable"))
          eldbus_message_iter_arguments_get(var, "b", &s->media_available);
        else if (!strcmp(key, "DeviceSize"))
          eldbus_message_iter_arguments_get(var, "t", &s->media_size);
        else if (!strcmp(key, "DriveIsMediaEjectable"))
          eldbus_message_iter_arguments_get(var, "b", &s->requires_eject);
        else if (!strcmp(key, "DriveCanDetach"))
          eldbus_message_iter_arguments_get(var, "b", &s->hotpluggable);
        else if (!strcmp(key, "DeviceIsMediaChangeDetectionInhibited"))
          eldbus_message_iter_arguments_get(var, "b", &s->media_check_enabled);
        else if (!strcmp(key, "DevicePresentationIconName"))
          {
             const char *icon;
             if (eldbus_message_iter_arguments_get(var, "s", &icon))
               {
                  if (s->icon.drive)
                    eina_stringshare_del(s->icon.drive);
                  s->icon.drive = NULL;
                  if (icon[0])
                    s->icon.drive = eina_stringshare_add(icon);
                  s->icon.volume = s->icon.drive;
               }
          }
     }
   if (!s->udi)
     {
        ERR("removing storage with null udi %s", s->dbus_path);
        ecore_idler_add(_storage_del, s->dbus_path);
        return;
     }
   if (s->system_internal)
     {
        DBG("removing storage internal %s", s->udi);
        ecore_idler_add(_storage_del, s->udi);
        return;
     }
   /* force it to be removable if it passed the above tests */
   s->removable = EINA_TRUE;
   INF("++STO:\n  udi: %s\n  bus: %s\n  drive_type: %s\n  model: %s\n  vendor: %s\n  serial: %s\n  icon.drive: %s\n  icon.volume: %s\n", s->udi, s->bus, s->drive_type, s->model, s->vendor, s->serial, s->icon.drive, s->icon.volume);
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
}

static Eina_Bool
_idler_volume_del(void *data)
{
   const char *path = data;
   _e_fm_main_udisks_volume_del(path);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_prop(void *data, const Eldbus_Message *msg,
                              Eldbus_Pending *pending EINA_UNUSED)
{
   E_Volume *v = data;
   E_Storage *s = NULL;
   const char *txt, *message;
   Eldbus_Message_Iter *array, *dict;

   DBG("volume=%s",v->dbus_path);

   if (eldbus_message_error_get(msg, &txt, &message))
     {
        ERR("Error: %s %s\nVolume: %s", txt, message, v->udi);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "a{sv}", &array))
     {
        ERR("Error getting values.");
        return;
     }

   while (eldbus_message_iter_get_and_next(array, 'e', &dict))
     {
        Eldbus_Message_Iter *var;
        const char *key;

        if (!eldbus_message_iter_arguments_get(dict, "sv", &key, &var))
          continue;
        if (!strcmp(key, "DeviceFile"))
          {
             const char *udi;
             if (!eldbus_message_iter_arguments_get(var, "s", &udi))
               continue;
             if (udi && v->first_time)
               eina_stringshare_replace(&v->udi, udi);
          }
        else if (!strcmp(key, "DeviceIsSystemInternal"))
          {
             Eina_Bool internal;
             eldbus_message_iter_arguments_get(var, "b", &internal);
             if (internal)
               {
                  DBG("removing is internal %s", v->dbus_path);
                  ecore_idler_add(_idler_volume_del, v->dbus_path);
                  return;
               }
          }
        else if (!strcmp(key, "DevicePresentationHide"))
          {
             Eina_Bool hid;
             eldbus_message_iter_arguments_get(var, "b", &hid);
             if (hid)
               {
                  DBG("removing is hidden %s", v->dbus_path);
                  ecore_idler_add(_idler_volume_del, v->dbus_path);
                  return;
               }
          }
        else if (!strcmp(key, "DeviceIsMediaChangeDetectionInhibited"))
          {
             Eina_Bool inibited;
             eldbus_message_iter_arguments_get(var, "b", &inibited);
             if (inibited)
               {
                  /* skip volumes with volume.ignore set */
                  DBG("removing is inibited %s", v->dbus_path);
                  ecore_idler_add(_idler_volume_del, v->dbus_path);
                  return;
               }
          }
        else if (!strcmp(key, "DeviceIsLuks"))
          eldbus_message_iter_arguments_get(var, "b", &v->encrypted);
        else if (!strcmp(key, "IdUuid"))
          {
             const char *uuid;
             if (!eldbus_message_iter_arguments_get(var, "s", &uuid))
               continue;
             eina_stringshare_replace(&v->uuid, uuid);
          }
        else if (!strcmp(key, "IdLabel"))
          {
             const char *label;
             if (!eldbus_message_iter_arguments_get(var, "s", &label))
               continue;
             eina_stringshare_replace(&v->label, label);
          }
        else if (!strcmp(key, "DeviceMountPaths"))
          {
             Eldbus_Message_Iter *inner_array;
             const char *path;

             if (!eldbus_message_iter_arguments_get(var, "as", &inner_array))
               continue;

             while (eldbus_message_iter_get_and_next(inner_array, 's', &path))
               {
                  eina_stringshare_replace(&v->mount_point, path);
                  break;
               }
          }
        else if (!strcmp(key, "IdType"))
          {
             const char *type;
             if (!eldbus_message_iter_arguments_get(var, "s", &type))
               continue;
             eina_stringshare_replace(&v->fstype, type);
          }
        else if (!strcmp(key, "DeviceSize"))
          eldbus_message_iter_arguments_get(var, "t", &v->size);
        else if (!strcmp(key, "DeviceIsMounted"))
          eldbus_message_iter_arguments_get(var, "b", &v->mounted);
        else if (!strcmp(key, "DeviceIsLuksCleartext"))
          eldbus_message_iter_arguments_get(var, "b", &v->unlocked);
        else if (!strcmp(key, "DeviceIsPartition"))
          eldbus_message_iter_arguments_get(var, "b", &v->partition);
        else if (!strcmp(key, "PartitionNumber"))
          eldbus_message_iter_arguments_get(var, "i", &v->partition_number);
        else if (!strcmp(key, "PartitionLabel"))
          {
             const char *partition_label;
             if (!eldbus_message_iter_arguments_get(var, "s", &partition_label))
               continue;
             eina_stringshare_replace(&v->partition_label, partition_label);
          }
        else if (!strcmp(key, "LuksCleartextSlave"))
          {
             const char *enc;
             E_Volume *venc;
             if (!eldbus_message_iter_arguments_get(var, "o", &enc))
               continue;
             eina_stringshare_replace(&v->partition_label, enc);
             venc = _e_fm_main_udisks_volume_find(enc);
             if (!venc)
               continue;
             eina_stringshare_replace(&v->parent, venc->parent);
             v->storage = venc->storage;
             v->storage->volumes = eina_list_append(v->storage->volumes, v);
          }
        else if (!strcmp(key, "PartitionSlave"))
          {
             char *partition_slave, buf[4096];
             if (!eldbus_message_iter_arguments_get(var, "o", &partition_slave))
               continue;
             if ((!partition_slave) || (strlen(partition_slave) < sizeof("/org/freedesktop/UDisks/devices/")))
               eina_stringshare_replace(&v->parent, partition_slave);
             else
               {
                  snprintf(buf, sizeof(buf), "/dev/%s", partition_slave + sizeof("/org/freedesktop/UDisks/devices/") - 1);
                  eina_stringshare_replace(&v->parent, buf);
               }
          }

     }
   if (!v->udi)
     {
        ERR("!udi %s", v->dbus_path);
        ecore_idler_add(_idler_volume_del, v);
        return;
     }
   if (!v->label) eina_stringshare_replace(&v->label, v->uuid);
   if (v->parent && v->partition)
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
        eina_stringshare_replace(&v->parent, v->udi);
        s = e_storage_find(v->udi);
        if (s)
          {
             v->storage = s;
             if (!eina_list_data_find_list(s->volumes, v))
               s->volumes = eina_list_append(s->volumes, v);
          }
        else
          {
             v->storage = _e_fm_main_udisks_storage_add(
                                           eina_stringshare_add(v->dbus_path));
             /* disk is both storage and volume */
             if (v->storage)
               v->storage->volumes = eina_list_append(v->storage->volumes, v);
          }
     }

   switch (v->optype)
     {
      case E_VOLUME_OP_TYPE_MOUNT:
        _e_fm_main_udisks_cb_vol_mounted(v);
        return;
      case E_VOLUME_OP_TYPE_UNMOUNT:
        _e_fm_main_udisks_cb_vol_unmounted(v);
        return;
      case E_VOLUME_OP_TYPE_EJECT:
        _e_fm_main_udisks_cb_vol_unmounted_before_eject(v);
        return;
      default:
        break;
     }

//   printf("++VOL:\n  udi: %s\n  uuid: %s\n  fstype: %s\n  size: %llu\n label: %s\n  partition: %d\n  partition_number: %d\n partition_label: %s\n  mounted: %d\n  mount_point: %s", v->udi, v->uuid, v->fstype, v->size, v->label, v->partition, v->partition_number, v->partition ? v->partition_label : "(not a partition)", v->mounted, v->mount_point);
//   if (s) printf("  for storage: %s", s->udi);
//   else printf("  storage unknown");
   v->validated = EINA_TRUE;
   e_fm_ipc_volume_add(v);
   return;
}

static int
_e_fm_main_udisks_format_error_msg(char     **buf,
                                   E_Volume  *v,
                                   const char *name,
                                   const char *message)
{
   int size, vu, vm = 1, en;
   char *tmp;

   vu = strlen(v->udi) + 1;
   if (v->mount_point) vm = strlen(v->mount_point) + 1;
   en = strlen(name) + 1;
   size = vu + vm + en + strlen(message) + 1;
   tmp = *buf = malloc(size);

   strcpy(tmp, v->udi);
   tmp += vu;
   if (v->mount_point)
     {
        strcpy(tmp, v->mount_point);
        tmp += vm;
     }
   else
     {
        *tmp = 0;
        tmp += vm;
     }
   strcpy(tmp, name);
   tmp += en;
   strcpy(tmp, message);

   return size;
}

static Eina_Bool
_e_fm_main_udisks_vol_mount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;

   if (v->op)
     eldbus_pending_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks_format_error_msg(&buf, v,
                       "org.enlightenment.fm2.MountTimeout",
                       "Unable to mount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_mounted(E_Volume *v)
{
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   if (!v->mount_point) return; /* come back later */

   v->optype = E_VOLUME_OP_TYPE_NONE;
   v->op = NULL;
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
}

static Eina_Bool
_e_fm_main_udisks_vol_unmount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   if (v->op)
     eldbus_pending_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks_format_error_msg(&buf, v,
                      "org.enlightenment.fm2.UnmountTimeout",
                      "Unable to unmount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_unmounted(E_Volume *v)
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

   v->mounted = EINA_FALSE;
   INF("UNMOUNT: %s from %s", v->udi, v->mount_point);
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
   char *buf;
   int size;

   v->guard = NULL;
   if (v->op)
     eldbus_pending_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks_format_error_msg(&buf, v,
                         "org.enlightenment.fm2.EjectTimeout",
                         "Unable to eject the media with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_volume_task_cb(void *data EINA_UNUSED, const Eldbus_Message *msg EINA_UNUSED,
                Eldbus_Pending *pending EINA_UNUSED)
{
   /**
    * if eldbus_proxy_send has callback == NULL it will return a NULL
    * but we need a Eldbus_Pending to be able to cancel it when timeout occurs
    */
   /* FIXME: this should not matter. If we don't have a callback, there's no
    * reason to cancel it... cancelling has no effect other than saying eldbus we
    * are not interested in the return anymore. I.e.: don't bother to  cancel it
    */
}

static Eldbus_Pending *
_volume_umount(Eldbus_Proxy *proxy)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;

   msg = eldbus_proxy_method_call_new(proxy, "FilesystemUnmount");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "as", &array);
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(proxy, msg, _volume_task_cb, NULL, -1);
}

static Eldbus_Pending *
_volume_eject(Eldbus_Proxy *proxy)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;

   msg = eldbus_proxy_method_call_new(proxy, "DriveEject");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "as", &array);
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(proxy, msg, _volume_task_cb, NULL, -1);
}

static Eldbus_Pending *
_volume_mount(Eldbus_Proxy *proxy, const char *fstype, Eina_List *opt)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;
   Eina_List *l;
   const char *opt_txt;

   msg = eldbus_proxy_method_call_new(proxy, "FilesystemMount");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "sas", fstype, &array);
   EINA_LIST_FOREACH(opt, l, opt_txt)
     eldbus_message_iter_basic_append(array, 's', opt_txt);
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(proxy, msg, _volume_task_cb, NULL, -1);
}

static Eina_Bool
_e_fm_main_udisks_cb_vol_ejecting_after_unmount(E_Volume *v)
{
   v->guard = ecore_timer_loop_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_eject_timeout, v);
   v->op = _volume_eject(v->storage->proxy);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks_cb_vol_unmounted_before_eject(E_Volume      *v)
{
   _e_fm_main_udisks_cb_vol_unmounted(v);

   // delay is required for all message handlers were executed after unmount
   ecore_timer_loop_add(1.0, (Ecore_Task_Cb)_e_fm_main_udisks_cb_vol_ejecting_after_unmount, v);
}

static void
_e_fm_main_udisks_cb_vol_ejected(E_Volume *v)
{
   char *buf;
   int size;

   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }

   v->optype = E_VOLUME_OP_TYPE_NONE;

   size = strlen(v->udi) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_EJECT_DONE,
                         0, 0, 0, buf, size);
}

E_Volume *
_e_fm_main_udisks_volume_add(const char *path,
                             Eina_Bool   first_time)
{
   E_Volume *v;
   Eldbus_Object *obj;

   if (!path) return NULL;
   if (_volume_find_by_dbus_path(path)) return NULL;
   v = calloc(1, sizeof(E_Volume));
   if (!v) return NULL;
   v->dbus_path = path;
   INF("VOL+ %s", path);
   v->efm_mode = EFM_MODE_USING_UDISKS_MOUNT;
   v->icon = NULL;
   v->first_time = first_time;
   _e_vols = eina_list_append(_e_vols, v);
   obj = eldbus_object_get(_e_fm_main_udisks_conn, UDISKS_BUS, path);
   v->proxy = eldbus_proxy_get(obj, UDISKS_DEVICE_INTERFACE);
   eldbus_proxy_property_get_all(v->proxy, _e_fm_main_udisks_cb_vol_prop, v);
   eldbus_proxy_signal_handler_add(v->proxy, "Changed",
                                  _e_fm_main_udisks_cb_prop_modified, v);
   v->guard = NULL;

   return v;
}

void
_e_fm_main_udisks_volume_del(const char *path)
{
   E_Volume *v;

   v = _volume_find_by_dbus_path(path);
   if (!v) return;
   _volume_del(v);
}

static void
_volume_del(E_Volume *v)
{
   if (v->guard)
     {
        ecore_timer_del(v->guard);
        v->guard = NULL;
     }
   if (v->validated)
     {
        INF("--VOL %s", v->udi);
        /* FIXME: send event of storage volume (disk) removed */
           ecore_ipc_server_send(_e_fm_ipc_server,
                                 6 /*E_IPC_DOMAIN_FM*/,
                                 E_FM_OP_VOLUME_DEL,
                                 0, 0, 0, v->udi, eina_stringshare_strlen(v->udi) + 1);
     }
   v->optype = E_VOLUME_OP_TYPE_NONE;
   if (v->storage && v->storage->requires_eject) return; /* udisks is stupid about ejectable media, so we have to keep stuff
                                   * around for all eternity instead of deleting it constantly. oh noes.
                                   */
   if (v->proxy)
     {
        Eldbus_Object *obj = eldbus_proxy_object_get(v->proxy);
        eldbus_proxy_unref(v->proxy);
        eldbus_object_unref(obj);
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

static E_Volume *
_volume_find_by_dbus_path(const char *path)
{
   Eina_List *l;
   E_Volume *v;

   if (!path) return NULL;
   EINA_LIST_FOREACH(_e_vols, l, v)
     if (!strcmp(path, v->dbus_path)) return v;
   return NULL;
}

void
_e_fm_main_udisks_volume_eject(E_Volume *v)
{
   if (!v || v->guard) return;
   if (v->mounted)
     {
        v->guard = ecore_timer_loop_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_unmount_timeout, v);
        v->op = _volume_umount(v->proxy);
     }
   else
     {
        v->guard = ecore_timer_loop_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_eject_timeout, v);
        v->op = _volume_eject(v->storage->proxy);
     }
   v->optype = E_VOLUME_OP_TYPE_EJECT;
}

void
_e_fm_main_udisks_volume_unmount(E_Volume *v)
{
   if (!v || v->guard) return;
   INF("unmount %s %s", v->udi, v->mount_point);

   v->guard = ecore_timer_loop_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_unmount_timeout, v);
   v->op = _volume_umount(v->proxy);
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

   INF("mount %s %s [fs type = %s]", v->udi, v->mount_point, v->fstype);

   // Map uid to current user if possible
   // Possible filesystems found in udisks source (src/udiskslinuxfilesystem.c) as of 2012-07-11
   if ((!strcmp(v->fstype, "vfat")) ||
       (!strcmp(v->fstype, "ntfs")) ||
       (!strcmp(v->fstype, "iso9660")) ||
       (!strcmp(v->fstype, "udf"))
       )
     {
        snprintf(buf, sizeof(buf), "uid=%i", (int)getuid());
        opt = eina_list_append(opt, buf);
     }

   // force utf8 as the encoding - e only likes/handles utf8. its the
   // pseudo-standard these days anyway for "linux" for intl text to work
   // everywhere. problem is some fs's use differing options and udisks
   // doesn't allow some options with certain filesystems.
   // Valid options found in udisks (src/udiskslinuxfilesystem.c) as of 2012-07-11
   // Note that these options are default with the latest udisks, kept here to
   // avoid breakage in the future (hopefully).
   if (!strcmp(v->fstype, "vfat"))
     {
        snprintf(buf2, sizeof(buf2), "utf8=1");
        opt = eina_list_append(opt, buf2);
     }
   else if ((!strcmp(v->fstype, "iso9660")) ||
            (!strcmp(v->fstype, "udf"))
            )
     {
        snprintf(buf2, sizeof(buf2), "iocharset=utf8");
        opt = eina_list_append(opt, buf2);
     }

   v->guard = ecore_timer_loop_add(E_FM_MOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks_vol_mount_timeout, v);

   // It was previously noted here that ubuntu 10.04 failed to mount if opt was passed to
   // e_udisks_volume_mount.  The reason at the time was unknown and apparently never found.
   // I theorize that this was due to improper mount options being passed (namely the utf8 options).
   // If this still fails on Ubuntu 10.04 then an actual fix should be found.
   v->op = _volume_mount(v->proxy, v->fstype, opt);

   eina_list_free(opt);
   v->optype = E_VOLUME_OP_TYPE_MOUNT;
}

void
_e_fm_main_udisks_init(void)
{
   eldbus_init();
   _e_fm_main_udisks_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!_e_fm_main_udisks_conn) return;

   eldbus_name_start(_e_fm_main_udisks_conn, UDISKS_BUS, 0,
                    _e_fm_main_udisks_name_start, NULL);
}

void
_e_fm_main_udisks_shutdown(void)
{
   if (_e_fm_main_udisks_proxy)
     {
        Eldbus_Object *obj;
        obj = eldbus_proxy_object_get(_e_fm_main_udisks_proxy);
        eldbus_proxy_unref(_e_fm_main_udisks_proxy);
        eldbus_object_unref(obj);
     }
   if (_e_fm_main_udisks_conn)
     {
        eldbus_connection_unref(_e_fm_main_udisks_conn);
        eldbus_shutdown();
     }
}

E_Storage *
_e_fm_main_udisks_storage_add(const char *path)
{
   E_Storage *s;
   Eldbus_Object *obj;

   if (!path) return NULL;
   if (_storage_find_by_dbus_path(path)) return NULL;
   s = calloc(1, sizeof(E_Storage));
   if (!s) return NULL;

   DBG("STORAGE+=%s", path);
   s->dbus_path = path;
   _e_stores = eina_list_append(_e_stores, s);
   obj = eldbus_object_get(_e_fm_main_udisks_conn, UDISKS_BUS, path);
   s->proxy = eldbus_proxy_get(obj, UDISKS_DEVICE_INTERFACE);
   eldbus_proxy_property_get_all(s->proxy, _e_fm_main_udisks_cb_store_prop, s);
   return s;
}

void
_e_fm_main_udisks_storage_del(const char *path)
{
   E_Storage *s;

   s = _storage_find_by_dbus_path(path);
   if (!s) return;
   if (s->validated)
     {
        INF("--STO %s", s->udi);
          ecore_ipc_server_send(_e_fm_ipc_server,
                                6 /*E_IPC_DOMAIN_FM*/,
                                E_FM_OP_STORAGE_DEL,
                                0, 0, 0, s->udi, strlen(s->udi) + 1);
     }
   _e_stores = eina_list_remove(_e_stores, s);
   if (s->proxy)
     {
        Eldbus_Object *obj = eldbus_proxy_object_get(s->proxy);
        eldbus_proxy_unref(s->proxy);
        eldbus_object_unref(obj);
     }
   _e_fm_shared_device_storage_free(s);
}

static E_Storage *
_storage_find_by_dbus_path(const char *path)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     if (!strcmp(path, s->dbus_path)) return s;
   return NULL;
}

E_Storage *
_e_fm_main_udisks_storage_find(const char *udi)
{
   Eina_List *l;
   E_Storage *s;

   EINA_LIST_FOREACH(_e_stores, l, s)
     {
        if (!s->udi) continue;
        if (!strcmp(udi, s->udi)) return s;
     }
   return NULL;
}

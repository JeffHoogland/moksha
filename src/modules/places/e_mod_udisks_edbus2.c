
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_EDBUS2

#include <EDBus.h>
#include <e.h>
#include "e_mod_places.h"


/* Local Function Prototypes */
static void _places_udisks_name_start(void *data, const EDBus_Message *msg, EDBus_Pending *pending);
static void _places_udisks_enumerate_devices_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending);
static void _places_udisks_device_add_cb(void *data, const EDBus_Message *msg);
static void _places_udisks_device_del_cb(void *data, const EDBus_Message *msg);
static void _places_udisks_device_changed_cb(void *data, const EDBus_Message *msg);
static void _places_udisks_vol_props_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending);

static Volume* _places_udisks_volume_add(const char *devpath, Eina_Bool first_time);
static void _places_udisks_mount_func(Volume *vol, Eina_List *opts);
static void _places_udisks_unmount_func(Volume *vol, Eina_List *opts);
static void _places_udisks_eject_func(Volume *vol, Eina_List *opts);
static void _places_udisks_free_func(Volume *vol);


/* Local Variables */
#define UDISKS_BUS "org.freedesktop.UDisks"
#define UDISKS_PATH "/org/freedesktop/UDisks"
#define UDISKS_IFACE "org.freedesktop.UDisks"
#define UDISKS_DEVICE_IFACE "org.freedesktop.UDisks.Device"

static EDBus_Connection *_places_dbus_conn = NULL;
static EDBus_Proxy *_places_udisks_proxy = NULL;


/* API */
Eina_Bool
places_udisks_edbus2_init(void)
{
   printf("PLACES: udisks2: init()\n");

   if (!edbus_init())
      return EINA_FALSE;

   _places_dbus_conn = edbus_connection_get(EDBUS_CONNECTION_TYPE_SYSTEM);
   if (!_places_dbus_conn)
   {
      printf("PLACES: udisks2: Error connecting to system bus. Is it running?\n");
      return EINA_FALSE;
   }

   edbus_name_start(_places_dbus_conn, UDISKS_BUS, 0,
                    _places_udisks_name_start, NULL);

   return EINA_TRUE;
}

void
places_udisks_edbus2_shutdown(void)
{
   if (_places_udisks_proxy) edbus_proxy_unref(_places_udisks_proxy);
   if (_places_dbus_conn) edbus_connection_unref(_places_dbus_conn);
   edbus_shutdown();
}

/* Implementation */
static void
_places_udisks_name_start(void *data, const EDBus_Message *msg,
                          EDBus_Pending *pending)
{
   EDBus_Object *obj;
   unsigned flag;

   EINA_SAFETY_ON_FALSE_RETURN(edbus_message_arguments_get(msg, "u", &flag));

   obj = edbus_object_get(_places_dbus_conn, UDISKS_BUS, UDISKS_PATH);
   _places_udisks_proxy = edbus_proxy_get(obj, UDISKS_IFACE);

   edbus_proxy_call(_places_udisks_proxy, "EnumerateDevices",
                    _places_udisks_enumerate_devices_cb, NULL, -1, "");

   edbus_proxy_signal_handler_add(_places_udisks_proxy, "DeviceAdded",
                                  _places_udisks_device_add_cb, NULL);
   edbus_proxy_signal_handler_add(_places_udisks_proxy, "DeviceRemoved",
                                  _places_udisks_device_del_cb, NULL);
}

static Volume*
_places_udisks_volume_add(const char *devpath, Eina_Bool first_time)
{
   EDBus_Object *obj;
   EDBus_Proxy *proxy;
   Volume *vol;

   // create the EDBus object and proxy
   obj = edbus_object_get(_places_dbus_conn, UDISKS_BUS, devpath);
   if (!obj) return NULL;
   proxy = edbus_proxy_get(obj, UDISKS_DEVICE_IFACE);
   if (!proxy) return NULL;

   // create the places volume
   vol = places_volume_add(devpath, first_time);
   if (!vol) return NULL;
   vol->backend_data = proxy;
   vol->mount_func = _places_udisks_mount_func;
   vol->unmount_func = _places_udisks_unmount_func;
   vol->eject_func = _places_udisks_eject_func;
   vol->free_func = _places_udisks_free_func;

   // get notification when the dive change
   edbus_proxy_signal_handler_add(proxy, "Changed",
                                  _places_udisks_device_changed_cb, vol);

   return vol;
}

/* Callback of UDisks method "EnumerateDevices()" */
static void
_places_udisks_enumerate_devices_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending)
{
   EDBus_Message_Iter *ao;
   const char *devpath;

   EINA_SAFETY_ON_TRUE_RETURN(edbus_message_error_get(msg, NULL, NULL));
   EINA_SAFETY_ON_FALSE_RETURN(edbus_message_arguments_get(msg, "ao", &ao));

   while (edbus_message_iter_get_and_next(ao, 'o', &devpath))
   {
      Volume *vol;
      vol = _places_udisks_volume_add(devpath, EINA_TRUE);
      if (!vol) return;
      edbus_proxy_property_get_all((EDBus_Proxy *)vol->backend_data,
                                   _places_udisks_vol_props_cb, vol);
   }
}

/* Callback for UDisks signal "DeviceAdded" */
static void
_places_udisks_device_add_cb(void *data, const EDBus_Message *msg)
{
   Volume *vol;
   char *devpath;

   EINA_SAFETY_ON_FALSE_RETURN(edbus_message_arguments_get(msg, "o", &devpath));
   printf("PLACES udisks: DeviceAdded [%s]\n", devpath);

   if ((vol = _places_udisks_volume_add(devpath, EINA_FALSE)))
      edbus_proxy_property_get_all((EDBus_Proxy *)vol->backend_data,
                                   _places_udisks_vol_props_cb, vol);
}

/* Callback for UDisks signal "DeviceRemoved" */
static void
_places_udisks_device_del_cb(void *data, const EDBus_Message *msg)
{
   Volume *vol;
   char *devpath;

   EINA_SAFETY_ON_FALSE_RETURN(edbus_message_arguments_get(msg, "o", &devpath));

   if ((vol = places_volume_by_id_get(devpath)))
      places_volume_del(vol);
}

/* Callback for UDisks signal "Changed" */
static void
_places_udisks_device_changed_cb(void *data, const EDBus_Message *msg)
{
   Volume *vol = data;

   if (!vol) return;
   edbus_proxy_property_get_all((EDBus_Proxy *)vol->backend_data,
                                _places_udisks_vol_props_cb, vol);
}

/* Callback for edbus_proxy_property_get_all() */
static void
_places_udisks_vol_props_cb(void *data, const EDBus_Message *msg, EDBus_Pending *pending)
{
   Volume *vol = data;
   EDBus_Message_Iter *array, *dict;

   const char *label = NULL;
   const char *partition_label = NULL;
   const char *device = NULL;
   const char *mount_point = NULL;
   const char *drive_type = NULL;
   const char *fstype = NULL;
   const char *bus = NULL;
   const char *model = NULL;
   const char *serial = NULL;
   const char *vendor = NULL;
   const char *id_usage = NULL;
   Eina_Bool mounted = EINA_FALSE;
   Eina_Bool unlocked = EINA_FALSE;
   Eina_Bool removable = EINA_FALSE;
   Eina_Bool requires_eject = EINA_FALSE;
   Eina_Bool can_detach = EINA_FALSE;
   Eina_Bool encrypted = EINA_FALSE;
   Eina_Bool media_available = EINA_FALSE;
   unsigned long long size = 0;

   EINA_SAFETY_ON_TRUE_RETURN(edbus_message_error_get(msg, NULL, NULL));
   EINA_SAFETY_ON_FALSE_RETURN(edbus_message_arguments_get(msg, "a{sv}", &array));

   // collect usefull props iterating over the dict
   while (edbus_message_iter_get_and_next(array, 'e', &dict))
   {
      EDBus_Message_Iter *var;
      const char *key;
      Eina_Bool bool;

      if (!edbus_message_iter_arguments_get(dict, "sv", &key, &var))
         continue;

      // skip volumes with volume.ignore set
      if (!strcmp(key, "DeviceIsMediaChangeDetectionInhibited"))
      {
         edbus_message_iter_arguments_get(var, "b", &bool);
         if (bool) return;
      }
      else if (!strcmp(key, "IdUsage"))
         edbus_message_iter_arguments_get(var, "s", &id_usage);
      else if (!strcmp(key, "DeviceFile"))
         edbus_message_iter_arguments_get(var, "s", &device);
      else if (!strcmp(key, "IdLabel"))
         edbus_message_iter_arguments_get(var, "s", &label);
      else if (!strcmp(key, "PartitionLabel"))
         edbus_message_iter_arguments_get(var, "s", &partition_label);
      else if (!strcmp(key, "IdType"))
         edbus_message_iter_arguments_get(var, "s", &fstype);
      else if (!strcmp(key, "DriveModel"))
         edbus_message_iter_arguments_get(var, "s", &model);
      else if (!strcmp(key, "DriveSerial"))
         edbus_message_iter_arguments_get(var, "s", &serial);
      else if (!strcmp(key, "DriveVendor"))
         edbus_message_iter_arguments_get(var, "s", &vendor);
      else if (!strcmp(key, "DriveConnectionInterface"))
         edbus_message_iter_arguments_get(var, "s", &bus);
      else if (!strcmp(key, "DeviceMountPaths"))
      {
         EDBus_Message_Iter *inner_array;
         if (!edbus_message_iter_arguments_get(var, "as", &inner_array))
            continue;
         edbus_message_iter_get_and_next(inner_array, 's', &mount_point);
      }
      else if (!strcmp(key, "DriveMediaCompatibility"))
      {
         EDBus_Message_Iter *inner_array;
         if (!edbus_message_iter_arguments_get(var, "as", &inner_array))
            continue;
         edbus_message_iter_get_and_next(inner_array, 's', &drive_type);
      }
      else if (!strcmp(key, "DeviceIsLuks"))
         edbus_message_iter_arguments_get(var, "b", &encrypted);
      else if (!strcmp(key, "DeviceIsMounted"))
         edbus_message_iter_arguments_get(var, "b", &mounted);
      else if (!strcmp(key, "DeviceIsLuksCleartext"))
         edbus_message_iter_arguments_get(var, "b", &unlocked);
      else if (!strcmp(key, "DeviceIsRemovable"))
         edbus_message_iter_arguments_get(var, "b", &removable);
      else if (!strcmp(key, "DriveIsMediaEjectable"))
         edbus_message_iter_arguments_get(var, "b", &requires_eject);
      else if (!strcmp(key, "DeviceIsMediaAvailable"))
         edbus_message_iter_arguments_get(var, "b", &media_available);
      else if (!strcmp(key, "DriveCanDetach"))
         edbus_message_iter_arguments_get(var, "b", &can_detach);
      else if (!strcmp(key, "DeviceSize"))
         edbus_message_iter_arguments_get(var, "t", &size);
   }

   // a cdrom has been ejected, invalidate the drive to 'hide' it
   if (!media_available && vol->valid)
   {
      vol->valid = EINA_FALSE;
      places_update_all_gadgets();
      return;
   }

   // skip volumes that aren't filesystems or crypto
   if (!id_usage || !id_usage[0]) return;
   if (!strcmp(id_usage, "crypto") && !encrypted)
      return;
   else if (strcmp(id_usage, "filesystem"))
     return;

   // choose the best label
   if (partition_label && partition_label[0])
      eina_stringshare_replace(&vol->label, partition_label);
   else if (label && label[0])
      eina_stringshare_replace(&vol->label, label);
   else if (mount_point && mount_point[0])
      eina_stringshare_replace(&vol->label, mount_point);
   else if (device && device[0])
      eina_stringshare_replace(&vol->label, vol->device);

   // store all other props in the Volume*
   if (device) eina_stringshare_replace(&vol->device, device);
   if (mount_point) eina_stringshare_replace(&vol->mount_point, mount_point);
   if (fstype) eina_stringshare_replace(&vol->fstype, fstype);
   if (model) eina_stringshare_replace(&vol->model, model);
   if (serial) eina_stringshare_replace(&vol->serial, serial);
   if (vendor) eina_stringshare_replace(&vol->vendor, vendor);
   if (bus) eina_stringshare_replace(&vol->bus, bus);
   if (drive_type) eina_stringshare_replace(&vol->drive_type, drive_type);
   vol->mounted = mounted;
   vol->unlocked = unlocked;
   vol->removable = removable;
   vol->requires_eject = requires_eject || can_detach;
   vol->encrypted = encrypted;
   vol->size = size;

   if (!vol->valid)
   {
      vol->valid = EINA_TRUE;
      // trigger a full redraw, is the only way to show a new device
      places_update_all_gadgets();
   }

   // the update is always needed to trigger auto_mount/auto_open
   places_volume_update(vol);
}

static void
_places_udisks_volume_task_cb(void *data, const EDBus_Message *msg,
                              EDBus_Pending *pending)
{
   // Volume *vol = data;

   // TODO alert if the operation has failed
   // printf("sig: '%s'\n", edbus_message_signature_get(msg));
   // char *str;
   // edbus_message_arguments_get(msg,"s", &str);
   // printf("%s\n", str);
}

static void
_places_udisks_mount_func(Volume *vol, Eina_List *opts)
{
   EDBus_Proxy *proxy = vol->backend_data;
   EDBus_Message_Iter *array, *main_iter;
   EDBus_Message *msg;
   Eina_List *l;
   const char *opt_txt;

   if (!proxy) return;

   msg = edbus_proxy_method_call_new(proxy, "FilesystemMount");
   main_iter = edbus_message_iter_get(msg);
   edbus_message_iter_arguments_append(main_iter, "sas", vol->fstype, &array);
   EINA_LIST_FOREACH(opts, l, opt_txt)
   {
      // printf("  opt: '%s'\n", opt_txt);
      edbus_message_iter_basic_append(array, 's', opt_txt);
   }
   edbus_message_iter_container_close(main_iter, array);

   edbus_proxy_send(proxy, msg, _places_udisks_volume_task_cb, vol, -1);
}

static void
_places_udisks_unmount_func(Volume *vol, Eina_List *opts)
{
   EDBus_Proxy *proxy = vol->backend_data;
   EDBus_Message_Iter *array, *main_iter;
   EDBus_Message *msg;

   if (!proxy) return;

   msg = edbus_proxy_method_call_new(proxy, "FilesystemUnmount");
   main_iter = edbus_message_iter_get(msg);
   edbus_message_iter_arguments_append(main_iter, "as", &array);
   // here we can use the force option if needed
   edbus_message_iter_container_close(main_iter, array);

   edbus_proxy_send(proxy, msg, _places_udisks_volume_task_cb, vol, -1);
}

static void
_places_udisks_eject_func(Volume *vol, Eina_List *opts)
{
   EDBus_Proxy *proxy = vol->backend_data;
   EDBus_Message_Iter *array, *main_iter;
   EDBus_Message *msg;

   if (!proxy) return;

   msg = edbus_proxy_method_call_new(proxy, "DriveEject");
   // msg = edbus_proxy_method_call_new(proxy, "DriveDetach");
   main_iter = edbus_message_iter_get(msg);
   edbus_message_iter_arguments_append(main_iter, "as", &array);
   edbus_message_iter_container_close(main_iter, array);

   edbus_proxy_send(proxy, msg, _places_udisks_volume_task_cb, vol, -1);
}

static void
_places_udisks_free_func(Volume *vol)
{
   EDBus_Object *obj;
   EDBus_Proxy *proxy;

   if (vol->backend_data)
   {
      proxy = vol->backend_data;
      obj = edbus_proxy_object_get(proxy);

      if (proxy) edbus_proxy_unref(proxy);
      if (obj) edbus_object_unref(obj);

      vol->backend_data = NULL;
   }
}

#endif

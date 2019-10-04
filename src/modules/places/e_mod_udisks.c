#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_UDISKS_MOUNT

#include <e.h>
#include "e_mod_places.h"
#include <E_DBus.h>
#include <E_Ukit.h>
#include <errno.h>

/* Convenience macro */
#define UDISKS_SH_DEL(_conn, _sh)  \
   if (_sh) { e_dbus_signal_handler_del(_conn, _sh); _sh = NULL; }

/* Local Function Prototypes */
static void _places_udisks_test(void *data __UNUSED__, DBusMessage *msg __UNUSED__, DBusError *error);
static void _places_udisks_poll(void *data __UNUSED__, DBusMessage *msg);
static void _places_udisks_device_add_cb(void *data __UNUSED__, DBusMessage *msg);
static void _places_udisks_device_rem_cb(void *data __UNUSED__, DBusMessage *msg);
static void _places_udisks_device_chg_cb(void *data __UNUSED__, DBusMessage *msg);

static void _places_udisks_all_devices_cb(void *user_data __UNUSED__, void *reply_data, DBusError *error);
static void _places_udisks_vol_prop_cb(void *data, void *reply_data, DBusError *error);
static void _places_udisks_sto_prop_cb(void *data, void *reply_data, DBusError *error);

static Volume* _places_udisks_volume_add(const char *udi, Eina_Bool first_time);
static void _places_udisks_mount_func(Volume *vol, Eina_List *opts);
static void _places_udisks_unmount_func(Volume *vol, Eina_List *opts __UNUSED__);
static void _places_udisks_eject_func(Volume *vol, Eina_List *opts __UNUSED__);

/* Local Variables */
static E_DBus_Connection *_places_udisks_conn = NULL;
static E_DBus_Signal_Handler *_places_udisks_sh_added = NULL;
static E_DBus_Signal_Handler *_places_udisks_sh_removed = NULL;
static E_DBus_Signal_Handler *_places_udisks_sh_changed = NULL;
static E_DBus_Signal_Handler *_places_udisks_udisks_poll = NULL;


/* Implementation */
Eina_Bool
places_udisks_init(void)
{
   DBusMessage *msg;

   printf("PLACES: udisks: init()\n");

   if (!e_dbus_init())
     {
        printf("Impossible to setup dbus.\n");
        return EINA_FALSE;
     }

   if (!e_ukit_init())
     {
        printf("Impossible to setup ukit.\n");
        return EINA_FALSE;
     }

   _places_udisks_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!_places_udisks_conn)
   {
      printf("Error connecting to system bus. Is it running?\n");
      return EINA_FALSE;
   }

   // search a running udisks ...
   e_dbus_get_name_owner(_places_udisks_conn, E_UDISKS_BUS,
                         _places_udisks_test, NULL);

   // ... a not running one    :/
   msg = dbus_message_new_method_call(E_UDISKS_BUS, E_UDISKS_PATH, E_UDISKS_BUS, "suuuuuup");
   e_dbus_method_call_send(_places_udisks_conn, msg, NULL,
                           (E_DBus_Callback_Func)_places_udisks_test,
                           NULL, -1, NULL);
   dbus_message_unref(msg);

   // ... and poll.
   if (!_places_udisks_udisks_poll)
     _places_udisks_udisks_poll = e_dbus_signal_handler_add(_places_udisks_conn,
                        E_DBUS_FDO_BUS, E_DBUS_FDO_PATH,E_DBUS_FDO_INTERFACE,
                        "NameOwnerChanged", _places_udisks_poll, NULL);

   return EINA_TRUE;
}

void
places_udisks_shutdown(void)
{
   if (_places_udisks_conn)
     {
        UDISKS_SH_DEL(_places_udisks_conn, _places_udisks_sh_added);
        UDISKS_SH_DEL(_places_udisks_conn, _places_udisks_sh_removed);
        UDISKS_SH_DEL(_places_udisks_conn, _places_udisks_sh_changed);
        UDISKS_SH_DEL(_places_udisks_conn, _places_udisks_udisks_poll);
        e_dbus_connection_close(_places_udisks_conn);
        _places_udisks_conn = NULL;
     }

   e_ukit_shutdown();
   e_dbus_shutdown();
}

static void
_places_udisks_test(void *data __UNUSED__, DBusMessage *msg __UNUSED__, DBusError *error)
{
   if (error && dbus_error_is_set(error))
     {
        printf("PLACES: dbus error: %s\n", error->message);
        dbus_error_free(error);
        return;
     }

   printf("PLACES: Udisks is OK!\n");
   _places_udisks_sh_added = e_dbus_signal_handler_add(_places_udisks_conn,
                             E_UDISKS_BUS, E_UDISKS_PATH, E_UDISKS_BUS,
                             "DeviceAdded", _places_udisks_device_add_cb, NULL);
   _places_udisks_sh_removed = e_dbus_signal_handler_add(_places_udisks_conn,
                             E_UDISKS_BUS, E_UDISKS_PATH, E_UDISKS_BUS,
                             "DeviceRemoved", _places_udisks_device_rem_cb, NULL);
   _places_udisks_sh_changed = e_dbus_signal_handler_add(_places_udisks_conn,
                             E_UDISKS_BUS, E_UDISKS_PATH, E_UDISKS_BUS,
                             "DeviceChanged", _places_udisks_device_chg_cb, NULL);

   e_udisks_get_all_devices(_places_udisks_conn,
                            _places_udisks_all_devices_cb, NULL);
}

/* Dbus CB for signal "NameOwnerChanged" */
static void
_places_udisks_poll(void *data __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   const char *name, *from, *to;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &from,
                              DBUS_TYPE_STRING, &to,
                              DBUS_TYPE_INVALID))
     {
        printf("PLACES: dbus error: %s\n", err.message);
        dbus_error_free(&err);
        return;
     }

   if ((name) && !strcmp(name, E_UDISKS_BUS))
     _places_udisks_test(NULL, NULL, NULL);
}

/* Reply of e_udisks_get_all_devices() */
static void
_places_udisks_all_devices_cb(void *user_data __UNUSED__, void *reply_data, DBusError *err)
{
   E_Ukit_String_List_Return *udisks_ret = reply_data;
   Eina_List *l;
   char *udi;
   Volume *v;

   if (dbus_error_is_set(err))
     {
        printf("PLACES: dbus error: %s\n", err->message);
        dbus_error_free(err);
        return;
     }

   if (!udisks_ret || !udisks_ret->strings) return;

   EINA_LIST_FOREACH(udisks_ret->strings, l, udi)
   {
      v = _places_udisks_volume_add(udi, EINA_TRUE);
      e_udisks_get_all_properties(_places_udisks_conn, udi,
                                  _places_udisks_vol_prop_cb, v);
   }
}

/* Dbus CB for signal "DeviceAdded" */
static void
_places_udisks_device_add_cb(void *data __UNUSED__, DBusMessage *msg)
{
   Volume *v;
   DBusError err;
   char *udi;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_OBJECT_PATH, &udi,
                              DBUS_TYPE_INVALID))
     {
        printf("PLACES: dbus error: %s\n", err.message);
        dbus_error_free(&err);
        return;
     }

   printf("PLACES udisks: DeviceAdded [%s]\n", udi);
   v = _places_udisks_volume_add(udi, EINA_FALSE);
   e_udisks_get_all_properties(_places_udisks_conn, udi,
                               _places_udisks_vol_prop_cb, v);
}

/* Dbus CB  for signal "DeviceRemoved" */
static void
_places_udisks_device_rem_cb(void *data __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   Volume *v;
   char *udi;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_OBJECT_PATH, &udi,
                              DBUS_TYPE_INVALID))
     {
        printf("PLACES: dbus error: %s\n", err.message);
        dbus_error_free(&err);
        return;
     }

   v = places_volume_by_id_get(udi);
   if (!v) return;

   printf("PLACES udisks: DeviceRemoved [%s]\n", v->id);
   places_volume_del(v);
}

/* Dbus CB  for signal "DeviceChanged */
static void
_places_udisks_device_chg_cb(void *data __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   Volume *v;
   char *udi;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_OBJECT_PATH, &udi,
                              DBUS_TYPE_INVALID))
     {
        printf("PLACES: dbus error: %s\n", err.message);
        dbus_error_free(&err);
        return;
     }

   v = places_volume_by_id_get(udi);
   if (!v) return;

   printf("PLACES udisks: DeviceChanged [%s]\n", v->id);
   e_udisks_get_all_properties(_places_udisks_conn, v->id,
                               _places_udisks_vol_prop_cb, v);
}

/* Reply of e_udisks_get_all_properties() (for volumes) */
static void
_places_udisks_vol_prop_cb(void *data, void *reply_data, DBusError *error)
{
   Volume *v = data;
   E_Ukit_Properties *udisks_ret = reply_data;
   Eina_Bool is_crypto = EINA_FALSE;
   const char *str = NULL;
   const char *slave;
   int err = 0;

   if (dbus_error_is_set(error))
     {
        printf("PLACES: dbus error: %s\n", error->message);
        dbus_error_free(error);
        return;
     }

   if (!v) return;

   // skip volumes with volume.ignore set
   if (e_ukit_property_bool_get(udisks_ret, "DeviceIsMediaChangeDetectionInhibited", &err) || err)
     return;
   // skip volumes with presentation.hide set
   if (e_ukit_property_bool_get(udisks_ret, "DevicePresentationHide", &err) || err)
     return;
   // skip volumes without a storage (slave partition)
   str = e_ukit_property_string_get(udisks_ret, "PartitionSlave", &err);
   if (err || !str) return;
   slave = str;

   // a cdrom has been ejected, invalidate the drive to 'hide' it
   if (!e_ukit_property_bool_get(udisks_ret, "DeviceIsMediaAvailable", &err))
     {
        if (v->valid)
          {
             printf("EJECTED %s\n", v->device);
             v->valid = EINA_FALSE;
             places_update_all_gadgets();
             return;
          }
     }

   // skip volumes that aren't filesystems or crypto
   str = e_ukit_property_string_get(udisks_ret, "IdUsage", &err);
   if (err || !str || !str[0]) return;
   if (!strcmp(str, "crypto"))
     {
        is_crypto = e_ukit_property_bool_get(udisks_ret, "DeviceIsLuks", &err);
        if (!is_crypto) return;
     }
   else if (strcmp(str, "filesystem"))
     return;

   str = e_ukit_property_string_get(udisks_ret, "IdLabel", &err);
   if (!err && str && str[0])
     {
        // first-choice label
        if (v->label) eina_stringshare_del(v->label);
        v->label = eina_stringshare_add(str);
     }

   if (!is_crypto)
     {
        const Eina_List *l;

        l = e_ukit_property_strlist_get(udisks_ret, "DeviceMountPaths", &err);
        if (!err && l && l->data)
          {
             if (v->mount_point) eina_stringshare_del(v->mount_point);
             v->mount_point = eina_stringshare_add(l->data);
             // second-choice label
             if (!v->label) v->label = eina_stringshare_add(v->mount_point);
          }

        str = e_ukit_property_string_get(udisks_ret, "IdType", &err);
        if (!err && str)
          {
             if (v->fstype) eina_stringshare_del(v->fstype);
             v->fstype = eina_stringshare_add(str);
          }

        str = e_ukit_property_string_get(udisks_ret, "DeviceFile", &err);
        if (!err && str)
          {
             if (v->device) eina_stringshare_del(v->device);
             v->device = eina_stringshare_add(str);
             // third-choice label
             if (!v->label) v->label = eina_stringshare_add(v->device);
          }

        v->size = e_ukit_property_uint64_get(udisks_ret, "DeviceSize", &err);
        v->mounted = e_ukit_property_bool_get(udisks_ret, "DeviceIsMounted", &err);
     }
   else
     {
        v->encrypted = EINA_TRUE;
        v->unlocked = e_ukit_property_bool_get(udisks_ret, "DeviceIsLuksCleartext", &err);
     }

   // TODO 'DeviceIsReadOnly'

   if(e_ukit_property_bool_get(udisks_ret, "DeviceIsDrive", &err))
     {
        // the device is itself also a storage
        e_udisks_get_all_properties(_places_udisks_conn, v->id,
                                    _places_udisks_sto_prop_cb, v);
     }
   else
     {
        // request the props for the parent storage
        e_udisks_get_all_properties(_places_udisks_conn, slave,
                                    _places_udisks_sto_prop_cb, v);
     }
}

/* Reply of e_udisks_get_all_properties() (for storage/drive) */
static void
_places_udisks_sto_prop_cb(void *data, void *reply_data, DBusError *error)
{
   E_Ukit_Properties *udisks_ret = reply_data;
   Volume *v = data;
   const char *str;
   int err = 0;
   const Eina_List *l;

   if (!v) return;
   if (dbus_error_is_set(error))
     {
        printf("PLACES: dbus error: %s\n", error->message);
        dbus_error_free(error);
        return;
     }

   str = e_ukit_property_string_get(udisks_ret, "DriveConnectionInterface", &err);
   if (!err && str)
     {
        if (v->bus) eina_stringshare_del(v->bus);
        v->bus = eina_stringshare_add(str);
     }

   l = e_ukit_property_strlist_get(udisks_ret, "DriveMediaCompatibility", &err);
   if (!err && l && l->data)
     {
        if (v->drive_type) eina_stringshare_del(v->drive_type);
        v->drive_type = eina_stringshare_add(l->data);
     }

   str = e_ukit_property_string_get(udisks_ret, "DriveModel", &err);
   if (!err && str)
     {
        if (v->model) eina_stringshare_del(v->model);
        v->model = eina_stringshare_add(str);
     }

   str = e_ukit_property_string_get(udisks_ret, "DriveVendor", &err);
   if (!err && str)
     {
        if (v->vendor) eina_stringshare_del(v->vendor);
        v->vendor = eina_stringshare_add(str);
     }

   str = e_ukit_property_string_get(udisks_ret, "DriveSerial", &err);
   if (!err && str)
     {
        if (v->serial) eina_stringshare_del(v->serial);
        v->serial = eina_stringshare_add(str);
     }

   v->removable = e_ukit_property_bool_get(udisks_ret, "DeviceIsRemovable", &err);
   v->requires_eject = e_ukit_property_bool_get(udisks_ret, "DriveIsMediaEjectable", &err);

   if (!v->valid)
     {
        v->valid = EINA_TRUE;
        // trigger a full redraw, the only way to show a new device
        places_update_all_gadgets();
        // the update is needed to trigger auto_mount/auto_open
        places_volume_update(v);
        // TODO JUST FOR DEBUG will remove sooner or later
        places_print_volume(v);
     }
   else
     {
        places_volume_update(v);
     }
}


/* Internals */
static Volume*
_places_udisks_volume_add(const char *udi, Eina_Bool first_time)
{
   Volume *v;

   v = places_volume_add(udi, first_time);
   if (!v) return NULL;

   v->mount_func = _places_udisks_mount_func;
   v->unmount_func = _places_udisks_unmount_func;
   v->eject_func = _places_udisks_eject_func;

   return v;
}

static void
_places_udisks_mount_func(Volume *vol, Eina_List *opts)
{
   e_udisks_volume_mount(_places_udisks_conn, vol->id, vol->fstype, opts);
}

static void
_places_udisks_unmount_func(Volume *vol, Eina_List *opts __UNUSED__)
{
   e_udisks_volume_unmount(_places_udisks_conn, vol->id, NULL);
}

static void
_places_udisks_eject_func(Volume *vol, Eina_List *opts __UNUSED__)
{
   e_udisks_volume_eject(_places_udisks_conn, vol->id, NULL);
}

#endif

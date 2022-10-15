#include "config.h"

#include "e_fm_shared_device.h"
#include "e_fm_shared_codec.h"
#include "e_fm_ipc.h"

#include "e_macros.h"
#include "e_fm_main_udisks2.h"
#include "e_fm_main.h"

#define E_TYPEDEFS
#include "e_fm_op.h"

#define UDISKS2_BUS "org.freedesktop.UDisks2"
#define UDISKS2_PATH "/org/freedesktop/UDisks2"
#define UDISKS2_INTERFACE "org.freedesktop.UDisks2"
#define UDISKS2_INTERFACE_BLOCK "org.freedesktop.UDisks2.Block"
#define UDISKS2_INTERFACE_DRIVE "org.freedesktop.UDisks2.Drive"
#define UDISKS2_INTERFACE_FILESYSTEM "org.freedesktop.UDisks2.Filesystem"

typedef struct U2_Block
{
   size_t Size;
   Eina_Stringshare *Device;
   Eina_Stringshare *parent;
   Eina_Bool volume;
   Eina_Stringshare *IdType;
   Eina_Stringshare *IdLabel;
   Eina_Stringshare *IdUUID;
   Eina_Bool HintIgnore;
   Eina_Bool HintSystem;
   Eina_Stringshare *HintName;
   Eina_Stringshare *HintIconName;
} U2_Block;

static Eldbus_Connection *_e_fm_main_udisks2_conn = NULL;
static Eldbus_Proxy *_e_fm_main_udisks2_proxy = NULL;
static Eina_List *_e_stores = NULL;
static Eina_List *_e_vols = NULL;

static void _e_fm_main_udisks2_cb_dev_all(void *data, const Eldbus_Message *msg,
                                         Eldbus_Pending *pending);
static void _e_fm_main_udisks2_cb_dev_add(void *data, const Eldbus_Message *msg);
static void _e_fm_main_udisks2_cb_dev_del(void *data, const Eldbus_Message *msg);
static void _e_fm_main_udisks2_cb_vol_mounted(E_Volume *v);
static void _e_fm_main_udisks2_cb_vol_unmounted(E_Volume *v);

static void      _e_fm_main_udisks2_cb_vol_ejected(E_Volume *v);
static int _e_fm_main_udisks2_format_error_msg(char     **buf,
                                              E_Volume  *v,
                                              const char *name,
                                              const char *message);
static void _e_fm_main_udisks2_cb_storage_prop_modified(void *data, const Eldbus_Message *msg, Eldbus_Pending *p);
static void _e_fm_main_udisks2_volume_mounts_update(E_Volume *v, Eldbus_Message_Iter *arr3, Eina_Bool first);
static Eina_Bool _e_fm_main_udisks2_vol_mount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks2_vol_unmount_timeout(E_Volume *v);
static Eina_Bool _e_fm_main_udisks2_vol_eject_timeout(E_Volume *v);
static E_Storage *_storage_find_by_dbus_path(const char *path);
static E_Volume *_volume_find_by_dbus_path(const char *path);
static void _volume_del(E_Volume *v);
static Eina_Bool _storage_del(void *data);
static void _e_fm_main_udisks2_cb_storage_prop_modified_cb(void *data, const Eldbus_Message *msg);
static E_Storage * _e_fm_main_udisks2_storage_drive_add(const char *udi, E_Storage *s, Eldbus_Message_Iter *arr3);

static Eina_List *vols_ejecting = NULL;

static Eina_List *stores_registered = NULL;

static Eina_Stringshare *
_util_fuckyouglib_convert(Eldbus_Message_Iter *fuckyouglib)
{
   Eldbus_Message_Iter *no_really;
   unsigned char c, buf[PATH_MAX] = {0};
   unsigned int x = 0;

   if (!eldbus_message_iter_arguments_get(fuckyouglib, "ay", &no_really)) return NULL;
   while (eldbus_message_iter_get_and_next(no_really, 'y', &c))
     {
        buf[x] = c;
        x++;
     }
   if (!buf[0]) return NULL;
   return eina_stringshare_add((char*)buf);
}

static void
_e_fm_main_udisks2_name_start(void *data EINA_UNUSED, const Eldbus_Message *msg,
                             Eldbus_Pending *pending EINA_UNUSED)
{
   unsigned flag = 0;
   Eldbus_Object *obj;

   if (!eldbus_message_arguments_get(msg, "u", &flag) || !flag)
     {
        _e_fm_main_udisks2_catch(EINA_FALSE);
        return;
     }
   obj = eldbus_object_get(_e_fm_main_udisks2_conn, UDISKS2_BUS, UDISKS2_PATH);
   eldbus_object_managed_objects_get(obj, _e_fm_main_udisks2_cb_dev_all, NULL);

   _e_fm_main_udisks2_proxy = eldbus_proxy_get(obj, ELDBUS_FDO_INTERFACE_OBJECT_MANAGER);
   eldbus_proxy_signal_handler_add(_e_fm_main_udisks2_proxy, "InterfacesAdded",
                                  _e_fm_main_udisks2_cb_dev_add, NULL);
   eldbus_proxy_signal_handler_add(_e_fm_main_udisks2_proxy, "InterfacesRemoved",
                                  _e_fm_main_udisks2_cb_dev_del, NULL);
   //eldbus_signal_handler_add(_e_fm_main_udisks2_conn, NULL,
     //NULL, ELDBUS_FDO_INTERFACE_PROPERTIES, "PropertiesChanged",
     //_e_fm_main_udisks2_cb_dev_add, NULL);
   _e_fm_main_udisks2_catch(EINA_TRUE); /* signal usage of udisks2 for mounting */
}

static void
_e_fm_main_udisks2_block_clear(U2_Block *u2)
{
   eina_stringshare_del(u2->Device);
   eina_stringshare_del(u2->parent);
   eina_stringshare_del(u2->IdType);
   eina_stringshare_del(u2->IdLabel);
   eina_stringshare_del(u2->IdUUID);
   eina_stringshare_del(u2->HintIconName);
   eina_stringshare_del(u2->HintName);

}

static void
_e_fm_main_udisks2_storage_block_add(E_Storage *s, U2_Block *u2)
{
   s->media_size = u2->Size;
   eina_stringshare_replace(&s->icon.drive, u2->HintIconName);
   s->system_internal = u2->HintIgnore;
}

static void
_e_fm_main_udisks2_volume_block_add(E_Volume *v, U2_Block *u2)
{
   v->validated = u2->volume && u2->Device && u2->parent && (!u2->HintSystem);
   if (!v->validated) return;
   v->size = u2->Size;
   eina_stringshare_replace(&v->udi, u2->Device);
   eina_stringshare_replace(&v->parent, u2->parent);
   eina_stringshare_replace(&v->icon, u2->HintIconName);
   eina_stringshare_replace(&v->uuid, u2->IdUUID);
   eina_stringshare_replace(&v->fstype, u2->IdType);
   eina_stringshare_replace(&v->label, u2->IdLabel);
   if (!v->label) v->label = eina_stringshare_ref(u2->HintName);

}

static void
_e_fm_main_udisks2_storage_add_send(E_Storage *s)
{
   void *msg_data;
   int msg_size;

   if (!s->validated) return;
   if (eina_list_data_find(stores_registered, s)) return;
   if ((!s->removable) && (!s->hotpluggable) && (!s->requires_eject))
     {
        DBG("removing likely internal storage %s", s->dbus_path);
        s->validated = 0;
        ecore_idler_add(_storage_del, s->dbus_path);
        return;
     }
   stores_registered = eina_list_append(stores_registered, s);
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

static int
_e_fm_main_udisks2_block_handle(Eldbus_Message_Iter *arr3, U2_Block *u2)
{
   Eldbus_Message_Iter *dict3;
   int block = 0;

   while (eldbus_message_iter_get_and_next(arr3, 'e', &dict3))
     {
        Eldbus_Message_Iter *var;
        char *key2, *val, *type;
        uint64_t u;
        Eina_Bool b;

        if (!eldbus_message_iter_arguments_get(dict3, "sv", &key2, &var))
          continue;

        type = eldbus_message_iter_signature_get(var);
        switch (type[0])
          {
           case 's':
           case 'o':
             eldbus_message_iter_arguments_get(var, type, &val);
             if ((!val) || (!val[0])) continue;
             break;
           case 't':
             eldbus_message_iter_arguments_get(var, type, &u);
             break;
           case 'b':
             eldbus_message_iter_arguments_get(var, type, &b);
             break;
           default: break;
          }
        if (!strcmp(key2, "Device"))
          {
             u2->Device = _util_fuckyouglib_convert(var);
             if (!strncmp(u2->Device, "/dev/ram", 8)) return -1;
          }
        else if (!strcmp(key2, "Size"))
          u2->Size = u;
        else if (!strcmp(key2, "Drive"))
          u2->parent = eina_stringshare_add(val);
        else if (!strcmp(key2, "IdUsage"))
          {
             if (!strcmp(val, "other")) return -1;
             u2->volume = !strcmp(val, "filesystem");
          }
        else if (!strcmp(key2, "IdType"))
          u2->IdType = eina_stringshare_add(val);
        else if (!strcmp(key2, "IdLabel"))
          u2->IdLabel = eina_stringshare_add(val);
        else if (!strcmp(key2, "IdUUID"))
          u2->IdUUID = eina_stringshare_add(val);
        else if (!strcmp(key2, "HintIgnore"))
          u2->HintIgnore = !!b;
        else if (!strcmp(key2, "HintSystem"))
          u2->HintSystem = !!b;
        else if (!strcmp(key2, "HintName"))
          u2->HintName = eina_stringshare_add(val);
        else if (!strcmp(key2, "HintIconName"))
          u2->HintIconName = eina_stringshare_add(val);
         block = 1;
     }
   return block;
}

static void
_e_fm_main_udisks2_handle_device(Eldbus_Message_Iter *dict)
{
   const char *udi;
   Eldbus_Message_Iter *arr2, *dict2;
   E_Volume *v = NULL;
   E_Storage *s = NULL;
   U2_Block u2;
   int block = 0;
   unsigned int pnum = UINT_MAX;
   Eina_Stringshare *pname = NULL;

   if (!eldbus_message_iter_arguments_get(dict, "oa{sa{sv}}", &udi, &arr2))
     return;
   memset(&u2, 0, sizeof(U2_Block));
   while (eldbus_message_iter_get_and_next(arr2, 'e', &dict2))
     {
        Eldbus_Message_Iter *arr3, *dict3;
        char *interface;

        if (!eldbus_message_iter_arguments_get(dict2, "sa{sv}", &interface, &arr3))
          continue;
        interface += sizeof(UDISKS2_INTERFACE);
        if (!strcmp(interface, "Partition"))
          {
             while (eldbus_message_iter_get_and_next(arr3, 'e', &dict3))
               {
                  Eldbus_Message_Iter *var;
                  char *key2, *val, *type;

                  if (!eldbus_message_iter_arguments_get(dict3, "sv", &key2, &var))
                    continue;

                  type = eldbus_message_iter_signature_get(var);
                  switch (type[0])
                    {
                     case 's':
                       if (strcmp(key2, "Name")) continue;
                       if (!eldbus_message_iter_arguments_get(var, type, &val)) continue;
                       if (val && val[0]) pname = eina_stringshare_add(val);
                       break;
                     case 'u':
                       if (strcmp(key2, "Number")) continue;
                       eldbus_message_iter_arguments_get(var, type, &pnum);
                       break;
                     default: break;
                    }
               }
             if (v)
               {
                  v->partition_label = pname;
                  pname = NULL;
                  v->partition_number = pnum;
                  v->partition = (v->partition_label || (pnum != UINT_MAX));
               }
          }
        else if (!strcmp(interface, "Filesystem"))
          {
             if (!v) v = _volume_find_by_dbus_path(udi);
             if (!v) v = _e_fm_main_udisks2_volume_add(udi, EINA_TRUE);
             _e_fm_main_udisks2_volume_mounts_update(v, arr3, 1);
             v->partition_label = pname;
             pname = NULL;
             if (pnum != UINT_MAX)
               {
                  v->partition_number = pnum;
                  v->partition = 1;
               }
             if (block)
               {
                  _e_fm_main_udisks2_volume_block_add(v, &u2);
                  if (!v->validated) E_FREE_FUNC(v, _volume_del);
               }
          }
        else if (!strcmp(interface, "Block"))
          {
             block = _e_fm_main_udisks2_block_handle(arr3, &u2);
             switch (block)
               {
                case -1: goto out;
                case 0: continue;
                default: break;
               }
             if ((!s) && (!v))
               {
                  v = _volume_find_by_dbus_path(udi);
                  s = _e_fm_main_udisks2_storage_find(u2.parent);
               }
             if (s)
               _e_fm_main_udisks2_storage_block_add(s, &u2);
             if (v)
               {
                  _e_fm_main_udisks2_volume_block_add(v, &u2);
                  if (!v->validated) E_FREE_FUNC(v, _volume_del);
               }
          }
        else if (!strcmp(interface, "Drive"))
          {
             s = _e_fm_main_udisks2_storage_drive_add(udi, s, arr3);
             if (block) _e_fm_main_udisks2_storage_block_add(s, &u2);
          }
     }
out:
   _e_fm_main_udisks2_block_clear(&u2);
   eina_stringshare_del(pname);
   if (v && v->validated) e_fm_ipc_volume_add(v);
   if (s)
     {
        Eina_List *l;

        if (!block) //cdrom :/
          s->udi = eina_stringshare_add(udi);
        EINA_LIST_FOREACH(_e_vols, l, v)
          {
             if ((v->parent == s->udi) || (v->parent == s->dbus_path))
               {
                  if (!v->storage)
                    {
                       v->storage = s;
                       s->volumes = eina_list_append(s->volumes, v);
                    }
               }
          }
        _e_fm_main_udisks2_storage_add_send(s);
     }
}

static void
_e_fm_main_udisks2_cb_dev_all(void *data EINA_UNUSED, const Eldbus_Message *msg,
                             Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *txt;
   Eldbus_Message_Iter *arr1, *dict1;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("Error %s %s.", name, txt);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "a{oa{sa{sv}}}", &arr1))
     {
        ERR("Error getting arguments.");
        return;
     }

   while (eldbus_message_iter_get_and_next(arr1, 'e', &dict1))
     _e_fm_main_udisks2_handle_device(dict1);
}

static void
_e_fm_main_udisks2_cb_vol_props(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   U2_Block u2;
   int block;
   E_Volume *v = data;
   Eldbus_Message_Iter *arr;

   memset(&u2, 0, sizeof(U2_Block));
   if (!eldbus_message_arguments_get(msg, "a{sv}", &arr))
     {
        ERR("WTF");
        return;
     }
   block = _e_fm_main_udisks2_block_handle(arr, &u2);
   if (block == 1)
     {
        Eina_Bool valid = v->validated;

        _e_fm_main_udisks2_volume_block_add(v, &u2);
        if (v->validated && (valid != v->validated))
          e_fm_ipc_volume_add(v);
        else if (!v->validated)
          _volume_del(v);
     }
   _e_fm_main_udisks2_block_clear(&u2);
}

static void
_e_fm_main_udisks2_cb_dev_add(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
   const char *interface;

   interface = eldbus_message_interface_get(msg);
   //path = eldbus_message_path_get(msg);
   //if (!strncmp(path, "/org/freedesktop/UDisks2/drives", sizeof("/org/freedesktop/UDisks2/drives") - 1))
     //{
        //E_Storage *s;
//
        //if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &arr, &arr2))
          //return;
        //interface += sizeof(UDISKS2_INTERFACE);
        //if (strcmp(interface, "Drive")) return;
        //s = _storage_find_by_dbus_path(path);
        //if (!s) return;
        //valid = s->validated;
        //_e_fm_main_udisks2_storage_drive_add(s->udi, s, arr);
        //if (valid != s->validated)
          //_e_fm_main_udisks2_storage_add_send(s);
     //}
   //else if (!strncmp(path, "/org/freedesktop/UDisks2/block_devices", sizeof("/org/freedesktop/UDisks2/block_devices") - 1))
     //{
        //E_Volume *v;
//
        //if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &arr, &arr2))
          //return;
        //v = _volume_find_by_dbus_path(path);
        //if (!v) v = _e_fm_main_udisks2_volume_add(path, 1);
//
        //interface += sizeof(UDISKS2_INTERFACE);
        //if (!strcmp(interface, "Filesystem"))
          //_e_fm_main_udisks2_volume_mounts_update(v, arr, 0);
        //else if (!strcmp(interface, "Block"))
          //{
             //U2_Block u2;
             //int block;
//
             //memset(&u2, 0, sizeof(U2_Block));
             //block = _e_fm_main_udisks2_block_handle(arr, &u2);
             //if (block == 1)
               //{
                  //valid = v->validated;
//
                  //_e_fm_main_udisks2_volume_block_add(v, &u2);
                  //if (v->validated && (valid != v->validated))
                    //e_fm_ipc_volume_add(v);
                  //else if (!v->validated)
                    //_volume_del(v);
               //}
             //_e_fm_main_udisks2_block_clear(&u2);
          //}
     //}
   //else
   E_Storage *s = eina_list_last_data_get(_e_stores);
   E_Volume *v = eina_list_last_data_get(_e_vols);
   if (!strcmp(interface, ELDBUS_FDO_INTERFACE_OBJECT_MANAGER))
     _e_fm_main_udisks2_handle_device(eldbus_message_iter_get(msg));
   if (_e_stores && (s != eina_list_last_data_get(_e_stores)))
     {
        s = eina_list_last_data_get(_e_stores);
        if (s->proxy)
          eldbus_proxy_property_get_all(s->proxy, _e_fm_main_udisks2_cb_storage_prop_modified, s);
     }
   if (_e_vols && (v != eina_list_last_data_get(_e_vols)))
     {
        Eldbus_Message *m;
        Eldbus_Object *obj;

        v = eina_list_last_data_get(_e_vols);
        m = eldbus_message_method_call_new(UDISKS2_BUS, v->dbus_path, ELDBUS_FDO_INTERFACE_PROPERTIES, "GetAll");
        eldbus_message_arguments_append(m, "s", UDISKS2_INTERFACE_BLOCK);
        obj = eldbus_proxy_object_get(v->proxy);
        eldbus_object_send(obj, m, _e_fm_main_udisks2_cb_vol_props, v, -1);
     }
}

static void
_e_fm_main_udisks2_cb_dev_del(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
   char *path, *interface;
   Eldbus_Message_Iter *arr;
   E_Volume *v;
   Eina_Bool vol = EINA_FALSE, sto = EINA_FALSE;

   if (!eldbus_message_arguments_get(msg, "oas", &path, &arr))
     return;
   DBG("DB DEV-: %s", path);

   while (eldbus_message_iter_get_and_next(arr, 's', &interface))
     {
        interface += sizeof(UDISKS2_INTERFACE);
        if (!strcmp(interface, "Filesystem"))
          vol = EINA_TRUE;
        else if (!strcmp(interface, "Drive"))
          sto = EINA_TRUE;
     }
   if (vol)
     {
        v = _volume_find_by_dbus_path(path);
        if (v)
          {
             if (v->mounted)
               {
                  v->optype = E_VOLUME_OP_TYPE_EJECT;
                  _e_fm_main_udisks2_cb_vol_unmounted(v);
               }
             if (v->optype == E_VOLUME_OP_TYPE_EJECT)
               _e_fm_main_udisks2_cb_vol_ejected(v);
             _volume_del(v);
          }
     }
   if (sto) _e_fm_main_udisks2_storage_del(path);
}

static void
_e_fm_main_udisks2_cb_storage_prop_modified(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   Eldbus_Message_Iter *arr;
   E_Storage *s = data;
   Eina_Bool valid;

   {
     const char *type = eldbus_message_signature_get(msg);
     if (type[0] == 's')
       {
          char *txt;

          if (eldbus_message_arguments_get(msg, type, &txt))
            {
               ERR("%s", txt);
            }
       }
   }
   if (!eldbus_message_arguments_get(msg, "a{sv}", &arr))
     return;
   valid = s->validated;
   _e_fm_main_udisks2_storage_drive_add(s->udi, s, arr);
   if (valid != s->validated)
     _e_fm_main_udisks2_storage_add_send(s);
}

static void
_e_fm_main_udisks2_cb_storage_prop_modified_cb(void *data, const Eldbus_Message *msg)
{
   const char *interface;
   Eldbus_Message_Iter *arr, *arr2;
   E_Storage *s = data;
   Eina_Bool valid;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &arr, &arr2))
     return;
   valid = s->validated;
   _e_fm_main_udisks2_storage_drive_add(s->udi, s, arr);
   if (valid != s->validated)
     _e_fm_main_udisks2_storage_add_send(s);
}

static void
_e_fm_main_udisks2_cb_vol_prop_modified(void *data, const Eldbus_Message *msg, Eldbus_Pending *p EINA_UNUSED)
{
   const char *interface;
   Eldbus_Message_Iter *arr, *arr2;
   E_Volume *v = data;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &interface, &arr, &arr2))
     return;
   interface += sizeof(UDISKS2_INTERFACE);
   if (!strcmp(interface, "Filesystem"))
     _e_fm_main_udisks2_volume_mounts_update(v, arr, 0);
   else if (!strcmp(interface, "Block"))
     {
        U2_Block u2;
        int block;

        memset(&u2, 0, sizeof(U2_Block));
        block = _e_fm_main_udisks2_block_handle(arr, &u2);
        if (block == 1)
          {
             Eina_Bool valid = v->validated;

             _e_fm_main_udisks2_volume_block_add(v, &u2);
             if (v->validated && (valid != v->validated))
               e_fm_ipc_volume_add(v);
             else if (!v->validated)
               _volume_del(v);
          }
        _e_fm_main_udisks2_block_clear(&u2);
     }
}

static void
_e_fm_main_udisks2_cb_vol_prop_modified_cb(void *data, const Eldbus_Message *msg)
{
   _e_fm_main_udisks2_cb_vol_prop_modified(data, msg, NULL);
}

static Eina_Bool
_storage_del(void *data)
{
   const char *path = data;
   _e_fm_main_udisks2_storage_del(path);
   return ECORE_CALLBACK_CANCEL;
}

static E_Storage *
_e_fm_main_udisks2_storage_drive_add(const char *udi, E_Storage *s, Eldbus_Message_Iter *arr3)
{
   Eldbus_Message_Iter *dict3;

   if (!s)
     {
        if (!s) s = _e_fm_main_udisks2_storage_find(udi);
        if (!s) s = _e_fm_main_udisks2_storage_add(udi);
        if (!s) return NULL;
     }
   if (s->system_internal)
     {
        DBG("removing storage internal %s", s->dbus_path);
        s->validated = 0;
        ecore_idler_add(_storage_del, s->dbus_path);
        return NULL;
     }
   while (eldbus_message_iter_get_and_next(arr3, 'e', &dict3))
     {
        Eldbus_Message_Iter *var;
        char *key2, *val, *type;
        Eina_Bool b;
        uint64_t t;

        if (!eldbus_message_iter_arguments_get(dict3, "sv", &key2, &var))
          continue;

        type = eldbus_message_iter_signature_get(var);
        switch (type[0])
          {
           case 's':
           case 'o':
             eldbus_message_iter_arguments_get(var, type, &val);
             break;
           case 'b':
             eldbus_message_iter_arguments_get(var, type, &b);
             break;
           case 't':
             eldbus_message_iter_arguments_get(var, type, &t);
             break;
           default: break;
          }

        if (!strcmp(key2, "ConnectionBus"))
          eina_stringshare_replace(&s->bus, val);
        else if (!strcmp(key2, "MediaCompatibility"))
          {
             Eldbus_Message_Iter *inner_array;
             const char *media;

             if (!eldbus_message_iter_arguments_get(var, "as", &inner_array))
               continue;

             if (eldbus_message_iter_get_and_next(inner_array, 's', &media))
               eina_stringshare_replace(&s->drive_type, media);
          }
        else if (!strcmp(key2, "Model"))
          eina_stringshare_replace(&s->model, (val && val[0]) ? val : NULL);
        else if (!strcmp(key2, "Vendor"))
           eina_stringshare_replace(&s->vendor, (val && val[0]) ? val : NULL);
        else if (!strcmp(key2, "Serial"))
          eina_stringshare_replace(&s->serial, (val && val[0]) ? val : NULL);
        else if (!strcmp(key2, "MediaAvailable"))
          s->media_available = !!b;
        else if (!strcmp(key2, "Size"))
          s->media_size = t;
        else if (!strcmp(key2, "Ejectable"))
          s->requires_eject = !!b;
        else if (!strcmp(key2, "Removable"))
          s->hotpluggable = !!b;
        else if (!strcmp(key2, "MediaRemovable"))
          s->removable = !!b;
     }

   INF("++STO:\n  udi: %s\n  bus: %s\n  drive_type: %s\n  model: %s\n  vendor: %s\n  serial: %s\n  icon.drive: %s\n  icon.volume: %s\n", s->udi, s->bus, s->drive_type, s->model, s->vendor, s->serial, s->icon.drive, s->icon.volume);
   s->validated = EINA_TRUE;

   if (!s->proxy)
     {
        Eldbus_Object *obj;

        obj = eldbus_object_get(_e_fm_main_udisks2_conn, UDISKS2_BUS, s->udi);
        s->proxy = eldbus_proxy_get(obj, UDISKS2_INTERFACE_DRIVE);
        eldbus_proxy_properties_monitor(s->proxy, 1);
        eldbus_proxy_properties_changed_callback_add(s->proxy, _e_fm_main_udisks2_cb_storage_prop_modified_cb, s);
     }
   return s;
}

static int
_e_fm_main_udisks2_format_error_msg(char     **buf,
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
_e_fm_main_udisks2_vol_mount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;

   E_FREE_FUNC(v->op, eldbus_pending_cancel);
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks2_format_error_msg(&buf, v,
                       "org.enlightenment.fm2.MountTimeout",
                       "Unable to mount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_MOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks2_cb_vol_mounted(E_Volume *v)
{
   char *buf;
   int size;

   E_FREE_FUNC(v->guard, ecore_timer_del);
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
_e_fm_main_udisks2_vol_unmount_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   if (v->op)
     eldbus_pending_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks2_format_error_msg(&buf, v,
                      "org.enlightenment.fm2.UnmountTimeout",
                      "Unable to unmount the volume with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_UNMOUNT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fm_main_udisks2_cb_vol_unmounted(E_Volume *v)
{
   char *buf;
   int size;

   E_FREE_FUNC(v->guard, ecore_timer_del);

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
_e_fm_main_udisks2_vol_eject_timeout(E_Volume *v)
{
   char *buf;
   int size;

   v->guard = NULL;
   if (v->op)
     eldbus_pending_cancel(v->op);
   v->op = NULL;
   v->optype = E_VOLUME_OP_TYPE_NONE;
   size = _e_fm_main_udisks2_format_error_msg(&buf, v,
                         "org.enlightenment.fm2.EjectTimeout",
                         "Unable to eject the media with specified time-out.");
   ecore_ipc_server_send(_e_fm_ipc_server, 6 /*E_IPC_DOMAIN_FM*/, E_FM_OP_EJECT_ERROR,
                         0, 0, 0, buf, size);
   free(buf);

   return ECORE_CALLBACK_CANCEL;
}

static Eldbus_Pending *
_volume_eject(Eldbus_Proxy *proxy)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;

   msg = eldbus_proxy_method_call_new(proxy, "Eject");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "a{sv}", &array);
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(proxy, msg, NULL, NULL, -1);
}

static void
_volume_eject_umount_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *name, *txt;
   E_Volume *v = data;

   if (eldbus_message_error_get(msg, &name, &txt))
     {
        ERR("%s: %s", name, txt);
     }
   else if (v->optype == E_VOLUME_OP_TYPE_EJECT)
     {
        vols_ejecting = eina_list_remove(vols_ejecting, v);
        _volume_eject(v->storage->proxy);
     }

}

static Eldbus_Pending *
_volume_umount(E_Volume *v)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter;

   msg = eldbus_proxy_method_call_new(v->proxy, "Unmount");
   main_iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(main_iter, "a{sv}", &array);
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(v->proxy, msg, _volume_eject_umount_cb, v, -1);
}

static Eldbus_Pending *
_volume_mount(Eldbus_Proxy *proxy, const char *fstype, const char *buf)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *array, *main_iter, *dict, *var;
   char opt_txt[] = "options";
   char fs_txt[] = "fstype";

   msg = eldbus_proxy_method_call_new(proxy, "Mount");
   main_iter = eldbus_message_iter_get(msg);
   if (!eldbus_message_iter_arguments_append(main_iter, "a{sv}", &array))
     ERR("E");
   if (fstype)
     {
        if (!eldbus_message_iter_arguments_append(array, "{sv}", &dict))
          {
             ERR("E");
          }
        eldbus_message_iter_basic_append(dict, 's', fs_txt);

        var = eldbus_message_iter_container_new(dict, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', fstype);
        eldbus_message_iter_container_close(dict, var);
        eldbus_message_iter_container_close(array, dict);

     }
   if (buf[0])
     {
        if (!eldbus_message_iter_arguments_append(array, "{sv}", &dict))
          {
             ERR("E");
          }
        eldbus_message_iter_basic_append(dict, 's', opt_txt);

        var = eldbus_message_iter_container_new(dict, 'v', "s");
        eldbus_message_iter_basic_append(var, 's', buf);
        eldbus_message_iter_container_close(dict, var);
        eldbus_message_iter_container_close(array, dict);
     }
   eldbus_message_iter_container_close(main_iter, array);

   return eldbus_proxy_send(proxy, msg, NULL, NULL, -1);
}

static void
_e_fm_main_udisks2_cb_vol_ejected(E_Volume *v)
{
   char *buf;
   int size;

   E_FREE_FUNC(v->guard, ecore_timer_del);
   v->optype = E_VOLUME_OP_TYPE_NONE;

   size = strlen(v->udi) + 1;
   buf = alloca(size);
   strcpy(buf, v->udi);
   ecore_ipc_server_send(_e_fm_ipc_server,
                         6 /*E_IPC_DOMAIN_FM*/,
                         E_FM_OP_EJECT_DONE,
                         0, 0, 0, buf, size);
}

static void
_e_fm_main_udisks2_volume_mounts_update(E_Volume *v, Eldbus_Message_Iter *arr3, Eina_Bool first)
{
   Eldbus_Message_Iter *dict3;
   Eina_List *mounts = NULL;

   while (eldbus_message_iter_get_and_next(arr3, 'e', &dict3))
     {
        Eldbus_Message_Iter *var;
        char *key2, *type;

        if (!eldbus_message_iter_arguments_get(dict3, "sv", &key2, &var))
          continue;

        type = eldbus_message_iter_signature_get(var);
        switch (type[0])
          {
           Eldbus_Message_Iter *fuckyouglib;

           case 'a':
             if (strcmp(key2, "MountPoints")) continue;
             /* glib developers are assholes, so this is "aay" instead of just "as" */
             if (!eldbus_message_iter_arguments_get(var, type, &fuckyouglib))
               {
                  ERR("fucked by glib I guess");
                  continue;
               }
             do
               {
                  Eina_Stringshare *m;

                  m = _util_fuckyouglib_convert(fuckyouglib);
                  if (m) mounts = eina_list_append(mounts, m);
               }
             while (eldbus_message_iter_next(fuckyouglib));
             EINA_FALLTHROUGH;
             /* no break */

           default: continue;
          }
     }
   if (first)
     {
        eina_stringshare_replace(&v->mount_point, eina_list_last_data_get(mounts));
        v->mounted = !!v->mount_point;
        if (v->storage)
          v->storage->media_available = v->mounted;
     }
   else if ((!v->mounted) || (!eina_list_data_find(mounts, v->mount_point)))
     {
        if (mounts)
          eina_stringshare_replace(&v->mount_point, eina_list_last_data_get(mounts));
        v->mounted = !!mounts;
        if (v->storage)
          v->storage->media_available = v->mounted;
        switch (v->optype)
          {
             case E_VOLUME_OP_TYPE_MOUNT:
               _e_fm_main_udisks2_cb_vol_mounted(v);
               break;
             case E_VOLUME_OP_TYPE_UNMOUNT:
               _e_fm_main_udisks2_cb_vol_unmounted(v);
               break;
             case E_VOLUME_OP_TYPE_EJECT:
               if (vols_ejecting && eina_list_data_find(vols_ejecting, v))
                 _e_fm_main_udisks2_cb_vol_unmounted(v);
               else
                 _e_fm_main_udisks2_cb_vol_ejected(v);
               break;
             default: break;
          }
     }
   E_FREE_LIST(mounts, eina_stringshare_del);
}

E_Volume *
_e_fm_main_udisks2_volume_add(const char *path, Eina_Bool first_time)
{
   E_Volume *v;
   Eldbus_Object *obj;

   if (!path) return NULL;
   if (_volume_find_by_dbus_path(path)) return NULL;
   v = calloc(1, sizeof(E_Volume));
   if (!v) return NULL;
   v->dbus_path = eina_stringshare_add(path);
   INF("VOL+ %s", path);
   v->efm_mode = EFM_MODE_USING_UDISKS2_MOUNT;
   v->first_time = first_time;
   _e_vols = eina_list_append(_e_vols, v);
   obj = eldbus_object_get(_e_fm_main_udisks2_conn, UDISKS2_BUS, path);
   v->proxy = eldbus_proxy_get(obj, UDISKS2_INTERFACE_FILESYSTEM);
   eldbus_proxy_properties_monitor(v->proxy, 1);
   eldbus_proxy_properties_changed_callback_add(v->proxy, _e_fm_main_udisks2_cb_vol_prop_modified_cb, v);

   return v;
}

void
_e_fm_main_udisks2_volume_del(const char *path)
{
   E_Volume *v;

   v = _volume_find_by_dbus_path(path);
   if (!v) return;
   _volume_del(v);
}

static void
_volume_del(E_Volume *v)
{
   E_FREE_FUNC(v->guard, ecore_timer_del);
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
_e_fm_main_udisks2_volume_find(const char *udi)
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
_e_fm_main_udisks2_volume_eject(E_Volume *v)
{
   if (!v || v->guard) return;
   if (!v->storage) v->storage = _e_fm_main_udisks2_storage_find(v->parent);
   if (!v->storage)
     {
        ERR("COULD NOT FIND DISK STORAGE!!!");
        return;
     }
   if (v->mounted)
     {
        v->guard = ecore_timer_loop_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks2_vol_unmount_timeout, v);
        v->op = _volume_umount(v);
        vols_ejecting = eina_list_append(vols_ejecting, v);
     }
   else
     {
        v->guard = ecore_timer_loop_add(E_FM_EJECT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks2_vol_eject_timeout, v);
        v->op = _volume_eject(v->storage->proxy);
     }
   v->optype = E_VOLUME_OP_TYPE_EJECT;
}

void
_e_fm_main_udisks2_volume_unmount(E_Volume *v)
{
   if (!v || v->guard) return;
   INF("unmount %s %s", v->udi, v->mount_point);

   v->guard = ecore_timer_loop_add(E_FM_UNMOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks2_vol_unmount_timeout, v);
   v->op = _volume_umount(v);
   v->optype = E_VOLUME_OP_TYPE_UNMOUNT;
}

void
_e_fm_main_udisks2_volume_mount(E_Volume *v)
{
   char buf[256] = {0};

   if ((!v) || (v->guard))
     return;

   INF("mount %s %s [fs type = %s]", v->udi, v->mount_point, v->fstype);

   // Map uid to current user if possible
   // Possible filesystems found in udisks2 source (src/udisks2linuxfilesystem.c) as of 2012-07-11
   if (!strcmp(v->fstype, "vfat"))
     snprintf(buf, sizeof(buf), "uid=%i,utf8=1", (int)getuid());
   else if ((!strcmp(v->fstype, "iso9660")) || (!strcmp(v->fstype, "udf")))
   // force utf8 as the encoding - e only likes/handles utf8. its the
   // pseudo-standard these days anyway for "linux" for intl text to work
   // everywhere. problem is some fs's use differing options and udisks2
   // doesn't allow some options with certain filesystems.
   // Valid options found in udisks2 (src/udisks2linuxfilesystem.c) as of 2012-07-11
   // Note that these options are default with the latest udisks2, kept here to
   // avoid breakage in the future (hopefully).
     snprintf(buf, sizeof(buf), "uid=%i,iocharset=utf8", (int)getuid());
   else if (!strcmp(v->fstype, "ntfs"))
     snprintf(buf, sizeof(buf), "uid=%i", (int)getuid());

   v->guard = ecore_timer_loop_add(E_FM_MOUNT_TIMEOUT, (Ecore_Task_Cb)_e_fm_main_udisks2_vol_mount_timeout, v);

   // It was previously noted here that ubuntu 10.04 failed to mount if opt was passed to
   // e_udisks2_volume_mount.  The reason at the time was unknown and apparently never found.
   // I theorize that this was due to improper mount options being passed (namely the utf8 options).
   // If this still fails on Ubuntu 10.04 then an actual fix should be found.
   v->op = _volume_mount(v->proxy, v->fstype, buf);

   v->optype = E_VOLUME_OP_TYPE_MOUNT;
}

void
_e_fm_main_udisks2_init(void)
{
   eldbus_init();
   _e_fm_main_udisks2_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!_e_fm_main_udisks2_conn) return;

   eldbus_name_start(_e_fm_main_udisks2_conn, UDISKS2_BUS, 0,
                    _e_fm_main_udisks2_name_start, NULL);
}

void
_e_fm_main_udisks2_shutdown(void)
{
   if (_e_fm_main_udisks2_proxy)
     {
        Eldbus_Object *obj;
        obj = eldbus_proxy_object_get(_e_fm_main_udisks2_proxy);
        eldbus_proxy_unref(_e_fm_main_udisks2_proxy);
        eldbus_object_unref(obj);
     }
   if (_e_fm_main_udisks2_conn)
     {
        eldbus_connection_unref(_e_fm_main_udisks2_conn);
        eldbus_shutdown();
     }
}

E_Storage *
_e_fm_main_udisks2_storage_add(const char *path)
{
   E_Storage *s;

   if (!path) return NULL;
   if (_storage_find_by_dbus_path(path)) return NULL;
   s = calloc(1, sizeof(E_Storage));
   if (!s) return NULL;

   DBG("STORAGE+=%s", path);
   s->dbus_path = eina_stringshare_add(path);
   s->udi = eina_stringshare_ref(s->dbus_path);
   _e_stores = eina_list_append(_e_stores, s);
   return s;
}

void
_e_fm_main_udisks2_storage_del(const char *path)
{
   E_Storage *s;

   s = _storage_find_by_dbus_path(path);
   if (!s) return;
   if (s->validated)
     {
        stores_registered = eina_list_remove(stores_registered, s);
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
   while (s->volumes)
     _volume_del(eina_list_data_get(s->volumes));
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
_e_fm_main_udisks2_storage_find(const char *udi)
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

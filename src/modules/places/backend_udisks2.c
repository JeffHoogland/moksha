
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef PLACES_HAVE_UDISKS2

#include <e.h>
#include <Eldbus.h>
#include "e_mod_main.h"
#include "e_mod_places.h"


/* Enable/Disable debug messages */
// #define PDBG(...) do {} while (0)
#define PDBG(...) printf("PLACES(ud2): "__VA_ARGS__)


/* UDisks2 defines */
#define UDISKS2_BUS "org.freedesktop.UDisks2"
#define UDISKS2_PATH "/org/freedesktop/UDisks2"

#define UDISKS2_MANAGER_PATH "/org/freedesktop/UDisks2/Manager"
#define UDISKS2_MANAGER_IFACE "org.freedesktop.UDisks2.Manager"

#define UDISKS2_BLOCK_IFACE          "org.freedesktop.UDisks2.Block"
#define UDISKS2_PARTITION_IFACE      "org.freedesktop.UDisks2.Partition"
#define UDISKS2_FILESYSTEM_IFACE     "org.freedesktop.UDisks2.Filesystem"
#define UDISKS2_DRIVE_IFACE          "org.freedesktop.UDisks2.Drive"
#define UDISKS2_PARTITIONTABLE_IFACE "org.freedesktop.UDisks2.PartitionTable"
#define UDISKS2_LOOP_IFACE           "org.freedesktop.UDisks2.Loop"
#define UDISKS2_SWAPSPACE_IFACE      "org.freedesktop.UDisks2.Swapspace"

#define UDISKS2_BLOCK_PREFIX "/org/freedesktop/UDisks2/block_devices/"
#define UDISKS2_DRIVE_PREFIX "/org/freedesktop/UDisks2/drives/"


/* Local backend data */
typedef struct Places_UDisks2_Backend_Data
{
   Eldbus_Object *block_obj;
   Eldbus_Object *drive_obj;
} Places_UDisks2_Backend_Data;


/* Local Function Prototypes */
static void _places_ud2_name_start(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__);

static void _places_ud2_get_managed_objects_cb(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending) __UNUSED__;
static void _places_ud2_interfaces_added_cb(void *data __UNUSED__, const Eldbus_Message *msg);
static void _places_ud2_interfaces_removed_cb(void *data __UNUSED__, const Eldbus_Message *msg);
static void _places_ud2_block_props_changed_cb(void *data, const Eldbus_Message *msg);
static void _places_ud2_drive_props_changed_cb(void *data, const Eldbus_Message *msg);

static void _places_ud2_read_block_ifaces(const char *obj_path, Eldbus_Message_Iter *ifaces_array, Eina_Bool first_time);
static void _places_ud2_read_drive_properies(Volume *vol, Eldbus_Message_Iter* props_array);
static void _places_ud2_drive_props_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__);

static Volume* _places_ud2_volume_add(const char *block_obj_path,  const char* drive_obj_path, Eina_Bool first_time);
static void _places_ud2_volume_finalize(Volume *vol);
static void _places_ud2_mount_func(Volume *vol, Eina_List *opts __UNUSED__);
static void _places_ud2_unmount_func(Volume *vol, Eina_List *opts __UNUSED__);
static void _places_ud2_eject_func(Volume *vol, Eina_List *opts __UNUSED__);
static void _places_ud2_volume_free_func(Volume *vol);


/* Local Variables */
static Eldbus_Connection *_places_ud2_conn = NULL;
static Eldbus_Object *_places_ud2_object_manager = NULL;


Eina_Bool
places_udisks2_init(void)
{
   // PDBG("init()\n");

   EINA_SAFETY_ON_FALSE_RETURN_VAL(eldbus_init(), EINA_FALSE);

   _places_ud2_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!_places_ud2_conn)
   {
      printf("udisks2: Error connecting to system bus.\n");
      return EINA_FALSE;
   }

   eldbus_name_start(_places_ud2_conn, UDISKS2_BUS, 0,
                     _places_ud2_name_start, NULL);
   return EINA_TRUE;
}


void
places_udisks2_shutdown(void)
{
   // PDBG("shutdown()\n");
   E_FREE_FUNC(_places_ud2_object_manager, eldbus_object_unref);
   E_FREE_FUNC(_places_ud2_conn, eldbus_connection_unref);
   eldbus_shutdown();
}


static Volume*
_places_ud2_volume_add(const char *block_obj_path, const char* drive_obj_path, Eina_Bool first_time)
{
   Places_UDisks2_Backend_Data *bdata;
   Volume *vol;

   // return a previously created Volume
   vol = places_volume_by_id_get(block_obj_path);
   if (vol) return vol;

   // create the backend data struct
   bdata = E_NEW(Places_UDisks2_Backend_Data, 1);
   if (!bdata) return NULL;

   // create the Eldbus block_device object
   bdata->block_obj = eldbus_object_get(_places_ud2_conn, UDISKS2_BUS, 
                                        block_obj_path);
   if (!bdata->block_obj) return NULL;

   // create the Eldbus drive object (if exists)
   if (drive_obj_path && drive_obj_path[0] && !eina_streq(drive_obj_path, "/")) 
   {
      bdata->drive_obj = eldbus_object_get(_places_ud2_conn, UDISKS2_BUS, 
                                           drive_obj_path);
      if (!bdata->drive_obj) return NULL;
   }

   // create the places Volume
   vol = places_volume_add(block_obj_path, first_time);
   if (!vol) return NULL;
   vol->backend_data = bdata;
   vol->mount_func = _places_ud2_mount_func;
   vol->unmount_func = _places_ud2_unmount_func;
   vol->eject_func = _places_ud2_eject_func;
   vol->free_func = _places_ud2_volume_free_func;

   // Get notifications on object change properties
   eldbus_object_signal_handler_add(bdata->block_obj, 
                                    ELDBUS_FDO_INTERFACE_PROPERTIES, 
                                    "PropertiesChanged",
                                    _places_ud2_block_props_changed_cb, vol);
   if (bdata->drive_obj)
      eldbus_object_signal_handler_add(bdata->drive_obj, 
                                       ELDBUS_FDO_INTERFACE_PROPERTIES, 
                                       "PropertiesChanged",
                                       _places_ud2_drive_props_changed_cb, vol);

   return vol;
}


static void
_places_ud2_volume_free_func(Volume *vol)
{
   Places_UDisks2_Backend_Data *bdata = vol->backend_data;
   if (bdata)
   {
      E_FREE_FUNC(bdata->block_obj, eldbus_object_unref);
      E_FREE_FUNC(bdata->drive_obj, eldbus_object_unref);
      E_FREE(vol->backend_data);
   }
}


/* Some strings in UDisks2 are exposed as array of char (ay), need to convert */
static char*
_places_udisks2_string_convert(Eldbus_Message_Iter *ay)
{
   char *p, buf[PATH_MAX] = {0};
   p = buf;
   while (eldbus_message_iter_get_and_next(ay, 'y', p))
   {
      if (!p) break;
      p++;
   }
   return strdup(buf);
}


/* The Udisks2 service is up and running, setup the ObjectManager */
static void
_places_ud2_name_start(void *data __UNUSED__, const Eldbus_Message *msg,
                           Eldbus_Pending *pending __UNUSED__)
{
   Eldbus_Proxy *proxy;

   PLACES_ON_MSG_ERROR_RETURN(msg);

   // get the UDisks2 ObjectManager object
   _places_ud2_object_manager = eldbus_object_get(_places_ud2_conn, 
                                                      UDISKS2_BUS, 
                                                      UDISKS2_PATH);
   proxy = eldbus_proxy_get(_places_ud2_object_manager, 
                            ELDBUS_FDO_INTERFACE_OBJECT_MANAGER);
   // NOTE: proxy will be automatically deleted on obj deletion
   
   // request all the objects managed by the ObjectManager
   eldbus_proxy_call(proxy, "GetManagedObjects",
                     _places_ud2_get_managed_objects_cb, NULL, -1, "");

   // connect the ObjectManager InterfacesAdded/Removed signal handlers
   eldbus_proxy_signal_handler_add(proxy, "InterfacesAdded",
                                  _places_ud2_interfaces_added_cb, NULL);
   eldbus_proxy_signal_handler_add(proxy, "InterfacesRemoved",
                                  _places_ud2_interfaces_removed_cb, NULL);
}


/* Callback for the DBus ObjectManager method 'GetManagedObjects' */
static void 
_places_ud2_get_managed_objects_cb(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__)
{
   Eldbus_Message_Iter *objs_array, *obj_entry;
   Eldbus_Message_Iter *ifaces_array;
   const char *obj_path;

   PLACES_ON_MSG_ERROR_RETURN(msg);

   // PDBG("GetManagedObjects\n");
   if (!eldbus_message_arguments_get(msg, "a{oa{sa{sv}}}", &objs_array))
     return;
   
   while (eldbus_message_iter_get_and_next(objs_array, 'e', &obj_entry))
   {
      if (eldbus_message_iter_arguments_get(obj_entry, "oa{sa{sv}}", 
                                            &obj_path, &ifaces_array))
      {
         // we are only interested at block_device objects
         if (eina_str_has_prefix(obj_path, UDISKS2_BLOCK_PREFIX))
            _places_ud2_read_block_ifaces(obj_path, ifaces_array, EINA_TRUE);
      }
   }
}


/* Callback for the ObjectManager signal 'InterfacesAdded' */
static void
_places_ud2_interfaces_added_cb(void *data __UNUSED__, const Eldbus_Message *msg)
{
   const char *obj_path;
   Eldbus_Message_Iter *ifaces_array;

   PLACES_ON_MSG_ERROR_RETURN(msg);

   if (eldbus_message_arguments_get(msg, "oa{sa{sv}}", &obj_path, &ifaces_array))
   {
      // PDBG("InterfacesAdded on obj: %s\n", obj_path);
      // we are only interested at block_device objects
      if (eina_str_has_prefix(obj_path, UDISKS2_BLOCK_PREFIX))
         _places_ud2_read_block_ifaces(obj_path, ifaces_array, EINA_FALSE);
   }
}


/* Callback for the ObjectManager signal 'InterfacesRemoved' */
static void
_places_ud2_interfaces_removed_cb(void *data __UNUSED__, const Eldbus_Message *msg)
{
   const char *obj_path, *iface_name;
   Eldbus_Message_Iter *array_ifaces;
   Volume *vol;

   PLACES_ON_MSG_ERROR_RETURN(msg);

   if (!eldbus_message_arguments_get(msg, "oas", &obj_path, &array_ifaces))
     return;

   while (eldbus_message_iter_get_and_next(array_ifaces, 's', &iface_name))
   {
      // PDBG("InterfaceRemoved obj:%s - iface:%s\n", obj_path, iface_name);
      if (eina_streq(iface_name, UDISKS2_FILESYSTEM_IFACE) || 
          eina_streq(iface_name, UDISKS2_BLOCK_IFACE))
         if ((vol = places_volume_by_id_get(obj_path)))
            places_volume_del(vol);
   }
}


/* Callback for DBUS signal "PropertiesChanged" on the block_device objects */
static void
_places_ud2_block_props_changed_cb(void *data, const Eldbus_Message *msg)
{
   Volume *vol = data;
   Eldbus_Message_Iter *changed_props, *invalidated_props, *props_entry, *var;
   const char *iface_name, *prop_name;
   char *mount_point = NULL;

   EINA_SAFETY_ON_NULL_RETURN(vol);
   PLACES_ON_MSG_ERROR_RETURN(msg);

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface_name, 
                                     &changed_props, &invalidated_props))
     return;
   
   // PDBG("PropertiesChanged  obj:%s - iface:%s\n", vol->id, iface_name);
   
   // atm we are only interested in the mounted state (on the FS iface)
   if (!eina_streq(iface_name, UDISKS2_FILESYSTEM_IFACE))
      return;

   while (eldbus_message_iter_get_and_next(changed_props, 'e', &props_entry))
   {
      if (!eldbus_message_iter_arguments_get(props_entry, "sv", &prop_name, &var))
         continue;

      // PDBG("   Changed prop: %s\n", prop_name);
      if (eina_streq(prop_name, "MountPoints"))
      {
         Eldbus_Message_Iter *mounts_array, *ay;
         if (eldbus_message_iter_arguments_get(var, "aay", &mounts_array))
            if (eldbus_message_iter_arguments_get(mounts_array, "ay", &ay))
               mount_point = _places_udisks2_string_convert(ay);
         eina_stringshare_replace(&vol->mount_point, mount_point);
         if (mount_point) free(mount_point);
         _places_ud2_volume_finalize(vol);
      }
   }
}


/* Callback for DBUS signal "PropertiesChanged" on the drive objects */
static void
_places_ud2_drive_props_changed_cb(void *data, const Eldbus_Message *msg)
{
   Volume *vol = data;
   Eldbus_Message_Iter *changed_props, *invalidated_props;
   const char *iface_name;

   EINA_SAFETY_ON_NULL_RETURN(vol);
   PLACES_ON_MSG_ERROR_RETURN(msg);

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface_name, 
                                     &changed_props, &invalidated_props))
     return;
   
   // PDBG("PropertiesChanged  obj:%s - iface:%s\n", vol->id, iface_name);
   if (eina_streq(iface_name, UDISKS2_DRIVE_IFACE))
      _places_ud2_read_drive_properies(vol, changed_props);
}


/* Read all the ifaces for a block_device object */
static void 
_places_ud2_read_block_ifaces(const char *block_obj_path, Eldbus_Message_Iter *ifaces_array, Eina_Bool first_time)
{
   Eldbus_Message_Iter *iface_entry, *props_array, *props_entry, *chrarray, *var;
   const char *iface_name, *key;
   Eina_Bool bool_val;
   
   char *device = NULL;
   char *mount_point = NULL;
   const char *drive_obj_path = NULL;
   const char *block_label = NULL;
   const char *partition_name = NULL;
   const char *fstype = NULL;
   unsigned long long blockdevice_size = 0;
   unsigned long long filesystem_size = 0;

   // PDBG("Parsing block_device: %s\n", block_obj_path);
   
   while (eldbus_message_iter_get_and_next(ifaces_array, 'e', &iface_entry))
   {
      if (!eldbus_message_iter_arguments_get(iface_entry, "sa{sv}", &iface_name, &props_array))
         continue;
      
      // PDBG("PLACES:    Iface: %s\n", iface_name);
      while (eldbus_message_iter_get_and_next(props_array, 'e', &props_entry))
      {
         if (!eldbus_message_iter_arguments_get(props_entry, "sv", &key, &var))
            continue;
         // PDBG("PLACES:      Prop: %s\n", key);

         /* block device with the following ifaces must be ignored */
         if(eina_streq(iface_name, UDISKS2_PARTITIONTABLE_IFACE) ||
            eina_streq(iface_name, UDISKS2_LOOP_IFACE) ||
            eina_streq(iface_name, UDISKS2_SWAPSPACE_IFACE))
         {
            goto skip;
         }

         /* Block iface */
         else if (eina_streq(iface_name, UDISKS2_BLOCK_IFACE)) 
         {
            // skip volumes with HintIgnore set
            if (eina_streq(key, "HintIgnore"))
            {
               eldbus_message_iter_arguments_get(var, "b", &bool_val);
               if (bool_val) goto skip;
            }
            else if (eina_streq(key, "Device"))
            {
               eldbus_message_iter_arguments_get(var, "ay", &chrarray);
               device = _places_udisks2_string_convert(chrarray);
            }
            else if (eina_streq(key, "IdLabel"))
               eldbus_message_iter_arguments_get(var, "s", &block_label);
            else if (eina_streq(key, "IdType"))
               eldbus_message_iter_arguments_get(var, "s", &fstype);
            else if (eina_streq(key, "Size"))
               eldbus_message_iter_arguments_get(var, "t", &blockdevice_size);
            else if (eina_streq(key, "Drive"))
               eldbus_message_iter_arguments_get(var, "o", &drive_obj_path);
         }
         
         /* Partition iface */
         else if (eina_streq(iface_name, UDISKS2_PARTITION_IFACE))
         {
            if (eina_streq(key, "Name"))
               eldbus_message_iter_arguments_get(var, "s", &partition_name);
         }

         /* Filesystem iface */
         else if (eina_streq(iface_name, UDISKS2_FILESYSTEM_IFACE))
         {
            if (eina_streq(key, "Size"))
               eldbus_message_iter_arguments_get(var, "t", &filesystem_size);
            else if (eina_streq(key, "MountPoints")) 
            {
               Eldbus_Message_Iter *inner_array, *chrarray2;
               if (eldbus_message_iter_arguments_get(var, "aay", &inner_array))
                  if (eldbus_message_iter_arguments_get(inner_array, "ay", &chrarray2))
                     mount_point = _places_udisks2_string_convert(chrarray2);
            }
         }
         // else PDBG("PLACES: WARNING - Unknown iface: %s\n", iface_name);
      }
   }

   // PDBG("Found device: %s\n", block_obj_path);

   // create (or get an already created) places Volume
   Volume *vol = _places_ud2_volume_add(block_obj_path, drive_obj_path, first_time);
   EINA_SAFETY_ON_NULL_GOTO(vol, skip);

   // choose the best label
   if (partition_name && partition_name[0])
      eina_stringshare_replace(&vol->label, partition_name);
   else if (block_label && block_label[0])
      eina_stringshare_replace(&vol->label, block_label);
   else if (device && device[0])
      eina_stringshare_replace(&vol->label, device);

   // store all other props in the Volume
   if (mount_point) eina_stringshare_replace(&vol->mount_point, mount_point);
   if (device) eina_stringshare_replace(&vol->device, device);
   if (fstype) eina_stringshare_replace(&vol->fstype, fstype);
   vol->size = filesystem_size > 0 ? filesystem_size : blockdevice_size;

   // now request the properties of the parent Drive
   Places_UDisks2_Backend_Data *bdata = vol->backend_data;
   if (bdata->drive_obj) 
   {
      Eldbus_Proxy *proxy;
      proxy = eldbus_proxy_get(bdata->drive_obj, UDISKS2_DRIVE_IFACE);
      eldbus_proxy_property_get_all(proxy, _places_ud2_drive_props_cb, vol);
      eldbus_proxy_unref(proxy);
   } 
   // else PDBG("WARNING - NO DRIVE FOR OBJECT %s\n", block_obj_path);

skip:
   if (device) free(device);
   if (mount_point) free(mount_point);
}


/* Callback for DBUS signal "PropertiesChanged" on the drive objects */
static void
_places_ud2_drive_props_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__)
{
   Volume *vol = data;
   Eldbus_Message_Iter *props_array;

   EINA_SAFETY_ON_NULL_RETURN(vol);
   PLACES_ON_MSG_ERROR_RETURN(msg);
   EINA_SAFETY_ON_FALSE_RETURN(eldbus_message_arguments_get(msg, "a{sv}", &props_array));
   // PDBG("Drive properties for: %s\n", vol->id);
   _places_ud2_read_drive_properies(vol, props_array);
}


/* Read the properties of a drive object (Drive ifaces) */
static void
_places_ud2_read_drive_properies(Volume *vol, Eldbus_Message_Iter* props_array)
{
   Eldbus_Message_Iter *prop_entry, *var;
   const char *key = NULL;
   const char *string = NULL;

   EINA_SAFETY_ON_NULL_RETURN(vol);

   while (eldbus_message_iter_get_and_next(props_array, 'e', &prop_entry))
   {
      if (!eldbus_message_iter_arguments_get(prop_entry, "sv", &key, &var))
         continue;

      if (eina_streq(key, "ConnectionBus"))
      {
         eldbus_message_iter_arguments_get(var, "s", &string);
         eina_stringshare_replace(&vol->bus, string);
      }
      else if (eina_streq(key, "Serial"))
      {
         eldbus_message_iter_arguments_get(var, "s", &string);
         eina_stringshare_replace(&vol->serial, string);
      }
      else if (eina_streq(key, "Vendor"))
      {
         eldbus_message_iter_arguments_get(var, "s", &string);
         eina_stringshare_replace(&vol->vendor, string);
      }
      else if (eina_streq(key, "Model")) 
      {
         eldbus_message_iter_arguments_get(var, "s", &string);
         eina_stringshare_replace(&vol->model, string);
      }
      else if (eina_streq(key, "Media"))
      {
         eldbus_message_iter_arguments_get(var, "s", &string);
         eina_stringshare_replace(&vol->drive_type, string);
      }
      else if (eina_streq(key, "MediaAvailable"))
         eldbus_message_iter_arguments_get(var, "b", &vol->media_available);
      else if (eina_streq(key, "Removable"))
         eldbus_message_iter_arguments_get(var, "b", &vol->removable);
      else if (eina_streq(key, "Ejectable"))
         eldbus_message_iter_arguments_get(var, "b", &vol->requires_eject);
   }

   _places_ud2_volume_finalize(vol);
}


/* Called after all properties has been readed */
static void
_places_ud2_volume_finalize(Volume *vol)
{
   Eina_Bool is_valid = EINA_TRUE;
   // PDBG("Validating %s\n", vol->id);

   // set mounted state
   if (vol->mount_point && vol->mount_point[0])
      vol->mounted = EINA_TRUE;
   else
      vol->mounted = EINA_FALSE;

   // media must be available
   if (!vol->media_available)
   {
      is_valid = EINA_FALSE;
   }


   // the update is always needed to trigger auto_mount/auto_open
   places_volume_update(vol);

   if (is_valid != vol->valid)
   {
      // trigger a full redraw, is the only way to show/hide a new device
      vol->valid = is_valid;
      places_update_all_gadgets();
   }
}


/* Callback for mount(), umount() and eject() calls */
static void
_places_udisks2_task_cb(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending __UNUSED__)
{
   const char *str;
   Eina_Bool ret;

   if (eldbus_message_error_get(msg, NULL, NULL))
   {
      ret = eldbus_message_arguments_get(msg, "s", &str);
      e_util_dialog_internal(_("Operation failed"),
                             ret ? str : _("Unknown error"));
   }
}


static void
_places_ud2_mount_func(Volume *vol, Eina_List *opts __UNUSED__)
{
   Places_UDisks2_Backend_Data *bdata = vol->backend_data;
   Eldbus_Message_Iter *iter, *array;
   Eldbus_Message *msg;

   EINA_SAFETY_ON_NULL_RETURN(bdata);

   // Call the Mount(a{sv}) method on the Filesystem iface
   msg = eldbus_object_method_call_new(bdata->block_obj, 
                                       UDISKS2_FILESYSTEM_IFACE, "Mount");
   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(iter, "a{sv}", &array);
   eldbus_message_iter_container_close(iter, array);
   eldbus_object_send(bdata->block_obj, msg, _places_udisks2_task_cb, vol, -1);
   
   // TODO options?? still needed??
   // eldbus_message_iter_arguments_append(main_iter, "sas", vol->fstype, &array);
   // Eina_List *l;
   // const char *opt_txt;
   // EINA_LIST_FOREACH(opts, l, opt_txt)
   // {
   //    PDBG("  opt: '%s'\n", opt_txt);
   //    eldbus_message_iter_basic_append(array, 's', opt_txt);
   // }
   // eldbus_message_iter_container_close(main_iter, array);
}


static void
_places_ud2_unmount_func(Volume *vol, Eina_List *opts __UNUSED__)
{
   Places_UDisks2_Backend_Data *bdata = vol->backend_data;
   Eldbus_Message_Iter *iter, *array;
   Eldbus_Message *msg;

   EINA_SAFETY_ON_NULL_RETURN(bdata);

   // Call the Unmount(a{sv}) method on the Filesystem iface
   msg = eldbus_object_method_call_new(bdata->block_obj, 
                                       UDISKS2_FILESYSTEM_IFACE, "Unmount");
   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(iter, "a{sv}", &array);
   eldbus_message_iter_container_close(iter, array);
   eldbus_object_send(bdata->block_obj, msg, _places_udisks2_task_cb, vol, -1);
}


static void
_places_ud2_eject_func(Volume *vol, Eina_List *opts __UNUSED__)
{
   Places_UDisks2_Backend_Data *bdata = vol->backend_data;
   Eldbus_Message_Iter *iter, *array;
   Eldbus_Message *msg;

   EINA_SAFETY_ON_NULL_RETURN(bdata);

   // Call the Eject(a{sv}) method on the drive object (Drive iface)
   msg = eldbus_object_method_call_new(bdata->drive_obj, 
                                       UDISKS2_DRIVE_IFACE, "Eject");
   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_arguments_append(iter, "a{sv}", &array);
   eldbus_message_iter_container_close(iter, array);
   eldbus_object_send(bdata->drive_obj, msg, _places_udisks2_task_cb, vol, -1);
}

#undef // PDBG
#endif

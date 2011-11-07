#include "e_illume_private.h"
#include "e_mod_kbd_device.h"

/* local function prototypes */
static void _e_mod_kbd_device_ignore_load(void);
static void _e_mod_kbd_device_ignore_load_file(const char *file);
static void _e_mod_kbd_device_kbd_add(const char *udi);
static void _e_mod_kbd_device_kbd_del(const char *udi);
static void _e_mod_kbd_device_kbd_eval(void);
#ifdef HAVE_EEZE
# include <Eeze.h>
static void _e_mod_kbd_device_udev_event(const char *device, Eeze_Udev_Event event, void *data __UNUSED__, Eeze_Udev_Watch *watch __UNUSED__);
#else
# include <E_Hal.h>
static void _e_mod_kbd_device_cb_input_kbd(void *data __UNUSED__, void *reply, DBusError *err);
static void _e_mod_kbd_device_cb_input_kbd_is(void *data, void *reply, DBusError *err);
static void _e_mod_kbd_device_dbus_add(void *data __UNUSED__, DBusMessage *msg);
static void _e_mod_kbd_device_dbus_del(void *data __UNUSED__, DBusMessage *msg);
static void _e_mod_kbd_device_dbus_chg(void *data __UNUSED__, DBusMessage *msg);
#endif

/* local variables */
static int have_real_kbd = 0;
#ifdef HAVE_EEZE
static Eeze_Udev_Watch *watch;
#else
static E_DBus_Connection *_dbus_conn = NULL;
static E_DBus_Signal_Handler *_dev_add = NULL;
static E_DBus_Signal_Handler *_dev_del = NULL;
static E_DBus_Signal_Handler *_dev_chg = NULL;
#endif
static Eina_List *_device_kbds = NULL, *_ignore_kbds = NULL;

void 
e_mod_kbd_device_init(void) 
{
   /* load the 'ignored' keyboard file */
   _e_mod_kbd_device_ignore_load();
#ifdef HAVE_EEZE
   eeze_init();
   watch = eeze_udev_watch_add(EEZE_UDEV_TYPE_KEYBOARD, EEZE_UDEV_EVENT_NONE,
                            _e_mod_kbd_device_udev_event, NULL);
#else
   e_dbus_init();
   e_hal_init();
   /* try to attach to the system dbus */
   if (!(_dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM))) return;

   /* ask HAL for any input keyboards */
   e_hal_manager_find_device_by_capability(_dbus_conn, "input.keyboard", 
                                           _e_mod_kbd_device_cb_input_kbd, NULL);

   /* setup dbus signal handlers for when a device gets added/removed/changed */
   _dev_add = 
     e_dbus_signal_handler_add(_dbus_conn, E_HAL_SENDER, 
                               E_HAL_MANAGER_PATH, 
                               E_HAL_MANAGER_INTERFACE, 
                               "DeviceAdded", _e_mod_kbd_device_dbus_add, NULL);
   _dev_del = 
     e_dbus_signal_handler_add(_dbus_conn, E_HAL_SENDER, 
                               E_HAL_MANAGER_PATH, 
                               E_HAL_MANAGER_INTERFACE, 
                               "DeviceRemoved", _e_mod_kbd_device_dbus_del, NULL);
   _dev_chg = 
     e_dbus_signal_handler_add(_dbus_conn, E_HAL_SENDER, 
                               E_HAL_MANAGER_PATH, 
                               E_HAL_MANAGER_INTERFACE, 
                               "NewCapability", _e_mod_kbd_device_dbus_chg, NULL);
#endif
}

void 
e_mod_kbd_device_shutdown(void) 
{
   char *str;

#ifdef HAVE_EEZE
   if (watch) eeze_udev_watch_del(watch);
   eeze_shutdown();
#else
   /* remove the dbus signal handlers if we can */
   if (_dev_add) e_dbus_signal_handler_del(_dbus_conn, _dev_add);
   if (_dev_del) e_dbus_signal_handler_del(_dbus_conn, _dev_del);
   if (_dev_chg) e_dbus_signal_handler_del(_dbus_conn, _dev_chg);
   e_hal_shutdown();
   e_dbus_shutdown();
#endif
   /* free the list of ignored keyboards */
   EINA_LIST_FREE(_ignore_kbds, str)
     eina_stringshare_del(str);

   /* free the list of keyboards */
   EINA_LIST_FREE(_device_kbds, str)
     eina_stringshare_del(str);
}

/* local functions */
static void 
_e_mod_kbd_device_ignore_load(void) 
{
   char buff[PATH_MAX];

   /* load the 'ignore' file from the user's home dir */
   e_user_dir_concat_static(buff, "keyboards/ignore_built_in_keyboards");
   _e_mod_kbd_device_ignore_load_file(buff);

   /* load the 'ignore' file from the system/module dir */
   snprintf(buff, sizeof(buff), 
            "%s/ignore_built_in_keyboards", _e_illume_mod_dir);
   _e_mod_kbd_device_ignore_load_file(buff);
}

static void 
_e_mod_kbd_device_ignore_load_file(const char *file) 
{
   char buff[PATH_MAX];
   FILE *f;

   /* can this file be opened */
   if (!(f = fopen(file, "r"))) return;

   /* parse out the info in the ignore file */
   while (fgets(buff, sizeof(buff), f))
     {
        char *p;
        int len;

        if (buff[0] == '#') continue;
        len = strlen(buff);
        if (len > 0)
          {
             if (buff[len - 1] == '\n') buff[len - 1] = 0;
          }
        p = buff;
        while (isspace(*p)) p++;

        /* append this kbd to the ignore list */
        if (*p) 
          {
             _ignore_kbds = 
               eina_list_append(_ignore_kbds, eina_stringshare_add(p));
          }
     }
   fclose(f);
}

#ifdef HAVE_EEZE
static void 
_e_mod_kbd_device_udev_event(const char *device, Eeze_Udev_Event event, void *data __UNUSED__, Eeze_Udev_Watch *watch __UNUSED__)
{
   if ((!device) || (!event)) return;

   if (((event & EEZE_UDEV_EVENT_ADD) == EEZE_UDEV_EVENT_ADD) ||
     ((event & EEZE_UDEV_EVENT_ONLINE) == EEZE_UDEV_EVENT_ONLINE))
     _e_mod_kbd_device_kbd_add(device);
   else if (((event & EEZE_UDEV_EVENT_REMOVE) == EEZE_UDEV_EVENT_REMOVE) ||
     ((event & EEZE_UDEV_EVENT_OFFLINE) == EEZE_UDEV_EVENT_OFFLINE))
     _e_mod_kbd_device_kbd_del(device);

   _e_mod_kbd_device_kbd_eval();
}
#else
static void 
_e_mod_kbd_device_cb_input_kbd(void *data __UNUSED__, void *reply, DBusError *err) 
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply;
   Eina_List *l;
   char *dev;

   if ((!ret) || (!ret->strings)) return;

   /* if dbus errored then cleanup and get out */
   if (dbus_error_is_set(err)) 
     {
        dbus_error_free(err);
        return;
     }

   /* for each returned keyboard, add it and evaluate it */
   EINA_LIST_FOREACH(ret->strings, l, dev) 
     {
        _e_mod_kbd_device_kbd_add(dev);
        _e_mod_kbd_device_kbd_eval();
     }
}

static void 
_e_mod_kbd_device_cb_input_kbd_is(void *data, void *reply, DBusError *err) 
{
   E_Hal_Device_Query_Capability_Return *ret = reply;
   char *udi = data;

   /* if dbus errored then cleanup and get out */
   if (dbus_error_is_set(err)) 
     {
        dbus_error_free(err);
        return;
     }

   /* if it's an input keyboard, than add it and eval */
   if ((ret) && (ret->boolean)) 
     {
        if (udi) 
          {
             _e_mod_kbd_device_kbd_add(udi);
             _e_mod_kbd_device_kbd_eval();
          }
     }
}

static void 
_e_mod_kbd_device_dbus_add(void *data __UNUSED__, DBusMessage *msg) 
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   e_hal_device_query_capability(_dbus_conn, udi, "input.keyboard", 
                                 _e_mod_kbd_device_cb_input_kbd_is, udi);
}

static void 
_e_mod_kbd_device_dbus_del(void *data __UNUSED__, DBusMessage *msg) 
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (udi) 
     {
        _e_mod_kbd_device_kbd_del(udi);
        _e_mod_kbd_device_kbd_eval();
     }
}

static void 
_e_mod_kbd_device_dbus_chg(void *data __UNUSED__, DBusMessage *msg) 
{
   DBusError err;
   char *udi, *cap;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, 
                         DBUS_TYPE_STRING, &cap, DBUS_TYPE_INVALID);
   if (cap) 
     {
        if (!strcmp(cap, "input.keyboard")) 
          {
             if (udi) 
               {
                  _e_mod_kbd_device_kbd_add(udi);
                  _e_mod_kbd_device_kbd_eval();
               }
          }
     }
}
#endif

static void 
_e_mod_kbd_device_kbd_add(const char *udi) 
{
   const char *str;
   Eina_List *l;

   if (!udi) return;
   EINA_LIST_FOREACH(_device_kbds, l, str)
     if (!strcmp(str, udi)) return;
   _device_kbds = eina_list_append(_device_kbds, eina_stringshare_add(udi));
}

static void 
_e_mod_kbd_device_kbd_del(const char *udi) 
{
   const char *str;
   Eina_List *l;

   if (!udi) return;
   EINA_LIST_FOREACH(_device_kbds, l, str)
     if (!strcmp(str, udi)) 
       {
          eina_stringshare_del(str);
          _device_kbds = eina_list_remove_list(_device_kbds, l);
          break;
       }
}

static void 
_e_mod_kbd_device_kbd_eval(void) 
{
   Eina_List *l, *ll;
   const char *g, *gg;
   int have_real = 0;

   have_real = eina_list_count(_device_kbds);
   EINA_LIST_FOREACH(_device_kbds, l, g)
     EINA_LIST_FOREACH(_ignore_kbds, ll, gg)
       if (e_util_glob_match(g, gg)) 
         {
            have_real--;
            break;
         }

   if (have_real != have_real_kbd) 
     {
        have_real_kbd = have_real;
#if 0
//        if (have_real_kbd) e_mod_kbd_disable();
        else
#endif
//          e_mod_kbd_enable();
     }
}

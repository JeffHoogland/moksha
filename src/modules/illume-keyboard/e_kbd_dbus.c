#include "e.h"
#include "e_mod_main.h"
#include "e_kbd.h"

/* local function prototypes */
static void _e_kbd_dbus_ignore_keyboards_load(void);
static void _e_kbd_dbus_ignore_keyboards_file_load(const char *file);
static void _e_kbd_dbus_cb_dev_input_keyboard(void *data, void *reply, DBusError *error);
static void _e_kbd_dbus_keyboard_add(const char *udi);
static void _e_kbd_dbus_keyboard_eval(void);
static void _e_kbd_dbus_cb_dev_add(void *data, DBusMessage *msg);
static void _e_kbd_dbus_cb_dev_del(void *data, DBusMessage *msg);
static void _e_kbd_dbus_cb_cap_add(void *data, DBusMessage *msg);
static void _e_kbd_dbus_cb_input_keyboard_is(void *user_data, void *method_return, DBusError *error);
static void _e_kbd_dbus_keyboard_del(const char *udi);

/* local variables */
static int have_real_keyboard = 0;
static E_DBus_Connection *_e_kbd_dbus_conn = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_add = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_del = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_chg = NULL;
static Eina_List *_e_kbd_dbus_keyboards = NULL;
static Eina_List *_e_kbd_dbus_real_ignore = NULL;

/* public functions */
EAPI void 
e_kbd_dbus_real_kbd_init(void) 
{
   have_real_keyboard = 0;
   _e_kbd_dbus_ignore_keyboards_load();
   _e_kbd_dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (_e_kbd_dbus_conn) 
     {
        e_hal_manager_find_device_by_capability(_e_kbd_dbus_conn, 
                                                "input.keyboard", 
                                                _e_kbd_dbus_cb_dev_input_keyboard, 
                                                NULL);
        _e_kbd_dbus_handler_dev_add = 
          e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal", 
                                    "/org/freedesktop/Hal/Manager", 
                                    "org.freedesktop.Hal.Manager", 
                                    "DeviceAdded", _e_kbd_dbus_cb_dev_add, NULL);
        _e_kbd_dbus_handler_dev_del = 
          e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal", 
                                    "/org/freedesktop/Hal/Manager", 
                                    "org.freedesktop.Hal.Manager", 
                                    "DeviceRemoved", _e_kbd_dbus_cb_dev_del, NULL);
        _e_kbd_dbus_handler_dev_chg = 
          e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal", 
                                    "/org/freedesktop/Hal/Manager", 
                                    "org.freedesktop.Hal.Manager", 
                                    "NewCapability", _e_kbd_dbus_cb_cap_add, NULL);
     }
}

EAPI void 
e_kbd_dbus_real_kbd_shutdown(void) 
{
   char *str;

   if (_e_kbd_dbus_conn) 
     {
        e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_add);
        e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_del);
        e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_chg);
     }
   EINA_LIST_FREE(_e_kbd_dbus_real_ignore, str)
     eina_stringshare_del(str);
   EINA_LIST_FREE(_e_kbd_dbus_keyboards, str)
     eina_stringshare_del(str);
   have_real_keyboard = 0;
}

/* local functions */
static void 
_e_kbd_dbus_ignore_keyboards_load(void) 
{
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "keyboards/ignore_built_in_keyboards");
   _e_kbd_dbus_ignore_keyboards_file_load(buff);
   snprintf(buff, sizeof(buff), 
            "%s/keyboards/ignore_built_in_keyboards", mod_dir);
   _e_kbd_dbus_ignore_keyboards_file_load(buff);
}

static void 
_e_kbd_dbus_ignore_keyboards_file_load(const char *file) 
{
   char buff[PATH_MAX];
   FILE *f;

   if (!(f = fopen(file, "r"))) return;
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
	if (*p)
	  _e_kbd_dbus_real_ignore = 
          eina_list_append(_e_kbd_dbus_real_ignore, eina_stringshare_add(p));
     }
   fclose(f);
}

static void 
_e_kbd_dbus_cb_dev_input_keyboard(void *data, void *reply, DBusError *error) 
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;
   Eina_List *l;
   char *device;

   if (!(ret = reply)) return;
   if (!ret->strings) return;
   if (dbus_error_is_set(error)) 
     {
        dbus_error_free(error);
        return;
     }
   EINA_LIST_FOREACH(ret->strings, l, device) 
     {
        _e_kbd_dbus_keyboard_add(device);
        _e_kbd_dbus_keyboard_eval();
     }
}

static void 
_e_kbd_dbus_keyboard_add(const char *udi) 
{
   const char *str;
   Eina_List *l;

   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, str)
     if (!strcmp(str, udi)) return;
   _e_kbd_dbus_keyboards = 
     eina_list_append(_e_kbd_dbus_keyboards, eina_stringshare_add(udi));
}

static void 
_e_kbd_dbus_keyboard_eval(void) 
{
   Eina_List *l, *ll;
   const char *g, *gg;
   int have_real = 0;

   have_real = eina_list_count(_e_kbd_dbus_keyboards);
   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, g)
     EINA_LIST_FOREACH(_e_kbd_dbus_real_ignore, ll, gg)
       if (e_util_glob_match(g, gg))
         {
            have_real--;
            break;
         }

   if (have_real != have_real_keyboard)
     {
	have_real_keyboard = have_real;
	if (have_real_keyboard)
	  e_kbd_all_disable();
	else
	  e_kbd_all_enable();
     }
}

static void 
_e_kbd_dbus_cb_dev_add(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi;
   int ret;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   udi = strdup(udi);
   ret = 
     e_hal_device_query_capability(_e_kbd_dbus_conn, udi, "input.keyboard", 
                                   _e_kbd_dbus_cb_input_keyboard_is, 
                                   udi);
}

static void 
_e_kbd_dbus_cb_dev_del(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   _e_kbd_dbus_keyboard_del(udi);
   _e_kbd_dbus_keyboard_eval();
}

static void 
_e_kbd_dbus_cb_cap_add(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi, *cap;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, 
                         DBUS_TYPE_STRING, &cap, DBUS_TYPE_INVALID);
   if (!strcmp(cap, "input.keyboard")) 
     {
        _e_kbd_dbus_keyboard_add(udi);
        _e_kbd_dbus_keyboard_eval();
     }
}

static void 
_e_kbd_dbus_cb_input_keyboard_is(void *user_data, void *method_return, DBusError *error) 
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret = method_return;

   if (dbus_error_is_set(error)) 
     {
        dbus_error_free(error);
        free(udi);
        return;
     }
   if ((ret) && (ret->boolean)) 
     {
        _e_kbd_dbus_keyboard_add(udi);
        _e_kbd_dbus_keyboard_eval();
     }
}

static void 
_e_kbd_dbus_keyboard_del(const char *udi) 
{
   Eina_List *l;
   char *str;

   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, str)
     if (!strcmp(str, udi)) 
       {
          eina_stringshare_del(str);
          _e_kbd_dbus_keyboards = 
            eina_list_remove_list(_e_kbd_dbus_keyboards, l);
          return;
       }
}

#include "E_Illume.h"
#include "e_kbd.h"
#include "e_kbd_dbus.h"

/* local variables */
static int have_real_kbd = 0;
static E_DBus_Connection *dbus_conn = NULL;
static E_DBus_Signal_Handler *dbus_dev_add = NULL;
static E_DBus_Signal_Handler *dbus_dev_del = NULL;
static E_DBus_Signal_Handler *dbus_dev_chg = NULL;
static Eina_List *dbus_kbds = NULL, *ignore_kbds = NULL;

/* local function prototypes */
static void _e_kbd_dbus_cb_input_kbd(void *data, void *reply, DBusError *err);
static void _e_kbd_dbus_cb_input_kbd_is(void *data, void *reply, DBusError *err);
static void _e_kbd_dbus_ignore_kbds_load(void);
static void _e_kbd_dbus_ignore_kbds_file_load(const char *file);
static void _e_kbd_dbus_kbd_add(const char *udi);
static void _e_kbd_dbus_kbd_del(const char *udi);
static void _e_kbd_dbus_kbd_eval(void);
static void _e_kbd_dbus_dev_add(void *data, DBusMessage *msg);
static void _e_kbd_dbus_dev_del(void *data, DBusMessage *msg);
static void _e_kbd_dbus_dev_chg(void *data, DBusMessage *msg);

/* public functions */
void 
e_kbd_dbus_init(void) 
{
   /* load the 'ignore' keyboard files */
   _e_kbd_dbus_ignore_kbds_load();

   /* attempt to connect to the system dbus */
   if (!(dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM))) return;

   /* ask HAL for any input keyboards */
   e_hal_manager_find_device_by_capability(dbus_conn, "input.keyboard", 
                                           _e_kbd_dbus_cb_input_kbd, NULL);

   /* setup dbus signal handlers for when a device gets added/removed/changed */
   dbus_dev_add = 
     e_dbus_signal_handler_add(dbus_conn, "org.freedesktop.Hal", 
                               "/org/freedesktop/Hal/Manager", 
                               "org.freedesktop.Hal.Manager", 
                               "DeviceAdded", _e_kbd_dbus_dev_add, NULL);
   dbus_dev_del = 
     e_dbus_signal_handler_add(dbus_conn, "org.freedesktop.Hal", 
                               "/org/freedesktop/Hal/Manager", 
                               "org.freedesktop.Hal.Manager", 
                               "DeviceRemoved", _e_kbd_dbus_dev_del, NULL);
   dbus_dev_chg = 
     e_dbus_signal_handler_add(dbus_conn, "org.freedesktop.Hal", 
                               "/org/freedesktop/Hal/Manager", 
                               "org.freedesktop.Hal.Manager", 
                               "NewCapability", _e_kbd_dbus_dev_chg, NULL);
}

void 
e_kbd_dbus_shutdown(void) 
{
   char *str;

   /* remove the dbus signal handlers if we can */
   if (dbus_conn) 
     {
        e_dbus_signal_handler_del(dbus_conn, dbus_dev_add);
        e_dbus_signal_handler_del(dbus_conn, dbus_dev_del);
        e_dbus_signal_handler_del(dbus_conn, dbus_dev_chg);
     }

   /* free the list of ignored keyboards */
   EINA_LIST_FREE(ignore_kbds, str)
     eina_stringshare_del(str);

   /* free the list of keyboards */
   EINA_LIST_FREE(dbus_kbds, str)
     eina_stringshare_del(str);
}

/* local functions */
static void 
_e_kbd_dbus_cb_input_kbd(void *data, void *reply, DBusError *err) 
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
        _e_kbd_dbus_kbd_add(dev);
        _e_kbd_dbus_kbd_eval();
     }
}

static void 
_e_kbd_dbus_cb_input_kbd_is(void *data, void *reply, DBusError *err) 
{
   E_Hal_Device_Query_Capability_Return *ret = reply;
   char *udi = data;

   /* if dbus errored then cleanup and get out */
   if (dbus_error_is_set(err)) 
     {
        dbus_error_free(err);
        if (udi) free(udi);
        return;
     }

   /* if it's an input keyboard, than add it and eval */
   if ((ret) && (ret->boolean)) 
     {
        _e_kbd_dbus_kbd_add(udi);
        _e_kbd_dbus_kbd_eval();
     }
}

static void 
_e_kbd_dbus_ignore_kbds_load(void) 
{
   char buff[PATH_MAX];

   /* load the 'ignore' file from the user's home dir */
   e_user_dir_concat_static(buff, "keyboards/ignore_built_in_keyboards");
   _e_kbd_dbus_ignore_kbds_file_load(buff);

   /* load the 'ignore' file from the system/module dir */
   snprintf(buff, sizeof(buff), 
            "%s/keyboards/ignore_built_in_keyboards", il_cfg->mod_dir);
   _e_kbd_dbus_ignore_kbds_file_load(buff);
}

static void 
_e_kbd_dbus_ignore_kbds_file_load(const char *file) 
{
   char buf[PATH_MAX];
   FILE *f;

   /* can this file be opened */
   if (!(f = fopen(file, "r"))) return;

   /* parse out the info in the ignore file */
   while (fgets(buf, sizeof(buf), f))
     {
	char *p;
	int len;

	if (buf[0] == '#') continue;
        len = strlen(buf);
	if (len > 0)
	  {
	     if (buf[len - 1] == '\n') buf[len - 1] = 0;
	  }
	p = buf;
	while (isspace(*p)) p++;

        /* append this kbd to the ignore list */
	if (*p) 
          ignore_kbds = eina_list_append(ignore_kbds, eina_stringshare_add(p));
     }
   fclose(f);
}

static void 
_e_kbd_dbus_kbd_add(const char *udi) 
{
   const char *str;
   Eina_List *l;

   EINA_LIST_FOREACH(dbus_kbds, l, str)
     if (!strcmp(str, udi)) return;
   dbus_kbds = eina_list_append(dbus_kbds, eina_stringshare_add(udi));
}

static void 
_e_kbd_dbus_kbd_del(const char *udi) 
{
   const char *str;
   Eina_List *l;

   EINA_LIST_FOREACH(dbus_kbds, l, str)
     if (!strcmp(str, udi)) 
       {
          eina_stringshare_del(str);
          dbus_kbds = eina_list_remove_list(dbus_kbds, l);
          return;
       }
}

static void 
_e_kbd_dbus_kbd_eval(void) 
{
   Eina_List *l, *ll;
   const char *g, *gg;
   int have_real = 0;

   have_real = eina_list_count(dbus_kbds);
   EINA_LIST_FOREACH(dbus_kbds, l, g)
     EINA_LIST_FOREACH(ignore_kbds, ll, gg)
       if (e_util_glob_match(g, gg)) 
         {
            have_real--;
            break;
         }
   if (have_real != have_real_kbd) 
     {
        have_real_kbd = have_real;
#if 0
        if (have_real_kbd) e_kbd_all_disable();
        else
#endif
          e_kbd_all_enable();
     }
}

static void 
_e_kbd_dbus_dev_add(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi;
   int ret;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   ret = e_hal_device_query_capability(dbus_conn, udi, "input.keyboard", 
                                       _e_kbd_dbus_cb_input_kbd_is, udi);
}

static void 
_e_kbd_dbus_dev_del(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (udi) 
     {
        _e_kbd_dbus_kbd_del(udi);
        _e_kbd_dbus_kbd_eval();
        free(udi);
     }
}

static void 
_e_kbd_dbus_dev_chg(void *data, DBusMessage *msg) 
{
   DBusError err;
   char *udi, *cap;

   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, 
                         DBUS_TYPE_STRING, &cap, DBUS_TYPE_INVALID);
   if (!strcmp(cap, "input.keyboard")) 
     {
        _e_kbd_dbus_kbd_add(udi);
        _e_kbd_dbus_kbd_eval();
     }
   if (cap) free(cap);
   if (udi) free(udi);
}

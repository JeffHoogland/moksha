#include "e_mod_main.h"

static int _log_dom = -1;
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

static DBusMessage *
cb_virtual_desktops(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusMessage *reply = dbus_message_new_method_return(msg);
   dbus_message_append_args(reply,
			    DBUS_TYPE_INT32, &(e_config->zone_desks_x_count),
			    DBUS_TYPE_INT32, &(e_config->zone_desks_y_count),
			    DBUS_TYPE_INVALID);
   DBG("GetVirtualCount: %d %d",
       e_config->zone_desks_x_count, e_config->zone_desks_y_count);

   return reply;
}

static DBusMessage *
cb_desktop_show(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   int x, y;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_INT32, &x,
			      DBUS_TYPE_INT32, &y,
			      DBUS_TYPE_INVALID))
     {
	ERR("could not get Show arguments: %s: %s", err.name, err.message);
	dbus_error_free(&err);
     }
   else
     {
	E_Zone *zone = e_util_zone_current_get(e_manager_current_get());
	DBG("show desktop %d,%d from zone %p.", x, y, zone);
	e_zone_desk_flip_to(zone, x, y);
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
cb_desktop_show_by_name(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   const char *name;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &name,
			      DBUS_TYPE_INVALID))
     {
	ERR("could not get Show arguments: %s: %s", err.name, err.message);
	dbus_error_free(&err);
     }
   else if (name)
     {
	E_Zone *zone = e_util_zone_current_get(e_manager_current_get());
	unsigned int i, count;

	DBG("show desktop %s from zone %p.", name, zone);
	count = zone->desk_x_count * zone->desk_y_count;
	for (i = 0; i < count; i++)
	  {
	     E_Desk *desk = zone->desks[i];
	     if ((desk->name) && (strcmp(desk->name, name) == 0))
	       {
		  DBG("show desktop %s (%d,%d) from zone %p.",
		      name, desk->x, desk->y, zone);
		  e_zone_desk_flip_to(zone, desk->x, desk->y);
		  break;
	       }
	  }
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
cb_desktop_lock(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBG("desklock requested");
   e_desklock_show();

   return dbus_message_new_method_return(msg);
}
static DBusMessage *
cb_desktop_unlock(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBG("deskunlock requested");
   e_desklock_hide();

   return dbus_message_new_method_return(msg);
}

static DBusMessage*
cb_desktop_bgadd(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   int container, zone, desk_x, desk_y;
   const char *path;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_INT32, &container,
			      DBUS_TYPE_INT32, &zone,
			      DBUS_TYPE_INT32, &desk_x,
			      DBUS_TYPE_INT32, &desk_y,
			      DBUS_TYPE_STRING, &path,
			      DBUS_TYPE_INVALID))
     {
	ERR("could not get Add arguments: %s: %s", err.name, err.message);
	dbus_error_free(&err);
     }
   else if (path)
     {
	DBG("add bg container=%d, zone=%d, pos=%d,%d path=%s",
	    container, zone, desk_x, desk_y, path);
	e_bg_add(container, zone, desk_x, desk_y, path);
	e_bg_update();
	e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage*
cb_desktop_bgdel(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   int container, zone, desk_x, desk_y;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_INT32, &container,
			      DBUS_TYPE_INT32, &zone,
			      DBUS_TYPE_INT32, &desk_x,
			      DBUS_TYPE_INT32, &desk_y,
			      DBUS_TYPE_INVALID))
     {
	ERR("could not get Del arguments: %s: %s", err.name, err.message);
	dbus_error_free(&err);
     }
   else
     {
	DBG("del bg container=%d, zone=%d, pos=%d,%d",
	    container, zone, desk_x, desk_y);

	e_bg_del(container, zone, desk_x, desk_y);
	e_bg_update();
	e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage*
cb_desktop_bglist(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   Eina_List *list;
   E_Config_Desktop_Background *bg;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(iiiis)", &arr);

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, list, bg)
     {
	DBusMessageIter sub;

	if (!bg || !bg->file)
	{
	   continue;
	}

	DBG("Background container=%d zone=%d pos=%d,%d path=%s",
	    bg->container, bg->zone, bg->desk_x, bg->desk_y, bg->file);

	dbus_message_iter_open_container(&arr, DBUS_TYPE_STRUCT, NULL, &sub);
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(bg->container));
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(bg->zone));
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(bg->desk_x));
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(bg->desk_y));
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(bg->file));
	dbus_message_iter_close_container(&arr, &sub);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

void msgbus_desktop_init(Eina_Array *ifaces)
{
   E_DBus_Interface *iface;

   if (_log_dom == -1)
     {
	_log_dom = eina_log_domain_register("msgbus_desktop", EINA_COLOR_BLUE);
	if (_log_dom < 0)
	  EINA_LOG_ERR("could not register msgbus_desktop log domain!");
     }

   iface = e_dbus_interface_new("org.enlightenment.wm.Desktop");
   if (iface)
     {
	e_dbus_interface_method_add(iface, "GetVirtualCount", "", "ii",
				    cb_virtual_desktops);
	e_dbus_interface_method_add(iface, "Show", "ii", "",
				    cb_desktop_show);
	e_dbus_interface_method_add(iface, "ShowByName", "s", "",
				    cb_desktop_show_by_name);
        e_dbus_interface_method_add(iface, "Lock", "", "",
                                    cb_desktop_lock);
	e_dbus_interface_method_add(iface, "Unlock", "", "",
                                    cb_desktop_unlock);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }

   iface = e_dbus_interface_new("org.enlightenment.wm.Desktop.Background");
   if (iface)
     {
	e_dbus_interface_method_add(iface, "Add", "iiiis", "",
				    cb_desktop_bgadd);
	e_dbus_interface_method_add(iface, "Del", "iiii", "",
				    cb_desktop_bgdel);
	e_dbus_interface_method_add(iface, "List", "", "a(iiiis)",
				    cb_desktop_bglist);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }
}

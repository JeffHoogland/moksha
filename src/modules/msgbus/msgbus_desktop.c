/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "msgbus_desktop.h"

static DBusMessage *
cb_virtual_desktops(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage* reply;
   DBusMessageIter iter;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,
				  &(e_config->zone_desks_x_count));
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,
				  &(e_config->zone_desks_y_count));

   return reply;
}

static DBusMessage*
cb_desktop_bgadd(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   int container, zone, desk_x, desk_y;
   char* path;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &container);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &zone);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &desk_x);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &desk_y);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &path);

   e_bg_add(container, zone, desk_x, desk_y, path);
   e_bg_update();
   e_config_save_queue();

   return dbus_message_new_method_return(msg);
}

static DBusMessage*
cb_desktop_bgdel(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessageIter iter;
   int container, zone, desk_x, desk_y;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &container);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &zone);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &desk_x);
   dbus_message_iter_next(&iter);
   dbus_message_iter_get_basic(&iter, &desk_y);

   e_bg_del(container, zone, desk_x, desk_y);
   e_bg_update();
   e_config_save_queue();

   return dbus_message_new_method_return(msg);
}

static DBusMessage*
cb_desktop_bglist(E_DBus_Object *obj, DBusMessage *msg)
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

	if (bg == NULL || bg->file == NULL)
	{
	   continue;
	}

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

   iface = e_dbus_interface_new("org.enlightenment.wm.Desktop");
   if (iface)
     {
	e_dbus_interface_method_add(iface, "GetVirtualCount", "", "ii",
				    cb_virtual_desktops);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }

   iface = e_dbus_interface_new("org.enlightenment.wm.Desktop.Background");
   if (iface)
     {
	e_dbus_interface_method_add(iface, "Add", "iiiis", "",
				    cb_desktop_bgadd);
	e_dbus_interface_method_add(iface, "Del", "iiii", "",
				    cb_desktop_bgadd);
	e_dbus_interface_method_add(iface, "List", "", "a(iiiis)",
				    cb_desktop_bglist);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }
}

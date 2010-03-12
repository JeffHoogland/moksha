/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e_mod_main.h"

static int _log_dom = -1;
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

static DBusMessage *
cb_langs(E_DBus_Object *obj __UNUSED__, DBusMessage *message)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;
   const Eina_List *l;
   const char *str;

   reply = dbus_message_new_method_return(message);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   EINA_LIST_FOREACH(e_intl_language_list(), l, str)
     {
	DBG("language: %s", str);
	dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &str);
     }

   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

void msgbus_lang_init(Eina_Array *ifaces)
{
   E_DBus_Interface* iface;

   if (_log_dom == -1)
     {
	_log_dom = eina_log_domain_register("msgbus_lang", EINA_COLOR_BLUE);
	if (_log_dom < 0)
	  EINA_LOG_ERR("could not register msgbus_lang log domain!");
     }

   iface = e_dbus_interface_new("org.enlightenment.wm.Language");
   if (iface)
     {
	e_dbus_interface_method_add(iface, "List", "", "as", cb_langs);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static E_DBus_Interface *iface = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/

DBusMessage *
cb_langs(E_DBus_Object *obj, DBusMessage *message)
{
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;
   Evas_List * languages;
   Evas_List * l;

   memset(&arr, 0, sizeof(DBusMessageIter));

   reply = dbus_message_new_method_return(message);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   languages = e_intl_language_list();
   for (l = languages; l; l = l->next) {
	const char *str;

	str = l->data;
	dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &str);
   }

   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "IPC - Language"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   iface = e_dbus_interface_new("org.enlightenment.wm.Language");
   if (!iface) return NULL;
   
   e_dbus_interface_method_add(iface, "List", "", "as", cb_langs);

   e_msgbus_interface_attach(iface);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_msgbus_interface_detach(iface);
   e_dbus_interface_unref(iface);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

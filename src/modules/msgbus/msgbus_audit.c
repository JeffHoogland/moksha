#include "e_mod_main.h"

static int _log_dom = -1;
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

static DBusMessage*
cb_audit_timer_dump(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBusMessage *reply;
   char *tmp;

   tmp = ecore_timer_dump();
   if (!tmp) tmp = strdup("Not enable, recompile Ecore with ecore_timer_dump.");

   reply = dbus_message_new_method_return(msg);
   dbus_message_append_args(reply, DBUS_TYPE_STRING, &tmp, DBUS_TYPE_INVALID);

   return reply;
}

void msgbus_audit_init(Eina_Array *ifaces)
{
   E_DBus_Interface *iface;

   if (_log_dom == -1)
     {
	_log_dom = eina_log_domain_register("msgbus_audit", EINA_COLOR_BLUE);
	if (_log_dom < 0)
	  EINA_LOG_ERR("could not register msgbus_audit log domain!");
     }

   iface = e_dbus_interface_new("org.enlightenment.wm.Audit");
   if (iface)
     {
        e_dbus_interface_method_add(iface, "Timers", "", "s",
                                    cb_audit_timer_dump);
	e_msgbus_interface_attach(iface);
	eina_array_push(ifaces, iface);
     }
}


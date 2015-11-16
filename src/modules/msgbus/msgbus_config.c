#include "e_mod_main.h"

static int _log_dom = -1;
#undef DBG
#undef WARN
#undef INF
#undef ERR
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)


static DBusMessage *
cb_config_load(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBG("config load requested");
   e_config_load();

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
cb_config_save(E_DBus_Object *obj __UNUSED__, DBusMessage *msg)
{
   DBG("config save requested");
   e_config_save();

   return dbus_message_new_method_return(msg);
}


void msgbus_config_init(Eina_Array *ifaces)
{
   E_DBus_Interface *iface;

   if (_log_dom == -1)
   {
      _log_dom = eina_log_domain_register("msgbus_config", EINA_COLOR_BLUE);
      if (_log_dom < 0)
      EINA_LOG_ERR("could not register msgbus_config log domain!");
   }

   iface = e_dbus_interface_new("org.enlightenment.wm.Config");
   if (iface)
   {
      e_dbus_interface_method_add(iface, "Load", "", "",
                                    cb_config_load);
      e_dbus_interface_method_add(iface, "Save", "", "",
                                    cb_config_save);
      e_msgbus_interface_attach(iface);
      eina_array_push(ifaces, iface);
   }
}

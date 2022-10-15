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


static Eldbus_Message *cb_config_load(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg);
static Eldbus_Message *cb_config_save(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg);
static Eldbus_Message *cb_config_save_block(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg);
static Eldbus_Message *cb_config_save_release(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg);

static const Eldbus_Method config_methods[] = {
   { "Load", NULL, NULL, cb_config_load, 0 },
   { "Save", NULL, NULL, cb_config_save, 0 },
   { "SaveBlock", NULL, NULL, cb_config_save_block, 0 },
   { "SaveRelease", NULL, NULL, cb_config_save_release, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Service_Interface_Desc config = {
   "org.enlightenment.wm.Config", config_methods, NULL, NULL, NULL, NULL
};

static Eldbus_Message *
cb_config_load(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   DBG("config load requested");
   e_config_load();

   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
cb_config_save(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   DBG("config save requested");
   e_config_save();

   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
cb_config_save_block(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   DBG("config save_block requested");
   e_config_save_block_set(1);

   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
cb_config_save_release(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   DBG("config save_release requested");
   e_config_save_block_set(0);
   e_bg_update();

   return eldbus_message_method_return_new(msg);
}

void msgbus_config_init(Eina_Array *ifaces)
{
   Eldbus_Service_Interface *iface;

   if (_log_dom == -1)
   {
      _log_dom = eina_log_domain_register("msgbus_config", EINA_COLOR_BLUE);
      if (_log_dom < 0)
         EINA_LOG_ERR("could not register msgbus_config log domain!");
   }

   iface = e_msgbus_interface_attach(&config);
   if (iface) eina_array_push(ifaces, iface);
}

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

static Eldbus_Message *_e_msgbus_module_load_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_unload_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_enable_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_disable_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_list_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static const Eldbus_Method module_methods[] = {
   { "Load", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_load_cb, 0 },
   { "Unload", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_unload_cb, 0 },
   { "Enable", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_enable_cb, 0 },
   { "Disable", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_disable_cb, 0 },
   { "List", NULL, ELDBUS_ARGS({"a(si)", "modules"}), _e_msgbus_module_list_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Service_Interface_Desc module = {
   "org.enlightenment.wm.Module", module_methods, NULL, NULL, NULL, NULL
};

/* Modules Handlers */
static Eldbus_Message *
_e_msgbus_module_load_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                         const Eldbus_Message *msg)
{
   char *mod;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &mod)) return reply;

   if (!e_module_find(mod))
     {
        e_module_new(mod);
        e_config_save_queue();
     }

   return reply;
}

static Eldbus_Message *
_e_msgbus_module_unload_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                           const Eldbus_Message *msg)
{
   char *mod;
   E_Module *m;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &mod)) return reply;

   if ((m = e_module_find(mod)))
     {
        e_module_disable(m);
        e_object_del(E_OBJECT(m));
        e_config_save_queue();
     }

   return reply;
}

static Eldbus_Message *
_e_msgbus_module_enable_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                           const Eldbus_Message *msg)
{
   char *mod;
   E_Module *m;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &mod)) return reply;

   if ((m = e_module_find(mod)))
     {
        e_module_enable(m);
        e_config_save_queue();
     }

   return reply;
}

static Eldbus_Message *
_e_msgbus_module_disable_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                            const Eldbus_Message *msg)
{
   char *mod;
   E_Module *m;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &mod)) return reply;

   if ((m = e_module_find(mod)))
     {
        e_module_disable(m);
        e_config_save_queue();
     }

   return reply;
}

static Eldbus_Message *
_e_msgbus_module_list_cb(const Eldbus_Service_Interface *iface EINA_UNUSED,
                         const Eldbus_Message *msg)
{
   Eina_List *l;
   E_Module *mod;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Eldbus_Message_Iter *main_iter, *array;

   EINA_SAFETY_ON_NULL_RETURN_VAL(reply, NULL);
   main_iter = eldbus_message_iter_get(reply);
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_iter, reply);

   eldbus_message_iter_arguments_append(main_iter, "a(si)", &array);
   EINA_SAFETY_ON_NULL_RETURN_VAL(array, reply);

   EINA_LIST_FOREACH(e_module_list(), l, mod)
     {
        Eldbus_Message_Iter *s;
        const char *name;
        int enabled;

        name = mod->name;
        enabled = mod->enabled;

        eldbus_message_iter_arguments_append(array, "(si)", &s);
        if (!s) continue;
        eldbus_message_iter_arguments_append(s, "si", name, enabled);
        eldbus_message_iter_container_close(array, s);
     }
   eldbus_message_iter_container_close(main_iter, array);

   return reply;
}

void msgbus_module_init(Eina_Array *ifaces)
{
   Eldbus_Service_Interface *iface;

   if (_log_dom == -1)
     {
        _log_dom = eina_log_domain_register("msgbus_module", EINA_COLOR_BLUE);
        if (_log_dom < 0)
          EINA_LOG_ERR("could not register msgbus_module log domain!");
     }

   iface = e_msgbus_interface_attach(&module);
   if (iface) eina_array_push(ifaces, iface);
}

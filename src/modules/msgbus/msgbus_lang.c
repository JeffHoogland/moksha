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

static Eldbus_Message *
cb_langs(const Eldbus_Service_Interface *iface EINA_UNUSED,
         const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Eldbus_Message_Iter *main_iter, *array;
   const Eina_List *l;
   const char *str;

   if (!reply) return NULL;

   main_iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(main_iter, "as", &array);
   if (!array) return reply;

   EINA_LIST_FOREACH(e_intl_language_list(), l, str)
     {
        DBG("language: %s", str);
        eldbus_message_iter_basic_append(array, 's', str);
     }
   eldbus_message_iter_container_close(main_iter, array);
   return reply;
}

static const Eldbus_Method methods[] = {
   { "List", NULL, ELDBUS_ARGS({"as", "langs"}), cb_langs, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc lang = {
   "org.enlightenment.wm.Language", methods, NULL, NULL, NULL, NULL
};

void msgbus_lang_init(Eina_Array *ifaces)
{
   Eldbus_Service_Interface *iface;

   if (_log_dom == -1)
     {
        _log_dom = eina_log_domain_register("msgbus_lang", EINA_COLOR_BLUE);
        if (_log_dom < 0)
          EINA_LOG_ERR("could not register msgbus_lang log domain!");
     }

   iface = e_msgbus_interface_attach(&lang);
   if (iface) eina_array_push(ifaces, iface);
}

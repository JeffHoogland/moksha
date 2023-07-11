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
cb_virtual_desktops(const Eldbus_Service_Interface *iface EINA_UNUSED,
                    const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   eldbus_message_arguments_append(reply, "ii", e_config->zone_desks_x_count,
                                   e_config->zone_desks_y_count);
   DBG("GetVirtualCount: %d %d",
       e_config->zone_desks_x_count, e_config->zone_desks_y_count);
   return reply;
}

static Eldbus_Message *
cb_desktop_show(const Eldbus_Service_Interface *iface EINA_UNUSED,
                const Eldbus_Message *msg)
{
   int x, y;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   E_Zone *zone;

   if (!eldbus_message_arguments_get(msg, "ii", &x, &y))
     {
        ERR("could not get Show arguments");
        return reply;
     }

   zone = e_util_zone_current_get(e_manager_current_get());
   DBG("show desktop %d,%d from zone %p.", x, y, zone);
   e_zone_desk_flip_to(zone, x, y);
   return reply;
}

static Eldbus_Message *
cb_desktop_show_by_name(const Eldbus_Service_Interface *iface EINA_UNUSED,
                        const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   const char *name;
   E_Zone *zone;
   unsigned int i, count;

   if (!eldbus_message_arguments_get(msg, "s", &name))
     {
        ERR("could not get Show arguments");
        return reply;
     }

   zone = e_util_zone_current_get(e_manager_current_get());
   DBG("show desktop %s from zone %p.", name, zone);
   count = zone->desk_x_count * zone->desk_y_count;
   for (i = 0; i < count; i++)
     {
        E_Desk *desk = zone->desks[i];
        if ((desk->name) && (strcmp(desk->name, name) == 0))
          {
             DBG("show desktop %s (%d,%d) from zone %p.", name, desk->x,
                 desk->y, zone);
             e_zone_desk_flip_to(zone, desk->x, desk->y);
             break;
          }
     }
   return reply;
}

static Eldbus_Message *
cb_desktop_lock(const Eldbus_Service_Interface *iface EINA_UNUSED,
                const Eldbus_Message *msg)
{
   DBG("desklock requested");
   //e_desklock_show_manual(EINA_FALSE);
   e_desklock_show(EINA_FALSE);
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
cb_desktop_unlock(const Eldbus_Service_Interface *iface EINA_UNUSED,
                  const Eldbus_Message *msg)
{
   DBG("deskunlock requested");
   e_desklock_hide();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
cb_desktop_bgadd(const Eldbus_Service_Interface *iface EINA_UNUSED,
                 const Eldbus_Message *msg)
{
   int container, zone, desk_x, desk_y;
   const char *path;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "iiiis", &container, &zone,
                                     &desk_x, &desk_y, &path))
     {
        ERR("could not get Add arguments");
        return reply;
     }
   DBG("add bg container=%d, zone=%d, pos=%d,%d path=%s",
       container, zone, desk_x, desk_y, path);
   e_bg_add(container, zone, desk_x, desk_y, path);
   e_bg_update();
   e_config_save_queue();
   return reply;
}

static Eldbus_Message *
cb_desktop_bgdel(const Eldbus_Service_Interface *iface EINA_UNUSED,
                 const Eldbus_Message *msg)
{
   int container, zone, desk_x, desk_y;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "iiii", &container, &zone, &desk_x, &desk_y))
     {
        ERR("could not get Del arguments");
        return reply;
     }
   DBG("del bg container=%d, zone=%d, pos=%d,%d",
       container, zone, desk_x, desk_y);

   e_bg_del(container, zone, desk_x, desk_y);
   e_bg_update();
   e_config_save_queue();
   return reply;
}

static Eldbus_Message *
cb_desktop_bglist(const Eldbus_Service_Interface *iface EINA_UNUSED,
                  const Eldbus_Message *msg)
{
   Eina_List *list;
   E_Config_Desktop_Background *bg;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   Eldbus_Message_Iter *main_iter, *array;

   if (!reply) return NULL;

   main_iter = eldbus_message_iter_get(reply);
   if (!main_iter) return reply;

   if (!eldbus_message_iter_arguments_append(main_iter, "a(iiiis)", &array))
     return reply;

   EINA_LIST_FOREACH(e_config->desktop_backgrounds, list, bg)
     {
        Eldbus_Message_Iter *s;

        if (!bg || !bg->file) continue;
        DBG("Background container=%d zone=%d pos=%d,%d path=%s",
	         bg->container, bg->zone, bg->desk_x, bg->desk_y, bg->file);

        eldbus_message_iter_arguments_append(array, "(iiiis)", &s);
        if (!s) continue;
        eldbus_message_iter_arguments_append(s, "iiiis", bg->zone,
           bg->container, bg->desk_x, bg->desk_y, bg->file);
        eldbus_message_iter_container_close(array, s);
     }
   eldbus_message_iter_container_close(main_iter, array);
   return reply;
}

static const Eldbus_Method desktop_methods[] = {
   { "GetVirtualCount", NULL, ELDBUS_ARGS({"i", "desk_x"}, {"i", "desk_y"}), cb_virtual_desktops, 0 },
   { "Show", ELDBUS_ARGS({"i", "desk_x"}, {"i", "desk_y"}), NULL, cb_desktop_show, 0 },
   { "ShowByName", ELDBUS_ARGS({"s", "desk_name"}), NULL, cb_desktop_show_by_name, 0 },
   { "Lock", NULL, NULL, cb_desktop_lock, 0 },
   { "Unlock", NULL, NULL, cb_desktop_unlock, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Method background_methods[] = {
   { "Add", ELDBUS_ARGS({"i", "container"}, {"i", "zone"}, {"i", "desk_x"}, {"i", "desk_y"}, {"s", "path"}), NULL, cb_desktop_bgadd, 0 },
   { "Del", ELDBUS_ARGS({"i", "container"}, {"i", "zone"}, {"i", "desk_x"}, {"i", "desk_y"}), NULL, cb_desktop_bgdel, 0 },
   { "List", NULL, ELDBUS_ARGS({"a(iiiis)", "array_of_bg"}), cb_desktop_bglist, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc desktop = {
   "org.enlightenment.wm.Desktop", desktop_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc bg = {
   "org.enlightenment.wm.Desktop.Background", background_methods, NULL, NULL, NULL, NULL
};

void msgbus_desktop_init(Eina_Array *ifaces)
{
   Eldbus_Service_Interface *iface;

   if (_log_dom == -1)
     {
        _log_dom = eina_log_domain_register("msgbus_desktop", EINA_COLOR_BLUE);
        if (_log_dom < 0)
          EINA_LOG_ERR("could not register msgbus_desktop log domain!");
     }
   iface = e_msgbus_interface_attach(&desktop);
   if (iface) eina_array_push(ifaces, iface);
   iface = e_msgbus_interface_attach(&bg);
   if (iface) eina_array_push(ifaces, iface);
}

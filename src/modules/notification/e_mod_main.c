#include "e_mod_main.h"

/* Gadcon function protos */
static E_Gadcon_Client *_gc_init(E_Gadcon   *gc,
                                 const char *name,
                                 const char *id,
                                 const char *style);
static void         _gc_shutdown(E_Gadcon_Client *gcc);
static char        *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class,
                             Evas                  *evas);
static const char  *_gc_id_new(E_Gadcon_Client_Class *client_class);
static void         _gc_id_del(E_Gadcon_Client_Class *client_class,
                               const char            *id);

/* Callback function protos */
static int  _notification_cb_notify(E_Notification_Daemon *daemon,
                                    E_Notification        *n);
static void _notification_cb_close_notification(E_Notification_Daemon *daemon,
                                                unsigned int           id);

/* Config function protos */
static Config *_notification_cfg_new(void);
static void    _notification_cfg_free(Config *cfg);

/* Global variables */
E_Module *notification_mod = NULL;
Config *notification_cfg = NULL;

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

/* Gadcon Api Functions */
const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, "notification",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, _gc_id_del,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

static E_Gadcon_Client *
_gc_init(E_Gadcon   *gc,
         const char *name,
         const char *id,
         const char *style)
{
   Notification_Box *b;
   E_Gadcon_Client *gcc;
   Config_Item *ci;
   Instance *inst;

   inst = E_NEW(Instance, 1);
   ci = notification_box_config_item_get(id);
   b = notification_box_get(ci->id, gc->evas);

   inst->ci = ci;
   b->inst = inst;
   inst->n_box = b;

   gcc = e_gadcon_client_new(gc, name, id, style, b->o_box);
   gcc->data = inst;
   inst->gcc = gcc;

   evas_object_event_callback_add(b->o_box, EVAS_CALLBACK_MOVE,
                                  notification_box_cb_obj_moveresize, inst);
   evas_object_event_callback_add(b->o_box, EVAS_CALLBACK_RESIZE,
                                  notification_box_cb_obj_moveresize, inst);
   notification_cfg->instances = eina_list_append(notification_cfg->instances, inst);
   _gc_orient(gcc, gc->orient);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   notification_box_visible_set(inst->n_box, EINA_FALSE);
   notification_cfg->instances = eina_list_remove(notification_cfg->instances, inst);
   free(inst);
}

void
_gc_orient(E_Gadcon_Client *gcc,
           E_Gadcon_Orient  orient)
{
   Instance *inst;

   inst = gcc->data;
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        notification_box_orient_set(inst->n_box, 1);
        e_gadcon_client_aspect_set(gcc, MAX(eina_list_count(inst->n_box->icons), 1) * 16, 16);
        break;

      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
        notification_box_orient_set(inst->n_box, 0);
        e_gadcon_client_aspect_set(gcc, 16, MAX(eina_list_count(inst->n_box->icons), 1) * 16);
        break;

      default:
        break;
     }
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return D_("Notification Box");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__,
         Evas                  *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj",
            e_module_dir_get(notification_mod));
   if (!e_theme_edje_object_set(o, "base/theme/modules/notification",
                                "icon"))
     edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci;

   ci = notification_box_config_item_get(NULL);
   return ci->id;
}

static void
_gc_id_del(E_Gadcon_Client_Class *client_class __UNUSED__,
           const char            *id)
{
   Config_Item *ci;

   notification_box_del(id);
   ci = notification_box_config_item_get(id);
   if (!ci) return;
   eina_stringshare_del(ci->id);
   notification_cfg->items = eina_list_remove(notification_cfg->items, ci);
   free(ci);
}

static unsigned int
_notification_notify(E_Notification *n)
{
   const char *appname = e_notification_app_name_get(n);
   unsigned int replaces_id = e_notification_replaces_id_get(n);
   unsigned int new_id;
   int popuped;

   if (replaces_id) new_id = replaces_id;
   else new_id = notification_cfg->next_id++;

   e_notification_id_set(n, new_id);

   popuped = notification_popup_notify(n, replaces_id, appname);
   notification_box_notify(n, replaces_id, new_id);

   if (!popuped)
     {
        e_notification_hint_urgency_set(n, 4);
        notification_popup_notify(n, replaces_id, appname);
     }

   return new_id;
}

static void
_notification_show_common(const char *summary,
                          const char *body,
                          int         replaces_id)
{
   E_Notification *n = e_notification_full_new
       ("enlightenment", replaces_id, "enlightenment", summary, body, -1);

   if (!n)
     return;

   _notification_notify(n);
   e_notification_unref(n);
}

static void
_notification_show_presentation(Eina_Bool enabled)
{
   const char *summary, *body;

   if (enabled)
     {
        summary = D_("Enter Presentation Mode");
        body = D_("Enlightenment is in <b>presentation</b> mode."
                  "<br>During presentation mode, screen saver, lock and "
                  "power saving will be disabled so you are not interrupted.");
     }
   else
     {
        summary = D_("Exited Presentation Mode");
        body = D_("Presentation mode is over."
                  "<br>Now screen saver, lock and "
                  "power saving settings will be restored.");
     }

   _notification_show_common(summary, body, 0);
}

static void
_notification_show_offline(Eina_Bool enabled)
{
   const char *summary, *body;

   if (enabled)
     {
        summary = D_("Enter Offline Mode");
        body = D_("Enlightenment is in <b>offline</b> mode.<br>"
                  "During offline mode, modules that use network will stop "
                  "polling remote services.");
     }
   else
     {
        summary = D_("Exited Offline Mode");
        body = D_("Now in <b>online</b> mode.<br>"
                  "Now modules that use network will "
                  "resume regular tasks.");
     }

   _notification_show_common(summary, body, 0);
}

static Eina_Bool
_notification_cb_config_mode_changed(Config *m_cfg,
                                     int   type __UNUSED__,
                                     void *event __UNUSED__)
{
   if (m_cfg->last_config_mode.presentation != e_config->mode.presentation)
     {
        m_cfg->last_config_mode.presentation = e_config->mode.presentation;
        _notification_show_presentation(e_config->mode.presentation);
     }

   if (m_cfg->last_config_mode.offline != e_config->mode.offline)
     {
        m_cfg->last_config_mode.offline = e_config->mode.offline;
        _notification_show_offline(e_config->mode.offline);
     }

   return EINA_TRUE;
}

static Eina_Bool
_notification_cb_initial_mode_timer(Config *m_cfg)
{
   if (e_config->mode.presentation)
     _notification_show_presentation(1);
   if (e_config->mode.offline)
     _notification_show_offline(1);

   m_cfg->initial_mode_timer = NULL;
   return EINA_FALSE;
}

/* Module Api Functions */
EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Notification"};

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Notification_Daemon *d;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", m->dir);
   /* register config panel entry */
   e_configure_registry_category_add("extensions", 90, D_("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add("extensions/notification", 30, D_("Notification"), NULL,
                                 buf, e_int_config_notification_module);

   conf_item_edd = E_CONFIG_DD_NEW("Notification_Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, show_label, INT);
   E_CONFIG_VAL(D, T, show_popup, INT);
   E_CONFIG_VAL(D, T, focus_window, INT);
   E_CONFIG_VAL(D, T, store_low, INT);
   E_CONFIG_VAL(D, T, store_normal, INT);
   E_CONFIG_VAL(D, T, store_critical, INT);

   conf_edd = E_CONFIG_DD_NEW("Notification_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, show_low, INT);
   E_CONFIG_VAL(D, T, show_normal, INT);
   E_CONFIG_VAL(D, T, show_critical, INT);
   E_CONFIG_VAL(D, T, corner, INT);
   E_CONFIG_VAL(D, T, timeout, FLOAT);
   E_CONFIG_VAL(D, T, force_timeout, INT);
   E_CONFIG_VAL(D, T, ignore_replacement, INT);
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   notification_cfg = e_config_domain_load("module.notification", conf_edd);
   if (notification_cfg &&
       !(e_util_module_config_check(D_("Notification Module"),
                                    notification_cfg->version,
                                    MOD_CFG_FILE_VERSION)))
     {
        _notification_cfg_free(notification_cfg);
     }

   if (!notification_cfg)
     {
        notification_cfg = _notification_cfg_new();
     }

   /* set up the notification daemon */
   e_notification_daemon_init();
   d = e_notification_daemon_add("e_notification_module", "Enlightenment");
   if (!d)
     {
        _notification_cfg_free(notification_cfg);
        notification_cfg = NULL;
        e_util_dialog_show(D_("Error During DBus Init!"),
                           D_("Error during DBus init! Please check if "
                              "dbus is correctly installed and running."));
        return NULL;
     }
   notification_cfg->daemon = d;
   e_notification_daemon_data_set(d, notification_cfg);
   e_notification_daemon_callback_notify_set(d, _notification_cb_notify);
   e_notification_daemon_callback_close_notification_set(d, _notification_cb_close_notification);

   notification_cfg->last_config_mode.presentation = e_config->mode.presentation;
   notification_cfg->last_config_mode.offline = e_config->mode.offline;
   notification_cfg->handlers = eina_list_append
       (notification_cfg->handlers, ecore_event_handler_add
         (E_EVENT_CONFIG_MODE_CHANGED, (Ecore_Event_Handler_Cb)_notification_cb_config_mode_changed,
         notification_cfg));
   notification_cfg->initial_mode_timer = ecore_timer_add
       (0.1, (Ecore_Task_Cb)_notification_cb_initial_mode_timer, notification_cfg);

   /* set up the borders events callbacks */
   notification_cfg->handlers = eina_list_append
       (notification_cfg->handlers, ecore_event_handler_add
         (E_EVENT_BORDER_REMOVE, (Ecore_Event_Handler_Cb)notification_box_cb_border_remove, NULL));

   notification_mod = m;
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   Ecore_Event_Handler *h;
   Config_Item *ci;

   e_gadcon_provider_unregister(&_gc_class);

   if (notification_cfg->initial_mode_timer)
     ecore_timer_del(notification_cfg->initial_mode_timer);

   EINA_LIST_FREE(notification_cfg->handlers, h)
     ecore_event_handler_del(h);

   if (notification_cfg->cfd) e_object_del(E_OBJECT(notification_cfg->cfd));
   e_configure_registry_item_del("extensions/notification");
   e_configure_registry_category_del("extensions");

   if (notification_cfg->menu)
     {
        e_menu_post_deactivate_callback_set(notification_cfg->menu, NULL, NULL);
        e_object_del(E_OBJECT(notification_cfg->menu));
        notification_cfg->menu = NULL;
     }

   EINA_LIST_FREE(notification_cfg->items, ci)
     {
        eina_stringshare_del(ci->id);
        free(ci);
     }

   notification_box_shutdown();
   notification_popup_shutdown();

   e_notification_daemon_free(notification_cfg->daemon);
   e_notification_daemon_shutdown();
   _notification_cfg_free(notification_cfg);
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   notification_mod = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   int ret;
   ret = e_config_domain_save("module.notification", conf_edd, notification_cfg);
   return ret;
}

/* Callbacks */
static int
_notification_cb_notify(E_Notification_Daemon *daemon __UNUSED__,
                        E_Notification        *n)
{
   return _notification_notify(n);
}

static void
_notification_cb_close_notification(E_Notification_Daemon *daemon __UNUSED__,
                                    unsigned int           id)
{
   notification_popup_close(id);
}

static Config *
_notification_cfg_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);
   cfg->cfd = NULL;
   cfg->version = MOD_CFG_FILE_VERSION;
   cfg->show_low = 0;
   cfg->show_normal = 1;
   cfg->show_critical = 1;
   cfg->timeout = 5.0;
   cfg->force_timeout = 0;
   cfg->ignore_replacement = 0;
   cfg->corner = CORNER_TR;

   return cfg;
}

static void
_notification_cfg_free(Config *cfg)
{
   E_FREE(cfg);
}


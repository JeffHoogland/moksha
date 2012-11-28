#include "e_mod_main.h"

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
        summary = _("Enter Presentation Mode");
        body = _("Enlightenment is in <b>presentation</b> mode."
                 "<br>During presentation mode, screen saver, lock and "
                 "power saving will be disabled so you are not interrupted.");
     }
   else
     {
        summary = _("Exited Presentation Mode");
        body = _("Presentation mode is over."
                 "<br>Now screen saver, lock and "
                 "power saving settings will be restored.");
     }

   _notification_show_common(summary, body, -1);
}

static void
_notification_show_offline(Eina_Bool enabled)
{
   const char *summary, *body;

   if (enabled)
     {
        summary = _("Enter Offline Mode");
        body = _("Enlightenment is in <b>offline</b> mode.<br>"
                 "During offline mode, modules that use network will stop "
                 "polling remote services.");
     }
   else
     {
        summary = _("Exited Offline Mode");
        body = _("Now in <b>online</b> mode.<br>"
                 "Now modules that use network will "
                 "resume regular tasks.");
     }

   _notification_show_common(summary, body, -1);
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

   snprintf(buf, sizeof(buf), "%s/e-module-notification.edj", m->dir);
   /* register config panel entry */
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add("extensions/notification", 30, 
                                 _("Notification"), NULL,
                                 buf, e_int_config_notification_module);


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
   E_CONFIG_VAL(D, T, dual_screen, INT);

   notification_cfg = e_config_domain_load("module.notification", conf_edd);
   if (notification_cfg &&
       !(e_util_module_config_check(_("Notification Module"),
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
        e_util_dialog_show(_("Error During DBus Init!"),
                           _("Error during DBus init! Please check if "
                              "dbus is correctly installed and running."));
        return NULL;
     }
   notification_cfg->daemon = d;
   e_notification_daemon_data_set(d, notification_cfg);
   e_notification_daemon_callback_notify_set(d, _notification_cb_notify);
   e_notification_daemon_callback_close_notification_set(d, _notification_cb_close_notification);

   notification_cfg->last_config_mode.presentation = e_config->mode.presentation;
   notification_cfg->last_config_mode.offline = e_config->mode.offline;
   notification_cfg->handler = ecore_event_handler_add
         (E_EVENT_CONFIG_MODE_CHANGED, (Ecore_Event_Handler_Cb)_notification_cb_config_mode_changed,
         notification_cfg);
   notification_cfg->initial_mode_timer = ecore_timer_add
       (0.1, (Ecore_Task_Cb)_notification_cb_initial_mode_timer, notification_cfg);

   notification_mod = m;
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   if (notification_cfg->initial_mode_timer)
     ecore_timer_del(notification_cfg->initial_mode_timer);

   if (notification_cfg->handler)
     ecore_event_handler_del(notification_cfg->handler);

   if (notification_cfg->cfd) e_object_del(E_OBJECT(notification_cfg->cfd));
   e_configure_registry_item_del("extensions/notification");
   e_configure_registry_category_del("extensions");

   notification_popup_shutdown();

   e_notification_daemon_free(notification_cfg->daemon);
   e_notification_daemon_shutdown();
   _notification_cfg_free(notification_cfg);
   E_CONFIG_DD_FREE(conf_edd);
   notification_mod = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return e_config_domain_save("module.notification", conf_edd, notification_cfg);
}

/* Callbacks */
static int
_notification_cb_notify(E_Notification_Daemon *d __UNUSED__,
                        E_Notification        *n)
{
   return _notification_notify(n);
}

static void
_notification_cb_close_notification(E_Notification_Daemon *d __UNUSED__,
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
   cfg->dual_screen = 0;
   cfg->corner = CORNER_TR;

   return cfg;
}

static void
_notification_cfg_free(Config *cfg)
{
   free(cfg);
}


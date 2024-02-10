#include "e.h"

///////////////////////////////////////////////////////////////////////////
#define E_BUS   "org.enlightenment.wm.service"
#define E_IFACE "org.enlightenment.wm.Core"
#define E_PATH  "/org/enlightenment/wm/RemoteObject"

static void           _e_msgbus_core_request_name_cb(void *data, const Eldbus_Message *msg,
                                                Eldbus_Pending *pending);

static Eldbus_Message *_e_msgbus_core_version_cb(const Eldbus_Service_Interface *iface,
                                                const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_core_restart_cb(const Eldbus_Service_Interface *iface,
                                                const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_core_shutdown_cb(const Eldbus_Service_Interface *iface,
                                                 const Eldbus_Message *msg);

static Eldbus_Message *_e_msgbus_module_load_cb(const Eldbus_Service_Interface *iface,
                                               const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_unload_cb(const Eldbus_Service_Interface *iface,
                                                 const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_enable_cb(const Eldbus_Service_Interface *iface,
                                                 const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_disable_cb(const Eldbus_Service_Interface *iface,
                                                  const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_module_list_cb(const Eldbus_Service_Interface *iface,
                                               const Eldbus_Message *msg);

static Eldbus_Message *_e_msgbus_profile_set_cb(const Eldbus_Service_Interface *iface,
                                               const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_profile_get_cb(const Eldbus_Service_Interface *iface,
                                               const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_profile_list_cb(const Eldbus_Service_Interface *iface,
                                                const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_profile_add_cb(const Eldbus_Service_Interface *iface,
                                               const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_profile_delete_cb(const Eldbus_Service_Interface *iface,
                                                  const Eldbus_Message *msg);

#define E_MSGBUS_WIN_ACTION_CB_PROTO(NAME)                                                   \
  static Eldbus_Message * _e_msgbus_window_##NAME##_cb(const Eldbus_Service_Interface * iface, \
                                                      const Eldbus_Message * msg)

E_MSGBUS_WIN_ACTION_CB_PROTO(list);
E_MSGBUS_WIN_ACTION_CB_PROTO(close);
E_MSGBUS_WIN_ACTION_CB_PROTO(kill);
E_MSGBUS_WIN_ACTION_CB_PROTO(focus);
E_MSGBUS_WIN_ACTION_CB_PROTO(iconify);
E_MSGBUS_WIN_ACTION_CB_PROTO(uniconify);
E_MSGBUS_WIN_ACTION_CB_PROTO(maximize);
E_MSGBUS_WIN_ACTION_CB_PROTO(unmaximize);

/* local subsystem globals */
EAPI E_Msgbus_Data *e_msgbus_data = NULL;

static const Eldbus_Method _e_core_methods[] = {
   { "Version", NULL, ELDBUS_ARGS({"s", "version"}), _e_msgbus_core_version_cb, 0 },
   { "Restart", NULL, NULL, _e_msgbus_core_restart_cb, 0 },
   { "Shutdown", NULL, NULL, _e_msgbus_core_shutdown_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method module_methods[] = {
   { "Load", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_load_cb, 0 },
   { "Unload", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_unload_cb, 0 },
   { "Enable", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_enable_cb, 0 },
   { "Disable", ELDBUS_ARGS({"s", "module"}), NULL, _e_msgbus_module_disable_cb, 0 },
   { "List", NULL, ELDBUS_ARGS({"a(si)", "modules"}),
     _e_msgbus_module_list_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method profile_methods[] = {
   { "Set", ELDBUS_ARGS({"s", "profile"}), NULL, _e_msgbus_profile_set_cb, 0 },
   { "Get", NULL, ELDBUS_ARGS({"s", "profile"}), _e_msgbus_profile_get_cb, 0 },
   { "List", NULL, ELDBUS_ARGS({"as", "array_profiles"}),
     _e_msgbus_profile_list_cb, 0 },
   { "Add", ELDBUS_ARGS({"s", "profile"}), NULL, _e_msgbus_profile_add_cb, 0 },
   { "Delete", ELDBUS_ARGS({"s", "profile"}), NULL, _e_msgbus_profile_delete_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Method window_methods[] = {
   { "List", NULL, ELDBUS_ARGS({"a(si)", "array_of_window"}),
     _e_msgbus_window_list_cb, 0 },
   { "Close", ELDBUS_ARGS({"i", "window_id"}), NULL, _e_msgbus_window_close_cb, 0 },
   { "Kill", ELDBUS_ARGS({"i", "window_id"}), NULL, _e_msgbus_window_kill_cb, 0 },
   { "Focus", ELDBUS_ARGS({"i", "window_id"}), NULL, _e_msgbus_window_focus_cb, 0 },
   { "Iconify", ELDBUS_ARGS({"i", "window_id"}), NULL,
     _e_msgbus_window_iconify_cb, 0 },
   { "Uniconify", ELDBUS_ARGS({"i", "window_id"}), NULL,
     _e_msgbus_window_uniconify_cb, 0 },
   { "Maximize", ELDBUS_ARGS({"i", "window_id"}), NULL,
     _e_msgbus_window_maximize_cb, 0 },
   { "Unmaximize", ELDBUS_ARGS({"i", "window_id"}), NULL,
     _e_msgbus_window_unmaximize_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};


static const Eldbus_Service_Interface_Desc _e_core_desc = {
   E_IFACE, _e_core_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc module_desc = {
   "org.enlightenment.wm.Module", module_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc profile_desc = {
   "org.enlightenment.wm.Profile", profile_methods, NULL, NULL, NULL, NULL
};

static const Eldbus_Service_Interface_Desc window_desc = {
   "org.enlightenment.wm.Window", window_methods, NULL, NULL, NULL, NULL
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
#define SCREENSAVER_BUS   "org.freedesktop.ScreenSaver"
#define SCREENSAVER_IFACE "org.freedesktop.ScreenSaver"
#define SCREENSAVER_PATH  "/org/freedesktop/ScreenSaver"
#define SCREENSAVER_PATH2  "/ScreenSaver"
static void            _e_msgbus_screensaver_request_name_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending);
static void            _e_msgbus_screensaver_inhibit_free(E_Msgbus_Data_Screensaver_Inhibit *inhibit);
static void            _e_msgbus_screensaver_owner_change_cb(void *data __UNUSED__, const char *bus, const char *old_id, const char *new_id);
static Eldbus_Message *_e_msgbus_screensaver_inhibit_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_screensaver_uninhibit_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_msgbus_screensaver_getactive_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

static const Eldbus_Method _screensaver_core_methods[] =
{
   { "Inhibit", ELDBUS_ARGS({"s", "application_name"}, {"s", "reason_for_inhibit"}), ELDBUS_ARGS({"u", "cookie"}), _e_msgbus_screensaver_inhibit_cb, 0 },
   { "UnInhibit", ELDBUS_ARGS({"u", "cookie"}), NULL, _e_msgbus_screensaver_uninhibit_cb, 0 },
   { "GetActive", NULL, ELDBUS_ARGS({"b", "active"}), _e_msgbus_screensaver_getactive_cb, 0 },
   { NULL, NULL, NULL, NULL, 0}
};

static const Eldbus_Service_Interface_Desc _screensaver_core_desc = {
   SCREENSAVER_IFACE, _screensaver_core_methods, NULL, NULL, NULL, NULL
};
///////////////////////////////////////////////////////////////////////////


/* externally accessible functions */
EINTERN int
e_msgbus_init(void)
{
   e_msgbus_data = E_NEW(E_Msgbus_Data, 1);

   eldbus_init();

   e_msgbus_data->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!e_msgbus_data->conn)
     {
        WRN("Cannot get ELDBUS_CONNECTION_TYPE_SESSION");
        return 0;
     }
   e_msgbus_data->e_iface = eldbus_service_interface_register
     (e_msgbus_data->conn, E_PATH, &_e_core_desc);
   eldbus_service_interface_register(e_msgbus_data->conn, E_PATH, &module_desc);
   eldbus_service_interface_register(e_msgbus_data->conn, E_PATH, &profile_desc);
   eldbus_service_interface_register(e_msgbus_data->conn, E_PATH, &window_desc);
   eldbus_name_request(e_msgbus_data->conn, E_BUS, 0,
                       _e_msgbus_core_request_name_cb, NULL);

   e_msgbus_data->screensaver_iface = eldbus_service_interface_register
     (e_msgbus_data->conn, SCREENSAVER_PATH, &_screensaver_core_desc);
   e_msgbus_data->screensaver_iface2 = eldbus_service_interface_register
     (e_msgbus_data->conn, SCREENSAVER_PATH2, &_screensaver_core_desc);
   eldbus_name_request(e_msgbus_data->conn, SCREENSAVER_BUS, 0,
                       _e_msgbus_screensaver_request_name_cb, NULL);
   return 1;
}

EINTERN int
e_msgbus_shutdown(void)
{
   E_Msgbus_Data_Screensaver_Inhibit *inhibit;

   if (e_msgbus_data->e_iface)
     eldbus_service_object_unregister(e_msgbus_data->e_iface);
   if (e_msgbus_data->conn)
     {
        eldbus_name_release(e_msgbus_data->conn, E_BUS, NULL, NULL);
        eldbus_connection_unref(e_msgbus_data->conn);
     }
   eldbus_shutdown();

   EINA_LIST_FREE(e_msgbus_data->screensaver_inhibits, inhibit)
     _e_msgbus_screensaver_inhibit_free(inhibit);

   E_FREE(e_msgbus_data);
   return 1;
}

EAPI Eldbus_Service_Interface *
e_msgbus_interface_attach(const Eldbus_Service_Interface_Desc *desc)
{
   if (!e_msgbus_data->e_iface) return NULL;
   return eldbus_service_interface_register(e_msgbus_data->conn, E_PATH, desc);
}

static void
_e_msgbus_core_request_name_cb(void *data __UNUSED__, const Eldbus_Message *msg,
                          Eldbus_Pending *pending __UNUSED__)
{
   unsigned int flag;

   if (eldbus_message_error_get(msg, NULL, NULL))
     {
        ERR("Could not request bus name");
        return;
     }

   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("Could not get arguments on on_name_request");
        return;
     }

   if (!(flag & ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER))
     WRN("Enlightenment core name already in use\n");
}

/* Core Handlers */
static Eldbus_Message *
_e_msgbus_core_version_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                          const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   EINA_SAFETY_ON_NULL_RETURN_VAL(reply, NULL);
   eldbus_message_arguments_append(reply, "s", VERSION);
   return reply;
}

static Eldbus_Message *
_e_msgbus_core_restart_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                          const Eldbus_Message *msg)
{
   e_sys_action_do(E_SYS_RESTART, NULL);
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_e_msgbus_core_shutdown_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                           const Eldbus_Message *msg)
{
   e_sys_action_do(E_SYS_EXIT, NULL);
   return eldbus_message_method_return_new(msg);
}

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
static void
_e_msgbus_screensaver_request_name_cb(void *data __UNUSED__,
                                      const Eldbus_Message *msg,
                                      Eldbus_Pending *pending __UNUSED__)
{
   unsigned int flag;

   if (eldbus_message_error_get(msg, NULL, NULL))
     {
        ERR("Could not request bus name");
        return;
     }
   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("Could not get arguments on on_name_request");
        return;
     }
   if (!(flag & ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER))
     WRN("Screensaver name already in use\n");
}

static void
_e_msgbus_screensaver_inhibit_free(E_Msgbus_Data_Screensaver_Inhibit *inhibit)
{
   printf("INH: inhibit remove %i [%s] [%s] [%s]\n", inhibit->cookie, inhibit->application, inhibit->reason, inhibit->sender);
   if (inhibit->application) eina_stringshare_del(inhibit->application);
   if (inhibit->reason) eina_stringshare_del(inhibit->reason);
   if (inhibit->sender)
     {
        eldbus_name_owner_changed_callback_del(e_msgbus_data->conn,
                                               inhibit->sender,
                                               _e_msgbus_screensaver_owner_change_cb,
                                               NULL);
        eina_stringshare_del(inhibit->sender);
     }
   free(inhibit);
}

static void
_e_msgbus_screensaver_owner_change_cb(void *data __UNUSED__, const char *bus __UNUSED__, const char *old_id, const char *new_id)
{
   Eina_List *l, *ll;
   E_Msgbus_Data_Screensaver_Inhibit *inhibit;
   Eina_Bool removed = EINA_FALSE;

   printf("INH: owner change... [%s] [%s] [%s]\n", bus, old_id, new_id);
   if ((new_id) && (!new_id[0]))
     {
        EINA_LIST_FOREACH_SAFE(e_msgbus_data->screensaver_inhibits, l, ll, inhibit)
          {
             if ((inhibit->sender) && (!strcmp(inhibit->sender, old_id)))
               {
                  _e_msgbus_screensaver_inhibit_free(inhibit);
                  e_msgbus_data->screensaver_inhibits =
                    eina_list_remove_list
                     (e_msgbus_data->screensaver_inhibits, l);
                  removed = EINA_TRUE;
               }
          }
        if ((removed) && (!e_msgbus_data->screensaver_inhibits))
          {
             // stop inhibiting SS
             e_screensaver_update();
          }
     }
}

static Eldbus_Message *
_e_msgbus_screensaver_inhibit_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                                 const Eldbus_Message *msg)
{
   Eldbus_Message *reply = NULL;
   static unsigned int cookie = 1;
   const char *application_name =  NULL, *reason_for_inhibit = NULL;
   E_Msgbus_Data_Screensaver_Inhibit *inhibit;

   if (!eldbus_message_arguments_get(msg, "ss",
                                     &application_name, &reason_for_inhibit))
     {
        ERR("Can't get application_name and reason_for_inhibit");
        goto err;
     }
   inhibit = E_NEW(E_Msgbus_Data_Screensaver_Inhibit, 1);
   if (inhibit)
     {
        cookie++;
        inhibit->application = eina_stringshare_add(application_name);
        inhibit->reason = eina_stringshare_add(reason_for_inhibit);
        inhibit->sender = eina_stringshare_add(eldbus_message_sender_get(msg));
        if (inhibit->sender)
          eldbus_name_owner_changed_callback_add(e_msgbus_data->conn,
                                                 inhibit->sender,
                                                 _e_msgbus_screensaver_owner_change_cb,
                                                 NULL, EINA_FALSE);
        inhibit->cookie = cookie;
        printf("INH: inhibit [%s] [%s] [%s] -> %i\n", inhibit->application, inhibit->reason, inhibit->sender, inhibit->cookie);
        e_msgbus_data->screensaver_inhibits =
          eina_list_append(e_msgbus_data->screensaver_inhibits, inhibit);
        reply = eldbus_message_method_return_new(msg);
        if (reply)
          eldbus_message_arguments_append(reply, "u", inhibit->cookie);
     }
   if ((e_msgbus_data->screensaver_inhibits) &&
       (eina_list_count(e_msgbus_data->screensaver_inhibits) == 1))
     {
        // start inhibiting SS
        e_screensaver_deactivate();
        e_screensaver_update();
     }
err:
   return reply;
}

static Eldbus_Message *
_e_msgbus_screensaver_uninhibit_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                                   const Eldbus_Message *msg)
{
   unsigned int cookie = 0;

   if (!eldbus_message_arguments_get(msg, "u", &cookie))
     return NULL;
   e_msgbus_screensaver_inhibit_remove(cookie);
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_e_msgbus_screensaver_getactive_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                                   const Eldbus_Message *msg)
{
   Eldbus_Message *reply = NULL;

   reply = eldbus_message_method_return_new(msg);
   printf("INH: getactive = %i\n", e_screensaver_on_get());
   if (reply)
     eldbus_message_arguments_append(reply, "b", e_screensaver_on_get());
   return reply;
}

EAPI void
e_msgbus_screensaver_inhibit_remove(unsigned int cookie)
{
   Eina_Bool removed = EINA_FALSE;
   Eina_List *l;
   E_Msgbus_Data_Screensaver_Inhibit *inhibit;

   EINA_LIST_FOREACH(e_msgbus_data->screensaver_inhibits, l, inhibit)
     {
        if (inhibit->cookie == cookie)
          {
             _e_msgbus_screensaver_inhibit_free(inhibit);
             e_msgbus_data->screensaver_inhibits =
               eina_list_remove_list
                 (e_msgbus_data->screensaver_inhibits, l);
             removed = EINA_TRUE;
             break;
          }
     }
   if ((removed) && (!e_msgbus_data->screensaver_inhibits))
     {
        // stop inhibiting SS
        e_screensaver_update();
     }
}

///////////////////////////////////////////////////////////////////////////

/* Modules Handlers */
static Eldbus_Message *
_e_msgbus_module_load_cb(const Eldbus_Service_Interface *iface __UNUSED__,
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
_e_msgbus_module_unload_cb(const Eldbus_Service_Interface *iface __UNUSED__,
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
_e_msgbus_module_enable_cb(const Eldbus_Service_Interface *iface __UNUSED__,
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
_e_msgbus_module_disable_cb(const Eldbus_Service_Interface *iface __UNUSED__,
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
_e_msgbus_module_list_cb(const Eldbus_Service_Interface *iface __UNUSED__,
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

/* Profile Handlers */
static Eldbus_Message *
_e_msgbus_profile_set_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                         const Eldbus_Message *msg)
{
   char *prof;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &prof))
     return reply;

   e_config_save_flush();
   e_config_profile_set(prof);
   e_config_profile_save();
   e_config_save_block_set(1);
   e_sys_action_do(E_SYS_RESTART, NULL);

   return reply;
}

static Eldbus_Message *
_e_msgbus_profile_get_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                         const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   const char *prof;

   EINA_SAFETY_ON_NULL_RETURN_VAL(reply, NULL);
   prof = e_config_profile_get();
   eldbus_message_arguments_append(reply, "s", prof);
   return reply;
}

static Eldbus_Message *
_e_msgbus_profile_list_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                          const Eldbus_Message *msg)
{
   Eina_List *l;
   char *name;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *array, *main_iter;

   reply = eldbus_message_method_return_new(msg);
   EINA_SAFETY_ON_NULL_RETURN_VAL(reply, NULL);

   main_iter = eldbus_message_iter_get(reply);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(main_iter, reply);

   eldbus_message_iter_arguments_append(main_iter, "as", &array);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(array, reply);

   l = e_config_profile_list();
   EINA_LIST_FREE(l, name)
     {
        eldbus_message_iter_basic_append(array, 's', name);
        free(name);
     }
   eldbus_message_iter_container_close(main_iter, array);

   return reply;
}

static Eldbus_Message *
_e_msgbus_profile_add_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                         const Eldbus_Message *msg)
{
   char *prof;
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);

   if (!eldbus_message_arguments_get(msg, "s", &prof))
     return reply;
   e_config_profile_add(prof);

   return reply;
}

static Eldbus_Message *
_e_msgbus_profile_delete_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                            const Eldbus_Message *msg)
{
   char *prof;

   if (!eldbus_message_arguments_get(msg, "s", &prof))
     return eldbus_message_method_return_new(msg);
   if (!strcmp(e_config_profile_get(), prof))
     return eldbus_message_error_new(msg,
                                    "org.enlightenment.DBus.InvalidArgument",
                                    "Can't delete active prof");
   e_config_profile_del(prof);
   return eldbus_message_method_return_new(msg);
}

/* Window handlers */
static Eldbus_Message *
_e_msgbus_window_list_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                         const Eldbus_Message *msg)
{
   const Eina_List *l;
   //E_Client *ec;
   E_Border *bd;
   Eldbus_Message *reply;
   Eldbus_Message_Iter *main_iter, *array;

   reply = eldbus_message_method_return_new(msg);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(reply, NULL);

   main_iter = eldbus_message_iter_get(reply);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(main_iter, reply);

   eldbus_message_iter_arguments_append(main_iter, "a(si)", &array);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(array, reply);

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        Eldbus_Message_Iter *s;
        const char *name;

        if (bd->hidden) continue;

        eldbus_message_iter_arguments_append(array, "(si)", &s);
        if (!s) continue;
        name = bd->client.icccm.name;
        if (!name) name = "";
        eldbus_message_iter_arguments_append(s, "si", name,
                                            bd->client.win);
        eldbus_message_iter_container_close(array, s);
     }
   eldbus_message_iter_container_close(main_iter, array);

   return reply;
}

#define E_MSGBUS_WIN_ACTION_CB_BEGIN(NAME) \
   static Eldbus_Message * \
   _e_msgbus_window_##NAME##_cb(const Eldbus_Service_Interface *iface __UNUSED__, \
                                const Eldbus_Message *msg) { \
      E_Border *bd; \
      int xwin; \
      if (!eldbus_message_arguments_get(msg, "i", &xwin)) \
        return eldbus_message_method_return_new(msg); \
      bd = e_border_find_by_client_window(xwin); \
      if (bd) {
#define E_MSGBUS_WIN_ACTION_CB_END \
      } \
      return eldbus_message_method_return_new(msg); \
   }

E_MSGBUS_WIN_ACTION_CB_BEGIN(close)
  e_border_act_close_begin(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(kill)
  e_border_act_kill_begin(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(focus)
  e_border_focus_set(bd, 1, 1);
if (!bd->lock_user_stacking)
  {
     if (e_config->border_raise_on_focus)
       e_border_raise(bd);
  }
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(iconify)
  e_border_iconify(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(uniconify)
  e_border_uniconify(bd);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(maximize)
  e_border_maximize(bd, e_config->maximize_policy);
E_MSGBUS_WIN_ACTION_CB_END

E_MSGBUS_WIN_ACTION_CB_BEGIN(unmaximize)
  e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
E_MSGBUS_WIN_ACTION_CB_END

/*static Eldbus_Message *
_e_msgbus_window_sendtodesktop_cb( const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   E_Client *ec;
   E_Zone * zone;
   E_Desk * desk;
   Eina_List *l = NULL;
   int xwin, zonenum, xdesk, ydesk;

   if (!eldbus_message_arguments_get(msg, "iiii", &xwin, &zonenum, &xdesk, &ydesk))
     return eldbus_message_method_return_new(msg);

   ec = e_pixmap_find_client(E_PIXMAP_TYPE_X, xwin);

   if (ec)
     {
        EINA_LIST_FOREACH(e_comp->zones, l, zone)
          {
             if ((int)zone->num == zonenum)
               {
                  if (xdesk < zone->desk_x_count && ydesk < zone->desk_y_count)
                    {
                       E_Desk *old_desk = ec->desk;
                       Eina_Bool was_focused = e_client_stack_focused_get(ec);

                       desk = e_desk_at_xy_get(zone, xdesk, ydesk);
                       if ((desk) && (desk != old_desk))
                         {
                            e_client_desk_set(ec, desk);
                            if (was_focused)
                              e_desk_last_focused_focus(old_desk);
                         }
                    }
               }
          }
     }

   return eldbus_message_method_return_new(msg);

}*/



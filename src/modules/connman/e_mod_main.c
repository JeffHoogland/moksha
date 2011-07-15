#include "e.h"
#include "e_mod_main.h"

/*
 * STATUS:
 *
 *    displays current status, allows connecting and
 *    disconnecting. needs connman 0.48 or even better from git.
 *
 * TODO:
 *
 *    MUST:
 *       1. improve gadget ui
 *
 *    GOOD:
 *       1. imporve mouse over popup ui
 *       2. nice popup using edje objects as rows, not simple lists (fancy)
 *       3. "Controls" for detailed information, similar to Mixer app
 *          it would contain switches to toggle offline and choose
 *          technologies that are enabled.
 *
 *    IDEAS:
 *       1. create static connections
 *       2. handle cellular: ask APN, Username and Password, use SetupRequired
 *       3. handle vpn, bluetooth, wimax
 *
 */

static E_Module *connman_mod = NULL;
static char tmpbuf[PATH_MAX]; /* general purpose buffer, just use immediately */

const char _e_connman_name[] = "connman";
const char _e_connman_Name[] = "Connection Manager";
int _e_connman_log_dom = -1;

static const char *e_str_idle = NULL;
static const char *e_str_association = NULL;
static const char *e_str_configuration = NULL;
static const char *e_str_ready = NULL;
static const char *e_str_login = NULL;
static const char *e_str_online = NULL;
static const char *e_str_disconnect = NULL;
static const char *e_str_failure = NULL;

const char *e_str_enabled = NULL;
const char *e_str_available = NULL;
const char *e_str_connected = NULL;
const char *e_str_offline = NULL;

static void _connman_service_ask_pass_and_connect(E_Connman_Service *service);
static void _connman_default_service_changed_delayed(E_Connman_Module_Context *ctxt);
static void _connman_gadget_update(E_Connman_Instance *inst);
static void _connman_tip_update(E_Connman_Instance *inst);

const char *
e_connman_theme_path(void)
{
#define TF "/e-module-connman.edj"
   size_t dirlen;

   dirlen = strlen(connman_mod->dir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
     return NULL;

   memcpy(tmpbuf, connman_mod->dir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static void
_connman_toggle_offline_mode_cb(void            *data,
                                DBusMessage *msg __UNUSED__,
                                DBusError       *error)
{
   E_Connman_Module_Context *ctxt = data;

   if ((!error) || (!dbus_error_is_set(error)))
     {
        ctxt->offline_mode_pending = EINA_FALSE;
        return;
     }

   _connman_dbus_error_show(_("Cannot toggle system's offline mode."), error);
   dbus_error_free(error);
}

void
_connman_toggle_offline_mode(E_Connman_Module_Context *ctxt)
{
   Eina_Bool offline;

   if ((!ctxt) || (!ctxt->has_manager))
     {
        _connman_operation_error_show(_("ConnMan Daemon is not running."));
        return;
     }

   if (!e_connman_manager_offline_mode_get(&offline))
     {
        _connman_operation_error_show
          (_("Query system's offline mode."));
        return;
     }

   offline = !offline;
   if (!e_connman_manager_offline_mode_set
         (offline, _connman_toggle_offline_mode_cb, ctxt))
     {
        _connman_operation_error_show
          (_("Cannot toggle system's offline mode."));
        return;
     }
}

static void
_connman_cb_toggle_offline_mode(E_Object *obj      __UNUSED__,
                                const char *params __UNUSED__)
{
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
     return;

   ctxt = connman_mod->data;
   _connman_toggle_offline_mode(ctxt);
}

struct connman_passphrase_data
{
   void                      (*cb)(void       *data,
                                   const char *password,
                                   const char *service_path);
   void                     *data;
   const char               *service_path;
   char                     *passphrase;
   E_Connman_Module_Context *ctxt;
   E_Dialog                 *dia;
   Evas_Object              *entry;
   Eina_Bool                 canceled;
   int                       cleartext;
};

#if 0 // NOT WORKING, e_widget_entry_password_set() changes stops editing!!!
static void
_connman_passphrase_ask_cleartext_changed(void        *data,
                                          Evas_Object *obj,
                                          void *event  __UNUSED__)
{
   struct connman_passphrase_data *d = data;
   e_widget_entry_password_set(d->entry, !e_widget_check_checked_get(obj));
   e_widget_entry_readonly_set(d->entry, 0);
   e_widget_focus_set(d->entry, 1);
}

#endif

static void
_connman_passphrase_ask_ok(void     *data,
                           E_Dialog *dia)
{
   struct connman_passphrase_data *d = data;
   d->canceled = EINA_FALSE;
   e_object_del(E_OBJECT(dia));
}

static void
_connman_passphrase_ask_cancel(void     *data,
                               E_Dialog *dia)
{
   struct connman_passphrase_data *d = data;
   d->canceled = EINA_TRUE;
   e_object_del(E_OBJECT(dia));
}

static void
_connman_passphrase_ask_del(void *data)
{
   E_Dialog *dia = data;
   struct connman_passphrase_data *d = e_object_data_get(E_OBJECT(dia));

   if (d->canceled)
     {
        free(d->passphrase);
        d->passphrase = NULL;
     }

   d->cb(d->data, d->passphrase, d->service_path);

   eina_stringshare_del(d->service_path);
   free(d->passphrase);
   E_FREE(d);
}

static void
_connman_passphrase_ask_key_down(void          *data,
                                 Evas *e        __UNUSED__,
                                 Evas_Object *o __UNUSED__,
                                 void          *event)
{
   Evas_Event_Key_Down *ev = event;
   struct connman_passphrase_data *d = data;

   if (strcmp(ev->keyname, "Return") == 0)
     _connman_passphrase_ask_ok(d, d->dia);
   else if (strcmp(ev->keyname, "Escape") == 0)
     _connman_passphrase_ask_cancel(d, d->dia);
}

static void
_connman_passphrase_ask(E_Connman_Service                       *service,
                        void                                     (*cb)(void *data,
                                                     const char *password,
                                                     const char *service_path),
                        const void                              *data)
{
   struct connman_passphrase_data *d;
   Evas_Object *list, *o;
   Evas *evas;
   char buf[512];
   const char *passphrase;
   int mw, mh;

   if (!cb)
     return;
   if (!service)
     {
        cb((void *)data, NULL, NULL);
        return;
     }

   d = E_NEW(struct connman_passphrase_data, 1);
   if (!d)
     {
        cb((void *)data, NULL, NULL);
        return;
     }
   d->cb = cb;
   d->data = (void *)data;
   d->service_path = eina_stringshare_add(service->path);
   d->ctxt = service->ctxt;
   d->canceled = EINA_TRUE; /* closing the dialog defaults to cancel */
   d->dia = e_dialog_new(NULL, "E", "connman_ask_passphrase");

   e_dialog_title_set(d->dia, _("ConnMan needs your passphrase"));
   e_dialog_icon_set(d->dia, "dialog-ask", 64);
   e_dialog_border_icon_set(d->dia, "dialog-ask");

   evas = d->dia->win->evas;

   list = e_widget_list_add(evas, 0, 0);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/dialog",
                           "e/widgets/dialog/text");
   snprintf(buf, sizeof(buf),
            _("Connection Manager needs your passphrase for <br>"
              "the service <hilight>%s</hilight>"),
            service->name);
   edje_object_part_text_set(o, "e.textblock.message", buf);
   edje_object_size_min_calc(o, &mw, &mh);
   evas_object_size_hint_min_set(o, mw, mh);
   evas_object_resize(o, mw, mh);
   evas_object_show(o);
   e_widget_list_object_append(list, o, 1, 1, 0.5);

   if (!e_connman_service_passphrase_get(service->element, &passphrase))
     passphrase = NULL;
   if (passphrase && passphrase[0])
     d->passphrase = strdup(passphrase);
   else
     d->passphrase = NULL;

   d->entry = o = e_widget_entry_add(evas, &d->passphrase, NULL, NULL, NULL);
   e_widget_entry_password_set(o, 0);
   evas_object_show(o);
   e_widget_list_object_append(list, o, 1, 0, 0.0);

#if 0 // NOT WORKING, e_widget_entry_password_set() changes stops editing!!!
   d->cleartext = 1;
   o = e_widget_check_add(evas, _("Show passphrase as clear text"),
                          &d->cleartext);
   evas_object_smart_callback_add
     (o, "changed", _connman_passphrase_ask_cleartext_changed, d);
   evas_object_show(o);
   e_widget_list_object_append(list, o, 1, 0, 0.0);
#endif

   e_widget_size_min_get(list, &mw, &mh);
   if (mw < 200)
     mw = 200;
   if (mh < 60)
     mh = 60;
   e_dialog_content_set(d->dia, list, mw, mh);

   e_dialog_button_add
     (d->dia, _("Ok"), NULL, _connman_passphrase_ask_ok, d);
   e_dialog_button_add
     (d->dia, _("Cancel"), NULL, _connman_passphrase_ask_cancel, d);

   evas_object_event_callback_add
     (d->dia->bg_object, EVAS_CALLBACK_KEY_DOWN,
     _connman_passphrase_ask_key_down, d);

   e_object_del_attach_func_set
     (E_OBJECT(d->dia), _connman_passphrase_ask_del);
   e_object_data_set(E_OBJECT(d->dia), d);

   e_dialog_button_focus_num(d->dia, 0);
   e_widget_focus_set(d->entry, 1);

   e_win_centered_set(d->dia->win, 1);
   e_dialog_show(d->dia);
}

static void
_connman_service_free(E_Connman_Service *service)
{
   eina_stringshare_del(service->path);
   eina_stringshare_del(service->name);
   eina_stringshare_del(service->type);
   eina_stringshare_del(service->mode);
   eina_stringshare_del(service->state);
   eina_stringshare_del(service->error);
   eina_stringshare_del(service->security);
   eina_stringshare_del(service->ipv4_method);
   eina_stringshare_del(service->ipv4_address);
   eina_stringshare_del(service->ipv4_netmask);

   E_FREE(service);
}

static void
_connman_service_changed(void                    *data,
                         const E_Connman_Element *element)
{
   E_Connman_Service *service = data;
   const char *str;
   unsigned char u8;
   Eina_Bool b;

#define GSTR(name_, getter)   \
  str = NULL;                 \
  if (!getter(element, &str)) \
    str = NULL;               \
  eina_stringshare_replace(&service->name_, str)

   GSTR(name, e_connman_service_name_get);
   GSTR(type, e_connman_service_type_get);
   GSTR(mode, e_connman_service_mode_get);
   GSTR(state, e_connman_service_state_get);
   GSTR(error, e_connman_service_error_get);
   GSTR(security, e_connman_service_security_get);
   GSTR(ipv4_method, e_connman_service_ipv4_configuration_method_get);

   if (service->ipv4_method && strcmp(service->ipv4_method, "dhcp") == 0)
     {
        GSTR(ipv4_address, e_connman_service_ipv4_address_get);
        GSTR(ipv4_netmask, e_connman_service_ipv4_netmask_get);
     }
   else
     {
        GSTR(ipv4_address,
             e_connman_service_ipv4_configuration_address_get);
        GSTR(ipv4_netmask,
             e_connman_service_ipv4_configuration_netmask_get);
     }
#undef GSTR

   if ((service->state != e_str_failure) && (service->error))
     eina_stringshare_replace(&service->error, NULL);

   if (!e_connman_service_strength_get(element, &u8))
     u8 = 0;
   service->strength = u8;

#define GBOOL(name_, getter) \
  b = EINA_FALSE;            \
  if (!getter(element, &b))  \
    b = EINA_FALSE;          \
  service->name_ = b

   GBOOL(favorite, e_connman_service_favorite_get);
   GBOOL(auto_connect, e_connman_service_auto_connect_get);
   GBOOL(pass_required, e_connman_service_passphrase_required_get);
#undef GBOOL

   if ((service->ctxt->default_service == service) ||
       (!service->ctxt->default_service))
     _connman_default_service_changed_delayed(service->ctxt);
   else
     DBG("Do not request for delayed changed as this is not the default.");
}

static void
_connman_service_freed(void *data)
{
   E_Connman_Service *service = data;
   E_Connman_Module_Context *ctxt = service->ctxt;

   ctxt->services = eina_inlist_remove
       (ctxt->services, EINA_INLIST_GET(service));

   _connman_service_free(service);

   if (ctxt->default_service == service)
     {
        ctxt->default_service = NULL;
        _connman_default_service_changed_delayed(ctxt);
     }
}

static E_Connman_Service *
_connman_service_new(E_Connman_Module_Context *ctxt,
                     E_Connman_Element        *element)
{
   E_Connman_Service *service;
   const char *str;
   unsigned char u8;
   Eina_Bool b;

   if (!element)
     return NULL;

   service = E_NEW(E_Connman_Service, 1);
   if (!service)
     return NULL;

   service->ctxt = ctxt;
   service->element = element;
   service->path = eina_stringshare_add(element->path);

#define GSTR(name_, getter)   \
  str = NULL;                 \
  if (!getter(element, &str)) \
    str = NULL;               \
  service->name_ = eina_stringshare_add(str)

   GSTR(name, e_connman_service_name_get);
   GSTR(type, e_connman_service_type_get);
   GSTR(mode, e_connman_service_mode_get);
   GSTR(state, e_connman_service_state_get);
   GSTR(error, e_connman_service_error_get);
   GSTR(security, e_connman_service_security_get);
   GSTR(ipv4_method, e_connman_service_ipv4_method_get);
   GSTR(ipv4_address, e_connman_service_ipv4_address_get);
   GSTR(ipv4_netmask, e_connman_service_ipv4_netmask_get);
#undef GSTR

   if ((service->state != e_str_failure) && (service->error))
     eina_stringshare_replace(&service->error, NULL);

   if (!e_connman_service_strength_get(element, &u8))
     u8 = 0;
   service->strength = u8;

#define GBOOL(name_, getter) \
  b = EINA_FALSE;            \
  if (!getter(element, &b))  \
    b = EINA_FALSE;          \
  service->name_ = b

   GBOOL(favorite, e_connman_service_favorite_get);
   GBOOL(auto_connect, e_connman_service_auto_connect_get);
   GBOOL(pass_required, e_connman_service_passphrase_required_get);
#undef GBOOL

   e_connman_element_listener_add
     (element, _connman_service_changed, service,
     _connman_service_freed);

   return service;
}

#define GSTR(name_, getter)   \
  str = NULL;                 \
  if (!getter(element, &str)) \
    str = NULL;               \
  eina_stringshare_replace(&t->name_, str)

static void
_connman_technology_free(E_Connman_Technology *t)
{
   eina_stringshare_del(t->path);
   eina_stringshare_del(t->name);
   eina_stringshare_del(t->type);
   eina_stringshare_del(t->state);

   E_FREE(t);
}

static void
_connman_technology_changed(void                    *data,
                            const E_Connman_Element *element)
{
   E_Connman_Technology *t = data;
   const char *str;

   GSTR(name, e_connman_technology_name_get);
   GSTR(type, e_connman_technology_type_get);
   GSTR(state, e_connman_technology_state_get);
}

static void
_connman_technology_freed(void *data)
{
   E_Connman_Technology *t = data;
   E_Connman_Module_Context *ctxt = t->ctxt;

   ctxt->technologies = eina_inlist_remove
       (ctxt->technologies, EINA_INLIST_GET(t));

   _connman_technology_free(t);
}

static E_Connman_Technology *
_connman_technology_new(E_Connman_Module_Context *ctxt,
                        E_Connman_Element        *element)
{
   E_Connman_Technology *t;
   const char *str;

   if (!element)
     return NULL;

   t = E_NEW(E_Connman_Technology, 1);
   if (!t)
     return NULL;

   t->ctxt = ctxt;
   t->element = element;
   t->path = eina_stringshare_add(element->path);

   GSTR(name, e_connman_technology_name_get);
   GSTR(type, e_connman_technology_type_get);
   GSTR(state, e_connman_technology_state_get);

   e_connman_element_listener_add
     (element, _connman_technology_changed, t,
     _connman_technology_freed);

   return t;
}

#undef GSTR

static void
_connman_service_disconnect_cb(void            *data,
                               DBusMessage *msg __UNUSED__,
                               DBusError       *error)
{
   E_Connman_Module_Context *ctxt = data;

   if (error && dbus_error_is_set(error))
     {
        if ((strcmp(error->name,
                    "org.moblin.connman.Error.NotConnected") != 0) ||
	    (strcmp(error->name,
                    "net.connman.Error.NotConnected") != 0))
          _connman_dbus_error_show(_("Disconnect from network service."),
                                   error);
        dbus_error_free(error);
     }

   _connman_default_service_changed_delayed(ctxt);
}

static void
_connman_service_disconnect(E_Connman_Service *service)
{
   if (!e_connman_service_disconnect
         (service->element, _connman_service_disconnect_cb, service->ctxt))
     _connman_operation_error_show(_("Disconnect from network service."));
}

struct connman_service_connect_data
{
   const char               *service_path;
   E_Connman_Module_Context *ctxt;
};

static void
_connman_service_connect_cb(void            *data,
                            DBusMessage *msg __UNUSED__,
                            DBusError       *error)
{
   struct connman_service_connect_data *d = data;

   if (error && dbus_error_is_set(error))
     {
        /* TODO: cellular might ask for SetupRequired to enter APN,
         * username and password
         */
          if ((strcmp(error->name,
                      "org.moblin.connman.Error.PassphraseRequired") == 0) ||
              (strcmp(error->name,
                      "org.moblin.connman.Error.Failed") == 0) ||
              (strcmp(error->name,
                      "net.connman.Error.PassphraseRequired") == 0) ||
              (strcmp(error->name,
                      "net.connman.Error.Failed") == 0))
            {
               E_Connman_Service *service;

               service = _connman_ctxt_find_service_stringshare
                   (d->ctxt, d->service_path);
               if (!service)
                 _connman_operation_error_show
                   (_("Service does not exist anymore"));
               else if (strcmp(service->type, "wifi") == 0)
                 {
                    _connman_service_disconnect(service);
                    _connman_service_ask_pass_and_connect(service);
                 }
               else
     /* TODO: cellular might ask for user and pass */
                 _connman_dbus_error_show(_("Connect to network service."),
                                          error);
            }
          else if ((strcmp(error->name,
                           "org.moblin.connman.Error.AlreadyConnected") != 0) ||
                   (strcmp(error->name,
                           "net.connman.Error.AlreadyConnected") != 0))
            _connman_dbus_error_show(_("Connect to network service."), error);

          dbus_error_free(error);
     }

   _connman_default_service_changed_delayed(d->ctxt);
   eina_stringshare_del(d->service_path);
   E_FREE(d);
}

static void
_connman_service_connect(E_Connman_Service *service)
{
   struct connman_service_connect_data *d;

   d = E_NEW(struct connman_service_connect_data, 1);
   if (!d)
     return;

   d->service_path = eina_stringshare_ref(service->path);
   d->ctxt = service->ctxt;

   if (!e_connman_service_connect
         (service->element, _connman_service_connect_cb, d))
     {
        eina_stringshare_del(d->service_path);
        E_FREE(d);
        _connman_operation_error_show(_("Connect to network service."));
     }
}

struct connman_service_ask_pass_data
{
   const char               *service_path;
   E_Connman_Module_Context *ctxt;
};

static void
_connman_service_ask_pass_and_connect__set_cb(void            *data,
                                              DBusMessage *msg __UNUSED__,
                                              DBusError       *error)
{
   struct connman_service_ask_pass_data *d = data;
   E_Connman_Service *service;

   service = _connman_ctxt_find_service_stringshare(d->ctxt, d->service_path);
   if (!service)
     {
        _connman_operation_error_show(_("Service does not exist anymore"));
        goto end;
     }

   if ((!error) || (!dbus_error_is_set(error)))
     _connman_service_connect(service);

end:
   if ((error) && (dbus_error_is_set(error)))
     dbus_error_free(error);
   eina_stringshare_del(d->service_path);
   E_FREE(d);
}

static void
_connman_service_ask_pass_and_connect__ask_cb(void       *data,
                                              const char *passphrase,
                                              const char *service_path)
{
   E_Connman_Module_Context *ctxt = data;
   E_Connman_Service *service;
   struct connman_service_ask_pass_data *d;

   service = _connman_ctxt_find_service_stringshare(ctxt, service_path);
   if (!service)
     {
        _connman_operation_error_show(_("Service does not exist anymore"));
        return;
     }

   if (!passphrase)
     {
        _connman_service_disconnect(service);
        return;
     }

   d = E_NEW(struct connman_service_ask_pass_data, 1);
   if (!d)
     return;
   d->service_path = eina_stringshare_ref(service_path);
   d->ctxt = ctxt;

   if (!e_connman_service_passphrase_set
         (service->element, passphrase,
         _connman_service_ask_pass_and_connect__set_cb, d))
     {
        eina_stringshare_del(d->service_path);
        E_FREE(d);
        _connman_operation_error_show(_("Could not set service's passphrase"));
        return;
     }
}

static void
_connman_service_ask_pass_and_connect(E_Connman_Service *service)
{
   _connman_passphrase_ask
     (service, _connman_service_ask_pass_and_connect__ask_cb, service->ctxt);
}

static void
_connman_services_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->services)
     {
        E_Connman_Service *service = (E_Connman_Service *)ctxt->services;
        e_connman_element_listener_del
          (service->element, _connman_service_changed, service);
        /* no need for free or unlink, since listener_del() calls
         * _connman_service_freed()
         */
        //ctxt->services = eina_inlist_remove(ctxt->services, ctxt->services);
        //_connman_service_free(service);
     }
}

static inline Eina_Bool
_connman_services_element_exists(const E_Connman_Module_Context *ctxt,
                                 const E_Connman_Element        *element)
{
   const E_Connman_Service *service;

   EINA_INLIST_FOREACH(ctxt->services, service)
     if (service->path == element->path)
       return EINA_TRUE;

   return EINA_FALSE;
}

static inline Eina_Bool
_connman_technologies_element_exists(const E_Connman_Module_Context *ctxt,
                                     const E_Connman_Element        *element)
{
   const E_Connman_Technology *t;

   EINA_INLIST_FOREACH(ctxt->technologies, t)
     {
        if (t->path == element->path)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
_connman_request_scan_cb(void *data       __UNUSED__,
                         DBusMessage *msg __UNUSED__,
                         DBusError       *error)
{
   if (error && dbus_error_is_set(error))
     {
        ERR("%s method failed with message \'%s\'", error->name, error->message);
        dbus_error_free(error);
     }

   return;
}

static void
_connman_technologies_load(E_Connman_Module_Context *ctxt)
{
   unsigned int count, i;
   E_Connman_Element **elements;

   if (!e_connman_manager_technologies_get(&count, &elements))
     return;

   DBG("Technologies = %d.", count);
   for (i = 0; i < count; i++)
     {
        E_Connman_Element *e = elements[i];
        E_Connman_Technology *t;

        if ((!e) || _connman_technologies_element_exists(ctxt, e))
          continue;

        t = _connman_technology_new(ctxt, e);
        if (!t)
          continue;

        DBG("Added technology: %s.", t->name);
        ctxt->technologies = eina_inlist_append
            (ctxt->technologies, EINA_INLIST_GET(t));
     }

   if (!e_connman_manager_request_scan("", _connman_request_scan_cb, NULL))
     ERR("Request scan on all technologies failed.");

   free(elements);
}

static void
_connman_services_load(E_Connman_Module_Context *ctxt)
{
   unsigned int i, count;
   E_Connman_Element **elements;

   if (!e_connman_manager_services_get(&count, &elements))
     return;

   for (i = 0; i < count; i++)
     {
        E_Connman_Element *e = elements[i];
        E_Connman_Service *service;

        if ((!e) || (_connman_services_element_exists(ctxt, e)))
          continue;

        service = _connman_service_new(ctxt, e);
        if (!service)
          continue;

        DBG("Added service: %s\n", service->name);
        ctxt->services = eina_inlist_append
            (ctxt->services, EINA_INLIST_GET(service));
     }

   /* no need to remove elements, as they remove themselves */
   free(elements);
}

static void
_connman_default_service_changed(E_Connman_Module_Context *ctxt)
{
   E_Connman_Service *itr, *def = NULL;
   E_Connman_Instance *inst;
   const Eina_List *l;
   const char *tech;

   EINA_INLIST_FOREACH(ctxt->services, itr)
     {
        if ((itr->state == e_str_ready) ||
            (itr->state == e_str_login) ||
            (itr->state == e_str_online))
          {
             def = itr;
             break;
          }
        else if ((itr->state == e_str_association) &&
                 ((!def) || (def && def->state != e_str_configuration)))
          def = itr;
        else if (itr->state == e_str_configuration)
          def = itr;
     }

   DBG("Default service changed to %p (%s)", def, def ? def->name : "");

   if (!e_connman_manager_technology_default_get(&tech))
     tech = NULL;
   if (eina_stringshare_replace(&ctxt->technology, tech))
     DBG("Manager technology is '%s'", tech);

   if (!e_connman_manager_offline_mode_get(&ctxt->offline_mode))
     ctxt->offline_mode = EINA_FALSE;

   if ((e_config->mode.offline != ctxt->offline_mode) &&
       (!ctxt->offline_mode_pending))
     {
        e_config->mode.offline = ctxt->offline_mode;
        e_config_mode_changed();
        e_config_save_queue();
     }

   ctxt->default_service = def;
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _connman_gadget_update(inst);
}

static void
_connman_services_reload(E_Connman_Module_Context *ctxt)
{
   _connman_services_load(ctxt);
   _connman_default_service_changed(ctxt);
}

static Eina_Bool
_connman_default_service_changed_delayed_do(void *data)
{
   E_Connman_Module_Context *ctxt = data;
   DBG("Do delayed change.");

   ctxt->poller.default_service_changed = NULL;
   _connman_default_service_changed(ctxt);
   return ECORE_CALLBACK_CANCEL;
}

static void
_connman_default_service_changed_delayed(E_Connman_Module_Context *ctxt)
{
   if (!ctxt->has_manager)
     return;
   DBG("Request delayed change.");
   if (ctxt->poller.default_service_changed)
     ecore_poller_del(ctxt->poller.default_service_changed);
   ctxt->poller.default_service_changed = ecore_poller_add
       (ECORE_POLLER_CORE, 1, _connman_default_service_changed_delayed_do, ctxt);
}

static void _connman_popup_del(E_Connman_Instance *inst);

static Eina_Bool
_connman_popup_input_window_mouse_up_cb(void    *data,
                                        int type __UNUSED__,
                                        void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Connman_Instance *inst = data;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   _connman_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_connman_popup_input_window_key_down_cb(void    *data,
                                        int type __UNUSED__,
                                        void    *event)
{
   Ecore_Event_Key *ev = event;
   E_Connman_Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   keysym = ev->key;
   if (strcmp(keysym, "Escape") == 0)
     _connman_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_connman_popup_input_window_destroy(E_Connman_Instance *inst)
{
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;

   ecore_event_handler_del(inst->ui.input.mouse_up);
   inst->ui.input.mouse_up = NULL;

   ecore_event_handler_del(inst->ui.input.key_down);
   inst->ui.input.key_down = NULL;
}

static void
_connman_popup_input_window_create(E_Connman_Instance *inst)
{
   Ecore_X_Window_Configure_Mask mask;
   Ecore_X_Window w, popup_w;
   E_Manager *man;

   man = e_manager_current_get();

   w = ecore_x_window_input_new(man->root, 0, 0, man->w, man->h);
   mask = (ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE |
           ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING);
   popup_w = inst->popup->win->evas_win;
   ecore_x_window_configure(w, mask, 0, 0, 0, 0, 0, popup_w,
                            ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_show(w);

   inst->ui.input.mouse_up =
     ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                             _connman_popup_input_window_mouse_up_cb, inst);

   inst->ui.input.key_down =
     ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                             _connman_popup_input_window_key_down_cb, inst);

   inst->ui.input.win = w;
}

static void
_connman_popup_cb_offline_mode_changed(void        *data,
                                       Evas_Object *obj)
{
   E_Connman_Instance *inst = data;
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Eina_Bool offline = e_widget_check_checked_get(obj);

   if ((!ctxt) || (!ctxt->has_manager))
     {
        _connman_operation_error_show(_("ConnMan Daemon is not running."));
        return;
     }

   if (!e_connman_manager_offline_mode_set
         (offline, _connman_toggle_offline_mode_cb, ctxt))
     {
        _connman_operation_error_show
          (_("Cannot toggle system's offline mode."));
        return;
     }
   ctxt->offline_mode_pending = EINA_TRUE;
}

static void
_connman_popup_cb_controls(void       *data,
                           void *data2 __UNUSED__)
{
   E_Connman_Instance *inst = data;

   if (!inst)
     return;
   if (inst->popup)
     _connman_popup_del(inst);
   if (inst->ctxt->conf_dialog)
     return;

   inst->ctxt->conf_dialog = e_connman_config_dialog_new(NULL, inst->ctxt);
}

static void
_connman_popup_service_selected(void *data)
{
   E_Connman_Instance *inst = data;
   E_Connman_Module_Context *ctxt = inst->ctxt;
   E_Connman_Service *service;

   if (inst->first_selection)
     {
        inst->first_selection = EINA_FALSE;
        return;
     }

   if (!inst->service_path)
     return;

   service = _connman_ctxt_find_service_stringshare(ctxt, inst->service_path);
   if (!service)
     return;

   _connman_popup_del(inst);

   if ((service->state != e_str_idle) &&
       (service->state != e_str_disconnect) &&
       (service->state != e_str_failure))
     _connman_service_disconnect(service);
   else if (service->pass_required)
     _connman_service_ask_pass_and_connect(service);
   else
     _connman_service_connect(service);
}

Evas_Object *
_connman_service_new_list_item(Evas              *evas,
                               E_Connman_Service *service)
{
   Evas_Object *icon;
   Edje_Message_Int msg;
   char buf[128];

   snprintf(buf, sizeof(buf), "e/modules/connman/icon/%s", service->type);
   icon = edje_object_add(evas);
   e_theme_edje_object_set(icon, "base/theme/modules/connman", buf);

   snprintf(buf, sizeof(buf), "e,state,%s", service->state);
   edje_object_signal_emit(icon, buf, "e");

   if (service->mode)
     {
        snprintf(buf, sizeof(buf), "e,mode,%s", service->mode);
        edje_object_signal_emit(icon, buf, "e");
     }

   if (service->security)
     {
        snprintf(buf, sizeof(buf), "e,security,%s", service->security);
        edje_object_signal_emit(icon, buf, "e");
     }

   if (service->favorite)
     edje_object_signal_emit(icon, "e,favorite,yes", "e");
   else
     edje_object_signal_emit(icon, "e,favorite,no", "e");

   if (service->auto_connect)
     edje_object_signal_emit(icon, "e,auto_connect,yes", "e");
   else
     edje_object_signal_emit(icon, "e,auto_connect,no", "e");

   if (service->pass_required)
     edje_object_signal_emit(icon, "e,pass_required,yes", "e");
   else
     edje_object_signal_emit(icon, "e,pass_required,no", "e");

   msg.val = service->strength;
   edje_object_message_send(icon, EDJE_MESSAGE_INT, 1, &msg);

   return icon;
}

static void
_connman_popup_update(E_Connman_Instance *inst)
{
   Evas_Object *list = inst->ui.list;
   E_Connman_Service *service;
   const char *default_path;
   Evas *evas = evas_object_evas_get(list);
   int i, selected;

   default_path = inst->ctxt->default_service ?
     inst->ctxt->default_service->path : NULL;

   /* TODO: replace this with a scroller + list of edje
    * objects that are more full of features
    */
   e_widget_ilist_freeze(list);
   e_widget_ilist_clear(list);
   i = 0;
   selected = -1;
   EINA_INLIST_FOREACH(inst->ctxt->services, service)
     {
        Evas_Object *icon;

        if (service->path == default_path)
          selected = i;
        i++;

        icon = _connman_service_new_list_item(evas, service);

        e_widget_ilist_append
          (list, icon, service->name, _connman_popup_service_selected,
          inst, service->path);
     }

   if (selected >= 0)
     {
        inst->first_selection = EINA_TRUE;
        e_widget_ilist_selected_set(list, selected);
     }
   else
     inst->first_selection = EINA_FALSE;

   e_widget_ilist_thaw(list);
   e_widget_ilist_go(list);

   e_widget_check_checked_set(inst->ui.offline_mode, inst->ctxt->offline_mode);
}

static void
_connman_popup_del(E_Connman_Instance *inst)
{
   eina_stringshare_replace(&inst->service_path, NULL);
   _connman_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_connman_popup_new(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas *evas;
   Evas_Coord mw, mh;

   if (inst->popup)
     {
        e_gadcon_popup_show(inst->popup);
        return;
     }

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   inst->ui.table = e_widget_table_add(evas, 0);

   if (ctxt->default_service)
     eina_stringshare_replace(&inst->service_path, ctxt->default_service->path);

   // TODO: get this size from edj
   inst->ui.list = e_widget_ilist_add(evas, 32, 32, &inst->service_path);
   e_widget_size_min_set(inst->ui.list, 180, 100);
   e_widget_table_object_append(inst->ui.table, inst->ui.list,
                                0, 0, 1, 5, 1, 1, 1, 1);

   inst->offline_mode = ctxt->offline_mode;
   inst->ui.offline_mode = e_widget_check_add
       (evas, _("Offline mode"), &inst->offline_mode);

   evas_object_show(inst->ui.offline_mode);
   e_widget_table_object_append(inst->ui.table, inst->ui.offline_mode,
                                0, 5, 1, 1, 1, 1, 1, 0);
   e_widget_on_change_hook_set
     (inst->ui.offline_mode, _connman_popup_cb_offline_mode_changed, inst);

   inst->ui.button = e_widget_button_add
       (evas, _("Controls"), NULL,
       _connman_popup_cb_controls, inst, NULL);
   e_widget_table_object_append(inst->ui.table, inst->ui.button,
                                0, 6, 1, 1, 1, 1, 1, 0);

   _connman_popup_update(inst);

   e_widget_size_min_get(inst->ui.table, &mw, &mh);
   if (mh < 200) mh = 200;
   if (mw < 200) mw = 200;
   e_widget_size_min_set(inst->ui.table, mw, mh);

   e_gadcon_popup_content_set(inst->popup, inst->ui.table);
   e_gadcon_popup_show(inst->popup);
   _connman_popup_input_window_create(inst);
}

static void
_connman_menu_cb_post(void        *data,
                      E_Menu *menu __UNUSED__)
{
   E_Connman_Instance *inst = data;
   if ((!inst) || (!inst->menu))
     return;
   if (inst->menu)
     {
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
}

static void
_connman_menu_cb_cfg(void           *data,
                     E_Menu *menu    __UNUSED__,
                     E_Menu_Item *mi __UNUSED__)
{
   E_Connman_Instance *inst = data;

   if (!inst)
     return;
   if (inst->popup)
     _connman_popup_del(inst);
   if (inst->ctxt->conf_dialog)
     return;

   inst->ctxt->conf_dialog = e_connman_config_dialog_new(NULL, inst->ctxt);
}

static void
_connman_menu_new(E_Connman_Instance    *inst,
                  Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *m;
   E_Menu_Item *mi;
   int x, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   m = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _connman_menu_cb_cfg, inst);

   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _connman_menu_cb_post, inst);
   inst->menu = m;
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_connman_tip_new(E_Connman_Instance *inst)
{
   Evas *e;

   inst->tip = e_gadcon_popup_new(inst->gcc);
   if (!inst->tip) return;

   e = inst->tip->win->evas;

   inst->o_tip = edje_object_add(e);
   e_theme_edje_object_set(inst->o_tip, "base/theme/modules/connman/tip",
                           "e/modules/connman/tip");

   _connman_tip_update(inst);

   e_gadcon_popup_content_set(inst->tip, inst->o_tip);
   e_gadcon_popup_show(inst->tip);
}

static void
_connman_tip_del(E_Connman_Instance *inst)
{
   evas_object_del(inst->o_tip);
   e_object_del(E_OBJECT(inst->tip));
   inst->tip = NULL;
   inst->o_tip = NULL;
}

static void
_connman_cb_mouse_down(void            *data,
                       Evas *evas       __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void            *event)
{
   E_Connman_Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   if (!inst)
     return;

   ev = event;
   if (ev->button == 1)
     {
        if (!inst->popup)
          _connman_popup_new(inst);
        else
          _connman_popup_del(inst);
     }
   else if (ev->button == 2)
     _connman_toggle_offline_mode(inst->ctxt);
   else if ((ev->button == 3) && (!inst->menu))
     _connman_menu_new(inst, ev);
}

static void
_connman_cb_mouse_in(void            *data,
                     Evas *evas       __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event      __UNUSED__)
{
   E_Connman_Instance *inst = data;

   if (inst->tip)
     return;

   _connman_tip_new(inst);
}

static void
_connman_cb_mouse_out(void            *data,
                      Evas *evas       __UNUSED__,
                      Evas_Object *obj __UNUSED__,
                      void *event      __UNUSED__)
{
   E_Connman_Instance *inst = data;

   if (!inst->tip)
     return;

   _connman_tip_del(inst);
}

static void
_connman_edje_view_update(E_Connman_Instance *inst,
                          Evas_Object        *o)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   const E_Connman_Service *service;
   Edje_Message_Int msg;
   char buf[128];

   if (!ctxt->has_manager)
     {
        edje_object_signal_emit(o, "e,unavailable", "e");
        edje_object_part_text_set(o, "e.text.name", _("No ConnMan"));
        edje_object_part_text_set(o, "e.text.error",
                                  _("No ConnMan server found."));
        edje_object_signal_emit(o, "e,changed,connected,no", "e");
        edje_object_part_text_set(o, "e.text.offline_mode", "");
        edje_object_signal_emit(o, "e,changed,offline_mode,no", "e");
        return;
     }

   edje_object_signal_emit(o, "e,available", "e");

   if (ctxt->offline_mode)
     {
        edje_object_signal_emit(o, "e,changed,offline_mode,yes", "e");
        edje_object_part_text_set(o, "e.text.offline_mode",
                                  _("Offline mode: all radios are turned off"));
     }
   else
     {
        edje_object_signal_emit(o, "e,changed,offline_mode,no", "e");
        edje_object_part_text_set(o, "e.text.offline_mode", "");
     }

   if (ctxt->technology && ctxt->technology[0])
     {
        edje_object_part_text_set(o, "e.text.technology",
                                  ctxt->technology);
        snprintf(buf, sizeof(buf), "e,changed,technology,%s",
                 ctxt->technology);
        edje_object_signal_emit(o, buf, "e");
     }
   else if (!ctxt->default_service)
     {
        edje_object_part_text_set(o, "e.text.technology", "");
        edje_object_signal_emit(o, "e,changed,technology,none", "e");
     }

   service = ctxt->default_service;
   if (!service)
     {
        edje_object_part_text_set(o, "e.text.name", _("No Connection"));
        edje_object_signal_emit(o, "e,changed,service,none", "e");
        edje_object_signal_emit(o, "e,changed,connected,no", "e");

        edje_object_part_text_set(o, "e.text.error", _("Not connected"));
        edje_object_signal_emit(o, "e,changed,error,no", "e");

        edje_object_part_text_set(o, "e.text.state", _("disconnect"));
        edje_object_signal_emit(o, "e,changed,state,disconnect", "e");

        edje_object_signal_emit(o, "e,changed,mode,no", "e");

        edje_object_signal_emit(o, "e,changed,mode,none", "e");
        edje_object_signal_emit(o, "e,changed,security,none", "e");

        edje_object_part_text_set(o, "e.text.ipv4_address", "");
        edje_object_signal_emit(o, "e,changed,ipv4_address,no", "e");

        edje_object_signal_emit(o, "e,changed,favorite,no", "e");
        edje_object_signal_emit(o, "e,changed,auto_connect,no", "e");
        edje_object_signal_emit(o, "e,changed,pass_required,no", "e");

        return;
     }

   edje_object_signal_emit(o, "e,changed,connected,yes", "e");

   if (service->name)
     edje_object_part_text_set(o, "e.text.name", service->name);
   else
     edje_object_part_text_set(o, "e.text.name", _("Unknown Name"));

   if (service->error)
     {
        edje_object_part_text_set(o, "e.text.error", service->error);
        edje_object_signal_emit(o, "e,changed,error,yes", "e");
     }
   else
     {
        edje_object_part_text_set(o, "e.text.error", _("No error"));
        edje_object_signal_emit(o, "e,changed,error,no", "e");
     }

   snprintf(buf, sizeof(buf), "e,changed,service,%s", service->type);
   edje_object_signal_emit(o, buf, "e");

   if (!ctxt->technology)
     {
        edje_object_part_text_set(o, "e.text.technology", service->type);
        snprintf(buf, sizeof(buf), "e,changed,technology,%s", service->type);
        edje_object_signal_emit(o, buf, "e");
     }

   snprintf(buf, sizeof(buf), "e,changed,state,%s", service->state);
   edje_object_signal_emit(o, buf, "e");
   edje_object_part_text_set(o, "e.text.state", _(service->state));

   if (service->mode)
     {
        snprintf(buf, sizeof(buf), "e,changed,mode,%s", service->mode);
        edje_object_signal_emit(o, buf, "e");
     }
   else
     edje_object_signal_emit(o, "e,changed,mode,none", "e");

   if (service->security)
     {
        snprintf(buf, sizeof(buf), "e,changed,security,%s", service->security);
        edje_object_signal_emit(o, buf, "e");
     }
   else
     edje_object_signal_emit(o, "e,changed,security,none", "e");

   if (service->ipv4_address)
     {
        edje_object_part_text_set(o, "e.text.ipv4_address", service->ipv4_address);
        edje_object_signal_emit(o, "e,changed,ipv4_address,yes", "e");
     }
   else
     {
        edje_object_part_text_set(o, "e.text.ipv4_address", "");
        edje_object_signal_emit(o, "e,changed,ipv4_address,no", "e");
     }

   if (service->favorite)
     edje_object_signal_emit(o, "e,changed,favorite,yes", "e");
   else
     edje_object_signal_emit(o, "e,changed,favorite,no", "e");

   if (service->auto_connect)
     edje_object_signal_emit(o, "e,changed,auto_connect,yes", "e");
   else
     edje_object_signal_emit(o, "e,changed,auto_connect,no", "e");

   if (service->pass_required)
     edje_object_signal_emit(o, "e,changed,pass_required,yes", "e");
   else
     edje_object_signal_emit(o, "e,changed,pass_required,no", "e");

   msg.val = service->strength;
   edje_object_message_send(o, EDJE_MESSAGE_INT, 1, &msg);
}

static void
_connman_tip_update(E_Connman_Instance *inst)
{
   _connman_edje_view_update(inst, inst->o_tip);
}

static void
_connman_gadget_update(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;

   if (!ctxt->has_manager && inst->popup)
     _connman_popup_del(inst);

   if (inst->popup)
     _connman_popup_update(inst);
   if (inst->tip)
     _connman_tip_update(inst);

   _connman_edje_view_update(inst, inst->ui.gadget);
}

/* Gadcon Api Functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon   *gc,
         const char *name,
         const char *id,
         const char *style)
{
   E_Connman_Instance *inst;
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;

   inst = E_NEW(E_Connman_Instance, 1);
   inst->ctxt = ctxt;
   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/connman",
                           "e/modules/connman/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN, _connman_cb_mouse_down, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_IN, _connman_cb_mouse_in, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_OUT, _connman_cb_mouse_out, inst);

   _connman_gadget_update(inst);

   ctxt->instances = eina_list_append(ctxt->instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Connman_Module_Context *ctxt;
   E_Connman_Instance *inst;

   if (!connman_mod)
     return;

   ctxt = connman_mod->data;
   if (!ctxt)
     return;

   inst = gcc->data;
   if (!inst)
     return;

   if (inst->menu)
     {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        e_object_del(E_OBJECT(inst->menu));
     }
   evas_object_del(inst->ui.gadget);

   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client       *gcc,
           E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _(_e_connman_Name);
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__,
         Evas                               *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_connman_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   E_Connman_Module_Context *ctxt;
   Eina_List *instances;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;
   if (!ctxt)
     return NULL;

   instances = ctxt->instances;
   snprintf(tmpbuf, sizeof(tmpbuf), "connman.%d", eina_list_count(instances));
   return tmpbuf;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, _e_connman_name,
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _e_connman_Name};

static const char _act_toggle_offline_mode[] = "toggle_offline_mode";
static const char _lbl_toggle_offline_mode[] = "Toggle Offline Mode";

static void
_connman_actions_register(E_Connman_Module_Context *ctxt)
{
   ctxt->actions.toggle_offline_mode = e_action_add(_act_toggle_offline_mode);
   if (ctxt->actions.toggle_offline_mode)
     {
        ctxt->actions.toggle_offline_mode->func.go =
          _connman_cb_toggle_offline_mode;
        e_action_predef_name_set
          (_(_e_connman_Name), _(_lbl_toggle_offline_mode), _act_toggle_offline_mode,
          NULL, NULL, 0);
     }
}

static void
_connman_actions_unregister(E_Connman_Module_Context *ctxt)
{
   if (ctxt->actions.toggle_offline_mode)
     {
        e_action_predef_name_del(_(_e_connman_Name), _(_lbl_toggle_offline_mode));
        e_action_del(_act_toggle_offline_mode);
     }
}

static Eina_Bool
_connman_manager_changed_do(void *data)
{
   E_Connman_Module_Context *ctxt = data;

   _connman_technologies_load(ctxt);
   _connman_services_reload(ctxt);

   ctxt->poller.manager_changed = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_connman_manager_changed(void                            *data,
                         const E_Connman_Element *element __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);
   ctxt->poller.manager_changed = ecore_poller_add
       (ECORE_POLLER_CORE, 1, _connman_manager_changed_do, ctxt);
}

static Eina_Bool
_connman_event_manager_in(void       *data,
                          int type    __UNUSED__,
                          void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   E_Connman_Element *element;

   ctxt->has_manager = EINA_TRUE;

   element = e_connman_manager_get();
   e_connman_element_listener_add
     (element, _connman_manager_changed, ctxt, NULL);

   _connman_services_reload(ctxt);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_connman_event_manager_out(void       *data,
                           int type    __UNUSED__,
                           void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;

   ctxt->has_manager = EINA_FALSE;
   eina_stringshare_replace(&ctxt->technology, NULL);

   _connman_services_free(ctxt);
   _connman_default_service_changed(ctxt);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_connman_event_mode_changed(void       *data,
                            int type    __UNUSED__,
                            void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   if ((ctxt->offline_mode == e_config->mode.offline) ||
       (!ctxt->has_manager))
     return ECORE_CALLBACK_PASS_ON;
   if (!ctxt->offline_mode_pending)
     {
        if (!e_connman_manager_offline_mode_set(e_config->mode.offline,
                                                _connman_toggle_offline_mode_cb, ctxt))
          _connman_operation_error_show(_("Cannot toggle system's offline mode."));
     }
   else
     ctxt->offline_mode_pending = EINA_FALSE;

   return ECORE_CALLBACK_PASS_ON;
}

static E_Config_Dialog *
_connman_config(E_Container       *con,
                const char *params __UNUSED__)
{
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
     return NULL;

   ctxt = connman_mod->data;
   if (!ctxt)
     return NULL;

   if (!ctxt->conf_dialog)
     ctxt->conf_dialog = e_connman_config_dialog_new(con, ctxt);

   return ctxt->conf_dialog;
}

static const char _reg_cat[] = "extensions";
static const char _reg_item[] = "extensions/connman";

static void
_connman_configure_registry_register(void)
{
   e_configure_registry_category_add(_reg_cat, 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add(_reg_item, 110, _(_e_connman_Name), NULL,
                                 e_connman_theme_path(),
                                 _connman_config);
}

static void
_connman_configure_registry_unregister(void)
{
   e_configure_registry_item_del(_reg_item);
   e_configure_registry_category_del(_reg_cat);
}

static void
_connman_events_register(E_Connman_Module_Context *ctxt)
{
   ctxt->event.manager_in = ecore_event_handler_add
       (E_CONNMAN_EVENT_MANAGER_IN, _connman_event_manager_in, ctxt);
   ctxt->event.manager_out = ecore_event_handler_add
       (E_CONNMAN_EVENT_MANAGER_OUT, _connman_event_manager_out, ctxt);
   ctxt->event.mode_changed = ecore_event_handler_add
       (E_EVENT_CONFIG_MODE_CHANGED, _connman_event_mode_changed, ctxt);
}

static void
_connman_events_unregister(E_Connman_Module_Context *ctxt)
{
   if (ctxt->event.manager_in)
     ecore_event_handler_del(ctxt->event.manager_in);
   if (ctxt->event.manager_out)
     ecore_event_handler_del(ctxt->event.manager_out);
   if (ctxt->event.mode_changed)
     ecore_event_handler_del(ctxt->event.mode_changed);
}

static inline void
_connman_status_stringshare_init(void)
{
   e_str_idle = eina_stringshare_add(N_("idle"));
   e_str_association = eina_stringshare_add(N_("association"));
   e_str_configuration = eina_stringshare_add(N_("configuration"));
   e_str_ready = eina_stringshare_add(N_("ready"));
   e_str_login = eina_stringshare_add(N_("login"));
   e_str_online = eina_stringshare_add(N_("online"));
   e_str_disconnect = eina_stringshare_add(N_("disconnect"));
   e_str_failure = eina_stringshare_add(N_("failure"));
   e_str_enabled = eina_stringshare_add(N_("enabled"));
   e_str_available = eina_stringshare_add(N_("available"));
   e_str_connected = eina_stringshare_add(N_("connected"));
   e_str_offline = eina_stringshare_add(N_("offline"));
}

static inline void
_connman_status_stringshare_del(void)
{
   eina_stringshare_replace(&e_str_idle, NULL);
   eina_stringshare_replace(&e_str_association, NULL);
   eina_stringshare_replace(&e_str_configuration, NULL);
   eina_stringshare_replace(&e_str_ready, NULL);
   eina_stringshare_replace(&e_str_login, NULL);
   eina_stringshare_replace(&e_str_online, NULL);
   eina_stringshare_replace(&e_str_disconnect, NULL);
   eina_stringshare_replace(&e_str_failure, NULL);
   eina_stringshare_replace(&e_str_enabled, NULL);
   eina_stringshare_replace(&e_str_available, NULL);
   eina_stringshare_replace(&e_str_connected, NULL);
   eina_stringshare_replace(&e_str_offline, NULL);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Connman_Module_Context *ctxt;
   E_DBus_Connection *c;

   _connman_status_stringshare_init();

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     goto error_dbus_bus_get;
   if (!e_connman_system_init(c))
     goto error_connman_system_init;

   ctxt = E_NEW(E_Connman_Module_Context, 1);
   if (!ctxt)
     goto error_connman_context;

   ctxt->services = NULL;
   ctxt->technologies = NULL;
   ctxt->conf_dialog = NULL;
   connman_mod = m;

   if (_e_connman_log_dom < 0)
     {
        _e_connman_log_dom = eina_log_domain_register("econnman", EINA_COLOR_ORANGE);
        if (_e_connman_log_dom < 0)
          {
             EINA_LOG_CRIT("could not register logging domain econnman");
             goto error_log_domain;
          }
     }

   _connman_configure_registry_register();
   _connman_actions_register(ctxt);
   e_gadcon_provider_register(&_gc_class);

   _connman_events_register(ctxt);

   return ctxt;

error_log_domain:
   _e_connman_log_dom = -1;
   connman_mod = NULL;
   E_FREE(ctxt);
error_connman_context:
   e_connman_system_shutdown();
error_connman_system_init:
error_dbus_bus_get:
   _connman_status_stringshare_del();
   return NULL;
}

static void
_connman_instances_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Connman_Instance *inst;

        inst = ctxt->instances->data;

        if (inst->popup)
          _connman_popup_del(inst);
        if (inst->tip)
          _connman_tip_del(inst);

        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Connman_Module_Context *ctxt;
   E_Connman_Element *element;

   ctxt = m->data;
   if (!ctxt)
     return 0;

   element = e_connman_manager_get();
   e_connman_element_listener_del
     (element, _connman_manager_changed, ctxt);

   _connman_events_unregister(ctxt);

   _connman_instances_free(ctxt);
   _connman_services_free(ctxt);

   _connman_configure_registry_unregister();
   _connman_actions_unregister(ctxt);
   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->poller.default_service_changed)
     ecore_poller_del(ctxt->poller.default_service_changed);
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);

   E_FREE(ctxt);
   connman_mod = NULL;

   e_connman_system_shutdown();

   _connman_status_stringshare_del();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Connman_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;
   return 1;
}


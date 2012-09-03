#include "e.h"
#include "e_mod_main.h"

/*
 * STATUS:
 *
 * ConnMan in, ConnMan out
 *
 */

static E_Module *connman_mod;
static char tmpbuf[4096]; /* general purpose buffer, just use immediately */

const char _e_connman_name[] = "connman";
const char _e_connman_Name[] = "Connection Manager";
int _e_connman_log_dom = -1;

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
_connman_edje_view_update(E_Connman_Instance *inst, Evas_Object *o)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
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

        edje_object_part_text_set(o, "e.text.technology", "");
        edje_object_signal_emit(o, "e,changed,technology,none", "e");


   if (1) // not connected
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
}

static void
_connman_gadget_update(E_Connman_Instance *inst)
{
   _connman_edje_view_update(inst, inst->ui.gadget);
}

static void
_connman_cb_toggle_offline_mode(E_Object *obj __UNUSED__,
                                const char *params __UNUSED__)
{
}

static void
_connman_cb_mouse_down(void *data, Evas *evas __UNUSED__,
                       Evas_Object *obj __UNUSED__, void *event)
{
}

static void
_connman_cb_mouse_in(void *data, Evas *evas __UNUSED__,
                     Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
}

static void
_connman_cb_mouse_out(void *data, Evas *evas __UNUSED__,
                      Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
}

/* Gadcon Api Functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
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

   evas_object_del(inst->ui.gadget);

   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _(_e_connman_Name);
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_connman_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
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

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, _e_connman_Name };

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

   ctxt->poller.manager_changed = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_connman_manager_changed(void *data)
{
   E_Connman_Module_Context *ctxt = data;
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);
   ctxt->poller.manager_changed = ecore_poller_add
       (ECORE_POLLER_CORE, 1, _connman_manager_changed_do, ctxt);
}

static Eina_Bool
_connman_event_manager_in(void *data, int type __UNUSED__,
                          void *event __UNUSED__)
{
   DBG("Manager in");
   E_Connman_Module_Context *ctxt = data;
   const Eina_List *l;
   E_Connman_Instance *inst;

   ctxt->has_manager = EINA_TRUE;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _connman_gadget_update(inst);

// Get manager
// Set poller when changed
// Load services
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_connman_event_manager_out(void *data, int type __UNUSED__,
                           void *event __UNUSED__)
{
   DBG("Manager out");
   E_Connman_Module_Context *ctxt = data;
   const Eina_List *l;
   E_Connman_Instance *inst;

   ctxt->has_manager = EINA_FALSE;
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _connman_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_connman_event_mode_changed(void *data, int type __UNUSED__,
                            void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   if ((ctxt->offline_mode == e_config->mode.offline) ||
       (!ctxt->has_manager))
     return ECORE_CALLBACK_PASS_ON;

//TODO: set offline mode

   return ECORE_CALLBACK_PASS_ON;
}

static E_Config_Dialog *
_connman_config(E_Container *con, const char *params __UNUSED__)
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

//TODO: register for state changed (offline-mode)
}

static void
_connman_events_unregister(E_Connman_Module_Context *ctxt)
{
   if (ctxt->event.manager_in)
     ecore_event_handler_del(ctxt->event.manager_in);
   if (ctxt->event.manager_out)
     ecore_event_handler_del(ctxt->event.manager_out);

// TODO: unregister for state changed (offline-mode)
//   if (ctxt->event.mode_changed)
//     ecore_event_handler_del(ctxt->event.mode_changed);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Connman_Module_Context *ctxt;
   E_DBus_Connection *c;

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     goto error_dbus_bus_get;
   if (!e_connman_system_init(c))
     goto error_connman_system_init;

   ctxt = E_NEW(E_Connman_Module_Context, 1);
   if (!ctxt)
     goto error_connman_context;

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
   return NULL;
}

static void
_connman_instances_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Connman_Instance *inst;

        inst = ctxt->instances->data;
#if 0
//TODO: enable this aprt
        if (inst->popup)
          _connman_popup_del(inst);
        if (inst->tip)
          _connman_tip_del(inst);
#endif
        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Connman_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;

   _connman_events_unregister(ctxt);
   _connman_instances_free(ctxt);
   _connman_configure_registry_unregister();
   _connman_actions_unregister(ctxt);
   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);

   E_FREE(ctxt);
   connman_mod = NULL;

   e_connman_system_shutdown();
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

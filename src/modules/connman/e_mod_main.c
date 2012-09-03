#include "e.h"
#include "e_mod_main.h"

#include "E_Connman.h"

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

static Evas_Object * _econnman_service_new_icon(struct Connman_Service *cs,
                                                Evas *evas)
{
   const char *type = econnman_service_type_to_str(cs->type);
   const char *state = econnman_state_to_str(cs->state);
   Evas_Object *icon;
   Edje_Message_Int msg;
   char buf[128];

   snprintf(buf, sizeof(buf), "e/modules/connman/icon/%s", type);
   icon = edje_object_add(evas);
   e_theme_edje_object_set(icon, "base/theme/modules/connman", buf);

   if (state)
     {
        snprintf(buf, sizeof(buf), "e,state,%s", state);
        edje_object_signal_emit(icon, buf, "e");
     }

   msg.val = cs->strength;
   edje_object_message_send(icon, EDJE_MESSAGE_INT, 1, &msg);

   return icon;
}

static void _econnman_popup_update(struct Connman_Manager *cm,
                                   E_Connman_Instance *inst)
{
   Evas_Object *list = inst->ui.popup.list;
   Evas *evas = evas_object_evas_get(list);
   struct Connman_Service *cs;

   EINA_SAFETY_ON_NULL_RETURN(cm);

   e_widget_ilist_freeze(list);
   e_widget_ilist_clear(list);

   EINA_INLIST_FOREACH(cm->services, cs)
     {
        Evas_Object *icon = _econnman_service_new_icon(cs, evas);
        e_widget_ilist_append(list, icon, cs->name, NULL, NULL, cs->obj.path);
     }

   e_widget_ilist_thaw(list);
   e_widget_ilist_go(list);
}

void econnman_mod_services_changed(struct Connman_Manager *cm)
{
   E_Connman_Module_Context *ctxt = connman_mod->data;
   const Eina_List *l;
   E_Connman_Instance *inst;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        if (!inst->popup)
          continue;

        _econnman_popup_update(cm, inst);
     }
}

static void _econnman_popup_del(E_Connman_Instance *inst);

static Eina_Bool _econnman_popup_input_window_mouse_up_cb(void *data,
                                                          int type __UNUSED__,
                                                          void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Connman_Instance *inst = data;

   if (ev->window != inst->ui.popup.input_win)
     return ECORE_CALLBACK_PASS_ON;

   _econnman_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void _econnman_popup_input_window_destroy(E_Connman_Instance *inst)
{
   ecore_x_window_free(inst->ui.popup.input_win);
   inst->ui.popup.input_win = 0;

   ecore_event_handler_del(inst->ui.popup.input_mouse_up);
   inst->ui.popup.input_mouse_up = NULL;
}

static void _econnman_popup_input_window_create(E_Connman_Instance *inst)
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

   inst->ui.popup.input_mouse_up =
         ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                 _econnman_popup_input_window_mouse_up_cb, inst);

   inst->ui.popup.input_win = w;
}

static void _econnman_popup_new(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas *evas;
   Evas_Coord mw, mh;
   Evas_Object *ot;

   EINA_SAFETY_ON_FALSE_RETURN(inst->popup == NULL);

   if (!ctxt->cm)
     return;

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   ot = e_widget_table_add(evas, 0);
   inst->ui.popup.list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_size_min_set(inst->ui.popup.list, 120, 100);
   e_widget_table_object_append(ot, inst->ui.popup.list, 0, 0, 1, 5,
                                1, 1, 1, 1);

   _econnman_popup_update(ctxt->cm, inst);

   e_widget_size_min_get(ot, &mw, &mh);
   if (mh < 200)
     mh = 200;
   if (mw < 200)
     mw = 200;
   e_widget_size_min_set(ot, mw, mh);

   e_gadcon_popup_content_set(inst->popup, ot);
   e_gadcon_popup_show(inst->popup);
   _econnman_popup_input_window_create(inst);
}

static void _econnman_popup_del(E_Connman_Instance *inst)
{
   _econnman_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void _econnman_mod_manager_update_inst(E_Connman_Module_Context *ctxt,
                                              E_Connman_Instance *inst,
                                              enum Connman_State state,
                                              enum Connman_Service_Type type)
{
   char buf[128];
   Evas_Object *o = inst->ui.gadget;
   const char *statestr;

   switch (state)
     {
      case CONNMAN_STATE_ONLINE:
         edje_object_signal_emit(o, "e,changed,connected,yes", "e");
         break;
      case CONNMAN_STATE_READY:
         edje_object_signal_emit(o, "e,changed,connected,yes", "e");
         break;
      case CONNMAN_STATE_IDLE:
      case CONNMAN_STATE_OFFLINE:
      case CONNMAN_STATE_NONE:
         edje_object_signal_emit(o, "e,changed,connected,no", "e");
         break;
     }

   statestr = econnman_state_to_str(state);
   snprintf(buf, sizeof(buf), "e,changed,state,%s", statestr);
   edje_object_signal_emit(o, statestr, "e");

   switch (type)
     {
      case CONNMAN_SERVICE_TYPE_ETHERNET:
         edje_object_signal_emit(o, "e,changed,technology,ethernet", "e");
         break;
      case CONNMAN_SERVICE_TYPE_WIFI:
         edje_object_signal_emit(o, "e,changed,technology,wifi", "e");
         break;
      case CONNMAN_SERVICE_TYPE_NONE:
         edje_object_signal_emit(o, "e,changed,technology,none", "e");
         break;
     }

   DBG("state=%d type=%d", state, type);
}

void econnman_mod_manager_update(struct Connman_Manager *cm)
{
   enum Connman_Service_Type type;
   E_Connman_Module_Context *ctxt = connman_mod->data;
   E_Connman_Instance *inst;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(cm);

   DBG("cm->services=%p", cm->services);

   if (cm->services && (cm->state == CONNMAN_STATE_ONLINE ||
                        cm->state == CONNMAN_STATE_READY))
     {
        struct Connman_Service *cs = EINA_INLIST_CONTAINER_GET(cm->services,
                                                      struct Connman_Service);
        type = cs->type;
     }
   else
     type = CONNMAN_SERVICE_TYPE_NONE;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _econnman_mod_manager_update_inst(ctxt, inst, cm->state, type);
}

static void _econnman_gadget_setup(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas_Object *o = inst->ui.gadget;

   DBG("has_manager=%d", ctxt->cm != NULL);

   if (!ctxt->cm)
     {
        edje_object_signal_emit(o, "e,unavailable", "e");
        edje_object_signal_emit(o, "e,changed,connected,no", "e");
     }
   else
     edje_object_signal_emit(o, "e,available", "e");

   return;
}

void econnman_mod_manager_inout(struct Connman_Manager *cm)
{
   E_Connman_Module_Context *ctxt = connman_mod->data;
   const Eina_List *l;
   E_Connman_Instance *inst;

   DBG("Manager %s", cm ? "in" : "out");
   ctxt->cm = cm;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _econnman_gadget_setup(inst);

   if (ctxt->cm)
     econnman_mod_manager_update(cm);
}

static void
_econnman_cb_mouse_down(void *data, Evas *evas __UNUSED__,
                       Evas_Object *obj __UNUSED__, void *event)
{
   E_Connman_Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;

   if (!inst)
     return;

   if (ev->button == 1)
     {
        if (!inst->popup)
          _econnman_popup_new(inst);
        else
          _econnman_popup_del(inst);
     }
}

static void
_econnman_cb_mouse_in(void *data, Evas *evas __UNUSED__,
                     Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
}

static void
_econnman_cb_mouse_out(void *data, Evas *evas __UNUSED__,
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
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN, _econnman_cb_mouse_down, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_IN, _econnman_cb_mouse_in, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_OUT, _econnman_cb_mouse_out, inst);

   _econnman_gadget_setup(inst);

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

   if (inst->popup)
     _econnman_popup_del(inst);

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

static E_Config_Dialog * _econnman_config(E_Container *con,
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
_econnman_configure_registry_register(void)
{
   e_configure_registry_category_add(_reg_cat, 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add(_reg_item, 110, _(_e_connman_Name), NULL,
                                 e_connman_theme_path(),
                                 _econnman_config);
}

static void
_econnman_configure_registry_unregister(void)
{
   e_configure_registry_item_del(_reg_item);
   e_configure_registry_category_del(_reg_cat);
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

   _econnman_configure_registry_register();
   e_gadcon_provider_register(&_gc_class);

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
_econnman_instances_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Connman_Instance *inst = ctxt->instances->data;
        ctxt->instances = eina_list_remove_list(ctxt->instances,
                                                ctxt->instances);
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

   _econnman_instances_free(ctxt);
   _econnman_configure_registry_unregister();
   e_gadcon_provider_unregister(&_gc_class);

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

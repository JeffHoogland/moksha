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
   Edje_Message_Int_Set *msg;
   Evas_Object *icon;
   char buf[128];

   snprintf(buf, sizeof(buf), "e/modules/connman/icon/%s", type);
   icon = edje_object_add(evas);
   e_theme_edje_object_set(icon, "base/theme/modules/connman", buf);

   msg = malloc(sizeof(*msg) + sizeof(int));
   msg->count = 2;
   msg->val[0] = cs->state;
   msg->val[1] = cs->strength;

   edje_object_message_send(icon, EDJE_MESSAGE_INT_SET, 1, msg);
   free(msg);

   return icon;
}

static Evas_Object * _econnman_service_new_end(struct Connman_Service *cs,
                                               Evas *evas)
{
   Eina_Iterator *iter;
   Evas_Object *end;
   void *security;
   char buf[128];

   end = edje_object_add(evas);
   e_theme_edje_object_set(end, "base/theme/modules/connman",
                           "e/modules/connman/end");

   if (!cs->security)
     return end;

   iter = eina_array_iterator_new(cs->security);
   while (eina_iterator_next(iter, &security))
     {
        snprintf(buf, sizeof(buf), "e,security,%s", (const char *)security);
        edje_object_signal_emit(end, buf, "e");
     }
   eina_iterator_free(iter);

   return end;
}

static void _econnman_disconnect_cb(void *data, const char *error)
{
   const char *path = data;

   if (error == NULL)
     return;

   ERR("Could not disconnect %s: %s", path, error);
}

static void _econnman_connect_cb(void *data, const char *error)
{
   const char *path = data;

   if (error == NULL)
     return;

   ERR("Could not connect %s: %s", path, error);
}

static void _econnman_popup_selected_cb(void *data)
{
   E_Connman_Instance *inst = data;
   const char *path;
   struct Connman_Service *cs;

   path = e_widget_ilist_selected_value_get(inst->ui.popup.list);
   if (path == NULL)
     return;

   cs = econnman_manager_find_service(inst->ctxt->cm, path);
   if (cs == NULL)
     return;

   switch (cs->state)
     {
      case CONNMAN_STATE_READY:
      case CONNMAN_STATE_ONLINE:
         INF("Disconnect %s", path);
         econnman_service_disconnect(cs, _econnman_disconnect_cb, (void *) path);
         break;
      default:
         INF("Connect %s", path);
         econnman_service_connect(cs, _econnman_connect_cb, (void *) path);
         break;
     }
}

static void _econnman_popup_update(struct Connman_Manager *cm,
                                   E_Connman_Instance *inst)
{
   Evas_Object *list = inst->ui.popup.list;
   Evas *evas = evas_object_evas_get(list);
   struct Connman_Service *cs;
   const char *hidden = "«hidden»";

   EINA_SAFETY_ON_NULL_RETURN(cm);

   e_widget_ilist_freeze(list);
   e_widget_ilist_clear(list);

   EINA_INLIST_FOREACH(cm->services, cs)
     {
        Evas_Object *icon = _econnman_service_new_icon(cs, evas);
        Evas_Object *end = _econnman_service_new_end(cs, evas);
        e_widget_ilist_append_full(list, icon, end, cs->name ?: hidden,
                                   _econnman_popup_selected_cb,
                                   inst, cs->obj.path);
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
                                                          int type, void *event)
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

static void
_econnman_configure_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   /* FIXME call econnman-bin or other set on modules settings */
}

static void _econnman_popup_new(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas_Object *list, *bt;
   Evas_Coord mw, mh;
   Evas *evas;

   EINA_SAFETY_ON_FALSE_RETURN(inst->popup == NULL);

   if (!ctxt->cm)
     return;

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   list = e_widget_list_add(evas, 0, 0);
   inst->ui.popup.list = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_size_min_set(inst->ui.popup.list, 120, 100);
   e_widget_list_object_append(list, inst->ui.popup.list, 1, 1, 0.5);

   _econnman_popup_update(ctxt->cm, inst);

   bt = e_widget_button_add(evas, "Configure", NULL,
                            _econnman_configure_cb, NULL, NULL);
   e_widget_list_object_append(list, bt, 1, 0, 0.5);

   e_widget_size_min_get(list, &mw, &mh);
   if (mh < 220)
     mh = 220;
   if (mw < 200)
     mw = 200;
   e_widget_size_min_set(list, mw, mh);

   e_gadcon_popup_content_set(inst->popup, list);
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
   Evas_Object *o = inst->ui.gadget;
   Edje_Message_Int_Set *msg;
   const char *typestr;
   char buf[128];

   msg = malloc(sizeof(*msg) + sizeof(int));
   msg->count = 2;
   msg->val[0] = state;
   /* FIXME check if it's possible to receive strenght as props of cm */
   if (type == -1)
       msg->val[1] = 0;
   else
       msg->val[1] = 100;

   edje_object_message_send(o, EDJE_MESSAGE_INT_SET, 1, msg);
   free(msg);

   typestr = econnman_service_type_to_str(type);
   snprintf(buf, sizeof(buf), "e,changed,technology,%s", typestr);
   edje_object_signal_emit(o, buf, "e");

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

   if ((cm->services) && ((cm->state == CONNMAN_STATE_ONLINE) ||
                          (cm->state == CONNMAN_STATE_READY)))
     /* FIXME would be nice to represent "configuring state".
        theme already supports it */
     /*                 (cm->state == CONNMAN_STATE_ASSOCIATION) ||
                          (cm->state == CONNMAN_STATE_CONFIGURATION))) */
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

static void _econnman_menu_cb_configure(void *data, E_Menu *menu,
                                        E_Menu_Item *mi)
{
   /* TODO */
}

static void _econnman_menu_new(E_Connman_Instance *inst,
                               Evas_Event_Mouse_Down *ev)
{
   E_Menu *m;
   E_Menu_Item *mi;
   int x, y;

   m = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _econnman_menu_cb_configure, inst);

   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m,
                         e_util_zone_current_get(e_manager_current_get()),
                         x + ev->output.x, y + ev->output.y, 1, 1,
                         E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
}

static void
_econnman_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
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
   else if (ev->button == 3)
     _econnman_menu_new(inst, ev);
}

static void
_econnman_cb_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event)
{
}

static void
_econnman_cb_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event)
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
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class)
{
   return _(_e_connman_Name);
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_connman_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
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

static E_Config_Dialog * _econnman_config(E_Container *con, const char *params)
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

   if (_e_connman_log_dom < 0)
     {
        _e_connman_log_dom = eina_log_domain_register("econnman",
                                                      EINA_COLOR_ORANGE);
        if (_e_connman_log_dom < 0)
          {
             EINA_LOG_CRIT("could not register logging domain econnman");
             goto error_log_domain;
          }
     }

   ctxt = E_NEW(E_Connman_Module_Context, 1);
   if (!ctxt)
     goto error_connman_context;

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     goto error_dbus_bus_get;
   if (!e_connman_system_init(c))
     goto error_connman_system_init;

   ctxt->conf_dialog = NULL;
   connman_mod = m;

   _econnman_configure_registry_register();
   e_gadcon_provider_register(&_gc_class);

   return ctxt;

error_connman_system_init:
error_dbus_bus_get:
   E_FREE(ctxt);
error_connman_context:
   eina_log_domain_unregister(_e_connman_log_dom);
error_log_domain:
   _e_connman_log_dom = -1;
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

   e_connman_system_shutdown();

   _econnman_instances_free(ctxt);
   _econnman_configure_registry_unregister();
   e_gadcon_provider_unregister(&_gc_class);

   E_FREE(ctxt);
   connman_mod = NULL;

   eina_log_domain_unregister(_e_connman_log_dom);
   _e_connman_log_dom = -1;

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

/* TODO: config dialog to select which modem to monitor */

#include "e.h"
#include "e_mod_main.h"

static E_Module *ofono_mod = NULL;
static char tmpbuf[PATH_MAX];

const char _e_ofono_name[] = "ofono";
const char _e_ofono_Name[] = N_("Mobile Modems Info");
int _e_ofono_module_log_dom = -1;

static void _ofono_gadget_update(E_Ofono_Instance *inst);
static void _ofono_tip_update(E_Ofono_Instance *inst);

const char *
e_ofono_theme_path(void)
{
#define TF "/e-module-ofono.edj"
   size_t dirlen;

   dirlen = strlen(ofono_mod->dir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
      return NULL;

   memcpy(tmpbuf, ofono_mod->dir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static void _ofono_popup_del(E_Ofono_Instance *inst);

static Eina_Bool
_ofono_popup_input_window_mouse_up_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Ofono_Instance *inst = data;

   if (ev->window != inst->ui.input.win)
      return ECORE_CALLBACK_PASS_ON;

   _ofono_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ofono_popup_input_window_key_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   E_Ofono_Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->ui.input.win)
      return ECORE_CALLBACK_PASS_ON;

   keysym = ev->key;
   if (!strcmp(keysym, "Escape"))
      _ofono_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_ofono_popup_input_window_destroy(E_Ofono_Instance *inst)
{
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;

   ecore_event_handler_del(inst->ui.input.mouse_up);
   inst->ui.input.mouse_up = NULL;

   ecore_event_handler_del(inst->ui.input.key_down);
   inst->ui.input.key_down = NULL;
}

static void
_ofono_popup_input_window_create(E_Ofono_Instance *inst)
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
                              _ofono_popup_input_window_mouse_up_cb, inst);

   inst->ui.input.key_down =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                              _ofono_popup_input_window_key_down_cb, inst);

   inst->ui.input.win = w;
}

static void
_ofono_toggle_powered_cb(void *data, DBusMessage *msg __UNUSED__, DBusError *error)
{
   E_Ofono_Instance *inst = data;

   if ((error) && (dbus_error_is_set(error)))
     _ofono_dbus_error_show(_("Failed to power modem on/off."), error);
   else
     DBG("new powered value set");

   e_widget_disabled_set(inst->ui.powered, (int)EINA_FALSE);
   inst->powered_pending = EINA_FALSE;

   dbus_error_free(error);
}

static void
_ofono_popup_cb_powered_changed(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Ofono_Instance *inst = data;
   E_Ofono_Module_Context *ctxt = inst->ctxt;
   Eina_Bool powered = e_widget_check_checked_get(obj);

   if ((!ctxt) || (!ctxt->has_manager))
     {
	_ofono_operation_error_show(_("oFono Daemon is not running."));
	return;
     }

   if (!e_ofono_modem_powered_set(inst->modem_element, powered,
				  _ofono_toggle_powered_cb, inst))
     {
	_ofono_operation_error_show(_("Cannot toggle modem's powered state."));
	return;
     }
   e_widget_disabled_set(obj, EINA_TRUE);
   inst->powered_pending = EINA_TRUE;
   DBG("powered = %d pending", !inst->powered);
}

static void
_ofono_popup_update(E_Ofono_Instance *inst)
{
   if (inst->name)
     e_widget_label_text_set(inst->ui.name, inst->name);
   else
     e_widget_label_text_set(inst->ui.name, _("No modem available"));
   e_widget_check_checked_set(inst->ui.powered, inst->powered);
}

static void
_ofono_popup_del(E_Ofono_Instance *inst)
{
   _ofono_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_ofono_popup_new(E_Ofono_Instance *inst)
{
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

   if (inst->name)
     inst->ui.name = e_widget_label_add(evas, inst->name);
   else
     inst->ui.name = e_widget_label_add(evas, "No modem available");
   e_widget_table_object_append(inst->ui.table, inst->ui.name,
				0, 0, 1, 1, 1, 1, 1, 1);
   evas_object_show(inst->ui.name);

   inst->int_powered = inst->powered;
   inst->ui.powered = e_widget_check_add(evas, _("Powered"),
					 &inst->int_powered);
   e_widget_table_object_append(inst->ui.table, inst->ui.powered,
				0, 1, 1, 1, 1, 1, 1, 1);
   if (inst->powered_pending)
     e_widget_disabled_set(inst->ui.powered, (int)EINA_TRUE);
   evas_object_show(inst->ui.powered);
   evas_object_smart_callback_add(inst->ui.powered, "changed",
				  _ofono_popup_cb_powered_changed, inst);

   _ofono_popup_update(inst);

   e_widget_size_min_get(inst->ui.table, &mw, &mh);
   e_widget_size_min_set(inst->ui.table, mw, mh);

   e_gadcon_popup_content_set(inst->popup, inst->ui.table);
   e_gadcon_popup_show(inst->popup);
   _ofono_popup_input_window_create(inst);
}

static void
_ofono_menu_cb_post(void *data, E_Menu *menu __UNUSED__)
{
   E_Ofono_Instance *inst = data;
   if ((!inst) || (!inst->menu))
      return;
   if (inst->menu)
     {
	e_object_del(E_OBJECT(inst->menu));
	inst->menu = NULL;
     }
}

static void
_ofono_menu_new(E_Ofono_Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *m;
   int x, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   m = e_menu_new();
   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _ofono_menu_cb_post, inst);
   inst->menu = m;
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_ofono_tip_new(E_Ofono_Instance *inst)
{
   Evas *e;

   inst->tip = e_gadcon_popup_new(inst->gcc);
   if (!inst->tip) return;

   e = inst->tip->win->evas;

   inst->o_tip = edje_object_add(e);
   e_theme_edje_object_set(inst->o_tip, "base/theme/modules/ofono/tip",
			   "e/modules/ofono/tip");

   _ofono_tip_update(inst);

   e_gadcon_popup_content_set(inst->tip, inst->o_tip);
   e_gadcon_popup_show(inst->tip);
}

static void
_ofono_tip_del(E_Ofono_Instance *inst)
{
   evas_object_del(inst->o_tip);
   e_object_del(E_OBJECT(inst->tip));
   inst->tip = NULL;
   inst->o_tip = NULL;
}

static void
_ofono_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   E_Ofono_Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   if (!inst)
      return;

   ev = event;
   if (ev->button == 1)
     {
	if (!inst->popup)
	  _ofono_popup_new(inst);
	else
	  _ofono_popup_del(inst);
     }
   else if (ev->button == 2)
     _ofono_popup_cb_powered_changed(inst, inst->ui.powered, NULL);
   else if ((ev->button == 3) && (!inst->menu))
     _ofono_menu_new(inst, ev);
}

static void
_ofono_cb_mouse_in(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   E_Ofono_Instance *inst = data;

   if (inst->tip)
     return;

   _ofono_tip_new(inst);
}

static void
_ofono_cb_mouse_out(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   E_Ofono_Instance *inst = data;

   if (!inst->tip)
     return;

   _ofono_tip_del(inst);
}

static void
_ofono_edje_view_update(E_Ofono_Instance *inst, Evas_Object *o)
{
   Edje_Message_Int msg;
   char buf[128];

   if (!inst->ctxt->has_manager)
     {
	edje_object_signal_emit(o, "e,unavailable", "e");
	edje_object_part_text_set(o, "e.text.error", _("ofonod is not running"));
	return;
     }

   edje_object_signal_emit(o, "e,available", "e");

   if (inst->name)
     edje_object_part_text_set(o, "e.text.name", inst->name);
   else
     edje_object_part_text_set(o, "e.text.name", _("Unknown name"));

   if (!inst->powered)
     {
	edje_object_part_text_set(o, "e.text.error", _("Modem powered off"));
	edje_object_signal_emit(o, "e,netinfo,unavailable", "e");
	return;
     }

   if (inst->status)
     {
	snprintf(buf, sizeof(buf), "%c%s",
		 toupper(inst->status[0]), inst->status + 1);
	edje_object_part_text_set(o, "e.text.status", buf);
	edje_object_signal_emit(o, "e,netinfo,available", "e");
     }
   else
     edje_object_part_text_set(o, "e.text.status", _("Unknown status"));

   if (inst->op)
     {
	edje_object_part_text_set(o, "e.text.op", inst->op);
	edje_object_signal_emit(o, "e,netinfo,available", "e");
     }
   else
     edje_object_part_text_set(o, "e.text.op", _("Unknown operator"));

   msg.val = inst->strength;
   edje_object_message_send(o, EDJE_MESSAGE_INT, 1, &msg);
}

static void
_ofono_tip_update(E_Ofono_Instance *inst)
{
   _ofono_edje_view_update(inst, inst->o_tip);
}

static void
_ofono_gadget_update(E_Ofono_Instance *inst)
{
   E_Ofono_Module_Context *ctxt = inst->ctxt;

   if (!ctxt->has_manager && inst->popup)
     _ofono_popup_del(inst);

   if (inst->popup)
     _ofono_popup_update(inst);
   if (inst->tip)
     _ofono_tip_update(inst);

   _ofono_edje_view_update(inst, inst->ui.gadget);
}

/* Gadcon Api Functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Ofono_Instance *inst;
   E_Ofono_Module_Context *ctxt;

   if (!ofono_mod)
      return NULL;

   ctxt = ofono_mod->data;

   inst = E_NEW(E_Ofono_Instance, 1);
   inst->ctxt = ctxt;
   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/ofono",
                           "e/modules/ofono/main");

   inst->path = NULL;
   inst->name = NULL;
   inst->powered = EINA_FALSE;
   inst->int_powered = 0;
   inst->status = NULL;
   inst->op = NULL;
   inst->strength = 0;
   inst->modem_element = NULL;
   inst->netreg_element = NULL;

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN, _ofono_cb_mouse_down, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_IN, _ofono_cb_mouse_in, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_OUT, _ofono_cb_mouse_out, inst);

   _ofono_gadget_update(inst);

   ctxt->instances = eina_list_append(ctxt->instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Ofono_Module_Context *ctxt;
   E_Ofono_Instance *inst;

   if (!ofono_mod)
      return;

   ctxt = ofono_mod->data;
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
   if (inst->popup)
     _ofono_popup_del(inst);
   if (inst->tip)
     _ofono_tip_del(inst);

   evas_object_del(inst->ui.gadget);

   eina_stringshare_del(inst->path);
   eina_stringshare_del(inst->name);
   eina_stringshare_del(inst->status);
   eina_stringshare_del(inst->op);

   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _(_e_ofono_Name);
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_ofono_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   E_Ofono_Module_Context *ctxt;
   Eina_List *instances;

   if (!ofono_mod)
      return NULL;

   ctxt = ofono_mod->data;
   if (!ctxt)
      return NULL;

   instances = ctxt->instances;
   snprintf(tmpbuf, sizeof(tmpbuf), "ofono.%d", eina_list_count(instances));
   return tmpbuf;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, _e_ofono_name,
   {
     _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
     e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _e_ofono_Name};

static Eina_Bool
_ofono_manager_changed_do(void *data)
{
   E_Ofono_Module_Context *ctxt = data;

   ctxt->poller.manager_changed = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_ofono_manager_changed(void *data, const E_Ofono_Element *element __UNUSED__)
{
   E_Ofono_Module_Context *ctxt = data;
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);
   ctxt->poller.manager_changed = ecore_poller_add
     (ECORE_POLLER_CORE, 1, _ofono_manager_changed_do, ctxt);
}

static Eina_Bool
_ofono_event_manager_in(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   E_Ofono_Element *element;
   E_Ofono_Module_Context *ctxt = data;
   E_Ofono_Instance *inst;
   Eina_List *l;

   DBG("manager in");

   ctxt->has_manager = EINA_TRUE;

   element = e_ofono_manager_get();
   e_ofono_element_listener_add(element, _ofono_manager_changed, ctxt, NULL);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
       _ofono_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_ofono_event_manager_out(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   E_Ofono_Module_Context *ctxt = data;
   E_Ofono_Instance *inst;
   Eina_List *l;

   DBG("manager out");

   ctxt->has_manager = EINA_FALSE;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
      _ofono_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_eofono_event_element_add(void *data, int type __UNUSED__, void *info)
{
   E_Ofono_Element *element = info;
   E_Ofono_Module_Context *ctxt = data;
   E_Ofono_Instance *inst;
   Eina_List *l;
   Eina_Bool have_inst = EINA_FALSE;

   DBG(">>> %s %s", element->path, element->interface);

   /* is there any instance taking care of this modem already? */
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
      if ((inst->path) && (inst->path == element->path))
	{
	   have_inst = EINA_TRUE;
	   break;
	}

   /* if no instance is handling this, is there any instance available */
   if ((!have_inst) && (e_ofono_element_is_modem(element)))
     EINA_LIST_FOREACH(ctxt->instances, l, inst)
	if (!inst->path)
	  {
	     inst->path = eina_stringshare_ref(element->path);
	     DBG("bound %s to an ofono module instance", inst->path);
	     have_inst = EINA_TRUE;
	     break;
	  }

   /* if still orphan, do nothing */
   if (!have_inst)
     return ECORE_CALLBACK_PASS_ON;

   if (e_ofono_element_is_modem(element))
     inst->modem_element = element;
   else if (e_ofono_element_is_netreg(element))
     inst->netreg_element = element;

   _ofono_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_eofono_event_element_del(void *data, int type __UNUSED__, void *info)
{
   E_Ofono_Element *element = info;
   E_Ofono_Module_Context *ctxt = data;
   E_Ofono_Instance *inst;
   Eina_List *l;
   Eina_Bool inst_found = EINA_FALSE;

   DBG("<<< %s %s", element->path, element->interface);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
      if ((inst->path) && (inst->path == element->path))
	{
	   inst_found = EINA_TRUE;
	   break;
	}

   if (!inst_found)
     return ECORE_CALLBACK_PASS_ON;

   if (e_ofono_element_is_modem(element))
     {
	inst->modem_element = NULL;
	eina_stringshare_replace(&inst->name, NULL);
	inst->powered = EINA_FALSE;
     }
   else if (e_ofono_element_is_netreg(element))
     {
	inst->netreg_element = NULL;
	eina_stringshare_replace(&inst->status, NULL);
	eina_stringshare_replace(&inst->op, NULL);
	inst->strength = 0;
     }

   _ofono_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_eofono_event_element_updated(void *data, int type __UNUSED__, void *event_info)
{
   E_Ofono_Element *element = event_info;
   E_Ofono_Module_Context *ctxt = data;
   E_Ofono_Instance *inst;
   Eina_List *l;
   Eina_Bool inst_found = EINA_FALSE;
   const char *tmp;

   DBG("!!! %s %s", element->path, element->interface);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
      if ((inst->path) && (inst->path == element->path))
	{
	   inst_found = EINA_TRUE;
	   break;
	}

   if (!inst_found)
     return ECORE_CALLBACK_PASS_ON;

   if (e_ofono_element_is_modem(element))
     {
	if (!e_ofono_modem_powered_get(element, &(inst->powered)))
	  inst->powered = 0;

	if (!e_ofono_modem_name_get(element, &tmp))
	  tmp = NULL;
	if ((!tmp) || (!tmp[0]))
	  tmp = inst->path;
	eina_stringshare_replace(&inst->name, tmp);

	DBG("!!! powered = %d, name = %s", inst->powered, inst->name);
     }
   else if (e_ofono_element_is_netreg(element))
     {
	if (!e_ofono_netreg_status_get(element, &tmp))
	  tmp = NULL;
	eina_stringshare_replace(&inst->status, tmp);

	if (!e_ofono_netreg_operator_get(element, &tmp))
	  tmp = NULL;
	eina_stringshare_replace(&inst->op, tmp);

	if (!e_ofono_netreg_strength_get(element, &(inst->strength)))
	  inst->strength = 0;

	DBG("!!! status = %s, operator = %s, strength = %d",
	    inst->status, inst->op, inst->strength);
     }

   _ofono_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_ofono_events_register(E_Ofono_Module_Context *ctxt)
{
   ctxt->event.manager_in = ecore_event_handler_add
      (E_OFONO_EVENT_MANAGER_IN, _ofono_event_manager_in, ctxt);
   ctxt->event.manager_out = ecore_event_handler_add
      (E_OFONO_EVENT_MANAGER_OUT, _ofono_event_manager_out, ctxt);
   ctxt->event.element_add = ecore_event_handler_add
      (E_OFONO_EVENT_ELEMENT_ADD, _eofono_event_element_add, ctxt);
   ctxt->event.element_del = ecore_event_handler_add
      (E_OFONO_EVENT_ELEMENT_DEL, _eofono_event_element_del, ctxt);
   ctxt->event.element_updated = ecore_event_handler_add
      (E_OFONO_EVENT_ELEMENT_UPDATED, _eofono_event_element_updated, ctxt);
}

static void
_ofono_events_unregister(E_Ofono_Module_Context *ctxt)
{
   if (ctxt->event.manager_in)
     ecore_event_handler_del(ctxt->event.manager_in);
   if (ctxt->event.manager_out)
     ecore_event_handler_del(ctxt->event.manager_out);
   if (ctxt->event.element_add)
     ecore_event_handler_del(ctxt->event.element_add);
   if (ctxt->event.element_del)
     ecore_event_handler_del(ctxt->event.element_del);
   if (ctxt->event.element_updated)
     ecore_event_handler_del(ctxt->event.element_updated);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Ofono_Module_Context *ctxt;
   E_DBus_Connection *c;

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
     goto error_dbus_bus_get;
   if (!e_ofono_system_init(c))
     goto error_ofono_system_init;

   ctxt = E_NEW(E_Ofono_Module_Context, 1);
   if (!ctxt)
     goto error_ofono_context;

   ofono_mod = m;

   if (_e_ofono_module_log_dom < 0)
     {
	_e_ofono_module_log_dom = eina_log_domain_register("e_module_ofono",
							   EINA_COLOR_ORANGE);
	if (_e_ofono_module_log_dom < 0)
	  {
	     EINA_LOG_CRIT("could not register logging domain e_module_ofono");
	     goto error_log_domain;
	  }
     }

   e_gadcon_provider_register(&_gc_class);

   _ofono_events_register(ctxt);

   return ctxt;

error_log_domain:
   _e_ofono_module_log_dom = -1;
   ofono_mod = NULL;
   E_FREE(ctxt);
error_ofono_context:
   e_ofono_system_shutdown();
error_ofono_system_init:
error_dbus_bus_get:
   return NULL;
}

static void
_ofono_instances_free(E_Ofono_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Ofono_Instance *inst;

        inst = ctxt->instances->data;

	if (inst->popup)
	  _ofono_popup_del(inst);
	if (inst->tip)
	  _ofono_tip_del(inst);

        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Ofono_Module_Context *ctxt;
   E_Ofono_Element *element;

   ctxt = m->data;
   if (!ctxt)
     return 0;

   element = e_ofono_manager_get();
   e_ofono_element_listener_del(element, _ofono_manager_changed, ctxt);

   _ofono_events_unregister(ctxt);

   _ofono_instances_free(ctxt);

   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);

   E_FREE(ctxt);
   ofono_mod = NULL;

   e_ofono_system_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Ofono_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;
   return 1;
}

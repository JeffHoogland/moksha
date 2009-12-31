/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/*
 * STATUS:
 *
 *    displays current status, segfaults often. needs connman from git
 *    (will be 0.48, still unreleased).
 *
 * TODO:
 *
 *    MUST:
 *       1. improve gadget ui
 *
 *    GOOD:
 *       1. mouse over popup with information such as IP and AP name
 *          (remove name from gadget)
 *       2. nice popup using edje objects as rows, not simple lists (fancy)
 *       3. "Controls" for detailed information, similar to Mixer app
 *          it would contain switches to toggle offline and choose
 *          technologies that are enabled.
 *       4. toggle internal e17 mode (Menu > Settings > Mode > Offline)
 *
 *    IDEAS:
 *       1. create static connections
 *
 */

static E_Module *connman_mod = NULL;
static char tmpbuf[PATH_MAX]; /* general purpose buffer, just use immediately */

static const char _name[] = "connman";
const char _Name[] = "Connection Manager";

static const char *e_str_idle = NULL;
static const char *e_str_association = NULL;
static const char *e_str_configuration = NULL;
static const char *e_str_ready = NULL;
static const char *e_str_disconnect = NULL;
static const char *e_str_failure = NULL;

static void _connman_default_service_changed_delayed(E_Connman_Module_Context *ctxt);
static void _connman_gadget_update(E_Connman_Instance *inst);

static const char *
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

static inline void
_connman_dbus_error_show(const char *msg, const DBusError *error)
{
   const char *name;

   if ((!error) || (!dbus_error_is_set(error)))
     return;

   name = error->name;
   if (strncmp(name, "org.moblin.connman.Error.",
	       sizeof("org.moblin.connman.Error.") - 1) == 0)
     name += sizeof("org.moblin.connman.Error.") - 1;

   e_util_dialog_show(_("Connman Server Operation Failed"),
		      _("Could not execute remote operation:<br>"
			"%s<br>"
			"Server Error <b>%s:</b> %s"),
		      msg, name, error->message);
}

static inline void
_connman_operation_error_show(const char *msg)
{
   e_util_dialog_show(_("Connman Operation Failed"),
		      _("Could not execute local operation:<br>%s"),
		      msg);
}

static void
_connman_toggle_offline_mode_cb(void *data, DBusMessage *msg __UNUSED__, DBusError *error)
{
   E_Connman_Module_Context *ctxt = data;

   if ((!error) || (!dbus_error_is_set(error)))
     {
	printf("DBG CONNMAN: successfuly toggled to offline mode\n");
	// XXX hack: connman does not emit propertychanged for this, they need to fix it
	e_connman_element_properties_sync(e_connman_manager_get());
	_connman_default_service_changed_delayed(ctxt);
	return;
     }

   _connman_dbus_error_show(_("Cannot toggle system's offline mode."), error);
   dbus_error_free(error);
}

static void
_connman_toggle_offline_mode(E_Connman_Module_Context *ctxt)
{
   bool offline;

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
_connman_cb_toggle_offline_mode(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   E_Connman_Module_Context *ctxt;

   if (!connman_mod)
      return;

   ctxt = connman_mod->data;
   _connman_toggle_offline_mode(ctxt);
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
_connman_service_changed(void *data, const E_Connman_Element *element)
{
   E_Connman_Service *service = data;
   const char *str;
   unsigned char u8;
   bool b;

#define GSTR(name, getter)			\
   if (!getter(element, &str))			\
     str = NULL;				\
   eina_stringshare_replace(&service->name, str)

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

   if (!e_connman_service_strength_get(element, &u8))
     u8 = 0;
   service->strength = u8;

#define GBOOL(name, getter)				\
   if (!getter(element, &b))				\
     b = EINA_FALSE;					\
   service->name = b

   GBOOL(favorite, e_connman_service_favorite_get);
   GBOOL(auto_connect, e_connman_service_auto_connect_get);
   GBOOL(pass_required, e_connman_service_passphrase_required_get);
#undef GBOOL


   printf("DBG CONNMAN: service details changed: (default=%p, %hhu)\n"
	  "    name....: %s\n"
	  "    state...: %s\n"
	  "    type....: %s\n"
	  "    error...: %s\n"
	  "    security: %s\n"
	  "    strength: %hhu\n"
	  "    flags...: favorite=%hhu, auto_connect=%hhu, pass_required=%hhu\n",
	  service->ctxt->default_service,
	  service->ctxt->default_service == service,
	  service->name,
	  service->state,
	  service->type,
	  service->error,
	  service->security,
	  service->strength,
	  service->favorite,
	  service->auto_connect,
	  service->pass_required);

   if ((service->ctxt->default_service == service) ||
       (!service->ctxt->default_service))
     _connman_default_service_changed_delayed(service->ctxt);
   else
     printf("DBG CONNMAN: do not request for delayed changed as this is not the default.\n");
}

static void
_connman_service_freed(void *data)
{
   E_Connman_Service *service = data;
   E_Connman_Module_Context *ctxt = service->ctxt;

   printf("DBG CONNMAN service freed %s\n", service->name);

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
_connman_service_new(E_Connman_Module_Context *ctxt, E_Connman_Element *element)
{
   E_Connman_Service *service;
   const char *str;
   unsigned char u8;
   bool b;

   if (!element)
     return NULL;

   service = E_NEW(E_Connman_Service, 1);
   if (!service)
     return NULL;

   service->ctxt = ctxt;
   service->element = element;
   service->path = eina_stringshare_add(element->path);

#define GSTR(name, getter)			\
   if (!getter(element, &str))			\
     str = NULL;				\
   service->name = eina_stringshare_add(str)

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

   if (!e_connman_service_strength_get(element, &u8))
     u8 = 0;
   service->strength = u8;

#define GBOOL(name, getter)				\
   if (!getter(element, &b))				\
     b = EINA_FALSE;					\
   service->name = b

   GBOOL(favorite, e_connman_service_favorite_get);
   GBOOL(auto_connect, e_connman_service_auto_connect_get);
   GBOOL(pass_required, e_connman_service_passphrase_required_get);
#undef GBOOL

   e_connman_element_listener_add
     (element, _connman_service_changed, service,
      _connman_service_freed);

   return service;
}

static void
_connman_service_ask_pass_and_connect(E_Connman_Service *service)
{
   e_util_dialog_show("TODO", "TODO!");
}

static void
_connman_service_connect_cb(void *data, DBusMessage *msg __UNUSED__, DBusError *error)
{
   E_Connman_Module_Context *ctxt = data;

   if (error && dbus_error_is_set(error))
     {
	if (strcmp(error->message,
		   "org.moblin.connman.Error.AlreadyConnected") != 0)
	  _connman_dbus_error_show(_("Connect to network service."), error);
	dbus_error_free(error);
     }

   _connman_default_service_changed_delayed(ctxt);
}

static void
_connman_service_connect(E_Connman_Service *service)
{
   if (!e_connman_service_connect
       (service->element, _connman_service_connect_cb, service->ctxt))
     _connman_operation_error_show(_("Connect to network service."));
}

static void
_connman_services_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->services)
     {
	E_Connman_Service *service = (E_Connman_Service *)ctxt->services;
	ctxt->services = eina_inlist_remove(ctxt->services, ctxt->services);
	_connman_service_free(service);
     }
}

static inline Eina_Bool
_connman_services_element_exists(const E_Connman_Module_Context *ctxt, const E_Connman_Element *element)
{
   const E_Connman_Service *service;

   EINA_INLIST_FOREACH(ctxt->services, service)
     if (service->path == element->path)
       return EINA_TRUE;

   return EINA_FALSE;
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
	  {
	     printf("DBG CONNMAN service already exists %p (%s)\n",
		    e, e ? e->path : "");
	     continue;
	  }

	service = _connman_service_new(ctxt, e);
	if (!service)
	  continue;

	printf("DBG CONNMAN added service: %s\n", service->name);
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
	if (itr->state == e_str_ready)
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

   printf("DBG CONNMAN: default service changed to %p (%s)\n", def, def ? def->name : "");

   if (!e_connman_manager_technology_default_get(&tech))
     tech = NULL;
   eina_stringshare_replace(&ctxt->technology, tech);
   printf("DBG CONNMAN: manager technology is '%s'\n", tech);

   if (!e_connman_manager_offline_mode_get(&ctxt->offline_mode))
     ctxt->offline_mode = EINA_FALSE;

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

static int
_connman_default_service_changed_delayed_do(void *data)
{
   E_Connman_Module_Context *ctxt = data;

   ctxt->poller.default_service_changed = NULL;
   printf("\033[32mDBG CONNMAN: do delayed change\033[0m\n");
   _connman_default_service_changed(ctxt);
   return 0;
}

static void
_connman_default_service_changed_delayed(E_Connman_Module_Context *ctxt)
{
   printf("\033[1;31mDBG CONNMAN: request delayed change\033[0m\n");
   if (ctxt->poller.default_service_changed)
     ecore_poller_del(ctxt->poller.default_service_changed);
   ctxt->poller.default_service_changed = ecore_poller_add
     (ECORE_POLLER_CORE, 1, _connman_default_service_changed_delayed_do, ctxt);
}

static void _connman_popup_del(E_Connman_Instance *inst);

static int
_connman_popup_input_window_mouse_up_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Connman_Instance *inst = data;

   if (ev->window != inst->ui.input.win)
      return 1;

   _connman_popup_del(inst);

   return 1;
}

static int
_connman_popup_input_window_key_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   E_Connman_Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->ui.input.win)
      return 1;

   keysym = ev->key;
   if (strcmp(keysym, "Escape") == 0)
      _connman_popup_del(inst);

   return 1;
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
_connman_popup_cb_offline_mode_changed(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Connman_Instance *inst = data;
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Eina_Bool offline = e_widget_check_checked_get(obj);

   if ((!ctxt) || (!ctxt->has_manager))
     {
	_connman_operation_error_show(_("ConnMan Daemon is not running."));
	return;
     }

   printf(">>>> OFFLINE=%hhu\n", offline);

   if (!e_connman_manager_offline_mode_set
       (offline, _connman_toggle_offline_mode_cb, ctxt))
     {
	_connman_operation_error_show
	  (_("Cannot toggle system's offline mode."));
	return;
     }
}

static void
_connman_popup_cb_controls(void *data, void *data2 __UNUSED__)
{
   E_Connman_Instance *inst = data;

   _connman_popup_del(inst);

   e_util_dialog_show("TODO", "TODO!");
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

   EINA_INLIST_FOREACH(ctxt->services, service)
     {
	if (service->path == inst->service_path)
	  {
	     _connman_popup_del(inst);

	     if (service->pass_required)
	       _connman_service_ask_pass_and_connect(service);
	     else
	       _connman_service_connect(service);
	     return;
	  }
     }
}

static void
_connman_popup_update(E_Connman_Instance *inst)
{
   Evas_Object *list = inst->ui.list;
   E_Connman_Service *service;
   const char *default_path;
   Evas *evas = evas_object_evas_get(list);
   int i, selected;
   char buf[128];

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
	Edje_Message_Int msg;

	if (service->path == default_path)
	  selected = i;
	i++;

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
   evas_object_smart_callback_add
     (inst->ui.offline_mode, "changed",
      _connman_popup_cb_offline_mode_changed, inst);

   inst->ui.button = e_widget_button_add
     (evas, _("Controls"), NULL,
      _connman_popup_cb_controls, inst, NULL);
   e_widget_table_object_append(inst->ui.table, inst->ui.button,
                                0, 6, 1, 1, 1, 1, 1, 0);

   _connman_popup_update(inst);

   e_widget_size_min_get(inst->ui.table, &mw, &mh);
   if (mh < 208) mh = 208;
   if (mw < 200) mw = 200;
   e_widget_size_min_set(inst->ui.table, mw, mh);

   e_gadcon_popup_content_set(inst->popup, inst->ui.table);
   e_gadcon_popup_show(inst->popup);
   _connman_popup_input_window_create(inst);
}

static void
_connman_menu_cb_post(void *data, E_Menu *menu __UNUSED__)
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
_connman_menu_cb_cfg(void *data, E_Menu *menu __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Connman_Instance *inst = data;

   if (!inst)
      return;
   if (inst->popup)
      _connman_popup_del(inst);

   e_util_dialog_show("TODO", "TODO!");
}

static void
_connman_menu_new(E_Connman_Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *mn;
   E_Menu_Item *mi;
   int x, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   mn = e_menu_new();
   e_menu_post_deactivate_callback_set(mn, _connman_menu_cb_post, inst);
   inst->menu = mn;

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _connman_menu_cb_cfg, inst);

   e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(mn, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_connman_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
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
_connman_gadget_update(E_Connman_Instance *inst)
{
   E_Connman_Module_Context *ctxt = inst->ctxt;
   Evas_Object *gadget = inst->ui.gadget;
   const E_Connman_Service *service;
   Edje_Message_Int msg;
   char buf[128];

   printf("\033[1;33mUPDATE GADGET\033[0m\n");

   if (!ctxt->has_manager)
     {
	if (inst->popup)
	  _connman_popup_del(inst);

	edje_object_signal_emit(gadget, "e,unavailable", "e");
	edje_object_part_text_set(gadget, "e.text.name", _("No ConnMan"));
	edje_object_part_text_set(gadget, "e.text.error",
				  _("No ConnMan server found."));
     }

   if (inst->popup)
     _connman_popup_update(inst);

   edje_object_signal_emit(gadget, "e,available", "e");

   if (ctxt->offline_mode)
     edje_object_signal_emit(gadget, "e,changed,offline_mode,yes", "e");
   else
     edje_object_signal_emit(gadget, "e,changed,offline_mode,no", "e");


   printf("DBG CONNMAN: technology: %s\n", ctxt->technology);

   if (ctxt->technology)
     {
	edje_object_part_text_set(gadget, "e.text.technology",
				  ctxt->technology);
	snprintf(buf, sizeof(buf), "e,changed,technology,%s",
		 ctxt->technology);
	edje_object_signal_emit(gadget, buf, "e");
     }
   else
     {
	edje_object_part_text_set(gadget, "e.text.technology", "");
	edje_object_signal_emit(gadget, "e,changed,technology,none", "e");
     }

   service = ctxt->default_service;
   printf("DBG CONNMAN: default_service: %p (%s)\n", service, service ? service->name : "");
   if (!service)
     {
	edje_object_part_text_set(gadget, "e.text.name", _("No Connection"));
	edje_object_signal_emit(gadget, "e,changed,service,none", "e");
	return;
     }

   printf("\033[0mDBG CONNMAN: service details:\n"
	  "    state: %s\n"
	  "    type: %s\n"
	  "    error: %s\n"
	  "    security: %s\n"
	  "    strength: %hhu\n"
	  "    flags: favorite=%hhu, auto_connect=%hhu, pass_required=%hhu\033[0m\n",
	  service->state,
	  service->type,
	  service->error,
	  service->security,
	  service->strength,
	  service->favorite,
	  service->auto_connect,
	  service->pass_required);

   if (service->name)
     edje_object_part_text_set(gadget, "e.text.name", service->name);
   else
     edje_object_part_text_set(gadget, "e.text.name", _("Unknown Name"));

   if (service->error)
     {
	edje_object_part_text_set(gadget, "e.text.error", service->error);
	edje_object_signal_emit(gadget, "e,changed,error,yes", "e");
     }
   else
     {
	edje_object_part_text_set(gadget, "e.text.error", _("No error"));
	edje_object_signal_emit(gadget, "e,changed,error,no", "e");
     }

   snprintf(buf, sizeof(buf), "e,changed,service,%s", service->type);
   edje_object_signal_emit(gadget, buf, "e");

   snprintf(buf, sizeof(buf), "e,changed,state,%s", service->state);
   edje_object_signal_emit(gadget, buf, "e");
   printf("DBG CONNMAN signal: %s\n", buf);

   if (service->mode)
     {
	snprintf(buf, sizeof(buf), "e,changed,mode,%s", service->mode);
	edje_object_signal_emit(gadget, buf, "e");
     }

   if (service->security)
     {
	snprintf(buf, sizeof(buf), "e,changed,security,%s", service->security);
	edje_object_signal_emit(gadget, buf, "e");
     }

   if (service->favorite)
     edje_object_signal_emit(gadget, "e,changed,favorite,yes", "e");
   else
     edje_object_signal_emit(gadget, "e,changed,favorite,no", "e");

   if (service->auto_connect)
     edje_object_signal_emit(gadget, "e,changed,auto_connect,yes", "e");
   else
     edje_object_signal_emit(gadget, "e,changed,auto_connect,no", "e");

   if (service->pass_required)
     edje_object_signal_emit(gadget, "e,changed,pass_required,yes", "e");
   else
     edje_object_signal_emit(gadget, "e,changed,pass_required,no", "e");


   msg.val = service->strength;
   edje_object_message_send(gadget, EDJE_MESSAGE_INT, 1, &msg);
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
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _(_Name);
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
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
   GADCON_CLIENT_CLASS_VERSION, _name,
   {
     _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
     e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};



EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _Name};

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
	(_(_Name), _(_lbl_toggle_offline_mode), _act_toggle_offline_mode,
	 NULL, NULL, 0);
   }
}

static void
_connman_actions_unregister(E_Connman_Module_Context *ctxt)
{
   if (ctxt->actions.toggle_offline_mode)
   {
      e_action_predef_name_del(_(_Name), _(_lbl_toggle_offline_mode));
      e_action_del(_act_toggle_offline_mode);
   }
}

static int
_connman_manager_changed_do(void *data)
{
   E_Connman_Module_Context *ctxt = data;

   _connman_services_reload(ctxt);

   ctxt->poller.manager_changed = NULL;
   return 0;
}

static void
_connman_manager_changed(void *data, const E_Connman_Element *element __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);
   ctxt->poller.manager_changed = ecore_poller_add
     (ECORE_POLLER_CORE, 1, _connman_manager_changed_do, ctxt);
}

static int
_connman_event_manager_in(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;
   E_Connman_Element *element;

   ctxt->has_manager = EINA_TRUE;

   element = e_connman_manager_get();
   e_connman_element_listener_add
     (element, _connman_manager_changed, ctxt, NULL);

   _connman_services_reload(ctxt);

   return 1;
}

static int
_connman_event_manager_out(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   E_Connman_Module_Context *ctxt = data;

   ctxt->has_manager = EINA_FALSE;
   eina_stringshare_replace(&ctxt->technology, NULL);

   _connman_services_free(ctxt);
   _connman_default_service_changed(ctxt);

   return 1;
}

static void
_connman_events_register(E_Connman_Module_Context *ctxt)
{
   ctxt->event.manager_in = ecore_event_handler_add
     (E_CONNMAN_EVENT_MANAGER_IN, _connman_event_manager_in, ctxt);
   ctxt->event.manager_out = ecore_event_handler_add
     (E_CONNMAN_EVENT_MANAGER_OUT, _connman_event_manager_out, ctxt);
}

static void
_connman_events_unregister(E_Connman_Module_Context *ctxt)
{
   if (ctxt->event.manager_in)
     ecore_event_handler_del(ctxt->event.manager_in);
   if (ctxt->event.manager_out)
     ecore_event_handler_del(ctxt->event.manager_out);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Connman_Module_Context *ctxt;
   E_DBus_Connection *c;

   e_str_idle = eina_stringshare_add("idle");
   e_str_association = eina_stringshare_add("association");
   e_str_configuration = eina_stringshare_add("configuration");
   e_str_ready = eina_stringshare_add("ready");
   e_str_disconnect = eina_stringshare_add("disconnect");
   e_str_failure = eina_stringshare_add("failure");

   c = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!c)
      return NULL;
   if (!e_connman_system_init(c))
     return NULL;

   ctxt = E_NEW(E_Connman_Module_Context, 1);
   if (!ctxt)
      return NULL;

   _connman_actions_register(ctxt);
   e_gadcon_provider_register(&_gc_class);

   _connman_events_register(ctxt);

   connman_mod = m;
   return ctxt;
}

static void
_connman_instances_free(E_Connman_Module_Context *ctxt)
{
   while (ctxt->instances)
   {
      E_Connman_Instance *inst;

      inst = ctxt->instances->data;
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

   _connman_actions_unregister(ctxt);
   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->poller.default_service_changed)
     ecore_poller_del(ctxt->poller.default_service_changed);
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);

   E_FREE(ctxt);
   connman_mod = NULL;

   e_connman_system_shutdown();

   eina_stringshare_replace(&e_str_idle, NULL);
   eina_stringshare_replace(&e_str_association, NULL);
   eina_stringshare_replace(&e_str_configuration, NULL);
   eina_stringshare_replace(&e_str_ready, NULL);
   eina_stringshare_replace(&e_str_disconnect, NULL);
   eina_stringshare_replace(&e_str_failure, NULL);

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

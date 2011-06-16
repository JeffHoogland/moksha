/*
 * TODO:
 *
 *   HIGH:
 *
 *     - check why return NULL from method call triggers cancel error
 *       after timeout.
 *     - find out why alias == address in _bluez_request_pincode_cb
 *     - more complete agent support (handle requests from devices)
 *     - handle device-disappeared events
 *     - icon with device state (trusted, connected, paired)
 *
 *   LOW:
 *
 *     - configure (probably module) timeout to trigger automatic rescan.
 *     - gadgets to show different adapters (see mixer module configuration)
 *     - module to choose the default adapter (see mixer module configuration)
 *     - icon with device class
 */
#include "e.h"
#include "e_mod_main.h"

static E_Module *bluez_mod = NULL;
static char tmpbuf[PATH_MAX]; /* general purpose buffer, just use immediately */

static const char _e_bluez_agent_path[] = "/org/enlightenment/bluez/Agent";
const char _e_bluez_name[] = "bluez";
const char _e_bluez_Name[] = "Bluetooth Manager";
int _e_bluez_log_dom = -1;

static void _bluez_gadget_update(E_Bluez_Instance *inst);
static void _bluez_tip_update(E_Bluez_Instance *inst);
static void _bluez_popup_update(E_Bluez_Instance *inst);

struct bluez_pincode_data
{
   void                    (*cb)(struct bluez_pincode_data *d);
   DBusMessage            *msg;
   E_Bluez_Module_Context *ctxt;
   char                   *pincode;
   const char             *alias;
   E_Dialog               *dia;
   Evas_Object            *entry;
   Eina_Bool               canceled;
};

const char *
e_bluez_theme_path(void)
{
#define TF "/e-module-bluez.edj"
   size_t dirlen;

   dirlen = strlen(bluez_mod->dir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
     return NULL;

   memcpy(tmpbuf, bluez_mod->dir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static void
_bluez_devices_clear(E_Bluez_Instance *inst)
{
   E_Bluez_Instance_Device *d;
   EINA_LIST_FREE(inst->devices, d)
     {
        eina_stringshare_del(d->address);
        eina_stringshare_del(d->alias);
        free(d);
     }
   inst->address = NULL;
   inst->alias = NULL;
}

static void
_bluez_discovery_cb(void            *data,
                    DBusMessage *msg __UNUSED__,
                    DBusError       *error)
{
   E_Bluez_Instance *inst = data;
   char *label;

   if (error && dbus_error_is_set(error))
     {
        _bluez_dbus_error_show(_("Cannot change adapter's discovery."), error);
        dbus_error_free(error);
        return;
     }

   inst->discovering = !inst->discovering;

   label = !inst->discovering ? _("Start Scan") : _("Stop Scan");
   e_widget_button_label_set(inst->ui.button, label);
}

static void
_bluez_create_paired_device_cb(void            *data,
                               DBusMessage *msg __UNUSED__,
                               DBusError       *error)
{
   const char *alias = data;

   if (error && dbus_error_is_set(error))
     {
        if (strcmp(error->name, "org.bluez.Error.AlreadyExists") != 0)
          _bluez_dbus_error_show(_("Cannot pair with device."), error);
        dbus_error_free(error);
        eina_stringshare_del(alias);
        return;
     }

   e_util_dialog_show
     (_("Bluetooth Manager"), _("Device '%s' successfully paired."), alias);
   eina_stringshare_del(alias);
}

static void
_bluez_toggle_powered_cb(void            *data,
                         DBusMessage *msg __UNUSED__,
                         DBusError       *error)
{
   E_Bluez_Instance *inst = data;

   if ((!error) || (!dbus_error_is_set(error)))
     {
        inst->powered_pending = EINA_FALSE;
        inst->powered = !inst->powered;

        if (!inst->powered)
          {
             _bluez_devices_clear(inst);

             if (inst->popup)
               _bluez_popup_update(inst);
          }

        _bluez_gadget_update(inst);
        return;
     }

   _bluez_dbus_error_show(_("Cannot toggle adapter's powered."), error);
   dbus_error_free(error);
}

void
_bluez_toggle_powered(E_Bluez_Instance *inst)
{
   Eina_Bool powered;

   if ((!inst) || (!inst->ctxt->has_manager))
     {
        _bluez_operation_error_show(_("BlueZ Daemon is not running."));
        return;
     }

   if (!inst->adapter)
     {
        _bluez_operation_error_show(_("No bluetooth adapter."));
        return;
     }

   if (!e_bluez_adapter_powered_get(inst->adapter, &powered))
     {
        _bluez_operation_error_show(_("Query adapter's powered."));
        return;
     }

   powered = !powered;

   if (!e_bluez_adapter_powered_set
         (inst->adapter, powered, _bluez_toggle_powered_cb, inst))
     {
        _bluez_operation_error_show(_("Query adapter's powered."));
        return;
     }
}

static void
_bluez_cb_toggle_powered(E_Object *obj      __UNUSED__,
                         const char *params __UNUSED__)
{
   E_Bluez_Module_Context *ctxt;
   const Eina_List *l;
   E_Bluez_Instance *inst;

   if (!bluez_mod)
     return;

   ctxt = bluez_mod->data;
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     if (inst->adapter) _bluez_toggle_powered(inst);
}

static void _bluez_popup_del(E_Bluez_Instance *inst);

static Eina_Bool
_bluez_popup_input_window_mouse_up_cb(void    *data,
                                      int type __UNUSED__,
                                      void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Bluez_Instance *inst = data;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   _bluez_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_bluez_popup_input_window_key_down_cb(void    *data,
                                      int type __UNUSED__,
                                      void    *event)
{
   Ecore_Event_Key *ev = event;
   E_Bluez_Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   keysym = ev->key;
   if (strcmp(keysym, "Escape") == 0)
     _bluez_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_bluez_popup_input_window_destroy(E_Bluez_Instance *inst)
{
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;

   ecore_event_handler_del(inst->ui.input.mouse_up);
   inst->ui.input.mouse_up = NULL;

   ecore_event_handler_del(inst->ui.input.key_down);
   inst->ui.input.key_down = NULL;
}

static void
_bluez_popup_input_window_create(E_Bluez_Instance *inst)
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
                             _bluez_popup_input_window_mouse_up_cb, inst);

   inst->ui.input.key_down =
     ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                             _bluez_popup_input_window_key_down_cb, inst);

   inst->ui.input.win = w;
}

static void
_bluez_popup_cb_powered_changed(void        *data,
                                Evas_Object *obj)
{
   E_Bluez_Instance *inst = data;
   E_Bluez_Module_Context *ctxt = inst->ctxt;
   Eina_Bool powered = e_widget_check_checked_get(obj);

   if ((!ctxt) || (!ctxt->has_manager))
     {
        _bluez_operation_error_show(_("BlueZ Daemon is not running."));
        return;
     }

   if (!inst->adapter)
     {
        _bluez_operation_error_show(_("No bluetooth adapter."));
        return;
     }

   if (!e_bluez_adapter_powered_set
         (inst->adapter, powered, _bluez_toggle_powered_cb, inst))
     {
        _bluez_operation_error_show
          (_("Cannot toggle adapter's powered."));
        return;
     }

   inst->powered_pending = EINA_TRUE;
}

static void
_bluez_pincode_ask_cb(struct bluez_pincode_data *d)
{
   DBusMessage *reply;

   if (!d->pincode)
     {
        e_util_dialog_show(_("Bluetooth Manager"), _("Invalid Pin Code."));
        return;
     }

   reply = dbus_message_new_method_return(d->msg);
   dbus_message_append_args
     (reply, DBUS_TYPE_STRING, &d->pincode, DBUS_TYPE_INVALID);

   dbus_message_set_no_reply(reply, EINA_TRUE);
   e_dbus_message_send(d->ctxt->agent.conn, reply, NULL, -1, NULL);
}

static void
bluez_pincode_ask_ok(void     *data,
                     E_Dialog *dia)
{
   struct bluez_pincode_data *d = data;
   d->canceled = EINA_FALSE;
   e_object_del(E_OBJECT(dia));
}

static void
bluez_pincode_ask_cancel(void     *data,
                         E_Dialog *dia)
{
   struct bluez_pincode_data *d = data;
   d->canceled = EINA_TRUE;
   e_object_del(E_OBJECT(dia));
}

static void
bluez_pincode_ask_del(void *data)
{
   E_Dialog *dia = data;
   struct bluez_pincode_data *d = e_object_data_get(E_OBJECT(dia));

   if (!d->canceled)
     d->cb(d);

   d->ctxt->agent.pending = eina_list_remove(d->ctxt->agent.pending, dia);

   free(d->pincode);
   dbus_message_unref(d->msg);
   eina_stringshare_del(d->alias);
   E_FREE(d);
}

static void
bluez_pincode_ask_key_down(void          *data,
                           Evas *e        __UNUSED__,
                           Evas_Object *o __UNUSED__,
                           void          *event)
{
   Evas_Event_Key_Down *ev = event;
   struct bluez_pincode_data *d = data;

   if (strcmp(ev->keyname, "Return") == 0)
     bluez_pincode_ask_ok(d, d->dia);
   else if (strcmp(ev->keyname, "Escape") == 0)
     bluez_pincode_ask_cancel(d, d->dia);
}

static void
bluez_pincode_ask(void                    (*cb)(struct bluez_pincode_data *),
                  DBusMessage            *msg,
                  const char             *alias,
                  E_Bluez_Module_Context *ctxt)
{
   struct bluez_pincode_data *d;
   Evas_Object *list, *o;
   Evas *evas;
   char buf[512];
   int mw, mh;

   if (!cb)
     return;

   d = E_NEW(struct bluez_pincode_data, 1);
   if (!d)
     return;

   d->cb = cb;
   d->ctxt = ctxt;
   d->alias = eina_stringshare_add(alias);
   d->msg = dbus_message_ref(msg);
   d->canceled = EINA_TRUE; /* closing the dialog defaults to cancel */
   d->dia = e_dialog_new(NULL, "E", "bluez_ask_pincode");

   snprintf(buf, sizeof(buf), _("Pairing with device '%s'"), alias);
   e_dialog_title_set(d->dia, buf);
   e_dialog_icon_set(d->dia, "dialog-ask", 32);
   e_dialog_border_icon_set(d->dia, "dialog-ask");

   evas = d->dia->win->evas;

   list = e_widget_list_add(evas, 0, 0);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/dialog",
                           "e/widgets/dialog/text");
   snprintf(buf, sizeof(buf),
            _("Enter the PIN code: "));
   edje_object_part_text_set(o, "e.textblock.message", buf);
   edje_object_size_min_calc(o, &mw, &mh);
   evas_object_size_hint_min_set(o, mw, mh);
   evas_object_resize(o, mw, mh);
   evas_object_show(o);
   e_widget_list_object_append(list, o, 1, 1, 0.5);

   d->entry = o = e_widget_entry_add(evas, &d->pincode, NULL, NULL, NULL);
   e_widget_entry_password_set(o, 0);
   evas_object_show(o);
   e_widget_list_object_append(list, o, 1, 0, 0.0);

   e_widget_size_min_get(list, &mw, &mh);
   if (mw < 200)
     mw = 200;
   if (mh < 60)
     mh = 60;
   e_dialog_content_set(d->dia, list, mw, mh);

   e_dialog_button_add
     (d->dia, _("Ok"), NULL, bluez_pincode_ask_ok, d);
   e_dialog_button_add
     (d->dia, _("Cancel"), NULL, bluez_pincode_ask_cancel, d);

   evas_object_event_callback_add
     (d->dia->bg_object, EVAS_CALLBACK_KEY_DOWN,
     bluez_pincode_ask_key_down, d);

   e_object_del_attach_func_set
     (E_OBJECT(d->dia), bluez_pincode_ask_del);
   e_object_data_set(E_OBJECT(d->dia), d);

   e_dialog_button_focus_num(d->dia, 0);
   e_widget_focus_set(d->entry, 1);

   e_win_centered_set(d->dia->win, 1);
   e_dialog_show(d->dia);

   ctxt->agent.pending = eina_list_append(ctxt->agent.pending, d->dia);
}

static DBusMessage *
_bluez_request_pincode_cb(E_DBus_Object *obj,
                          DBusMessage   *msg)
{
   E_Bluez_Module_Context *ctxt = e_dbus_object_data_get(obj);
   E_Bluez_Element *element;
   const char *path;
   const char *alias;

   // TODO: seems that returning NULL is causing pin code rquest to be canceled!

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
                             DBUS_TYPE_INVALID) == FALSE)
     return NULL;

   element = e_bluez_device_get(path);
   if (!element)
     alias = path;
   else
     {
        if (!e_bluez_device_alias_get(element, &alias))
          {
             if (!e_bluez_device_name_get(element, &alias))
               alias = path;
          }
     }
   // TODO: find out why alias == address, then remove debug:
   fprintf(stderr, ">>> request pin code of '%s' (%s)\n", alias, path);
   bluez_pincode_ask(_bluez_pincode_ask_cb, msg, alias, ctxt);
   return NULL;
}

static void
_bluez_popup_cb_scan(void       *data,
                     void *data2 __UNUSED__)
{
   E_Bluez_Instance *inst = data;
   int ret;

   if (!inst->adapter)
     ret = 0;
   else if (inst->discovering)
     ret = e_bluez_adapter_stop_discovery
         (inst->adapter, _bluez_discovery_cb, inst);
   else
     {
        inst->last_scan = ecore_loop_time_get();

        _bluez_devices_clear(inst);

        ret = e_bluez_adapter_start_discovery
            (inst->adapter, _bluez_discovery_cb, inst);

        _bluez_popup_update(inst);
     }

   if (!ret)
     ERR("Failed on discovery procedure");
}

static void
_bluez_popup_cb_controls(void       *data,
                         void *data2 __UNUSED__)
{
   E_Bluez_Instance *inst = data;
   if (inst->popup)
     _bluez_popup_del(inst);
   if (inst->conf_dialog)
     return;
   if (!inst->adapter)
     return;
   inst->conf_dialog = e_bluez_config_dialog_new(NULL, inst);
}

static void
_bluez_popup_device_selected(void *data)
{
   E_Bluez_Instance *inst = data;
   const char *address = inst->address;
   const char *alias;
   const char *cap = "DisplayYesNo";
   const E_Bluez_Instance_Device *d;
   const Eina_List *l;

   if (inst->popup)
     _bluez_popup_del(inst);

   if (!address)
     {
        ERR("no device selected for pairing.");
        return;
     }

   inst->alias = address;
   EINA_LIST_FOREACH(inst->devices, l, d)
     {
        if (address == d->alias)
          {
             inst->alias = d->alias;
             break;
          }
     }

   if (!inst->alias)
     {
        ERR("device %s does not have an alias.", address);
        return;
     }

   alias = eina_stringshare_ref(inst->alias);
   if (!e_bluez_adapter_create_paired_device
         (inst->adapter, _e_bluez_agent_path, cap, address,
         _bluez_create_paired_device_cb, alias))
     {
        eina_stringshare_del(alias);
        return;
     }
}

static Eina_Bool
_bluez_event_devicefound(void       *data,
                         int type    __UNUSED__,
                         void *event)
{
   E_Bluez_Module_Context *ctxt = data;
   E_Bluez_Device_Found *device = event;
   E_Bluez_Instance *inst;
   const Eina_List *l_inst;
   const char *alias;

   // TODO: get properties such as paired, connected, trusted, class, icon...
   // TODO: check if the adapter contains device->name and if so get path.

   alias = e_bluez_devicefound_alias_get(device);

   EINA_LIST_FOREACH(ctxt->instances, l_inst, inst)
     {
        const Eina_List *l_dev;
        E_Bluez_Instance_Device *dev;
        Eina_Bool found = EINA_FALSE;

        if (inst->adapter != device->adapter) continue;

        EINA_LIST_FOREACH(inst->devices, l_dev, dev)
          {
             if (dev->address == device->name)
               {
                  found = EINA_TRUE;
                  break;
               }
          }

        if (found) continue;

        dev = malloc(sizeof(E_Bluez_Instance_Device));
        if (!dev) continue;

        dev->address = eina_stringshare_ref(device->name);
        dev->alias = eina_stringshare_ref(alias);

        inst->devices = eina_list_append(inst->devices, dev);

        if (inst->ui.list)
          {
             e_widget_ilist_append
               (inst->ui.list, NULL, dev->alias,
               _bluez_popup_device_selected, inst, dev->address);
             e_widget_ilist_go(inst->ui.list);
          }
     }

   return 1;
}

static void
_bluez_popup_update(E_Bluez_Instance *inst)
{
   Evas_Object *list = inst->ui.list;
   int selected;
   const char *label;
   E_Bluez_Instance_Device *d;
   Eina_List *l;

   /* TODO: replace this with a scroller + list of edje
    * objects that are more full of features
    */
   selected = e_widget_ilist_selected_get(list);
   e_widget_ilist_freeze(list);
   e_widget_ilist_clear(list);

   EINA_LIST_FOREACH(inst->devices, l, d)
     {
        e_widget_ilist_append
          (inst->ui.list, NULL, d->alias,
          _bluez_popup_device_selected, inst, d->address);
     }

   if (selected >= 0)
     {
        inst->first_selection = EINA_TRUE;
        e_widget_ilist_selected_set(list, selected);
     }
   else
     inst->first_selection = EINA_FALSE;

   e_widget_ilist_go(list);

   e_widget_check_checked_set(inst->ui.powered, inst->powered);
   label = inst->discovering ? _("Stop Scan") : _("Start Scan");
   e_widget_button_label_set(inst->ui.button, label);
   e_widget_disabled_set(inst->ui.button, !inst->powered);
}

static void
_bluez_popup_del(E_Bluez_Instance *inst)
{
   _bluez_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_bluez_popup_new(E_Bluez_Instance *inst)
{
   Evas_Object *ol;
   Evas *evas;
   Evas_Coord mw, mh;
   const char *label;
   Eina_Bool b, needs_scan = EINA_FALSE;

   if (inst->popup)
     {
        e_gadcon_popup_show(inst->popup);
        return;
     }

   if (!inst->adapter)
     {
        _bluez_operation_error_show(_("No bluetooth adapter."));
        return;
     }

   if (!e_bluez_adapter_discovering_get(inst->adapter, &b))
     {
        _bluez_operation_error_show(_("Can't get Discovering property"));
        return;
     }
   inst->discovering = b;
   // maybe auto-scan if did not in the last 30 minutes?
   // seems scan will hurt things like bluetooth audio playback, so don't do it
   if ((!inst->discovering) && (inst->last_scan <= 0.0) && (inst->ui.powered))
     {
        label = _("Stop Scan");
        needs_scan = EINA_TRUE;
     }
   else
     label = inst->discovering ? _("Stop Scan") : _("Start Scan");

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   ol = e_widget_list_add(evas, 0, 0);

   // TODO: get this size from edj
   inst->ui.list = e_widget_ilist_add(evas, 32, 32, &inst->address);
   e_widget_size_min_set(inst->ui.list, 180, 100);
   e_widget_list_object_append(ol, inst->ui.list, 1, 1, 0.5);

   inst->powered = inst->powered;
   inst->ui.powered = e_widget_check_add(evas, _("Powered"), &inst->powered);
   e_widget_on_change_hook_set
     (inst->ui.powered, _bluez_popup_cb_powered_changed, inst);
   e_widget_list_object_append(ol, inst->ui.powered, 1, 0, 0.5);

   inst->ui.button = e_widget_button_add
       (evas, label, NULL, _bluez_popup_cb_scan, inst, NULL);
   e_widget_list_object_append(ol, inst->ui.button, 1, 0, 0.5);

   inst->ui.control = e_widget_button_add
       (evas, _("Controls"), NULL, _bluez_popup_cb_controls, inst, NULL);
   e_widget_list_object_append(ol, inst->ui.control, 1, 0, 0.5);

   _bluez_popup_update(inst);

   e_widget_size_min_get(ol, &mw, &mh);
   if (mh < 200) mh = 200;
   if (mw < 200) mw = 200;
   e_widget_size_min_set(ol, mw, mh);

   e_gadcon_popup_content_set(inst->popup, ol);
   e_gadcon_popup_show(inst->popup);
   _bluez_popup_input_window_create(inst);

   if (needs_scan) _bluez_popup_cb_scan(inst, NULL);
}

static void
_bluez_menu_cb_post(void        *data,
                    E_Menu *menu __UNUSED__)
{
   E_Bluez_Instance *inst = data;
   if ((!inst) || (!inst->menu))
     return;
   if (inst->menu)
     {
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
}

static void
_bluez_menu_cb_cfg(void           *data,
                   E_Menu *menu    __UNUSED__,
                   E_Menu_Item *mi __UNUSED__)
{
   E_Bluez_Instance *inst = data;
   if (inst->popup)
     _bluez_popup_del(inst);
   if (inst->conf_dialog)
     return;
   if (!inst->adapter)
     return;
   inst->conf_dialog = e_bluez_config_dialog_new(NULL, inst);
}

static void
_bluez_menu_new(E_Bluez_Instance      *inst,
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
   e_menu_item_callback_set(mi, _bluez_menu_cb_cfg, inst);

   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_menu_post_deactivate_callback_set(m, _bluez_menu_cb_post, inst);
   inst->menu = m;

   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_bluez_tip_new(E_Bluez_Instance *inst)
{
   Evas *e;

   inst->tip = e_gadcon_popup_new(inst->gcc);
   if (!inst->tip) return;

   e = inst->tip->win->evas;

   inst->o_tip = edje_object_add(e);
   e_theme_edje_object_set(inst->o_tip, "base/theme/modules/bluez/tip",
                           "e/modules/bluez/tip");

   _bluez_tip_update(inst);

   e_gadcon_popup_content_set(inst->tip, inst->o_tip);
   e_gadcon_popup_show(inst->tip);
}

static void
_bluez_tip_del(E_Bluez_Instance *inst)
{
   evas_object_del(inst->o_tip);
   e_object_del(E_OBJECT(inst->tip));
   inst->tip = NULL;
   inst->o_tip = NULL;
}

static void
_bluez_cb_mouse_down(void            *data,
                     Evas *evas       __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void            *event)
{
   E_Bluez_Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   if (!inst)
     return;

   ev = event;
   if (ev->button == 1)
     {
        if (!inst->popup)
          _bluez_popup_new(inst);
        else
          _bluez_popup_del(inst);
     }
   else if (ev->button == 2)
     _bluez_toggle_powered(inst);
   else if ((ev->button == 3) && (!inst->menu))
     _bluez_menu_new(inst, ev);
}

static void
_bluez_cb_mouse_in(void            *data,
                   Evas *evas       __UNUSED__,
                   Evas_Object *obj __UNUSED__,
                   void *event      __UNUSED__)
{
   E_Bluez_Instance *inst = data;

   if (inst->tip)
     return;

   _bluez_tip_new(inst);
}

static void
_bluez_cb_mouse_out(void            *data,
                    Evas *evas       __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event      __UNUSED__)
{
   E_Bluez_Instance *inst = data;

   if (!inst->tip)
     return;

   _bluez_tip_del(inst);
}

static void
_bluez_edje_view_update(E_Bluez_Instance *inst,
                        Evas_Object      *o)
{
   E_Bluez_Module_Context *ctxt = inst->ctxt;
   const char *name;

   if (!ctxt->has_manager)
     {
        edje_object_part_text_set(o, "e.text.powered", "");
        edje_object_part_text_set(o, "e.text.status", "");
        edje_object_signal_emit(o, "e,changed,service,none", "e");
        edje_object_part_text_set(o, "e.text.name", _("No Bluetooth daemon"));
        edje_object_signal_emit(o, "e,changed,name", "e");
        return;
     }

   if (!inst->adapter)
     {
        edje_object_part_text_set(o, "e.text.powered", "");
        edje_object_part_text_set(o, "e.text.status", "");
        edje_object_signal_emit(o, "e,changed,off", "e");
        edje_object_part_text_set(o, "e.text.name", _("No Bluetooth adapter"));
        edje_object_signal_emit(o, "e,changed,name", "e");
        return;
     }

   if (!e_bluez_adapter_name_get(inst->adapter, &name))
     name = "";
   edje_object_part_text_set(o, "e.text.name", name);
   edje_object_signal_emit(o, "e,changed,name", "e");

   if (inst->powered)
     {
        if (inst->discoverable)
          {
             edje_object_signal_emit(o, "e,changed,powered", "e");
             edje_object_part_text_set
               (o, "e.text.status",
               _("Bluetooth is powered and discoverable."));
          }
        else
          {
             edje_object_signal_emit(o, "e,changed,hidden", "e");
             edje_object_part_text_set
               (o, "e.text.status", _("Bluetooth is powered and hidden."));
          }
     }
   else
     {
        edje_object_signal_emit(o, "e,changed,off", "e");
        edje_object_part_text_set(o, "e.text.status", _("Bluetooth is off."));
     }
}

static void
_bluez_tip_update(E_Bluez_Instance *inst)
{
   _bluez_edje_view_update(inst, inst->o_tip);
}

static void
_bluez_gadget_update(E_Bluez_Instance *inst)
{
   E_Bluez_Module_Context *ctxt = inst->ctxt;

   if (inst->popup && ((!ctxt->has_manager) || (!inst->adapter)))
     _bluez_popup_del(inst);

   if (inst->popup)
     _bluez_popup_update(inst);
   if (inst->tip)
     _bluez_tip_update(inst);

   _bluez_edje_view_update(inst, inst->ui.gadget);
}

/* Gadcon Api Functions */

static E_Gadcon_Client *
_gc_init(E_Gadcon   *gc,
         const char *name,
         const char *id,
         const char *style)
{
   E_Bluez_Instance *inst;
   E_Bluez_Module_Context *ctxt;

   if (!bluez_mod)
     return NULL;

   ctxt = bluez_mod->data;

   inst = E_NEW(E_Bluez_Instance, 1);
   inst->ctxt = ctxt;
   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/bluez",
                           "e/modules/bluez/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN, _bluez_cb_mouse_down, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_IN, _bluez_cb_mouse_in, inst);
   evas_object_event_callback_add
     (inst->ui.gadget, EVAS_CALLBACK_MOUSE_OUT, _bluez_cb_mouse_out, inst);

   // TODO: instead of getting the default adapter, get the adapter for
   // each instance. See the mixer module.
   if (ctxt->default_adapter)
     inst->adapter = e_bluez_adapter_get(ctxt->default_adapter);
   else
     inst->adapter = NULL;

   if (inst->adapter)
     {
        Eina_Bool powered, discoverable, discovering;

        if (e_bluez_adapter_powered_get(inst->adapter, &powered))
          inst->powered = powered;

        if (e_bluez_adapter_discoverable_get(inst->adapter, &discoverable))
          inst->discoverable = discoverable;

        if (e_bluez_adapter_discovering_get(inst->adapter, &discovering))
          inst->discovering = discovering;
     }

   _bluez_gadget_update(inst);

   ctxt->instances = eina_list_append(ctxt->instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Bluez_Module_Context *ctxt;
   E_Bluez_Instance *inst;

   if (!bluez_mod)
     return;

   ctxt = bluez_mod->data;
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

   _bluez_devices_clear(inst);

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
   return _(_e_bluez_Name);
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__,
         Evas                               *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_bluez_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   E_Bluez_Module_Context *ctxt;

   if (!bluez_mod)
     return NULL;

   ctxt = bluez_mod->data;
   if (!ctxt)
     return NULL;

   snprintf(tmpbuf, sizeof(tmpbuf), "bluez.%d",
            eina_list_count(ctxt->instances));
   return tmpbuf;
}

static const E_Gadcon_Client_Class _gc_class =
{
   GADCON_CLIENT_CLASS_VERSION, _e_bluez_name,
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _e_bluez_Name};

static const char _act_toggle_powered[] = "toggle_powered";
static const char _lbl_toggle_powered[] = "Toggle Powered";

static void
_bluez_actions_register(E_Bluez_Module_Context *ctxt)
{
   ctxt->actions.toggle_powered = e_action_add(_act_toggle_powered);
   if (ctxt->actions.toggle_powered)
     {
        ctxt->actions.toggle_powered->func.go =
          _bluez_cb_toggle_powered;
        e_action_predef_name_set
          (_(_e_bluez_Name), _(_lbl_toggle_powered), _act_toggle_powered,
          NULL, NULL, 0);
     }
}

static void
_bluez_actions_unregister(E_Bluez_Module_Context *ctxt)
{
   if (ctxt->actions.toggle_powered)
     {
        e_action_predef_name_del(_(_e_bluez_Name), _(_lbl_toggle_powered));
        e_action_del(_act_toggle_powered);
     }
}

static Eina_Bool
_bluez_manager_changed_do(void *data)
{
   E_Bluez_Module_Context *ctxt = data;

   //FIXME: reload the default adapter maybe?

   ctxt->poller.manager_changed = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_bluez_manager_changed(void                          *data,
                       const E_Bluez_Element *element __UNUSED__)
{
   E_Bluez_Module_Context *ctxt = data;
   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);
   ctxt->poller.manager_changed = ecore_poller_add
       (ECORE_POLLER_CORE, 1, _bluez_manager_changed_do, ctxt);
}

static void
_properties_sync_callback(void            *data,
                          DBusMessage *msg __UNUSED__,
                          DBusError       *err)
{
   E_Bluez_Instance *inst = data;
   Eina_Bool powered;
   Eina_Bool discoverable;

   if (err && dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }

   if (!e_bluez_adapter_powered_get(inst->adapter, &powered))
     {
        _bluez_operation_error_show(_("Query adapter's powered."));
        return;
     }

   inst->powered = powered;

   if (!e_bluez_adapter_discoverable_get(inst->adapter, &discoverable))
     {
        _bluez_operation_error_show(_("Query adapter's discoverable."));
        return;
     }

   inst->discoverable = discoverable;
}

static void
_default_adapter_callback(void          *data,
                          DBusMessage   *msg,
                          DBusError *err __UNUSED__)
{
   E_Bluez_Module_Context *ctxt = data;
   const Eina_List *l;
   E_Bluez_Instance *inst;
   const char *path;

   if (err && dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
                             DBUS_TYPE_INVALID) == FALSE)
     return;

   eina_stringshare_replace(&ctxt->default_adapter, path);

   // TODO: instead of getting the default adapter, get the adapter for
   // each instance. See the mixer module.
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        inst->adapter = e_bluez_adapter_get(path);

        e_bluez_element_properties_sync_full
          (inst->adapter, _properties_sync_callback, inst);
     }
}

static Eina_Bool
_bluez_event_manager_in(void       *data,
                        int type    __UNUSED__,
                        void *event __UNUSED__)
{
   E_Bluez_Module_Context *ctxt = data;
   E_Bluez_Element *element;
   Eina_List *l;
   E_Bluez_Instance *inst;

   ctxt->has_manager = EINA_TRUE;

   element = e_bluez_manager_get();
   if (!e_bluez_manager_default_adapter(_default_adapter_callback, ctxt))
     return ECORE_CALLBACK_DONE;

   e_bluez_element_listener_add(element, _bluez_manager_changed, ctxt, NULL);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _bluez_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_bluez_event_manager_out(void       *data,
                         int type    __UNUSED__,
                         void *event __UNUSED__)
{
   E_Bluez_Module_Context *ctxt = data;
   E_Bluez_Instance *inst;
   Eina_List *l;

   ctxt->has_manager = EINA_FALSE;
   eina_stringshare_replace(&ctxt->default_adapter, NULL);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     _bluez_gadget_update(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_bluez_event_element_updated(void       *data,
                             int type    __UNUSED__,
                             void *event __UNUSED__)
{
   E_Bluez_Module_Context *ctxt = data;
   E_Bluez_Element *element = event;
   Eina_Bool powered, discoverable, discovering;
   E_Bluez_Instance *inst;
   Eina_List *l;

   if (!e_bluez_element_is_adapter(element)) return ECORE_CALLBACK_PASS_ON;

   if (!e_bluez_adapter_powered_get(element, &powered))
     powered = EINA_FALSE;

   if (!e_bluez_adapter_discoverable_get(element, &discoverable))
     discoverable = EINA_FALSE;

   if (!e_bluez_adapter_discovering_get(element, &discovering))
     discovering = EINA_FALSE;

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        if (inst->adapter != element) continue;

        inst->powered = powered;
        inst->discoverable = discoverable;
        inst->discovering = discovering;
        _bluez_gadget_update(inst);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_bluez_events_register(E_Bluez_Module_Context *ctxt)
{
   ctxt->event.manager_in = ecore_event_handler_add
       (E_BLUEZ_EVENT_MANAGER_IN, _bluez_event_manager_in, ctxt);
   ctxt->event.manager_out = ecore_event_handler_add
       (E_BLUEZ_EVENT_MANAGER_OUT, _bluez_event_manager_out, ctxt);
   ctxt->event.element_updated = ecore_event_handler_add
       (E_BLUEZ_EVENT_ELEMENT_UPDATED, _bluez_event_element_updated, ctxt);
   ctxt->event.device_found = ecore_event_handler_add
       (E_BLUEZ_EVENT_DEVICE_FOUND, _bluez_event_devicefound, ctxt);

   // TODO: E_BLUEZ_EVENT_DEVICE_DISAPPEARED
}

static void
_bluez_events_unregister(E_Bluez_Module_Context *ctxt)
{
   if (ctxt->event.manager_in)
     ecore_event_handler_del(ctxt->event.manager_in);
   if (ctxt->event.manager_out)
     ecore_event_handler_del(ctxt->event.manager_out);
   if (ctxt->event.device_found)
     ecore_event_handler_del(ctxt->event.device_found);
}

static void
_bluez_agent_register(E_Bluez_Module_Context *ctxt)
{
   E_DBus_Object *o;

   ctxt->agent.iface = e_dbus_interface_new("org.bluez.Agent");
   if (!ctxt->agent.iface)
     return;

   o = e_dbus_object_add(ctxt->agent.conn, _e_bluez_agent_path, ctxt);
   e_dbus_object_interface_attach(o, ctxt->agent.iface);
   e_dbus_interface_method_add
     (ctxt->agent.iface, "RequestPinCode", "o", "s", _bluez_request_pincode_cb);
   // TODO: RequestPasskey
   // TODO: RequestConfirmation
   // TODO: Authorize
   // TODO: DisplayPasskey
   // TODO: ConfirmModeChange
   // TODO: Cancel
   // TODO: Release

   ctxt->agent.obj = o;
}

static void
_bluez_agent_unregister(E_Bluez_Module_Context *ctxt)
{
   E_Object *o;

   EINA_LIST_FREE(ctxt->agent.pending, o)
     e_object_del(o);

   e_dbus_object_interface_detach(ctxt->agent.obj, ctxt->agent.iface);
   e_dbus_object_free(ctxt->agent.obj);
   e_dbus_interface_unref(ctxt->agent.iface);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Bluez_Module_Context *ctxt = E_NEW(E_Bluez_Module_Context, 1);
   if (!ctxt)
     return NULL;

   ctxt->agent.conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if ((!ctxt->agent.conn) || (!e_bluez_system_init(ctxt->agent.conn)))
     goto error_bluez_system_init;

   bluez_mod = m;

   if (_e_bluez_log_dom < 0)
     {
        _e_bluez_log_dom = eina_log_domain_register("ebluez", EINA_COLOR_ORANGE);
        if (_e_bluez_log_dom < 0)
          {
             //EINA_LOG_CRIT("could not register logging domain ebluez");
               goto error_log_domain;
          }
     }

   _bluez_agent_register(ctxt);
   _bluez_actions_register(ctxt);
   e_gadcon_provider_register(&_gc_class);

   _bluez_events_register(ctxt);

   return ctxt;

error_log_domain:
   _e_bluez_log_dom = -1;
   bluez_mod = NULL;
   e_bluez_system_shutdown();
error_bluez_system_init:
   E_FREE(ctxt);
   return NULL;
}

static void
_bluez_instances_free(E_Bluez_Module_Context *ctxt)
{
   E_Bluez_Instance *inst;
   EINA_LIST_FREE(ctxt->instances, inst)
     {
        if (inst->popup)
          _bluez_popup_del(inst);
        if (inst->tip)
          _bluez_tip_del(inst);

        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Bluez_Module_Context *ctxt = m->data;
   E_Bluez_Element *element;

   if (!ctxt)
     return 0;

   element = e_bluez_manager_get();
   e_bluez_element_listener_del(element, _bluez_manager_changed, ctxt);

   _bluez_events_unregister(ctxt);
   _bluez_instances_free(ctxt);

   _bluez_actions_unregister(ctxt);
   _bluez_agent_unregister(ctxt);
   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->poller.manager_changed)
     ecore_poller_del(ctxt->poller.manager_changed);

   eina_stringshare_del(ctxt->default_adapter);

   E_FREE(ctxt);
   bluez_mod = NULL;

   e_bluez_system_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}


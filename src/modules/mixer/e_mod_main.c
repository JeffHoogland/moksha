#include "e_mod_main.h"
#ifdef HAVE_ENOTIFY
#include <E_Notify.h>
#endif

static void      _mixer_popup_timer_new(E_Mixer_Instance *inst);
static Eina_Bool _mixer_popup_timer_cb(void *data);

static E_Module *mixer_mod = NULL;
static char tmpbuf[4096]; /* general purpose buffer, just use immediately */

static const char _conf_domain[] = "module.mixer";
static const char _name[] = "mixer";
const char _e_mixer_Name[] = N_("Mixer");
Eina_Bool _mixer_using_default = EINA_FALSE;
E_Mixer_Volume_Get_Cb e_mod_mixer_volume_get;
E_Mixer_Volume_Set_Cb e_mod_mixer_volume_set;
E_Mixer_Mute_Get_Cb e_mod_mixer_mute_get;
E_Mixer_Mute_Set_Cb e_mod_mixer_mute_set;
E_Mixer_Capture_Cb e_mod_mixer_mutable_get;
E_Mixer_State_Get_Cb e_mod_mixer_state_get;
E_Mixer_Capture_Cb e_mod_mixer_capture_get;
E_Mixer_Cb e_mod_mixer_new;
E_Mixer_Cb e_mod_mixer_del;
E_Mixer_Cb e_mod_mixer_channel_default_name_get;
E_Mixer_Cb e_mod_mixer_channel_get_by_name;
E_Mixer_Cb e_mod_mixer_channel_name_get;
E_Mixer_Cb e_mod_mixer_channel_del;
E_Mixer_Cb e_mod_mixer_channel_free;
E_Mixer_Cb e_mod_mixer_channels_free;
E_Mixer_Cb e_mod_mixer_channels_get;
E_Mixer_Cb e_mod_mixer_channels_names_get;
E_Mixer_Cb e_mod_mixer_card_name_get;
E_Mixer_Cb e_mod_mixer_cards_get;
E_Mixer_Cb e_mod_mixer_cards_free;
E_Mixer_Cb e_mod_mixer_card_default_get;

static void _mixer_actions_unregister(E_Mixer_Module_Context *ctxt);
static void _mixer_actions_register(E_Mixer_Module_Context *ctxt);

static void
_mixer_notify(const float val, E_Mixer_Instance *inst __UNUSED__)
{
#ifdef HAVE_ENOTIFY
   E_Notification *n;
   E_Mixer_Module_Context *ctxt;
   char *icon, buf[56];
   int ret;

   if (val > 100.0 || val < 0.0)
     return;

   if (!(ctxt = (E_Mixer_Module_Context *)mixer_mod->data) || !ctxt->desktop_notification)
     return;

   ret = snprintf(buf, (sizeof(buf) - 1), "%s: %d%%", _("New volume"), (int)(val + 0.5));
   if ((ret < 0) || ((unsigned int)ret > sizeof(buf)))
     return;
   //Names are taken from FDO icon naming scheme
   if (val == 0.0)
     icon = "audio-volume-muted";
   else if ((val > 33.3) && (val < 66.6))
     icon = "audio-volume-medium";
   else if (val < 33.3)
     icon = "audio-volume-low";
   else
     icon = "audio-volume-high";

   n = e_notification_full_new(_("Mixer"), 0, icon, _("Volume changed"), buf, 2000);
   e_notification_replaces_id_set(n, EINA_TRUE);
   e_notification_send(n, NULL, NULL);
   e_notification_unref(n);
#endif
}

const char *
e_mixer_theme_path(void)
{
#define TF "/e-module-mixer.edj"
   size_t dirlen;

   dirlen = strlen(mixer_mod->dir);
   if (dirlen >= sizeof(tmpbuf) - sizeof(TF))
     return NULL;

   memcpy(tmpbuf, mixer_mod->dir, dirlen);
   memcpy(tmpbuf + dirlen, TF, sizeof(TF));

   return tmpbuf;
#undef TF
}

static int
_mixer_gadget_configuration_defaults(E_Mixer_Gadget_Config *conf)
{
   E_Mixer_System *sys;
   const char *card, *channel;

   card = e_mod_mixer_card_default_get();
   if (!card)
     return 0;

   sys = e_mod_mixer_new(card);
   if (!sys)
     {
        eina_stringshare_del(card);
        return 0;
     }

   channel = e_mod_mixer_channel_default_name_get(sys);
   e_mod_mixer_del(sys);

   if (!channel)
     {
        eina_stringshare_del(card);
        return 0;
     }

   eina_stringshare_del(conf->card);
   conf->card = card;
   eina_stringshare_del(conf->channel_name);
   conf->channel_name = channel;
   conf->lock_sliders = 1;
   conf->show_locked = 0;
   conf->keybindings_popup = 0;
   conf->state.left = conf->state.right = conf->state.mute = -1;

   return 1;
}

static E_Mixer_Gadget_Config *
_mixer_gadget_configuration_new(E_Mixer_Module_Config *mod_conf, const char *id)
{
   E_Mixer_Gadget_Config *conf;

   conf = E_NEW(E_Mixer_Gadget_Config, 1);
   if (!conf)
     return NULL;

   _mixer_gadget_configuration_defaults(conf);
   conf->id = eina_stringshare_add(id);
   if (!mod_conf->gadgets)
     mod_conf->gadgets = eina_hash_string_superfast_new(NULL);
   eina_hash_direct_add(mod_conf->gadgets, conf->id, conf);

   return conf;
}

static inline void
_mixer_gadget_configuration_free_int(E_Mixer_Gadget_Config *conf)
{
   if (conf->dialog)
     e_object_del(E_OBJECT(conf->dialog));

   if (conf->card)
     eina_stringshare_del(conf->card);
   if (conf->channel_name)
     eina_stringshare_del(conf->channel_name);

   eina_stringshare_del(conf->id);
   free(conf);
}

static void
_mixer_gadget_configuration_free(E_Mixer_Module_Config *mod_conf, E_Mixer_Gadget_Config *conf)
{
   if (!mod_conf)
     return;
   if (!conf)
     return;
   eina_hash_del(mod_conf->gadgets, conf->id, conf);
   if (!eina_hash_population(mod_conf->gadgets))
     eina_hash_free(mod_conf->gadgets);
   _mixer_gadget_configuration_free_int(conf);
}

static Eina_Bool
_mixer_gadget_configuration_free_foreach(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *hdata, void *fdata __UNUSED__)
{
   _mixer_gadget_configuration_free_int(hdata);
   return 1;
}

#if 0
static Eina_Bool
_mixer_module_configuration_alert(void *data)
{
   e_util_dialog_show(_("Mixer Settings Updated"), "%s", (char *)data);
   return ECORE_CALLBACK_CANCEL;
}

#endif

static E_Mixer_Module_Config *
_mixer_module_configuration_new(void)
{
   E_Mixer_Module_Config *conf;

   conf = E_NEW(E_Mixer_Module_Config, 1);
   conf->desktop_notification = 1;

   return conf;
}

static void
_mixer_module_configuration_free(E_Mixer_Module_Config *conf)
{
   if (!conf)
     return;

   if (conf->gadgets)
     {
        eina_hash_foreach(conf->gadgets,
                          _mixer_gadget_configuration_free_foreach, NULL);
        eina_hash_free(conf->gadgets);
     }
   eina_stringshare_del(conf->default_gc_id);
   free(conf);
}

static void
_mixer_popup_update(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   if (!inst->popup) return;

   state = &inst->mixer_state;

   if (inst->ui.left)
     e_slider_value_set(inst->ui.left, state->left);
   if (inst->ui.right)
     e_slider_value_set(inst->ui.right, state->right);
   if (inst->ui.mute)
     e_widget_check_checked_set(inst->ui.mute, state->mute);
}

static void
_mixer_gadget_update(E_Mixer_Instance *inst)
{
   Edje_Message_Int_Set *msg;

   if (!inst)
     return;

   msg = alloca(sizeof(Edje_Message_Int_Set) + (2 * sizeof(int)));
   msg->count = 3;
   msg->val[0] = inst->mixer_state.mute;
   msg->val[1] = inst->mixer_state.left;
   msg->val[2] = inst->mixer_state.right;
   edje_object_message_send(inst->ui.gadget, EDJE_MESSAGE_INT_SET, 0, msg);

   edje_object_signal_emit(inst->ui.gadget, "e,action,volume,change", "e");

   if (inst->popup)
     _mixer_popup_update(inst);
}

static void
_mixer_balance_left(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);
   if (state->left >= 0)
     {
        if (state->left > 5)
          state->left -= 5;
        else
          state->left = 0;
     }
   if (state->right >= 0)
     {
        if (state->right < 95)
          state->right += 5;
        else
          state->right = 100;
     }

   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_balance_right(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);
   if (state->left >= 0)
     {
        if (state->left < 95)
          state->left += 5;
        else
          state->left = 100;
     }
   if (state->right >= 0)
     {
        if (state->right > 5)
          state->right -= 5;
        else
          state->right = 0;
     }
   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_volume_increase(E_Mixer_Instance *inst, Eina_Bool non_ui)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);
   if (state->left >= 0)
     {
        if (state->left < 95)
          state->left += 5;
        else
          state->left = 100;
     }

   if (state->right >= 0)
     {
        if (state->right < 95)
          state->right += 5;
        else
          state->right = 100;
     }

   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   _mixer_gadget_update(inst);
   if (non_ui)
     _mixer_notify(((float)state->left + (float)state->right) / 2.0, inst);
}

static void
_mixer_volume_decrease(E_Mixer_Instance *inst, Eina_Bool non_ui)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);
   if (state->left >= 0)
     {
        if (state->left > 5)
          state->left -= 5;
        else
          state->left = 0;
     }
   if (state->right >= 0)
     {
        if (state->right > 5)
          state->right -= 5;
        else
          state->right = 0;
     }

   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   _mixer_gadget_update(inst);
   if (non_ui)
     _mixer_notify(((float)state->left + (float)state->right) / 2.0, inst);
}

static void
_mixer_toggle_mute(E_Mixer_Instance *inst, Eina_Bool non_ui)
{
   E_Mixer_Channel_State *state;

   if (!e_mod_mixer_mutable_get(inst->sys, inst->channel))
     return;

   state = &inst->mixer_state;
   e_mod_mixer_mute_get(inst->sys, inst->channel, &state->mute);
   state->mute = !state->mute;
   e_mod_mixer_mute_set(inst->sys, inst->channel, state->mute);
   if (!state->mute) e_mod_mixer_volume_set(inst->sys, inst->channel, state->left, state->right);
   _mixer_gadget_update(inst);
   if (non_ui)
     {
        if (state->mute)
          _mixer_notify(0.0, inst);
        else
          _mixer_notify(((float)state->left + (float)state->right) / 2.0, inst);
     }
}

static void
_mixer_popup_cb_volume_left_change(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   E_Mixer_Channel_State *state = &inst->mixer_state;

   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);

   state->left = (int)e_slider_value_get(obj);
   if (inst->conf->lock_sliders)
     {
        state->right = state->left;
        e_slider_value_set(inst->ui.right, state->right);
     }

   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   if (!_mixer_using_default) _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_volume_right_change(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   E_Mixer_Channel_State *state = &inst->mixer_state;

   e_mod_mixer_volume_get(inst->sys, inst->channel,
                          &state->left, &state->right);

   state->right = (int)e_slider_value_get(obj);
   if (inst->conf->lock_sliders)
     {
        state->left = state->right;
        e_slider_value_set(inst->ui.left, state->left);
     }

   e_mod_mixer_volume_set(inst->sys, inst->channel,
                          state->left, state->right);
   if (!_mixer_using_default) _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_mute_change(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   E_Mixer_Channel_State *state = &inst->mixer_state;

   state->mute = e_widget_check_checked_get(obj);
   e_mod_mixer_mute_set(inst->sys, inst->channel, state->mute);

   if (!_mixer_using_default) _mixer_gadget_update(inst);
}

static Evas_Object *
_mixer_popup_add_slider(E_Mixer_Instance *inst, int value, void (*cb)(void *data, Evas_Object *obj, void *event_info))
{
   Evas_Object *slider = e_slider_add(inst->popup->win->evas);
   evas_object_show(slider);
   e_slider_orientation_set(slider, 0);
   e_slider_value_set(slider, value);
   e_slider_value_range_set(slider, 0.0, 100.0);
   e_slider_value_format_display_set(slider, NULL);
   evas_object_smart_callback_add(slider, "changed", cb, inst);

   return slider;
}

static void
_mixer_app_cb_del(E_Dialog *dialog __UNUSED__, void *data)
{
   E_Mixer_Module_Context *ctxt = data;
   ctxt->mixer_dialog = NULL;
}

static void _mixer_popup_del(E_Mixer_Instance *inst);

static Eina_Bool
_mixer_popup_input_window_mouse_up_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   E_Mixer_Instance *inst = data;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   _mixer_popup_del(inst);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_mixer_popup_input_window_key_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   E_Mixer_Instance *inst = data;
   const char *keysym;

   if (ev->window != inst->ui.input.win)
     return ECORE_CALLBACK_PASS_ON;

   keysym = ev->key;
   if (strcmp(keysym, "Escape") == 0)
     _mixer_popup_del(inst);
   else if (strcmp(keysym, "Up") == 0)
     _mixer_volume_increase(inst, EINA_FALSE);
   else if (strcmp(keysym, "Down") == 0)
     _mixer_volume_decrease(inst, EINA_FALSE);
   else if ((strcmp(keysym, "Return") == 0) ||
            (strcmp(keysym, "KP_Enter") == 0))
     _mixer_toggle_mute(inst, EINA_FALSE);
   else
     {
        E_Action *act;
        Eina_List *l;
        E_Config_Binding_Key *binding;
        E_Binding_Modifier mod;
        Eina_Bool handled = EINA_FALSE;

        EINA_LIST_FOREACH(e_config->key_bindings, l, binding)
          {
             if (binding->action &&
                 (strcmp(binding->action, "volume_increase") &&
                  strcmp(binding->action, "volume_decrease") &&
                  strcmp(binding->action, "volume_mute")))
               continue;

             mod = 0;

             if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
               mod |= E_BINDING_MODIFIER_SHIFT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
               mod |= E_BINDING_MODIFIER_CTRL;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
               mod |= E_BINDING_MODIFIER_ALT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
               mod |= E_BINDING_MODIFIER_WIN;

             if (binding->key && (!strcmp(binding->key, ev->key)) &&
                 ((binding->modifiers == mod) || (binding->any_mod)))
               {
                  if (!(act = e_action_find(binding->action))) continue;
                  if (act->func.go_key)
                    act->func.go_key(E_OBJECT(inst->gcc->gadcon->zone), binding->params, ev);
                  else if (act->func.go)
                    act->func.go(E_OBJECT(inst->gcc->gadcon->zone), binding->params);
                  handled = EINA_TRUE;
               }
          }
        if (!handled) _mixer_popup_del(inst);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_mixer_popup_input_window_destroy(E_Mixer_Instance *inst)
{
   e_grabinput_release(0, inst->ui.input.win);
   ecore_x_window_free(inst->ui.input.win);
   inst->ui.input.win = 0;

   ecore_event_handler_del(inst->ui.input.mouse_up);
   inst->ui.input.mouse_up = NULL;

   ecore_event_handler_del(inst->ui.input.key_down);
   inst->ui.input.key_down = NULL;
}

static void
_mixer_popup_input_window_create(E_Mixer_Instance *inst)
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
                             _mixer_popup_input_window_mouse_up_cb, inst);

   inst->ui.input.key_down =
     ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                             _mixer_popup_input_window_key_down_cb, inst);

   inst->ui.input.win = w;
   e_grabinput_get(0, 0, inst->ui.input.win);
}

static void
_mixer_popup_del(E_Mixer_Instance *inst)
{
   _mixer_popup_input_window_destroy(inst);
   e_object_del(E_OBJECT(inst->popup));
   inst->ui.label = NULL;
   inst->ui.left = NULL;
   inst->ui.right = NULL;
   inst->ui.mute = NULL;
   inst->ui.table = NULL;
   inst->ui.button = NULL;
   inst->popup = NULL;
   if (inst->popup_timer)
     ecore_timer_del(inst->popup_timer);
   inst->popup_timer = NULL;
}

static void
_mixer_app_select_current(E_Dialog *dialog, E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf = inst->conf;

   e_mixer_app_dialog_select(dialog, conf->card, conf->channel_name);
}

static void
_mixer_popup_cb_mixer(void *data, void *data2 __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   E_Mixer_Module_Context *ctxt;
   E_Container *con;

   _mixer_popup_del(inst);

   ctxt = mixer_mod->data;
   if (ctxt->mixer_dialog)
     {
        _mixer_app_select_current(ctxt->mixer_dialog, inst);
        e_dialog_show(ctxt->mixer_dialog);
        return;
     }

   con = e_container_current_get(e_manager_current_get());
   ctxt->mixer_dialog = e_mixer_app_dialog_new(con, _mixer_app_cb_del, ctxt);

   _mixer_app_select_current(ctxt->mixer_dialog, inst);
}

static void
_mixer_popup_new(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;
   Evas *evas;
   Evas_Coord mw, mh;
   int colspan;

   if (inst->conf->dialog)
     return;

   state = &inst->mixer_state;
   e_mod_mixer_state_get(inst->sys, inst->channel, state);

   if ((state->right >= 0) &&
       (inst->conf->show_locked || (!inst->conf->lock_sliders)))
     colspan = 2;
   else
     colspan = 1;

   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;

   inst->ui.table = e_widget_table_add(evas, 0);

   inst->ui.label = e_widget_label_add(evas, inst->conf->channel_name);
   e_widget_table_object_append(inst->ui.table, inst->ui.label,
                                0, 0, colspan, 1, 0, 0, 0, 0);

   if (state->left >= 0)
     {
        inst->ui.left = _mixer_popup_add_slider(
            inst, state->left, _mixer_popup_cb_volume_left_change);
        e_widget_table_object_append(inst->ui.table, inst->ui.left,
                                     0, 1, 1, 1, 1, 1, 1, 1);
     }
   else
     inst->ui.left = NULL;

   if ((state->right >= 0) &&
       (inst->conf->show_locked || (!inst->conf->lock_sliders)))
     {
        inst->ui.right = _mixer_popup_add_slider(
            inst, state->right, _mixer_popup_cb_volume_right_change);
        e_widget_table_object_append(inst->ui.table, inst->ui.right,
                                     1, 1, 1, 1, 1, 1, 1, 1);
     }
   else
     inst->ui.right = NULL;

   if (e_mod_mixer_mutable_get(inst->sys, inst->channel))
     {
        inst->ui.mute = e_widget_check_add(evas, _("Mute"), &state->mute);
        evas_object_show(inst->ui.mute);
        e_widget_table_object_append(inst->ui.table, inst->ui.mute,
                                     0, 2, colspan, 1, 1, 1, 1, 0);
        evas_object_smart_callback_add(inst->ui.mute, "changed",
                                       _mixer_popup_cb_mute_change, inst);
     }
   else
     inst->ui.mute = NULL;

   inst->ui.button = e_widget_button_add(evas, NULL, "preferences-system",
                                         _mixer_popup_cb_mixer, inst, NULL);
   e_widget_table_object_append(inst->ui.table, inst->ui.button,
                                0, 7, colspan, 1, 1, 1, 1, 0);

   e_widget_size_min_get(inst->ui.table, &mw, &mh);
   if (mh < 208) mh = 208;
   e_widget_size_min_set(inst->ui.table, mw, mh);

   e_gadcon_popup_content_set(inst->popup, inst->ui.table);
   e_gadcon_popup_show(inst->popup);
   _mixer_popup_input_window_create(inst);
}

static void
_mixer_popup_timer_new(E_Mixer_Instance *inst)
{
   if (inst->popup)
     {
        if (inst->popup_timer)
          {
             ecore_timer_del(inst->popup_timer);
             inst->popup_timer = ecore_timer_add(1.0, _mixer_popup_timer_cb, inst);
          }
     }
   else
     {
        _mixer_popup_new(inst);
        inst->popup_timer = ecore_timer_add(1.0, _mixer_popup_timer_cb, inst);
     }
}

static Eina_Bool
_mixer_popup_timer_cb(void *data)
{
   E_Mixer_Instance *inst;
   inst = data;

   if (inst->popup)
     _mixer_popup_del(inst);
   inst->popup_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_mixer_menu_cb_cfg(void *data, E_Menu *menu __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   E_Container *con;

   if (inst->popup)
     _mixer_popup_del(inst);
   con = e_container_current_get(e_manager_current_get());
   inst->conf->dialog = e_mixer_config_dialog_new(con, inst->conf);
}

static void
_mixer_menu_new(E_Mixer_Instance *inst, Evas_Event_Mouse_Down *ev)
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
   e_menu_item_callback_set(mi, _mixer_menu_cb_cfg, inst);

   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                         1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                            EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_mixer_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   E_Mixer_Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;

   if (ev->button == 1)
     {
        if (!inst->popup)
          _mixer_popup_new(inst);
        else
          _mixer_popup_del(inst);
     }
   else if (ev->button == 2)
     _mixer_toggle_mute(inst, EINA_FALSE);
   else if (ev->button == 3)
     _mixer_menu_new(inst, ev);
}

static void
_mixer_cb_mouse_wheel(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   E_Mixer_Instance *inst = data;
   Evas_Event_Mouse_Wheel *ev = event;

   if (ev->direction == 0)
     {
        if (ev->z > 0)
          _mixer_volume_decrease(inst, EINA_FALSE);
        else if (ev->z < 0)
          _mixer_volume_increase(inst, EINA_FALSE);
     }
   else if (_mixer_using_default && (ev->direction == 1)) /* invalid with pulse */
     {
        if (ev->z > 0)
          _mixer_balance_left(inst);
        else if (ev->z < 0)
          _mixer_balance_right(inst);
     }
}

static int
_mixer_sys_setup(E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf;

   conf = inst->conf;
   if ((!_mixer_using_default) && (!e_mixer_pulse_ready())) return 1;

   if (!conf->card)
     {
        ERR("conf->card in mixer sys setup is NULL");
        return 1;
     }
   
   if (inst->sys)
     e_mod_mixer_del(inst->sys);

   inst->sys = e_mod_mixer_new(conf->card);
   if (!inst->sys)
     {
        inst->channel = NULL;
        return 0;
     }

   inst->channel = e_mod_mixer_channel_get_by_name(inst->sys, conf->channel_name);
   return !!inst->channel;
}

static int
_mixer_system_cb_update(void *data, E_Mixer_System *sys __UNUSED__)
{
   E_Mixer_Instance *inst = data;
   e_mod_mixer_state_get(inst->sys, inst->channel, &inst->mixer_state);
   _mixer_gadget_update(inst);

   return 1;
}

int
e_mixer_update(E_Mixer_Instance *inst)
{
   int r;

   e_modapi_save(mixer_mod);
   if ((!inst) || (!inst->conf))
     return 0;

   r = _mixer_sys_setup(inst);
   if (r && _mixer_using_default)
     e_mixer_system_callback_set(inst->sys, _mixer_system_cb_update, inst);

   return r;
}

static int
_mixer_sys_setup_default_card(E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf;
   const char *card;

   conf = inst->conf;
   conf->using_default = EINA_TRUE;
   eina_stringshare_del(conf->card);

   card = e_mod_mixer_card_default_get();
   if (!card)
     goto error;

   inst->sys = e_mod_mixer_new(card);
   if (!inst->sys)
     goto system_error;

   conf->card = card;
   return 1;

system_error:
   eina_stringshare_del(card);
error:
   conf->card = NULL;
   return 0;
}

static int
_mixer_sys_setup_default_channel(E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf;
   const char *channel_name;

   conf = inst->conf;
   if (conf->channel_name)
     eina_stringshare_del(conf->channel_name);

   channel_name = e_mod_mixer_channel_default_name_get(inst->sys);
   if (!channel_name)
     goto error;

   inst->channel = e_mod_mixer_channel_get_by_name(inst->sys, channel_name);
   if (!inst->channel)
     goto system_error;

   conf->channel_name = channel_name;
   return 1;

system_error:
   eina_stringshare_del(channel_name);
error:
   conf->channel_name = NULL;
   return 0;
}

static int
_mixer_sys_setup_defaults(E_Mixer_Instance *inst)
{
   if ((!_mixer_using_default) && (!e_mixer_pulse_ready())) return 1;
   if ((!inst->sys) && (!_mixer_sys_setup_default_card(inst)))
     return 0;

   return _mixer_sys_setup_default_channel(inst);
}

void
e_mod_mixer_pulse_ready(Eina_Bool ready)
{
   E_Mixer_Instance *inst;
   E_Mixer_Module_Context *ctxt;
   Eina_List *l;
   Eina_Bool pulse = !_mixer_using_default;
   static Eina_Bool called = EINA_FALSE;

   if (!mixer_mod) return;

   if (called && (ready != _mixer_using_default)) return; // prevent multiple calls
   ctxt = mixer_mod->data;
   if (pulse != _mixer_using_default)
     {
        EINA_LIST_FOREACH(ctxt->instances, l, inst)
          {
             e_mod_mixer_channel_del(inst->channel);
             e_mod_mixer_del(inst->sys);
             inst->channel = NULL;
             inst->sys = NULL;
          }
     }
   if (ready) e_mixer_pulse_setup();
   else e_mixer_default_setup();

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        if (pulse != _mixer_using_default)
          _mixer_gadget_configuration_defaults(inst->conf);
        if ((!_mixer_sys_setup(inst)) && (!_mixer_sys_setup_defaults(inst)))
          {
             if (inst->sys)
               e_mod_mixer_del(inst->sys);
             inst->sys = NULL;
             return;
          }
        if (_mixer_using_default) e_mixer_system_callback_set(inst->sys, _mixer_system_cb_update, inst);
        else e_mixer_system_callback_set(inst->sys, NULL, NULL);
        if ((inst->mixer_state.left > -1) && (inst->mixer_state.right > -1) && (inst->mixer_state.mute > -1))
          e_mod_mixer_volume_set(inst->sys, inst->channel,
                                 inst->mixer_state.left, inst->mixer_state.right);
        else
          e_mod_mixer_state_get(inst->sys, inst->channel, &inst->mixer_state);
        _mixer_gadget_update(inst);
     }
   called = EINA_TRUE;
}

void
e_mod_mixer_pulse_update(void)
{
   E_Mixer_Instance *inst;
   E_Mixer_Module_Context *ctxt;
   Eina_List *l;

   if (!mixer_mod) return;

   ctxt = mixer_mod->data;
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        if (inst->conf->using_default) 
          _mixer_sys_setup_default_card(inst);
        e_mod_mixer_state_get(inst->sys, inst->channel, &inst->mixer_state);
        _mixer_gadget_update(inst);
     }
}

/* Gadcon Api Functions */
static void _mixer_module_configuration_setup(E_Mixer_Module_Context *ctxt);

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   E_Mixer_Instance *inst;
   E_Mixer_Module_Context *ctxt;
   E_Mixer_Gadget_Config *conf;

   if (!mixer_mod)
     return NULL;

   ctxt = mixer_mod->data;
   _mixer_actions_register(ctxt);
   if (!ctxt->conf)
     {
        _mixer_module_configuration_setup(ctxt);
        if (!ctxt->conf)
          return NULL;
     }

   conf = eina_hash_find(ctxt->conf->gadgets, id);
   if (!conf)
     {
        conf = _mixer_gadget_configuration_new(ctxt->conf, id);
        if (!conf)
          return NULL;
     }

   inst = E_NEW(E_Mixer_Instance, 1);
   inst->conf = conf;
   inst->mixer_state.right = inst->conf->state.right;
   inst->mixer_state.left = inst->conf->state.left;
   inst->mixer_state.mute = inst->conf->state.mute;
   conf->instance = inst;
   if ((!_mixer_sys_setup(inst)) && (!_mixer_sys_setup_defaults(inst)))
     {
        if (inst->sys)
          e_mod_mixer_del(inst->sys);
        _mixer_gadget_configuration_free(ctxt->conf, conf);
        E_FREE(inst);
        return NULL;
     }

   if (_mixer_using_default) e_mixer_system_callback_set(inst->sys, _mixer_system_cb_update, inst);

   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/mixer",
                           "e/modules/mixer/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mixer_cb_mouse_down, inst);
   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOUSE_WHEEL,
                                  _mixer_cb_mouse_wheel, inst);

   if (inst->sys)
     {
        if (_mixer_using_default &&
             ((inst->mixer_state.left > -1) && (inst->mixer_state.right > -1) && (inst->mixer_state.mute > -1)))
          e_mod_mixer_volume_set(inst->sys, inst->channel,
                                 inst->mixer_state.left, inst->mixer_state.right);
        else
          e_mod_mixer_state_get(inst->sys, inst->channel, &inst->mixer_state);
        _mixer_gadget_update(inst);
     }

   if (!ctxt->conf->default_gc_id)
     {
        ctxt->conf->default_gc_id = eina_stringshare_add(id);
        ctxt->default_instance = inst;
     }
   else if ((!ctxt->default_instance) ||
            (strcmp(id, ctxt->conf->default_gc_id) == 0))
     ctxt->default_instance = inst;

   ctxt->instances = eina_list_append(ctxt->instances, inst);

   return inst->gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   E_Mixer_Module_Context *ctxt;
   E_Mixer_Instance *inst;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt)
     return;

   inst = gcc->data;
   if (!inst)
     return;

   inst->conf->state.mute = inst->mixer_state.mute;
   inst->conf->state.left = inst->mixer_state.left;
   inst->conf->state.right = inst->mixer_state.right;
   evas_object_del(inst->ui.gadget);
   e_mod_mixer_channel_del(inst->channel);
   e_mod_mixer_del(inst->sys);

   inst->conf->instance = NULL;
   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   if (ctxt->default_instance == inst)
     {
        ctxt->default_instance = NULL;
        _mixer_actions_unregister(ctxt);
     }

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
   return (char *)_(_e_mixer_Name);
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o = edje_object_add(evas);
   edje_object_file_set(o, e_mixer_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   E_Mixer_Module_Context *ctxt;
   Eina_List *instances;

   if (!mixer_mod)
     return NULL;

   ctxt = mixer_mod->data;
   if (!ctxt)
     return NULL;

   instances = ctxt->instances;
   snprintf(tmpbuf, sizeof(tmpbuf), "mixer.%d", eina_list_count(instances));
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

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _e_mixer_Name};

static void
_mixer_cb_volume_increase(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (!ctxt->default_instance)
     return;

   if (ctxt->default_instance->conf->keybindings_popup)
     _mixer_popup_timer_new(ctxt->default_instance);
   _mixer_volume_increase(ctxt->default_instance, EINA_TRUE);
}

static void
_mixer_cb_volume_decrease(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (!ctxt->default_instance)
     return;

   if (ctxt->default_instance->conf->keybindings_popup)
     _mixer_popup_timer_new(ctxt->default_instance);
   _mixer_volume_decrease(ctxt->default_instance, EINA_TRUE);
}

static void
_mixer_cb_volume_mute(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (!ctxt->default_instance)
     return;

   if (ctxt->default_instance->conf->keybindings_popup)
     _mixer_popup_timer_new(ctxt->default_instance);
   _mixer_toggle_mute(ctxt->default_instance, EINA_TRUE);
}

static E_Config_Dialog *
_mixer_module_config(E_Container *con, const char *params __UNUSED__)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return NULL;

   ctxt = mixer_mod->data;
   if (!ctxt)
     return NULL;

   if (ctxt->conf_dialog)
     return NULL;

   if (!ctxt->conf)
     {
        _mixer_module_configuration_setup(ctxt);
        if (!ctxt->conf)
          return NULL;
     }

   ctxt->conf_dialog = e_mixer_config_module_dialog_new(con, ctxt);
   return ctxt->conf_dialog;
}

static const char _reg_cat[] = "extensions";
static const char _reg_item[] = "extensions/mixer";

static void
_mixer_configure_registry_register(void)
{
   e_configure_registry_category_add(_reg_cat, 90, _("Extensions"), NULL,
                                     "preferences-extensions");
   e_configure_registry_item_add(_reg_item, 30, _(_e_mixer_Name), NULL,
                                 "preferences-desktop-mixer",
                                 _mixer_module_config);
}

static void
_mixer_configure_registry_unregister(void)
{
   e_configure_registry_item_del(_reg_item);
   e_configure_registry_category_del(_reg_cat);
}

static E_Config_DD *
_mixer_module_configuration_descriptor_new(E_Config_DD *gadget_conf_edd)
{
   E_Config_DD *conf_edd;

   conf_edd = E_CONFIG_DD_NEW("Mixer_Module_Config", E_Mixer_Module_Config);
   if (!conf_edd)
     return NULL;
   E_CONFIG_VAL(conf_edd, E_Mixer_Module_Config, version, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Module_Config, default_gc_id, STR);
   E_CONFIG_HASH(conf_edd, E_Mixer_Module_Config, gadgets, gadget_conf_edd);
   E_CONFIG_VAL(conf_edd, E_Mixer_Module_Config, desktop_notification, INT);

   return conf_edd;
}

static inline void
_mixer_module_configuration_descriptor_free(E_Config_DD *conf_edd)
{
   if (!conf_edd)
     return;
   E_CONFIG_DD_FREE(conf_edd);
}

static E_Config_DD *
_mixer_gadget_configuration_descriptor_new(void)
{
   E_Config_DD *conf_edd;

   conf_edd = E_CONFIG_DD_NEW("Mixer_Gadget_Config", E_Mixer_Gadget_Config);
   if (!conf_edd)
     return NULL;
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, lock_sliders, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, show_locked, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, keybindings_popup, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, card, STR);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, channel_name, STR);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, using_default, UCHAR);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, state.mute, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, state.left, INT);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, state.right, INT);

   return conf_edd;
}

static inline void
_mixer_gadget_configuration_descriptor_free(E_Config_DD *conf_edd)
{
   if (!conf_edd)
     return;
   E_CONFIG_DD_FREE(conf_edd);
}

static E_Mixer_Module_Config *
_mixer_module_configuration_load(E_Config_DD *module_conf_edd)
{
   E_Mixer_Module_Config *conf;

   conf = e_config_domain_load(_conf_domain, module_conf_edd);

   if (!conf)
     return _mixer_module_configuration_new();

   if (conf && !e_util_module_config_check(_("Mixer Module"), conf->version,
                                           MOD_CONFIG_FILE_VERSION))
     {
        _mixer_module_configuration_free(conf);
        return _mixer_module_configuration_new();
     }

   return conf;
}

static void
_mixer_module_configuration_setup(E_Mixer_Module_Context *ctxt)
{
   E_Config_DD *module_edd, *gadget_edd;

   gadget_edd = _mixer_gadget_configuration_descriptor_new();
   module_edd = _mixer_module_configuration_descriptor_new(gadget_edd);
   ctxt->gadget_conf_edd = gadget_edd;
   ctxt->module_conf_edd = module_edd;
   ctxt->conf = _mixer_module_configuration_load(module_edd);

   ctxt->conf->version = MOD_CONFIG_FILE_VERSION;
   ctxt->desktop_notification = ctxt->conf->desktop_notification;
}

static const char _act_increase[] = "volume_increase";
static const char _act_decrease[] = "volume_decrease";
static const char _act_mute[] = "volume_mute";
static const char _lbl_increase[] = N_("Increase Volume");
static const char _lbl_decrease[] = N_("Decrease Volume");
static const char _lbl_mute[] = N_("Mute Volume");

static void
_mixer_actions_register(E_Mixer_Module_Context *ctxt)
{
   if (!ctxt->actions.incr)
     {
        ctxt->actions.incr = e_action_add(_act_increase);
        if (ctxt->actions.incr)
          {
             ctxt->actions.incr->func.go = _mixer_cb_volume_increase;
             e_action_predef_name_set(_e_mixer_Name, _lbl_increase,
                                      _act_increase, NULL, NULL, 0);
          }
     }

   if (!ctxt->actions.decr)
     {
        ctxt->actions.decr = e_action_add(_act_decrease);
        if (ctxt->actions.decr)
          {
             ctxt->actions.decr->func.go = _mixer_cb_volume_decrease;
             e_action_predef_name_set(_e_mixer_Name, _lbl_decrease,
                                      _act_decrease, NULL, NULL, 0);
          }
     }

   if (!ctxt->actions.mute)
     {
        ctxt->actions.mute = e_action_add(_act_mute);
        if (ctxt->actions.mute)
          {
             ctxt->actions.mute->func.go = _mixer_cb_volume_mute;
             e_action_predef_name_set(_e_mixer_Name, _lbl_mute, _act_mute,
                                      NULL, NULL, 0);
             e_managers_keys_ungrab();
             e_managers_keys_grab();
          }
     }
}

static void
_mixer_actions_unregister(E_Mixer_Module_Context *ctxt)
{
   if (ctxt->actions.incr)
     {
        e_action_predef_name_del(_e_mixer_Name, _lbl_increase);
        e_action_del(_act_increase);
        ctxt->actions.incr = NULL;
     }

   if (ctxt->actions.decr)
     {
        e_action_predef_name_del(_e_mixer_Name, _lbl_decrease);
        e_action_del(_act_decrease);
        ctxt->actions.decr = NULL;
     }

   if (ctxt->actions.mute)
     {
        e_action_predef_name_del(_e_mixer_Name, _lbl_mute);
        e_action_del(_act_mute);
        e_managers_keys_ungrab();
        e_managers_keys_grab();
        ctxt->actions.mute = NULL;
     }
}

void
e_mixer_default_setup(void)
{
   e_mod_mixer_volume_get = (void *)e_mixer_system_get_volume;
   e_mod_mixer_volume_set = (void *)e_mixer_system_set_volume;
   e_mod_mixer_mute_get = (void *)e_mixer_system_get_mute;
   e_mod_mixer_mute_set = (void *)e_mixer_system_set_mute;
   e_mod_mixer_mutable_get = (void *)e_mixer_system_can_mute;
   e_mod_mixer_state_get = (void *)e_mixer_system_get_state;
   e_mod_mixer_capture_get = (void *)e_mixer_system_has_capture;
   e_mod_mixer_new = (void *)e_mixer_system_new;
   e_mod_mixer_del = (void *)e_mixer_system_del;
   e_mod_mixer_channel_default_name_get = (void *)e_mixer_system_get_default_channel_name;
   e_mod_mixer_channel_get_by_name = (void *)e_mixer_system_get_channel_by_name;
   e_mod_mixer_channel_name_get = (void *)e_mixer_system_get_channel_name;
   e_mod_mixer_channel_del = (void *)e_mixer_system_channel_del;
   e_mod_mixer_channels_free = (void *)e_mixer_system_free_channels;
   e_mod_mixer_channels_get = (void *)e_mixer_system_get_channels;
   e_mod_mixer_channels_names_get = (void *)e_mixer_system_get_channels_names;
   e_mod_mixer_card_name_get = (void *)e_mixer_system_get_card_name;
   e_mod_mixer_cards_get = (void *)e_mixer_system_get_cards;
   e_mod_mixer_cards_free = (void *)e_mixer_system_free_cards;
   e_mod_mixer_card_default_get = (void *)e_mixer_system_get_default_card;
   _mixer_using_default = EINA_TRUE;
}

void
e_mixer_pulse_setup(void)
{
   E_Mixer_Instance *inst;
   E_Mixer_Module_Context *ctxt;
   Eina_List *l;

   e_mod_mixer_volume_get = (void *)e_mixer_pulse_get_volume;
   e_mod_mixer_volume_set = (void *)e_mixer_pulse_set_volume;
   e_mod_mixer_mute_get = (void *)e_mixer_pulse_get_mute;
   e_mod_mixer_mute_set = (void *)e_mixer_pulse_set_mute;
   e_mod_mixer_mutable_get = (void *)e_mixer_pulse_can_mute;
   e_mod_mixer_state_get = (void *)e_mixer_pulse_get_state;
   e_mod_mixer_capture_get = (void *)e_mixer_pulse_has_capture;
   e_mod_mixer_new = (void *)e_mixer_pulse_new;
   e_mod_mixer_del = (void *)e_mixer_pulse_del;
   e_mod_mixer_channel_default_name_get = (void *)e_mixer_pulse_get_default_channel_name;
   e_mod_mixer_channel_get_by_name = (void *)e_mixer_pulse_get_channel_by_name;
   e_mod_mixer_channel_name_get = (void *)e_mixer_pulse_get_channel_name;
   e_mod_mixer_channel_del = (void *)e_mixer_pulse_channel_del;
   e_mod_mixer_channels_free = (void *)e_mixer_pulse_free_channels;
   e_mod_mixer_channels_get = (void *)e_mixer_pulse_get_channels;
   e_mod_mixer_channels_names_get = (void *)e_mixer_pulse_get_channels_names;
   e_mod_mixer_card_name_get = (void *)e_mixer_pulse_get_card_name;
   e_mod_mixer_cards_get = (void *)e_mixer_pulse_get_cards;
   e_mod_mixer_cards_free = (void *)e_mixer_pulse_free_cards;
   e_mod_mixer_card_default_get = (void *)e_mixer_pulse_get_default_card;
   _mixer_using_default = EINA_FALSE;

   if (!mixer_mod) return;

   ctxt = mixer_mod->data;
   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     {
        if (!inst->conf->card)
          _mixer_gadget_configuration_defaults(inst->conf);
     }
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = E_NEW(E_Mixer_Module_Context, 1);
   if (!ctxt)
     return NULL;

#ifdef HAVE_ENOTIFY
   e_notification_init();
#endif

   _mixer_configure_registry_register();
   e_gadcon_provider_register(&_gc_class);
   if (!e_mixer_pulse_init()) e_mixer_default_setup();
   else e_mixer_pulse_setup();

   mixer_mod = m;
   return ctxt;
}

static void
_mixer_instances_free(E_Mixer_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
        E_Mixer_Instance *inst = ctxt->instances->data;
        e_object_del(E_OBJECT(inst->gcc));
     }
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;

   _mixer_instances_free(ctxt);

   if (ctxt->conf_dialog)
     e_object_del(E_OBJECT(ctxt->conf_dialog));

   if (ctxt->mixer_dialog)
     e_object_del(E_OBJECT(ctxt->mixer_dialog));

   _mixer_configure_registry_unregister();
   _mixer_actions_unregister(ctxt);
   e_gadcon_provider_unregister(&_gc_class);

   if (ctxt->conf)
     {
        _mixer_module_configuration_free(ctxt->conf);
        _mixer_gadget_configuration_descriptor_free(ctxt->gadget_conf_edd);
        _mixer_module_configuration_descriptor_free(ctxt->module_conf_edd);
     }

#ifdef HAVE_ENOTIFY
   e_notification_shutdown();
#endif
   e_mixer_pulse_shutdown();

   E_FREE(ctxt);
   mixer_mod = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = m->data;
   if (!ctxt)
     return 0;
   if (!ctxt->conf)
     return 1;

   return e_config_domain_save(_conf_domain, ctxt->module_conf_edd, ctxt->conf);
}


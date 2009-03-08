#include "e_mod_main.h"
#include "e_mod_system.h"

static E_Module *mixer_mod = NULL;
static char tmpbuf[PATH_MAX]; /* general purpose buffer, just use immediately */

static const char _conf_domain[] = "module.mixer";
static const char _name[] = "mixer";
const char _Name[] = "Mixer";

const char *
e_mixer_theme_path(void)
{
#define TF "/e-module-mixer.edj"
   int dirlen;

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
   char *card, *channel;

   card = e_mixer_system_get_default_card();
   if (!card)
     return 0;

   sys = e_mixer_system_new(card);
   if (!sys)
     {
	free(card);
	return 0;
     }

   channel = e_mixer_system_get_default_channel_name(sys);
   e_mixer_system_del(sys);

   if (!channel)
     {
	free(card);
	return 0;
     }

   conf->card = eina_stringshare_add(card);
   conf->channel_name = eina_stringshare_add(channel);
   conf->lock_sliders = 1;
   conf->show_locked = 0;

   free(card);
   free(channel);

   return 1;
}

static E_Mixer_Gadget_Config *
_mixer_gadget_configuration_new(E_Mixer_Module_Config *mod_conf, const char *id)
{
   E_Mixer_Gadget_Config *conf;

   conf = E_NEW(E_Mixer_Gadget_Config, 1);
   if (!conf)
     return NULL;

   if (!_mixer_gadget_configuration_defaults(conf))
     {
	E_FREE(conf);
	return NULL;
     }

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
_mixer_gadget_configuration_free_foreach(const Eina_Hash *hash, const void *key, void *hdata, void *fdata)
{
   _mixer_gadget_configuration_free_int(hdata);
   return 1;
}

static int
_mixer_module_configuration_alert(void *data)
{
   e_util_dialog_show(_("Mixer Settings Updated"), "%s", (char *)data);
   return 0;
}

static E_Mixer_Module_Config *
_mixer_module_configuration_new(void)
{
   E_Mixer_Module_Config *conf;

   conf = E_NEW(E_Mixer_Module_Config, 1);
   if (!conf)
     return NULL;

   conf->version = MOD_CONF_VERSION;

   return conf;
}

static void
_mixer_module_configuration_free(E_Mixer_Module_Config *conf)
{
   if (!conf)
     return;

   eina_hash_foreach(conf->gadgets,
		     _mixer_gadget_configuration_free_foreach, NULL);
   eina_hash_free(conf->gadgets);
   free(conf);
}

static void
_mixer_popup_update(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

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

   e_mixer_system_get_state(inst->sys, inst->channel, &inst->mixer_state);

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
   e_mixer_system_get_volume(inst->sys, inst->channel,
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

   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_balance_right(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mixer_system_get_volume(inst->sys, inst->channel,
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
   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_volume_increase(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mixer_system_get_volume(inst->sys, inst->channel,
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

   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_volume_decrease(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   state = &inst->mixer_state;
   e_mixer_system_get_volume(inst->sys, inst->channel,
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

   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_toggle_mute(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;

   if (!e_mixer_system_can_mute(inst->sys, inst->channel))
     return;

   state = &inst->mixer_state;
   e_mixer_system_get_mute(inst->sys, inst->channel, &state->mute);
   state->mute = !state->mute;
   e_mixer_system_set_mute(inst->sys, inst->channel, state->mute);
   _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_volume_left_change(void *data, Evas_Object *obj, void *event)
{
   E_Mixer_Instance *inst;
   E_Mixer_Channel_State *state;

   inst = data;
   if (!inst)
     return;

   state = &inst->mixer_state;
   e_mixer_system_get_volume(inst->sys, inst->channel,
			     &state->left, &state->right);

   state->left = (int)e_slider_value_get(obj);
   if (inst->conf->lock_sliders)
     {
        state->right = state->left;
        e_slider_value_set(inst->ui.right, state->right);
     }

   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_volume_right_change(void *data, Evas_Object *obj, void *event)
{
   E_Mixer_Instance *inst;
   E_Mixer_Channel_State *state;

   inst = data;
   if (!inst)
     return;

   state = &inst->mixer_state;
   e_mixer_system_get_volume(inst->sys, inst->channel,
			     &state->left, &state->right);

   state->right = (int)e_slider_value_get(obj);
   if (inst->conf->lock_sliders)
     {
        state->left = state->right;
        e_slider_value_set(inst->ui.left, state->left);
     }

   e_mixer_system_set_volume(inst->sys, inst->channel,
			     state->left, state->right);
   _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_mute_change(void *data, Evas_Object *obj, void *event)
{
   E_Mixer_Instance *inst;
   E_Mixer_Channel_State *state;

   inst = data;
   if (!inst)
     return;

   state = &inst->mixer_state;
   state->mute = e_widget_check_checked_get(obj);
   e_mixer_system_set_mute(inst->sys, inst->channel, state->mute);

   _mixer_gadget_update(inst);
}

static void
_mixer_popup_cb_resize(Evas_Object *obj, int *w, int *h)
{
   int mw, mh;

   e_widget_min_size_get(obj, &mw, &mh);
   if (mh < 200) mh = 200;
   if (mw < 60) mw = 60;
   if (w) *w = (mw + 8);
   if (h) *h = (mh + 8);
}

static Evas_Object *
_mixer_popup_add_slider(E_Mixer_Instance *inst, int value, void (*cb) (void *data, Evas_Object *obj, void *event_info))
{
   Evas_Object *slider;

   slider = e_slider_add(inst->popup->win->evas);
   evas_object_show(slider);
   e_slider_orientation_set(slider, 0);
   e_slider_value_set(slider, value);
   e_slider_value_range_set(slider, 0.0, 100.0);
   e_slider_value_format_display_set(slider, NULL);
   evas_object_smart_callback_add(slider, "changed", cb, inst);

   return slider;
}

static void
_mixer_app_cb_del(E_Dialog *dialog, void *data)
{
   E_Mixer_Module_Context *ctxt = data;
   ctxt->mixer_dialog = NULL;
}

static void _mixer_popup_del(E_Mixer_Instance *inst);

static int
_mixer_popup_input_window_mouse_up_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev = event;
   E_Mixer_Instance *inst = data;

   if (ev->win != inst->ui.input.win)
     return 1;

   _mixer_popup_del(inst);

   return 1;
}

static int
_mixer_popup_input_window_key_down_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev = event;
   E_Mixer_Instance *inst = data;
   const char *keysym;

   if (ev->win != inst->ui.input.win)
     return 1;

   keysym = ev->keysymbol;
   if (strcmp(keysym, "Escape") == 0)
     _mixer_popup_del(inst);
   else if (strcmp(keysym, "Up") == 0)
     _mixer_volume_increase(inst);
   else if (strcmp(keysym, "Down") == 0)
     _mixer_volume_decrease(inst);
   else if ((strcmp(keysym, "Return") == 0) ||
	    (strcmp(keysym, "KP_Enter") == 0))
     _mixer_toggle_mute(inst);
   else
     _mixer_popup_del(inst); /* XXX really? */

   return 1;
}

static void
_mixer_popup_input_window_destroy(E_Mixer_Instance *inst)
{
   ecore_x_window_del(inst->ui.input.win);
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
     ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
			     _mixer_popup_input_window_mouse_up_cb, inst);

   inst->ui.input.key_down =
     ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
			     _mixer_popup_input_window_key_down_cb, inst);

   inst->ui.input.win = w;
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
}

static void
_mixer_app_select_current(E_Dialog *dialog, E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf = inst->conf;

   e_mixer_app_dialog_select(dialog, conf->card, conf->channel_name);
}


static void
_mixer_popup_cb_mixer(void *data, void *data2)
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
   ctxt->mixer_dialog = e_mixer_app_dialog_new(
      con, _mixer_app_cb_del, ctxt);

   _mixer_app_select_current(ctxt->mixer_dialog, inst);
}

static void
_mixer_popup_new(E_Mixer_Instance *inst)
{
   E_Mixer_Channel_State *state;
   Evas *evas;
   int colspan;

   if (inst->conf->dialog)
     return;

   state = &inst->mixer_state;
   e_mixer_system_get_state(inst->sys, inst->channel, state);

   if ((state->right >= 0) &&
       (inst->conf->show_locked || (!inst->conf->lock_sliders)))
       colspan = 2;
   else
       colspan = 1;

   inst->popup = e_gadcon_popup_new(inst->gcc, _mixer_popup_cb_resize);
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

   if (e_mixer_system_can_mute(inst->sys, inst->channel))
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

   inst->ui.button = e_widget_button_add(evas, _("Controls"), NULL,
					 _mixer_popup_cb_mixer, inst, NULL);
   e_widget_table_object_append(inst->ui.table, inst->ui.button,
                                0, 7, colspan, 1, 1, 1, 1, 0);

   e_gadcon_popup_content_set(inst->popup, inst->ui.table);
   e_gadcon_popup_show(inst->popup);
   _mixer_popup_input_window_create(inst);
}

static void
_mixer_menu_cb_post(void *data, E_Menu *menu)
{
   E_Mixer_Instance *inst;

   inst = data;
   if ((!inst) || (!inst->menu))
     return;
   if (inst->menu)
     {
	e_object_del(E_OBJECT(inst->menu));
	inst->menu = NULL;
     }
}

static void
_mixer_menu_cb_cfg(void *data, E_Menu *menu, E_Menu_Item *mi)
{
   E_Mixer_Instance *inst;
   E_Container *con;

   inst = data;
   if (!inst)
     return;
   if (inst->popup)
     _mixer_popup_del(inst);
   con = e_container_current_get(e_manager_current_get());
   inst->conf->dialog = e_mixer_config_dialog_new(con, inst->conf);
}

static void
_mixer_menu_new(E_Mixer_Instance *inst, Evas_Event_Mouse_Down *ev)
{
   E_Zone *zone;
   E_Menu *mn;
   E_Menu_Item *mi;
   int x, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   mn = e_menu_new();
   e_menu_post_deactivate_callback_set(mn, _mixer_menu_cb_post, inst);
   inst->menu = mn;

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _mixer_menu_cb_cfg, inst);

   e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
   e_menu_activate_mouse(mn, zone, x + ev->output.x, y + ev->output.y,
			 1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
   evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
			    EVAS_BUTTON_NONE, ev->timestamp, NULL);
}

static void
_mixer_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Mixer_Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   if (!inst)
     return;

   ev = event;
   if (ev->button == 1)
     {
        if (!inst->popup)
	  _mixer_popup_new(inst);
        else
	  _mixer_popup_del(inst);
     }
   else if (ev->button == 2)
     _mixer_toggle_mute(inst);
   else if ((ev->button == 3) && (!inst->menu))
     _mixer_menu_new(inst, ev);
}

static void
_mixer_cb_mouse_wheel(void *data, Evas *evas, Evas_Object *obj, void *event)
{
   E_Mixer_Instance *inst;
   Evas_Event_Mouse_Wheel *ev;

   inst = data;
   if (!inst)
     return;

   ev = event;
   if (ev->direction == 0)
     {
	if (ev->z > 0)
	  _mixer_volume_decrease(inst);
	else if (ev->z < 0)
	  _mixer_volume_increase(inst);
     }
   else if (ev->direction == 1)
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

   if (inst->sys)
     e_mixer_system_del(inst->sys);

   inst->sys = e_mixer_system_new(conf->card);
   if (!inst->sys)
     {
	inst->channel = NULL;
	return 0;
     }

   inst->channel = e_mixer_system_get_channel_by_name(inst->sys,
						      conf->channel_name);
   return inst->channel != NULL;
}

static int
_mixer_system_cb_update(void *data, E_Mixer_System *sys)
{
   E_Mixer_Instance *inst;

   inst = data;
   e_mixer_system_get_state(inst->sys, inst->channel, &inst->mixer_state);
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
   if (r)
     e_mixer_system_callback_set(inst->sys, _mixer_system_cb_update, inst);

   return r;
}

static int
_mixer_sys_setup_default_card(E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf;
   char *card;

   conf = inst->conf;
   if (conf->card)
     eina_stringshare_del(conf->card);

   card = e_mixer_system_get_default_card();
   if (!card)
     goto error;

   inst->sys = e_mixer_system_new(card);
   if (!inst->sys)
     goto system_error;

   conf->card = eina_stringshare_add(card);
   free(card);
   return 1;


 system_error:
   free(card);
 error:
   conf->card = NULL;
   return 0;
}

static int
_mixer_sys_setup_default_channel(E_Mixer_Instance *inst)
{
   E_Mixer_Gadget_Config *conf;
   char *channel_name;

   conf = inst->conf;
   if (conf->channel_name)
     eina_stringshare_del(conf->channel_name);

   channel_name = e_mixer_system_get_default_channel_name(inst->sys);
   if (!channel_name)
     goto error;

   inst->channel = e_mixer_system_get_channel_by_name(inst->sys, channel_name);
   if (!inst->channel)
     goto system_error;

   conf->channel_name = eina_stringshare_add(channel_name);
   free(channel_name);
   return 1;

 system_error:
   free(channel_name);
 error:
   conf->channel_name = NULL;
   return 0;
}

static int
_mixer_sys_setup_defaults(E_Mixer_Instance *inst)
{
   if ((!inst->sys) && (!_mixer_sys_setup_default_card(inst)))
     return 0;

   return _mixer_sys_setup_default_channel(inst);
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
   conf->instance = inst;
   if ((!_mixer_sys_setup(inst)) && (!_mixer_sys_setup_defaults(inst)))
     {
	if (inst->sys)
	  e_mixer_system_del(inst->sys);
	_mixer_gadget_configuration_free(ctxt->conf, conf);
	E_FREE(inst);
	return NULL;
     }
   e_mixer_system_callback_set(inst->sys, _mixer_system_cb_update, inst);

   inst->ui.gadget = edje_object_add(gc->evas);
   e_theme_edje_object_set(inst->ui.gadget, "base/theme/modules/mixer",
			   "e/modules/mixer/main");

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->ui.gadget);
   inst->gcc->data = inst;

   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mixer_cb_mouse_down, inst);
   evas_object_event_callback_add(inst->ui.gadget, EVAS_CALLBACK_MOUSE_WHEEL,
				  _mixer_cb_mouse_wheel, inst);

   e_mixer_system_get_state(inst->sys, inst->channel, &inst->mixer_state);
   _mixer_gadget_update(inst);

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

   if (inst->menu)
     {
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
	e_object_del(E_OBJECT(inst->menu));
     }
   evas_object_del(inst->ui.gadget);
   e_mixer_system_channel_del(inst->channel);
   e_mixer_system_del(inst->sys);

   inst->conf->instance = NULL;
   ctxt->instances = eina_list_remove(ctxt->instances, inst);

   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return _(_Name);
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;

   o = edje_object_add(evas);
   edje_object_file_set(o, e_mixer_theme_path(), "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
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
   {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL},
   E_GADCON_CLIENT_STYLE_PLAIN
};



EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, _Name};

static void
_mixer_cb_volume_increase(E_Object *obj, const char *params)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (ctxt->default_instance)
     _mixer_volume_increase(ctxt->default_instance);
}

static void
_mixer_cb_volume_decrease(E_Object *obj, const char *params)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (ctxt->default_instance)
     _mixer_volume_decrease(ctxt->default_instance);
}

static void
_mixer_cb_volume_mute(E_Object *obj, const char *params)
{
   E_Mixer_Module_Context *ctxt;

   if (!mixer_mod)
     return;

   ctxt = mixer_mod->data;
   if (!ctxt->conf)
     return;

   if (ctxt->default_instance)
     _mixer_toggle_mute(ctxt->default_instance);
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
static const char _reg_item[] = "extensions/e";

static void
_mixer_configure_registry_register(void)
{
   e_configure_registry_category_add(_reg_cat, 90, _("Extensions"), NULL,
                                     "enlightenment/extensions");
   e_configure_registry_item_add(_reg_item, 30, _(_Name), NULL,
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
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, card, STR);
   E_CONFIG_VAL(conf_edd, E_Mixer_Gadget_Config, channel_name, STR);

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

   if (conf->version != MOD_CONF_VERSION)
     {
	_mixer_module_configuration_free(conf);
	conf = _mixer_module_configuration_new();
	if (!conf)
	  return NULL;

	ecore_timer_add(1.0, _mixer_module_configuration_alert,
			_("Mixer Module Settings data changed.<br>"
			   "Your old configuration has been replaced with "
			   "new default.<br>Sorry for the inconvenience."));
	return conf;
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
}

static const char _act_increase[] = "volume_increase";
static const char _act_decrease[] = "volume_decrease";
static const char _act_mute[] = "volume_mute";
static const char _lbl_increase[] = "Increase Volume";
static const char _lbl_decrease[] = "Decrease Volume";
static const char _lbl_mute[] = "Mute Volume";

static void
_mixer_actions_register(E_Mixer_Module_Context *ctxt)
{
   ctxt->actions.incr = e_action_add(_act_increase);
   if (ctxt->actions.incr)
     {
        ctxt->actions.incr->func.go = _mixer_cb_volume_increase;
        e_action_predef_name_set(_(_Name), _(_lbl_increase), _act_increase,
				 NULL, NULL, 0);
     }

   ctxt->actions.decr = e_action_add(_act_decrease);
   if (ctxt->actions.decr)
     {
        ctxt->actions.decr->func.go = _mixer_cb_volume_decrease;
        e_action_predef_name_set(_(_Name), _(_lbl_decrease), _act_decrease,
				 NULL, NULL, 0);
     }

   ctxt->actions.mute = e_action_add(_act_mute);
   if (ctxt->actions.mute)
     {
        ctxt->actions.mute->func.go = _mixer_cb_volume_mute;
        e_action_predef_name_set(_(_Name), _(_lbl_mute), _act_mute,
                                 NULL, NULL, 0);
     }
}

static void
_mixer_actions_unregister(E_Mixer_Module_Context *ctxt)
{
   if (ctxt->actions.incr)
     {
        e_action_predef_name_del(_(_Name), _(_lbl_increase));
        e_action_del(_act_increase);
     }

   if (ctxt->actions.decr)
     {
        e_action_predef_name_del(_(_Name), _(_lbl_decrease));
        e_action_del(_act_decrease);
     }

   if (ctxt->actions.mute)
     {
        e_action_predef_name_del(_(_Name), _(_lbl_mute));
        e_action_del(_act_mute);
     }
}

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Mixer_Module_Context *ctxt;

   ctxt = E_NEW(E_Mixer_Module_Context, 1);
   if (!ctxt)
     return NULL;

   _mixer_configure_registry_register();
   _mixer_actions_register(ctxt);
   e_gadcon_provider_register(&_gc_class);
   mixer_mod = m;
   return ctxt;
}

static void
_mixer_instances_free(E_Mixer_Module_Context *ctxt)
{
   while (ctxt->instances)
     {
	E_Mixer_Instance *inst;

	inst = ctxt->instances->data;
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

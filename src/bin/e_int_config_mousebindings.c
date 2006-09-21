#include "e.h"


#define EXAMPLE_STRING "example : "
#define E_BINDING_CONTEXT_NUMBER  10


#define TEXT_PRESS_MOUSE_BINIDING_SEQUENCE _("Please hold any modifier you want<br>" \
					     "and press any button on your mouse,<br> or roll a" \
					     " wheel, to assign mouse binding." \
					     "<br>Press <hilight>Escape</highlight> to abort.")

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _fill_data(E_Config_Dialog_Data *cfdata);

static void _fill_actions_list(E_Config_Dialog_Data *cfdata);
static void _auto_apply_changes(E_Config_Dialog_Data *cfdata);
/******** updates ************/
static void _update_binding_list(E_Config_Dialog_Data *cfdata);
static void _update_binding_list_selection(E_Config_Dialog_Data *cfdata, const void *bind);
static void _update_buttons(E_Config_Dialog_Data *cfdata);
static void _update_binding_params(E_Config_Dialog_Data *cfdata);

/******** callbacks ***********/
static void _binding_change_cb(void *data);
static void _action_change_cb(void *data);
static void _add_mouse_binding_cb(void *data, void *data2);
static void _modify_mouse_binding_cb(void *data, void *data2);
static void _delete_mouse_binding_cb(void *data, void *data2);
static void _restore_default_cb(void *data, void *data2);

static int _grab_key_down_cb(void *data, int type, void *event);
static int _grab_mouse_down_cb(void *data, int type, void *event);
static int _grab_mouse_wheel_cb(void *data, int type, void *event);
static void _grab_wnd_hide_cb(E_Config_Dialog_Data *cfdata);

/******** helpers *************/
static char *_helper_button_name_get(E_Config_Binding_Mouse *eb);
static char *_helper_wheel_name_get(E_Config_Binding_Wheel *bw);
static char *_helper_modifier_name_get(int mod);

/********* sorts ***************/
static int _mouse_binding_sort_cb(void *d1, void *d2);
static int _wheel_binding_sort_cb(void *d1, void *d2);


#define E_ACTION_NUMBER 32
const char *action_to_name[E_ACTION_NUMBER][2] = { 
       {"window_move", "Move Window"}, {"window_resize", "Resize Window" },
       {"window_raise", "Raise Window"}, {"window_menu", "Window Menu"},
       {"window_lower", "Lower Window"}, {"window_close", "Close Window"},
       {"window_kill", "Kill Window"}, {"window_sticky_toggle", "Sticky Window"},
       {"window_iconic_toggle", "Iconify Window"}, {"window_shaded_toggle", "Shade Window"},
       {"window_bordless_toggle", "Bordless Window"}, {"desk_flip_by", "Flip Desktop By #:#"},
       {"desk_deskshow_toggle", "Show Desktop"}, {"desk_flip_to", "Flip To Desktop #:#"},
       {"desk_linear_flip_by", "Flip Desktop By #"},
       {"desk_linear_flip_to", "Flip To Desktop #"},
       {"screen_send_to", "Move Mouse To Screen #"},
       {"screen_send_by", "Move Mouse By Screen #"},
       {"window_move_to", "Move Window To Position"}, {"window_move_by", "Shift Window"},
       {"window_resize_by", "Resize Window By Size"},
       {"window_drag_icon", "Drag Window Icon"},
       {"window_desk_move_by", "Move Window By Desktop #"},
       {"window_desk_move_to", "Move Window To Desktop #"}, {"menu_show", "Show Menu"},
       {"exec", "Run User Command"}, {"app", "Launch Application"},
       {"winlist", "Run Winlist Dialog"}, {"restart", "Restart Enlightenment"},
       {"exit", "Exit Enlightenment"}, {"exebuf", "Run Command Dialog"},
       {"desk_lock", "Lock Screen"}
};


struct _E_Config_Dialog_Data
{
  E_Config_Dialog *cfd;

  Evas *evas;

  struct
    {
       Evas_List *mouse;
       Evas_List *wheel;
    } binding;

  struct
    {
       char *binding;
       char *action;
       char *params;
       int context;

       char *cur;
       int add; //just to distinguesh among two buttons add/modify

       E_Dialog *dia;
       Ecore_X_Window mousebind_win;
       Evas_List *handlers;
    } locals;

  struct 
    { 
       Evas_Object *o_binding_list;
       Evas_Object *o_action_list;
       Evas_Object *o_add;
       Evas_Object *o_mod;
       Evas_Object *o_del;
       Evas_Object *o_params;

       struct {
	 Evas_Object *o_any, *o_border, *o_menu, *o_winlist, *o_popup, *o_zone,
		     *o_container, *o_manager, *o_none;
       } context;
    } gui;
};

EAPI E_Config_Dialog *
e_int_config_mousebindings(E_Container *con)
{
  E_Config_Dialog *cfd;
  E_Config_Dialog_View *v;

  v = E_NEW(E_Config_Dialog_View, 1);

  v->create_cfdata = _create_data;
  v->free_cfdata = _free_data;
  v->basic.apply_cfdata = _basic_apply_data;
  v->basic.create_widgets = _basic_create_widgets;
  v->override_auto_apply = 0;

  cfd = e_config_dialog_new(con,
			    _("Mouse Binding Settings"),
			    "E", "_config_mousebindings_dialog",
			    "enlightenment/mouse_clean", 0, v, NULL);
  return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   E_Config_Binding_Mouse *eb, *eb2;
   E_Config_Binding_Wheel *bw, *bw2;

   cfdata->locals.binding = strdup("");
   cfdata->locals.action = strdup("");
   cfdata->locals.params = strdup("");
   cfdata->locals.context = E_BINDING_CONTEXT_ANY;
   cfdata->binding.mouse = NULL;
   cfdata->binding.wheel = NULL;
   cfdata->locals.mousebind_win = 0;
   cfdata->locals.handlers =  NULL;
   cfdata->locals.dia = NULL;

   for (l = e_config->mouse_bindings; l; l = l->next)
     {
	eb = l->data;

	eb2 = E_NEW(E_Config_Binding_Mouse, 1);
	eb2->context = eb->context;
	eb2->button = eb->button;
	eb2->modifiers = eb->modifiers;
	eb2->any_mod = eb->any_mod;
	eb2->action = eb->action == NULL ? NULL : evas_stringshare_add(eb->action);
	eb2->params = eb->params == NULL ? NULL : evas_stringshare_add(eb->params);

	cfdata->binding.mouse = evas_list_append(cfdata->binding.mouse, eb2);
     }

   for (l = e_config->wheel_bindings; l; l = l->next)
     {
	bw = l->data;

	bw2 = E_NEW(E_Config_Binding_Wheel, 1);
	bw2->context = bw->context;
	bw2->direction = bw->direction;
	bw2->z = bw->z;
	bw2->modifiers = bw->modifiers;
	bw2->any_mod = bw->any_mod;
	bw2->action = bw->action == NULL ? NULL : evas_stringshare_add(bw->action);
	bw2->params = bw->params == NULL ? NULL : evas_stringshare_add(bw->params);

	cfdata->binding.wheel = evas_list_append(cfdata->binding.wheel, bw2);
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   
   _fill_data(cfdata);
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   while (cfdata->binding.mouse)
     {
	eb = cfdata->binding.mouse->data;
	if (eb->action) evas_stringshare_del(eb->action);
	if (eb->params) evas_stringshare_del(eb->params);
	E_FREE(eb);
	cfdata->binding.mouse =
	   evas_list_remove_list(cfdata->binding.mouse, cfdata->binding.mouse);
     }

   while (cfdata->binding.wheel)
     {
	bw = cfdata->binding.wheel->data;
	if (bw->action) evas_stringshare_del(bw->action);
	if (bw->params) evas_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->binding.wheel =
	   evas_list_remove_list(cfdata->binding.wheel, cfdata->binding.wheel);
     }

   if (cfdata->locals.binding) free(cfdata->locals.binding);
   if (cfdata->locals.action) free(cfdata->locals.action);
   if (cfdata->locals.params) free(cfdata->locals.params);
   if (cfdata->locals.cur) free(cfdata->locals.cur);
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ 
   Evas_List *l;
   E_Config_Binding_Mouse *eb, *eb2;
   E_Config_Binding_Wheel *bw, *bw2;

   _auto_apply_changes(cfdata);


   e_border_button_bindings_ungrab_all();
   while (e_config->mouse_bindings)
     {
	eb = e_config->mouse_bindings->data;
	e_bindings_mouse_del(eb->context, eb->button, eb->modifiers, eb->any_mod,
			     eb->action, eb->params);
	if (eb->action) evas_stringshare_del(eb->action);
	if (eb->params) evas_stringshare_del(eb->params);
	E_FREE(eb);
	e_config->mouse_bindings = 
	   evas_list_remove_list(e_config->mouse_bindings, e_config->mouse_bindings);
     }

   for (l = cfdata->binding.mouse; l; l = l->next)
     {
	eb = l->data;

	eb2 = E_NEW(E_Config_Binding_Mouse, 1);
	eb2->context = eb->context;
	eb2->button = eb->button;
	eb2->modifiers = eb->modifiers;
	eb2->any_mod = eb->any_mod;
	eb2->action = eb->action == NULL ? NULL : evas_stringshare_add(eb->action);
	eb2->params = eb->params == NULL ? NULL : evas_stringshare_add(eb->params);

	e_config->mouse_bindings = evas_list_append(e_config->mouse_bindings, eb2);
	e_bindings_mouse_add(eb2->context, eb2->button, eb2->modifiers, eb2->any_mod,
			     eb2->action, eb2->params);
     }

   while (e_config->wheel_bindings)
     {
	bw = e_config->wheel_bindings->data;
	e_bindings_wheel_del(bw->context, bw->direction, bw->z, bw->modifiers, bw->any_mod,
			     bw->action, bw->params);
	if (bw->action) evas_stringshare_del(bw->action);
	if (bw->params) evas_stringshare_del(bw->params);
	E_FREE(bw);

	e_config->wheel_bindings = 
	   evas_list_remove_list(e_config->wheel_bindings, e_config->wheel_bindings);
     }

   for (l = cfdata->binding.wheel; l; l = l->next)
     {
	bw = l->data;

	bw2 = E_NEW(E_Config_Binding_Wheel, 1);
	bw2->context = bw->context;
	bw2->direction = bw->direction;
	bw2->z = bw->z;
	bw2->modifiers = bw->modifiers;
	bw2->any_mod = bw->any_mod;
	bw2->action = bw->action == NULL ? NULL : evas_stringshare_add(bw->action);
	bw2->params = bw->params == NULL ? NULL : evas_stringshare_add(bw->params);

	e_config->wheel_bindings = evas_list_append(e_config->wheel_bindings, bw2);
	e_bindings_wheel_add(bw2->context, bw2->direction, bw2->z, bw2->modifiers,
			     bw2->any_mod, bw2->action, bw2->params);
     }
   e_border_button_bindings_grab_all();

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *ol2, *ol3, *of;
   E_Radio_Group *rg;

   cfdata->evas = evas;

   ol = e_widget_list_add(evas, 0, 1);
   of = e_widget_framelist_add(evas, _("Mouse Bindings"), 0);

   o = e_widget_ilist_add(evas, 32, 32, &(cfdata->locals.binding));
   cfdata->gui.o_binding_list = o;
   e_widget_min_size_set(o, 250, 280);
   e_widget_ilist_go(o);
   e_widget_framelist_object_append(of, o);

   ol2 = e_widget_list_add(evas, 1, 1);
   o = e_widget_button_add(evas, _("Add"), NULL, _add_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_add = o;
   e_widget_list_object_append(ol2, o, 1, 1, 0.5);

   o = e_widget_button_add(evas, _("Modify"), NULL, _modify_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_mod = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol2, o, 1, 1, 0.5);

   o = e_widget_button_add(evas, _("Delete"), NULL, _delete_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol2, o, 1, 1, 0.5);
   e_widget_framelist_object_append(of, ol2);

   o = e_widget_button_add(evas, _("Restore Mouse and Wheel Binding Defaults"), NULL,
			   _restore_default_cb, cfdata, NULL);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);


   ol2 = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   o = e_widget_ilist_add(evas, 24, 24, &(cfdata->locals.action));
   cfdata->gui.o_action_list = o;
   e_widget_min_size_set(o, 250, 180); 
   e_widget_ilist_go(o);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Action Params"), 0);
   o = e_widget_entry_add(evas, &(cfdata->locals.params));
   e_widget_disabled_set(o, 1);
   cfdata->gui.o_params = o;
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Action Context"), 1);
   rg = e_widget_radio_group_new(&(cfdata->locals.context));
   ol3 = e_widget_list_add(evas, 0, 0);

   o = e_widget_radio_add(evas, _("Any"), E_BINDING_CONTEXT_ANY, rg);
   cfdata->gui.context.o_any = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);

   o = e_widget_radio_add(evas, _("Border"), E_BINDING_CONTEXT_BORDER, rg);
   cfdata->gui.context.o_border = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);

   o = e_widget_radio_add(evas, _("Menu"), E_BINDING_CONTEXT_MENU, rg);
   cfdata->gui.context.o_menu = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);
   e_widget_framelist_object_append(of, ol3);

   ol3 = e_widget_list_add(evas, 0, 0);
   o = e_widget_radio_add(evas, _("Win List"), E_BINDING_CONTEXT_WINLIST, rg);
   cfdata->gui.context.o_winlist = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);

   o = e_widget_radio_add(evas, _("Popup"), E_BINDING_CONTEXT_POPUP, rg);
   cfdata->gui.context.o_popup = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);

   o = e_widget_radio_add(evas, _("Zone"), E_BINDING_CONTEXT_ZONE, rg);
   cfdata->gui.context.o_zone = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);
   e_widget_framelist_object_append(of, ol3);

   ol3 = e_widget_list_add(evas, 0, 0);
   o = e_widget_radio_add(evas, _("Container"), E_BINDING_CONTEXT_CONTAINER, rg);
   cfdata->gui.context.o_container = o;
   e_widget_disabled_set(o, 1);

   e_widget_list_object_append(ol3, o, 1, 1, 0.5);
   o = e_widget_radio_add(evas, _("Manager"), E_BINDING_CONTEXT_MANAGER, rg);
   cfdata->gui.context.o_manager = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);

   o = e_widget_radio_add(evas, _("None"), E_BINDING_CONTEXT_NONE, rg);
   cfdata->gui.context.o_none = o;
   e_widget_disabled_set(o, 1);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);
   e_widget_framelist_object_append(of, ol3);
   e_widget_list_object_append(ol2, of, 1, 1, 0.5);

   e_widget_list_object_append(ol, ol2, 1, 1, 0.5);

   _fill_actions_list(cfdata);
   _update_binding_list(cfdata);

   return ol;
}

static void
_fill_actions_list(E_Config_Dialog_Data *cfdata)
{
   char buf[1024];
   int i;

   for (i = 0; i < E_ACTION_NUMBER; i++)
     {
	snprintf(buf, sizeof(buf), "%d", i);
	e_widget_ilist_append(cfdata->gui.o_action_list, NULL, action_to_name[i][1],
			      _action_change_cb, cfdata, buf);
     } 
   e_widget_ilist_go(cfdata->gui.o_action_list);
}

static void
_auto_apply_changes(E_Config_Dialog_Data *cfdata)
{
   char *n;
   const char *action;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   if (!cfdata->locals.cur) return;

   n = cfdata->locals.cur;

   if (cfdata->locals.cur[0] == 'm')
     {
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	if (!eb) return;

	eb->context = cfdata->locals.context;

	if (eb->action) evas_stringshare_del(eb->action);
	if (e_widget_ilist_selected_get(cfdata->gui.o_action_list) >= 0) 
	  action = action_to_name[atoi(cfdata->locals.action)][0];
	else
	  action = NULL;
	eb->action = action ? evas_stringshare_add(action) : NULL;

	if (eb->params) evas_stringshare_del(eb->params);
	if ((!cfdata->locals.params) || (!cfdata->locals.params[0]) ||
	    (!strncmp(cfdata->locals.params, EXAMPLE_STRING, strlen(EXAMPLE_STRING))))
	  eb->params = NULL;
	else
	  eb->params = evas_stringshare_add(cfdata->locals.params);
     }
   else
     {
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	if (!bw) return;

	bw->context = cfdata->locals.context;
	if (bw->action) evas_stringshare_del(bw->action);
	if (e_widget_ilist_selected_get(cfdata->gui.o_action_list) >= 0)
	  action = action_to_name[atoi(cfdata->locals.action)][0];
	else
	  action = NULL;
	bw->action = action ? evas_stringshare_add(action) : NULL;

	if (bw->params) evas_stringshare_del(bw->params);
	if ((!cfdata->locals.params) || (!cfdata->locals.params[0]))
	  bw->params = NULL;
	else
	  bw->params = evas_stringshare_add(cfdata->locals.params);
     }
}
/********* updates *************/
static void 
_update_binding_actions(E_Config_Dialog_Data *cfdata)
{
   char *n;
   int i;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   const char *action;

   e_widget_ilist_clear(cfdata->gui.o_action_list);
   _fill_actions_list(cfdata);

   if (!cfdata->locals.cur) return;

   n = cfdata->locals.cur;
   if (cfdata->locals.cur[0] == 'm')
     {
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	if (!eb) return;
	action = eb->action;
     }
   else
     {
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	if (!bw) return;
	action = bw->action;
     }

   if (action && action[0])
     { 
	for (i = 0; i < E_ACTION_NUMBER; i++) 
	  { 
	     if (!strcmp(action_to_name[i][0], action)) 
	       { 
		  e_widget_ilist_selected_set(cfdata->gui.o_action_list, i); 
		  break; 
	       }
	  }
     }
   else
     cfdata->locals.action[0] = '\0';
}
static void
_update_binding_list(E_Config_Dialog_Data *cfdata)
{
   char *icon = NULL, *button, *mods;
   char label[1024], val[10];
   int i;
   Evas_List *l;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   e_widget_ilist_clear(cfdata->gui.o_binding_list);

   if (cfdata->binding.mouse) 
     { 
	cfdata->binding.mouse = evas_list_sort(cfdata->binding.mouse, 
	      evas_list_count(cfdata->binding.mouse), _mouse_binding_sort_cb);
	
	e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, "Mouse Buttons");
     }

   for (l = cfdata->binding.mouse, i = 0; l; l = l->next, i++)
     {
	Evas_Object *ic;
	eb = l->data;

	button = _helper_button_name_get(eb);
	mods = _helper_modifier_name_get(eb->modifiers);

	snprintf(label, sizeof(label), "%s%s%s", button ? button : "", mods[0] ? " + ": "",
	         mods ? mods : "");
	if (button) free(button);
	if (mods) free(mods);

	switch (eb->button)
	  {
	   case 1:
	      icon = "enlightenment/mouse_left";
	      break;
	   case 2:
	      icon = "enlightenment/mouse_middle";
	      break;
	   case 3:
	      icon = "enlightenment/mouse_right";
	      break;
	   default:
	      icon = "enlightenment/mouse_extra";
	  }

	snprintf(val, sizeof(val), "m%d", i);

	ic = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(ic, icon);
	e_widget_ilist_append(cfdata->gui.o_binding_list, ic, label, _binding_change_cb,
			      cfdata, val);
     }

   if (cfdata->binding.wheel) 
     {
	cfdata->binding.wheel = evas_list_sort(cfdata->binding.wheel, 
	      evas_list_count(cfdata->binding.wheel), _wheel_binding_sort_cb); 
	
	e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, "Mouse Wheels");
     }

   for (l = cfdata->binding.wheel, i = 0; l; l = l->next, i++)
     {
	Evas_Object *ic;
	bw = l->data;

	button = _helper_wheel_name_get(bw);
	mods = _helper_modifier_name_get(bw->modifiers);

	snprintf(label, sizeof(label), "%s%s%s", button ? button : "", mods[0] ? " + ": "",
	        mods ? mods : "");
	if (button) free(button);
	if (mods) free(mods);

	snprintf(val, sizeof(val), "w%d", i);

	ic = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(ic, "enlightenment/mouse_wheel");
	e_widget_ilist_append(cfdata->gui.o_binding_list, ic, label, _binding_change_cb,
			      cfdata, val);
     }

   e_widget_ilist_go(cfdata->gui.o_binding_list);
}
static void
_update_binding_list_selection(E_Config_Dialog_Data *cfdata, const void *bind)
{
   Evas_List *l;
   int sel;

   for (l = cfdata->binding.mouse, sel = 0; l; l = l->next, sel++)
     {
	if (l->data == bind)
	  {
	     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel + 1);
	     return;
	  }
     }

   if (evas_list_count(cfdata->binding.mouse)) 
     sel = evas_list_count(cfdata->binding.mouse) + 1;
   else
     sel = 0;

   for (l = cfdata->binding.wheel; l; l = l->next, sel++)
     {
	if (l->data == bind)
	  {
	     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel + 1);
	     return;
	  }
     }
}
static void
_update_buttons(E_Config_Dialog_Data *cfdata)
{
   if (!cfdata->locals.cur)
     { 
	e_widget_disabled_set(cfdata->gui.o_mod, 1); 
	e_widget_disabled_set(cfdata->gui.o_del, 1);
	return;
     }

   e_widget_disabled_set(cfdata->gui.o_mod, 0);
   e_widget_disabled_set(cfdata->gui.o_del, 0);
}
static void
_update_binding_context(E_Config_Dialog_Data *cfdata)
{
   char *n;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   E_Binding_Context ctxt;

   e_widget_disabled_set(cfdata->gui.context.o_any, 1);
   e_widget_radio_toggle_set(cfdata->gui.context.o_any, 1);
   e_widget_disabled_set(cfdata->gui.context.o_border, 1);
   e_widget_disabled_set(cfdata->gui.context.o_menu, 1);
   e_widget_disabled_set(cfdata->gui.context.o_winlist, 1);
   e_widget_disabled_set(cfdata->gui.context.o_popup, 1);
   e_widget_disabled_set(cfdata->gui.context.o_zone, 1);
   e_widget_disabled_set(cfdata->gui.context.o_container, 1);
   e_widget_disabled_set(cfdata->gui.context.o_manager, 1);
   e_widget_disabled_set(cfdata->gui.context.o_none, 1);

   if (!cfdata->locals.cur) return;

   n = cfdata->locals.cur;
   if (cfdata->locals.cur[0] == 'm')
     {
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	if (!eb) return;
	ctxt = eb->context;
     }
   else
     {
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	if (!bw) return;
	ctxt = bw->context;
     }

   e_widget_disabled_set(cfdata->gui.context.o_any, 0);
   e_widget_disabled_set(cfdata->gui.context.o_border, 0);
   e_widget_disabled_set(cfdata->gui.context.o_menu, 0);
   e_widget_disabled_set(cfdata->gui.context.o_winlist, 0);
   e_widget_disabled_set(cfdata->gui.context.o_popup, 0);
   e_widget_disabled_set(cfdata->gui.context.o_zone, 0);
   e_widget_disabled_set(cfdata->gui.context.o_container, 0);
   e_widget_disabled_set(cfdata->gui.context.o_manager, 0);
   e_widget_disabled_set(cfdata->gui.context.o_none, 0);

   if (ctxt == E_BINDING_CONTEXT_ANY) 
     e_widget_radio_toggle_set(cfdata->gui.context.o_any, 1);
   else if (ctxt == E_BINDING_CONTEXT_BORDER)
     e_widget_radio_toggle_set(cfdata->gui.context.o_border, 1);
   else if (ctxt == E_BINDING_CONTEXT_MENU)
     e_widget_radio_toggle_set(cfdata->gui.context.o_menu, 1);
   else if (ctxt == E_BINDING_CONTEXT_WINLIST)
     e_widget_radio_toggle_set(cfdata->gui.context.o_winlist, 1);
   else if (ctxt == E_BINDING_CONTEXT_POPUP)
     e_widget_radio_toggle_set(cfdata->gui.context.o_popup, 1);
   else if (ctxt == E_BINDING_CONTEXT_ZONE)
     e_widget_radio_toggle_set(cfdata->gui.context.o_zone, 1);
   else if (ctxt == E_BINDING_CONTEXT_CONTAINER)
     e_widget_radio_toggle_set(cfdata->gui.context.o_container, 1);
   else if (ctxt == E_BINDING_CONTEXT_MANAGER)
     e_widget_radio_toggle_set(cfdata->gui.context.o_manager, 1);
   else if (ctxt == E_BINDING_CONTEXT_NONE)
     e_widget_radio_toggle_set(cfdata->gui.context.o_none, 1);
}
static void
_update_binding_params(E_Config_Dialog_Data *cfdata)
{ 
   char *n;
   const char *params;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);

   if (!cfdata->locals.cur) return; 
   
   e_widget_disabled_set(cfdata->gui.o_params, 0);

   n = cfdata->locals.cur;
   if (cfdata->locals.cur[0] == 'm')
     {
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	params = eb->params;
     }
   else
     {
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	params = bw->params;
     }

   if ((!params) || (!params[0]))
     { 
	//FIXME: need to put example
	//e_widget_entry_text_set(cfdata->gui.o_params, EXAMPLE_STRING);
	e_widget_entry_clear(cfdata->gui.o_params);
     }
   else
     e_widget_entry_text_set(cfdata->gui.o_params, params);
}
/******** callbacks ***********/
static void
_action_change_cb(void *data)
{
   const char *action;
   char *n;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   if (!cfdata->locals.cur) return;

   action = action_to_name[atoi(cfdata->locals.action)][0];

   n = cfdata->locals.cur;
   if (cfdata->locals.cur[0] == 'm')
     {
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	if (!eb || !eb->action)
	  { 
	     //FIXME: need to put here an example how to make binding 
	     e_widget_entry_clear(cfdata->gui.o_params);
	     return;
	  }

	if (!strcmp(eb->action, action))
	  {
	     if (!eb->params)
	       { 
		  //FIXME: need to put here an example how to make binding 
		  //e_widget_entry_text_set(cfdata->gui.o_params, EXAMPLE_STRING);
		  e_widget_entry_clear(cfdata->gui.o_params);
	       }
	     else
	       e_widget_entry_text_set(cfdata->gui.o_params, eb->params);
	  }
	else
	  { 
	     //FIXME: need to put here an example how to make binding 
	     //e_widget_entry_text_set(cfdata->gui.o_params, EXAMPLE_STRING);
	     e_widget_entry_clear(cfdata->gui.o_params);
	  }
     }
   else
     {
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	if (!bw || !bw->action)
	  { 
	     //FIXME: need to put here an example how to make binding 
	     e_widget_entry_clear(cfdata->gui.o_params);
	     return;
	  }

	if (!strcmp(bw->action, action))
	  { 
	     if (!bw->params)
	       {
		  //FIXME: need to put here an example how to make binding 
		  e_widget_entry_clear(cfdata->gui.o_params);
	       }
	     else
	       e_widget_entry_text_set(cfdata->gui.o_params, bw->params);
	  }
	else
	  { 
	     //FIXME: need to put here an example how to make binding 
	     //e_widget_entry_text_set(cfdata->gui.o_params, EXAMPLE_STRING);
	     e_widget_entry_clear(cfdata->gui.o_params);
	  }
     }
}

static void
_binding_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);

   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   if (cfdata->locals.binding[0]) 
     cfdata->locals.cur = strdup(cfdata->locals.binding);

   _update_binding_context(cfdata);
   _update_binding_actions(cfdata);
   _update_binding_params(cfdata);
   _update_buttons(cfdata);
}
static void
_add_mouse_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;

   if (cfdata->locals.mousebind_win != 0) return;

   cfdata->locals.add = 1;
   cfdata->locals.dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
				     "E", "_mousebind_getmouse_dialog");
   if (!cfdata->locals.dia) return;
   e_dialog_title_set(cfdata->locals.dia, _("Mouse Binding Sequence"));
   e_dialog_icon_set(cfdata->locals.dia, "enlightenment/mouse_clean", 64);
   e_dialog_text_set(cfdata->locals.dia, TEXT_PRESS_MOUSE_BINIDING_SEQUENCE);
   e_win_centered_set(cfdata->locals.dia->win, 1);
   e_win_borderless_set(cfdata->locals.dia->win, 1);

   cfdata->locals.mousebind_win = ecore_x_window_input_new(e_manager_current_get()->root,
							   0, 0, 1, 1);
   ecore_x_window_show(cfdata->locals.mousebind_win);
   e_grabinput_get(cfdata->locals.mousebind_win, 0, cfdata->locals.mousebind_win);

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
				 _grab_key_down_cb, cfdata));

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
				 _grab_mouse_down_cb, cfdata));

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
				 _grab_mouse_wheel_cb, cfdata));

   e_dialog_show(cfdata->locals.dia);
}
static void
_modify_mouse_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;

   if (cfdata->locals.mousebind_win != 0) return;

   cfdata->locals.add = 0;
   cfdata->locals.dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
				     "E", "_mousebind_getmouse_dialog");
   if (!cfdata->locals.dia) return;
   e_dialog_title_set(cfdata->locals.dia, _("Mouse Binding Sequence"));
   e_dialog_icon_set(cfdata->locals.dia, "enlightenment/mouse_clean", 64);
   e_dialog_text_set(cfdata->locals.dia, TEXT_PRESS_MOUSE_BINIDING_SEQUENCE);
   e_win_centered_set(cfdata->locals.dia->win, 1);
   e_win_borderless_set(cfdata->locals.dia->win, 1);

   cfdata->locals.mousebind_win = ecore_x_window_input_new(e_manager_current_get()->root,
							   0, 0, 1, 1);
   ecore_x_window_show(cfdata->locals.mousebind_win);
   e_grabinput_get(cfdata->locals.mousebind_win, 0, cfdata->locals.mousebind_win);

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
				 _grab_key_down_cb, cfdata));

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
				 _grab_mouse_down_cb, cfdata));

   cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
				 _grab_mouse_wheel_cb, cfdata));

   e_dialog_show(cfdata->locals.dia);
}
static void
_delete_mouse_binding_cb(void *data, void *data2)
{
   Evas_List *l;
   char *n;
   int sel;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   cfdata = data;

   sel = e_widget_ilist_selected_get(cfdata->gui.o_binding_list);
   if (cfdata->locals.binding[0] == 'm') 
     {
	n = cfdata->locals.binding;
	l = evas_list_nth_list(cfdata->binding.mouse, atoi(++n));

	if (l)
	  { 
	     eb = l->data; 
	     if (eb->action) evas_stringshare_del(eb->action); 
	     if (eb->params) evas_stringshare_del(eb->params); 
	     E_FREE(eb); 
	     cfdata->binding.mouse = evas_list_remove_list(cfdata->binding.mouse, l);
	  }
     }
   else
     {
	n = cfdata->locals.binding;
	l = evas_list_nth_list(cfdata->binding.wheel, atoi(++n));

	if (l)
	  {
	     bw = l->data;
	     if (bw->action) evas_stringshare_del(bw->action);
	     if (bw->params) evas_stringshare_del(bw->params);
	     E_FREE(bw);

	     cfdata->binding.wheel = evas_list_remove_list(cfdata->binding.wheel, l);
	  }
     }

   _update_binding_list(cfdata);
   if (sel >= e_widget_ilist_count(cfdata->gui.o_binding_list))
     sel = e_widget_ilist_count(cfdata->gui.o_binding_list) - 1;


   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;
   if (!e_widget_ilist_count(cfdata->gui.o_binding_list))
     { 
	_update_binding_context(cfdata);
	_update_binding_params(cfdata);
	_update_buttons(cfdata);
	e_widget_ilist_selected_set(cfdata->gui.o_action_list, 0);
     }
   else 
     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel);
}
static void
_restore_default_cb(void *data, void *data2)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   while (cfdata->binding.mouse)
     {
	eb = cfdata->binding.mouse->data;
	if (eb->action) evas_stringshare_del(eb->action);
	if (eb->params) evas_stringshare_del(eb->params);
	E_FREE(eb);
	cfdata->binding.mouse =
	   evas_list_remove_list(cfdata->binding.mouse, cfdata->binding.mouse);
     }

   while (cfdata->binding.wheel)
     {
	bw = cfdata->binding.wheel->data;
	if (bw->action) evas_stringshare_del(bw->action);
	if (bw->params) evas_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->binding.wheel =
	   evas_list_remove_list(cfdata->binding.wheel, cfdata->binding.wheel);
     }
#define CFG_MOUSEBIND(_context, _button, _modifiers, _anymod, _action, _params) \
   eb = E_NEW(E_Config_Binding_Mouse, 1); \
   eb->context = _context; \
   eb->button = _button; \
   eb->modifiers = _modifiers; \
   eb->any_mod = _anymod; \
   eb->action = _action == NULL ? NULL : evas_stringshare_add(_action); \
   eb->params = _params == NULL ? NULL : evas_stringshare_add(_params); \
   cfdata->binding.mouse = evas_list_append(cfdata->binding.mouse, eb) 
   
   CFG_MOUSEBIND(E_BINDING_CONTEXT_BORDER, 1, E_BINDING_MODIFIER_ALT, 0, "window_move", NULL); 
   CFG_MOUSEBIND(E_BINDING_CONTEXT_BORDER, 2, E_BINDING_MODIFIER_ALT, 0, "window_resize", NULL); 
   CFG_MOUSEBIND(E_BINDING_CONTEXT_BORDER, 3, E_BINDING_MODIFIER_ALT, 0, "window_menu", NULL); 
   CFG_MOUSEBIND(E_BINDING_CONTEXT_ZONE, 1, 0, 0, "menu_show", "main"); 
   CFG_MOUSEBIND(E_BINDING_CONTEXT_ZONE, 2, 0, 0, "menu_show", "clients"); 
   CFG_MOUSEBIND(E_BINDING_CONTEXT_ZONE, 3, 0, 0, "menu_show", "favorites");


#define CFG_WHEELBIND(_context, _direction, _z, _modifiers, _anymod, _action, _params) \
   bw = E_NEW(E_Config_Binding_Wheel, 1); \
   bw->context = _context; \
   bw->direction = _direction; \
   bw->z = _z; \
   bw->modifiers = _modifiers; \
   bw->any_mod = _anymod; \
   bw->action = _action == NULL ? NULL : evas_stringshare_add(_action); \
   bw->params = _params == NULL ? NULL : evas_stringshare_add(_params); \
   cfdata->binding.wheel = evas_list_append(cfdata->binding.wheel, bw) 
   
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 0, -1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 1, -1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 0, 1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 1, 1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_POPUP, 0, -1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_POPUP, 1, -1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_POPUP, 0, 1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_POPUP, 1, 1, E_BINDING_MODIFIER_NONE, 1, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 0, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 1, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 0, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_CONTAINER, 1, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_BORDER, 0, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_BORDER, 1, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_BORDER, 0, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND(E_BINDING_CONTEXT_BORDER, 1, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1");

   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   _update_binding_list(cfdata);
   _update_buttons(cfdata);
   _update_binding_params(cfdata);
   _update_binding_context(cfdata);
   e_widget_ilist_selected_set(cfdata->gui.o_action_list, 0);
}

static int
_grab_mouse_down_cb(void *data, int type, void *event)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *eb;
   int mod = E_BINDING_MODIFIER_NONE;
   Ecore_X_Event_Mouse_Button_Down *ev = event;

   cfdata = data;
   if (ev->win != cfdata->locals.mousebind_win) return 1; 

   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT)
     mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL)
     mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT)
     mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN)
     mod |= E_BINDING_MODIFIER_WIN; 
   
   
   if (cfdata->locals.add) 
     { 
	eb = E_NEW(E_Config_Binding_Mouse, 1); 
	eb->context = E_BINDING_CONTEXT_ANY; 
	eb->button = ev->button; 
	eb->modifiers = mod; 
	eb->any_mod = 0; 
	eb->action = evas_stringshare_add(action_to_name[0][0]); 
	eb->params = NULL; 
	
	cfdata->binding.mouse = evas_list_append(cfdata->binding.mouse, eb);
     }
   else
     {
	char *n = cfdata->locals.cur; 
	
	eb = evas_list_nth(cfdata->binding.mouse, atoi(++n));
	if (eb)
	  {
	     eb->button = ev->button;
	     eb->modifiers = mod;
	  }
     } 
   _update_binding_list(cfdata); 
   _update_binding_list_selection(cfdata, eb); 

   _grab_wnd_hide_cb(cfdata);
   _update_buttons(cfdata);
   return 1;
}
static int _grab_mouse_wheel_cb(void *data, int type, void *event)
{
   E_Config_Binding_Wheel *bw;
   E_Config_Dialog_Data *cfdata;
   Ecore_X_Event_Mouse_Wheel *ev = event;
   int mod = E_BINDING_MODIFIER_NONE;

   cfdata = data;

   if (ev->win != cfdata->locals.mousebind_win) return 1;

   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT)
     mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL)
     mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT)
     mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN)
     mod |= E_BINDING_MODIFIER_WIN; 
   
   
   if (cfdata->locals.add)
     {
	bw = E_NEW(E_Config_Binding_Wheel, 1);
	bw->context = E_BINDING_CONTEXT_ANY;
	bw->direction = ev->direction;
	bw->z = ev->z;
	bw->modifiers = mod;
	bw->any_mod = 0;
	bw->action = evas_stringshare_add(action_to_name[0][0]);
	bw->params = NULL; 
	
	cfdata->binding.wheel = evas_list_append(cfdata->binding.wheel, bw);
     }
   else 
     {
	char *n = cfdata->locals.cur; 
	
	bw = evas_list_nth(cfdata->binding.wheel, atoi(++n));
	if (bw)
	  {
	     bw->direction = ev->direction;
	     bw->z = ev->z;
	     bw->modifiers = mod;
	  }
     } 
   _update_binding_list(cfdata); 
   _update_binding_list_selection(cfdata, bw);

   _grab_wnd_hide_cb(cfdata);
   _update_buttons(cfdata);

   return 1;
}
static int
_grab_key_down_cb(void *data, int type, void *event)
{
   E_Config_Dialog_Data *cfdata;
   Ecore_X_Event_Key_Down *ev = event;

   cfdata = data;

   if (ev->win != cfdata->locals.mousebind_win) return 1;

   if (!strcmp(ev->keyname, "Escape") &&
       !(ev->modifiers & ECORE_X_MODIFIER_SHIFT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_CTRL) &&
       !(ev->modifiers & ECORE_X_MODIFIER_ALT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_WIN))
     { 
	_grab_wnd_hide_cb(cfdata);
     }
   return 1;
}
static void
_grab_wnd_hide_cb(E_Config_Dialog_Data *cfdata)
{
   while (cfdata->locals.handlers)
     {
	ecore_event_handler_del(cfdata->locals.handlers->data);
	cfdata->locals.handlers = evas_list_remove_list(cfdata->locals.handlers,
							cfdata->locals.handlers);
     }
   cfdata->locals.handlers = NULL;
   e_grabinput_release(cfdata->locals.mousebind_win, cfdata->locals.mousebind_win);
   ecore_x_window_del(cfdata->locals.mousebind_win);
   cfdata->locals.mousebind_win = 0;

   e_object_del(E_OBJECT(cfdata->locals.dia));
   cfdata->locals.dia = NULL;
}
/******** helpers *************/
static char *
_helper_modifier_name_get(int mod)
{
   char mods[1024]="";

   if (mod & E_BINDING_MODIFIER_SHIFT)
     snprintf(mods, sizeof(mods), "SHIFT");

   if (mod & E_BINDING_MODIFIER_CTRL)
     {
	if (mods[0]) strcat(mods, " + ");
	strcat(mods, "CTRL");
     }

   if (mod & E_BINDING_MODIFIER_ALT)
     {
	if (mods[0]) strcat(mods, " + ");
	strcat(mods, "ALT");
     }

   if (mod & E_BINDING_MODIFIER_WIN)
     {
	if (mods[0]) strcat(mods, " + ");
	strcat(mods, "WIN");
     }

   return strdup(mods);
}
static char *
_helper_button_name_get(E_Config_Binding_Mouse *eb)
{
   char *name = NULL;
   char buf[1024]="";

   switch (eb->button)
     { 
      case 1:
	 name = strdup("Left Button");
	 break;
      case 2:
	 name = strdup("Middle Button");
	 break;
      case 3: 
	 name = strdup("Right Button");
	 break;
      case 4:
      case 5:
      case 6:
      case 7:
	 break;
      default:
	 snprintf(buf, sizeof(buf), "Extra Button (%d)", eb->button);
	 name = strdup(buf);
     }
   return name;
}
static char *
_helper_wheel_name_get(E_Config_Binding_Wheel *bw)
{
   char *name = NULL;
   char buf[1024] = "";

   switch (bw->direction)
     {
      case 0:
	 if (bw->z >= 0) 
	   name = strdup("Mouse Wheel Up");
	 else
	   name = strdup("Mouse Wheel Down");
	 break;
      default:
	 if (bw->z >= 0) 
	   snprintf(buf, sizeof(buf), "Extra Wheel (%d) Up", bw->direction);
	 else
	   snprintf(buf, sizeof(buf), "Extra Wheel (%d) Down", bw->direction);
	 name = strdup(buf);
     }
   return name;
}
/********* sorts ***************/
static int
_mouse_binding_sort_cb(void *d1, void *d2)
{
   E_Config_Binding_Mouse *eb, *eb2;

   eb = d1;
   eb2 = d2;

   if (eb->button < eb2->button) return -1;
   else if (eb->button > eb2->button) return 1;
   else
     {
	if (eb->modifiers < eb2->modifiers) return -1;
	else if (eb->modifiers > eb2->modifiers) return 1;
     }
   return 0;
}
static int
_wheel_binding_sort_cb(void *d1, void *d2)
{
   E_Config_Binding_Wheel *bw, *bw2;

   bw = d1;
   bw2 = d2;

   if (bw->direction < bw2->direction)
     {
	return -1;
     }
   else if (bw->direction > bw2->direction)
     {
	return 1;
     }
   else
     {
	if ((bw->z < 0) && (bw2->z > 0))
	  {
	     return 1;
	  }
	else if ((bw->z > 0) && (bw2->z < 0))
	  {
	     return -1;
	  }
	else if (((bw->z < 0) && (bw2->z < 0)) ||
	         ((bw->z > 0) && (bw2->z > 0)))
	  {
	     if (bw->modifiers < bw2->modifiers)
	       return -1;
	     else if (bw->modifiers > bw2->modifiers)
	       return 1;
	  }
     }
   return 0;
}

#include "e.h"

#define TEXT_NO_PARAMS _("<None>")
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

/******************* Callbacks *************/
static void _binding_change_cb(void *data);
static void _action_change_cb(void *data);
static void _delete_mouse_binding_cb(void *data, void *data2);
static void _delete_all_mouse_binding_cb(void *data, void *data2);
static void _restore_mouse_binding_defaults_cb(void *data, void *data2);
static void _add_mouse_binding_cb(void *data, void *data2);
static void _modify_mouse_binding_cb(void *data, void *data2);

/******************* Updates *****************/
static void _update_action_list(E_Config_Dialog_Data *cfdata);
static void _update_action_params(E_Config_Dialog_Data *cfdata);
static void _update_mouse_binding_list(E_Config_Dialog_Data *cfdata);
static void _update_buttons(E_Config_Dialog_Data *cfdata);
static void _update_binding_context(E_Config_Dialog_Data *cfdata);

/****************** Helper *****************/
static void _find_key_binding_action(const char *action, const char *params, int *g, int *a, int *n);
static char *_helper_button_name_get(E_Config_Binding_Mouse *eb);
static char *_helper_wheel_name_get(E_Config_Binding_Wheel *bw);
static char *_helper_modifier_name_get(int mod);
static void _auto_apply_changes(E_Config_Dialog_Data *cfdata);

/********* Sorting ***************/
static int _mouse_binding_sort_cb(const void *d1, const void *d2);
static int _wheel_binding_sort_cb(const void *d1, const void *d2);

/********* grab window **********/
static void _grab_wnd_show(E_Config_Dialog_Data *cfdata);
static void _grab_wnd_hide(E_Config_Dialog_Data *cfdata);
static int _grab_mouse_down_cb(void *data, int type, void *event);
static int _grab_mouse_wheel_cb(void *data, int type, void *event);
static int _grab_key_down_cb(void *data, int type, void *event);

struct _E_Config_Dialog_Data
{
  E_Config_Dialog *cfd;

  Evas *evas;

  struct
    {
       Eina_List *mouse;
       Eina_List *wheel;
    } binding;

  struct
    {
       char *binding;
       char *action;
       char *params;
       int context;

       char *cur;
       int add; /*just to distinguesh among two buttons add/modify */

       E_Dialog *dia;
       Ecore_X_Window bind_win;
       Eina_List *handlers;
    } locals;

  struct 
    { 
       Evas_Object *o_binding_list;
       Evas_Object *o_action_list;
       Evas_Object *o_params;
       Evas_Object *o_del;
       Evas_Object *o_mod;
       Evas_Object *o_del_all;
       struct {
	 Evas_Object *o_any, *o_border, *o_menu, *o_winlist, *o_popup, *o_zone,
		     *o_container, *o_manager, *o_none;
       } context;
    } gui;
};

EAPI E_Config_Dialog *
e_int_config_mousebindings(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_mousebindings_dialog")) return NULL;
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
   Eina_List *l;
   E_Config_Binding_Mouse *eb, *eb2;
   E_Config_Binding_Wheel *bw, *bw2;

   cfdata->locals.binding = strdup("");
   cfdata->locals.action = strdup("");
   cfdata->locals.params = strdup("");
   cfdata->locals.context = E_BINDING_CONTEXT_ANY;
   cfdata->binding.mouse = NULL;
   cfdata->binding.wheel = NULL;
   cfdata->locals.bind_win = 0;
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
	eb2->action = eb->action == NULL ? NULL : eina_stringshare_add(eb->action);
	eb2->params = eb->params == NULL ? NULL : eina_stringshare_add(eb->params);

	cfdata->binding.mouse = eina_list_append(cfdata->binding.mouse, eb2);
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
	bw2->action = bw->action == NULL ? NULL : eina_stringshare_add(bw->action);
	bw2->params = bw->params == NULL ? NULL : eina_stringshare_add(bw->params);

	cfdata->binding.wheel = eina_list_append(cfdata->binding.wheel, bw2);
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
	if (eb->action) eina_stringshare_del(eb->action);
	if (eb->params) eina_stringshare_del(eb->params);
	E_FREE(eb);
	cfdata->binding.mouse =
	   eina_list_remove_list(cfdata->binding.mouse, cfdata->binding.mouse);
     }

   while (cfdata->binding.wheel)
     {
	bw = cfdata->binding.wheel->data;
	if (bw->action) eina_stringshare_del(bw->action);
	if (bw->params) eina_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->binding.wheel =
	   eina_list_remove_list(cfdata->binding.wheel, cfdata->binding.wheel);
     }

   if (cfdata->locals.binding) free(cfdata->locals.binding);
   if (cfdata->locals.action) free(cfdata->locals.action);
   if (cfdata->locals.params) free(cfdata->locals.params);
   if (cfdata->locals.cur) free(cfdata->locals.cur);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ 
   Eina_List *l;
   E_Config_Binding_Mouse *eb, *eb2;
   E_Config_Binding_Wheel *bw, *bw2;

   _auto_apply_changes(cfdata);

   e_border_button_bindings_ungrab_all();
   while (e_config->mouse_bindings)
     {
	eb = e_config->mouse_bindings->data;
	e_bindings_mouse_del(eb->context, eb->button, eb->modifiers, eb->any_mod,
			     eb->action, eb->params);
	if (eb->action) eina_stringshare_del(eb->action);
	if (eb->params) eina_stringshare_del(eb->params);
	E_FREE(eb);
	e_config->mouse_bindings = 
	   eina_list_remove_list(e_config->mouse_bindings, e_config->mouse_bindings);
     }

   for (l = cfdata->binding.mouse; l; l = l->next)
     {
	eb = l->data;

	eb2 = E_NEW(E_Config_Binding_Mouse, 1);
	eb2->context = eb->context;
	eb2->button = eb->button;
	eb2->modifiers = eb->modifiers;
	eb2->any_mod = eb->any_mod;
	eb2->action = eb->action == NULL ? NULL : eina_stringshare_add(eb->action);
	eb2->params = eb->params == NULL ? NULL : eina_stringshare_add(eb->params);

	e_config->mouse_bindings = eina_list_append(e_config->mouse_bindings, eb2);
	e_bindings_mouse_add(eb2->context, eb2->button, eb2->modifiers, eb2->any_mod,
			     eb2->action, eb2->params);
     }

   while (e_config->wheel_bindings)
     {
	bw = e_config->wheel_bindings->data;
	e_bindings_wheel_del(bw->context, bw->direction, bw->z, bw->modifiers, bw->any_mod,
			     bw->action, bw->params);
	if (bw->action) eina_stringshare_del(bw->action);
	if (bw->params) eina_stringshare_del(bw->params);
	E_FREE(bw);

	e_config->wheel_bindings = 
	   eina_list_remove_list(e_config->wheel_bindings, e_config->wheel_bindings);
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
	bw2->action = bw->action == NULL ? NULL : eina_stringshare_add(bw->action);
	bw2->params = bw->params == NULL ? NULL : eina_stringshare_add(bw->params);

	e_config->wheel_bindings = eina_list_append(e_config->wheel_bindings, bw2);
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
   Evas_Object *o, *of, *ot, *ob;
   E_Radio_Group *rg;
   
   cfdata->evas = evas;
   o = e_widget_list_add(evas, 0, 1);
   ot = e_widget_frametable_add(evas, _("Mouse Bindings"), 0);
   ob = e_widget_ilist_add(evas, 32, 32, &(cfdata->locals.binding));
   cfdata->gui.o_binding_list = ob;
   e_widget_min_size_set(ob, 200, 200);
   e_widget_frametable_object_append(ot, ob, 0, 0, 2, 1, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Add Binding"), NULL, _add_mouse_binding_cb, cfdata, NULL);
   e_widget_frametable_object_append(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete Binding"), NULL, _delete_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Modify Binding"), NULL, _modify_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_mod = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(ot, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete All"), NULL, _delete_all_mouse_binding_cb, cfdata, NULL);
   cfdata->gui.o_del_all = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(ot, ob, 1, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Restore Mouse and Wheel Binding Defaults"), "enlightenment/e",
			    _restore_mouse_binding_defaults_cb, cfdata, NULL);
   e_widget_frametable_object_append(ot, ob, 0, 3, 2, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->locals.action));
   cfdata->gui.o_action_list = ob;
   e_widget_min_size_set(ob, 200, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 3, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Action Params"), 0);
   ob = e_widget_entry_add(evas, &(cfdata->locals.params), NULL, NULL, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->gui.o_params = ob;
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 3, 1, 1, 1, 1, 0);

   of = e_widget_frametable_add(evas, _("Action Context"), 1);
   rg = e_widget_radio_group_new(&(cfdata->locals.context));
   ob = e_widget_radio_add(evas, _("Any"), E_BINDING_CONTEXT_ANY, rg);
   cfdata->gui.context.o_any = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Border"), E_BINDING_CONTEXT_BORDER, rg);
   cfdata->gui.context.o_border = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Menu"), E_BINDING_CONTEXT_MENU, rg);
   cfdata->gui.context.o_menu = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Win List"), E_BINDING_CONTEXT_WINLIST, rg);
   cfdata->gui.context.o_winlist = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Popup"), E_BINDING_CONTEXT_POPUP, rg);
   cfdata->gui.context.o_popup = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Zone"), E_BINDING_CONTEXT_ZONE, rg);
   cfdata->gui.context.o_zone = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Container"), E_BINDING_CONTEXT_CONTAINER, rg);
   cfdata->gui.context.o_container = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 2, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("Manager"), E_BINDING_CONTEXT_MANAGER, rg);
   cfdata->gui.context.o_manager = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 2, 1, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_add(evas, _("None"), E_BINDING_CONTEXT_NONE, rg);
   cfdata->gui.context.o_none = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 2, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, of, 0, 2, 3, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   
   _update_mouse_binding_list(cfdata);
   _fill_actions_list(cfdata);
   
   e_dialog_resizable_set(cfd->dia, 1);
   return o;
}

static void
_fill_actions_list(E_Config_Dialog_Data *cfdata)
{
   char buf[1024];
   Eina_List *l, *l2;
   E_Action_Group *actg;
   E_Action_Description *actd;
   int g, a;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_action_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_action_list);
   
   e_widget_ilist_clear(cfdata->gui.o_action_list);
   for (l = e_action_groups_get(), g = 0; l; l = l->next, g++)
     {
	actg = l->data;

	if (!actg->acts) continue;

	e_widget_ilist_header_append(cfdata->gui.o_action_list, NULL, actg->act_grp);

	for (l2 = actg->acts, a = 0; l2; l2 = l2->next, a++)
	  {
	     actd = l2->data;

	     snprintf(buf, sizeof(buf), "%d %d", g, a);
	     e_widget_ilist_append(cfdata->gui.o_action_list, NULL, actd->act_name,
				   _action_change_cb, cfdata, buf);
	  }
     }
   e_widget_ilist_go(cfdata->gui.o_action_list);
   e_widget_ilist_thaw(cfdata->gui.o_action_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_action_list));
}

/******************* Callbacks *************/
static void
_add_mouse_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _auto_apply_changes(cfdata);

   cfdata->locals.add = 1;
   _grab_wnd_show(cfdata);
}

static void
_modify_mouse_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _auto_apply_changes(cfdata);

   cfdata->locals.add = 0;
   _grab_wnd_show(cfdata);
}

static void
_action_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _update_action_params(cfdata);
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

   _update_buttons(cfdata);
   _update_action_list(cfdata);
   _update_binding_context(cfdata);
}

static void
_delete_all_mouse_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   cfdata = data;

   /* FIXME: need confirmation dialog */
   
   while (cfdata->binding.mouse)
     {
	eb = cfdata->binding.mouse->data;
	if (eb->action) eina_stringshare_del(eb->action);
	if (eb->params) eina_stringshare_del(eb->params);
	E_FREE(eb);
	cfdata->binding.mouse =
	   eina_list_remove_list(cfdata->binding.mouse, cfdata->binding.mouse);
     }

   while (cfdata->binding.wheel)
     {
	bw = cfdata->binding.wheel->data;
	if (bw->action) eina_stringshare_del(bw->action);
	if (bw->params) eina_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->binding.wheel =
	   eina_list_remove_list(cfdata->binding.wheel, cfdata->binding.wheel);
     }

   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);
   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);

   _update_buttons(cfdata);
   _update_binding_context(cfdata);
}

static void
_delete_mouse_binding_cb(void *data, void *data2)
{
   Eina_List *l;
   int sel, n;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   cfdata = data;

   sel = e_widget_ilist_selected_get(cfdata->gui.o_binding_list);
   if (cfdata->locals.binding[0] == 'm')
     {
	sscanf(cfdata->locals.binding, "m%d", &n);
	l = eina_list_nth_list(cfdata->binding.mouse, n);
	if (l)
	  {
	     eb = l->data;
	     if (eb->action) eina_stringshare_del(eb->action);
	     if (eb->params) eina_stringshare_del(eb->params);
	     E_FREE(eb);
	     cfdata->binding.mouse = eina_list_remove_list(cfdata->binding.mouse, l);
	  }
     }
   else if (cfdata->locals.binding[0] == 'w')
     {
	sscanf(cfdata->locals.binding, "w%d", &n);
	l = eina_list_nth_list(cfdata->binding.wheel, n);
	if (l)
	  {
	     bw = l->data;
	     if (bw->action) eina_stringshare_del(bw->action);
	     if (bw->params) eina_stringshare_del(bw->params);
	     E_FREE(bw);
	     cfdata->binding.wheel = eina_list_remove_list(cfdata->binding.wheel, l);
	  }
     }
   else
     return;

   _update_mouse_binding_list(cfdata);
   if (sel >= e_widget_ilist_count(cfdata->gui.o_binding_list))
     sel = e_widget_ilist_count(cfdata->gui.o_binding_list) - 1;


   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;
   if (!e_widget_ilist_count(cfdata->gui.o_binding_list))
     { 
	
	_update_binding_context(cfdata);
	_update_buttons(cfdata); 
	
	e_widget_ilist_unselect(cfdata->gui.o_action_list);
	e_widget_entry_clear(cfdata->gui.o_params);
	e_widget_disabled_set(cfdata->gui.o_params, 1);
     }
   else 
     { 
	if (e_widget_ilist_nth_is_header(cfdata->gui.o_binding_list, sel)) sel++;
	e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel);
     }
}

static void
_restore_mouse_binding_defaults_cb(void *data, void *data2)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   while (cfdata->binding.mouse)
     {
	eb = cfdata->binding.mouse->data;
	if (eb->action) eina_stringshare_del(eb->action);
	if (eb->params) eina_stringshare_del(eb->params);
	E_FREE(eb);
	cfdata->binding.mouse =
	   eina_list_remove_list(cfdata->binding.mouse, cfdata->binding.mouse);
     }

   while (cfdata->binding.wheel)
     {
	bw = cfdata->binding.wheel->data;
	if (bw->action) eina_stringshare_del(bw->action);
	if (bw->params) eina_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->binding.wheel =
	   eina_list_remove_list(cfdata->binding.wheel, cfdata->binding.wheel);
     }
#define CFG_MOUSEBIND_DFLT(_context, _button, _modifiers, _anymod, _action, _params) \
   eb = E_NEW(E_Config_Binding_Mouse, 1); \
   eb->context = _context; \
   eb->button = _button; \
   eb->modifiers = _modifiers; \
   eb->any_mod = _anymod; \
   eb->action = _action == NULL ? NULL : eina_stringshare_add(_action); \
   eb->params = _params == NULL ? NULL : eina_stringshare_add(_params); \
   cfdata->binding.mouse = eina_list_append(cfdata->binding.mouse, eb) 
   
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_BORDER, 1, E_BINDING_MODIFIER_ALT, 0, "window_move", NULL); 
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_BORDER, 2, E_BINDING_MODIFIER_ALT, 0, "window_resize", NULL); 
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_BORDER, 3, E_BINDING_MODIFIER_ALT, 0, "window_menu", NULL); 
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_ZONE, 1, 0, 0, "menu_show", "main"); 
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_ZONE, 2, 0, 0, "menu_show", "clients"); 
   CFG_MOUSEBIND_DFLT(E_BINDING_CONTEXT_ZONE, 3, 0, 0, "menu_show", "favorites");

#define CFG_WHEELBIND_DFLT(_context, _direction, _z, _modifiers, _anymod, _action, _params) \
   bw = E_NEW(E_Config_Binding_Wheel, 1); \
   bw->context = _context; \
   bw->direction = _direction; \
   bw->z = _z; \
   bw->modifiers = _modifiers; \
   bw->any_mod = _anymod; \
   bw->action = _action == NULL ? NULL : eina_stringshare_add(_action); \
   bw->params = _params == NULL ? NULL : eina_stringshare_add(_params); \
   cfdata->binding.wheel = eina_list_append(cfdata->binding.wheel, bw) 
   
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_CONTAINER, 0, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_CONTAINER, 1, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_CONTAINER, 0, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_CONTAINER, 1, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_BORDER, 0, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_BORDER, 1, -1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "-1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_BORDER, 0, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1"); 
   CFG_WHEELBIND_DFLT(E_BINDING_CONTEXT_BORDER, 1, 1, E_BINDING_MODIFIER_ALT, 0, 
	 "desk_linear_flip_by", "1");

   if (cfdata->locals.cur) free(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   _update_mouse_binding_list(cfdata);
   _update_buttons(cfdata);
   _update_binding_context(cfdata);

   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);
}

/******************* Updates *****************/
static void
_update_action_list(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   const char *action, *params;
   int j = -1, i, n;

   if (!cfdata->locals.cur) return;

   if (cfdata->locals.cur[0] == 'm')
     {
	sscanf(cfdata->locals.cur, "m%d", &n);
	eb = eina_list_nth(cfdata->binding.mouse, n);
	if (!eb)
	  {
	     e_widget_ilist_unselect(cfdata->gui.o_action_list);
	     e_widget_entry_clear(cfdata->gui.o_params);
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     return;
	  }
	action = eb->action;
	params = eb->params;
     }
   else if (cfdata->locals.cur[0] == 'w')
     {
	sscanf(cfdata->locals.cur, "w%d", &n);
	bw = eina_list_nth(cfdata->binding.wheel, n);
	if (!bw)
	  {
	     e_widget_ilist_unselect(cfdata->gui.o_action_list);
	     e_widget_entry_clear(cfdata->gui.o_params);
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     return;
	  }
	action = bw->action;
	params = bw->params;
     }
   else
     return;

   _find_key_binding_action(action, params, NULL, NULL, &j);
   if (j >= 0)
     {
	for (i = 0; i < e_widget_ilist_count(cfdata->gui.o_action_list); i++)
	  {
	     if (i > j) break;
	     if (e_widget_ilist_nth_is_header(cfdata->gui.o_action_list, i)) j++;
	  }
     }

   if (j >= 0)
     {
	if (j == e_widget_ilist_selected_get(cfdata->gui.o_action_list))
	  _update_action_params(cfdata);
	else
	  e_widget_ilist_selected_set(cfdata->gui.o_action_list, j);
     }
   else
     {
	e_widget_ilist_unselect(cfdata->gui.o_action_list);
	if (cfdata->locals.action) free(cfdata->locals.action);
	cfdata->locals.action = strdup("");
	e_widget_entry_clear(cfdata->gui.o_params);
     }
}

static void
_update_mouse_binding_list(E_Config_Dialog_Data *cfdata)
{
   char *icon = NULL, *button, *mods;
   char label[1024], val[10];
   int i;
   Eina_List *l;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_binding_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_binding_list);
   
   e_widget_ilist_clear(cfdata->gui.o_binding_list);

   if (cfdata->binding.mouse) 
     { 
	cfdata->binding.mouse = eina_list_sort(cfdata->binding.mouse, 
	      eina_list_count(cfdata->binding.mouse), _mouse_binding_sort_cb);
	
	e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, "Mouse Buttons");
     }

   for (l = cfdata->binding.mouse, i = 0; l; l = l->next, i++)
     {
	Evas_Object *ic;
	eb = l->data;

	button = _helper_button_name_get(eb);
	mods = _helper_modifier_name_get(eb->modifiers);

	if (mods && mods[0])
	  snprintf(label, sizeof(label), "%s + %s", button ? button : "", mods);
	else
	  snprintf(label, sizeof(label), "%s", button ? button : "");
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
	cfdata->binding.wheel = eina_list_sort(cfdata->binding.wheel, 
	      eina_list_count(cfdata->binding.wheel), _wheel_binding_sort_cb); 
	
	e_widget_ilist_header_append(cfdata->gui.o_binding_list, NULL, "Mouse Wheels");
     }

   for (l = cfdata->binding.wheel, i = 0; l; l = l->next, i++)
     {
	Evas_Object *ic;
	bw = l->data;

	button = _helper_wheel_name_get(bw);
	mods = _helper_modifier_name_get(bw->modifiers);

	if (mods && mods[0])
	  snprintf(label, sizeof(label), "%s + %s", button ? button : "", mods);
	else
	  snprintf(label, sizeof(label), "%s", button ? button : "");
	if (button) free(button);
	if (mods) free(mods);

	snprintf(val, sizeof(val), "w%d", i);

	ic = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(ic, "enlightenment/mouse_wheel");
	e_widget_ilist_append(cfdata->gui.o_binding_list, ic, label, _binding_change_cb,
			      cfdata, val);
     }

   e_widget_ilist_go(cfdata->gui.o_binding_list);
   e_widget_ilist_thaw(cfdata->gui.o_binding_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_binding_list));

   if (eina_list_count(cfdata->binding.mouse) + eina_list_count(cfdata->binding.wheel))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);
}

static void
_update_action_params(E_Config_Dialog_Data *cfdata)
{
   int g, a, b;
   E_Action_Group *actg;
   E_Action_Description *actd;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   const char *action, *params;

#define MB_EXAMPLE_PARAMS \
   if ((!actd->param_example) || (!actd->param_example[0])) \
     e_widget_entry_text_set(cfdata->gui.o_params, TEXT_NO_PARAMS); \
   else \
     e_widget_entry_text_set(cfdata->gui.o_params, actd->param_example)

   if ((!cfdata->locals.action) || (!cfdata->locals.action[0]))
     {
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	e_widget_entry_clear(cfdata->gui.o_params);
     }
   sscanf(cfdata->locals.action, "%d %d", &g, &a);

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   if (actd->act_params)
     {
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	e_widget_entry_text_set(cfdata->gui.o_params, actd->act_params);
	return;
     }

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0]))
     {
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	MB_EXAMPLE_PARAMS;
	return;
     }

   if (!actd->editable)
     e_widget_disabled_set(cfdata->gui.o_params, 1);
   else
     e_widget_disabled_set(cfdata->gui.o_params, 0);

   if (cfdata->locals.cur[0] == 'm')
     {
	sscanf(cfdata->locals.cur, "m%d", &b);
	eb = eina_list_nth(cfdata->binding.mouse, b);
	if (!eb)
	  {
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     MB_EXAMPLE_PARAMS;
	     return;
	  }
	action = eb->action;
	params = eb->params;
     }
   else if (cfdata->locals.cur[0] == 'w')
     {
	sscanf(cfdata->locals.cur, "w%d", &b);
	bw = eina_list_nth(cfdata->binding.wheel, b);
	if (!bw)
	  {
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     MB_EXAMPLE_PARAMS;
	     return;
	  }
	action = bw->action;
	params = bw->params;
     }
   else
     { 
	e_widget_disabled_set(cfdata->gui.o_params, 1);
        MB_EXAMPLE_PARAMS;
	return;
     }

   if (action)
     {
	if (!strcmp(action, actd->act_cmd))
	  {
	     if ((!params) || (!params[0]))
	       MB_EXAMPLE_PARAMS;
	     else
	       e_widget_entry_text_set(cfdata->gui.o_params, params);
	  }
	else
	  MB_EXAMPLE_PARAMS;
     }
   else
     MB_EXAMPLE_PARAMS;
}

static void
_update_buttons(E_Config_Dialog_Data *cfdata)
{
   if (e_widget_ilist_count(cfdata->gui.o_binding_list))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);

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
   int n;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   E_Binding_Context ctxt;

   /* disable all the radios. */
   e_widget_radio_toggle_set(cfdata->gui.context.o_any, 1);
   e_widget_disabled_set(cfdata->gui.context.o_any, 1);
   e_widget_disabled_set(cfdata->gui.context.o_border, 1);
   e_widget_disabled_set(cfdata->gui.context.o_menu, 1);
   e_widget_disabled_set(cfdata->gui.context.o_winlist, 1);
   e_widget_disabled_set(cfdata->gui.context.o_popup, 1);
   e_widget_disabled_set(cfdata->gui.context.o_zone, 1);
   e_widget_disabled_set(cfdata->gui.context.o_container, 1);
   e_widget_disabled_set(cfdata->gui.context.o_manager, 1);
   e_widget_disabled_set(cfdata->gui.context.o_none, 1);

   if (!cfdata->locals.cur) return;

   if (cfdata->locals.cur[0] == 'm')
     {
	sscanf(cfdata->locals.cur, "m%d", &n);
	eb = eina_list_nth(cfdata->binding.mouse, n);
	if (!eb) return;
	ctxt = eb->context;
     }
   else if (cfdata->locals.cur[0] == 'w')
     {
	sscanf(cfdata->locals.cur, "w%d", &n);
	bw = eina_list_nth(cfdata->binding.wheel, n);
	if (!bw) return;
	ctxt = bw->context;
     }
   else
     return;

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

/****************** Helper *****************/
static void
_auto_apply_changes(E_Config_Dialog_Data *cfdata)
{
   int n, g, a;
   E_Config_Binding_Mouse *eb = NULL;
   E_Config_Binding_Wheel *bw = NULL;

   E_Action_Group *actg;
   E_Action_Description *actd;
   const char **action = NULL, **params = NULL;

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0])) return;

   if (cfdata->locals.cur[0] == 'm')
     {
	sscanf(cfdata->locals.cur, "m%d", &n);
	eb = eina_list_nth(cfdata->binding.mouse, n);
	if (!eb) return;

	eb->context = cfdata->locals.context;
	action = &(eb->action);
	params = &(eb->params);
     }
   else if (cfdata->locals.cur[0] == 'w')
     {
	sscanf(cfdata->locals.cur, "w%d", &n);
	bw = eina_list_nth(cfdata->binding.wheel, n);
	if (!bw) return;

	bw->context = cfdata->locals.context;
	action = &(bw->action);
	params = &(bw->params);
     } 
   
   if (*action) eina_stringshare_del(*action); 
   if (*params) eina_stringshare_del(*params); 
   *action = NULL; 
   *params = NULL;

   if ((!cfdata->locals.action) || (!cfdata->locals.action[0])) return;

   sscanf(cfdata->locals.action, "%d %d", &g, &a);

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   if (actd->act_cmd) *action = eina_stringshare_add(actd->act_cmd);
   if (actd->act_params) 
     *params = eina_stringshare_add(actd->act_params);
   else
     {
	int ok = 1;
	if (cfdata->locals.params)
	  {
	     if (!strcmp(cfdata->locals.params, TEXT_NO_PARAMS)) ok = 0;

	     if ((actd->param_example) && (!strcmp(cfdata->locals.params, actd->param_example)))
	       ok = 0;
	  }
	else
	  ok = 0;

	if (ok)
	  *params = eina_stringshare_add(cfdata->locals.params);
     }
}

static void
_find_key_binding_action(const char *action, const char *params, int *g, int *a, int *n)
{
   Eina_List *l, *l2;
   int gg = -1, aa = -1, nn = -1, found;
   E_Action_Group *actg;
   E_Action_Description *actd;

   if (g) *g = -1;
   if (a) *a = -1;
   if (n) *n = -1;

   found = 0;
   for (l = e_action_groups_get(), gg = 0, nn = 0; l; l = l->next, gg++)
     {
	actg = l->data;

	for (l2 = actg->acts, aa = 0; l2; l2 = l2->next, aa++)
	  {
	     actd = l2->data;

	     if (!strcmp((!action ? "" : action), (!actd->act_cmd ? "" : actd->act_cmd)))
	       {
		  if (!params || !params[0])
		    {
		       if ((!actd->act_params) || (!actd->act_params[0]))
			 {
			    if (g) *g = gg;
			    if (a) *a = aa;
			    if (n) *n = nn;
			    return;
			 }
		       else
			 continue;
		    }
		  else
		    {
		       if ((!actd->act_params) || (!actd->act_params[0]))
			 {
			    if (g) *g = gg;
			    if (a) *a = aa;
			    if (n) *n = nn;
			    found = 1;
			 }
		       else
			 {
			    if (!strcmp(params, actd->act_params))
			      {
				 if (g) *g = gg;
				 if (a) *a = aa;
				 if (n) *n = nn;
				 return;
			      }
			 }
		    }
	       }
	     nn++;
	  }
	if (found) break;
     }

   if (!found)
     {
	if (g) *g = -1;
	if (a) *a = -1;
	if (n) *n = -1;
     }
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

/********* Sorting ***************/
static int
_mouse_binding_sort_cb(const void *d1, const void *d2)
{
   const E_Config_Binding_Mouse *eb, *eb2;

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
_wheel_binding_sort_cb(const void *d1, const void *d2)
{
   const E_Config_Binding_Wheel *bw, *bw2;

   bw = d1;
   bw2 = d2;

   if (bw->direction < bw2->direction) return -1;
   else if (bw->direction > bw2->direction) return 1;
   else
     {
	if ((bw->z < 0) && (bw2->z > 0)) return 1;
	else if ((bw->z > 0) && (bw2->z < 0)) return -1;
	else if (((bw->z < 0) && (bw2->z < 0)) ||
	         ((bw->z > 0) && (bw2->z > 0)))
	  {
	     if (bw->modifiers < bw2->modifiers) return -1;
	     else if (bw->modifiers > bw2->modifiers) return 1;
	  }
     }
   return 0;
}

static void
_grab_wnd_show(E_Config_Dialog_Data *cfdata)
{
   E_Manager *man;

   if (cfdata->locals.bind_win != 0) return;

   man = e_manager_current_get();

   cfdata->locals.dia = e_dialog_new(e_container_current_get(man), 
				     "E", "_mousebind_getmouse_dialog");
   if (!cfdata->locals.dia) return;
   e_dialog_title_set(cfdata->locals.dia, _("Mouse Binding Sequence"));
   e_dialog_icon_set(cfdata->locals.dia, "enlightenment/mouse_clean", 48);
   e_dialog_text_set(cfdata->locals.dia, TEXT_PRESS_MOUSE_BINIDING_SEQUENCE);
   e_win_centered_set(cfdata->locals.dia->win, 1);
   e_win_borderless_set(cfdata->locals.dia->win, 1);

   cfdata->locals.bind_win = ecore_x_window_input_new(man->root, 0, 0,
						      man->w, man->h);
   ecore_x_window_show(cfdata->locals.bind_win);
   e_grabinput_get(cfdata->locals.bind_win, 0, cfdata->locals.bind_win);

   cfdata->locals.handlers = eina_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
				 _grab_key_down_cb, cfdata));

   cfdata->locals.handlers = eina_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
				 _grab_mouse_down_cb, cfdata));

   cfdata->locals.handlers = eina_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
				 _grab_mouse_wheel_cb, cfdata));

   e_dialog_show(cfdata->locals.dia);
   ecore_x_icccm_transient_for_set(cfdata->locals.dia->win->evas_win, cfdata->cfd->dia->win->evas_win);
}

static void
_grab_wnd_hide(E_Config_Dialog_Data *cfdata)
{
   while (cfdata->locals.handlers)
     {
	ecore_event_handler_del(cfdata->locals.handlers->data);
	cfdata->locals.handlers = 
	   eina_list_remove_list(cfdata->locals.handlers, cfdata->locals.handlers);
     }
   cfdata->locals.handlers = NULL;
   e_grabinput_release(cfdata->locals.bind_win, cfdata->locals.bind_win);
   ecore_x_window_del(cfdata->locals.bind_win);
   cfdata->locals.bind_win = 0;

   e_object_del(E_OBJECT(cfdata->locals.dia));
   cfdata->locals.dia = NULL;
}

static int
_grab_mouse_down_cb(void *data, int type, void *event)
{
   Eina_List *l;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *eb = NULL;
   E_Config_Binding_Wheel *bw;
   int mod = E_BINDING_MODIFIER_NONE, n;

   Ecore_X_Event_Mouse_Button_Down *ev;
   
   ev = event;
   cfdata = data;

   if (ev->win != cfdata->locals.bind_win) return 1; 

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
	eb->action = NULL; 
	eb->params = NULL; 
	
	cfdata->binding.mouse = eina_list_append(cfdata->binding.mouse, eb);
     }
   else
     {
	if (cfdata->locals.cur[0] == 'm')
	  { 
	     sscanf(cfdata->locals.cur, "m%d", &n); 
	     eb = eina_list_nth(cfdata->binding.mouse, n); 
	     if (eb) 
	       { 
		  eb->button = ev->button; 
		  eb->modifiers = mod; 
	       }
	  }
	else if (cfdata->locals.cur[0] == 'w')
	  {
	     sscanf(cfdata->locals.cur, "w%d", &n);
	     l = eina_list_nth_list(cfdata->binding.wheel, n);
	     bw = l->data;

	     eb = E_NEW(E_Config_Binding_Mouse, 1); 
	     eb->context = bw->context;
	     eb->button = ev->button; 
	     eb->modifiers = mod; 
	     eb->any_mod = 0; 
	     eb->action = bw->action; 
	     eb->params = bw->params; 

	     cfdata->binding.mouse = eina_list_append(cfdata->binding.mouse, eb);

	     bw->action = NULL;
	     bw->params = NULL;
	     E_FREE(bw);
	     cfdata->binding.wheel = eina_list_remove_list(cfdata->binding.wheel, l);
	  }
     } 
   _update_mouse_binding_list(cfdata); 

   if (cfdata->locals.add)
     { 
	for (l = cfdata->binding.mouse, n = 0; l; l = l->next, n++)
	  if (l->data == eb) break;

	e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n + 1); 

	if (cfdata->locals.action) free(cfdata->locals.action);
	cfdata->locals.action = strdup("");
	e_widget_ilist_unselect(cfdata->gui.o_action_list);
	e_widget_entry_clear(cfdata->gui.o_params);
	e_widget_disabled_set(cfdata->gui.o_params, 1);
     }
   else
     { 
	for (l = cfdata->binding.mouse, n = 0; l; l = l->next, n++) 
	  if (l->data == eb) break; 

	if (cfdata->locals.cur) free(cfdata->locals.cur);
	cfdata->locals.cur = NULL;

	e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n + 1);
     }
   _update_buttons(cfdata);
   _grab_wnd_hide(cfdata);

   return 1;
}

static int 
_grab_mouse_wheel_cb(void *data, int type, void *event)
{
   Eina_List *l;
   E_Config_Binding_Wheel *bw = NULL;
   E_Config_Binding_Mouse *eb = NULL;
   E_Config_Dialog_Data *cfdata;
   Ecore_X_Event_Mouse_Wheel *ev; 
   int mod = E_BINDING_MODIFIER_NONE, n;

   ev = event;
   cfdata = data;

   if (ev->win != cfdata->locals.bind_win) return 1;

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
	bw->action = NULL;
	bw->params = NULL; 
	
	cfdata->binding.wheel = eina_list_append(cfdata->binding.wheel, bw);
     }
   else 
     {
	if (cfdata->locals.cur[0] == 'm')
	  {
	     sscanf(cfdata->locals.cur, "m%d", &n);
	     l = eina_list_nth_list(cfdata->binding.mouse, n);
	     eb = l->data; 
	     
	     bw = E_NEW(E_Config_Binding_Wheel, 1);
	     bw->context = eb->context;
	     bw->direction = ev->direction;
	     bw->z = ev->z;
	     bw->modifiers = mod;
	     bw->any_mod = 0;
	     bw->action = eb->action;
	     bw->params = eb->params; 

	     cfdata->binding.wheel = eina_list_append(cfdata->binding.wheel, bw);

	     E_FREE(eb);
	     cfdata->binding.mouse = eina_list_remove_list(cfdata->binding.mouse, l);
	  }
	else if (cfdata->locals.cur[0] == 'w')
	  {
	     sscanf(cfdata->locals.cur, "w%d", &n);
	     bw = eina_list_nth(cfdata->binding.wheel, n);
	     if (bw)
	       {
		  bw->direction = ev->direction;
		  bw->z = ev->z;
		  bw->modifiers = mod;
	       }
	  }
     } 
   _update_mouse_binding_list(cfdata); 

   if (cfdata->locals.add)
     { 
	for (l = cfdata->binding.wheel, n = 0; l; l = l->next, n++)
	  if (l->data == bw) break;

	if (eina_list_count(cfdata->binding.mouse))
	  { 
	     n += eina_list_count(cfdata->binding.mouse) + 2;
	     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n); 
	  }
	else 
	  e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n + 1); 

	e_widget_ilist_unselect(cfdata->gui.o_action_list);
	if (cfdata->locals.action) free(cfdata->locals.action);
	cfdata->locals.action = strdup("");
	e_widget_entry_clear(cfdata->gui.o_params);
	e_widget_disabled_set(cfdata->gui.o_params, 1);
     }
   else
     {
	for (l = cfdata->binding.wheel, n = 0; l; l = l->next, n++)
	  if (l->data == bw) break;

	if (cfdata->locals.cur) free(cfdata->locals.cur);
	cfdata->locals.cur = NULL;

	if (eina_list_count(cfdata->binding.mouse)) 
	  { 
	     n += eina_list_count(cfdata->binding.mouse) + 2;
	     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n);
	  }
	else
	  e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n + 1);
     }
   _update_buttons(cfdata);

   _grab_wnd_hide(cfdata);
   return 1;
}

static int
_grab_key_down_cb(void *data, int type, void *event)
{
   E_Config_Dialog_Data *cfdata;
   Ecore_X_Event_Key_Down *ev = event;

   cfdata = data;

   if (ev->win != cfdata->locals.bind_win) return 1;

   if (!strcmp(ev->keyname, "Escape") &&
       !(ev->modifiers & ECORE_X_MODIFIER_SHIFT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_CTRL) &&
       !(ev->modifiers & ECORE_X_MODIFIER_ALT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_WIN))
     { 
	_grab_wnd_hide(cfdata);
     }
   return 1;
}

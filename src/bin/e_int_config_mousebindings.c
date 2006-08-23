#include "e.h"

#define E_BINDING_CONTEXT_NUMBER  10

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _fill_data(E_Config_Dialog_Data *cfdata);

static void _fill_action_ilist(E_Config_Dialog_Data *cfdata);
static void _update_binding_action_list(E_Config_Dialog_Data *cfdata);
static void _update_context_radios(E_Config_Dialog_Data *cfdata);
static void _update_mod_key_check_boxes(E_Config_Dialog_Data *cfdata);
static void _update_action_list(E_Config_Dialog_Data *cfdata);
static void _update_action_params(E_Config_Dialog_Data *cfdata);
static void _update_delete_button(E_Config_Dialog_Data *cfdata);

static void _apply_binding_changes(E_Config_Dialog_Data *cfdata);

static void _binding_change_cb(void *data, Evas_Object *obj);
static void _binding_action_ilist_change_cb(void *data);
static void _action_ilist_change_cb(void *data);
static void _add_binding_cb(void *data, void *data2);
static void _delete_binding_cb(void *data, void *data2);

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

  int mouse_binding;

  E_Config_Binding_Mouse *current_mouse_binding;
  E_Config_Binding_Wheel *current_wheel_binding;


  E_Binding_Context  context;
  char *binding_ilist_value;
  char *action_ilist_value;

  int shift_mod, ctrl_mod, alt_mod, win_mod;
  char *params;

  Evas_List *mouse_bindings[5];

  struct 
    { 
       Evas_Object *o_binding_action_list;
       Evas_Object *o_action_list, *o_params;

       Evas_Object *o_add, *o_del;

       struct { 
	 Evas_Object *o_any, *o_border, *o_menu, *o_winlist, *o_popup, *o_zone,
		     *o_container, *o_manager, *o_none;
       } context;
       struct {
	 Evas_Object *o_shift, *o_ctrl, *o_alt, *o_win;
       } mod_key;
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
			    "enlightenment/mouse_binding_left", 0, v, NULL);
  return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Evas_List *l;
   E_Config_Binding_Mouse *eb, *eb2;
   E_Config_Binding_Wheel *bw, *bw2;


   cfdata->mouse_binding = 0;
   cfdata->context = E_BINDING_CONTEXT_ANY;
   cfdata->binding_ilist_value = strdup("");
   cfdata->shift_mod = 0;
   cfdata->ctrl_mod = 0;
   cfdata->alt_mod = 0;
   cfdata->win_mod = 0;
   cfdata->params = strdup("");
   cfdata->current_mouse_binding = NULL;
   cfdata->current_wheel_binding = NULL;

   for (l = e_config->mouse_bindings; l; l = l->next)
     {
	eb = l->data;

	if (!eb->action) continue;

	eb2 = E_NEW(E_Config_Binding_Mouse, 1);
	eb2->context = eb->context;
	eb2->button = eb->button;
	eb2->modifiers = eb->modifiers;
	eb2->any_mod = eb->any_mod;
	eb2->action = eb->action == NULL ? NULL : evas_stringshare_add(eb->action);
	eb2->params = eb->params == NULL ? NULL : evas_stringshare_add(eb->params);

	if (eb2->button == 1) 
	  cfdata->mouse_bindings[0] = evas_list_append(cfdata->mouse_bindings[0], eb2);
	else if (eb2->button == 2) 
	  cfdata->mouse_bindings[1] = evas_list_append(cfdata->mouse_bindings[1], eb2);
	else if (eb2->button == 3) 
	  cfdata->mouse_bindings[2] = evas_list_append(cfdata->mouse_bindings[2], eb2);
     }

   for (l = e_config->wheel_bindings; l; l = l->next)
     {
	bw = l->data;
	
	if (!bw) continue;

	if (bw->direction == 1) continue;

	bw2 = E_NEW(E_Config_Binding_Wheel, 1);
	bw2->context = bw->context;
	bw2->direction = bw->direction;
	bw2->z = bw->z;
	bw2->modifiers = bw->modifiers;
	bw2->any_mod = bw->any_mod;
	bw2->action = bw->action == NULL ? NULL : evas_stringshare_add(bw->action);
	bw2->params = bw->params == NULL ? NULL : evas_stringshare_add(bw->params);

	if (bw->z < 0)
	  cfdata->mouse_bindings[3] = evas_list_append(cfdata->mouse_bindings[3], bw2);
	else
	  cfdata->mouse_bindings[4] = evas_list_append(cfdata->mouse_bindings[4], bw2);
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
   int i;
   E_Config_Binding_Mouse *bm;
   E_Config_Binding_Wheel *bw;

   for (i = 0; i < 5; i++)
     {
	while (cfdata->mouse_bindings[i])
	  {
	     switch (i)
	       {
		case 0:
		case 1:
		case 2: 
		   bm = cfdata->mouse_bindings[i]->data; 
		   if (bm->action) evas_stringshare_del(bm->action);
		   if (bm->params) evas_stringshare_del(bm->params); 
		   E_FREE(bm);
		   break;
		case 3:
		case 4:
		   bw = cfdata->mouse_bindings[i]->data;
		   if (bw->action) evas_stringshare_del(bw->action);
		   if (bw->params) evas_stringshare_del(bw->params); 
		   E_FREE(bw);
		   break;
	       }
	     cfdata->mouse_bindings[i] = 
		evas_list_remove_list(cfdata->mouse_bindings[i], cfdata->mouse_bindings[i]);
	  }
     }
   if (cfdata->binding_ilist_value) free(cfdata->binding_ilist_value);
   if (cfdata->action_ilist_value) free(cfdata->action_ilist_value);
   if (cfdata->params) free(cfdata->params);

   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{ 
   int i;
   Evas_List *l;
   E_Config_Binding_Mouse *bm, *bm2;
   E_Config_Binding_Wheel *bw, *bw2;

   if (!cfdata) return 0;

   _apply_binding_changes(cfdata);

   e_border_button_bindings_ungrab_all();
   while (e_config->mouse_bindings)
     {
	bm = e_config->mouse_bindings->data;
	e_bindings_mouse_del(bm->context, bm->button, bm->modifiers, bm->any_mod, bm->action,
			     bm->params);
	if (bm->action) evas_stringshare_del(bm->action);
	if (bm->params) evas_stringshare_del(bm->params);
	E_FREE(bm);

	e_config->mouse_bindings = evas_list_remove_list(e_config->mouse_bindings, e_config->mouse_bindings);
     }

   while (e_config->wheel_bindings)
     {
	bw = e_config->wheel_bindings->data;
	e_bindings_wheel_del(bw->context, bw->direction, bw->z, bw->modifiers, bw->any_mod,
			     bw->action, bw->params);
	if (bw->action) evas_stringshare_del(bw->action);
	if (bw->params) evas_stringshare_del(bw->params);
	E_FREE(bw);
	e_config->wheel_bindings = evas_list_remove_list(e_config->wheel_bindings, e_config->wheel_bindings);
     }

   for (i = 0; i < 5; i++)
     {
	switch (i)
	  {
	   case 0:
	   case 1:
	   case 2:
	      for (l = cfdata->mouse_bindings[i]; l; l = l->next)
		{
		   bm = l->data;

		   bm2 = E_NEW(E_Config_Binding_Mouse, 1);
		   bm2->context = bm->context;
		   bm2->button = bm->button;
		   bm2->modifiers = bm->modifiers;
		   bm2->any_mod = bm->any_mod;
		   bm2->action = bm->action == NULL ? NULL : evas_stringshare_add(bm->action);
		   bm2->params = bm->params == NULL ? NULL : evas_stringshare_add(bm->params);

		   e_config->mouse_bindings = evas_list_append(e_config->mouse_bindings, bm2);
		   e_bindings_mouse_add(bm2->context, bm2->button, bm2->modifiers, bm2->any_mod,
				        bm2->action, bm2->params);
		}
	      break;
	   case 3:
	   case 4:
	      for (l = cfdata->mouse_bindings[i]; l; l = l->next)
		{
		   bw = l->data;

		   bw2 = E_NEW(E_Config_Binding_Wheel, 1);
		   bw2->context = bw->context;
		   bw2->direction = 0;
		   bw2->z = bw->z;
		   bw2->modifiers = bw->modifiers;
		   bw2->any_mod = 0;//bw->any_mod;
		   bw2->action = bw->action == NULL ? NULL : evas_stringshare_add(bw->action);
		   bw2->params = bw->params == NULL ? NULL : evas_stringshare_add(bw->params);

		   e_config->wheel_bindings = evas_list_append(e_config->wheel_bindings, bw2);
		   e_bindings_wheel_add(bw2->context, bw2->direction, bw2->z, bw2->modifiers,
				        bw2->any_mod, bw2->action, bw2->params);

		   bw2 = E_NEW(E_Config_Binding_Wheel, 1);
		   bw2->context = bw->context;
		   bw2->direction = 1;
		   bw2->z = bw->z;
		   bw2->modifiers = bw->modifiers;
		   bw2->any_mod = 0;//bw->any_mod;
		   bw2->action = bw->action == NULL ? NULL : evas_stringshare_add(bw->action);
		   bw2->params = bw->params == NULL ? NULL : evas_stringshare_add(bw->params);

		   e_config->wheel_bindings = evas_list_append(e_config->wheel_bindings, bw2);
		   e_bindings_wheel_add(bw2->context, bw2->direction, bw2->z, bw2->modifiers,
				        bw2->any_mod, bw2->action, bw2->params);
		}
	      break;
	  }
     }
   e_border_button_bindings_grab_all();

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *ol2, *ol3, *of, *of2, *ot;
   E_Radio_Group *rg;


   cfdata->evas = evas;

   ol = e_widget_list_add(evas, 0, 0); 
   
   ol2 = e_widget_list_add(evas, 0, 1); 
   of = e_widget_framelist_add(evas, _("Mouse Binding"), 0); 
   
   rg = e_widget_radio_group_new(&(cfdata->mouse_binding)); 
   ol3 = e_widget_list_add(evas, 0, 1); 
   o = e_widget_radio_icon_add(evas, _("Left Button"), "enlightenment/mouse_binding_left",
			       48, 48, 0, rg); 
   e_widget_on_change_hook_set(o, _binding_change_cb, cfdata); 
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   
   o = e_widget_radio_icon_add(evas, _("Middle Button"), "enlightenment/mouse_binding_middle",
			       48, 48, 1, rg); 
   e_widget_on_change_hook_set(o, _binding_change_cb, cfdata); 
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   
   o = e_widget_radio_icon_add(evas, _("Right Button"), "enlightenment/mouse_binding_right",
			       48, 48, 2, rg); 
   e_widget_on_change_hook_set(o, _binding_change_cb, cfdata); 
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   
   e_widget_framelist_object_append(of, ol3); 
   
   ol3 = e_widget_list_add(evas, 0, 1); 
   o = e_widget_radio_icon_add(evas, _("Mouse Wheel Up"), "enlightenment/mouse_binding_scroll_up",
			       48, 48, 3, rg); 
   e_widget_on_change_hook_set(o, _binding_change_cb, cfdata);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   
   o = e_widget_radio_icon_add(evas, _("Mouse Wheel Down"),
			       "enlightenment/mouse_binding_scroll_down", 48, 48, 4, rg); 
   e_widget_on_change_hook_set(o, _binding_change_cb, cfdata);
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   e_widget_framelist_object_append(of, ol3);
   
   e_widget_list_object_append(ol2, of, 1, 1, 0.5); 
   of = e_widget_framelist_add(evas, _("Binding Action List"), 0);
   o = e_widget_ilist_add(evas, 32, 32, &(cfdata->binding_ilist_value));
   cfdata->gui.o_binding_action_list = o;
   e_widget_min_size_set(o, 250, 100);
   e_widget_ilist_go(o);
   e_widget_framelist_object_append(of, o); 
   
   ol3 = e_widget_list_add(evas, 0, 1);
   o = e_widget_button_add(evas, _("Add"), NULL, _add_binding_cb, cfdata, NULL);
   cfdata->gui.o_add = o;
   e_widget_list_object_append(ol3, o, 1, 1, 0.5); 
   
   o = e_widget_button_add(evas, _("Delete"), NULL, _delete_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = o;
   e_widget_list_object_append(ol3, o, 1, 1, 0.5);
   e_widget_disabled_set(o, 1);
   e_widget_framelist_object_append(of, ol3);
   
   e_widget_list_object_append(ol2, of, 1, 1, 0.5); 
   e_widget_list_object_append(ol, ol2, 1, 1, 0.5); 
   
   of = e_widget_framelist_add(evas, _("Action Settings"), 1); 
   ol2 = e_widget_list_add(evas, 0, 0); 
   of2 = e_widget_framelist_add(evas, _("Modifier Key"), 1); 
   o = e_widget_check_add(evas, _("Shift"), &(cfdata->shift_mod)); 
   cfdata->gui.mod_key.o_shift = o;
   e_widget_framelist_object_append(of2, o); 
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_check_add(evas, _("Ctrl"), &(cfdata->ctrl_mod)); 
   cfdata->gui.mod_key.o_ctrl = o;
   e_widget_framelist_object_append(of2, o); 
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_check_add(evas, _("Alt"), &(cfdata->alt_mod)); 
   cfdata->gui.mod_key.o_alt = o;
   e_widget_framelist_object_append(of2, o); 
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_check_add(evas, _("Win"), &(cfdata->win_mod)); 
   cfdata->gui.mod_key.o_win = o;
   e_widget_framelist_object_append(of2, o);
   e_widget_disabled_set(o, 1); 
   
   e_widget_list_object_append(ol2, of2, 1, 1, 0.5); 
   of2 = e_widget_framelist_add(evas, _("Context"), 0);
   ot = e_widget_table_add(evas, 0);
   
   rg = e_widget_radio_group_new((int*)(&(cfdata->context))); 
   o = e_widget_radio_add(evas, _("Any"), E_BINDING_CONTEXT_ANY, rg); 
   cfdata->gui.context.o_any = o;
   e_widget_table_object_append(ot, o, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Border"), E_BINDING_CONTEXT_BORDER, rg); 
   cfdata->gui.context.o_border = o;
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Menu"), E_BINDING_CONTEXT_MENU, rg); 
   cfdata->gui.context.o_menu = o;
   e_widget_table_object_append(ot, o, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Win List"), E_BINDING_CONTEXT_WINLIST, rg); 
   cfdata->gui.context.o_winlist = o;
   e_widget_table_object_append(ot, o, 1, 0, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Popup"), E_BINDING_CONTEXT_POPUP, rg); 
   cfdata->gui.context.o_popup = o;
   e_widget_table_object_append(ot, o, 1, 1, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Zone"), E_BINDING_CONTEXT_ZONE, rg); 
   cfdata->gui.context.o_zone = o;
   e_widget_table_object_append(ot, o, 1, 2, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Container"), E_BINDING_CONTEXT_CONTAINER, rg); 
   cfdata->gui.context.o_container = o;
   e_widget_table_object_append(ot, o, 2, 0, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("Manager"), E_BINDING_CONTEXT_MANAGER, rg); 
   cfdata->gui.context.o_manager = o;
   e_widget_table_object_append(ot, o, 2, 1, 1, 1, 1, 1, 1, 1);
   e_widget_disabled_set(o, 1); 
   
   o = e_widget_radio_add(evas, _("None"), E_BINDING_CONTEXT_NONE, rg); 
   cfdata->gui.context.o_none = o;
   e_widget_table_object_append(ot, o, 2, 2, 1, 1, 1, 1, 1, 1); e_widget_disabled_set(o, 1); 
   
   e_widget_framelist_object_append(of2, ot);
   e_widget_list_object_append(ol2, of2, 1, 1, 0.5);
   
   e_widget_framelist_object_append(of, ol2); 
   
   ol2 = e_widget_list_add(evas, 0, 0);
   of2 = e_widget_framelist_add(evas, _("Action"), 0);
   
   o = e_widget_ilist_add(evas, 32, 32, &(cfdata->action_ilist_value));
   cfdata->gui.o_action_list = o;
   e_widget_min_size_set(o, 250, 100);
   e_widget_ilist_go(o); 
   e_widget_disabled_set(o, 1);
   e_widget_framelist_object_append(of2, o); 
   
   e_widget_list_object_append(ol2, of2, 1, 1, 0.5); 
   
   of2 = e_widget_framelist_add(evas, _("Params"), 0);
   
   o = e_widget_entry_add(evas, &(cfdata->params));
   cfdata->gui.o_params = o;
   e_widget_framelist_object_append(of2, o);
   e_widget_disabled_set(o, 1);
   
   e_widget_list_object_append(ol2, of2, 1, 1, 0.5);
   e_widget_framelist_object_append(of, ol2);
   
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   _update_binding_action_list(cfdata);
   _update_delete_button(cfdata);

   e_dialog_resizable_set(cfd->dia, 1);
   return ol;
}

static void
_update_binding_action_list(E_Config_Dialog_Data *cfdata)
{ 
   char buf[256] = "";
   const char *action;
   char indx[256];
   Evas_List *l;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   int i, j;

   e_widget_ilist_clear(cfdata->gui.o_binding_action_list);

   if (cfdata->mouse_binding < 3)
     {
	for (l = cfdata->mouse_bindings[cfdata->mouse_binding], i = 0; l; l = l->next, i++)
	  {
	     eb = l->data;
	     
	     for (j = 0; j < E_ACTION_NUMBER; j++)
	       {
		  if (!strcmp(action_to_name[j][0], eb->action))
		    break;
	       }

	     if (j < E_ACTION_NUMBER)
	       action = action_to_name[j][1];
	     else
	       action = _("Unknown");

	     snprintf(buf, sizeof(buf), "%s %d: %s", _("Action"), i, action); 
	     snprintf(indx, sizeof(indx), "%d", i);
	     e_widget_ilist_append(cfdata->gui.o_binding_action_list, NULL, buf,
				   _binding_action_ilist_change_cb, cfdata, indx);
	  }
     }
   else
     {
	for (l = cfdata->mouse_bindings[cfdata->mouse_binding], i = 0; l; l = l->next, i++)
	  {
	     bw = l->data;

	     for (j = 0; j < E_ACTION_NUMBER; j++)
	       {
		  if (!strcmp(action_to_name[j][0], bw->action))
		    break;
	       }

	     if (j < E_ACTION_NUMBER)
	       action = action_to_name[j][1];
	     else
	       action = _("Unknown");

	     snprintf(buf, sizeof(buf), "%s %d: %s", _("Action"), i, action); 
	     snprintf(indx, sizeof(indx), "%d", i);
	     e_widget_ilist_append(cfdata->gui.o_binding_action_list, NULL, buf,
				   _binding_action_ilist_change_cb, cfdata, indx);
	  }
     } 
   e_widget_ilist_go(cfdata->gui.o_binding_action_list); 

   if (!cfdata->mouse_bindings[cfdata->mouse_binding]) 
     { 
	_update_context_radios(cfdata); 
	_update_mod_key_check_boxes(cfdata); 
	_update_action_list(cfdata); 
	_update_action_params(cfdata);
     } 
   else 
     e_widget_ilist_selected_set(cfdata->gui.o_binding_action_list, 0);
}

static void
_update_context_radios(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;
   
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

   if ((!cfdata->current_mouse_binding) && (!cfdata->current_wheel_binding))
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

   if (cfdata->mouse_binding < 3)
     { 
	eb = cfdata->current_mouse_binding;

	if (eb->context == E_BINDING_CONTEXT_ANY)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_any, 1);
	else if (eb->context == E_BINDING_CONTEXT_BORDER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_border, 1);
	else if (eb->context == E_BINDING_CONTEXT_MENU)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_menu, 1);
	else if (eb->context == E_BINDING_CONTEXT_WINLIST)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_winlist, 1);
	else if (eb->context == E_BINDING_CONTEXT_POPUP)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_popup, 1);
	else if (eb->context == E_BINDING_CONTEXT_ZONE)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_zone, 1);
	else if (eb->context == E_BINDING_CONTEXT_CONTAINER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_container, 1);
	else if (eb->context == E_BINDING_CONTEXT_MANAGER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_manager, 1);
	else if (eb->context == E_BINDING_CONTEXT_NONE)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_none, 1);
	else
	  e_widget_radio_toggle_set(cfdata->gui.context.o_none, 1);
     }
   else
     {
	bw = cfdata->current_wheel_binding;

	if (bw->context == E_BINDING_CONTEXT_ANY)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_any, 1);
	else if (bw->context == E_BINDING_CONTEXT_BORDER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_border, 1);
	else if (bw->context == E_BINDING_CONTEXT_MENU)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_menu, 1);
	else if (bw->context == E_BINDING_CONTEXT_WINLIST)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_winlist, 1);
	else if (bw->context == E_BINDING_CONTEXT_POPUP)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_popup, 1);
	else if (bw->context == E_BINDING_CONTEXT_ZONE)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_zone, 1);
	else if (bw->context == E_BINDING_CONTEXT_CONTAINER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_container, 1);
	else if (bw->context == E_BINDING_CONTEXT_MANAGER)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_manager, 1);
	else if (bw->context == E_BINDING_CONTEXT_NONE)
	  e_widget_radio_toggle_set(cfdata->gui.context.o_none, 1);
	else
	  e_widget_radio_toggle_set(cfdata->gui.context.o_none, 1);
     }
}
static void 
_update_mod_key_check_boxes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   e_widget_disabled_set(cfdata->gui.mod_key.o_shift, 1);
   e_widget_disabled_set(cfdata->gui.mod_key.o_ctrl, 1);
   e_widget_disabled_set(cfdata->gui.mod_key.o_alt, 1);
   e_widget_disabled_set(cfdata->gui.mod_key.o_win, 1); 
   e_widget_check_checked_set(cfdata->gui.mod_key.o_shift, 0);
   e_widget_check_checked_set(cfdata->gui.mod_key.o_ctrl, 0);
   e_widget_check_checked_set(cfdata->gui.mod_key.o_alt, 0);
   e_widget_check_checked_set(cfdata->gui.mod_key.o_win, 0);

   if ((!cfdata->current_mouse_binding) && (!cfdata->current_wheel_binding))
     return; 
   
   e_widget_disabled_set(cfdata->gui.mod_key.o_shift, 0); 
   e_widget_disabled_set(cfdata->gui.mod_key.o_ctrl, 0); 
   e_widget_disabled_set(cfdata->gui.mod_key.o_alt, 0); 
   e_widget_disabled_set(cfdata->gui.mod_key.o_win, 0);

   if (cfdata->mouse_binding < 3)
     {
	eb = cfdata->current_mouse_binding;

	if (eb->modifiers & E_BINDING_MODIFIER_SHIFT)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_shift, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_shift, 0);

	if (eb->modifiers & E_BINDING_MODIFIER_CTRL)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_ctrl, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_ctrl, 0);

	if (eb->modifiers & E_BINDING_MODIFIER_ALT)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_alt, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_alt, 0);

	if (eb->modifiers & E_BINDING_MODIFIER_WIN)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_win, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_win, 0);
     }
   else
     {
	bw = cfdata->current_wheel_binding;

	if (bw->modifiers & E_BINDING_MODIFIER_SHIFT)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_shift, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_shift, 0);

	if (bw->modifiers & E_BINDING_MODIFIER_CTRL)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_ctrl, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_ctrl, 0);

	if (bw->modifiers & E_BINDING_MODIFIER_ALT)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_alt, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_alt, 0);

	if (bw->modifiers & E_BINDING_MODIFIER_WIN)
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_win, 1);
	else
	  e_widget_check_checked_set(cfdata->gui.mod_key.o_win, 0);
     }
}

static void
_update_action_list(E_Config_Dialog_Data *cfdata)
{
   int i;
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   if ((cfdata->current_mouse_binding) || (cfdata->current_wheel_binding))
     _fill_action_ilist(cfdata);
   else 
     { 
	_update_action_params(cfdata);
	e_widget_ilist_clear(cfdata->gui.o_action_list);
	return;
     }

   if (cfdata->mouse_binding < 3)
     { 
	eb = cfdata->current_mouse_binding; 
	     
	for (i = 0; i < E_ACTION_NUMBER; i++) 
	  { 
	     if (!strcmp(eb->action, action_to_name[i][0])) 
	       break; 
	  } 
	e_widget_ilist_selected_set(cfdata->gui.o_action_list, i); 
     }
   else
     {
	bw = cfdata->current_wheel_binding;

	for (i = 0; i < E_ACTION_NUMBER; i++)
	  {
	     if (!strcmp(bw->action, action_to_name[i][0]))
	       break;
	  }
	e_widget_ilist_selected_set(cfdata->gui.o_action_list, i); 
     } 
}

static void
_update_action_params(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   if ((!cfdata->current_mouse_binding) && (!cfdata->current_wheel_binding))
     { 
        e_widget_entry_clear(cfdata->gui.o_params);
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	return;
     }
   e_widget_disabled_set(cfdata->gui.o_params, 0);

   if (cfdata->mouse_binding < 3)
     {
	eb = cfdata->current_mouse_binding; 
	if (eb->params) 
	  e_widget_entry_text_set(cfdata->gui.o_params, eb->params);
	else 
	  e_widget_entry_clear(cfdata->gui.o_params); 
     }
   else
     {
	bw = cfdata->current_wheel_binding;
	if (bw->params)
	  e_widget_entry_text_set(cfdata->gui.o_params, bw->params);
	else 
	  e_widget_entry_clear(cfdata->gui.o_params); 
     }
}

static void
_fill_action_ilist(E_Config_Dialog_Data *cfdata)
{
   char buf[256];
   int j;

   if (e_widget_ilist_count(cfdata->gui.o_action_list))
     return;

   for (j = 0; j < E_ACTION_NUMBER; j++)
     {
	snprintf(buf, sizeof(buf), "%d", j);
	e_widget_ilist_append(cfdata->gui.o_action_list, NULL, action_to_name[j][1], 
			      _action_ilist_change_cb, cfdata, buf);
     } 
   e_widget_ilist_go(cfdata->gui.o_action_list);
}


static void
_apply_binding_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Mouse *eb;
   E_Config_Binding_Wheel *bw;

   if (cfdata->current_mouse_binding)
     {
	eb = cfdata->current_mouse_binding;

	eb->context = cfdata->context;
	eb->modifiers = E_BINDING_MODIFIER_NONE;
	if (cfdata->shift_mod)
	  eb->modifiers |= E_BINDING_MODIFIER_SHIFT;
	if (cfdata->ctrl_mod)
	  eb->modifiers |= E_BINDING_MODIFIER_CTRL;
	if (cfdata->alt_mod)
	  eb->modifiers |= E_BINDING_MODIFIER_ALT;
	if (cfdata->win_mod)
	  eb->modifiers |= E_BINDING_MODIFIER_WIN;
	if (eb->action) evas_stringshare_del(eb->action);
	eb->action = evas_stringshare_add(action_to_name[atoi(cfdata->action_ilist_value)][0]);
	if (eb->params) evas_stringshare_del(eb->params);
	eb->params = cfdata->params[0] ? evas_stringshare_add(cfdata->params) : NULL;
     }
   else if (cfdata->current_wheel_binding)
     {
	bw = cfdata->current_wheel_binding;

	bw->context = cfdata->context;
	bw->direction = 0;
	bw->modifiers = E_BINDING_MODIFIER_NONE;
	if (cfdata->shift_mod)
	  bw->modifiers |= E_BINDING_MODIFIER_SHIFT;
	if (cfdata->ctrl_mod)
	  bw->modifiers |= E_BINDING_MODIFIER_CTRL;
	if (cfdata->alt_mod)
	  bw->modifiers |= E_BINDING_MODIFIER_ALT;
	if (cfdata->win_mod)
	  bw->modifiers |= E_BINDING_MODIFIER_WIN;
	if (bw->action) evas_stringshare_del(bw->action);
	bw->action = evas_stringshare_add(action_to_name[atoi(cfdata->action_ilist_value)][0]);
	if (bw->params) evas_stringshare_del(bw->params);
	bw->params = cfdata->params[0] ? evas_stringshare_add(cfdata->params) : NULL;
     }
}

static void
_update_delete_button(E_Config_Dialog_Data *cfdata)
{
   if (evas_list_count(cfdata->mouse_bindings[cfdata->mouse_binding]))
     e_widget_disabled_set(cfdata->gui.o_del, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del, 1);
}

/* callbacks */
static void
_binding_change_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _apply_binding_changes(cfdata);
   cfdata->current_mouse_binding = NULL;
   cfdata->current_wheel_binding = NULL;

   _update_binding_action_list(cfdata);
   _update_delete_button(cfdata);
}

static void
_binding_action_ilist_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _apply_binding_changes(cfdata);

   if (cfdata->mouse_binding < 3)
     { 
	cfdata->current_mouse_binding = 
	   evas_list_nth(cfdata->mouse_bindings[cfdata->mouse_binding], 
		 atoi(cfdata->binding_ilist_value));
	cfdata->current_wheel_binding = NULL;
     }
   else
     {
	cfdata->current_mouse_binding = NULL;
	cfdata->current_wheel_binding = 
	   evas_list_nth(cfdata->mouse_bindings[cfdata->mouse_binding], 
		 atoi(cfdata->binding_ilist_value));
     }

   _update_context_radios(cfdata);
   _update_mod_key_check_boxes(cfdata);
   _update_action_list(cfdata);
   _update_action_params(cfdata);
}

static void
_action_ilist_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _update_action_params(cfdata);
}

static void
_add_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *bm;
   E_Config_Binding_Wheel *bw;

   cfdata = data;

   if (cfdata->mouse_binding < 3)
     {
	bm = E_NEW(E_Config_Binding_Mouse, 1);
	if (!bm) return;
	bm->context = E_BINDING_CONTEXT_ANY;
	switch (cfdata->mouse_binding)
	  {
	   case 0:
	      bm->button = 1;
	      break;
	   case 1:
	      bm->button = 2;
	      break;
	   case 2:
	      bm->button = 3;
	      break;
	  }
	bm->modifiers = E_BINDING_MODIFIER_NONE;
	bm->any_mod = 0;
	bm->action = evas_stringshare_add(action_to_name[0][0]);
	bm->params = NULL;

	cfdata->mouse_bindings[cfdata->mouse_binding] =
	   evas_list_append(cfdata->mouse_bindings[cfdata->mouse_binding], bm);
     }
   else
     {
	bw = E_NEW(E_Config_Binding_Wheel, 1);
	if (!bw) return;
	bw->context = E_BINDING_CONTEXT_ANY; 
	bw->direction = 0;
	if (cfdata->mouse_binding == 3) 
	  bw->z = -1;
	else if (cfdata->mouse_binding == 4)
	  bw->z = 1;
	bw->modifiers = E_BINDING_MODIFIER_NONE;
	bw->any_mod = 0;
	bw->action = evas_stringshare_add(action_to_name[0][0]);
	bw->params = NULL;

	cfdata->mouse_bindings[cfdata->mouse_binding] =
	   evas_list_append(cfdata->mouse_bindings[cfdata->mouse_binding], bw);
     }

   _update_binding_action_list(cfdata);
   _update_delete_button(cfdata);
   e_widget_ilist_selected_set(cfdata->gui.o_binding_action_list,
			       e_widget_ilist_count(cfdata->gui.o_binding_action_list) - 1);
}

static void
_delete_binding_cb(void *data, void *data2)
{
   int current_selection = 0;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Mouse *bm;
   E_Config_Binding_Wheel *bw;

   cfdata = data;
   current_selection = e_widget_ilist_selected_get(cfdata->gui.o_binding_action_list);

   if (cfdata->mouse_binding < 3)
     {
	bm = cfdata->current_mouse_binding;
	cfdata->mouse_bindings[cfdata->mouse_binding] =
	   evas_list_remove(cfdata->mouse_bindings[cfdata->mouse_binding], bm);

	if (bm->action) evas_stringshare_del(bm->action);
	if (bm->params) evas_stringshare_del(bm->params);
	E_FREE(bm);
	cfdata->current_mouse_binding = NULL;
     }
   else
     {
	bw = cfdata->current_wheel_binding;
	cfdata->mouse_bindings[cfdata->mouse_binding] =
	   evas_list_remove(cfdata->mouse_bindings[cfdata->mouse_binding], bw);

	if (bw->action) evas_stringshare_del(bw->action);
	if (bw->params) evas_stringshare_del(bw->params);
	E_FREE(bw);
	cfdata->current_wheel_binding = NULL;
     }

   _update_binding_action_list(cfdata);

   if (!e_widget_ilist_count(cfdata->gui.o_binding_action_list))
     {
	_update_context_radios(cfdata);
	_update_mod_key_check_boxes(cfdata);
	_update_action_list(cfdata);
	_update_action_params(cfdata);
	_update_delete_button(cfdata);
     }
   else
     {
	if (current_selection >= e_widget_ilist_count(cfdata->gui.o_binding_action_list))
	  e_widget_ilist_selected_set(cfdata->gui.o_binding_action_list,
				      e_widget_ilist_count(cfdata->gui.o_binding_action_list) - 1);
	else
	  e_widget_ilist_selected_set(cfdata->gui.o_binding_action_list, current_selection);
     }
}

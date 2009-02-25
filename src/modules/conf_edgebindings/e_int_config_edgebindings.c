/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define TEXT_NONE_ACTION_EDGE _("<None>")
#define TEXT_PRESS_EDGE_SEQUENCE _("Please select an edge,<br>" \
      "or click <hilight>Close</hilight> to abort.<br><br>" \
      "To change the delay of this action,<br>use the slider:" \
      )

#define TEXT_NO_PARAMS _("<None>")

static void	    *_create_data(E_Config_Dialog *cfd);
static void	    _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int	    _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					  E_Config_Dialog_Data *cfdata);

/********* private functions ***************/
static void _fill_actions_list(E_Config_Dialog_Data *cfdata);

/**************** Updates ***********/
static void _update_edge_binding_list(E_Config_Dialog_Data *cfdata);
static void _update_action_list(E_Config_Dialog_Data *cfdata);
static void _update_action_params(E_Config_Dialog_Data *cfdata);
static void _update_buttons(E_Config_Dialog_Data *cfdata);

/**************** Callbacks *********/
static void _binding_change_cb(void *data);
static void _action_change_cb(void *data);
static void _delete_all_edge_binding_cb(void *data, void *data2);
static void _delete_edge_binding_cb(void *data, void *data2);
static void _restore_edge_binding_defaults_cb(void *data, void *data2);
static void _add_edge_binding_cb(void *data, void *data2);
static void _modify_edge_binding_cb(void *data, void *data2);

/********* Helper *************************/
static char *_edge_binding_text_get(E_Zone_Edge edge, float delay, int mod);
static void _auto_apply_changes(E_Config_Dialog_Data *cfdata);
static void _find_edge_binding_action(const char *action, const char *params, int *g, int *a, int *n);

/********* Sorting ************************/
static int _edge_binding_sort_cb(const void *d1, const void *d2);

/**************** grab window *******/
static void _edge_grab_wnd_show(E_Config_Dialog_Data *cfdata);
static void _edge_grab_wnd_cb_apply(void *data, E_Dialog *dia);
static void _edge_grab_wnd_cb_close(void *data, E_Dialog *dia);
static void _edge_grab_wnd_slider_changed_cb(void *data, Evas_Object *obj);
static void _edge_grab_wnd_selected_edge_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edge_grab_wnd_selection_apply(E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   Evas *evas;
   struct
     {
	Eina_List *edge;
     } binding;
   struct
     {
	char *binding, *action, *params;
	const char *cur;
	double delay;
	int cur_act, add;
	E_Zone_Edge edge;
	int modifiers;

	E_Dialog *dia;
     } locals;
   struct
     {
	Evas_Object *o_add, *o_mod, *o_del, *o_del_all;
	Evas_Object *o_binding_list, *o_action_list;
	Evas_Object *o_params, *o_selector, *o_slider;
     } gui;

   const char *params;
   E_Config_Dialog *cfd;
};

EAPI E_Config_Dialog *
e_int_config_edgebindings(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_edgebindings_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Edge Binding Settings"), "E", 
			     "_config_edgebindings_dialog",
			     "enlightenment/edges", 0, v, NULL);
   if ((params) && (params[0]))
     {
	cfd->cfdata->params = eina_stringshare_add(params);
	_add_edge_binding_cb(cfd->cfdata, NULL);
     }

   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Eina_List *l = NULL;
   E_Config_Binding_Edge *bi, *bi2;

   cfdata->locals.action = strdup("");
   cfdata->locals.params = strdup("");
   cfdata->locals.binding = strdup("");
   cfdata->locals.cur = NULL;
   cfdata->locals.dia = NULL;
   cfdata->locals.delay = 0.3;
   cfdata->binding.edge = NULL;

   for (l = e_config->edge_bindings; l; l = l->next)
     {
	bi = l->data;
	if (!bi) continue;

	bi2 = E_NEW(E_Config_Binding_Edge, 1);
	bi2->context = bi->context;
	bi2->edge = bi->edge;
	bi2->modifiers = bi->modifiers;
	bi2->any_mod = bi->any_mod;
	bi2->delay = bi->delay;
	bi2->action = bi->action == NULL ? NULL : eina_stringshare_add(bi->action);
	bi2->params = bi->params == NULL ? NULL : eina_stringshare_add(bi->params);

	cfdata->binding.edge = eina_list_append(cfdata->binding.edge, bi2);
     }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Edge *bi;

   while (cfdata->binding.edge)
     {
	bi = cfdata->binding.edge->data;
	if (bi->action) eina_stringshare_del(bi->action);
	if (bi->params) eina_stringshare_del(bi->params);
	E_FREE(bi);
	cfdata->binding.edge = eina_list_remove_list(cfdata->binding.edge, cfdata->binding.edge);
     }

   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   if (cfdata->locals.binding) free(cfdata->locals.binding);
   if (cfdata->locals.action) free(cfdata->locals.action);
   if (cfdata->locals.params) free(cfdata->locals.params);
   if (cfdata->params) eina_stringshare_del(cfdata->params);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l = NULL;
   E_Config_Binding_Edge *bi, *bi2;

   _auto_apply_changes(cfdata);

   while (e_config->edge_bindings)
     {
	bi = e_config->edge_bindings->data;
	e_bindings_edge_del(bi->context, bi->edge, bi->modifiers, bi->any_mod, 
			   bi->action, bi->params, bi->delay);
	e_config->edge_bindings =
	   eina_list_remove_list(e_config->edge_bindings, e_config->edge_bindings);

	if (bi->action) eina_stringshare_del(bi->action);
	if (bi->params) eina_stringshare_del(bi->params);
	E_FREE(bi);
     }

   for (l = cfdata->binding.edge; l; l = l->next)
     {
	bi2 = l->data;

	if (bi2->edge < 0) continue;

	bi = E_NEW(E_Config_Binding_Edge, 1);
	bi->context = bi2->context;
	bi->edge = bi2->edge;
	bi->modifiers = bi2->modifiers;
	bi->any_mod = bi2->any_mod;
	bi->delay = bi2->delay;
	bi->action =
	   ((!bi2->action) || (!bi2->action[0])) ? NULL : eina_stringshare_add(bi2->action);
	bi->params =
	   ((!bi2->params) || (!bi2->params[0])) ? NULL : eina_stringshare_add(bi2->params);

	e_config->edge_bindings = eina_list_append(e_config->edge_bindings, bi);
	e_bindings_edge_add(bi->context, bi->edge, bi->modifiers, bi->any_mod,
			   bi->action, bi->params, bi->delay);
     }
   e_config_save_queue();

   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *ob;

   cfdata->evas = evas;
   o = e_widget_list_add(evas, 0, 1);
   
   of = e_widget_frametable_add(evas, _("Edge Bindings"), 0);
   ob = e_widget_ilist_add(evas, 32, 32, &(cfdata->locals.binding));
   cfdata->gui.o_binding_list = ob;   
   e_widget_min_size_set(ob, 200, 200);
   e_widget_frametable_object_append(of, ob, 0, 0, 2, 1, 1, 1, 1, 1);
   ob = e_widget_button_add(evas, _("Add Edge"), NULL, _add_edge_binding_cb, cfdata, NULL);
   cfdata->gui.o_add = ob;
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete Edge"), NULL, _delete_edge_binding_cb, cfdata, NULL);
   cfdata->gui.o_del = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Modify Edge"), NULL, _modify_edge_binding_cb, cfdata, NULL);
   cfdata->gui.o_mod = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Delete All"), NULL, _delete_all_edge_binding_cb, cfdata, NULL);
   cfdata->gui.o_del_all = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_frametable_object_append(of, ob, 1, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_button_add(evas, _("Restore Default Bindings"), "enlightenment/e", _restore_edge_binding_defaults_cb, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 0, 3, 2, 1, 1, 0, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   ot = e_widget_table_add(evas, 0);
   of = e_widget_framelist_add(evas, _("Action"), 0);
   ob = e_widget_ilist_add(evas, 24, 24, &(cfdata->locals.action));
   cfdata->gui.o_action_list = ob;
   e_widget_min_size_set(ob, 200, 280);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Action Params"), 0);
   ob = e_widget_entry_add(evas, &(cfdata->locals.params), NULL, NULL, NULL);
   cfdata->gui.o_params = ob;
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);

   _update_edge_binding_list(cfdata);
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

/**************** Callbacks *********/

static void
_add_edge_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);

   cfdata->locals.add = 1;
   _edge_grab_wnd_show(cfdata);
}

static void
_modify_edge_binding_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);

   cfdata->locals.add = 0;
   if (cfdata->locals.cur && cfdata->locals.cur[0])
     { 
	E_Config_Binding_Edge *bi;
	int n;

	if (sscanf(cfdata->locals.cur, "e%d", &n) != 1)
	  return;

	bi = eina_list_nth(cfdata->binding.edge, n);
	cfdata->locals.edge = bi->edge;
	cfdata->locals.delay = ((double) bi->delay);
	cfdata->locals.modifiers = bi->modifiers;
     }
   else return;
   _edge_grab_wnd_show(cfdata);
}

static void
_binding_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   _auto_apply_changes(cfdata);
   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   if ((!cfdata->locals.binding) || (!cfdata->locals.binding[0])) return; 

   cfdata->locals.cur = eina_stringshare_add(cfdata->locals.binding);

   _update_buttons(cfdata);
   _update_action_list(cfdata);
}

static void
_action_change_cb(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _update_action_params(cfdata);
}

static void
_delete_all_edge_binding_cb(void *data, void *data2)
{
   E_Config_Binding_Edge *bi;
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   while (cfdata->binding.edge)
     {
	bi = cfdata->binding.edge->data;
	if (bi->action) eina_stringshare_del(bi->action);
	if (bi->params) eina_stringshare_del(bi->params);
	E_FREE(bi);

	cfdata->binding.edge = eina_list_remove_list(cfdata->binding.edge, cfdata->binding.edge);
     }

   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);
   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);

   _update_buttons(cfdata);
}

static void
_delete_edge_binding_cb(void *data, void *data2)
{
   Eina_List *l = NULL;
   int sel, n;
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Edge *bi;

   cfdata = data;

   sel = e_widget_ilist_selected_get(cfdata->gui.o_binding_list);
   if (cfdata->locals.binding[0] == 'e')
     {
	if (sscanf(cfdata->locals.binding, "e%d", &n) != 1)
	  return;

	l = eina_list_nth_list(cfdata->binding.edge, n);

	if (l)
	  {
	     bi = l->data;
	     if (bi->action) eina_stringshare_del(bi->action);
	     if (bi->params) eina_stringshare_del(bi->params);
	     E_FREE(bi);
	     cfdata->binding.edge = eina_list_remove_list(cfdata->binding.edge, l);
	  }
     }

   _update_edge_binding_list(cfdata);

   if (sel >= e_widget_ilist_count(cfdata->gui.o_binding_list))
     sel = e_widget_ilist_count(cfdata->gui.o_binding_list) - 1;

   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL; 
   
   e_widget_ilist_selected_set(cfdata->gui.o_binding_list, sel);
   if (sel < 0)
     { 
	e_widget_ilist_unselect(cfdata->gui.o_action_list);
	e_widget_entry_clear(cfdata->gui.o_params);
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	_update_buttons(cfdata);
     }
}
static void
_restore_edge_binding_defaults_cb(void *data, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Binding_Edge *bi;

   cfdata = data;

   while (cfdata->binding.edge)
     {
	bi = cfdata->binding.edge->data;
	if (bi->action) eina_stringshare_del(bi->action);
	if (bi->params) eina_stringshare_del(bi->params);
	E_FREE(bi);
	cfdata->binding.edge = eina_list_remove_list(cfdata->binding.edge, cfdata->binding.edge);
     }

#define CFG_EDGEBIND_DFLT(_context, _edge, _modifiers, _anymod, _action, _params, _delay) \
   bi = E_NEW(E_Config_Binding_Edge, 1); \
   bi->context = _context; \
   bi->edge = _edge; \
   bi->modifiers = _modifiers; \
   bi->any_mod = _anymod; \
   bi->delay = _delay; \
   bi->action = _action == NULL ? NULL : eina_stringshare_add(_action); \
   bi->params = _params == NULL ? NULL : eina_stringshare_add(_params); \
   cfdata->binding.edge = eina_list_append(cfdata->binding.edge, bi)

   CFG_EDGEBIND_DFLT(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT,
      	 0, 0, "desk_flip_in_direction", NULL, 0.3);
   CFG_EDGEBIND_DFLT(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT,
      	 0, 0, "desk_flip_in_direction", NULL, 0.3);
   CFG_EDGEBIND_DFLT(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP,
      	 0, 0, "desk_flip_in_direction", NULL, 0.3);
   CFG_EDGEBIND_DFLT(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM,
      	 0, 0, "desk_flip_in_direction", NULL, 0.3);
   
   if (cfdata->locals.cur) eina_stringshare_del(cfdata->locals.cur);
   cfdata->locals.cur = NULL;

   _update_edge_binding_list(cfdata);
   _update_buttons(cfdata);

   e_widget_ilist_unselect(cfdata->gui.o_action_list);
   e_widget_entry_clear(cfdata->gui.o_params);
   e_widget_disabled_set(cfdata->gui.o_params, 1);
}

/**************** Updates ***********/
static void
_update_action_list(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Edge *bi;
   int j = -1, i, n;
   const char *action, *params;

   if (!cfdata->locals.cur) return;

   if (cfdata->locals.cur[0] == 'e')
     {
	if (sscanf(cfdata->locals.cur, "e%d", &n) != 1)
	  return;

	bi = eina_list_nth(cfdata->binding.edge, n);
	if (!bi)
	  {
	     e_widget_ilist_unselect(cfdata->gui.o_action_list);
	     e_widget_entry_clear(cfdata->gui.o_params);
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     return;
	  }
	action = bi->action;
	params = bi->params;
     }
   else
     return;

   _find_edge_binding_action(action, params, NULL, NULL, &j);

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

   /*if (cfdata->locals.cur[0] == 'e')
     {
	sscanf(cfdata->locals.cur, "e%d", &n);
	bi = eina_list_nth(cfdata->binding.edge, n);
	if (!bi)
	  {
	     e_widget_ilist_unselect(cfdata->gui.o_action_list);
	     e_widget_entry_clear(cfdata->gui.o_params);
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     return;
	  }

	_find_edge_binding_action(bi, NULL, NULL, &j);
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
     }*/
}

static void
_update_action_params(E_Config_Dialog_Data *cfdata)
{
   int g, a, b;
   E_Action_Group *actg;
   E_Action_Description *actd;
   E_Config_Binding_Edge *bi;
   const char *action, *params;

#define EDGE_EXAMPLE_PARAMS \
   if ((!actd->param_example) || (!actd->param_example[0])) \
     e_widget_entry_text_set(cfdata->gui.o_params, TEXT_NO_PARAMS); \
   else \
     e_widget_entry_text_set(cfdata->gui.o_params, actd->param_example)


   if ((!cfdata->locals.action) || (!cfdata->locals.action[0]))
     {
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	e_widget_entry_clear(cfdata->gui.o_params);
	return;
     }
   if (sscanf(cfdata->locals.action, "%d %d", &g, &a) != 2)
     return;

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
	EDGE_EXAMPLE_PARAMS;
	return;
     }
   
   if (!actd->editable)
     e_widget_disabled_set(cfdata->gui.o_params, 1);
   else
     e_widget_disabled_set(cfdata->gui.o_params, 0); 

   if (cfdata->locals.cur[0] == 'e')
     {
	if (sscanf(cfdata->locals.cur, "e%d", &b) != 1)
	  {
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     EDGE_EXAMPLE_PARAMS;
	     return;
	  }

	bi = eina_list_nth(cfdata->binding.edge, b);
	if (!bi)
	  {
	     e_widget_disabled_set(cfdata->gui.o_params, 1);
	     EDGE_EXAMPLE_PARAMS;
	     return;
	  }
	action = bi->action;
	params = bi->params;
     }
   else
     {
	e_widget_disabled_set(cfdata->gui.o_params, 1);
	EDGE_EXAMPLE_PARAMS;
	return;
     }

   if (action)
     {
	if (!strcmp(action, actd->act_cmd))
	  {
	     if ((!params) || (!params[0]))
	       EDGE_EXAMPLE_PARAMS;
	     else
	       e_widget_entry_text_set(cfdata->gui.o_params, params);
	  }
	else
	  EDGE_EXAMPLE_PARAMS;
     }
   else
     EDGE_EXAMPLE_PARAMS;
}

static void
_update_edge_binding_list(E_Config_Dialog_Data *cfdata)
{
   int i;
   char *b, b2[64];
   Eina_List *l;
   E_Config_Binding_Edge *bi;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.o_binding_list));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.o_binding_list);
   
   e_widget_ilist_clear(cfdata->gui.o_binding_list);
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   if (cfdata->binding.edge)
     {
	cfdata->binding.edge = eina_list_sort(cfdata->binding.edge,
	      eina_list_count(cfdata->binding.edge), _edge_binding_sort_cb);
     }

   for (l = cfdata->binding.edge, i = 0; l; l = l->next, i++)
     {
	Evas_Object *ic;

	bi = l->data;

	b = _edge_binding_text_get(bi->edge, bi->delay, bi->modifiers);
	if (!b) continue;

	ic = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(ic, "enlightenment/edges");

	snprintf(b2, sizeof(b2), "e%d", i);
	e_widget_ilist_append(cfdata->gui.o_binding_list, ic, b,
			      _binding_change_cb, cfdata, b2);
	free(b);
     }
   e_widget_ilist_go(cfdata->gui.o_binding_list);

   e_widget_ilist_thaw(cfdata->gui.o_binding_list);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.o_binding_list));
   
   if (eina_list_count(cfdata->binding.edge))
     e_widget_disabled_set(cfdata->gui.o_del_all, 0);
   else
     e_widget_disabled_set(cfdata->gui.o_del_all, 1);
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

/*************** Sorting *****************************/
static int
_edge_binding_sort_cb(const void *d1, const void *d2)
{
   int i, j;
   const E_Config_Binding_Edge *bi, *bi2;

   bi = d1;
   bi2 = d2;

   i = 0; j = 0;
   if (bi->modifiers & E_BINDING_MODIFIER_CTRL) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_ALT) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_SHIFT) i++;
   if (bi->modifiers & E_BINDING_MODIFIER_WIN) i++;
   
   if (bi2->modifiers & E_BINDING_MODIFIER_CTRL) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_ALT) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_SHIFT) j++;
   if (bi2->modifiers & E_BINDING_MODIFIER_WIN) j++;
   
   if (i < j) return -1;
   else if (i > j) return 1; 

   if (bi->modifiers < bi2->modifiers) return -1;
   else if (bi->modifiers > bi2->modifiers) return 1;

   if (bi->edge < bi2->edge) return -1;
   else if (bi->edge > bi2->edge) return 1;
   
   return 0;
}

/**************** grab window *******/
static void
_edge_grab_wnd_show(E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *obg, *os;
   E_Manager *man;
   Evas *evas;
   Evas_Coord minw, minh;
   const char *bgfile;
   int tw, th;
   
   if (cfdata->locals.dia != 0) return;

   man = e_manager_current_get();
   
   cfdata->locals.dia = e_dialog_normal_win_new(e_container_current_get(man),
				     "E", "_edgebind_getedge_dialog");
   if (!cfdata->locals.dia) return;
   e_dialog_title_set(cfdata->locals.dia, _("Edge Binding Sequence"));
   e_dialog_icon_set(cfdata->locals.dia, "enlightenment/edges", 48);
   e_dialog_button_add(cfdata->locals.dia, _("Apply"), NULL, _edge_grab_wnd_cb_apply, cfdata);
   e_dialog_button_add(cfdata->locals.dia, _("Close"), NULL, _edge_grab_wnd_cb_close, cfdata);
   e_win_centered_set(cfdata->locals.dia->win, 1);

   evas = e_win_evas_get(cfdata->locals.dia->win);

   cfdata->gui.o_selector = o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/modules/conf_edgebindings",
			   "e/modules/conf_edgebindings/selection");

   cfdata->gui.o_slider = os = e_widget_slider_add(evas, 1, 0, _("%.2f seconds"), 0.0, 2.0, 0.05, 0, &(cfdata->locals.delay), NULL, 200);
   edje_object_part_swallow(o, "e.swallow.slider", os);
   e_widget_on_change_hook_set(os, _edge_grab_wnd_slider_changed_cb, cfdata);
   evas_object_show(os);

   edje_object_part_text_set(o, "e.text.description", TEXT_PRESS_EDGE_SEQUENCE);

   edje_object_size_min_get(o, &minw, &minh);
   if (!minw || !minh)
     edje_object_size_min_calc(o, &minw, &minh);

   e_dialog_content_set(cfdata->locals.dia, o, minw, minh);
   
   bgfile = e_bg_file_get(0, 0, 0, 0);
   obg = e_thumb_icon_add(evas);
   e_icon_fill_inside_set(obg, 0);
   e_thumb_icon_file_set(obg, bgfile, "e/desktop/background");
   edje_object_part_geometry_get(o, "e.swallow.background", NULL, NULL, &tw, &th);
   e_thumb_icon_size_set(obg, tw, th);
   edje_object_part_swallow(o, "e.swallow.background", obg);
   e_thumb_icon_begin(obg);
   evas_object_show(obg);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _edge_grab_wnd_selected_edge_cb, cfdata);

   e_dialog_show(cfdata->locals.dia);
   ecore_x_icccm_transient_for_set(cfdata->locals.dia->win->evas_win, cfdata->cfd->dia->win->evas_win);
}

static void
_edge_grab_wnd_hide(E_Config_Dialog_Data *cfdata)
{
   e_object_del(E_OBJECT(cfdata->locals.dia));
   cfdata->locals.dia = NULL;
}

static void
_edge_grab_wnd_cb_apply(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _edge_grab_wnd_selection_apply(cfdata);
   _edge_grab_wnd_hide(cfdata);
}

static void
_edge_grab_wnd_cb_close(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _edge_grab_wnd_hide(cfdata);
}

static void
_edge_grab_wnd_slider_changed_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;
   char *label = NULL;

   if (!cfdata->locals.edge) return;
   label = _edge_binding_text_get(cfdata->locals.edge, ((float) cfdata->locals.delay), cfdata->locals.modifiers);
   edje_object_part_text_set(cfdata->gui.o_selector, "e.text.selection", label);
   if (label) E_FREE(label);
}

static void
_edge_grab_wnd_selected_edge_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *event;
   E_Config_Dialog_Data *cfdata;
   E_Zone_Edge edge;
   Evas_Coord xx, yy, x, y, w, h;
   char *label;
   
   if (!(cfdata = data)) return;
   if (!(event = event_info)) return;
   if (event->button != 1) return;

   evas_object_geometry_get(cfdata->gui.o_selector, &xx, &yy, NULL, NULL);
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.top_left", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_TOP_LEFT;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.top", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_TOP;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.top_right", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_TOP_RIGHT;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.right", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_RIGHT;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.bottom_right", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_BOTTOM_RIGHT;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.bottom", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_BOTTOM;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.bottom_left", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_BOTTOM_LEFT;
	goto stop;
     }
   edje_object_part_geometry_get(cfdata->gui.o_selector, "e.edge.left", &x, &y, &w, &h);
   if (E_INSIDE(event->canvas.x, event->canvas.y, x + xx, y + yy, w, h))
     {
	edge = E_ZONE_EDGE_LEFT;
	goto stop;
     }
   return;

stop:
   cfdata->locals.edge = edge;
   cfdata->locals.modifiers = 0;

   if (evas_key_modifier_is_set(event->modifiers, "Control"))
     cfdata->locals.modifiers |= E_BINDING_MODIFIER_CTRL;
   if (evas_key_modifier_is_set(event->modifiers, "Shift"))
     cfdata->locals.modifiers |= E_BINDING_MODIFIER_SHIFT;
   if (evas_key_modifier_is_set(event->modifiers, "Alt"))
     cfdata->locals.modifiers |= E_BINDING_MODIFIER_ALT;
   if (evas_key_modifier_is_set(event->modifiers, "Win"))
     cfdata->locals.modifiers |= E_BINDING_MODIFIER_WIN;

   label = _edge_binding_text_get(cfdata->locals.edge, ((float) cfdata->locals.delay), cfdata->locals.modifiers);
   edje_object_part_text_set(cfdata->gui.o_selector, "e.text.selection", label);
   if (label) E_FREE(label);
}

static void
_edge_grab_wnd_selection_apply(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Edge *bi = NULL, *bi2 = NULL;
   Eina_List *l;
   char *label;
   int found = 0, n = -1;
   
   if (cfdata->locals.add)
     {
	found = 0;
	for (l = cfdata->binding.edge; l && !found; l = l->next)
	  {
	     bi = l->data;
	     if ((bi->modifiers == cfdata->locals.modifiers) &&
	         (bi->edge == cfdata->locals.edge) &&
		 ((bi->delay * 1000) == (cfdata->locals.delay * 1000)))
	       found = 1;
	  }
     }
   else
     {
	if (cfdata->locals.cur && cfdata->locals.cur[0] &&
	      (sscanf(cfdata->locals.cur, "e%d", &n) == 1))
	  { 
	     found = 0; 
	     bi = eina_list_nth(cfdata->binding.edge, n);

	     for (l = cfdata->binding.edge; l && !found; l = l->next)
	       {
		  bi2 = l->data;
		  if (bi == bi2) continue;
		  if ((bi->modifiers == cfdata->locals.modifiers) &&
		      (bi->edge == cfdata->locals.edge) &&
		      ((bi->delay * 1000) == (cfdata->locals.delay * 1000)))
		    found = 1;
	       }
	  }
     }

   if (!found)
     {
	if (cfdata->locals.add)
	  {
	     bi = E_NEW(E_Config_Binding_Edge, 1);
	     bi->context = E_BINDING_CONTEXT_ZONE;
	     bi->edge = cfdata->locals.edge;
	     bi->any_mod = 0;
	     bi->delay = (float) (cfdata->locals.delay);
	     bi->action = NULL;
	     bi->params = NULL;
	     bi->modifiers = cfdata->locals.modifiers;
	     cfdata->binding.edge = eina_list_append(cfdata->binding.edge, bi);
	  }
	else
	  {
	     if (cfdata->locals.cur && cfdata->locals.cur[0] &&
		   (sscanf(cfdata->locals.cur, "e%d", &n) == 1))
	       {
		  bi = eina_list_nth(cfdata->binding.edge, n);

		  bi->modifiers = cfdata->locals.modifiers;
		  bi->delay = cfdata->locals.delay;
		  bi->edge = cfdata->locals.edge;
	       }
	  }

	if (cfdata->locals.add) 
	  {
	     _update_edge_binding_list(cfdata);

	     for (l = cfdata->binding.edge, n = 0; l; l = l->next, n++)
	       {
		  if (l->data == bi) break;
	       }
	     e_widget_ilist_selected_set(cfdata->gui.o_binding_list, n);
	     e_widget_ilist_unselect(cfdata->gui.o_action_list);
	     if (cfdata->locals.action) free(cfdata->locals.action);
	     cfdata->locals.action = strdup("");
	     if ((cfdata->params) && (cfdata->params[0]))
	       {
		  int j, g = -1;
		  _find_edge_binding_action("exec", NULL, &g, NULL, &j);
		  if (j >= 0)
		    {
		       e_widget_ilist_unselect(cfdata->gui.o_action_list);
		       e_widget_ilist_selected_set(cfdata->gui.o_action_list, (j + g + 1));
		       e_widget_entry_clear(cfdata->gui.o_params);
		       e_widget_entry_text_set(cfdata->gui.o_params, cfdata->params);
		    } 
	       }
	     else
	       {
		  e_widget_entry_clear(cfdata->gui.o_params);
		  e_widget_disabled_set(cfdata->gui.o_params, 1);
	       }
	  }
	else
	  {
	     label = _edge_binding_text_get(bi->edge, bi->delay, bi->modifiers);
	     e_widget_ilist_nth_label_set(cfdata->gui.o_binding_list, n, label);
	     free(label);
	  }
     }
   else
     { 
	int g, a, j;
	const char *label = NULL;
	E_Action_Group *actg = NULL;
	E_Action_Description *actd = NULL;

	if (cfdata->locals.add) 
	  _find_edge_binding_action(bi->action, bi->params, &g, &a, &j);
	else
	  _find_edge_binding_action(bi2->action, bi2->params, &g, &a, &j);

	actg = eina_list_nth(e_action_groups_get(), g);
	if (actg) actd = eina_list_nth(actg->acts, a);

	if (actd) label = actd->act_name;

	e_util_dialog_show(_("Binding Edge Error"), 
			   _("The binding key sequence, that you choose,"
			     " is already used by <br>" 
			     "<hilight>%s</hilight> action.<br>" 
			     "Please choose another binding edge sequence."), 
			   label ? label : _("Unknown")); 
     }
}


/********** Helper *********************************/
static void
_auto_apply_changes(E_Config_Dialog_Data *cfdata)
{
   int n, g, a, ok;
   E_Config_Binding_Edge *bi;
   E_Action_Group *actg;
   E_Action_Description *actd;

   if ((!cfdata->locals.cur) || (!cfdata->locals.cur[0]) ||
       (!cfdata->locals.action) || (!cfdata->locals.action[0])) return;

   if (sscanf(cfdata->locals.cur, "e%d", &n) != 1)
     return;
   if (sscanf(cfdata->locals.action, "%d %d", &g, &a) != 2)
     return;

   bi = eina_list_nth(cfdata->binding.edge, n);
   if (!bi) return;

   actg = eina_list_nth(e_action_groups_get(), g);
   if (!actg) return;
   actd = eina_list_nth(actg->acts, a);
   if (!actd) return;

   if (bi->action) eina_stringshare_del(bi->action);
   bi->action = NULL;

   if (actd->act_cmd) bi->action = eina_stringshare_add(actd->act_cmd);

   if (bi->params) eina_stringshare_del(bi->params);
   bi->params = NULL;

   if (actd->act_params) 
     bi->params = eina_stringshare_add(actd->act_params);
   else
     {
	ok = 1;
	if (cfdata->locals.params)
	  {
	     if (!strcmp(cfdata->locals.params, TEXT_NO_PARAMS))
	       ok = 0;
	     
	     if ((actd->param_example) && (!strcmp(cfdata->locals.params, actd->param_example)))
	       ok = 0;
	  }
	else
	  ok = 0;

	if (ok)
	  bi->params = eina_stringshare_add(cfdata->locals.params);
     }
}

static void
_find_edge_binding_action(const char *action, const char *params, int *g, int *a, int *n)
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
_edge_binding_text_get(E_Zone_Edge edge, float delay, int mod)
{
   char b[256] = "";

   if (mod & E_BINDING_MODIFIER_CTRL)
     strcat(b, _("CTRL"));

   if (mod & E_BINDING_MODIFIER_ALT)
     {
	if (b[0]) strcat(b, " + ");
	strcat(b, _("ALT"));
     }

   if (mod & E_BINDING_MODIFIER_SHIFT)
     {
	if (b[0]) strcat(b, " + ");
	strcat(b, _("SHIFT"));
     }

   if (mod & E_BINDING_MODIFIER_WIN)
     {
	if (b[0]) strcat(b, " + ");
	strcat(b, _("WIN"));
     }
   
   if (edge)
     {
	char *l;
	if (b[0]) strcat(b, " + ");

	if (edge == E_ZONE_EDGE_LEFT)
	  l = strdup("Left Edge");
	else if (edge == E_ZONE_EDGE_TOP)
	  l = strdup("Top Edge");
	else if (edge == E_ZONE_EDGE_RIGHT)
	  l = strdup("Right Edge");
	else if (edge == E_ZONE_EDGE_BOTTOM)
	  l = strdup("Bottom Edge");
	else if (edge == E_ZONE_EDGE_TOP_LEFT)
	  l = strdup("Top Left Edge");
	else if (edge == E_ZONE_EDGE_TOP_RIGHT)
	  l = strdup("Top Right Edge");
	else if (edge == E_ZONE_EDGE_BOTTOM_RIGHT)
	  l = strdup("Bottom Right Edge");
	else if (edge == E_ZONE_EDGE_BOTTOM_LEFT)
	  l = strdup("Bottom Left Edge");

	strcat(b, l);
	free(l);
     }

   if (delay)
     {
	char buf[20];

	if (b[0]) strcat(b, " ");
	snprintf(buf, 20, "%.2fs", delay);
	strcat(b, buf);
     }

   if (!b[0]) return strdup(TEXT_NONE_ACTION_EDGE);
   return strdup(b);
}

#include "e.h"

#define ACTION_LIST_ICON_W  24
#define ACTION_LIST_ICON_H  24

#define BINDING_LIST_ICON_W 16
#define BINDING_LIST_ICON_H 16

#define BTN_ASSIGN_KEYBINDING_TEXT _("Choose a Key")

#define TEXT_ACTION _("Action")
#define TEXT_NONE_ACTION_KEY _("<None>")
#define TEXT_PRESS_KEY_SEQUENCE _("Please press key sequence,<br>" \
				  "or <hilight>Escape</hilight> to abort")

#define ILIST_ICON_WITH_KEYBIND	    "enlightenment/keys"
#define ILIST_ICON_WITHOUT_KEYBIND  ""

#define AG_UNSORTED _("Unsorted")
#define AG_AN_UNKNOWN _("Unknown")

#define E_BINDING_CONTEXT_NUMBER  10

static void	    *_create_data(E_Config_Dialog *cfd);
static void	    _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int	    _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);


/*******************************************************************************************/
static void	    _e_keybinding_action_ilist_cb_change(void *data, Evas_Object *obj);
static void	    _e_keybinding_binding_ilist_cb_change(void *data, Evas_Object *obj);
static void	    _e_keybinding_default_keybinding_settings(E_Config_Dialog_Data *cfdata);

static void	    _e_keybinding_keybind_cb_del_keybinding(void *data, void *data2);
//static void	    _e_keybinding_keybind_delete_keybinding(E_Config_Dialog_Data *cfdata);

static void	    _e_keybinding_keybind_cb_add_keybinding(void *data, void *data2);

static void	    _e_keybinding_keybind_cb_new_shortcut(void *data, void *data2);

static void	    _e_keybinding_update_binding_list(E_Config_Dialog_Data *cfdata);

static void	    _e_keybinding_update_keybinding_button(E_Config_Dialog_Data *cfdata);
static void	    _e_keybinding_update_add_delete_buttons(E_Config_Dialog_Data *cfdata);
static void	    _e_keybinding_update_context_radios(E_Config_Dialog_Data *cfdata);
static void	    _e_keybinding_update_action_param_entries(E_Config_Dialog_Data *cfdata);

static void	    _e_keybinding_update_binding_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata);
static void	    _e_keybinding_update_action_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata);

static char	    *_e_keybinding_get_keybinding_text(E_Config_Binding_Key *bk);

static int	    _e_keybinding_cb_shortcut_key_down(void *data, int type, void *event);
static int	    _e_keybinding_cb_mouse_handler_dumb(void *data, int type, void *event);

static int	    _e_keybinding_keybind_cb_auto_apply(E_Config_Dialog_Data *cfdata);


static void _fill_data(E_Config_Dialog_Data *cfdata);
static int  _action_group_list_sort_cb(void *e1, void *e2);
static int  _action_group_actions_list_sort_cb(void *e1, void *e2);
/*******************************************************************************************/

typedef struct _action2
{
   const char  *action_name;
   const char  *action_cmd;
   const char  *action_params;
   int	def_action;
   int	restrictions;
   Evas_List   *key_bindings;
} ACTION2;

typedef struct _action_group
{
   const char  *action_group;
   Evas_List   *actions; // Here ACTION2 structure is used.
} ACTION_GROUP;

struct _E_Config_Dialog_Data
{
  E_Config_Dialog *cfd;

  ACTION2      *current_act;
  int	       current_act_selector;

  Evas	*evas;

  int binding_context;
  char *key_action;
  char *key_params;

  struct
    {
      Evas_Object *action_ilist;
      Evas_Object *binding_ilist;

      Evas_Object *btn_add;
      Evas_Object *btn_del;
      Evas_Object *btn_keybind;
      
      Evas_Object *bind_context[E_BINDING_CONTEXT_NUMBER];
      Evas_Object *key_action;
      Evas_Object *key_params;

      E_Dialog	  *confirm_dialog;
    } gui;

  struct {
    Ecore_X_Window  keybind_win;
    Evas_List	    *handlers;
    E_Dialog	    *dia;
  }locals;

  int changed;
};

/*******************************************************************************/


Evas_List   *action_group_list=NULL;

int e_int_config_keybindings_register_action_predef_name(const char *action_group,
							 const char *action_name,
							 const char *action_cmd,
							 const char *action_params,
							 E_Keybindings_Restrict restrictions,
							 int flag)
{
   ACTION_GROUP	  *actg = NULL;
   ACTION2	  *act = NULL;
   Evas_List	  *ll;

   if (!action_group || !action_name)
     return 0;

   for (ll = action_group_list; ll; ll = ll->next)
     {
	actg = ll->data;

	if (!strcmp(actg->action_group, action_group))
	  break;
	actg = NULL;
     }

   if (actg == NULL)
     {
	actg = E_NEW(ACTION_GROUP, 1);
	if (!actg)
	  return 0;

	actg->action_group = evas_stringshare_add(action_group);
	actg->actions = NULL;

	action_group_list = evas_list_append(action_group_list, actg);

	action_group_list = evas_list_sort(action_group_list, evas_list_count(action_group_list),
					   _action_group_list_sort_cb);
     }

   for (ll = actg->actions; ll; ll = ll->next)
     {
	act = ll->data;
	if (!strcmp(act->action_name, action_name))
	  break;
	act = NULL;
     }

   if (act)
     return 1;


   act = E_NEW(ACTION2, 1);
   if (!act)
     return 0;
   
   act->action_name    = evas_stringshare_add(action_name);
   act->action_cmd     = action_cmd == NULL ? NULL : evas_stringshare_add(action_cmd);
   act->action_params  = action_params == NULL ? NULL : evas_stringshare_add(action_params);
   act->restrictions   = restrictions;
   act->def_action     = flag;
   act->key_bindings   = NULL;

   actg->actions = evas_list_append(actg->actions, act);
#if 0
   actg->actions = evas_list_sort(actg->actions, evas_list_count(actg->actions),
				  _action_group_actions_list_sort_cb);
#endif

   return 1;
}

int e_int_config_keybindings_unregister_action_predef_name(const char *action_group,
							   const char *action_name)
{
   ACTION_GROUP	  *actg;
   ACTION2	  *act;
   Evas_List *l, *l2;

   for (l = action_group_list; l; l = l->next)
     {
	actg = l->data;
	if (!strcmp(actg->action_group, action_group))
	  {
	     for (l2 = actg->actions; l2; l2 = l2->next)
	       {
		  act = l2->data;
		  if (!strcmp(act->action_name, action_name))
		    {
		       actg->actions = evas_list_remove_list(actg->actions, l2);

		       if (act->action_name) evas_stringshare_del(act->action_name);
		       if (act->action_cmd) evas_stringshare_del(act->action_cmd);
		       if (act->action_params) evas_stringshare_del(act->action_params);

		       while (act->key_bindings)
			 {
			    E_Config_Binding_Key *eb = act->key_bindings->data;
			    if (eb->key) evas_stringshare_del(eb->key);
			    if (eb->action) evas_stringshare_del(eb->action);
			    if (eb->params) evas_stringshare_del(eb->params);
			    E_FREE(eb);

			    act->key_bindings = evas_list_remove_list(act->key_bindings,
								      act->key_bindings);

			 }
		       E_FREE(act);
		       break;
		    }
	       }

	     if (evas_list_count(actg->actions) == 0)
	       {
		  action_group_list = evas_list_remove_list(action_group_list, l);
		  if (actg->action_group) evas_stringshare_del(actg->action_group);
		  E_FREE(actg);
	       }
	     break;
	  }
     }

   return 1;
}

void e_int_config_keybindings_unregister_all_action_predef_names()
{
   ACTION_GROUP	  *actg;
   ACTION2	  *act;

   while (action_group_list)
     {
	actg = action_group_list->data;

	while (actg->actions)
	  {
	     act = actg->actions->data;

	     if (act->action_name) evas_stringshare_del(act->action_name);
	     if (act->action_cmd) evas_stringshare_del(act->action_cmd);
	     if (act->action_params) evas_stringshare_del(act->action_params);

	     while (act->key_bindings) 
	       { 
		  E_Config_Binding_Key *eb = act->key_bindings->data;
		  if (eb->key) evas_stringshare_del(eb->key);
		  if (eb->action) evas_stringshare_del(eb->action);
		  if (eb->params) evas_stringshare_del(eb->params);
		  E_FREE(eb);

		  act->key_bindings = evas_list_remove_list(act->key_bindings,
							    act->key_bindings);
	       }
	     /*for (l3 = act->key_bindings; l3; l3 = l3->next) 
	       { 
		  E_Config_Binding_Key   *eb = l3->data;
		  if (eb->key) evas_stringshare_del(eb->key);
		  if (eb->action) evas_stringshare_del(eb->action);
		  if (eb->params) evas_stringshare_del(eb->params);
		  E_FREE(eb); 
	       }*/

	     E_FREE(act);

	     actg->actions = evas_list_remove_list(actg->actions, actg->actions);
	  }

	if (actg->action_group) evas_stringshare_del(actg->action_group);
	E_FREE(actg);

	action_group_list = evas_list_remove_list(action_group_list, action_group_list);
     }

   action_group_list = NULL;
}
/*******************************************************************************/


EAPI E_Config_Dialog *
e_int_config_keybindings(E_Container *con)
{
  E_Config_Dialog *cfd;
  E_Config_Dialog_View *v;

  v = E_NEW(E_Config_Dialog_View, 1);

  v->create_cfdata = _create_data;
  v->free_cfdata = _free_data;
  v->basic.apply_cfdata = _basic_apply_data;
  v->basic.create_widgets = _basic_create_widgets;
  v->override_auto_apply = 1;

  cfd = e_config_dialog_new(con, _("Key Binding Settings"), "enlightenment/keys", 0, v, NULL);
  return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Key	*eb, *t;
   Evas_List   *l, *l2, *l3;
   ACTION_GROUP	  *actg;
   ACTION2	  *act;

   e_int_config_keybindings_register_action_predef_name(AG_UNSORTED, AG_AN_UNKNOWN,
						        NULL, NULL, 
						        EDIT_RESTRICT_NONE, 1);

   for (l = e_config->key_bindings; l; l = l->next)
     { 
	int found;
	t = l->data;

	found = 0;
	for (l2 = action_group_list; l2 && !found; l2 = l2->next)
	  {
	     actg = l2->data;

	     /* here we are looking for actions with params */
	     for (l3 = actg->actions; l3 && !found; l3 = l3->next)
	       {
		  act = l3->data;

		  if (((!act->action_cmd || !act->action_cmd[0]) && (t->action && t->action[0])) ||
		      ((!t->action || !t->action[0]) && (act->action_cmd && act->action_cmd[0])))
			continue;

		  if (t->params && t->params[0]) // here we have that action has params
		    {
		       if (!act->action_params || !act->action_params[0])
			 continue;

		       if (strcmp(!act->action_cmd ? "" : act->action_cmd,
				  !t->action ? "" : t->action) == 0 &&
			   strcmp(act->action_params, t->params) == 0)
			 {
			    eb = E_NEW(E_Config_Binding_Key, 1);
			    if (!eb) continue;

			    eb->context = t->context;
			    eb->modifiers = t->modifiers;
			    eb->key = (!t->key) ? evas_stringshare_add("") :
						evas_stringshare_add(t->key);
			    eb->action = (!t->action) ? NULL : evas_stringshare_add(t->action);
			    eb->params = (!t->params) ? NULL : evas_stringshare_add(t->params);
			    eb->any_mod = t->any_mod;

			    act->key_bindings = evas_list_append(act->key_bindings, eb);

			    found = 1;
			 }
		    }
	       }

	     /* here we are looking for actions without parmas and for unsorted actions */
	     for (l3 = actg->actions; l3 && !found; l3 = l3->next)
	       {
		  act = l3->data;

		  if (act->action_params && act->action_params[0])
		    continue;

		  if (!strcmp(actg->action_group, AG_UNSORTED) &&
		      !strcmp(act->action_name, AG_AN_UNKNOWN))
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       if (!eb) continue;

		       eb->context = t->context;
		       eb->modifiers = t->modifiers;
		       eb->key = t->key == NULL ? evas_stringshare_add("") :
						  evas_stringshare_add(t->key);
		       eb->action = t->action == NULL ? NULL : evas_stringshare_add(t->action);
		       eb->params = t->params == NULL ? NULL : evas_stringshare_add(t->params);
		       eb->any_mod = t->any_mod;

		       act->key_bindings = evas_list_append(act->key_bindings, eb);
		       found = 1;
		       break;
		    }

		  if (((!act->action_cmd || !act->action_cmd[0]) && (t->action && t->action[0])) ||
		      ((!t->action || !t->action[0]) && (act->action_cmd && act->action_cmd[0])))
			continue;

		  if (strcmp(!act->action_cmd ? "" : act->action_cmd,
			    !t->action ? "" : t->action) == 0)
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       if (!eb) continue;

		       eb->context = t->context;
		       eb->modifiers = t->modifiers;
		       eb->key = (!t->key) ? evas_stringshare_add("") :
				     evas_stringshare_add(t->key);
		       eb->action = (!t->action) ? NULL : evas_stringshare_add(t->action);
		       eb->params = (!t->params) ? NULL : evas_stringshare_add(t->params);
		       eb->any_mod = t->any_mod;

		       act->key_bindings = evas_list_append(act->key_bindings, eb);

		       found = 1;
		    }
	       }
	  }
     }
   cfdata->locals.keybind_win = 0;
   cfdata->locals.handlers = NULL;
   cfdata->locals.dia = NULL;
   cfdata->changed = 0;
}
static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   
   cfdata->binding_context = -1;//E_BINDING_CONTEXT_ANY;
   cfdata->key_action = strdup("");
   cfdata->key_params = strdup("");
   
   _fill_data(cfdata);
   cfdata->cfd = cfd;
   
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List   *l, *l2;
   E_FREE(cfdata->key_action);
   E_FREE(cfdata->key_params);

   for (l = action_group_list; l; l = l->next)
     {
	ACTION_GROUP *actg = l->data;
	for (l2 = actg->actions; l2; l2 = l2->next)
	  {
	     ACTION2 *act = l2->data;
	     while (act->key_bindings)
	       {
		  E_Config_Binding_Key *eb = act->key_bindings->data;

		  if (eb)
		    {
		       if (eb->key) evas_stringshare_del(eb->key);
		       if (eb->action) evas_stringshare_del(eb->action);
		       if (eb->params) evas_stringshare_del(eb->params);
		       E_FREE(eb);
		    }
		  act->key_bindings = evas_list_remove_list(act->key_bindings, act->key_bindings);
	       }
	  }
     }
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List *l, *l2, *l3;
   if (!cfdata) return 0;

   if (cfdata->current_act)
     if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 1)
       { 
	  //TODO: message box which should ask if we really should proceed.
	  //If yes, then the current 'empty' binding will be deleted
	  //_keybind_delete_keybinding(cfdata);
       }

   e_managers_keys_ungrab();
   while(e_config->key_bindings)
     {
	E_Config_Binding_Key  *eb;

	eb = e_config->key_bindings->data;
	e_bindings_key_del(eb->context, eb->key, eb->modifiers, eb->any_mod,
			   eb->action, eb->params);
	e_config->key_bindings = evas_list_remove_list(e_config->key_bindings,
						       e_config->key_bindings);
	if (eb->key) evas_stringshare_del(eb->key);
	if (eb->action) evas_stringshare_del(eb->action);
	if (eb->params) evas_stringshare_del(eb->params);
	E_FREE(eb);
     }

   for (l = action_group_list; l; l = l->next)
     {
	ACTION_GROUP *actg = l->data;
	for (l2 = actg->actions; l2; l2 = l2->next)
	  {
	     ACTION2 *act = l2->data;
	     for (l3 = act->key_bindings; l3; l3 = l3->next)
	       {
		  E_Config_Binding_Key *eb, *eb2;
		  eb = l3->data;
		  if (!eb || !eb->key || !eb->key[0]) continue;

		  eb2 = E_NEW(E_Config_Binding_Key, 1);
		  if (!eb2) continue;

		  eb2->context = eb->context;
		  eb2->key = evas_stringshare_add(eb->key);
		  eb2->modifiers = eb->modifiers;
		  eb2->any_mod = eb->any_mod;
		  eb2->action = !eb->action || !eb->action[0] ? NULL :
							        evas_stringshare_add(eb->action);
		  eb2->params = !eb->params || !eb->params[0] ? NULL :
							        evas_stringshare_add(eb->params);
		  e_config->key_bindings = evas_list_append(e_config->key_bindings, eb2);
		  e_bindings_key_add(eb->context, eb->key, eb->modifiers, eb->any_mod,
				     eb->action, eb->params);
	       }
	  }
     }
   e_managers_keys_grab();
   e_config_save_queue();
   cfdata->changed = 0;
   return 1;
}
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
  Evas_Object *ot;
  Evas_Object *ob, *of;
  E_Radio_Group *rg;

  cfdata->evas = evas;

  ot = e_widget_table_add(evas, 0);
  {
    of = e_widget_framelist_add(evas, _("Action"), 0);
    {
      cfdata->gui.action_ilist = e_widget_ilist_add(evas, ACTION_LIST_ICON_W,
						    ACTION_LIST_ICON_H, NULL); 
      {
	Evas_List *l, *l2;
	ACTION_GROUP *actg = NULL;
	ACTION2	     *act = NULL;

	e_widget_on_change_hook_set(cfdata->gui.action_ilist, _e_keybinding_action_ilist_cb_change,
				    cfdata);

	for (l = action_group_list; l; l = l->next)
	  {
	     //TODO:possible: do not show Unsorted:Unknow group if there are no unsorted actions.
	     actg = l->data;
	     e_widget_ilist_header_append(cfdata->gui.action_ilist, NULL, actg->action_group);

	     for (l2 = actg->actions; l2; l2 = l2->next)
	       {
		  Evas_Object *ic = NULL;
		  act = l2->data;

		  ic = edje_object_add(evas);
		  if (evas_list_count(act->key_bindings)) 
		    e_util_edje_icon_set(ic, ILIST_ICON_WITH_KEYBIND);
		  else
		    e_util_edje_icon_set(ic, ILIST_ICON_WITHOUT_KEYBIND);

		  e_widget_ilist_append(cfdata->gui.action_ilist, ic,
				        act->action_name, NULL, NULL, NULL);
	       }
	  }
	e_widget_min_size_set(cfdata->gui.action_ilist, 250, 330);
	e_widget_ilist_go(cfdata->gui.action_ilist);
      }
      e_widget_framelist_object_append(of, cfdata->gui.action_ilist);
    }
    e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

    of = e_widget_framelist_add(evas, _("Key Bindings"), 0);
    {
      Evas_Object *ot1;
      /* bindings list */
      cfdata->gui.binding_ilist = e_widget_ilist_add(evas, BINDING_LIST_ICON_W,
						     BINDING_LIST_ICON_H, NULL);
      e_widget_on_change_hook_set(cfdata->gui.binding_ilist, _e_keybinding_binding_ilist_cb_change,
				  cfdata);
      e_widget_min_size_set(cfdata->gui.binding_ilist, 250, 200);
      e_widget_ilist_go(cfdata->gui.binding_ilist);
      e_widget_framelist_object_append(of, cfdata->gui.binding_ilist);
      /****************/

      ot1 = e_widget_table_add(evas, 0);
      {
	/* add keybinding button */
	cfdata->gui.btn_add = e_widget_button_add(evas, _("Add Key"), NULL,
						  _e_keybinding_keybind_cb_add_keybinding, cfdata,
						  NULL);
	e_widget_disabled_set(cfdata->gui.btn_add, 1);
	e_widget_min_size_set(cfdata->gui.btn_add, 140, 28);
	e_widget_table_object_append(ot1, cfdata->gui.btn_add, 0, 0, 1, 1, 1, 1, 1, 1);
	/****************/

	/* delete keybinding button */
	cfdata->gui.btn_del = e_widget_button_add(evas, _("Remove Key"), NULL,
						  _e_keybinding_keybind_cb_del_keybinding, cfdata,
						  NULL);
	e_widget_disabled_set(cfdata->gui.btn_del, 1);
	e_widget_min_size_set(cfdata->gui.btn_del, 140, 28);
	e_widget_table_object_append(ot1, cfdata->gui.btn_del, 1, 0, 1, 1, 1, 1, 1, 1);
	/****************/
      }
      e_widget_framelist_object_append(of, ot1);

#if 0       
      /* context options */
      ot1 = e_widget_frametable_add(evas, _("Binding Context"), 0);
      {
	rg = e_widget_radio_group_new((int *)(&(cfdata->binding_context)));

	// first radio column
	ob = e_widget_radio_add(evas, _("Any"), E_BINDING_CONTEXT_ANY, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 0, 0, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("Border"), E_BINDING_CONTEXT_BORDER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 0, 1, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("Zone"), E_BINDING_CONTEXT_ZONE, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 0, 2, 1, 1, 1, 1, 1, 1);

	// second radio column
	ob = e_widget_radio_add(evas, _("Container"), E_BINDING_CONTEXT_CONTAINER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 1, 0, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("Manager"), E_BINDING_CONTEXT_MANAGER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 1, 1, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("Menu"), E_BINDING_CONTEXT_MENU, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 1, 2, 1, 1, 1, 1, 1, 1);

	// third radio column
	ob = e_widget_radio_add(evas, _("Win List"), E_BINDING_CONTEXT_WINLIST, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 2, 0, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("Popup"), E_BINDING_CONTEXT_POPUP, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 2, 1, 1, 1, 1, 1, 1, 1);

	ob = e_widget_radio_add(evas, _("None"), E_BINDING_CONTEXT_NONE, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 2, 2, 1, 1, 1, 1, 1, 1);

	// Fourth radio column
	ob = e_widget_radio_add(evas, _("Unknown"), E_BINDING_CONTEXT_UNKNOWN, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(ot1, ob, 3, 0, 1, 1, 1, 1, 1, 1);
      }
      e_widget_framelist_object_append(of, ot1);
#endif
       
      /* key action */
      ot1 = e_widget_frametable_add(evas, _("Key & Action"), 0);
      {
	ob = e_widget_label_add(evas, _("Binding"));
	e_widget_frametable_object_append(ot1, ob, 0, 0, 1, 1, 1, 1, 1, 1);

	ob = e_widget_label_add(evas, _(":"));
	e_widget_frametable_object_append(ot1, ob, 1, 0, 1, 1, 1, 1, 1, 1);

	/* assign keybinding button */
	cfdata->gui.btn_keybind = e_widget_button_add(evas, BTN_ASSIGN_KEYBINDING_TEXT, NULL,
						      _e_keybinding_keybind_cb_new_shortcut,
						      cfdata, NULL);
	e_widget_disabled_set(cfdata->gui.btn_keybind, 1);
	e_widget_min_size_set(cfdata->gui.btn_keybind, 180, 28);
	e_widget_frametable_object_append(ot1, cfdata->gui.btn_keybind, 2, 0, 1, 1, 1, 1, 1, 1);
	/****************/

	ob = e_widget_label_add(evas, _("Action"));
	e_widget_frametable_object_append(ot1, ob, 0, 1, 1, 1, 1, 1, 1, 1);

	ob = e_widget_label_add(evas, _(":"));
	e_widget_frametable_object_append(ot1, ob, 1, 1, 1, 1, 1, 1, 1, 1);

	cfdata->gui.key_action = e_widget_entry_add(evas, &(cfdata->key_action));
	e_widget_disabled_set(cfdata->gui.key_action, 1);
	e_widget_min_size_set(cfdata->gui.key_action, 180, 25);
	e_widget_frametable_object_append(ot1, cfdata->gui.key_action, 2, 1, 1, 1, 1, 1, 1, 1);

	ob = e_widget_label_add(evas, _("Parameters"));
	e_widget_frametable_object_append(ot1, ob, 0, 2, 1, 1, 1, 1, 1, 1);

	ob = e_widget_label_add(evas, _(":"));
	e_widget_frametable_object_append(ot1, ob, 1, 2, 1, 1, 1, 1, 1, 1);

	cfdata->gui.key_params = e_widget_entry_add(evas, &(cfdata->key_params));
	e_widget_disabled_set(cfdata->gui.key_params, 1);
	e_widget_min_size_set(cfdata->gui.key_params, 180, 25);
	e_widget_frametable_object_append(ot1, cfdata->gui.key_params, 2, 2, 1, 1, 1, 1, 1, 1);
      }
      e_widget_framelist_object_append(of, ot1);
    }
    e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
  }

  /*_update_keybinding_button(cfdata);
  _update_add_delete_buttons(cfdata);
  _update_context_radios(cfdata);
  _update_action_param_entries(cfdata);*/

  e_dialog_resizable_set(cfd->dia, 0);
  return ot;
}

static void
_e_keybinding_binding_ilist_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   int indx;

   if (!(cfdata = data)) return;

   if (cfdata->current_act)
     if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 1)
       {
	  //TODO: message box which should ask if we really should proceed.
	  //If yes, then the current 'empty' binding will be deleted
	  //_keybind_delete_keybinding(cfdata);
       }

   if (!cfdata->current_act)
     return;

   indx = e_widget_ilist_selected_get(cfdata->gui.binding_ilist);
   if (indx < 0 || indx >= e_widget_ilist_count(cfdata->gui.binding_ilist))
     return;

   cfdata->current_act_selector = indx;

   _e_keybinding_update_context_radios(cfdata);
   _e_keybinding_update_action_param_entries(cfdata);
   _e_keybinding_update_keybinding_button(cfdata);
   _e_keybinding_update_add_delete_buttons(cfdata);

}
static void
_e_keybinding_action_ilist_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List   *l, *l2;
   ACTION_GROUP	  *actg = NULL;
   ACTION2	  *act = NULL;
   char *label;
   int done;

   cfdata = data;

   if (!cfdata) return;

   if (cfdata->current_act)
     if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 1)
       {
	  //TODO: message box which should ask if we really should proceed.
	  //If yes, then the current 'empty' binding will be deleted
	  //_keybind_delete_keybinding(cfdata);
       }
   _e_keybinding_default_keybinding_settings(cfdata);

   label = strdup(e_widget_ilist_selected_label_get(obj));

   done = 0;
   for (l = action_group_list; l && !done; l = l->next)
     {
	actg = l->data;

	for (l2 = actg->actions; l2 && !done; l2 = l2->next)
	  {
	     act = l2->data;

	     if (!strcmp(act->action_name, label))
	       {
		  cfdata->current_act  = act;
		  done = 1;
	       }
	  }
     }

   _e_keybinding_update_binding_list(cfdata);
   _e_keybinding_update_add_delete_buttons(cfdata);
   _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);

#if 0
   if (cfdata->changed == 0)
     {
	e_dialog_button_disable_num_set(cfdata->cfd->dia, 0, 1);
	e_dialog_button_disable_num_set(cfdata->cfd->dia, 1, 1);
     }
#endif
}
static void
_e_keybinding_default_keybinding_settings(E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  cfdata->current_act		 = NULL;
  cfdata->current_act_selector	 = -1;

  cfdata->binding_context = -1;
  E_FREE(cfdata->key_action);
  cfdata->key_action = strdup("");
  E_FREE(cfdata->key_params);
  cfdata->key_params = strdup("");

  _e_keybinding_update_keybinding_button(cfdata);
  _e_keybinding_update_add_delete_buttons(cfdata);
  _e_keybinding_update_context_radios(cfdata);
  _e_keybinding_update_action_param_entries(cfdata);
}

static void
_e_keybinding_update_add_delete_buttons(E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  e_widget_disabled_set(cfdata->gui.btn_add, 1);
  e_widget_disabled_set(cfdata->gui.btn_del, 1);

  if (!cfdata->current_act) return;

  e_widget_disabled_set(cfdata->gui.btn_add, 0);

  if (cfdata->current_act_selector >= 0)
    e_widget_disabled_set(cfdata->gui.btn_del, 0);
}

static void
_e_keybinding_update_keybinding_button(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Key	*eb;

   if (!cfdata) return;

   if (!cfdata->current_act || cfdata->current_act_selector < 0)
     {
	e_widget_disabled_set(cfdata->gui.btn_keybind, 1);
	e_widget_button_label_set(cfdata->gui.btn_keybind, BTN_ASSIGN_KEYBINDING_TEXT);
     }
   else
     {
	eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector);

	e_widget_disabled_set(cfdata->gui.btn_keybind, 0);
	if (eb && eb->key && eb->key[0])
	  {
	     char *b = _e_keybinding_get_keybinding_text(eb);
	     e_widget_button_label_set(cfdata->gui.btn_keybind, b);
	     free(b);
	  }
	else
	  e_widget_button_label_set(cfdata->gui.btn_keybind, BTN_ASSIGN_KEYBINDING_TEXT);
     }
}
static void
_e_keybinding_update_context_radios(E_Config_Dialog_Data *cfdata)
{
  E_Config_Binding_Key	*eb;

  if (!cfdata) return;

  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP], 0);
  e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY], 0);

  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP], 1);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY], 1);

  if (cfdata->current_act_selector < 0) return;

  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP], 0);
  e_widget_disabled_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY], 0);

  if ((eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector)) == NULL)
    return;

  if (eb->context == E_BINDING_CONTEXT_NONE)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_NONE;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_UNKNOWN)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_UNKNOWN;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_BORDER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_BORDER;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_ZONE)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_ZONE;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_CONTAINER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_CONTAINER;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_MANAGER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_MANAGER;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_MENU)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_MENU;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_WINLIST)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_WINLIST;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_POPUP)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_POPUP;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP], 1);
    }

  if (eb->context == E_BINDING_CONTEXT_ANY)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_ANY;
      e_widget_radio_toggle_set(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY], 1);
    }
}

static void
_e_keybinding_update_action_param_entries(E_Config_Dialog_Data *cfdata)
{
  E_Config_Binding_Key	*eb;

  if (!cfdata) return;

  e_widget_entry_text_set(cfdata->gui.key_action, "");
  e_widget_entry_text_set(cfdata->gui.key_params, "");

  e_widget_disabled_set(cfdata->gui.key_action, 1);
  e_widget_disabled_set(cfdata->gui.key_params, 1);

  if (!cfdata->current_act) return;
  if (!(eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector)))
    return;

  if (cfdata->key_action) E_FREE(cfdata->key_action);
  cfdata->key_action = eb->action == NULL ? NULL : strdup(eb->action);

  if (eb->action) e_widget_entry_text_set(cfdata->gui.key_action, eb->action);

  if (cfdata->key_params) E_FREE(cfdata->key_params);
  cfdata->key_params = eb->params == NULL ? NULL : strdup(eb->params);

  if (eb->params) e_widget_entry_text_set(cfdata->gui.key_params, eb->params);

  if (!(cfdata->current_act->restrictions & EDIT_RESTRICT_ACTION))
    e_widget_disabled_set(cfdata->gui.key_action, 0);

  if (!(cfdata->current_act->restrictions & EDIT_RESTRICT_PARAMS))
    e_widget_disabled_set(cfdata->gui.key_params, 0);
}

static void
_e_keybinding_cb_confirm_dialog_yes(void *data)
{
   E_Config_Binding_Key	*eb;
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data))
   if ((!cfdata->current_act) && (cfdata->current_act_selector < 0)) return;

   eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector);
   cfdata->current_act->key_bindings = evas_list_remove(cfdata->current_act->key_bindings, eb);

   if (eb->key) evas_stringshare_del(eb->key);
   if (eb->action) evas_stringshare_del(eb->action);
   if (eb->params) evas_stringshare_del(eb->params);
   E_FREE(eb);

   if (cfdata->current_act_selector >= evas_list_count(cfdata->current_act->key_bindings))
     cfdata->current_act_selector = evas_list_count(cfdata->current_act->key_bindings) - 1;

   _e_keybinding_update_binding_list(cfdata);
   e_widget_ilist_go(cfdata->gui.binding_ilist);
   e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->current_act_selector);

   _e_keybinding_update_keybinding_button(cfdata);
   _e_keybinding_update_add_delete_buttons(cfdata);
   _e_keybinding_update_context_radios(cfdata);
   _e_keybinding_update_action_param_entries(cfdata);

   // nice iface features //
   _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);
   _e_keybinding_update_binding_ilist_cur_selection_icon(cfdata);

  //cfdata->changed = 1;
}

static void
_e_keybinding_keybind_cb_del_keybinding(void *data, void *data2)
{
   char buf[4096];
   E_Config_Dialog_Data *cfdata = data;

   if (!cfdata) return;

   snprintf(buf, sizeof(buf), _("You requested to delete \"%s\" keybinding.<br>"
				"<br>"
				"Are you sure you want to delete it?"),
	    e_widget_ilist_selected_label_get(cfdata->gui.binding_ilist));

   e_confirm_dialog_show(_("Delete?"), "enlightenment/exit", buf, NULL, NULL,
			 _e_keybinding_cb_confirm_dialog_yes, NULL, cfdata, NULL);

}
static void
_e_keybinding_update_binding_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata)
{
  return;
}

static void
_e_keybinding_update_action_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata)
{
   Evas_Object *icon;
   if (!cfdata || !cfdata->current_act) return;


   if (evas_list_count(cfdata->current_act->key_bindings) > 1)
     {
	icon = edje_object_add(cfdata->evas);
	e_util_edje_icon_set(icon, ILIST_ICON_WITH_KEYBIND);
     }
   else if(evas_list_count(cfdata->current_act->key_bindings) == 1)
     { 
	E_Config_Binding_Key  *eb;
	eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector);

	if (eb && eb->key && eb->key[0])
	  {
	     icon = edje_object_add(cfdata->evas);
	     e_util_edje_icon_set(icon, ILIST_ICON_WITH_KEYBIND);
	  }
	else
	  icon = NULL;
     }
   else
     icon = NULL;
   e_widget_ilist_nth_icon_set(cfdata->gui.action_ilist,
			       e_widget_ilist_selected_get(cfdata->gui.action_ilist), icon);
}

static char *
_e_keybinding_get_keybinding_text(E_Config_Binding_Key *bk)
{
  char b[256] = "";

  if (!bk) return strdup(b);

  if (bk->modifiers & E_BINDING_MODIFIER_CTRL)
    strcat(b,_("CTRL"));

  if (bk->modifiers & E_BINDING_MODIFIER_ALT)
    {
      if (b[0])
	strcat(b," + ");
      strcat(b,_("ALT"));
    }

  if (bk->modifiers & E_BINDING_MODIFIER_SHIFT)
    {
      if (b[0])
	strcat(b," + ");
      strcat(b,_("SHIFT"));
    }

  if (bk->modifiers & E_BINDING_MODIFIER_WIN)
    {
      if (b[0])
	strcat(b," + ");
      strcat(b,_("WIN"));
    }

  if (bk->key && bk->key[0])
    {
      if (b[0])
	strcat(b," + ");
      if (strlen(bk->key) == 1)
	{
	  char *l = strdup(bk->key);
	  l[0] = (char)toupper(bk->key[0]);
	  strcat(b, l);
	  free(l);
	}
      else
	strcat(b, bk->key );
    }

  if (!b[0])
    strcpy(b, TEXT_NONE_ACTION_KEY);
  return strdup(b);
}

static void
_e_keybinding_update_binding_list(E_Config_Dialog_Data *cfdata)
{
   int i;
   char buf[4096];
   Evas_List   *l;

   if (!cfdata || !cfdata->current_act) return;

   e_widget_ilist_clear(cfdata->gui.binding_ilist);

   for (l = cfdata->current_act->key_bindings, i = 0; l; l = l->next, i++)
     {
	char *b;
	E_Config_Binding_Key  *eb = l->data;

	if (!eb) continue;

	b = _e_keybinding_get_keybinding_text(eb);
	snprintf(buf, sizeof(buf), "%s %d : %s", TEXT_ACTION, i, b);
	free(b);
	e_widget_ilist_append(cfdata->gui.binding_ilist, NULL, buf, NULL, NULL, NULL);
     }
   _e_keybinding_update_keybinding_button(cfdata);
   _e_keybinding_update_add_delete_buttons(cfdata);
   _e_keybinding_update_context_radios(cfdata);
   _e_keybinding_update_action_param_entries(cfdata);

   _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);
   _e_keybinding_update_binding_ilist_cur_selection_icon(cfdata);

   e_widget_ilist_go(cfdata->gui.binding_ilist);
   e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->current_act_selector);
}

static void
_e_keybinding_keybind_cb_add_keybinding(void *data, void *data2)
{
   E_Config_Binding_Key	*eb;
   E_Config_Dialog_Data *cfdata = data;

   if (!cfdata) return;
   if (!cfdata->current_act) return;

   if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 1)
     {
	//TODO: message box, that a keybinding cannot be added
	//until the current is assigned.
     }

   eb = E_NEW(E_Config_Binding_Key, 1);
   if (!eb) return;

   eb->context = E_BINDING_CONTEXT_ANY;
   eb->key     = evas_stringshare_add("");
   eb->modifiers = E_BINDING_MODIFIER_NONE;
   eb->action = !cfdata->current_act->action_cmd ? evas_stringshare_add("") :
			      evas_stringshare_add(cfdata->current_act->action_cmd);
   eb->params = !cfdata->current_act->action_params ? evas_stringshare_add("") :
			      evas_stringshare_add(cfdata->current_act->action_params);

   cfdata->current_act->key_bindings = evas_list_append(cfdata->current_act->key_bindings, eb);
   cfdata->current_act_selector = evas_list_count(cfdata->current_act->key_bindings) - 1;

   _e_keybinding_update_binding_list(cfdata);

   e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->current_act_selector);
   e_widget_ilist_go(cfdata->gui.binding_ilist);

   _e_keybinding_update_keybinding_button(cfdata);
   _e_keybinding_update_add_delete_buttons(cfdata);
   _e_keybinding_update_context_radios(cfdata);
   _e_keybinding_update_action_param_entries(cfdata);

   // nice iface features //
   _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);
   _e_keybinding_update_binding_ilist_cur_selection_icon(cfdata);
}

static int
_e_keybinding_keybind_cb_auto_apply(E_Config_Dialog_Data *cfdata)
{
   E_Config_Binding_Key	*eb;

   if (!cfdata) return 0;
   if (!cfdata->current_act || cfdata->current_act_selector < 0) return 1;

   eb = evas_list_nth(cfdata->current_act->key_bindings, cfdata->current_act_selector);
   if (!eb) return 0;

   eb->context = cfdata->binding_context;
   eb->any_mod = 0;
   if (eb->action) evas_stringshare_del(eb->action);
   eb->action = (!cfdata->key_action || !cfdata->key_action[0]) ? NULL :
	        evas_stringshare_add(cfdata->key_action);

   if (eb->params) evas_stringshare_del(eb->params);
   eb->params = (!cfdata->key_params || !cfdata->key_params[0]) ? NULL :
	        evas_stringshare_add(cfdata->key_params);
   return 1;
}

static void
_e_keybinding_keybind_cb_new_shortcut(void *data, void *data2)
{
  E_Config_Dialog_Data *cfdata = data;

  if (!cfdata || cfdata->locals.keybind_win != 0) return;

  cfdata->locals.dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
  if (!cfdata->locals.dia) return;
  e_dialog_title_set(cfdata->locals.dia, _("Binding Key Sequence"));
  e_dialog_icon_set(cfdata->locals.dia, "enlightenment/e", 64);
  e_dialog_text_set(cfdata->locals.dia, TEXT_PRESS_KEY_SEQUENCE);
  e_win_centered_set(cfdata->locals.dia->win, 1);

  cfdata->locals.keybind_win = ecore_x_window_input_new(e_manager_current_get()->root,
							0, 0, 1, 1);
  ecore_x_window_show(cfdata->locals.keybind_win);

  e_grabinput_get(cfdata->locals.keybind_win, 0, cfdata->locals.keybind_win);

  cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN,
						      _e_keybinding_cb_shortcut_key_down,
						      cfdata));
  cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
						      _e_keybinding_cb_mouse_handler_dumb,
						      NULL));
  cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
						      _e_keybinding_cb_mouse_handler_dumb,
						      NULL));
  cfdata->locals.handlers = evas_list_append(cfdata->locals.handlers,
			      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
						      _e_keybinding_cb_mouse_handler_dumb, NULL));
  e_dialog_show(cfdata->locals.dia);
}
static void
_e_keybinding_keybind_shortcut_wnd_hide(E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  while (cfdata->locals.handlers)
    {
      ecore_event_handler_del(cfdata->locals.handlers->data);
      cfdata->locals.handlers = evas_list_remove_list(cfdata->locals.handlers,
						      cfdata->locals.handlers);
    }
  cfdata->locals.handlers = NULL;
  e_grabinput_release(cfdata->locals.keybind_win, cfdata->locals.keybind_win);
  ecore_x_window_del(cfdata->locals.keybind_win);
  cfdata->locals.keybind_win = 0;

  e_object_del(E_OBJECT(cfdata->locals.dia));
  cfdata->locals.dia = NULL;
}
static int
_e_keybinding_cb_shortcut_key_down(void *data, int type, void *event)
{
   E_Config_Binding_Key	   *eb;
   E_Config_Dialog_Data	   *cfdata  = data;
   Ecore_X_Event_Key_Down  *ev	    = event;
   
   if (ev->win != cfdata->locals.keybind_win) return 1;
   
   if (!strcmp(ev->keyname, "Escape") &&
       !(ev->modifiers & ECORE_X_MODIFIER_SHIFT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_CTRL) &&
       !(ev->modifiers & ECORE_X_MODIFIER_ALT) &&
       !(ev->modifiers & ECORE_X_MODIFIER_WIN))
     {
	_e_keybinding_keybind_shortcut_wnd_hide(cfdata);
     }
   else
     {
	printf("'%s' '%s' '%s'\n", ev->keyname, ev->keysymbol, ev->key_compose);
	if (!strcmp(ev->keyname, "Control_L") || !strcmp(ev->keyname, "Control_R") ||
	    !strcmp(ev->keyname, "Shift_L") || !strcmp(ev->keyname, "Shift_R") ||
	    !strcmp(ev->keyname, "Alt_L") || !strcmp(ev->keyname, "Alt_R") ||
	    !strcmp(ev->keyname, "Super_L") || !strcmp(ev->keyname, "Super_R"))
	  ;
	else
	  {
	     if (cfdata && cfdata->current_act && cfdata->current_act_selector >= 0)
	       {
                  ACTION_GROUP *actg;
                  ACTION2 *act;
		  Evas_List   *l, *l2, *l3;
		  int found;
		  int mod = E_BINDING_MODIFIER_NONE;

		  if (ev->modifiers & ECORE_X_MODIFIER_SHIFT)
		    mod |= E_BINDING_MODIFIER_SHIFT;
		  if (ev->modifiers & ECORE_X_MODIFIER_CTRL)
		    mod |= E_BINDING_MODIFIER_CTRL;
		  if (ev->modifiers & ECORE_X_MODIFIER_ALT)
		    mod |= E_BINDING_MODIFIER_ALT;
		  if (ev->modifiers & ECORE_X_MODIFIER_WIN)
		    mod |= E_BINDING_MODIFIER_WIN;

		  found = 0;
		  for (l = action_group_list; l && !found; l = l->next)
		    {
                       actg = l->data;
		       for (l2 = actg->actions; l2 && !found; l2 = l2->next)
			 {
                            act = l2->data;
			    for (l3 = act->key_bindings; l3 && !found; l3 = l3->next)
			      {
				 eb = l3->data;
				 if (eb->modifiers == mod && !strcmp(ev->keyname, eb->key))
				   found = 1;
			      }
			 }
		    }

		  if (!found)
		    {
		       eb = evas_list_nth(cfdata->current_act->key_bindings,
					  cfdata->current_act_selector);
		       eb->modifiers = mod;
		       if (eb->key) evas_stringshare_del(eb->key);
		       eb->key = evas_stringshare_add(ev->keyname);

		       _e_keybinding_update_binding_list(cfdata);
		    }
		  else
                    {
                       char buf[4096];

                       snprintf(buf, sizeof(buf),
                                _("The binding key sequence, that you choose,"
                                  " is already used by<br>"
                                  "<hilight>%s</hilight> action.<br>"
                                  "Please choose another binding key"
                                  " sequence."),
                                act->action_name);
                       e_util_dialog_show(_("Binding Key Error"), buf);
                    }

	       }
	       _e_keybinding_keybind_shortcut_wnd_hide(cfdata);
	  }
     }
   return 1;
}
static int
_e_keybinding_cb_mouse_handler_dumb(void *data, int type, void *event)
{
   return 1;
}

/*******************/
static int  _action_group_list_sort_cb(void *e1, void *e2)
{
   ACTION_GROUP	  *actg1 = e1;
   ACTION_GROUP	  *actg2 = e2;

   if (!e1) return 1;
   if (!e2) return -1;

   if (!strcmp(actg1->action_group, AG_UNSORTED)) return 1;
   if (!strcmp(actg2->action_group, AG_UNSORTED)) return -1;

   return strcmp(actg1->action_group, actg2->action_group);
}
static int  _action_group_actions_list_sort_cb(void *e1, void *e2)
{
   ACTION2  *act1 = e1;
   ACTION2  *act2 = e2;

   if (!e1) return 1;
   if (!e2) return -1;

   if (!strcmp(act1->action_name, AG_AN_UNKNOWN)) return 1;
   if (!strcmp(act2->action_name, AG_AN_UNKNOWN)) return -1;

   return strcmp(act1->action_name, act2->action_name);
}

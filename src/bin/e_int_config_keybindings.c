#include "e.h"

#define	BTN_NEXT_KEYBINDING_TEXT  _("Next Key Binding")
#define BTN_PREV_KEYBINDING_TEXT  _("Prev Key Binding")
#define BTN_ADD_KEYBINDING_TEXT	  _("Add Key Binding")
#define BTN_DEL_KEYBINDING_TEXT	  _("Delete Key Binding")

#define ILIST_ICON_WITH_KEYBIND	    "enlightenment/e"
#define ILIST_ICON_WITHOUT_KEYBIND  ""

#define	_DEFAULT_ACTION	0
#define _NONDEFAULT_ACTION  1

#define EDIT_RESTRICT_NONE  (0 << 0)
#define EDIT_RESTRICT_ACTION (1 << 0)
#define EDIT_RESTRICT_PARAMS (1 << 1)

#define E_BINDING_CONTEXT_NUMBER  10

static void	    *_create_data(E_Config_Dialog *cfd);
static void	    _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int	    _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);

static void	    _ilist_kb_cb_change(void *data, Evas_Object *obj);

static void	    _keybind_cb_next_keybind(void *data, void *data2);
static void	    _keybind_cb_prev_keybind(void *data, void *data2);
static void	    _keybind_cb_add_keybinding(void *data, void *data2);
static void	    _keybind_cb_del_keybinding(void *data, void *data2);

static void	    _entry_keybind_cb_text_change(void *data, Evas_Object *obj);

static int	    _keybind_cb_auto_apply(E_Config_Dialog_Data *cfdata);
static int	    _keybind_delete_keybinding(E_Config_Dialog_Data *cfdata);

static void	    _update_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata);

typedef struct _E_Config_KeyBind  E_Config_KeyBind;
typedef struct _E_Widget_IList_Data	  E_Widget_IList_Data;
typedef struct _E_Widget_Radio_Data	  E_Widget_Radio_Data;
typedef struct _E_Widget_Checkbox_Data	  E_Widget_Checkbox_Data;
typedef struct _E_Widget_Button_Data	  E_Widget_Button_Data;
typedef struct _E_Widget_Entry_Data	  E_Widget_Entry_Data;

typedef struct _E_Smart_Item		  E_Smart_Item;
typedef struct _E_Smart_Data		  E_Smart_Data;

/*typedef struct
{
  char *key;
  int  modifiers;
  int  context;
}KEY_ACTION_BINDING;*/

typedef struct
{
  char  *action_name;
  char  *action_cmd;
  char  *action_params;
  int	def_action;
  int	restrictions;
}ACTION;

const ACTION actions_predefined_names[ ] = {
  {"Flip Desktop Left", "desk_flip_by", "-1 0", _DEFAULT_ACTION,
						EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Flip Desktop Right", "desk_flip_by", "1 0", _DEFAULT_ACTION,
						EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Flip Desktop Up", "desk_flip_by", "0 -1", _DEFAULT_ACTION,
					      EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Flip Desktop Down", "desk_flip_by", "0 1", _DEFAULT_ACTION,
					       EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Flip Desktop By", "desk_flip_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Flip Desktop To", "desk_flip_to", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Switch To Left Desktop", "desk_linear_flip_by", "-1", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Right Desktop", "desk_linear_flip_by", "1", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Flip Desktop Linearly", "desk_linear_flip_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  
  {"Switch To Desktop 0", "desk_linear_flip_to", "0", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 1", "desk_linear_flip_to", "1", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 2", "desk_linear_flip_to", "2", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 3", "desk_linear_flip_to", "3", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 4", "desk_linear_flip_to", "4", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 5", "desk_linear_flip_to", "5", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 6", "desk_linear_flip_to", "6", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 7", "desk_linear_flip_to", "7", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 8", "desk_linear_flip_to", "8", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 9", "desk_linear_flip_to", "9", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 10", "desk_linear_flip_to", "10", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop 11", "desk_linear_flip_to", "11", _DEFAULT_ACTION,
						    EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Switch To Desktop", "desk_linear_flip_to", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Run Command", "exebuf", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Exit \'E\'", "exit", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Restart \'E\'", "restart", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Show Main Menu", "menu_show", "main", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Show Favorites Menu", "menu_show", "favorites", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Show Clients Menu", "menu_show", "clients", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Menu Show", "menu_show", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Desktop Lock", "desk_lock", NULL, _DEFAULT_ACTION, 
				      EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Toggle Edit Mode", "edit_mode_toggle", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Window List - Next Window", "winlist", "next", _DEFAULT_ACTION, 
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window List - Previous Window", "winlist", "prev", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Window Move To Next Desktop", "window_desk_move_by", "1 0", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Previous Desktop", "window_desk_move_by", "-1 0", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move By Desktop", "window_desk_move_by", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Window Move To Desktop 0", "window_desk_move_to", "0", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 1", "window_desk_move_to", "1", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 2", "window_desk_move_to", "2", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 3", "window_desk_move_to", "3", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 4", "window_desk_move_to", "4", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 5", "window_desk_move_to", "5", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 6", "window_desk_move_to", "6", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 7", "window_desk_move_to", "7", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 8", "window_desk_move_to", "8", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 9", "window_desk_move_to", "9", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 10", "window_desk_move_to", "10", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop 11", "window_desk_move_to", "11", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Move To Desktop", "window_desk_move_to", NULL, _NONDEFAULT_ACTION,
							  EDIT_RESTRICT_ACTION },

  {"Window Maximize", "window_maximized_toggle", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Maximize Vertically", "window_maximized_toggle", "vertical", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Maximize Horizontally", "window_maximized_toggle", "horizontal", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Window Iconic Mode Toggle", "window_iconic_toggle", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Shaded Up Mode Toggle", "window_shaded_toggle", "up", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Shaded Down Mode Toggle", "window_shaded_toggle", "down", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Shaded Left Mode Toggle", "window_shaded_toggle", "left", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Shaded Right Mode Toggle", "window_shaded_toggle", "right", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Sticky Mode Toggle", "window_sticky_toggle", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Window Lower", "window_lower", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Raise", "window_raise", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Close", "window_close", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Kill", "window_kill", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Window Menu", "window_menu", NULL, _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Window Move", "window_move", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Resize", "window_resize", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Move To", "window_move_to", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Move By", "window_move_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Resize", "window_resize_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Drag Icon", "window_drag_icon", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Application", "app", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"User Defined Action", "exec", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
//  {"Unknown Action", NULL, NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_NONE },
  {NULL, NULL, NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_NONE }
};

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object   *smart_obj;
   Evas_Object   *box_obj;
   Evas_List     *items;
   int            selected;
   Evas_Coord     icon_w, icon_h;
   unsigned char  selector : 1;
};

struct _E_Smart_Item
{
   E_Smart_Data  *sd;
   Evas_Object   *base_obj;
   Evas_Object   *icon_obj;
   void         (*func) (void *data, void *data2);
   void         (*func_hilight) (void *data, void *data2);
   void          *data;
   void          *data2;
};

struct _E_Widget_IList_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_ilist;
   Evas_List *callbacks;
   char **value;
};

struct _E_Widget_Radio_Data
{
   E_Radio_Group *group;
   Evas_Object *o_radio;
   int valnum;
};

struct _E_Widget_Checkbox_Data
{
  Evas_Object *o_check;
  int *valptr;
};

struct _E_Widget_Button_Data
{
   Evas_Object *o_button;
   Evas_Object *o_icon;
   void (*func) (void *data, void *data2);
   void *data;
   void *data2;   
};

struct _E_Widget_Entry_Data
{
   Evas_Object *o_entry;
   Evas_Object *obj;
   char **valptr;
   void (*on_change_func) (void *data, Evas_Object *obj);
   void  *on_change_data;
};

struct _E_Config_KeyBind
{
  int acn;
  Evas_List *bk_list;
};

struct _E_Config_Dialog_Data
{
  Evas_List *key_bindings;

  int cur_eckb_kb_sel;
  E_Config_KeyBind  *cur_eckb;
  Evas	*evas;

  int binding_context;
  struct
    {
      int shift;
      int ctrl;
      int alt;
      int win;
    } bind_mod;
  char *key_bind;
  char *key_action;
  char *key_params;

  struct
    {
      Evas_Object *ilist;
      /*Evas_Object *btn_add;
      Evas_Object *btn_del;*/
      
      Evas_Object *btn_prev_keybind;
      Evas_Object *btn_next_keybind;
      Evas_Object *btn_add_keybind;
      Evas_Object *btn_del_keybind;

      Evas_Object *bind_context[E_BINDING_CONTEXT_NUMBER];
      struct
	{
	  Evas_Object *shift;
	  Evas_Object *ctrl;
	  Evas_Object *alt;
	  Evas_Object *win;
	} bind_mod_obj;
      Evas_Object *key_bind;
      Evas_Object *key_action;
      Evas_Object *key_params;
    } gui;
};


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

  cfd = e_config_dialog_new(con, _("Key Binding Settings"), NULL, 0, v, NULL);
  return cfd;
}

static void
_fill_keybindings_data(E_Config_Dialog_Data *cfdata)
{
  int i, j;
  int found;
  E_Config_KeyBind  *eckb;
  E_Config_Binding_Key	*bk, *t;
  const char *action_name = NULL;

  for( i = 0; actions_predefined_names[i].action_name; i++ )
    {
      eckb = E_NEW(E_Config_KeyBind, 1);
      if(!eckb)
	continue;

      eckb->acn = i;

      eckb->bk_list = NULL;
      cfdata->key_bindings = evas_list_append(cfdata->key_bindings, eckb);
      eckb = NULL;
    }

  for (i = 0; i < evas_list_count(e_config->key_bindings); i++)
    {
      t = evas_list_nth(e_config->key_bindings, i);

      bk = NULL;
      for (j = 0; j < evas_list_count(cfdata->key_bindings); j++)
	{
	  eckb = evas_list_nth(cfdata->key_bindings, j);

	  if (t->params == NULL)
	    {
	      // should I check if actions_pre... has a non-NULL params ?
	      if (strcasecmp(actions_predefined_names[eckb->acn].action_cmd == NULL ? "" :
			     actions_predefined_names[eckb->acn].action_cmd, t->action) == 0)
		{
		  // here we found a first occurience of such action;
		  bk = E_NEW(E_Config_Binding_Key, 1);
		  if (!bk)
		    continue;

		  bk->context = t->context;
		  bk->key = strdup(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? NULL : strdup(t->action);
		  bk->params = t->params == NULL ? NULL : strdup(t->params);

		  eckb->bk_list = evas_list_append(eckb->bk_list, bk);
		  break;
		}
	    }
	  else if (t->action != NULL && t->params != NULL)
	    {
	      if (strcasecmp(actions_predefined_names[eckb->acn].action_cmd == NULL ? 
			     "" : actions_predefined_names[eckb->acn].action_cmd, 
			     t->action) == 0 &&
		  strcasecmp(actions_predefined_names[eckb->acn].action_params == NULL ? 
			     "" : actions_predefined_names[eckb->acn].action_params,
			     t->params) == 0)
		{
		  bk = E_NEW(E_Config_Binding_Key, 1);
		  if (!bk)
		    continue;

		  bk->context = t->context;
		  bk->key = strdup(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? NULL : strdup(t->action);
		  bk->params = t->params == NULL ? NULL : strdup(t->params);

		  eckb->bk_list = evas_list_append(eckb->bk_list, bk);
		  break;
		}
	    }

	  if (!bk && actions_predefined_names[eckb->acn].def_action == _NONDEFAULT_ACTION)
	    {
	      if (strcasecmp(actions_predefined_names[eckb->acn].action_cmd == NULL ? 
			     "" : actions_predefined_names[eckb->acn].action_cmd, 
			     t->action) == 0 )
		{
		  bk = E_NEW(E_Config_Binding_Key, 1);
		  if (!bk)
		    continue;

		  bk->context = t->context;
		  bk->key = strdup(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? NULL : strdup(t->action);
		  bk->params = t->params == NULL ? NULL : strdup(t->params);

		  eckb->bk_list = evas_list_append(eckb->bk_list, bk);
		  break;
		}
	    }// if (!bk ...)
	}
    }
}

static void *
_create_data(E_Config_Dialog *cfd)
{
  E_Config_Dialog_Data *cfdata;

  cfdata = E_NEW(E_Config_Dialog_Data, 1);
  
  cfdata->binding_context = E_BINDING_CONTEXT_ANY;
  cfdata->key_bind = strdup("");
  cfdata->key_action = strdup("");
  cfdata->key_params = strdup("");
  return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  unsigned int i, j;
  unsigned int size, size2;
  E_Config_KeyBind  *eckb;
  E_Config_Binding_Key *bk;


  E_FREE(cfdata->key_bind);
  E_FREE(cfdata->key_action);
  E_FREE(cfdata->key_params);

  size = evas_list_count(cfdata->key_bindings);
  for (i = 0; i < size; i++)
    {
      eckb = evas_list_nth(cfdata->key_bindings, i);
      if (eckb)
	{
	  size2 = evas_list_count(eckb->bk_list);
	  for (j = 0; j < size2; j++)
	    {
	      bk = evas_list_nth(eckb->bk_list, j);
	      if (bk)
		{
		  E_FREE(bk->key);
		  E_FREE(bk->action);
		  E_FREE(bk->params);
		  E_FREE(bk);
		}
	    }
	  evas_list_free(eckb->bk_list);
	  E_FREE(eckb);
	}
    }
  evas_list_free(cfdata->key_bindings);
  free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  E_Config_KeyBind  *eckb;
  E_Config_Binding_Key	*bk, *bk2;
  size_t size, size2;
  unsigned int i, j;

  //we should call for autoapply here, since this prevents a bug, when only one
  //NEW keybinding is made and apply/ok button is pressed. Thus, this keybinding does
  //not lost.
  if (cfdata->cur_eckb)
    if (_keybind_cb_auto_apply(cfdata) != 0)
      {
	//TODO: message box which should ask if we really should proceed.
	//If yes, then the current 'empty' binding will be deleted
	_keybind_delete_keybinding(cfdata);
      }

  // here the removing of the old keybindings goes
  while (e_config->key_bindings)
    {
       E_Config_Binding_Key *eb;
       
       eb = e_config->key_bindings->data;
       e_config->key_bindings  = evas_list_remove_list(e_config->key_bindings,
						       e_config->key_bindings);
       if (eb->key) evas_stringshare_del(eb->key);
       if (eb->action) evas_stringshare_del(eb->action);
       if (eb->params) evas_stringshare_del(eb->params);
       E_FREE(eb);
    }

  // here the adding of new keybinds are going.
  size = evas_list_count(cfdata->key_bindings);
  for (i = 0; i < size; i++)
    {
      eckb = evas_list_nth(cfdata->key_bindings, i);
      if (eckb)
	{
	  size2 = evas_list_count(eckb->bk_list);
	  for (j = 0; j < size2; j++)
	    {
	      bk = evas_list_nth(eckb->bk_list, j);
	      if (bk && bk->key && strlen(bk->key) > 0)
		{
		  bk2 = E_NEW(E_Config_Binding_Key, 1);
		  if (bk2)
		    {
		      bk2->context = bk->context;
		      bk2->key = (char *)evas_stringshare_add(bk->key);
		      bk2->modifiers = bk->modifiers;
		      bk2->any_mod = bk->any_mod;
		      bk2->action = bk->action == NULL ? NULL : 
							 (char *)evas_stringshare_add(bk->action);
		      bk2->params = bk->params == NULL ? NULL : 
							 (char *)evas_stringshare_add(bk->params);
		      e_config->key_bindings = evas_list_append(e_config->key_bindings, bk2);
		    }
		}
	    }
	}
    }
  e_config_save_queue();
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
  int i;
  Evas_Object *o, *oft, *oft1, *of1, *of2, *ot, *ot1, *btn, *ilist;
  Evas_Object *ob;
  E_Radio_Group *rg;
  int mw, mh;

  cfdata->evas = evas;
  _fill_keybindings_data(cfdata);

  o = e_widget_list_add(evas, 0, 0);

  oft = e_widget_table_add(evas, 0);
  {
    of1 = e_widget_framelist_add(evas, _("Action"), 0);
    {
      cfdata->gui.ilist = ilist = e_widget_ilist_add(evas, 16, 16, NULL);
      {
	e_widget_on_change_hook_set(ilist, _ilist_kb_cb_change, cfdata);

	for( i = 0; i < evas_list_count(cfdata->key_bindings); i++ )
	  {
	    Evas_Object *ic;
	    E_Config_KeyBind  *eckb;

	    eckb = evas_list_nth(cfdata->key_bindings, i);
	    ic = edje_object_add(evas);
	    if (eckb && eckb->bk_list != NULL)
	      e_util_edje_icon_set(ic, ILIST_ICON_WITH_KEYBIND);
	    else
	      e_util_edje_icon_set(ic, ILIST_ICON_WITHOUT_KEYBIND);

	    e_widget_ilist_append(ilist, ic, 
				  _(actions_predefined_names[eckb->acn].action_name),
				  NULL, NULL, NULL);
	  }
	e_widget_min_size_set(ilist, 250, 330);
	e_widget_ilist_go(ilist);
      }
      e_widget_framelist_object_append(of1, ilist);
      
      /*ot = e_widget_table_add(evas, 0);
      {
	btn = e_widget_button_add(evas, _("Add"), NULL, NULL/callback/, NULL, NULL);
	cfdata->gui.btn_add = btn;
	//e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 110, 28);
	e_widget_table_object_append(ot, btn, 1, 0, 1, 1, 1, 1, 1, 1);

	btn = e_widget_button_add(evas, _("Delete"), NULL, NULL/callback/, NULL, NULL);
	cfdata->gui.btn_del = btn;
	e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 110, 28);
	e_widget_table_object_append(ot, btn, 2, 0, 1, 1, 1, 1, 1, 1);
      }
      e_widget_framelist_object_append(of1, ot );*/
    }
    e_widget_table_object_append(oft, of1, 0, 0, 1, 1, 1, 1, 1, 1);

    of2 = e_widget_framelist_add(evas, _("Parameters"), 0);
    {
      rg = e_widget_radio_group_new((int *)(&(cfdata->binding_context)));
      oft1 = e_widget_frametable_add(evas, _("Binding Context"), 0);
      {
	// first radio column
	ob = e_widget_radio_add(evas, _("Any"), E_BINDING_CONTEXT_ANY, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 0, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("Border"), E_BINDING_CONTEXT_BORDER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 0, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("Zone"), E_BINDING_CONTEXT_ZONE, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 0, 2, 1, 1, 1, 0, 1, 0);

	// second radio column
	ob = e_widget_radio_add(evas, _("Container"), E_BINDING_CONTEXT_CONTAINER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 1, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("Manager"), E_BINDING_CONTEXT_MANAGER, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 1, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("Menu"), E_BINDING_CONTEXT_MENU, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 1, 2, 1, 1, 1, 0, 1, 0);

	// third radio column
	ob = e_widget_radio_add(evas, _("Win List"), E_BINDING_CONTEXT_WINLIST, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 2, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("Popup"), E_BINDING_CONTEXT_POPUP, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 2, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_radio_add(evas, _("None"), E_BINDING_CONTEXT_NONE, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 2, 2, 1, 1, 1, 0, 1, 0);

	// Fourth radio column
	ob = e_widget_radio_add(evas, _("Unknown"), E_BINDING_CONTEXT_UNKNOWN, rg);
	cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN] = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 3, 0, 1, 1, 1, 0, 1, 0);
      }
      e_widget_framelist_object_append(of2, oft1);

      oft1 = e_widget_frametable_add(evas, _("Key Binding"), 0);
      {
	ob = e_widget_label_add(evas, _("Key :"));
	e_widget_frametable_object_append(oft1, ob, 0, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_entry_add(evas, &(cfdata->key_bind));
	e_widget_entry_on_change_callback_set(ob, _entry_keybind_cb_text_change, cfdata);
	cfdata->gui.key_bind = ob;
	//e_widget_disabled_set(ob, 1);
	e_widget_min_size_set(ob, 200, 25);
	e_widget_frametable_object_append(oft1, ob, 1, 0, 4, 1, 1, 0, 1, 0);

	ob = e_widget_check_add(evas, _("Shift"), &(cfdata->bind_mod.shift));
	cfdata->gui.bind_mod_obj.shift = ob;
	//e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 0, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_check_add(evas, _("Control"), &(cfdata->bind_mod.ctrl));
	cfdata->gui.bind_mod_obj.ctrl = ob;
	//e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 1, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_check_add(evas, _("Alt"), &(cfdata->bind_mod.alt));
	cfdata->gui.bind_mod_obj.alt = ob;
	//e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 2, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_check_add(evas, _("Win"), &(cfdata->bind_mod.win));
	cfdata->gui.bind_mod_obj.win = ob;
	//e_widget_disabled_set(ob, 1);
	e_widget_frametable_object_append(oft1, ob, 3, 1, 1, 1, 1, 0, 1, 0);
      }
      e_widget_framelist_object_append(of2, oft1);

      oft1 = e_widget_frametable_add(evas, _("Key Action"), 0);
      {
	ob = e_widget_label_add(evas, _("Action :"));
	e_widget_frametable_object_append(oft1, ob, 0, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_entry_add(evas, &(cfdata->key_action));
	cfdata->gui.key_action = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_min_size_set(ob, 200, 25);
	e_widget_frametable_object_append(oft1, ob, 1, 0, 1, 1, 1, 0, 1, 0);

	ob = e_widget_label_add(evas, _("Params :"));
	e_widget_frametable_object_append(oft1, ob, 0, 1, 1, 1, 1, 0, 1, 0);

	ob = e_widget_entry_add(evas, &(cfdata->key_params));
	cfdata->gui.key_params = ob;
	e_widget_disabled_set(ob, 1);
	e_widget_min_size_set(ob, 200, 25);
	e_widget_frametable_object_append(oft1, ob, 1, 1, 1, 1, 1, 0, 1, 0);
      }
      e_widget_framelist_object_append(of2, oft1);

      oft1 = e_widget_table_add(evas, 0);
      {
	btn = e_widget_button_add(evas, BTN_PREV_KEYBINDING_TEXT, NULL, _keybind_cb_prev_keybind,
				  cfdata, NULL);
	cfdata->gui.btn_prev_keybind = btn;
	e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 140, 28);
	e_widget_table_object_append(oft1, btn, 1, 0, 1, 1, 1, 1, 1, 1);

	btn = e_widget_button_add(evas, BTN_NEXT_KEYBINDING_TEXT, NULL, _keybind_cb_next_keybind,
				  cfdata, NULL);
	cfdata->gui.btn_next_keybind = btn;
	e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 140, 28);
	e_widget_table_object_append(oft1, btn, 2, 0, 1, 1, 1, 1, 1, 1);

	btn = e_widget_button_add(evas, BTN_ADD_KEYBINDING_TEXT, NULL, _keybind_cb_add_keybinding,
								    cfdata, NULL);
	cfdata->gui.btn_add_keybind = btn;
	e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 140, 28);
	e_widget_table_object_append(oft1, btn, 1, 1, 1, 1, 1, 1, 1, 1);

	btn = e_widget_button_add(evas, BTN_DEL_KEYBINDING_TEXT, NULL, _keybind_cb_del_keybinding,
								 cfdata, NULL);
	cfdata->gui.btn_del_keybind = btn;
	e_widget_disabled_set(btn, 1);
	//edje_object_size_min_calc(btn, &mw, &mh);
	e_widget_min_size_set(btn, 140, 28);
	e_widget_table_object_append(oft1, btn, 2, 1, 1, 1, 1, 1, 1, 1);
      }
      e_widget_framelist_object_append(of2, oft1);
    }
    e_widget_table_object_append(oft, of2, 1, 0, 1, 1, 1, 1, 1, 1);
  }
  e_widget_list_object_append(o, oft, 1, 1, 0.5);
  e_dialog_resizable_set(cfd->dia, 0);
  return o;
}

static void _update_context_radios(E_Config_Dialog_Data *cfdata)
{
  E_Widget_Radio_Data	*wd;
  E_Config_Binding_Key	*bk;

  if (cfdata == NULL) return;

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY]);
  edje_object_signal_emit(wd->o_radio, "toggle_off", "");

  if (cfdata->cur_eckb == NULL) return;
  if (cfdata->cur_eckb->bk_list == NULL) return;

  if ((bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel)) == NULL )
    return;

  if (bk->context == E_BINDING_CONTEXT_NONE)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_NONE;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_NONE]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_UNKNOWN)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_UNKNOWN;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_UNKNOWN]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_BORDER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_BORDER;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_BORDER]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_ZONE)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_ZONE;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_ZONE]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_CONTAINER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_CONTAINER;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_CONTAINER]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_MANAGER)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_MANAGER;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_MANAGER]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_MENU)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_MENU;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_MENU]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_WINLIST)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_WINLIST;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_WINLIST]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_POPUP)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_POPUP;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_POPUP]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }

  if (bk->context == E_BINDING_CONTEXT_ANY)
    {
      cfdata->binding_context = E_BINDING_CONTEXT_ANY;
      wd = e_widget_data_get(cfdata->gui.bind_context[E_BINDING_CONTEXT_ANY]);
      edje_object_signal_emit(wd->o_radio, "toggle_on", "");
    }
}
static void _update_modifiers_checkboxs(E_Config_Dialog_Data *cfdata)
{
  E_Widget_Checkbox_Data  *wd;
  E_Config_Binding_Key	*bk;

  if (cfdata == NULL) return;

  wd = e_widget_data_get(cfdata->gui.bind_mod_obj.ctrl);
  edje_object_signal_emit(wd->o_check, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_mod_obj.alt);
  edje_object_signal_emit(wd->o_check, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_mod_obj.shift);
  edje_object_signal_emit(wd->o_check, "toggle_off", "");

  wd = e_widget_data_get(cfdata->gui.bind_mod_obj.win);
  edje_object_signal_emit(wd->o_check, "toggle_off", "");

  if (cfdata->cur_eckb == NULL) return;
  if (cfdata->cur_eckb->bk_list == NULL) return;

  if ((bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel)) == NULL )
    return;

  cfdata->bind_mod.ctrl = 0;
  cfdata->bind_mod.alt = 0;
  cfdata->bind_mod.shift = 0;
  cfdata->bind_mod.win = 0;

  if (bk->modifiers & E_BINDING_MODIFIER_CTRL)
    {
      cfdata->bind_mod.ctrl = 1;
      wd = e_widget_data_get(cfdata->gui.bind_mod_obj.ctrl);
      edje_object_signal_emit(wd->o_check, "toggle_on", "");
    }

  if (bk->modifiers & E_BINDING_MODIFIER_ALT)
    {
      cfdata->bind_mod.alt = 1;
      wd = e_widget_data_get(cfdata->gui.bind_mod_obj.alt);
      edje_object_signal_emit(wd->o_check, "toggle_on", "");
    }

  if (bk->modifiers & E_BINDING_MODIFIER_SHIFT)
    {
      cfdata->bind_mod.shift = 1;
      wd = e_widget_data_get(cfdata->gui.bind_mod_obj.shift);
      edje_object_signal_emit(wd->o_check, "toggle_on", "");
    }

  if (bk->modifiers & E_BINDING_MODIFIER_WIN)
    {
      cfdata->bind_mod.win = 1;
      wd = e_widget_data_get(cfdata->gui.bind_mod_obj.win);
      edje_object_signal_emit(wd->o_check, "toggle_on", "");
    }
}

static void _update_key_binding_entry(E_Config_Dialog_Data *cfdata)
{
  E_Widget_Entry_Data *wd;
  E_Config_Binding_Key *bk;

  if( cfdata == NULL ) return;

  wd = e_widget_data_get(cfdata->gui.key_bind);
  e_entry_text_set(wd->o_entry, "");


  if (cfdata->cur_eckb == NULL)return;
  if (cfdata->cur_eckb->bk_list == NULL) return;

  if ((bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel)) == NULL )
    return;

  if (cfdata->key_bind != NULL)
    E_FREE(cfdata->key_bind);
  cfdata->key_bind = bk->key == NULL ? NULL : strdup(bk->key);

  if (bk->key)
    {
      wd = e_widget_data_get(cfdata->gui.key_bind);
      e_entry_text_set(wd->o_entry, bk->key);
    }
}

static void _update_action_param_entries(E_Config_Dialog_Data *cfdata)
{
  E_Widget_Entry_Data *wd;
  E_Config_Binding_Key	*bk;

  if (cfdata == NULL) return;

  wd = e_widget_data_get(cfdata->gui.key_action);
  e_entry_text_set(wd->o_entry, "");

  wd = e_widget_data_get(cfdata->gui.key_params);
  e_entry_text_set(wd->o_entry, "");

  e_widget_disabled_set(cfdata->gui.key_action, 1);
  e_widget_disabled_set(cfdata->gui.key_params, 1);

  if (cfdata->cur_eckb == NULL) return;
  if (cfdata->cur_eckb->bk_list == NULL) return;

  if ((bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel)) == NULL)
    return;

  if (cfdata->key_action != NULL)
    E_FREE(cfdata->key_action);
  cfdata->key_action = bk->action == NULL ? NULL : strdup(bk->action);

  if (bk->action)
    {

      wd = e_widget_data_get(cfdata->gui.key_action);
      e_entry_text_set(wd->o_entry, bk->action);
    }

  if (cfdata->key_params != NULL)
    E_FREE(cfdata->key_params);
  cfdata->key_params = bk->params == NULL ? NULL : strdup(bk->params);

  if (bk->params)
    {

      wd = e_widget_data_get(cfdata->gui.key_params);
      e_entry_text_set(wd->o_entry, bk->params);
    }

  if (!(actions_predefined_names[cfdata->cur_eckb->acn].restrictions & EDIT_RESTRICT_ACTION))
    e_widget_disabled_set(cfdata->gui.key_action, 0);

  if (!(actions_predefined_names[cfdata->cur_eckb->acn].restrictions & EDIT_RESTRICT_PARAMS))
    e_widget_disabled_set(cfdata->gui.key_params, 0);
}

static void
_update_next_prev_add_del_buttons(E_Config_Dialog_Data *cfdata)
{
  char buf[ 50 ];
  E_Widget_Button_Data	*wd_next;
  E_Widget_Button_Data	*wd_prev;
  E_Widget_Button_Data	*wd_add;
  E_Widget_Button_Data	*wd_del;
  E_Config_Binding_Key	*bk;

  if (cfdata == NULL) return;

  wd_next = e_widget_data_get(cfdata->gui.btn_next_keybind);
  wd_prev = e_widget_data_get(cfdata->gui.btn_prev_keybind);
  wd_add  = e_widget_data_get(cfdata->gui.btn_add_keybind);
  wd_del  = e_widget_data_get(cfdata->gui.btn_del_keybind);

  if (evas_list_count(cfdata->cur_eckb->bk_list) > 1)
    {
      if (cfdata->cur_eckb_kb_sel >= (evas_list_count(cfdata->cur_eckb->bk_list) - 1))
	{
	  e_widget_disabled_set(cfdata->gui.btn_next_keybind, 1);
	  sprintf(buf, "%s (%d)", BTN_NEXT_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel);
	  edje_object_part_text_set(wd_next->o_button, "label", buf);

	  e_widget_disabled_set(cfdata->gui.btn_prev_keybind, 0);
	  sprintf(buf, "%s (%d)", BTN_PREV_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel - 1);
	  edje_object_part_text_set(wd_prev->o_button, "label", buf);
	}
      else if (cfdata->cur_eckb_kb_sel <= 0)
	{
	  e_widget_disabled_set(cfdata->gui.btn_next_keybind, 0);
	  sprintf(buf, "%s (%d)", BTN_NEXT_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel + 1);
	  edje_object_part_text_set(wd_next->o_button, "label", buf);

	  e_widget_disabled_set(cfdata->gui.btn_prev_keybind, 1);
	  sprintf(buf, "%s (%d)", BTN_PREV_KEYBINDING_TEXT, 0);
	  edje_object_part_text_set(wd_prev->o_button, "label", buf);
	}
      else
	{
	  e_widget_disabled_set(cfdata->gui.btn_next_keybind, 0);
	  sprintf(buf, "%s (%d)", BTN_NEXT_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel + 1);
	  edje_object_part_text_set(wd_next->o_button, "label", buf);

	  e_widget_disabled_set(cfdata->gui.btn_prev_keybind, 0);
	  sprintf(buf, "%s (%d)", BTN_PREV_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel - 1);
	  edje_object_part_text_set(wd_prev->o_button, "label", buf);
	}
    }
  else
    {
      e_widget_disabled_set(cfdata->gui.btn_next_keybind, 1);
      sprintf(buf, "%s (%d)", BTN_NEXT_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel);
      edje_object_part_text_set(wd_next->o_button, "label", buf);

      e_widget_disabled_set(cfdata->gui.btn_prev_keybind, 1);
      sprintf(buf, "%s (%d)", BTN_PREV_KEYBINDING_TEXT, 0);
      edje_object_part_text_set(wd_prev->o_button, "label", buf);
    }

  if (cfdata->key_bind == NULL || strlen(cfdata->key_bind) == 0)
    {
      e_widget_disabled_set(cfdata->gui.btn_add_keybind, 1);
      sprintf(buf, "%s (%d)", BTN_ADD_KEYBINDING_TEXT,
			      evas_list_count(cfdata->cur_eckb->bk_list)-1);
      edje_object_part_text_set(wd_add->o_button, "label", buf);
    }
  else
    {
      e_widget_disabled_set(cfdata->gui.btn_add_keybind, 0);
      sprintf(buf, "%s (%d)", BTN_ADD_KEYBINDING_TEXT, evas_list_count(cfdata->cur_eckb->bk_list));
      edje_object_part_text_set(wd_add->o_button, "label", buf);
    }


  if (evas_list_count(cfdata->cur_eckb->bk_list) == 1)
    {
      if (cfdata->key_bind == NULL || strlen(cfdata->key_bind) == 0)
	{
	  e_widget_disabled_set(cfdata->gui.btn_del_keybind, 1);
	}
      else
	{
	  e_widget_disabled_set(cfdata->gui.btn_del_keybind, 0);
	}
	sprintf(buf, "%s (%d)", BTN_DEL_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel);
	edje_object_part_text_set(wd_del->o_button, "label", buf);
    }
    else
      {
	e_widget_disabled_set(cfdata->gui.btn_del_keybind, 0);
	sprintf(buf, "%s (%d)", BTN_DEL_KEYBINDING_TEXT, cfdata->cur_eckb_kb_sel);
	edje_object_part_text_set(wd_del->o_button, "label", buf);
      }
}

static void
_update_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata)
{
  int sel_indx;
  E_Smart_Item *si;
  E_Smart_Data *sd;
  E_Widget_IList_Data *wd;
  Evas_Object *obj;

  if (!cfdata) return;
  if (!cfdata->cur_eckb) return;

  wd = e_widget_data_get(cfdata->gui.ilist);

  obj = wd->o_ilist;
  
  sd = evas_object_smart_data_get(obj);
  if ((!obj) || (!sd) || 
      (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), "e_ilist")))
    return;

  si = evas_list_nth(sd->items, sd->selected);
  if (si)
    {
      edje_object_part_unswallow(si->base_obj, si->icon_obj);
      evas_object_hide(si->icon_obj);
      if (evas_list_count(cfdata->cur_eckb->bk_list) > 1)
	{
	  if (si->icon_obj == NULL) si->icon_obj = edje_object_add(cfdata->evas);
	  e_util_edje_icon_set(si->icon_obj, ILIST_ICON_WITH_KEYBIND);
	}
      else if (evas_list_count(cfdata->cur_eckb->bk_list) == 1)
	{
	  if (cfdata->key_bind && strlen(cfdata->key_bind) > 0)
	    {
	      if (si->icon_obj == NULL) si->icon_obj = edje_object_add(cfdata->evas);
	      e_util_edje_icon_set(si->icon_obj, ILIST_ICON_WITH_KEYBIND);
	    }
	  else
	    si->icon_obj = NULL;
	    //e_util_edje_icon_set(si->icon_obj, ILIST_ICON_WITHOUT_KEYBIND);
	}
      else
	si->icon_obj = NULL;
	//e_util_edje_icon_set(si->icon_obj, ILIST_ICON_WITHOUT_KEYBIND);
      
      if (si->icon_obj)
	{
	  edje_extern_object_min_size_set(si->icon_obj, sd->icon_w, sd->icon_h);
	  edje_object_part_swallow(si->base_obj, "icon_swallow", si->icon_obj);
	  evas_object_show(si->icon_obj);
	}
    }
  e_widget_ilist_go(cfdata->gui.ilist);
}

static void
_ilist_kb_cb_change(void *data, Evas_Object *obj)
{
  char buf[256];

  int acn, i;
  char *label;
  E_Config_Dialog_Data	*cfdata;
  E_Config_KeyBind  *eckb = NULL;
  E_Config_Binding_Key	*bk;

  cfdata = data;

  if (cfdata->cur_eckb)
    if (_keybind_cb_auto_apply(cfdata) != 0)
      {
	//TODO: message box which should ask if we really should proceed.
	//If yes, then the current 'empty' binding will be deleted
	_keybind_delete_keybinding(cfdata);
      }

  label = strdup(e_widget_ilist_selected_label_get(obj));
  for (acn = 0; strcasecmp(label, actions_predefined_names[acn].action_name == NULL ? "" :
				 actions_predefined_names[acn].action_name) != 0; acn++ );

  for (i = 0; evas_list_count(cfdata->key_bindings); i++)
    {
      eckb = evas_list_nth(cfdata->key_bindings, i);
      if (eckb->acn == acn)
	break;
      eckb = NULL;
    }
  cfdata->cur_eckb = eckb;
  cfdata->cur_eckb_kb_sel = 0;

  if (!cfdata->cur_eckb->bk_list || evas_list_count(cfdata->cur_eckb->bk_list) == 0)
    {
      bk = E_NEW(E_Config_Binding_Key, 1);

      bk->key = NULL;
      bk->modifiers = E_BINDING_MODIFIER_NONE;
      bk->any_mod = 0;
      bk->action = actions_predefined_names[acn].action_cmd == NULL ? NULL :
					       strdup(actions_predefined_names[acn].action_cmd);
      bk->params = actions_predefined_names[acn].action_params == NULL ? NULL :
					       strdup(actions_predefined_names[acn].action_params);

      cfdata->cur_eckb->bk_list = evas_list_append(cfdata->cur_eckb->bk_list, bk);
    }

  if (cfdata->cur_eckb)
    {
      _update_context_radios(cfdata);
      _update_modifiers_checkboxs(cfdata);
      _update_key_binding_entry(cfdata);
      _update_action_param_entries(cfdata);
      _update_next_prev_add_del_buttons(cfdata);
    }
}


static void
_keybind_cb_next_keybind(void *data, void *data2)
{
  E_Config_Dialog_Data *cfdata;
  int old_kb_sel;

  cfdata = data;

  if (cfdata == NULL) return;
  if (cfdata->cur_eckb == NULL) return;

  old_kb_sel = cfdata->cur_eckb_kb_sel;

  if (_keybind_cb_auto_apply(cfdata) != 0)
    {
      //TODO: message box which should ask if we really should proceed.
      //If yes, then the current 'empty' binding will be deleted
      //_keybind_cb_del_keybinding(cfdata, NULL);
      _keybind_delete_keybinding(cfdata);

      if (old_kb_sel == 0)
	;
      else
	cfdata->cur_eckb_kb_sel ++;
    }
  else
    cfdata->cur_eckb_kb_sel ++;

  if (cfdata->cur_eckb_kb_sel >= evas_list_count(cfdata->cur_eckb->bk_list))
    cfdata->cur_eckb_kb_sel = evas_list_count(cfdata->cur_eckb->bk_list) - 1;

  _update_context_radios(cfdata);
  _update_modifiers_checkboxs(cfdata);
  _update_key_binding_entry(cfdata);
  _update_action_param_entries(cfdata);
  _update_next_prev_add_del_buttons(cfdata);
}

static void
_keybind_cb_prev_keybind(void *data, void *data2)
{
  char buf[50];
  E_Config_Dialog_Data *cfdata;
  E_Widget_Button_Data *wd_next, *wd_prev;

  int old_kb_sel;
  int old_bk_list_size;

  cfdata = data;

  if (cfdata == NULL) return;
  if (cfdata->cur_eckb == NULL) return;

  old_kb_sel = cfdata->cur_eckb_kb_sel;
  old_bk_list_size = evas_list_count(cfdata->cur_eckb->bk_list);

  if (_keybind_cb_auto_apply(cfdata) != 0)
    {
      //TODO: message box which should ask if we really should proceed.
      //If yes, then the current 'empty' binding will be deleted
      //_keybind_cb_del_keybinding(cfdata, NULL);
      _keybind_delete_keybinding(cfdata);

      if (old_kb_sel == old_bk_list_size - 1)
	;
      else
	cfdata->cur_eckb_kb_sel --;
    }
  else
    cfdata->cur_eckb_kb_sel --;

  if (cfdata->cur_eckb_kb_sel < 0)
    cfdata->cur_eckb_kb_sel = 0;

  _update_context_radios(cfdata);
  _update_modifiers_checkboxs(cfdata);
  _update_key_binding_entry(cfdata);
  _update_action_param_entries(cfdata);
  _update_next_prev_add_del_buttons(cfdata);
}

static void
_keybind_cb_add_keybinding(void *data, void *data2)
{
  E_Config_Dialog_Data	*cfdata;
  E_Config_Binding_Key	*bk;

  cfdata = data;

  if (cfdata == NULL) return;

  if (_keybind_cb_auto_apply(cfdata) != 0)
    {
      //TODO: message box, that a keybinding cannot be added
      //until the current is assigned.
      return;
    }

  bk = E_NEW(E_Config_Binding_Key, 1);
  if (!bk)
    return;

  bk->context = E_BINDING_CONTEXT_ANY;
  bk->key = NULL;
  bk->modifiers = E_BINDING_MODIFIER_NONE;
  bk->action = actions_predefined_names[cfdata->cur_eckb->acn].action_cmd == NULL ? NULL :
    strdup(actions_predefined_names[cfdata->cur_eckb->acn].action_cmd);
  bk->params = actions_predefined_names[cfdata->cur_eckb->acn].action_params == NULL ? NULL :
    strdup(actions_predefined_names[cfdata->cur_eckb->acn].action_params);

  cfdata->cur_eckb->bk_list = evas_list_append(cfdata->cur_eckb->bk_list, bk);
  cfdata->cur_eckb_kb_sel = evas_list_count(cfdata->cur_eckb->bk_list) - 1;

  _update_context_radios(cfdata);
  _update_modifiers_checkboxs(cfdata);
  _update_key_binding_entry(cfdata);
  _update_action_param_entries(cfdata);
  _update_next_prev_add_del_buttons(cfdata);
}


static void
_keybind_cb_del_keybinding(void *data, void *data2)
{
  int i;
  E_Config_Dialog_Data	*cfdata;
  E_Config_Binding_Key	*bk, *bk_del;
  Evas_List *new_bk_list;

  cfdata = data;

  if (cfdata == NULL) return;
  if (cfdata->cur_eckb == NULL)return;

  _keybind_delete_keybinding(cfdata);

  _update_context_radios(cfdata);
  _update_modifiers_checkboxs(cfdata);
  _update_key_binding_entry(cfdata);
  _update_action_param_entries(cfdata);
  _update_next_prev_add_del_buttons(cfdata);
  _update_ilist_cur_selection_icon(cfdata);
}

static void
_entry_keybind_cb_text_change(void *data, Evas_Object *obj)
{
  E_Config_Dialog_Data *cfdata = data;
  _update_ilist_cur_selection_icon(cfdata);
  _update_next_prev_add_del_buttons(cfdata);
}

static int 
_keybind_cb_auto_apply(E_Config_Dialog_Data *cfdata)
{
  E_Config_Binding_Key	*bk;
  if (!cfdata || !cfdata->cur_eckb)
    return -1;

  if (cfdata->key_bind == NULL || strlen(cfdata->key_bind) == 0)
    return -1;

  bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
  if (bk == NULL)
    return -1;

  bk->context = cfdata->binding_context;
  if (bk->key) E_FREE(bk->key);
  bk->key = strdup(cfdata->key_bind);

  bk->modifiers = 0;
  if (cfdata->bind_mod.shift)
    bk->modifiers |= E_BINDING_MODIFIER_SHIFT;
  if (cfdata->bind_mod.ctrl)
    bk->modifiers |= E_BINDING_MODIFIER_CTRL;
  if (cfdata->bind_mod.alt)
    bk->modifiers |= E_BINDING_MODIFIER_ALT;
  if (cfdata->bind_mod.win)
    bk->modifiers |= E_BINDING_MODIFIER_WIN;

  bk->any_mod = 0;
  if (bk->action) E_FREE(bk->action);
  bk->action = (cfdata->key_action == NULL || strlen(cfdata->key_action) == 0) ? NULL :
	       strdup(cfdata->key_action);
  if (bk->params) E_FREE(bk->params);
  bk->params = (cfdata->key_params == NULL || strlen(cfdata->key_params) == 0) ? NULL :
	       strdup(cfdata->key_params);

  return 0;
}

/* deletes an entry e from kb_list and returns a new list
 * without that entry.
 * On error returns NULL
 */
static int
_keybind_delete_keybinding(E_Config_Dialog_Data *cfdata)
{
  int i;
  unsigned int kb_list_size;
  E_Config_Binding_Key *bk_del;
  Evas_List  *new_kb_list;
  if (!cfdata || !cfdata->cur_eckb || !cfdata->cur_eckb->bk_list)
    return -1;

  new_kb_list = NULL;

  kb_list_size = evas_list_count(cfdata->cur_eckb->bk_list);

  if (kb_list_size > 1)
    {
      bk_del = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
      if (!bk_del)return -1;

      for (i = 0; i < kb_list_size; i++)
	{
	  if (i != cfdata->cur_eckb_kb_sel)
	    new_kb_list = evas_list_append(new_kb_list,
					   evas_list_nth(cfdata->cur_eckb->bk_list, i));
	}

      if (bk_del->key != NULL)
	E_FREE(bk_del->key);
      if (bk_del->action != NULL)
	E_FREE(bk_del->action);
      if (bk_del->params != NULL)
	E_FREE(bk_del->params);
      bk_del->key = bk_del->action = bk_del->params = NULL;
      E_FREE(bk_del);

      evas_list_free(cfdata->cur_eckb->bk_list);
      cfdata->cur_eckb->bk_list = new_kb_list;
    }
  else if (kb_list_size == 1)
    {
      bk_del = evas_list_nth(cfdata->cur_eckb->bk_list, 0);
      if (!bk_del) return -1;
      bk_del->context = E_BINDING_CONTEXT_NONE;
      if (bk_del->key != NULL)
	E_FREE(bk_del->key);
      bk_del->key = NULL;
      bk_del->modifiers = E_BINDING_MODIFIER_NONE;
      bk_del->any_mod = 0;
    }

  if (cfdata->cur_eckb_kb_sel >= evas_list_count(cfdata->cur_eckb->bk_list))
    cfdata->cur_eckb_kb_sel = evas_list_count(cfdata->cur_eckb->bk_list) - 1;

  return 0;
}

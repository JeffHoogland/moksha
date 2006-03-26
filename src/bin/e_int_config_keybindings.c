#include "e.h"

/* FIXME: this dialog needs to keep up with the actions in actions.c - this
 * isnt' easily "automatic - should probably add this info to actions.c so
 *  this can list an existing set of actions and extra ones are added from
 * a list automatically retrieved from actions.c
 */

#define ACTION_LIST_ICON_W  24
#define ACTION_LIST_ICON_H  24

#define BINDING_LIST_ICON_W 16
#define BINDING_LIST_ICON_H 16

#define BTN_ASSIGN_KEYBINDING_TEXT _("Assign Key Binding...")

#define TEXT_ACTION _("Action")
#define TEXT_NONE_ACTION_KEY _("<None>")
#define TEXT_PRESS_KEY_SEQUENCE _("Please press key sequence,<br>or <hilight>Escape"\
				  "</hilight> to abort")

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


/*******************************************************************************************/
static void	    _e_keybinding_action_ilist_cb_change(void *data, Evas_Object *obj);
static void	    _e_keybinding_binding_ilist_cb_change(void *data, Evas_Object *obj);
static void	    _e_keybinding_default_keybinding_settings(E_Config_Dialog_Data *cfdata);

static void	    _e_keybinding_keybind_cb_del_keybinding(void *data, void *data2);
static int	    _e_keybinding_keybind_delete_keybinding(E_Config_Dialog_Data *cfdata);

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

static void	    _e_keybinding_shortcut_wnd_hide(E_Config_Dialog_Data *cfdata);
static int	    _e_keybinding_cb_shortcut_key_down(void *data, int type, void *event);
static int	    _e_keybinding_cb_mouse_handler_dumb(void *data, int type, void *event);

static int	    _e_keybinding_keybind_cb_auto_apply(E_Config_Dialog_Data *cfdata);
/*******************************************************************************************/

typedef struct _E_Config_KeyBind	  E_Config_KeyBind;
typedef struct _E_Widget_IList_Data	  E_Widget_IList_Data;
typedef struct _E_Widget_Radio_Data	  E_Widget_Radio_Data;
typedef struct _E_Widget_Button_Data	  E_Widget_Button_Data;
typedef struct _E_Widget_Entry_Data	  E_Widget_Entry_Data;

typedef struct _E_Smart_Item		  E_Smart_Item;
typedef struct _E_Smart_Data		  E_Smart_Data;

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
  {"Flip Desktop Linearly...", "desk_linear_flip_by", NULL, _NONDEFAULT_ACTION,
							    EDIT_RESTRICT_ACTION },
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
  {"Switch To Desktop...", "desk_linear_flip_to", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Run Command", "exebuf", NULL, _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Exit Enlightenment", "exit", NULL,
				 _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Restart Enlightenment", "restart", NULL,
				    _DEFAULT_ACTION, EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },

  {"Show Main Menu", "menu_show", "main", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Show Favorites Menu", "menu_show", "favorites", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Show Clients Menu", "menu_show", "clients", _DEFAULT_ACTION,
					  EDIT_RESTRICT_ACTION | EDIT_RESTRICT_PARAMS },
  {"Show Menu...", "menu_show", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

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
  {"Window Move By Desktop...", "window_desk_move_by", NULL, _DEFAULT_ACTION,
							     EDIT_RESTRICT_ACTION },
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
  {"Window Move To Desktop...", "window_desk_move_to", NULL, _NONDEFAULT_ACTION,
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

  {"Window Move...", "window_move", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Move To...", "window_move_to", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Move By...", "window_move_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Resize To...", "window_resize", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Resize By...", "window_resize_by", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"Window Drag Icon", "window_drag_icon", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },

  {"Application", "app", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
  {"User Defined Actions...", "exec", NULL, _NONDEFAULT_ACTION, EDIT_RESTRICT_ACTION },
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
  E_Config_Dialog *cfd;
  Evas_List *key_bindings;

  int cur_eckb_kb_sel;
  E_Config_KeyBind  *cur_eckb;
  Evas	*evas;

  int binding_context;
  char *key_bind;
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
    } gui;

  struct {
    Ecore_X_Window  keybind_win;
    Evas_List	    *handlers;
    E_Dialog	    *dia;
  }locals;

  int changed;
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
  //v->override_auto_apply = 1;

  cfd = e_config_dialog_new(con, _("Key Binding Settings"), NULL, 0, v, NULL);
  return cfd;
}

static void
_fill_keybindings_data(E_Config_Dialog_Data *cfdata)
{
  int i, j;
  E_Config_KeyBind  *eckb;
  E_Config_Binding_Key	*bk, *t;

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
		  bk->key = evas_stringshare_add(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? 
		     NULL : evas_stringshare_add(t->action);
		  bk->params = t->params == NULL ? 
		     NULL : evas_stringshare_add(t->params);

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
		  bk->key = evas_stringshare_add(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? 
		     NULL : evas_stringshare_add(t->action);
		  bk->params = t->params == NULL ? 
		     NULL : evas_stringshare_add(t->params);

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
		  bk->key = evas_stringshare_add(t->key);
		  bk->modifiers = t->modifiers;
		  bk->any_mod = t->any_mod;
		  bk->action = t->action == NULL ? 
		     NULL : evas_stringshare_add(t->action);
		  bk->params = t->params == NULL ? 
		     NULL : evas_stringshare_add(t->params);

		  eckb->bk_list = evas_list_append(eckb->bk_list, bk);
		  break;
		}
	    }// if (!bk ...)
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
  cfdata->key_bind = strdup("");
  cfdata->key_action = strdup("");
  cfdata->key_params = strdup("");

  _fill_keybindings_data(cfdata);
  cfdata->cfd = cfd;

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
		  if (bk->key) evas_stringshare_del(bk->key);
		  if (bk->action) evas_stringshare_del(bk->action);
		  if (bk->params) evas_stringshare_del(bk->params);
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
    if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 0)
      {
	//TODO: message box which should ask if we really should proceed.
	//If yes, then the current 'empty' binding will be deleted
	//_keybind_delete_keybinding(cfdata);
      }

  // here the removing of the old keybindings goes
  e_managers_keys_ungrab();
  while (e_config->key_bindings)
    {
       E_Config_Binding_Key *eb;
       
       eb = e_config->key_bindings->data;

       e_bindings_key_del(eb->context, eb->key, eb->modifiers, eb->any_mod,
			  eb->action, eb->params);

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
		      bk2->key = evas_stringshare_add(bk->key);
		      bk2->modifiers = bk->modifiers;
		      bk2->any_mod = bk->any_mod;
		      bk2->action = bk->action == NULL ? NULL : 
							 (char *)evas_stringshare_add(bk->action);
		      bk2->params = bk->params == NULL ? NULL : 
							 (char *)evas_stringshare_add(bk->params);
		      e_config->key_bindings = evas_list_append(e_config->key_bindings, bk2);
		      e_bindings_key_add(bk->context, bk->key, bk->modifiers, bk->any_mod,
					 bk->action, bk->params);
		    }
		}
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
  int i;
  Evas_Object *ot;
  Evas_Object *ob, *of;
  E_Radio_Group *rg;
  int mw, mh;

  cfdata->evas = evas;
  //_fill_keybindings_data(cfdata);

  ot = e_widget_table_add(evas, 0);
  {
    of = e_widget_framelist_add(evas, _("Action"), 0);
    {
      cfdata->gui.action_ilist = e_widget_ilist_add(evas, ACTION_LIST_ICON_W,
						    ACTION_LIST_ICON_H, NULL); 
      {
	Evas_List *l;
	e_widget_on_change_hook_set(cfdata->gui.action_ilist, _e_keybinding_action_ilist_cb_change,
				    cfdata);

	for (l = cfdata->key_bindings; l; l = l->next)
	  {
	    Evas_Object *ic;
	    E_Config_KeyBind *eckb;

	    eckb = l->data;
	    ic = edje_object_add(evas);
	    if (eckb && eckb->bk_list)
	      e_util_edje_icon_set(ic, ILIST_ICON_WITH_KEYBIND);
	    else
	      e_util_edje_icon_set(ic, ILIST_ICON_WITHOUT_KEYBIND);

	    e_widget_ilist_append(cfdata->gui.action_ilist, ic,
				  _(actions_predefined_names[eckb->acn].action_name),
				  NULL, NULL, NULL);
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
      e_widget_min_size_set(cfdata->gui.binding_ilist, 250, 100);
      e_widget_ilist_go(cfdata->gui.binding_ilist);
      e_widget_framelist_object_append(of, cfdata->gui.binding_ilist);
      /****************/

      ot1 = e_widget_table_add(evas, 0);
      {
	/* add keybinding button */
	cfdata->gui.btn_add = e_widget_button_add(evas, _("Add Key Binding"), NULL,
						  _e_keybinding_keybind_cb_add_keybinding, cfdata,
						  NULL);
	e_widget_disabled_set(cfdata->gui.btn_add, 1);
	e_widget_min_size_set(cfdata->gui.btn_add, 140, 28);
	e_widget_table_object_append(ot1, cfdata->gui.btn_add, 0, 0, 1, 1, 1, 1, 1, 1);
	/****************/

	/* delete keybinding button */
	cfdata->gui.btn_del = e_widget_button_add(evas, _("Delete Key Binding"), NULL,
						  _e_keybinding_keybind_cb_del_keybinding, cfdata,
						  NULL);
	e_widget_disabled_set(cfdata->gui.btn_del, 1);
	e_widget_min_size_set(cfdata->gui.btn_del, 140, 28);
	e_widget_table_object_append(ot1, cfdata->gui.btn_del, 1, 0, 1, 1, 1, 1, 1, 1);
	/****************/
      }
      e_widget_framelist_object_append(of, ot1);

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

      /* key action */
      ot1 = e_widget_frametable_add(evas, _("Key Binding"), 0);
      {
	ob = e_widget_label_add(evas, _("Key Binding"));
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

	ob = e_widget_label_add(evas, _("Params"));
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
  int indx;
  E_Config_Dialog_Data *cfdata;
  E_Config_Binding_Key *bk;

  cfdata = data;
  if (!cfdata) return;

  if (cfdata->cur_eckb)
    if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 0)
      {
	//TODO: message box which should ask if we really should proceed.
	//If yes, then the current 'empty' binding will be deleted
	//_keybind_delete_keybinding(cfdata);
      }

  indx = e_widget_ilist_selected_get(cfdata->gui.binding_ilist);
  if (indx < 0 || indx >= e_widget_ilist_count(cfdata->gui.binding_ilist))
  {
    return;
  }

  cfdata->cur_eckb_kb_sel = indx;

  if (cfdata->cur_eckb)
  {
    _e_keybinding_update_context_radios(cfdata);
    _e_keybinding_update_action_param_entries(cfdata);
    _e_keybinding_update_keybinding_button(cfdata);
    _e_keybinding_update_add_delete_buttons(cfdata);
  }
}
static void
_e_keybinding_action_ilist_cb_change(void *data, Evas_Object *obj)
{
  int acn;
  E_Config_Dialog_Data *cfdata;
  E_Config_KeyBind     *eckb;
  Evas_List *l;
  char *label;

  cfdata = data;

  if (!cfdata) return;


  if (cfdata->cur_eckb)
    if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 0)
      {
	//TODO: message box which should ask if we really should proceed.
	//If yes, then the current 'empty' binding will be deleted
	//_keybind_delete_keybinding(cfdata);
      }
  _e_keybinding_default_keybinding_settings(cfdata);

  label = strdup(e_widget_ilist_selected_label_get(obj));
  for (acn = 0; strcasecmp(label, _(actions_predefined_names[acn].action_name) == NULL ? "" :
			   _(actions_predefined_names[acn].action_name)) != 0; acn++ );

  for (l = cfdata->key_bindings; l; l = l->next)
    {
      eckb = l->data;
      if (eckb->acn == acn)
	break;
      eckb = NULL;
    }

  cfdata->cur_eckb = eckb;
  if (evas_list_count(cfdata->cur_eckb->bk_list))
    cfdata->cur_eckb_kb_sel = 0;

  _e_keybinding_update_binding_list(cfdata);
  _e_keybinding_update_add_delete_buttons(cfdata);

  /*if (cfdata->changed == 0)
    {*/
      //e_dialog_button_disable_num_set(cfdata->cfd->dia, 0, 1);
      //e_dialog_button_disable_num_set(cfdata->cfd->dia, 1, 1);
    //}
}

static void
_e_keybinding_default_keybinding_settings(E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  cfdata->cur_eckb = NULL;
  cfdata->cur_eckb_kb_sel = -1;
  E_FREE(cfdata->key_bind);
  cfdata->key_bind = strdup("");
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

  if (!cfdata->cur_eckb) return;

  e_widget_disabled_set(cfdata->gui.btn_add, 0);

  if (cfdata->cur_eckb_kb_sel >= 0)
    e_widget_disabled_set(cfdata->gui.btn_del, 0);
}

static void
_e_keybinding_update_keybinding_button(E_Config_Dialog_Data *cfdata)
{
  char *b;
  E_Config_Binding_Key *bk;
  E_Widget_Button_Data	*wd;

  if (cfdata == NULL) return;

  wd = e_widget_data_get(cfdata->gui.btn_keybind);

  if (cfdata->cur_eckb == NULL || cfdata->cur_eckb_kb_sel < 0)
    {
      e_widget_disabled_set(cfdata->gui.btn_keybind, 1);
      edje_object_part_text_set(wd->o_button, "label", BTN_ASSIGN_KEYBINDING_TEXT);

      E_FREE(cfdata->key_bind);
      cfdata->key_bind = strdup("");
    }
  else
    {
      bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
      
      E_FREE(cfdata->key_bind);
      cfdata->key_bind = strdup(bk->key);

      e_widget_disabled_set(cfdata->gui.btn_keybind, 0);
      if (bk && bk->key && bk->key[0])
	{
	  b = _e_keybinding_get_keybinding_text(bk);
	  edje_object_part_text_set(wd->o_button, "label", b);
	  free(b);
	}
      else
	edje_object_part_text_set(wd->o_button, "label", BTN_ASSIGN_KEYBINDING_TEXT);
    }
}


static void
_e_keybinding_update_context_radios(E_Config_Dialog_Data *cfdata)
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

  if (cfdata->cur_eckb_kb_sel < 0) return;

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

static void
_e_keybinding_update_action_param_entries(E_Config_Dialog_Data *cfdata)
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
_e_keybinding_keybind_cb_del_keybinding(void *data, void *data2)
{
  E_Config_Dialog_Data	*cfdata;

  cfdata = data;

  if (cfdata == NULL) return;
  if (cfdata->cur_eckb == NULL || cfdata->cur_eckb_kb_sel < 0)return;

  if (_e_keybinding_keybind_delete_keybinding(cfdata) != 0)
    return;

  _e_keybinding_update_binding_list(cfdata);
  e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->cur_eckb_kb_sel);
  e_widget_ilist_go(cfdata->gui.binding_ilist);

  _e_keybinding_update_keybinding_button(cfdata);
  _e_keybinding_update_add_delete_buttons(cfdata);
  _e_keybinding_update_context_radios(cfdata);
  _e_keybinding_update_action_param_entries(cfdata);

  // nice iface features //
  _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);
  _e_keybinding_update_binding_ilist_cur_selection_icon(cfdata);

  //cfdata->changed = 1;
}

/* deletes an entry e from kb_list and returns a new list
 * without that entry.
 * On error returns NULL
 */
static int
_e_keybinding_keybind_delete_keybinding(E_Config_Dialog_Data *cfdata)
{
  int i;
  unsigned int kb_list_size;
  E_Config_Binding_Key *bk_del;

  Evas_List  *new_kb_list, *bk_del_list;

  if (!cfdata || !cfdata->cur_eckb || !cfdata->cur_eckb->bk_list ||
      cfdata->cur_eckb_kb_sel < 0)
    return -1;

  new_kb_list = NULL;

  bk_del = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
  cfdata->cur_eckb->bk_list = evas_list_remove(cfdata->cur_eckb->bk_list, bk_del);

  if (!evas_list_count(cfdata->cur_eckb->bk_list))
    {
      evas_list_free(cfdata->cur_eckb->bk_list);
      cfdata->cur_eckb->bk_list = NULL;
    }

  if (bk_del->key) evas_stringshare_del(bk_del->key);
  if (bk_del->action) evas_stringshare_del(bk_del->action);
  if (bk_del->params) evas_stringshare_del(bk_del->params);
  bk_del->key = bk_del->action = bk_del->params = NULL;
  E_FREE(bk_del);

  if (cfdata->cur_eckb_kb_sel >= evas_list_count(cfdata->cur_eckb->bk_list))
    cfdata->cur_eckb_kb_sel = evas_list_count(cfdata->cur_eckb->bk_list) - 1;

  return 0;
}
static void
_e_keybinding_update_binding_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata)
{
  return;
}
static void
_e_keybinding_update_action_ilist_cur_selection_icon(E_Config_Dialog_Data *cfdata)
{
  E_Smart_Item *si;
  E_Smart_Data *sd;
  E_Widget_IList_Data *wd;
  Evas_Object *obj;

  if (!cfdata) return;
  if (!cfdata->cur_eckb) return;

  wd = e_widget_data_get(cfdata->gui.action_ilist);

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
  Evas_List *l;
  char buf[1024];

  if (!cfdata || !cfdata->cur_eckb) return;

  e_widget_ilist_clear(cfdata->gui.binding_ilist);

  for (l = cfdata->cur_eckb->bk_list, i = 0; l; l = l->next)
    {
      char *b;
      E_Config_Binding_Key *bk = l->data;
      if (!bk) continue;

      b = _e_keybinding_get_keybinding_text(bk);
      snprintf(buf, sizeof(buf), "%s %d : %s", TEXT_ACTION, i, b);
      free(b);
      e_widget_ilist_append(cfdata->gui.binding_ilist, NULL, buf, NULL, NULL, NULL);
      i++;
    }

  _e_keybinding_update_keybinding_button(cfdata);
  _e_keybinding_update_add_delete_buttons(cfdata);
  _e_keybinding_update_context_radios(cfdata);
  _e_keybinding_update_action_param_entries(cfdata);

  // nice iface features //
  _e_keybinding_update_action_ilist_cur_selection_icon(cfdata);
  _e_keybinding_update_binding_ilist_cur_selection_icon(cfdata);

  e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->cur_eckb_kb_sel);
  e_widget_ilist_go(cfdata->gui.binding_ilist);
}

static void
_e_keybinding_keybind_cb_add_keybinding(void *data, void *data2)
{
  E_Config_Dialog_Data	*cfdata;
  E_Config_Binding_Key	*bk;

  cfdata = data;

  if (cfdata == NULL) return;

  if (_e_keybinding_keybind_cb_auto_apply(cfdata) != 0)
    {
      //TODO: message box, that a keybinding cannot be added
      //until the current is assigned.
      return;
    }

  bk = E_NEW(E_Config_Binding_Key, 1);
  if (!bk)
    return;

  bk->context = E_BINDING_CONTEXT_ANY;
  bk->key = strdup("");
  bk->modifiers = E_BINDING_MODIFIER_NONE;
  bk->action = actions_predefined_names[cfdata->cur_eckb->acn].action_cmd == NULL ? 
     evas_stringshare_add("") :
     evas_stringshare_add(actions_predefined_names[cfdata->cur_eckb->acn].action_cmd);
  bk->params = actions_predefined_names[cfdata->cur_eckb->acn].action_params == NULL ? 
     evas_stringshare_add("") :
     evas_stringshare_add(actions_predefined_names[cfdata->cur_eckb->acn].action_params);

  cfdata->cur_eckb->bk_list = evas_list_append(cfdata->cur_eckb->bk_list, bk);
  cfdata->cur_eckb_kb_sel = evas_list_count(cfdata->cur_eckb->bk_list) - 1;

  _e_keybinding_update_binding_list(cfdata);
  e_widget_ilist_selected_set(cfdata->gui.binding_ilist, cfdata->cur_eckb_kb_sel);
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
  E_Config_Binding_Key	*bk;
  if (!cfdata || !cfdata->cur_eckb)
    return -1;

  if (cfdata->cur_eckb_kb_sel < 0) return 0;

  bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
  if (bk == NULL)
    return -1;

  bk->context = cfdata->binding_context;
  if (bk->key) evas_stringshare_del(bk->key);
  bk->key = evas_stringshare_add(cfdata->key_bind);

  bk->any_mod = 0;
  if (bk->action) evas_stringshare_del(bk->action);
  bk->action = (cfdata->key_action == NULL || strlen(cfdata->key_action) == 0) ? NULL :
	       evas_stringshare_add(cfdata->key_action);
  if (bk->params) evas_stringshare_del(bk->params);
  bk->params = (cfdata->key_params == NULL || strlen(cfdata->key_params) == 0) ? NULL :
	       evas_stringshare_add(cfdata->key_params);
  return 0;
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
  E_Config_Binding_Key	*bk;
  E_Config_Dialog_Data *cfdata = data;
  Ecore_X_Event_Key_Down  *ev = event;

  if (ev->win != cfdata->locals.keybind_win) return 1;

  if (!strcmp(ev->keysymbol, "Escape") &&
      !(ev->modifiers & ECORE_X_MODIFIER_SHIFT) &&
      !(ev->modifiers & ECORE_X_MODIFIER_CTRL) &&
      !(ev->modifiers & ECORE_X_MODIFIER_ALT) &&
      !(ev->modifiers & ECORE_X_MODIFIER_WIN))
  {
    _e_keybinding_keybind_shortcut_wnd_hide(cfdata);
  }
  else
    {

      if (!strcmp(ev->keysymbol, "Control_L") || !strcmp(ev->keysymbol, "Control_R") ||
	  !strcmp(ev->keysymbol, "Shift_L") || !strcmp(ev->keysymbol, "Shift_R") ||
	  !strcmp(ev->keysymbol, "Alt_L") || !strcmp(ev->keysymbol, "Alt_R") ||
	  !strcmp(ev->keysymbol, "Super_L") || !strcmp(ev->keysymbol, "Super_R"))
	;
      else
	{
	  if (cfdata && cfdata->cur_eckb && cfdata->cur_eckb_kb_sel >= 0 &&
	      cfdata->cur_eckb->bk_list)
	    {
	      Evas_List *l, *l2;

	      E_Config_KeyBind *eckb;
	      E_Config_Binding_Key *bk_tmp;

	      int found = 0;
	      int mod = E_BINDING_MODIFIER_NONE;

	      if (ev->modifiers & ECORE_X_MODIFIER_SHIFT)
		mod |= E_BINDING_MODIFIER_SHIFT;
	      if (ev->modifiers & ECORE_X_MODIFIER_CTRL)
		mod |= E_BINDING_MODIFIER_CTRL;
	      if (ev->modifiers & ECORE_X_MODIFIER_ALT)
		mod |= E_BINDING_MODIFIER_ALT;
	      if (ev->modifiers & ECORE_X_MODIFIER_WIN)
		mod |= E_BINDING_MODIFIER_WIN;

	      for (l = cfdata->key_bindings; l && !found; l = l->next)
		{
		  eckb = l->data;
		  for (l2 = eckb->bk_list; l2 && !found; l2 = l2->next)
		    {
		      bk_tmp = l2->data;

		      if (bk_tmp->modifiers == mod && !strcmp(ev->keysymbol, bk_tmp->key))
			found = 1;
		    }
		}

	      if (!found)
		{
		  bk = evas_list_nth(cfdata->cur_eckb->bk_list, cfdata->cur_eckb_kb_sel);
		  bk->modifiers = E_BINDING_MODIFIER_NONE;

		  bk->modifiers = mod;

		  if (bk->key)
		    evas_stringshare_del(bk->key);
		  bk->key = evas_stringshare_add(ev->keysymbol);

		  _e_keybinding_update_binding_list(cfdata);
		}
	      else
		{
		  e_util_dialog_show(_("Binding Key Error"), 
				     _("The binding key sequence, that you choose,"
					" is already used.<br>Please choose another binding key"
					" sequence."));
		}
	      _e_keybinding_keybind_shortcut_wnd_hide(cfdata);
	    }
	}
    }
  return 1;
}
static int
_e_keybinding_cb_mouse_handler_dumb(void *data, int type, void *event)
{
  return 1;
}

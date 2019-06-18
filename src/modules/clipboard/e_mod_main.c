#include "e_mod_main.h"
#include "x_clipboard.h"
#include "config_defaults.h"
#include "history.h"

EINTERN int _e_clipboard_log_dom = -1;

/* Stuff for convenience to compress code */
#define CLIP_TRIM_MODE(x) (x->trim_nl + 2 * (x->trim_ws))
#define MOUSE_BUTTON ECORE_EVENT_MOUSE_BUTTON_DOWN
typedef Evas_Event_Mouse_Down Mouse_Event;

/* gadcon requirements */
static     Evas_Object  *_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas * evas);
static const char       *_gc_id_new(const E_Gadcon_Client_Class *client_class);
static E_Gadcon_Client  *_gc_init(E_Gadcon * gc, const char *name, const char *id, const char *style);
static void              _gc_orient(E_Gadcon_Client * gcc, E_Gadcon_Orient orient __UNUSED__);
static const char       *_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__);
static void              _gc_shutdown(E_Gadcon_Client * gcc);

/* Define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class = {
   GADCON_CLIENT_CLASS_VERSION,
   "clipboard",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
      e_gadcon_site_is_not_toolbar
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* Set the version and the name IN the code (not just the .desktop file)
 * but more specifically the api version it was compiled for so E can skip
 * modules that are compiled for an incorrect API version safely */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Clipboard"};

/* actual module specifics   */
Config *clip_cfg = NULL;
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
Mod_Inst *clip_inst = NULL; /* Need by e_mod_config.c */
static E_Action *act = NULL;

/*   First some callbacks   */
static Eina_Bool _cb_clipboard_request(void *data __UNUSED__);

static Eina_Bool _cb_event_selection(Instance *instance, int type __UNUSED__, Ecore_X_Event_Selection_Notify * event);
static Eina_Bool _cb_event_owner(Instance *instance __UNUSED__, int type __UNUSED__, Ecore_X_Event_Fixes_Selection_Notify * event);
static void      _cb_menu_item(Clip_Data *selected_clip);
static void      _cb_menu_post_deactivate(void *data, E_Menu *menu __UNUSED__);
static void      _cb_menu_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event);
static void      _cb_context_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event);
static void      _cb_clear_history(Instance *inst);
static void      _cb_dialog_delete(void *data __UNUSED__);
static void      _cb_dialog_keep(void *data __UNUSED__);
static void      _cb_action_switch(E_Object *o __UNUSED__, const char *params, Instance *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event);
static void      _cb_config_show(void *data__UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);

/*   And then some auxillary functions */
static void      _clip_config_new(E_Module *m);
static void      _clip_config_free(void);
static void      _clip_inst_free(Instance *inst);
static void      _clip_add_item(Clip_Data *clip_data);
static int       _menu_fill(Instance *inst, Eina_Bool mouse_event);
static void      _clear_history(void);
static void      _x_clipboard_update(const char *text);
static Eina_List *     _item_in_history(Clip_Data *cd);
static int             _clip_compare(Clip_Data *cd, char *text);
static void  _set_mouse_coord(Instance *inst, Eina_Bool mouse_event,
                              Evas_Coord * const x, Evas_Coord * const y,
                              Evas_Coord * const w, Evas_Coord * const h);

/* new module needs a new config :), or config too old and we need one anyway */
static void
_clip_config_new(E_Module *m)
{
  /* setup defaults */
  if (!clip_cfg) {
    clip_cfg = E_NEW(Config, 1);

    clip_cfg->label_length_changed = EINA_FALSE;

    clip_cfg->clip_copy      = CF_DEFAULT_COPY;
    clip_cfg->clip_select    = CF_DEFAULT_SELECT;
    clip_cfg->sync           = CF_DEFAULT_SYNC;
    clip_cfg->persistence    = CF_DEFAULT_PERSISTANCE;
    clip_cfg->hist_reverse   = CF_DEFAULT_HIST_REVERSE;
    clip_cfg->hist_items     = CF_DEFAULT_HIST_ITEMS;
    clip_cfg->confirm_clear  = CF_DEFAULT_CONFIRM;
    clip_cfg->autosave       = CF_DEFAULT_AUTOSAVE;
    clip_cfg->save_timer     = CF_DEFAULT_SAVE_TIMER;
    clip_cfg->label_length   = CF_DEFAULT_LABEL_LENGTH;
    clip_cfg->ignore_ws      = CF_DEFAULT_IGNORE_WS;
    clip_cfg->ignore_ws_copy = CF_DEFAULT_IGNORE_WS_COPY;
    clip_cfg->trim_ws        = CF_DEFAULT_WS;
    clip_cfg->trim_nl        = CF_DEFAULT_NL;
  }
  E_CONFIG_LIMIT(clip_cfg->clip_copy, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->clip_select, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->sync, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->persistence, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->hist_reverse, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->hist_items, HIST_MIN, HIST_MAX);
  E_CONFIG_LIMIT(clip_cfg->label_length, LABEL_MIN, LABEL_MAX);
  E_CONFIG_LIMIT(clip_cfg->save_timer, TIMER_MIN, TIMER_MAX);
  E_CONFIG_LIMIT(clip_cfg->confirm_clear, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->autosave, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->ignore_ws, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->ignore_ws_copy, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->trim_ws, 0, 1);
  E_CONFIG_LIMIT(clip_cfg->trim_nl, 0, 1);


  /* update the version */
  clip_cfg->version = MOD_CONFIG_FILE_VERSION;

  clip_cfg->module = m;
  /* save the config to disk */
  e_config_save_queue();
}

/* This is called when we need to cleanup the actual configuration,
 * for example when our configuration is too old */
static void
_clip_config_free(void)
{
  Config_Item *ci;

  EINA_LIST_FREE(clip_cfg->items, ci){
    eina_stringshare_del(ci->id);
    free(ci);
  }
  clip_cfg->module = NULL;
  E_FREE(clip_cfg);
}

/*
 * This function is called when you add the Module to a Shelf or Gadgets,
 *   this is where you want to add functions to do things.
 */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
  Evas_Object *o;
  E_Gadcon_Client *gcc;

  Instance *inst = NULL;
  inst = E_NEW(Instance, 1);

  o = e_icon_add(gc->evas);
  e_icon_fdo_icon_set(o, "edit-paste");
  evas_object_show(o);

  gcc = e_gadcon_client_new(gc, name, id, style, o);
  gcc->data = inst;

  inst->gcc = gcc;
  inst->o_button = o;

  e_gadcon_client_util_menu_attach(gcc);
  evas_object_event_callback_add(inst->o_button, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb) _cb_menu_show, inst);
  evas_object_event_callback_add(inst->o_button, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb) _cb_context_show, inst);

  return gcc;
}

/*
 * This function is called when you remove the Module from a Shelf or Gadgets,
 * what this function really does is clean up, it removes everything the module
 * displays
 */
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
  _clip_inst_free(gcc->data);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
  e_gadcon_client_aspect_set (gcc, 16, 16);
  e_gadcon_client_min_size_set (gcc, 16, 16);
}

/*
 * This function sets the Gadcon name of the module,
 *  do not confuse this with E_Module_Api
 */
static const char *
_gc_label (const E_Gadcon_Client_Class *client_class __UNUSED__)
{
  return "Clipboard";
}

/*
 * This functions sets the Gadcon icon, the icon you see when you go to add
 * the module to a Shelf or Gadgets.
 */
static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas * evas)
{
  Evas_Object *o = e_icon_add(evas);
  e_icon_fdo_icon_set(o, "edit-paste");
  return o;
}

/*
 * This function sets the id for the module, so it is unique from other
 * modules
 */
static const char *
_gc_id_new (const E_Gadcon_Client_Class *client_class __UNUSED__)
{
  return _gadcon_class.name;
}

/**
 * @brief  Generic call back function to Create Clipboard menu
 *
 * @param  inst, pointer to calling Instance
 *         event, pointer to Ecore Event which triggered call back
 *
 * @return void
 *
 *  Here we attempt to handle all Ecore events which trigger display of the
 *  clipboard menu: either right clicking a Clipboard gadget or pressing a key
 *  or key combination bound to Clipboard Show Float menu. The generic nature
 *  of this function is a preliminary atempt to avoid repeated code but at the
 *  same time keep the logic relatively simple.
 *
 */

static void
_cb_menu_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event)
{
  EINA_SAFETY_ON_NULL_RETURN(data);
  EINA_SAFETY_ON_NULL_RETURN(event);

  Eina_Bool mouse_event = (ecore_event_current_type_get() == MOUSE_BUTTON);
  Instance *inst = mouse_event ? data: clip_inst->inst;
  Evas_Coord x, y, w, h;
  unsigned dir, timestamp;

  /* Ignore all mouse events but right clicks  */
  if (mouse_event)
    IF_TRUE_RETURN(event->button != 1);
  /* Set time_stamp and Evas coordinates  */
  timestamp = mouse_event ? event->timestamp : 1;
  _set_mouse_coord(inst, mouse_event, &x,&y, &w, &h);
  /* Fill menu  */
  dir = _menu_fill(inst, mouse_event);
  if (inst->gcc) e_gadcon_locked_set(inst->gcc->gadcon, EINA_TRUE);
  /* Activate Menu  */
  e_menu_activate_mouse(inst->menu,
                        e_util_zone_current_get(e_manager_current_get()),
                        x, y, w, h, dir, timestamp);
}

static void
_cb_context_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event)
{
  EINA_SAFETY_ON_NULL_RETURN(data);
  EINA_SAFETY_ON_NULL_RETURN(event);
  /* Ignore all mouse events but left clicks  */
  IF_TRUE_RETURN(event->button != 3);

  Instance *inst = data;
  Evas_Coord x, y;
  E_Menu *m;
  E_Menu_Item *mi;

  /* create popup menu  */
  m = e_menu_new();
  mi = e_menu_item_new(m);
  e_menu_item_label_set(mi, _("Settings"));
  e_util_menu_item_theme_icon_set(mi, "preferences-system");
  e_menu_item_callback_set(mi, _cb_config_show, inst);

  /* Each Gadget Client has a utility menu from the Container  */
  m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
  e_menu_post_deactivate_callback_set(m, _cb_menu_post_deactivate, inst);

  e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);

  /* show the menu relative to gadgets position  */
  e_menu_activate_mouse(m, e_util_zone_current_get(e_manager_current_get()),
                        (x + event->output.x),
                        (y + event->output.y), 1, 1,
                        E_MENU_POP_DIRECTION_AUTO, event->timestamp);
  evas_event_feed_mouse_up(inst->gcc->gadcon->evas, event->button,
                        EVAS_BUTTON_NONE, event->timestamp, NULL);
}

static void
_set_mouse_coord(Instance *inst,
                 Eina_Bool mouse_event,
                 Evas_Coord * const x, Evas_Coord * const y,
                 Evas_Coord * const w, Evas_Coord * const h)
{
   int cx, cy;
   E_Container *con;
   E_Manager   *man;

  if (mouse_event){
    evas_object_geometry_get(inst->o_button, x, y, w, h);
    e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy,
                                        NULL, NULL);
    *x += cx;
    *y += cy;
  } else {
    man = e_manager_current_get();
    con = e_container_current_get(man);
    ecore_x_pointer_xy_get(con->win, x, y);
    *w = 1;
    *h = 1;
  }
}

static int
_menu_fill(Instance *inst, Eina_Bool mouse_event)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(inst, E_GADCON_ORIENT_VERT);

  E_Menu_Item *mi;
  /* Default Orientation of menu for float list */
  unsigned dir = E_GADCON_ORIENT_VERT;

  inst->menu = e_menu_new();
  if (!mouse_event){
    e_menu_post_deactivate_callback_set(inst->menu, _cb_menu_post_deactivate, inst);
  }

  if (clip_inst->items){
    Eina_List *it;
    Clip_Data *clip;

    /* Flag to see if Label len changed */
    Eina_Bool label_length_changed = clip_cfg->label_length_changed;
    clip_cfg->label_length_changed = EINA_FALSE;

    /* revert list if selected  */
    if (clip_cfg->hist_reverse)
      clip_inst->items=eina_list_reverse(clip_inst->items);

    /* show list in history menu  */
    EINA_LIST_FOREACH(clip_inst->items, it, clip){
      mi = e_menu_item_new(inst->menu);
      if (label_length_changed) {
        free(clip->name);
        set_clip_name(&clip->name, clip->content,
                       clip_cfg->ignore_ws, clip_cfg->label_length);
      }
      e_menu_item_label_set(mi, clip->name);
      e_menu_item_callback_set(mi, (E_Menu_Cb)_cb_menu_item, clip);
    }
  }
  else {
    mi = e_menu_item_new(inst->menu);
    e_menu_item_label_set(mi, _("Empty"));
    e_menu_item_disabled_set(mi, EINA_TRUE);
  }

  mi = e_menu_item_new(inst->menu);
  e_menu_item_separator_set(mi, EINA_TRUE);

  mi = e_menu_item_new(inst->menu);
  e_menu_item_label_set(mi, _("Clear"));
  e_util_menu_item_theme_icon_set(mi, "edit-clear");
  e_menu_item_callback_set(mi, (E_Menu_Cb) _cb_clear_history, inst);

  if (clip_inst->items)
    e_menu_item_disabled_set(mi, EINA_FALSE);
  else
    e_menu_item_disabled_set(mi, EINA_TRUE);

  mi = e_menu_item_new(inst->menu);
  e_menu_item_separator_set(mi, EINA_TRUE);

  mi = e_menu_item_new(inst->menu);
  e_menu_item_label_set(mi, _("Settings"));
  e_util_menu_item_theme_icon_set(mi, "preferences-system");
  e_menu_item_callback_set(mi, _cb_config_show, NULL);

  if (mouse_event) {
    e_menu_post_deactivate_callback_set(inst->menu, _cb_menu_post_deactivate, inst);
    /* Proper menu orientation
     *  We display not relatively to the gadget, but similarly to
     *  the start menu - thus the need for direction etc.
     */
    switch (inst->gcc->gadcon->orient) {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        dir = E_MENU_POP_DIRECTION_DOWN;
        break;
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        dir = E_MENU_POP_DIRECTION_UP;
        break;

      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_LB:
        dir = E_MENU_POP_DIRECTION_RIGHT;
        break;

      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_RB:
        dir = E_MENU_POP_DIRECTION_LEFT;
        break;

      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
      default:
        dir = E_MENU_POP_DIRECTION_AUTO;
        break;
    }
  }
  return dir;
}

static Eina_Bool
_cb_event_selection(Instance *instance, int type __UNUSED__, Ecore_X_Event_Selection_Notify * event)
{
  Ecore_X_Selection_Data_Text *text_data;
  Clip_Data *cd = NULL;
  char *last="";

  EINA_SAFETY_ON_NULL_RETURN_VAL(instance, EINA_TRUE);

  if (clip_inst->items)
    last =  ((Clip_Data *) eina_list_data_get (clip_inst->items))->content;

  if ((text_data = clipboard.get_text(event))) {
    if (strcmp(last, text_data->text ) != 0) {
      if (text_data->data.length == 0)
        return ECORE_CALLBACK_DONE;
      if (clip_cfg->ignore_ws_copy && is_empty(text_data->text)) {
        clipboard.clear();
        return ECORE_CALLBACK_PASS_ON;
      }
      cd = E_NEW(Clip_Data, 1);
      if (!set_clip_content(&cd->content, text_data->text,
                             CLIP_TRIM_MODE(clip_cfg))) {
        CRI("Something bad happened !!");
        /* Try to continue */
        E_FREE(cd);
        goto error;
      }
      if (!set_clip_name(&cd->name, cd->content,
                    clip_cfg->ignore_ws, clip_cfg->label_length)){
        CRI("Something bad happened !!");
        /* Try to continue */
        E_FREE(cd);
        goto error;
      }
      _clip_add_item(cd);

    }
  }
  error:
  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_event_owner(Instance *instance __UNUSED__, int type __UNUSED__, Ecore_X_Event_Fixes_Selection_Notify * event)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(event, ECORE_CALLBACK_DONE);

  static Ecore_X_Window  last_owner = 0;
  Ecore_X_Window owner = ecore_x_selection_owner_get(event->atom);

  if (owner == last_owner)
  {    _cb_clipboard_request(NULL);
      return ECORE_CALLBACK_DONE;
  }
  last_owner = owner;
  /* If we lost owner of clipboard */
  if (event->reason)
     /* Reset clipboard and gain ownership of it */
    _cb_menu_item(eina_list_data_get(clip_inst->items));
  else
    _cb_clipboard_request(NULL);
  return ECORE_CALLBACK_DONE;
}

/* Updates clipboard content with the selected text of the modules Menu */
void
_x_clipboard_update(const char *text)
{
  EINA_SAFETY_ON_NULL_RETURN(clip_inst);
  EINA_SAFETY_ON_NULL_RETURN(text);

  clipboard.set(clip_inst->win, text, strlen(text) + 1);
}

static void
_clip_add_item(Clip_Data *cd)
{
  Eina_List *it;
  EINA_SAFETY_ON_NULL_RETURN(cd);

  if (*cd->content == 0) {
    ERR("Warning Clip content is Empty!");
    clipboard.clear(); /* stop event selection cb */
    return;
  }

  if ((it = _item_in_history(cd))) {
    /* Move to top of list */
    clip_inst->items = eina_list_promote_list(clip_inst->items, it);
  } else {
    /* add item to the list */
    if (eina_list_count(clip_inst->items) < clip_cfg->hist_items) {
      clip_inst->items = eina_list_prepend(clip_inst->items, cd);
    }
    else {
      /* remove last item from the list */
      clip_inst->items = eina_list_remove_list(clip_inst->items, eina_list_last(clip_inst->items));
      /*  add clipboard data stored in cd to the list as a first item */
      clip_inst->items = eina_list_prepend(clip_inst->items, cd);
    }
  }

  /* saving list to the file */
  clip_inst->update_history = EINA_TRUE;
  clip_save(clip_inst->items, EINA_FALSE);
}

static Eina_List *
_item_in_history(Clip_Data *cd)
{
  /* Safety check: should never happen */
  EINA_SAFETY_ON_NULL_RETURN_VAL(cd, NULL);
  if (clip_inst->items)
    return eina_list_search_unsorted_list(clip_inst->items, (Eina_Compare_Cb) _clip_compare, cd->content);
  else
    return NULL;
}

static int
_clip_compare(Clip_Data *cd, char *text)
{
  return strcmp(cd->content, text);
}

static void
_clear_history(void)
{
  EINA_SAFETY_ON_NULL_RETURN(clip_inst);
  if (clip_inst->items)
    E_FREE_LIST(clip_inst->items, free_clip_data);

  /* Ensure clipboard is clear and save history */
  clipboard.clear();
  clip_inst->update_history = EINA_TRUE;
  clip_save(clip_inst->items, EINA_TRUE);
}

Eet_Error
clip_save(Eina_List *items, Eina_Bool force)
{
  if(clip_inst->update_history && clip_cfg->persistence && (clip_cfg->autosave || force))
  {
    clip_inst->update_history = EINA_FALSE;
    return save_history(items);
  }
  else
    return EET_ERROR_NONE;
}

static void
_cb_clear_history(Instance *inst __UNUSED__)
{
  EINA_SAFETY_ON_NULL_RETURN(clip_cfg);

  if (clip_cfg->confirm_clear) {
    e_confirm_dialog_show(_("Confirm History Deletion"),
                          "application-exit",
                          _("You wish to delete the clipboards history.<br>"
                          "<br>"
                          "Are you sure you want to delete it?"),
                          _("Delete"), _("Keep"),
                          _cb_dialog_delete, NULL, NULL, NULL,
                          _cb_dialog_keep, NULL);
  }
  else
    _clear_history();
}

static void
_cb_dialog_keep(void *data __UNUSED__)
{
  return;
}

static void
_cb_dialog_delete(void *data __UNUSED__)
{
  _clear_history();
}

static Eina_Bool
_cb_clipboard_request(void *data __UNUSED__)
{
  ecore_x_fixes_selection_notification_request(ECORE_X_ATOM_SELECTION_CLIPBOARD);
  clipboard.request(clip_inst->win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
  return EINA_TRUE;
}

Eina_Bool
cb_clipboard_save(void *data __UNUSED__)
{
    clip_save(clip_inst->items, EINA_TRUE);
    return EINA_TRUE;
}

static void
_cb_menu_item(Clip_Data *selected_clip)
{
  _x_clipboard_update(selected_clip->content);
}

static void
_cb_menu_post_deactivate(void *data, E_Menu *menu __UNUSED__)
{
  EINA_SAFETY_ON_NULL_RETURN(data);
  Instance *inst = data;

  if (inst->gcc)
     e_gadcon_locked_set(inst->gcc->gadcon, EINA_FALSE);
  if (inst->o_button && e_icon_edje_get(inst->o_button))
     e_icon_edje_emit(inst->o_button, "e,state,unfocused", "e");
  if (inst->menu) {
    e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
    e_object_del(E_OBJECT(inst->menu));
    inst->menu = NULL;
  }
}

static void
_cb_action_switch(E_Object *o __UNUSED__, const char *params, Instance *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, Mouse_Event *event)
{
  if (!strcmp(params, "float"))
    _cb_menu_show(data, NULL, NULL, event);
  else if (!strcmp(params, "settings"))
    _cb_config_show(data, NULL, NULL);
  else if (!strcmp(params, "clear"))
    /* Only call clear dialog if there is something to clear */
    if (clip_inst->items) _cb_clear_history(NULL);
}

void
free_clip_data(Clip_Data *clip)
{
  EINA_SAFETY_ON_NULL_RETURN(clip);
  free(clip->name);
  free(clip->content);
  free(clip);
}

static void
_clip_inst_free(Instance *inst)
{
  EINA_SAFETY_ON_NULL_RETURN(inst);
  inst->gcc = NULL;
  if (inst->menu) {
    e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
    e_object_del(E_OBJECT(inst->menu));
    inst->menu = NULL;
  }
  if(inst->o_button)
    evas_object_del(inst->o_button);
  E_FREE(inst);
}

/*
 * This is the first function called by e17 when you load the module
 */
EAPI void *
e_modapi_init (E_Module *m)
{
  Eet_Error hist_err;

  /* Display this Modules config info in the main Config Panel
   * Under Preferences catogory */
  e_configure_registry_item_add("extensions/clipboard", 10,
            "Clipboard Settings", NULL,
            "edit-paste", config_clipboard_module);

  conf_item_edd = E_CONFIG_DD_NEW("clip_cfg_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
  E_CONFIG_VAL(D, T, id, STR);
  conf_edd = E_CONFIG_DD_NEW("clip_cfg", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
  E_CONFIG_LIST(D, T, items, conf_item_edd);
  E_CONFIG_VAL(D, T, version, INT);
  E_CONFIG_VAL(D, T, clip_copy, INT);
  E_CONFIG_VAL(D, T, clip_select, INT);
  E_CONFIG_VAL(D, T, sync, INT);
  E_CONFIG_VAL(D, T, persistence, INT);
  E_CONFIG_VAL(D, T, hist_reverse, INT);
  E_CONFIG_VAL(D, T, hist_items, DOUBLE);
  E_CONFIG_VAL(D, T, confirm_clear, INT);
  E_CONFIG_VAL(D, T, autosave, INT);
  E_CONFIG_VAL(D, T, save_timer, DOUBLE);
  E_CONFIG_VAL(D, T, label_length, DOUBLE);
  E_CONFIG_VAL(D, T, ignore_ws, INT);
  E_CONFIG_VAL(D, T, ignore_ws_copy, INT);
  E_CONFIG_VAL(D, T, trim_ws, INT);
  E_CONFIG_VAL(D, T, trim_nl, INT);

  /* Tell E to find any existing module data. First run ? */
  clip_cfg = e_config_domain_load("module.clipboard", conf_edd);

   if (clip_cfg) {
     /* Check config version */
     if (!e_util_module_config_check("Clipboard", clip_cfg->version, MOD_CONFIG_FILE_VERSION))
       _clip_config_free();
   }

  /* If we don't have a config yet, or it got erased above,
   * then create a default one */
  if (!clip_cfg)
    _clip_config_new(m);

  /* Be sure we initialize our clipboard 'object' */
  init_clipboard_struct(clip_cfg);

  /* Initialize Einna_log for developers */
  _e_clipboard_log_dom = eina_log_domain_register("Clipboard", EINA_COLOR_ORANGE);
  eina_log_domain_level_set("Clipboard", EINA_LOG_LEVEL_INFO);

  //e_module_delayed_set(m, 1);

  /* Add Module Key Binding actions */
  act = e_action_add("clipboard");
  if (act) {
    act->func.go = (void *) _cb_action_switch;
    e_action_predef_name_set(_("Clipboard"), ACT_FLOAT, "clipboard", "float",    NULL, 0);
    e_action_predef_name_set(_("Clipboard"), ACT_CONFIG,   "clipboard", "settings", NULL, 0);
    e_action_predef_name_set(_("Clipboard"), ACT_CLEAR,   "clipboard", "clear",    NULL, 0);
  }

  /* Create a global clip_inst for our module
   *   complete with a hidden window for event notification purposes
   */
  clip_inst = E_NEW(Mod_Inst, 1);
  clip_inst->inst = E_NEW(Instance, 1);

  /* Create an invisible window for clipboard input purposes
   *   It is my understanding this should not displayed.*/
  //clip_inst->win = ecore_x_window_input_new(0, 10, 10, 100, 100);
   clip_inst->win = ecore_x_window_new(0, 0, 0, 1, 1);

  /* Now add some callbacks to handle clipboard events */
  ecore_x_fixes_selection_notification_request(ecore_x_atom_get("CLIPBOARD"));
  E_LIST_HANDLER_APPEND(clip_inst->handle, ECORE_X_EVENT_SELECTION_NOTIFY, _cb_event_selection, clip_inst);
  E_LIST_HANDLER_APPEND(clip_inst->handle, ECORE_X_EVENT_FIXES_SELECTION_NOTIFY, _cb_event_owner, clip_inst);
  clipboard.request(clip_inst->win, ECORE_X_SELECTION_TARGET_UTF8_STRING);

  /* Read History file and set clipboard */
  clip_inst->update_history = EINA_TRUE;
  hist_err = read_history(&(clip_inst->items), clip_cfg->ignore_ws, (unsigned int) clip_cfg->label_length);

  if (hist_err == EET_ERROR_NONE && eina_list_count(clip_inst->items))
    _cb_menu_item(eina_list_data_get(clip_inst->items));
  else
    /* Something must be wrong with history file
     *   so we create a new one */
    clip_save(clip_inst->items, EINA_TRUE);
  /* Make sure the history read has no more items than allowed
   *  by clipboard config file. This should never happen without user
   *  intervention of some kind. */
  if (clip_inst->items)
    if (eina_list_count(clip_inst->items) > clip_cfg->hist_items) {
      /* FIXME: Do we need to warn user in case this is backed up data
       *         being restored ? */
      WRN("History File truncation!");
      truncate_history(clip_cfg->hist_items);
  }

  clip_inst->update_history = EINA_FALSE;
  /* Don't let this be zero, for any reason.
   *    If it is the timer call back function uses 100% cpu */
  if (!EINA_DBL_NONZERO(clip_cfg->save_timer))
       clip_cfg->save_timer   = 1;
  /* Start timer if needed */
  if (clip_cfg->persistence && !clip_cfg->autosave)
    clip_inst->save_timer = ecore_timer_loop_add(clip_cfg->save_timer, cb_clipboard_save, NULL);
  /* Tell any gadget containers (shelves, etc) that we provide a module */
  e_gadcon_provider_register(&_gadcon_class);

  /* Give E the module */
  return m;
}

static void
_cb_config_show(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
  if (!clip_cfg) return;
  if (clip_cfg->config_dialog) return;
  config_clipboard_module(NULL, NULL);
}

/*
 * This function is called by e17 when you unload the module,
 * here you should free all resources used while the module was enabled.
 */
EAPI int
e_modapi_shutdown (E_Module *m __UNUSED__)
{
  Config_Item *ci;

  /* The 2 following EINA SAFETY checks should never happen
   *  and I usually avoid gotos but here I feel their use is harmless */
  EINA_SAFETY_ON_NULL_GOTO(clip_inst, noclip);

  /* Be sure history is saved              */
  clip_save(clip_inst->items, EINA_TRUE);
  /* Kill our clip_inst window and cleanup */
  if (clip_inst->win)
    ecore_x_window_free(clip_inst->win);
  E_FREE_LIST(clip_inst->handle, ecore_event_handler_del);
  clip_inst->handle = NULL;
  E_FREE_LIST(clip_inst->items, free_clip_data);
  _clip_inst_free(clip_inst->inst);
  if (clip_inst->save_timer)
      ecore_timer_del(clip_inst->save_timer);
  E_FREE(clip_inst);

noclip:
  EINA_SAFETY_ON_NULL_GOTO(clip_cfg, noconfig);

  /* Kill the config dialog */
  while((clip_cfg->config_dialog = e_config_dialog_get("E", "preferences/clipboard")))
    e_object_del(E_OBJECT(clip_cfg->config_dialog));

  if(clip_cfg->config_dialog)
    e_object_del(E_OBJECT(clip_cfg->config_dialog));
  E_FREE(clip_cfg->config_dialog);

  /* Cleanup our item list */
  EINA_LIST_FREE(clip_cfg->items, ci){
    eina_stringshare_del(ci->id);
    free(ci);
  }
  clip_cfg->module = NULL;
  /* keep the planet green */
  E_FREE(clip_cfg);

noconfig:
  /* Unregister the config dialog from the main panel */
  e_configure_registry_item_del("preferences/clipboard");

  /* Clean up all key binding actions */
  if (act) {
    e_action_predef_name_del("Clipboard", ACT_FLOAT);
    e_action_predef_name_del("Clipboard", ACT_CONFIG);
    e_action_predef_name_del("Clipboard", ACT_CLEAR);
    e_action_del("clipboard");
    act = NULL;
  }

  /* Clean EET */
  E_CONFIG_DD_FREE(conf_edd);
  E_CONFIG_DD_FREE(conf_item_edd);

  /* Shutdown Logger */
    eina_log_domain_unregister(_e_clipboard_log_dom);
   _e_clipboard_log_dom = -1;

  /* Tell E the module is now unloaded. Gets removed from shelves, etc. */
  e_gadcon_provider_unregister(&_gadcon_class);

  /* So long and thanks for all the fish */
  return 1;
}

/*
 * This function is used to save and store configuration info on local
 * storage
 */
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
  e_config_domain_save("module.clipboard", conf_edd, clip_cfg);
  return 1;
}

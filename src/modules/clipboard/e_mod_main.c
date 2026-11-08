#include "e_mod_main.h"
#include "x_clipboard.h"
#include "config_defaults.h"
#include "history.h"

EINTERN int _e_clipboard_log_dom = -1;

/* Stuff for convenience to compress code */
#define CLIP_TRIM_MODE(x) (x->trim_nl + 2 * (x->trim_ws))
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
static unsigned int _clip_id_counter = 0;

/*   First some callbacks   */
static Eina_Bool _cb_clipboard_request(void *data __UNUSED__);
static void      _cb_clipboard_retry(void);
static void      _cb_image_data(const void *data, size_t len);

static Eina_Bool _cb_event_selection(void *data, int type, void *event);
static Eina_Bool _cb_event_owner(void *data, int type, void *event);
static void      _cb_gadget_click(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info);
static void      _cb_context_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info);
static void      _cb_context_post_deactivate(void *data, E_Menu *menu __UNUSED__);
static void      _cb_dialog_delete_item(void *data);
static void      _cb_dialog_clear(void *data __UNUSED__);
static void      _cb_action_switch(E_Object *o __UNUSED__, const char *params);
static void      _cb_config_show(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);

/*   And then some auxillary functions */
static void        _clip_config_new(E_Module *m);
static void        _clip_config_free(void);
static void        _clip_inst_free(Instance *inst);
static Clip_Data  *_clip_add_item(Clip_Data *clip_data, Eina_Bool force);
static void        _clip_changed(void);
static void        _clear_history(void);
static void        _x_clipboard_update(const Clip_Data *cd);
static Eina_List  *_item_in_history(const Clip_Data *cd);
static int         _clip_compare(const Clip_Data *item, const Clip_Data *cd);

/* new module needs a new config :), or config too old and we need one anyway */
static void
_clip_config_new(E_Module *m)
{
   /* setup defaults */
   if (!clip_cfg)
     {
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
       clip_cfg->ignore_hist    = CF_DEFAULT_IGNORE_HIST;
       clip_cfg->trim_ws        = CF_DEFAULT_WS;
       clip_cfg->trim_nl        = CF_DEFAULT_NL;
       clip_cfg->store_images   = CF_DEFAULT_STORE_IMAGES;
       clip_cfg->image_max_mb   = CF_DEFAULT_IMAGE_MAX_MB;
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
   E_CONFIG_LIMIT(clip_cfg->ignore_hist, 0, 1);
   E_CONFIG_LIMIT(clip_cfg->trim_ws, 0, 1);
   E_CONFIG_LIMIT(clip_cfg->trim_nl, 0, 1);
   E_CONFIG_LIMIT(clip_cfg->store_images, 0, 1);
   E_CONFIG_LIMIT(clip_cfg->image_max_mb, IMG_MB_MIN, IMG_MB_MAX);


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

   EINA_LIST_FREE(clip_cfg->items, ci)
     {
       eina_stringshare_del(ci->id);
       free(ci);
     }
   clip_cfg->module = NULL;
   E_FREE(clip_cfg);
}

unsigned int
clip_id_next(void)
{
   return ++_clip_id_counter;
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

   o = edje_object_add(gc->evas);

   if (!e_theme_edje_object_set(o, "base/theme/modules/clipboard",
                           "e/modules/clipboard/main"))
     {
       evas_object_del(o);
       o = e_icon_add(gc->evas);
       if (!e_icon_fdo_icon_set(o, "clipboard"))
          e_icon_fdo_icon_set(o, "edit-paste");
     }

   evas_object_show(o);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_button = o;

   e_gadcon_client_util_menu_attach(gcc);
   evas_object_event_callback_add(inst->o_button, EVAS_CALLBACK_MOUSE_DOWN, _cb_gadget_click, inst);
   evas_object_event_callback_add(inst->o_button, EVAS_CALLBACK_MOUSE_DOWN, _cb_context_show, inst);

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
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf (buf, sizeof(buf), "%s/e-module-clipboard.edj", e_module_dir_get(clip_cfg->module));
   edje_object_file_set(o, buf, "icon");
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

/* Left click on the gadget: show or hide the history window */
static void
_cb_gadget_click(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Mouse_Event *event = event_info;

   EINA_SAFETY_ON_NULL_RETURN(event);
   IF_TRUE_RETURN(event->button != 1);
   clip_list_toggle();
}

/* Right click on the gadget: a small menu with the settings */
static void
_cb_context_show(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Mouse_Event *event = event_info;
   Instance *inst = data;
   Evas_Coord x, y;
   E_Menu *m;
   E_Menu_Item *mi;

   EINA_SAFETY_ON_NULL_RETURN(inst);
   EINA_SAFETY_ON_NULL_RETURN(event);
   /* Ignore all mouse events but right clicks  */
   IF_TRUE_RETURN(event->button != 3);

   if (inst->menu)
     {
       e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
       e_object_del(E_OBJECT(inst->menu));
       inst->menu = NULL;
     }

   /* create popup menu  */
   m = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system");
   e_menu_item_callback_set(mi, _cb_config_show, inst);

   /* Each Gadget Client has a utility menu from the Container  */
   m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
   inst->menu = m;
   e_menu_post_deactivate_callback_set(m, _cb_context_post_deactivate, inst);

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
_cb_context_post_deactivate(void *data, E_Menu *menu __UNUSED__)
{
   Instance *inst = data;

   EINA_SAFETY_ON_NULL_RETURN(inst);
   if (inst->menu)
     {
       e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
       e_object_del(E_OBJECT(inst->menu));
       inst->menu = NULL;
     }
}

/* ------------------------------------------------------------------------
 *  X selection events
 * ------------------------------------------------------------------------ */

/* Binary garbage must never become a history item */
static Eina_Bool
_utf8_valid(const char *s)
{
   const unsigned char *p = (const unsigned char *) s;

   while (*p)
     {
        int n;

        if (*p < 0x80) n = 0;
        else if ((*p & 0xE0) == 0xC0) n = 1;
        else if ((*p & 0xF0) == 0xE0) n = 2;
        else if ((*p & 0xF8) == 0xF0) n = 3;
        else return EINA_FALSE;
        p++;
        while (n--)
          {
             if ((*p & 0xC0) != 0x80) return EINA_FALSE;
             p++;
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_cb_event_selection(void *data, int type __UNUSED__, void *ev)
{
   Ecore_X_Event_Selection_Notify *event = ev;
   Ecore_X_Selection_Data_Text *text_data;
   Clip_Data *cd = NULL;
   const char *last = "";

   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_TRUE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, EINA_TRUE);

   /* Efl version 1.24.x and greater destroy the abilty to copy and paste in e_entry
    *  Even worse these EFLs cause Buggy behavior with our clipboard in e_entries.
    *  if ECORE_CALLBACK_DONE is not return in these EFLs pastes happens multiple times in an e_entry
    *
    * FIXME */

   Eina_Bool cb_ret = (EFL_MINOR < 24) ? ECORE_CALLBACK_PASS_ON : ECORE_CALLBACK_DONE;

   /* The owner of the clipboard told us what it has: if it is only an image
    *  go and get it. */
   if (clip_cfg->store_images && event->target &&
       (event->selection == ECORE_X_SELECTION_CLIPBOARD) &&
       !strcmp(event->target, "TARGETS"))
     {
        Eina_Bool png, txt;

        x_clipboard_targets_parse(event, &png, &txt);
        /* Text wins if both are offered (cells copied from a spreadsheet,
         *  for instance). Only now, with the TARGETS answer read, is it safe
         *  to ask for the text: see x_clipboard.h */
        if (png && !txt)
          x_clipboard_image_fetch((size_t) (clip_cfg->image_max_mb * 1024.0 * 1024.0),
                                  _cb_image_data, _cb_clipboard_retry);
        else
          x_clipboard_request_text(clip_inst->win);
        return cb_ret;
     }

   if (clip_inst->items)
     {
        const Clip_Data *top = eina_list_data_get(clip_inst->items);
        if (top->type == CLIP_TYPE_TEXT)
          last = top->content;
     }

   if ((text_data = clipboard.get_text(event)))
     {
       if (strcmp(last, text_data->text ) != 0)
         {
           if (text_data->data.length == 0)
             return ECORE_CALLBACK_DONE;
           if (!_utf8_valid(text_data->text))
             {
               WRN("Selection is not valid UTF-8, ignored");
               return cb_ret;
             }
           if (clip_cfg->ignore_ws_copy && is_empty(text_data->text))
             {
               clipboard.clear();
               return cb_ret;
             }
           cd = E_NEW(Clip_Data, 1);
           if (!cd) return cb_ret;
           cd->type = CLIP_TYPE_TEXT;
           cd->lock = strdup("U");
           if (!cd->lock ||
               !set_clip_content(&cd->content, text_data->text,
                                 CLIP_TRIM_MODE(clip_cfg)) ||
               !set_clip_name(&cd->name, cd->content,
                              clip_cfg->ignore_ws, clip_cfg->label_length))
             {
               CRI("Something bad happened !!");
               /* Try to continue */
               free_clip_data(cd);
               return cb_ret;
             }
           _clip_add_item(cd, EINA_FALSE);
         }
     }
   return cb_ret;
}

/* A PNG read from the clipboard by x_clipboard_image_fetch() */
static void
_cb_image_data(const void *data, size_t len)
{
   Clip_Data *cd;
   unsigned int w, h;
   char *id;

   if (!clip_cfg || !clip_inst || !clip_cfg->store_images) return;
   /* "Ignore history" is on: don't even write the file */
   if (clip_cfg->ignore_hist) return;

   if (!clip_image_png_info(data, len, &w, &h))
     {
        INF("Clipboard image is not a valid PNG, ignored");
        return;
     }
   id = clip_image_id_new(data, len);
   if (!id) return;

   /* Same as the newest item? Then there is nothing to do. */
   if (clip_inst->items)
     {
        const Clip_Data *top = eina_list_data_get(clip_inst->items);
        if ((top->type == CLIP_TYPE_IMAGE) && !strcmp(top->content, id))
          {
             free(id);
             return;
          }
     }
   if (!clip_image_store(id, data, len))
     {
        ERR("Unable to store clipboard image");
        free(id);
        return;
     }
   cd = E_NEW(Clip_Data, 1);
   if (!cd)
     {
        free(id);
        return;
     }
   cd->type    = CLIP_TYPE_IMAGE;
   cd->content = id;
   cd->w       = w;
   cd->h       = h;
   cd->size    = len;
   cd->lock    = strdup("U");
   cd->name    = clip_image_label_new(w, h, len);
   if (!cd->lock || !cd->name)
     {
        free_clip_data(cd);
        return;
     }
   _clip_add_item(cd, EINA_FALSE);
}

static Eina_Bool
_cb_event_owner(void *data __UNUSED__, int type __UNUSED__, void *ev)
{
   Ecore_X_Event_Fixes_Selection_Notify *event = ev;

   EINA_SAFETY_ON_NULL_RETURN_VAL(event, ECORE_CALLBACK_DONE);

   static Ecore_X_Window  last_owner = 0;
   Ecore_X_Window owner = ecore_x_selection_owner_get(event->atom);

   if (owner == last_owner)
     {
       _cb_clipboard_request(NULL);
       return ECORE_CALLBACK_DONE;
     }

   last_owner = owner;
   /* If we lost owner of clipboard */
   if (event->reason && clip_inst->items)
      /* Reset clipboard and gain ownership of it */
     _x_clipboard_update(eina_list_data_get(clip_inst->items));
   else
     _cb_clipboard_request(NULL);
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_cb_clipboard_request(void *data __UNUSED__)
{
   ecore_x_fixes_selection_notification_request(ECORE_X_ATOM_SELECTION_CLIPBOARD);
   if (clip_cfg->store_images && x_clipboard_images_tracked())
     {
        /* CLIPBOARD: ask what is on offer first, the text is requested when
         *  the answer arrives (one request at a time, see x_clipboard.h).
         *  PRIMARY uses a property of its own, so that is requested now. */
        x_clipboard_request_targets(clip_inst->win);
        if (clipboard.track_mode == CLIP_SELECTION_BOTH)
          ecore_x_selection_primary_request(clip_inst->win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
   else
     clipboard.request(clip_inst->win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
   return EINA_TRUE;
}

static void
_cb_clipboard_retry(void)
{
   if (clip_inst) _cb_clipboard_request(NULL);
}

/* Puts an item in the clipboard (text or image) */
static void
_x_clipboard_update(const Clip_Data *cd)
{
   EINA_SAFETY_ON_NULL_RETURN(clip_inst);
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (cd->type == CLIP_TYPE_IMAGE)
     {
        char path[PATH_MAX];

        if (!x_clipboard_images_tracked()) return;
        if (!clip_image_path_get(cd->content, path, sizeof(path)) ||
            !ecore_file_exists(path))
          {
             WRN("Image file of history item is missing");
             return;
          }
        x_clipboard_set_image(path);
        return;
     }
   clipboard.set(clip_inst->win, cd->content, strlen(cd->content) + 1);
}

/* ------------------------------------------------------------------------
 *  History
 * ------------------------------------------------------------------------ */

Clip_Data *
clip_find(unsigned int id)
{
   Eina_List *l;
   Clip_Data *cd;

   if (!id || !clip_inst) return NULL;
   EINA_LIST_FOREACH(clip_inst->items, l, cd)
     if (cd->id == id) return cd;
   return NULL;
}

/* Frees an item which is NOT in the history any more. If it is an image
 *  its file is removed too (unless another item still uses it). */
void
clip_discard(Clip_Data *cd)
{
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (cd->type == CLIP_TYPE_IMAGE)
     {
        Eina_List *l;
        Clip_Data *other;
        Eina_Bool used = EINA_FALSE;

        if (clip_inst)
          EINA_LIST_FOREACH(clip_inst->items, l, other)
            if ((other != cd) && (other->type == CLIP_TYPE_IMAGE) &&
                !strcmp(other->content, cd->content))
              {
                 used = EINA_TRUE;
                 break;
              }
        if (!used) clip_image_remove(cd->content);
     }
   free_clip_data(cd);
}

/* To be called after every change of the history */
static void
_clip_changed(void)
{
   clip_inst->update_history = EINA_TRUE;
   clip_save(clip_inst->items, EINA_FALSE);
   clip_list_refresh();
}

/* Adds an item to the top of the history, takes ownership of 'cd'.
 *  force: add even if "Ignore history" is set (items added by hand)
 *  Returns the item which now represents it in the history (which is not
 *  'cd' if the same content was already there) or NULL. */
static Clip_Data *
_clip_add_item(Clip_Data *cd, Eina_Bool force)
{
   Eina_List *it, *l;
   Clip_Data *old, *result;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, NULL);

   if (clip_cfg->ignore_hist && !force)
     {
        free_clip_data(cd);
        return NULL;
     }

   if (!cd->content || (*cd->content == 0))
     {
       ERR("Warning Clip content is Empty!");
       free_clip_data(cd);
       clipboard.clear(); /* stop event selection cb */
       return NULL;
     }

   if (!cd->id) cd->id = clip_id_next();

   if ((it = _item_in_history(cd)))
     {
       /* Move to top of list. This is the same content, so the new item is
        *  not needed (and must not delete the image file the old one uses) */
       result = eina_list_data_get(it);
       clip_inst->items = eina_list_promote_list(clip_inst->items, it);
       free_clip_data(cd);
     }
   else
     {
       result = cd;
       /* add item to the list */
       clip_inst->items = eina_list_prepend(clip_inst->items, cd);
       if (eina_list_count(clip_inst->items) > clip_cfg->hist_items)
         {
           EINA_LIST_REVERSE_FOREACH(clip_inst->items, l, old)
             {
               /* let's find the first unlocked item (reversely) */
               if ((old != cd) && !strcmp(old->lock, "U"))
                 {
                   /* remove last unlocked item from the list */
                   clip_inst->items = eina_list_remove_list(clip_inst->items, l);
                   clip_discard(old);
                   break;
                 }
             }
           /* If every item is locked the list is allowed to grow. */
         }
     }

   _clip_changed();
   return result;
}

static Eina_List *
_item_in_history(const Clip_Data *cd)
{
   /* Safety check: should never happen */
   EINA_SAFETY_ON_NULL_RETURN_VAL(cd, NULL);
   if (clip_inst->items)
     return eina_list_search_unsorted_list(clip_inst->items, (Eina_Compare_Cb) _clip_compare, cd);
   else
     return NULL;
}

/* Two items are the same if type and content are the same. For images the
 *  content is the id derived from the image data. */
static int
_clip_compare(const Clip_Data *item, const Clip_Data *cd)
{
  if (item->type != cd->type) return 1;
  return strcmp(item->content, cd->content);
}

static void
_clear_history(void)
{
   Clip_Data *cd;

   EINA_SAFETY_ON_NULL_RETURN(clip_inst);
   EINA_LIST_FREE(clip_inst->items, cd)
     clip_discard(cd);

   /* Ensure clipboard is clear and save history */
   clipboard.clear();
   clip_inst->update_history = EINA_TRUE;
   clip_save(clip_inst->items, EINA_TRUE);
   clip_list_refresh();
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

Eina_Bool
cb_clipboard_save(void *data __UNUSED__)
{
   clip_save(clip_inst->items, EINA_TRUE);
   return EINA_TRUE;
}

/* Label of text items depends on configuration */
void
clip_labels_refresh(void)
{
   Eina_List *l;
   Clip_Data *cd;
   char *name;

   if (!clip_cfg || !clip_inst || !clip_cfg->label_length_changed) return;
   clip_cfg->label_length_changed = EINA_FALSE;
   EINA_LIST_FOREACH(clip_inst->items, l, cd)
     {
        if (cd->type != CLIP_TYPE_TEXT) continue;
        name = NULL;
        if (set_clip_name(&name, cd->content, clip_cfg->ignore_ws,
                          clip_cfg->label_length))
          {
             free(cd->name);
             cd->name = name;
          }
     }
}

/* ---- Operations triggered from the list window ---- */

void
clip_use_item(Clip_Data *cd)
{
   EINA_SAFETY_ON_NULL_RETURN(cd);
   _x_clipboard_update(cd);
}

void
clip_add_text(const char *text)
{
   Clip_Data *cd, *item;

   EINA_SAFETY_ON_NULL_RETURN(clip_inst);
   if (!text || is_empty(text)) return;

   cd = E_NEW(Clip_Data, 1);
   if (!cd) return;
   cd->type = CLIP_TYPE_TEXT;
   cd->lock = strdup("U");
   if (!cd->lock ||
       !set_clip_content(&cd->content, text, CLIP_TRIM_MODE(clip_cfg)) ||
       !set_clip_name(&cd->name, cd->content, clip_cfg->ignore_ws,
                      clip_cfg->label_length))
     {
        free_clip_data(cd);
        return;
     }
   item = _clip_add_item(cd, EINA_TRUE);
   /* The newest item is what is in the clipboard */
   if (item) _x_clipboard_update(item);
}

void
clip_edit_text(unsigned int id, const char *text)
{
   Clip_Data *cd = clip_find(id);
   char *content = NULL, *name = NULL;

   if (!cd || (cd->type != CLIP_TYPE_TEXT) || !text) return;
   if (!set_clip_content(&content, text, CLIP_TRIM_MODE(clip_cfg))) return;
   if (!content || is_empty(content))
     {
        free(content);
        return;
     }
   if (!set_clip_name(&name, content, clip_cfg->ignore_ws, clip_cfg->label_length))
     {
        free(content);
        return;
     }
   free(cd->content);
   free(cd->name);
   cd->content = content;
   cd->name = name;

   /* If it is the newest item it is also what is in the clipboard */
   if (cd == eina_list_data_get(clip_inst->items))
     _x_clipboard_update(cd);
   _clip_changed();
}

void
clip_toggle_lock(Clip_Data *cd)
{
   char *lock;

   EINA_SAFETY_ON_NULL_RETURN(cd);
   lock = strdup(strcmp(cd->lock, "L") ? "L" : "U");
   if (!lock) return;
   free(cd->lock);
   cd->lock = lock;
   _clip_changed();
}

static void
_cb_dialog_delete_item(void *data)
{
   Clip_Data *cd = clip_find((unsigned int) (uintptr_t) data);

   if (!cd) return;   /* it is gone already */
   clip_inst->items = eina_list_remove(clip_inst->items, cd);
   clip_discard(cd);
   _clip_changed();
}

void
clip_delete_request(Clip_Data *cd)
{
   EINA_SAFETY_ON_NULL_RETURN(clip_cfg);
   EINA_SAFETY_ON_NULL_RETURN(cd);

   if (clip_cfg->confirm_clear)
     {
       e_confirm_dialog_show(_("Confirm Item Deletion"),
                               "application-exit",
                             _("You wish to delete the current item.<br>"
                               "<br>"
                               "Are you sure you want to delete it?"),
                             _("Delete"), _("Keep"),
                             _cb_dialog_delete_item, NULL,
                             (void *) (uintptr_t) cd->id, NULL,
                             NULL, NULL);
     }
   else
     _cb_dialog_delete_item((void *) (uintptr_t) cd->id);
}

static void
_cb_dialog_clear(void *data __UNUSED__)
{
   _clear_history();
}

void
clip_clear_request(void)
{
   EINA_SAFETY_ON_NULL_RETURN(clip_cfg);
   EINA_SAFETY_ON_NULL_RETURN(clip_inst);
   /* Only if there is something to clear */
   if (!clip_inst->items) return;

   if (clip_cfg->confirm_clear)
     {
       e_confirm_dialog_show(_("Confirm History Deletion"),
                            "application-exit",
                            _("You wish to delete the clipboards history.<br>"
                              "<br>"
                              "Are you sure you want to delete it?"),
                            _("Delete"), _("Keep"),
                            _cb_dialog_clear, NULL, NULL, NULL,
                            NULL, NULL);
     }
   else
     _clear_history();
}

/*
 * The action called by key bindings
 *  (Preferences/Input/Key Bindings, "Clipboard" group).
 */
static void
_cb_action_switch(E_Object *o __UNUSED__, const char *params)
{
   EINA_SAFETY_ON_NULL_RETURN(params);

   if (!strcmp(params, "float"))
     clip_list_toggle();
   else if (!strcmp(params, "settings"))
     _cb_config_show(NULL, NULL, NULL);
   else if (!strcmp(params, "clear"))
     clip_clear_request();
}

void
free_clip_data(Clip_Data *clip)
{
   EINA_SAFETY_ON_NULL_RETURN(clip);
   free(clip->name);
   free(clip->content);
   free(clip->lock);
   free(clip);
}

static void
_clip_inst_free(Instance *inst)
{
   EINA_SAFETY_ON_NULL_RETURN(inst);
   inst->gcc = NULL;
   if (inst->menu)
     {
       e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
       e_object_del(E_OBJECT(inst->menu));
       inst->menu = NULL;
     }
   if(inst->o_button)
     evas_object_del(inst->o_button);
   E_FREE(inst);
}

static Eina_Bool
_clip_cb_changed_icon_set(__UNUSED__ void *data, __UNUSED__ int ev_type, __UNUSED__ void *ev)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   e_gadcon_provider_register(&_gadcon_class);
   return ECORE_CALLBACK_PASS_ON;
}

/*
 * This is the first function called by e17 when you load the module
 */
EAPI void *
e_modapi_init (E_Module *m)
{
   Eet_Error hist_err;

   /* Initialize Einna_log for developers */
   _e_clipboard_log_dom = eina_log_domain_register("Clipboard", EINA_COLOR_ORANGE);
   // eina_log_domain_level_set("Clipboard", EINA_LOG_LEVEL_INFO);

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
   E_CONFIG_VAL(D, T, ignore_hist, INT);
   E_CONFIG_VAL(D, T, trim_ws, INT);
   E_CONFIG_VAL(D, T, trim_nl, INT);
   E_CONFIG_VAL(D, T, store_images, INT);
   E_CONFIG_VAL(D, T, image_max_mb, DOUBLE);

   /* Tell E to find any existing module data. First run ? */
   clip_cfg = e_config_domain_load("module.clipboard", conf_edd);

   if (clip_cfg)
     {
       /* Check config version */
       if (!e_util_module_config_check("Clipboard", clip_cfg->version, MOD_CONFIG_FILE_VERSION))
        _clip_config_free();
     }

   /* If we don't have a config yet, or it got erased above,
    * then create a default one */
   if (!clip_cfg)
     _clip_config_new(m);
   else if (clip_cfg->version < MOD_CONFIG_FILE_VERSION)
     {
       /* Config of an older generation: the new values are zero, give them
        *  proper defaults. */
       if (clip_cfg->version < (MOD_CONFIG_FILE_EPOCH * 1000000) + 6)
         {
           clip_cfg->store_images = CF_DEFAULT_STORE_IMAGES;
           clip_cfg->image_max_mb = CF_DEFAULT_IMAGE_MAX_MB;
         }
       clip_cfg->version = MOD_CONFIG_FILE_VERSION;
       e_config_save_queue();
     }

   clip_cfg->module = m;
   /* Be sure we initialize our clipboard 'object' */
   init_clipboard_struct(clip_cfg);

   // e_module_delayed_set(m, 1);

   /* Add Module Key Binding actions */
   act = e_action_add("clipboard");
   if (act)
     {
       act->func.go = _cb_action_switch;
       e_action_predef_name_set(_("Clipboard"), ACT_FLOAT, "clipboard", "float", NULL, 0);
       e_action_predef_name_set(_("Clipboard"), ACT_CONFIG, "clipboard", "settings", NULL, 0);
       e_action_predef_name_set(_("Clipboard"), ACT_CLEAR, "clipboard", "clear", NULL, 0);
     }

   /* Create a global clip_inst for our module
    *   complete with a hidden window for event notification purposes
    */
   clip_inst = E_NEW(Mod_Inst, 1);

   /* Create an invisible window for clipboard input purposes
    *   It is my understanding this should not displayed.*/
    // clip_inst->win = ecore_x_window_input_new(0, 10, 10, 100, 100);
   clip_inst->win = ecore_x_window_new(0, 0, 0, 1, 1);

   /* Now add some callbacks to handle clipboard events */
   ecore_x_fixes_selection_notification_request(ecore_x_atom_get("CLIPBOARD"));
   E_LIST_HANDLER_APPEND(clip_inst->handle, ECORE_X_EVENT_SELECTION_NOTIFY, _cb_event_selection, clip_inst);
   E_LIST_HANDLER_APPEND(clip_inst->handle, ECORE_X_EVENT_FIXES_SELECTION_NOTIFY, _cb_event_owner, clip_inst);
   E_LIST_HANDLER_APPEND(clip_inst->handle, E_EVENT_CONFIG_ICON_THEME, _clip_cb_changed_icon_set, NULL);
   E_LIST_HANDLER_APPEND(clip_inst->handle, EFREET_EVENT_ICON_CACHE_UPDATE, _clip_cb_changed_icon_set, NULL);
   _cb_clipboard_request(NULL);

   /* Read History file and set clipboard */
   clip_inst->update_history = EINA_TRUE;
   hist_err = read_history(&(clip_inst->items), clip_cfg->ignore_ws, (unsigned int) clip_cfg->label_length);

   if (hist_err == EET_ERROR_NONE && eina_list_count(clip_inst->items))
     _x_clipboard_update(eina_list_data_get(clip_inst->items));
   else
     /* Something must be wrong with history file
      *   so we create a new one */
     clip_save(clip_inst->items, EINA_TRUE);
   /* Make sure the history read has no more items than allowed
    *  by clipboard config file. This should never happen without user
    *  intervention of some kind. */
   if (clip_inst->items)
     {
       if (eina_list_count(clip_inst->items) > clip_cfg->hist_items)
         {
           /* FIXME: Do we need to warn user in case this is backed up data
            *         being restored ? */
           WRN("History File truncation!");
           truncate_history(clip_cfg->hist_items);
         }
     }
   /* Delete cached images nobody refers to any more
    *  (history deleted, crash, truncated history...) */
   clip_image_cleanup(clip_inst->items);

   clip_inst->update_history = EINA_FALSE;
   /* Don't let this be zero, for any reason.
    * If it is the timer call back function uses 100% cpu */
   if (!EINA_DBL_NONZERO(clip_cfg->save_timer))
     clip_cfg->save_timer   = 1;
   /* Start timer if needed */
   if (clip_cfg->persistence && !clip_cfg->autosave)
     clip_inst->save_timer = ecore_timer_loop_add(clip_cfg->save_timer, cb_clipboard_save, NULL);
   /* Tell any gadget containers (shelves, etc) that we provide a module */
   e_gadcon_provider_register(&_gadcon_class);

   /* Give E the module */
   return clip_cfg->module;
}

static void
_cb_config_show(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   if (!clip_cfg) return;
   if (clip_cfg->config_dialog) return;
   config_clipboard_module(NULL, NULL);
}

void
clip_config_show(void)
{
   _cb_config_show(NULL, NULL, NULL);
}

/*
 * This function is called by e17 when you unload the module,
 * here you should free all resources used while the module was enabled.
 */
EAPI int
e_modapi_shutdown (E_Module *m __UNUSED__)
{
   Config_Item *ci;
   Clip_Data *cd;

   /* The history window must go before the history does */
   clip_list_close(EINA_TRUE);
   x_clipboard_image_shutdown();

   /* The 2 following EINA SAFETY checks should never happen
    *  and I usually avoid gotos but here I feel their use is harmless */
   EINA_SAFETY_ON_NULL_GOTO(clip_inst, noclip);

   if (clip_inst->save_timer)
     {
       ecore_timer_del(clip_inst->save_timer);
       clip_inst->save_timer = NULL;
     }
   /* Be sure history is saved              */
   if (clip_cfg) clip_save(clip_inst->items, EINA_TRUE);
   /* Kill our clip_inst window and cleanup */
   if (clip_inst->win)
     ecore_x_window_free(clip_inst->win);
   E_FREE_LIST(clip_inst->handle, ecore_event_handler_del);
   clip_inst->handle = NULL;
   /* Without a saved history nobody will ever look at the cached images */
   EINA_LIST_FREE(clip_inst->items, cd)
     {
       if (clip_cfg && !clip_cfg->persistence && (cd->type == CLIP_TYPE_IMAGE))
         clip_image_remove(cd->content);
       free_clip_data(cd);
     }
   E_FREE(clip_inst);

noclip:
   EINA_SAFETY_ON_NULL_GOTO(clip_cfg, noconfig);

   /* Kill the config dialog */
   while((clip_cfg->config_dialog = e_config_dialog_get("E", "preferences/clipboard")))
     e_object_del(E_OBJECT(clip_cfg->config_dialog));

   /* Cleanup our item list */
   EINA_LIST_FREE(clip_cfg->items, ci)
     {
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
   if (act)
     {
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

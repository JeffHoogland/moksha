/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#ifdef HAVE_ECORE_IMF
#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>
#endif

typedef struct _E_Entry_Smart_Data E_Entry_Smart_Data;

struct _E_Entry_Smart_Data
{
   Evas_Object *entry_object;
   Evas_Object *editable_object;
   E_Menu *popup;
   Ecore_Event_Handler *selection_handler;
#ifdef HAVE_ECORE_IMF
   Ecore_IMF_Context *imf_context;
#endif
   Ecore_Event_Handler *imf_ee_commit_handler;
   Ecore_Event_Handler *imf_ee_delete_handler;
   
   int enabled;
   int focused;
   int selection_dragging;
   int selection_mode;
   float valign;
   int min_width;
   int height;
};

/* local subsystem functions */
static void _e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_key_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int _e_entry_x_selection_notify_handler(void *data, int type, void *event);

static void _e_entry_x_selection_update(Evas_Object *entry);
static void _e_entry_key_down_windows(Evas_Object *entry, Evas_Event_Key_Down *event);
static void _e_entry_key_down_emacs(Evas_Object *entry, Evas_Event_Key_Down *event);

static void _e_entry_smart_add(Evas_Object *object);
static void _e_entry_smart_del(Evas_Object *object);
static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_entry_smart_show(Evas_Object *object);
static void _e_entry_smart_hide(Evas_Object *object);
static void _e_entry_color_set(Evas_Object *object, int r, int g, int b, int a);
static void _e_entry_clip_set(Evas_Object *object, Evas_Object *clip);
static void _e_entry_clip_unset(Evas_Object *object);
static void _e_entry_cb_menu_post(void *data, E_Menu *m);
static void _e_entry_cb_cut(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_copy(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_paste(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_select_all(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_entry_cb_delete(void *data, E_Menu *m, E_Menu_Item *mi);
#ifdef HAVE_ECORE_IMF
static int _e_entry_cb_imf_retrieve_surrounding(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos);
static int _e_entry_cb_imf_event_commit(void *data, int type, void *event);
static int _e_entry_cb_imf_event_delete_surrounding(void *data, int type, void *event);
#endif

/* local subsystem globals */
static Evas_Smart *_e_entry_smart = NULL;
static int _e_entry_smart_use = 0;
static int _e_entry_emacs_keybindings = 0;


/* externally accessible functions */

/**
 * Creates a new entry object. An entry is a field where the user can type
 * single-line text.
 * Use the "changed" smart callback to know when the content of the entry is
 * changed
 *
 * @param evas the evas where the entry object should be added
 * @return Returns the new entry object
 */
EAPI Evas_Object *
e_entry_add(Evas *evas)
{
   if (!_e_entry_smart)
     {
        static const Evas_Smart_Class sc =
	  {
	     "e_entry",
	       EVAS_SMART_CLASS_VERSION,
	       _e_entry_smart_add,
	       _e_entry_smart_del,
	       _e_entry_smart_move,
	       _e_entry_smart_resize,
	       _e_entry_smart_show,
	       _e_entry_smart_hide,
	       _e_entry_color_set,
	       _e_entry_clip_set,
	       _e_entry_clip_unset,
	       NULL,
	       NULL,
	       NULL,
	       NULL
	  };
	_e_entry_smart = evas_smart_class_new(&sc);
        _e_entry_smart_use = 0;
     }
   
   _e_entry_smart_use++;
   return evas_object_smart_add(evas, _e_entry_smart);
}

/**
 * Sets the text of the entry object
 *
 * @param entry an entry object
 * @param text the text to set
 */
EAPI void
e_entry_text_set(Evas_Object *entry, const char *text)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   
   e_editable_text_set(sd->editable_object, text);
   evas_object_smart_callback_call(entry, "changed", NULL);
}

/**
 * Gets the text of the entry object
 *
 * @param entry an entry object
 * @return Returns the text of the entry object
 */
EAPI const char *
e_entry_text_get(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return NULL;
   return e_editable_text_get(sd->editable_object);
}

/**
 * Clears the entry object
 *
 * @param entry an entry object
 */
EAPI void
e_entry_clear(Evas_Object *entry)
{
   e_entry_text_set(entry, "");
}

/**
 * Gets the editable object used by the entry object. It will allow you to have
 * better control on the text, the cursor or the selection of the entry with
 * the e_editable_*() functions.
 *
 * @param entry an entry object
 * @return Returns the editable object used by the entry object
 */
EAPI Evas_Object *
e_entry_editable_object_get(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return NULL;
   return sd->editable_object;
}

/**
 * Sets whether or not the entry object is in password mode. In password mode,
 * the entry displays '*' instead of the characters
 *
 * @param entry an entry object
 * @param password_mode 1 to turn on password mode, 0 to turn it off
 */
EAPI void
e_entry_password_set(Evas_Object *entry, int password_mode)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   e_editable_password_set(sd->editable_object, password_mode);
#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     ecore_imf_context_input_mode_set(sd->imf_context,
                                      password_mode ? ECORE_IMF_INPUT_MODE_FULL & ECORE_IMF_INPUT_MODE_INVISIBLE :
                                                      ECORE_IMF_INPUT_MODE_FULL);
#endif
}

/**
 * Gets the minimum size of the entry object
 *
 * @param entry an entry object
 * @param minw the location where to store the minimun width of the entry
 * @param minh the location where to store the minimun height of the entry
 */
EAPI void
e_entry_min_size_get(Evas_Object *entry, Evas_Coord *minw, Evas_Coord *minh)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   
   if (minw) *minw = sd->min_width;
   if (minh) *minh = sd->height;
}

/**
 * Focuses the entry object. It will receives keyboard events and the user could
 * then type some text (the entry should also be enabled. The cursor and the
 * selection will be shown
 *
 * @param entry the entry to focus
 */
EAPI void
e_entry_focus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->focused)
      return;
   
   evas_object_focus_set(entry, 1);
   edje_object_signal_emit(sd->entry_object, "e,state,focused", "e");
   if (!sd->selection_dragging)
     {
        e_editable_cursor_move_to_end(sd->editable_object);
#ifdef HAVE_ECORE_IMF
        if (sd->imf_context)
          {
             ecore_imf_context_reset(sd->imf_context);
             ecore_imf_context_cursor_position_set(sd->imf_context,
                                                   e_editable_cursor_pos_get(sd->editable_object));
          }
#endif
        e_editable_selection_move_to_end(sd->editable_object);
     }
   if (sd->enabled)
      e_editable_cursor_show(sd->editable_object);
   e_editable_selection_show(sd->editable_object);
#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
        ecore_imf_context_reset(sd->imf_context);
        ecore_imf_context_focus_in(sd->imf_context);
     }
#endif
   sd->focused = 1;
}

/**
 * Unfocuses the entry object. It will no longer receives keyboard events so
 * the user could no longer type some text. The cursor and the selection will
 * be hidden
 *
 * @param entry the entry object to unfocus
 */
EAPI void
e_entry_unfocus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!sd->focused)
     return;
   
   evas_object_focus_set(entry, 0);
   edje_object_signal_emit(sd->entry_object, "e,state,unfocused", "e");
   e_editable_cursor_hide(sd->editable_object);
   e_editable_selection_hide(sd->editable_object);
#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
        ecore_imf_context_reset(sd->imf_context);
        ecore_imf_context_focus_out(sd->imf_context);
     }
#endif
   sd->focused = 0;
}

/**
 * Enables the entry object: the user will be able to type text
 *
 * @param entry the entry object to enable
 */
EAPI void
e_entry_enable(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (sd->enabled)
     return;
   
   edje_object_signal_emit(sd->entry_object, "e,state,enabled", "e");
   e_editable_enable(sd->editable_object);
   if (sd->focused)
     e_editable_cursor_show(sd->editable_object);
   sd->enabled = 1;
}

/**
 * Disables the entry object: the user won't be able to type anymore. Selection
 * will still be possible (to copy the text)
 *
 * @param entry the entry object to disable
 */
EAPI void
e_entry_disable(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!sd->enabled)
     return;
   
   edje_object_signal_emit(sd->entry_object, "e,state,disabled", "e");
   e_editable_disable(sd->editable_object);
   e_editable_cursor_hide(sd->editable_object);
   sd->enabled = 0;
}


/* Private functions */

/* Called when a key has been pressed by the user */
static void
_e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;

   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	Ecore_IMF_Event_Key_Down ev;

	ecore_imf_evas_event_key_down_wrap(event_info, &ev);
	if (ecore_imf_context_filter_event(sd->imf_context,
					   ECORE_IMF_EVENT_KEY_DOWN,
					   (Ecore_IMF_Event *) &ev))
	  return;
     }
#endif

   if (_e_entry_emacs_keybindings)
     _e_entry_key_down_emacs(obj, event_info);
   else
     _e_entry_key_down_windows(obj, event_info);
}

/* Called when a key has been released by the user */
static void
_e_entry_key_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;

   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	Ecore_IMF_Event_Key_Up ev;

	ecore_imf_evas_event_key_up_wrap(event_info, &ev);
	if (ecore_imf_context_filter_event(sd->imf_context,
					   ECORE_IMF_EVENT_KEY_UP,
					   (Ecore_IMF_Event *) &ev))
	  return;
     }
#endif
}

/* Called when the entry object is pressed by the mouse */
static void
_e_entry_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   Evas_Event_Mouse_Down *event;
   Evas_Coord ox, oy;
   int pos;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   if (!(event = event_info))
     return;

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	Ecore_IMF_Event_Mouse_Down ev;

	ecore_imf_evas_event_mouse_down_wrap(event_info, &ev);
	if (ecore_imf_context_filter_event(sd->imf_context,
					   ECORE_IMF_EVENT_MOUSE_DOWN,
					   (Ecore_IMF_Event *) &ev))
	  return;
     }
#endif

   evas_object_geometry_get(sd->editable_object, &ox, &oy, NULL, NULL);
   pos = e_editable_pos_get_from_coords(sd->editable_object,
                                        event->canvas.x - ox,
                                        event->canvas.y - oy);
  
   if (event->button == 1)
     {
	if (event->flags & EVAS_BUTTON_TRIPLE_CLICK)
	  {
	     e_editable_select_all(sd->editable_object);
	     _e_entry_x_selection_update(obj);
	  }
	else if (event->flags & EVAS_BUTTON_DOUBLE_CLICK)
	  {
	     e_editable_select_word(sd->editable_object, pos);
	     _e_entry_x_selection_update(obj);
	  }
        else
          {
             e_editable_cursor_pos_set(sd->editable_object, pos);
             if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
               e_editable_selection_pos_set(sd->editable_object, pos);
             
             sd->selection_dragging = 1;
          }
     }
   else if (event->button == 2)
     {
        E_Win *win;
        
        e_editable_cursor_pos_set(sd->editable_object, pos);
        e_editable_selection_pos_set(sd->editable_object, pos);
        
        if ((win = e_win_evas_object_win_get(obj)))
          ecore_x_selection_primary_request(win->evas_win,
					    ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
   else if (event->button == 3) 
     {
	E_Menu_Item *mi;
	E_Manager *man;
	E_Container *con;
	int x, y;
	int cursor_pos, selection_pos;
	int start_pos, end_pos;
        int s_enabled, s_selecting, s_empty, s_passwd;

	cursor_pos = e_editable_cursor_pos_get(sd->editable_object);
	selection_pos = e_editable_selection_pos_get(sd->editable_object);
	start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
	end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;

	s_selecting = (start_pos != end_pos);
        s_enabled = sd->enabled;
        s_empty = !e_editable_text_length_get(sd->editable_object);
        s_passwd = e_editable_password_get(sd->editable_object);
	
        if (!s_selecting && !s_enabled && s_empty) return;

	man = e_manager_current_get();
	con = e_container_current_get(man);
	ecore_x_pointer_xy_get(con->win, &x, &y);

	/* Popup a menu */
	sd->popup = e_menu_new();
	e_menu_post_deactivate_callback_set(sd->popup, 
					    _e_entry_cb_menu_post, sd);
        if (s_selecting)
	  {
             if (s_enabled)
               {
                  mi = e_menu_item_new(sd->popup);
                  e_menu_item_label_set(mi, _("Delete"));
                  e_util_menu_item_theme_icon_set(mi, "edit-delete");
                  e_menu_item_callback_set(mi, _e_entry_cb_delete, sd);
             
                  mi = e_menu_item_new(sd->popup);
                  e_menu_item_separator_set(mi, 1);

                  if (!s_passwd)
                    {
                       mi = e_menu_item_new(sd->popup);
                       e_menu_item_label_set(mi, _("Cut"));
                       e_util_menu_item_theme_icon_set(mi, "edit-cut");
                       e_menu_item_callback_set(mi, _e_entry_cb_cut, sd);
                    }
               }
             if (!s_passwd)
               {
		  mi = e_menu_item_new(sd->popup);
		  e_menu_item_label_set(mi, _("Copy"));
		  e_util_menu_item_theme_icon_set(mi, "edit-copy");
		  e_menu_item_callback_set(mi, _e_entry_cb_copy, sd);
	       }
	  }
        if (sd->enabled)
          {
             mi = e_menu_item_new(sd->popup);
             e_menu_item_label_set(mi, _("Paste"));
             e_util_menu_item_theme_icon_set(mi, "edit-paste");
             e_menu_item_callback_set(mi, _e_entry_cb_paste, sd);
          }
        if (!s_empty)
          {
             mi = e_menu_item_new(sd->popup);
             e_menu_item_separator_set(mi, 1);
             
             mi = e_menu_item_new(sd->popup);
             e_menu_item_label_set(mi, _("Select All"));
             e_util_menu_item_theme_icon_set(mi, "edit-select-all");
             e_menu_item_callback_set(mi, _e_entry_cb_select_all, sd);
          }

	e_menu_activate_mouse(sd->popup, e_util_zone_current_get(man),
			      x, y, 1, 1, 
			      E_MENU_POP_DIRECTION_DOWN, event->timestamp);
     }

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
        ecore_imf_context_reset(sd->imf_context);
        ecore_imf_context_cursor_position_set(sd->imf_context,
                                              e_editable_cursor_pos_get(sd->editable_object));
     }
#endif
}

/* Called when the entry object is released by the mouse */
static void
_e_entry_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   
#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	Ecore_IMF_Event_Mouse_Up ev;

	ecore_imf_evas_event_mouse_up_wrap(event_info, &ev);
	if (ecore_imf_context_filter_event(sd->imf_context,
					   ECORE_IMF_EVENT_MOUSE_UP,
					   (Ecore_IMF_Event *) &ev))
	  return;
     }
#endif

   if (sd->selection_dragging)
     {
        sd->selection_dragging = 0;
        _e_entry_x_selection_update(obj);
     }
}

/* Called when the mouse moves over the entry object */
static void
_e_entry_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   Evas_Event_Mouse_Move *event;
   Evas_Coord ox, oy;
   int pos;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   if (!(event = event_info))
     return;

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	Ecore_IMF_Event_Mouse_Move ev;

	ecore_imf_evas_event_mouse_move_wrap(event_info, &ev);
	if (ecore_imf_context_filter_event(sd->imf_context,
					   ECORE_IMF_EVENT_MOUSE_MOVE,
					   (Ecore_IMF_Event *) &ev))
	  return;
     }
#endif

   if (sd->selection_dragging)
     {
        evas_object_geometry_get(sd->editable_object, &ox, &oy, NULL, NULL);
        pos = e_editable_pos_get_from_coords(sd->editable_object,
                                             event->cur.canvas.x - ox,
                                             event->cur.canvas.y - oy);
        e_editable_cursor_pos_set(sd->editable_object, pos);
#ifdef HAVE_ECORE_IMF
        if (sd->imf_context)
          {
             ecore_imf_context_reset(sd->imf_context);
             ecore_imf_context_cursor_position_set(sd->imf_context,
                                                   pos);
          }
#endif
     }
}

/* Called when the the "selection_notify" event is emitted */
static int
_e_entry_x_selection_notify_handler(void *data, int type, void *event)
{
   Evas_Object *entry;
   E_Entry_Smart_Data *sd;
   Ecore_X_Event_Selection_Notify *ev;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   int changed = 0;
   
   if ((!(entry = data)) || (!(sd = evas_object_smart_data_get(entry))))
     return 1;
   if (!sd->focused)
     return 1;
   
   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   
   ev = event;
   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
        if (strcmp(ev->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0)
          {
             Ecore_X_Selection_Data_Text *text_data;
             
             text_data = ev->data;
             if (selecting && !_e_entry_emacs_keybindings)
               changed |= e_editable_delete(editable, start_pos, end_pos);
             changed |= e_editable_insert(editable, start_pos, text_data->text);
          }
     }
   
   if (changed)
     evas_object_smart_callback_call(entry, "changed", NULL);
   
   return 1;
}

/* Updates the X selection with the selected text of the entry */
static void
_e_entry_x_selection_update(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   E_Win *win;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   char *text;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if (!(win = e_win_evas_object_win_get(entry)))
     return;
   
   editable = sd->editable_object;
   if (e_editable_password_get(editable)) return;

   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
  
   if ((!selecting) ||
       (!(text = e_editable_text_range_get(editable, start_pos, end_pos))))
     return;

   ecore_x_selection_primary_set(win->evas_win, text, strlen(text) + 1);
   free(text);
}

/* Treats the "key down" event to mimick the behavior of Windows/Gtk2/Qt */
static void 
_e_entry_key_down_windows(Evas_Object *entry, Evas_Event_Key_Down *event)
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   int changed = 0;
   int selection_changed = 0;
   char *range;
   E_Win *win;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if ((!event) || (!event->keyname))
     return;

   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   
   /* Move the cursor/selection to the left */
   if (strcmp(event->key, "Left") == 0)
     {
        if (evas_key_modifier_is_set(event->modifiers, "Shift"))
          {
             e_editable_cursor_move_left(editable);
             selection_changed = 1;
          }
        else if (selecting)
          {
             if (cursor_pos < selection_pos)
               e_editable_selection_pos_set(editable, cursor_pos);
             else
               e_editable_cursor_pos_set(editable, selection_pos);
          }
        else
          {
             e_editable_cursor_move_left(editable);
             e_editable_selection_pos_set(editable,
                                          e_editable_cursor_pos_get(editable));
          }
     }
   /* Move the cursor/selection to the right */
   else if (strcmp(event->key, "Right") == 0)
     {
        if (evas_key_modifier_is_set(event->modifiers, "Shift"))
          {
             e_editable_cursor_move_right(editable);
             selection_changed = 1;
          }
        else if (selecting)
          {
             if (cursor_pos > selection_pos)
               e_editable_selection_pos_set(editable, cursor_pos);
             else
               e_editable_cursor_pos_set(editable, selection_pos);
          }
        else
          {
             e_editable_cursor_move_right(editable);
             e_editable_selection_pos_set(editable,
                                          e_editable_cursor_pos_get(editable));
          }
     }
   /* Move the cursor/selection to the start of the entry */
   else if (strcmp(event->keyname, "Home") == 0)
     {
        e_editable_cursor_move_to_start(editable);
        if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
          e_editable_selection_pos_set(editable,
                                       e_editable_cursor_pos_get(editable));
        else
          selection_changed = 1;
     }
   /* Move the cursor/selection to the end of the entry */
   else if (strcmp(event->keyname, "End") == 0)
     {
        e_editable_cursor_move_to_end(editable);
        if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
          e_editable_selection_pos_set(editable,
                                       e_editable_cursor_pos_get(editable));
        else
          selection_changed = 1;
     }
   /* Delete the previous character */
   else if ((sd->enabled) && (strcmp(event->keyname, "BackSpace") == 0))
     {
        if (selecting)
          changed = e_editable_delete(editable, start_pos, end_pos);
        else
          changed = e_editable_delete(editable, cursor_pos - 1, cursor_pos);
     }
   /* Delete the next character */
   else if ((sd->enabled) && (strcmp(event->keyname, "Delete") == 0))
     {
        if (selecting)
          changed = e_editable_delete(editable, start_pos, end_pos);
        else
          changed = e_editable_delete(editable, cursor_pos, cursor_pos + 1);
     }
   /* Ctrl + A,C,X,V */
   else if (evas_key_modifier_is_set(event->modifiers, "Control"))
     {
        if (strcmp(event->keyname, "a") == 0)
          {
             e_editable_select_all(editable);
             selection_changed = 1;
          }
        else if ((strcmp(event->keyname, "x") == 0) ||
                 (strcmp(event->keyname, "c") == 0))
          {
             if (!e_editable_password_get(editable) && selecting)
               {
                 range = e_editable_text_range_get(editable, start_pos, end_pos);
                 if (range)
                   {
                      if ((win = e_win_evas_object_win_get(entry)))
                        ecore_x_selection_clipboard_set(win->evas_win,
                                                        range,
                                                        strlen(range) + 1);
                      free(range);
                   }
                 if ((sd->enabled) && (strcmp(event->keyname, "x") == 0))
                   changed = e_editable_delete(editable, start_pos, end_pos);
               }
           }
        else if ((sd->enabled) && (strcmp(event->keyname, "v") == 0))
          {
             if ((win = e_win_evas_object_win_get(entry)))
               ecore_x_selection_clipboard_request(win->evas_win,
						   ECORE_X_SELECTION_TARGET_UTF8_STRING);
          }
     }
   /* Otherwise, we insert the corresponding character */
   else if ((event->string) && ((sd->enabled)) &&
          ((strlen(event->string) != 1) || (event->string[0] >= 0x20)))
     {
        if (selecting)
          changed |= e_editable_delete(editable, start_pos, end_pos);
        changed |= e_editable_insert(editable, start_pos, event->string);
     }

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
        ecore_imf_context_reset(sd->imf_context);
        ecore_imf_context_cursor_position_set(sd->imf_context,
                                              e_editable_cursor_pos_get(editable));
     }
#endif

   if (changed)
     evas_object_smart_callback_call(entry, "changed", NULL);
   if (selection_changed)
     _e_entry_x_selection_update(entry);
}

/* Treats the "key down" event to mimick the behavior of Emacs */
static void 
_e_entry_key_down_emacs(Evas_Object *entry, Evas_Event_Key_Down *event)
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   int changed = 0;
   int selection_changed = 0;
   char *range;
   E_Win *win;
   
   if ((!entry) || (!(sd = evas_object_smart_data_get(entry))))
     return;
   if ((!event) || (!event->keyname))
     return;

   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   
   /* Move the cursor/selection to the left */
   if ((strcmp(event->key, "Left") == 0) ||
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "b") == 0)))
     {
        e_editable_cursor_move_left(editable);
        if (sd->selection_mode)
          selection_changed = 1;
        else
	  e_editable_selection_pos_set(editable,
				       e_editable_cursor_pos_get(editable));
     }
   /* Move the cursor/selection to the right */
   else if ((strcmp(event->key, "Right") == 0) ||
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "f") == 0)))
     {
        e_editable_cursor_move_right(editable);
        if (sd->selection_mode)
          selection_changed = 1;
        else
	  e_editable_selection_pos_set(editable,
				       e_editable_cursor_pos_get(editable));
     }
   /* Move the cursor/selection to the start of the entry */
   else if ((strcmp(event->keyname, "Home") == 0) ||
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "a") == 0)))
     {
        e_editable_cursor_move_to_start(editable);
        if (sd->selection_mode)
          selection_changed = 1;
        else
          e_editable_selection_pos_set(editable,
                                       e_editable_cursor_pos_get(editable));
     }
   /* Move the cursor/selection to the end of the entry */
   else if ((strcmp(event->keyname, "End") == 0) ||
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "e") == 0)))
     {
        e_editable_cursor_move_to_end(editable);
        if (sd->selection_mode)
          selection_changed = 1;
        else
          e_editable_selection_pos_set(editable,
                                       e_editable_cursor_pos_get(editable));
     }
   /* Delete the previous character */
   else if ((sd->enabled) && (strcmp(event->keyname, "BackSpace") == 0))
     changed = e_editable_delete(editable, cursor_pos - 1, cursor_pos);
   /* Delete the next character */
   else if ((sd->enabled) && 
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "d") == 0)))
     changed = e_editable_delete(editable, cursor_pos, cursor_pos + 1);
   /* Delete until end of line */
   else if ((sd->enabled) && 
      ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "k") == 0)))
     changed = e_editable_delete(editable, cursor_pos,
                                 e_editable_text_length_get(editable));
   /* Toggle the selection mode */
   else if ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
       (strcmp(event->key, "space") == 0))
     {
        if (sd->selection_mode)
          {
	     e_editable_selection_pos_set(editable, cursor_pos);
	     sd->selection_mode = 0;
          }
        else
	  sd->selection_mode = 1;
     }
   /* Cut/Copy */
   else if ((evas_key_modifier_is_set(event->modifiers, "Control") ||
	     evas_key_modifier_is_set(event->modifiers, "Shift")) &&
	    (strcmp(event->key, "w") == 0))
     {
	if (!e_editable_password_get(editable) && selecting)
          {
	     range = e_editable_text_range_get(editable, start_pos, end_pos);
	     if (range)
	       {
		  if ((win = e_win_evas_object_win_get(entry)))
		    ecore_x_selection_clipboard_set(win->evas_win,
						    range,
						    strlen(range) + 1);
		  free(range);
	       }
	     if ((sd->enabled) && (evas_key_modifier_is_set(event->modifiers, "Control")))
	       {
		  changed = e_editable_delete(editable, start_pos, end_pos);
		  sd->selection_mode = 0;
	       }
	  }
     }
   /* Paste */
   else if ((sd->enabled) && 
	    ((evas_key_modifier_is_set(event->modifiers, "Control")) &&
	     (strcmp(event->key, "y") == 0)))
     {
        if ((win = e_win_evas_object_win_get(entry)))
          ecore_x_selection_clipboard_request(win->evas_win,
					      ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
   /* Otherwise, we insert the corresponding character */
   else if ((event->string) &&
	    ((strlen(event->string) != 1) ||
	     (event->string[0] >= 0x20 && event->string[0] != 0x7f)))
     changed = e_editable_insert(editable, cursor_pos, event->string);

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
        ecore_imf_context_reset(sd->imf_context);
        ecore_imf_context_cursor_position_set(sd->imf_context,
                                              e_editable_cursor_pos_get(editable));
     }
#endif

   if (changed)
     evas_object_smart_callback_call(entry, "changed", NULL);
   if (selection_changed)
     _e_entry_x_selection_update(entry);
}

/* Editable object's smart methods */

static void 
_e_entry_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Entry_Smart_Data *sd;
   Evas_Object *o;
   int cw, ch;
   const char *ctx_id;
#ifdef HAVE_ECORE_IMF
   const Ecore_IMF_Context_Info *ctx_info;
#endif
   
   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = malloc(sizeof(E_Entry_Smart_Data));
   if (!sd) return;
   
   evas_object_smart_data_set(object, sd);

#ifdef HAVE_ECORE_IMF
   ctx_id = ecore_imf_context_default_id_get();
   if (ctx_id)
     {
	ctx_info = ecore_imf_context_info_by_id_get(ctx_id);
	if (!ctx_info->canvas_type ||
	    strcmp(ctx_info->canvas_type, "evas") == 0)
	  sd->imf_context = ecore_imf_context_add(ctx_id);
	else
	  {
	     ctx_id = ecore_imf_context_default_id_by_canvas_type_get("evas");
	     if (ctx_id)
	       sd->imf_context = ecore_imf_context_add(ctx_id);
	     else
	       sd->imf_context = NULL;
	  }
     }
   else
     sd->imf_context = NULL;

   if (sd->imf_context)
     {
        ecore_imf_context_client_window_set(sd->imf_context,
					    ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
        ecore_imf_context_client_canvas_set(sd->imf_context, evas);
        ecore_imf_context_retrieve_surrounding_callback_set(sd->imf_context,
                                                            _e_entry_cb_imf_retrieve_surrounding,
                                                            sd);
        sd->imf_ee_commit_handler = ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT,
                                                            _e_entry_cb_imf_event_commit,
                                                            object);
        sd->imf_ee_delete_handler = ecore_event_handler_add(ECORE_IMF_EVENT_DELETE_SURROUNDING,
                                                            _e_entry_cb_imf_event_delete_surrounding,
                                                            sd);
     }
#endif

   sd->enabled = 1;
   sd->focused = 0;
   sd->selection_dragging = 0;
   sd->selection_mode = 0;
   sd->valign = 0.5;
   
   o = edje_object_add(evas);
   sd->entry_object = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/entry");
   evas_object_smart_member_add(o, object);
   
   o = e_editable_add(evas);
   sd->editable_object = o;
   e_editable_theme_set(o, "base/theme/widgets", "e/widgets/entry");
   e_editable_cursor_hide(o);
   e_editable_char_size_get(o, &cw, &ch);
   edje_extern_object_min_size_set(o, cw, ch);
   edje_object_part_swallow(sd->entry_object, "e.swallow.text", o);
   edje_object_size_min_calc(sd->entry_object, &sd->min_width, &sd->height);
   evas_object_show(o);
   
   evas_object_event_callback_add(object, EVAS_CALLBACK_KEY_DOWN,
                                  _e_entry_key_down_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_KEY_UP,
                                  _e_entry_key_up_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_entry_mouse_down_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_UP,
                                  _e_entry_mouse_up_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_MOVE,
                                  _e_entry_mouse_move_cb, NULL);
   sd->selection_handler = ecore_event_handler_add(
                                       ECORE_X_EVENT_SELECTION_NOTIFY,
                                       _e_entry_x_selection_notify_handler,
                                       object);
}

static void 
_e_entry_smart_del(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

#ifdef HAVE_ECORE_IMF
   if (sd->imf_context)
     {
	ecore_event_handler_del(sd->imf_ee_commit_handler);
	ecore_event_handler_del(sd->imf_ee_delete_handler);
	ecore_imf_context_del(sd->imf_context);
     }
#endif

   evas_object_event_callback_del(object, EVAS_CALLBACK_KEY_DOWN,
                                  _e_entry_key_down_cb);
   evas_object_event_callback_del(object, EVAS_CALLBACK_KEY_UP,
                                  _e_entry_key_up_cb);
   evas_object_event_callback_del(object, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_entry_mouse_down_cb);
   evas_object_event_callback_del(object, EVAS_CALLBACK_MOUSE_UP,
                                  _e_entry_mouse_up_cb);
   evas_object_event_callback_del(object, EVAS_CALLBACK_MOUSE_MOVE,
                                  _e_entry_mouse_move_cb);
   
   ecore_event_handler_del(sd->selection_handler);
   evas_object_del(sd->editable_object);
   evas_object_del(sd->entry_object);
   free(sd);
}

static void 
_e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Entry_Smart_Data *sd;
   Evas_Coord prev_x, prev_y;
   Evas_Coord ox, oy;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_geometry_get(object, &prev_x, &prev_y, NULL, NULL);
   evas_object_geometry_get(sd->entry_object, &ox, &oy, NULL, NULL);
   evas_object_move(sd->entry_object, ox + (x - prev_x), oy + (y - prev_y));
}

static void 
_e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Entry_Smart_Data *sd;
   Evas_Coord x, y;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_geometry_get(object, &x, &y, NULL, NULL);
   evas_object_move(sd->entry_object, x, y + ((h - sd->height) * sd->valign));
   evas_object_resize(sd->entry_object, w, sd->height);
}

static void 
_e_entry_smart_show(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_show(sd->entry_object);
}

static void 
_e_entry_smart_hide(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_hide(sd->entry_object);
}

static void 
_e_entry_color_set(Evas_Object *object, int r, int g, int b, int a)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_color_set(sd->entry_object, r, g, b, a);
}

static void 
_e_entry_clip_set(Evas_Object *object, Evas_Object *clip)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_set(sd->entry_object, clip);
}

static void 
_e_entry_clip_unset(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_unset(sd->entry_object);
}

static void 
_e_entry_cb_menu_post(void *data, E_Menu *m) 
{
   E_Entry_Smart_Data *sd;

   sd = data;
   if (!sd->popup) return;
   e_object_del(E_OBJECT(sd->popup));
   sd->popup = NULL;
}

static void 
_e_entry_cb_cut(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting, changed;
   char *range;
   E_Win *win;

   sd = data;
   if (!sd->enabled) return;
   
   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   if (!selecting) return;
   
   range = e_editable_text_range_get(editable, start_pos, end_pos);
   if (range)
     {
	if ((win = e_win_evas_object_win_get(sd->entry_object)))
	  ecore_x_selection_clipboard_set(win->evas_win,
					  range, strlen(range) + 1);
	free(range);
     }   
   changed = e_editable_delete(editable, start_pos, end_pos);
   if (changed)
     evas_object_smart_callback_call(sd->entry_object, "changed", NULL);
}

static void 
_e_entry_cb_copy(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   char *range;
   E_Win *win;

   sd = data;
   
   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   if (!selecting) return;
   
   range = e_editable_text_range_get(editable, start_pos, end_pos);
   if (range)
     {
	if ((win = e_win_evas_object_win_get(sd->entry_object)))
	  ecore_x_selection_clipboard_set(win->evas_win,
					  range, strlen(range) + 1);
	free(range);
     }
}

static void 
_e_entry_cb_paste(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Entry_Smart_Data *sd;
   E_Win *win;
   
   sd = data;
   if (!sd->enabled) return;
   
   if ((win = e_win_evas_object_win_get(sd->entry_object)))
     ecore_x_selection_clipboard_request(win->evas_win,
					 ECORE_X_SELECTION_TARGET_UTF8_STRING);
}

static void 
_e_entry_cb_select_all(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Entry_Smart_Data *sd;
   
   sd = data;

   e_editable_select_all(sd->editable_object);
   _e_entry_x_selection_update(sd->entry_object);
}

static void 
_e_entry_cb_delete(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   char *range;

   sd = data;
   if (!sd->enabled) return;
   
   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);
   if (!selecting) return;
   
   range = e_editable_text_range_get(editable, start_pos, end_pos);
   if (range)
     {
	e_editable_delete(editable, start_pos, end_pos);
	evas_object_smart_callback_call(sd->entry_object, "changed", NULL);
	free(range);
     }
}

#ifdef HAVE_ECORE_IMF
static int
 _e_entry_cb_imf_retrieve_surrounding(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos)
{
   E_Entry_Smart_Data *sd;

   sd = data;

   if (text)
     {
        const char *str;

        str = e_editable_text_get(sd->editable_object);
        *text = str ? strdup(str) : strdup("");
     }

   if (cursor_pos)
     *cursor_pos = e_editable_cursor_pos_get(sd->editable_object);

   return 1;
}

static int
_e_entry_cb_imf_event_commit(void *data, int type, void *event)
{
   Evas_Object *entry;
   E_Entry_Smart_Data *sd;
   Ecore_IMF_Event_Commit *ev = event;
   Evas_Object *editable;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   int changed = 0;

   if ((!(entry = data)) || (!(sd = evas_object_smart_data_get(entry))))
     return 1;

   if (sd->imf_context != ev->ctx)
     return 1;

   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   selection_pos = e_editable_selection_pos_get(editable);
   start_pos = (cursor_pos <= selection_pos) ? cursor_pos : selection_pos;
   end_pos = (cursor_pos >= selection_pos) ? cursor_pos : selection_pos;
   selecting = (start_pos != end_pos);

   if (selecting)
     changed |= e_editable_delete(editable, start_pos, end_pos);
   changed |= e_editable_insert(editable, start_pos, ev->str);

   if (changed)
     evas_object_smart_callback_call(entry, "changed", NULL);

   return 0;
}

static int
_e_entry_cb_imf_event_delete_surrounding(void *data, int type, void *event)
{
   E_Entry_Smart_Data *sd;
   Ecore_IMF_Event_Delete_Surrounding *ev = event;
   Evas_Object *editable;
   int cursor_pos;

   sd = data;

   if (sd->imf_context != ev->ctx)
     return 1;

   editable = sd->editable_object;
   cursor_pos = e_editable_cursor_pos_get(editable);
   e_editable_delete(editable,
                     cursor_pos + ev->offset,
                     cursor_pos + ev->offset + ev->n_chars);

   return 0;
}
#endif

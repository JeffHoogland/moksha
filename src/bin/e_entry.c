/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Entry_Smart_Data E_Entry_Smart_Data;

struct _E_Entry_Smart_Data
{
   Evas_Object *entry_object;
   Evas_Object *editable_object;
   
   int enabled;
   int focused;
   int selection_dragging;
   float valign;
   int min_width;
   int height;
};

/* local subsystem functions */
static void _e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_entry_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _e_entry_smart_add(Evas_Object *object);
static void _e_entry_smart_del(Evas_Object *object);
static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_entry_smart_show(Evas_Object *object);
static void _e_entry_smart_hide(Evas_Object *object);
static void _e_entry_color_set(Evas_Object *object, int r, int g, int b, int a);
static void _e_entry_clip_set(Evas_Object *object, Evas_Object *clip);
static void _e_entry_clip_unset(Evas_Object *object);

/* local subsystem globals */
static Evas_Smart *_e_entry_smart = NULL;
static int _e_entry_smart_use = 0;


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
        _e_entry_smart = evas_smart_new("e_entry",
				    _e_entry_smart_add, /* add */
				    _e_entry_smart_del, /* del */
				    NULL, NULL, NULL, NULL, NULL, /* stacking */
				    _e_entry_smart_move, /* move */
				    _e_entry_smart_resize, /* resize */
				    _e_entry_smart_show, /* show */
				    _e_entry_smart_hide, /* hide */
				    _e_entry_color_set, /* color_set */
				    _e_entry_clip_set, /* clip_set */
				    _e_entry_clip_unset, /* clip_unset */
				    NULL); /* data*/
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
   
   if (minw)   *minw = sd->min_width;
   if (minh)   *minh = sd->height;
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
   edje_object_signal_emit(sd->entry_object, "focus_in", "");
   if (sd->enabled)
      e_editable_cursor_show(sd->editable_object);
   e_editable_selection_show(sd->editable_object);
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
   edje_object_signal_emit(sd->entry_object, "focus_out", "");
   e_editable_cursor_move_to_end(sd->editable_object);
   e_editable_selection_move_to_end(sd->editable_object);
   e_editable_cursor_hide(sd->editable_object);
   e_editable_selection_hide(sd->editable_object);
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
   
   edje_object_signal_emit(entry, "enabled", "");
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
   
   edje_object_signal_emit(entry, "disabled", "");
   e_editable_cursor_hide(sd->editable_object);
   sd->enabled = 0;
}


/* Private functions */

/* Called when a key has been pressed by the user */
static void
_e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   Evas_Object *editable;
   Evas_Event_Key_Down *event;
   int cursor_pos, selection_pos;
   int start_pos, end_pos;
   int selecting;
   int changed = 0;
   char *range;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   if (!(event = event_info) || !(event->keyname))
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
          e_editable_cursor_move_left(editable);
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
          e_editable_cursor_move_right(editable);
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
     }
   /* Move the cursor/selection to the end of the entry */
   else if (strcmp(event->keyname, "End") == 0)
     {
        e_editable_cursor_move_to_end(editable);
        if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
          e_editable_selection_pos_set(editable,
                                       e_editable_cursor_pos_get(editable));
     }
   /* Remove the previous character */
   else if ((sd->enabled) && (strcmp(event->keyname, "BackSpace") == 0))
     {
        if (selecting)
          changed = e_editable_delete(editable, start_pos, end_pos);
        else
          changed = e_editable_delete(editable, cursor_pos - 1, cursor_pos);
     }
   /* Remove the next character */
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
          e_editable_select_all(editable);
        else if ((strcmp(event->keyname, "x") == 0) ||
                 (strcmp(event->keyname, "c") == 0))
          {
             if (selecting)
               {
                 range = e_editable_text_range_get(editable, start_pos, end_pos);
                 if (range)
                   {
                      //ecore_x_selection_clipboard_set();
                      free(range);
                   }
                 if ((sd->enabled) && (strcmp(event->keyname, "x") == 0))
                   changed = e_editable_delete(editable, start_pos, end_pos);
               }
           }
        else if (strcmp(event->keyname, "v") == 0)
        {
           //ecore_x_selection_clipboard_request();
        }
     }
   /* Otherwise, we insert the corresponding character */
   else if ((event->string) &&
          ((strlen(event->string) != 1) || (event->string[0] >= 0x20)))
     {
        if (selecting)
          changed = e_editable_delete(editable, start_pos, end_pos);
        changed |= e_editable_insert(editable, start_pos, event->string);
     }
   
   if (changed)
     evas_object_smart_callback_call(obj, "changed", NULL);
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
   
   evas_object_geometry_get(sd->editable_object, &ox, &oy, NULL, NULL);
   pos = e_editable_pos_get_from_coords(sd->editable_object,
                                        event->canvas.x - ox,
                                        event->canvas.y - oy);
   if (pos >= 0)
     {
        e_editable_cursor_pos_set(sd->editable_object, pos);
        if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
          e_editable_selection_pos_set(sd->editable_object, pos);
        
        sd->selection_dragging = 1;
     }
}

/* Called when the entry object is released by the mouse */
static void
_e_entry_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Entry_Smart_Data *sd;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   sd->selection_dragging = 0;
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
   
   if (sd->selection_dragging)
     {
        evas_object_geometry_get(sd->editable_object, &ox, &oy, NULL, NULL);
        pos = e_editable_pos_get_from_coords(sd->editable_object,
                                             event->cur.canvas.x - ox,
                                             event->cur.canvas.y - oy);
        if (pos >= 0)
          e_editable_cursor_pos_set(sd->editable_object, pos);
     }
}

/* Editable object's smart methods */

static void _e_entry_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Entry_Smart_Data *sd;
   Evas_Object *o;
   int cw, ch;
   
   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = malloc(sizeof(E_Entry_Smart_Data));
   if (!sd)
     return;
   
   evas_object_smart_data_set(object, sd);
   
   sd->enabled = 1;
   sd->focused = 0;
   sd->selection_dragging = 0;
   sd->valign = 0.5;
   
   o = edje_object_add(evas);
   sd->entry_object = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "widgets/entry");
   evas_object_smart_member_add(o, object);
   
   o = e_editable_add(evas);
   sd->editable_object = o;
   e_editable_theme_set(o, "base/theme/widgets", "widgets/entry");
   e_editable_cursor_hide(o);
   e_editable_char_size_get(o, &cw, &ch);
   edje_extern_object_min_size_set(o, cw, ch);
   edje_object_part_swallow(sd->entry_object, "text_area", o);
   edje_object_size_min_calc(sd->entry_object, &sd->min_width, &sd->height);
   evas_object_show(o);
   
   evas_object_event_callback_add(object, EVAS_CALLBACK_KEY_DOWN,
                                  _e_entry_key_down_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_entry_mouse_down_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_UP,
                                  _e_entry_mouse_up_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_MOVE,
                                  _e_entry_mouse_move_cb, NULL);
}

static void _e_entry_smart_del(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_del(sd->editable_object);
   evas_object_del(sd->entry_object);
}

static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
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

static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Entry_Smart_Data *sd;
   Evas_Coord x, y;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_geometry_get(object, &x, &y, NULL, NULL);
   evas_object_move(sd->entry_object, x, y + ((h - sd->height) * sd->valign));
   evas_object_resize(sd->entry_object, w, sd->height);
}

static void _e_entry_smart_show(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_show(sd->entry_object);
}

static void _e_entry_smart_hide(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_hide(sd->entry_object);
}

static void _e_entry_color_set(Evas_Object *object, int r, int g, int b, int a)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_color_set(sd->entry_object, r, g, b, a);
}

static void _e_entry_clip_set(Evas_Object *object, Evas_Object *clip)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_set(sd->entry_object, clip);
}

static void _e_entry_clip_unset(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   evas_object_clip_unset(sd->entry_object);
}

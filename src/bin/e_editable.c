/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define E_EDITABLE_CURSOR_SHOW_DELAY 1.25
#define E_EDITABLE_CURSOR_HIDE_DELAY 0.25
#define E_EDITABLE_CURSOR_MARGIN 5

#define E_EDITABLE_BLOCK_SIZE 128
#define E_EDITABLE_SIZE_TO_ALLOC(length) \
   (((length) + (E_EDITABLE_BLOCK_SIZE - 1)) / E_EDITABLE_BLOCK_SIZE) * E_EDITABLE_BLOCK_SIZE

typedef struct _E_Editable_Smart_Data E_Editable_Smart_Data;

struct _E_Editable_Smart_Data
{
   Evas_Object *clip_object;
   Evas_Object *event_object;
   Evas_Object *text_object;
   Evas_Object *cursor_object;
   Evas_Object *selection_object;

   Ecore_Timer *cursor_timer;
   int cursor_pos;
   int cursor_visible;
   int selection_pos;
   int selection_dragging;
   int selection_visible;
   int selectable;
   
   char *text;
   int char_length;
   int unicode_length;
   int allocated_length;
   
   char *font;
   int font_size;
   Evas_Text_Style_Type font_style;
   int average_char_w;
   int average_char_h;
};

/* local subsystem functions */
static int _e_editable_text_insert(Evas_Object *editable, int pos, const char *text);
static int _e_editable_text_delete(Evas_Object *editable, int start, int end);
static void _e_editable_cursor_update(Evas_Object *editable);
static void _e_editable_selection_update(Evas_Object *editable);
static void _e_editable_text_position_update(Evas_Object *editable);
static int _e_editable_cursor_pos_get_from_coords(Evas_Object *editable, Evas_Coord canvas_x, Evas_Coord canvas_y);

static int _e_editable_cursor_timer_cb(void *data);
static void _e_editable_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_editable_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_editable_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _e_editable_smart_add(Evas_Object *object);
static void _e_editable_smart_del(Evas_Object *object);
static void _e_editable_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_editable_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_editable_smart_show(Evas_Object *object);
static void _e_editable_smart_hide(Evas_Object *object);
static void _e_editable_color_set(Evas_Object *object, int r, int g, int b, int a);
static void _e_editable_clip_set(Evas_Object *object, Evas_Object *clip);
static void _e_editable_clip_unset(Evas_Object *object);

/* local subsystem globals */
static Evas_Smart *_e_editable_smart = NULL;
static int _e_editable_smart_use = 0;


/* externally accessible functions */

/**
 * Creates a new editable object. An editable object is an evas smart object in
 * which the user can type some single-line text, select it and delete it.
 *
 * @param evas The evas where to add the editable object
 * @return Returns the new editable object
 */
EAPI Evas_Object *
e_editable_add(Evas *evas)
{
   if (!_e_editable_smart)
     {
        _e_editable_smart = evas_smart_new("e_editable",
				    _e_editable_smart_add, /* add */
				    _e_editable_smart_del, /* del */
				    NULL, NULL, NULL, NULL, NULL, /* stacking */
				    _e_editable_smart_move, /* move */
				    _e_editable_smart_resize, /* resize */
				    _e_editable_smart_show, /* show */
				    _e_editable_smart_hide, /* hide */
				    _e_editable_color_set, /* color_set */
				    _e_editable_clip_set, /* clip_set */
				    _e_editable_clip_unset, /* clip_unset */
				    NULL); /* data*/
        _e_editable_smart_use = 0;
     }
   
   _e_editable_smart_use++;
   return evas_object_smart_add(evas, _e_editable_smart);
}

/**
 * Sets the text of the editable object
 *
 * @param editable an editable object
 * @param text the text to set
 */
EAPI void
e_editable_text_set(Evas_Object *editable, const char *text)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   free(sd->text);
   sd->text = NULL;
   sd->char_length = 0;
   sd->unicode_length = 0;
   sd->allocated_length = -1;
   
   if (_e_editable_text_insert(editable, 0, text) <= 0)
     {
        sd->text = malloc((E_EDITABLE_BLOCK_SIZE + 1) * sizeof(char));
        sd->text[0] = '\0';
        sd->char_length = 0;
        sd->unicode_length = 0;
        sd->allocated_length = E_EDITABLE_BLOCK_SIZE;
     }
   
   sd->cursor_pos = 0;
   sd->selection_pos = 0;
   _e_editable_cursor_update(editable);
}

/**
 * Gets the entire text of the editable object
 *
 * @param editable an editable object
 * @return Returns the entire text of the editable object
 */
EAPI const char *
e_editable_text_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return NULL;
   return sd->text;
}

/**
 * Gets a range of the text of the editable object, from position @a start to
 * position @a end
 *
 * @param editable an editable object
 * @param start the start position of the text range to get
 * @param end the end position of the text range to get
 * @return Returns the range of text. The returned string will have to be freed
 */
EAPI char *
e_editable_text_range_get(Evas_Object *editable, int start, int end)
{
   E_Editable_Smart_Data *sd;
   char *range;
   int start_id, end_id;
   int i;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return NULL;
   
   start = E_CLAMP(start, 0, sd->unicode_length);
   end = E_CLAMP(end, 0, sd->unicode_length);
   if (end <= start)
     return NULL;
   
   start_id = 0;
   end_id = 0;
   for (i = 0; i < end; i++)
     {
        end_id = evas_string_char_next_get(sd->text, end_id, NULL);
        if (i < start)
          start_id = end_id;
     }
   
   if (end_id <= start_id)
      return NULL;
   
   range = malloc((end_id - start_id + 1) * sizeof(char));
   strncpy(range, &sd->text[start_id], end_id - start_id);
   range[end_id - start_id] = '\0';
   
   return range;
}

/**
 * Gets the unicode length of the text of the entry. The unicode length is not
 * always the length returned by strlen() since a UTF-8 char can take several
 * bytes
 *
 * @param editable an editable object
 * @return Returns the unicode length of the text of the entry
 */
EAPI int
e_editable_text_length_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   return sd->unicode_length;
}

/**
 * Inserts some text at the given position in the editable object
 *
 * @param editable the editable object in which the text should be inserted
 * @param pos the position where to insert the text
 * @param text the text to insert
 * @return Returns 1 if the text has been modified, 0 otherwise
 */
EAPI int
e_editable_insert(Evas_Object *editable, int pos, const char *text)
{
   E_Editable_Smart_Data *sd;
   int unicode_length;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   
   unicode_length = _e_editable_text_insert(editable, pos, text);
   if (unicode_length <= 0)
     return 0;
   
   if (sd->cursor_pos >= pos)
     e_editable_cursor_pos_set(editable, sd->cursor_pos + unicode_length);
   if (sd->selection_pos >= pos)
     e_editable_selection_pos_set(editable, sd->selection_pos + unicode_length);
   
   _e_editable_text_position_update(editable);
   return 1;
}

/**
 * Deletes the text of the editable object, between position "start" and
 * position "end"
 *
 * @param editable the editable object in which the text should be deleted
 * @param start the position of the first char to delete
 * @param end the position of the last char to delete
 * @return Returns 1 if the text has been modified, 0 otherwise
 */
EAPI int
e_editable_delete(Evas_Object *editable, int start, int end)
{
   E_Editable_Smart_Data *sd;
   int unicode_length;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   
   unicode_length = _e_editable_text_delete(editable, start, end);
   if (unicode_length <= 0)
     return 0;
   
   if (sd->cursor_pos > end)
     e_editable_cursor_pos_set(editable, sd->cursor_pos - unicode_length);
   else if (sd->cursor_pos > start)
     e_editable_cursor_pos_set(editable, start);
   
   if (sd->selection_pos > end)
     e_editable_selection_pos_set(editable, sd->selection_pos - unicode_length);
   else if (sd->selection_pos > start)
     e_editable_selection_pos_set(editable, start);
   
   _e_editable_text_position_update(editable);
   return 1;
}

/**
 * Moves the cursor of the editable object to the given position
 *
 * @param editable an editable object
 * @param pos the position where to move the cursor
 */
EAPI void
e_editable_cursor_pos_set(Evas_Object *editable, int pos)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   pos = E_CLAMP(pos, 0, sd->unicode_length);
   if ((sd->cursor_pos == pos))
     return;
   
   sd->cursor_pos = pos;
   _e_editable_cursor_update(editable);
}

/**
 * Gets the position of the cursor of the editable object
 *
 * @param editable an editable object
 * @return Returns the position of the cursor of the editable object
 */
EAPI int
e_editable_cursor_pos_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   return sd->cursor_pos;
}

/**
 * Moves the cursor to the start of the editable object
 *
 * @param editable an editable object
 */
EAPI void
e_editable_cursor_move_to_start(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_cursor_pos_set(editable, 0);
}

/**
 * Moves the cursor to the end of the editable object
 *
 * @param editable an editable object
 */
EAPI void
e_editable_cursor_move_to_end(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_cursor_pos_set(editable, sd->unicode_length);
}

/**
 * Moves the cursor backward by one character offset
 *
 * @param editable an editable object
 */
EAPI void
e_editable_cursor_move_left(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_cursor_pos_set(editable, sd->cursor_pos - 1);
}

/**
 * Moves the cursor forward by one character offset
 *
 * @param editable an editable object
 */
EAPI void
e_editable_cursor_move_right(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_cursor_pos_set(editable, sd->cursor_pos + 1);
}

/**
 * Shows the cursor of the editable object
 *
 * @param editable the editable object whose cursor should be shown
 */
EAPI void
e_editable_cursor_show(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (sd->cursor_visible)
     return;
   
   sd->cursor_visible = 1;
   if (evas_object_visible_get(editable))
     {
        evas_object_show(sd->cursor_object);
        sd->cursor_timer = ecore_timer_add(E_EDITABLE_CURSOR_SHOW_DELAY,
                                           _e_editable_cursor_timer_cb,
                                           editable);
     }
}

/**
 * Hides the cursor of the editable object
 *
 * @param editable the editable object whose cursor should be hidden
 */
EAPI void
e_editable_cursor_hide(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!sd->cursor_visible)
     return;
   
   sd->cursor_visible = 0;
   if (evas_object_visible_get(editable))
     {
        evas_object_hide(sd->cursor_object);
        ecore_timer_del(sd->cursor_timer);
        sd->cursor_timer = NULL;
     }
}

/**
 * Moves the selection bound of the editable object to the given position
 *
 * @param editable an editable object
 * @param pos the position where to move the selection bound
 */
EAPI void
e_editable_selection_pos_set(Evas_Object *editable, int pos)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   pos = E_CLAMP(pos, 0, sd->unicode_length);
   if ((sd->selection_pos == pos))
     return;
   
   sd->selection_pos = pos;
   _e_editable_selection_update(editable);
}

/**
 * Gets the position of the selection bound of the editable object. If the
 * editable object is not selectable, this function returns the position of the
 * cursor instead.
 *
 * @param editable an editable object
 * @return Returns the position of the selection bound of the editable object
 */
EAPI int
e_editable_selection_pos_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   return sd->selectable ? sd->selection_pos : sd->cursor_pos;
}

/**
 * Moves the selection bound to the start of the editable object
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_move_to_start(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_selection_pos_set(editable, 0);
}

/**
 * Moves the selection bound to the end of the editable object
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_move_to_end(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_selection_pos_set(editable, sd->unicode_length);
}

/**
 * Moves the selection bound backward by one character offset
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_move_left(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_selection_pos_set(editable, sd->selection_pos - 1);
}

/**
 * Moves the selection bound forward by one character offset
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_move_right(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_selection_pos_set(editable, sd->selection_pos + 1);
}

/**
 * Selects all the text of the editable object. The selection bound will be
 * moved to the start of the editable object and the cursor will be moved to
 * the end
 *
 * @param editable an editable object
 */
EAPI void
e_editable_select_all(Evas_Object *editable)
{
   if (!editable)
      return;
   e_editable_selection_move_to_start(editable);
   e_editable_cursor_move_to_end(editable);
}

/**
 * Unselects all the text of the editable object. The selection bound will be
 * moved to the cursor position
 *
 * @param editable an editable object
 */
EAPI void
e_editable_unselect_all(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   e_editable_selection_pos_set(editable, sd->cursor_pos);
}

/**
 * Shows the selection of the editable object. The editable object need to be
 * selectable (see e_editable_selectable_set())
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_show(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (sd->selection_visible)
     return;
   
   sd->selection_visible = 1;
   if ((sd->selectable) && (evas_object_visible_get(editable)) &&
       (sd->cursor_pos != sd->selection_pos))
     evas_object_show(sd->selection_object);
}

/**
 * Hides the selection of the editable object
 *
 * @param editable an editable object
 */
EAPI void
e_editable_selection_hide(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!sd->selection_visible)
     return;
   
   sd->selection_visible = 0;
   evas_object_hide(sd->selection_object);
}

/**
 * Sets whether or not the editable object is selectable. If the editable object
 * is not selectable, the selection rectangle won't be shown, and
 * e_editable_selection_pos_get() will then return the position of the cursor
 *
 * @param editable an editable object
 * @param selection 1 to make the editable object selectable, 0 otherwise
 */
EAPI void
e_editable_selectable_set(Evas_Object *editable, int selectable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (sd->selectable == selectable)
     return;
   
   sd->selectable = selectable;
   
   if (sd->selectable)
     e_editable_unselect_all(editable);
   
   if ((sd->selectable) && (sd->selection_visible) &&
       (evas_object_visible_get(editable)) &&
       (sd->cursor_pos != sd->selection_pos))
     evas_object_show(sd->selection_object);
   else
     evas_object_hide(sd->selection_object);
}

/**
 * Gets whether or not the editable object is selectable
 *
 * @param editable an editable object
 * @return Returns 1 if the editable object is selectable, 0 otherwise
 */
EAPI int
e_editable_selectable_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   return sd->selectable;
}

/**
 * A utility function to get the average size of a character written inside
 * the editable object
 *
 * @param editable an editable object
 * @param w the location where to store the average width of a character
 * @param h the location where to store the average height of a character
 */
EAPI void
e_editable_char_size_get(Evas_Object *editable, int *w, int *h)
{
   int tw = 0, th = 0;
   Evas *evas;
   E_Editable_Smart_Data *sd;
   Evas_Object *text_object;
   char *text = "Tout est bon dans l'abricot sauf le noyau!"
                "Wakey wakey! Eggs and Bakey!";

   if (w)   *w = 0;
   if (h)   *h = 0;
   
   if ((!editable) || (!(evas = evas_object_evas_get(editable))))
     return;
   if ((!(sd = evas_object_smart_data_get(editable))) || (!sd->font))
     return;
   
   if ((sd->average_char_w <= 0) || (sd->average_char_h <= 0))
     {
        text_object = evas_object_text_add(evas);
        evas_object_text_font_set(text_object, sd->font, sd->font_size);
        evas_object_text_style_set(text_object, sd->font_style);
        evas_object_text_text_set(text_object, text);
        evas_object_geometry_get(text_object, NULL, NULL, &tw, &th);
        evas_object_del(text_object);
        
        sd->average_char_w = tw / strlen(text);
        sd->average_char_h = th;
     }
   
   if (w)   *w = sd->average_char_w;
   if (h)   *h = sd->average_char_h;
}

/* Private functions */

/* A utility function to insert some text inside the editable object.
 * It doesn't update the position of the cursor, nor the selection...
 */
static int
_e_editable_text_insert(Evas_Object *editable, int pos, const char *text)
{
   E_Editable_Smart_Data *sd;
   int char_length, unicode_length, prev_length;
   int index;
   int i;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   if ((!text) || (*text == '\0'))
     return 0;
   
   if (pos < 0)
     pos = 0;
   else if (pos > sd->unicode_length)
     pos = sd->unicode_length;
   
   char_length = -1;
   unicode_length = -1;
   for (i = 0; i != char_length; i = evas_string_char_next_get(text, i, NULL))
     {
        char_length = i;
        unicode_length++;
     }
   
   index = 0;
   for (i = 0; i < pos; i++)
     index = evas_string_char_next_get(sd->text, index, NULL);
   
   if ((unicode_length <= 0) || (char_length <= 0))
   if ((unicode_length <= 0) || (char_length <= 0))
     return 0;
   
   prev_length = sd->char_length;
   sd->char_length += char_length;
   sd->unicode_length += unicode_length;
   
   if (sd->char_length > sd->allocated_length)
     {
        sd->text = realloc(sd->text,
                           E_EDITABLE_SIZE_TO_ALLOC(sd->char_length) + 1);
        sd->allocated_length = E_EDITABLE_SIZE_TO_ALLOC(sd->char_length);
     }
   
   if (prev_length > index)
     memmove(&sd->text[index + char_length], &sd->text[index], prev_length - index);
   strncpy(&sd->text[index], text, char_length);
   sd->text[sd->char_length] = '\0';
   
   evas_object_text_text_set(sd->text_object, sd->text);
   
   return unicode_length;
}

/* A utility function to delete a range of text from the editable object.
 * It doesn't update the position of the cursor, nor the selection...
 */
static int
_e_editable_text_delete(Evas_Object *editable, int start, int end)
{
   E_Editable_Smart_Data *sd;
   int start_id, end_id;
   int i;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   
   start = E_CLAMP(start, 0, sd->unicode_length);
   end = E_CLAMP(end, 0, sd->unicode_length);
   if (end <= start)
     return 0;
   
   start_id = 0;
   end_id = 0;
   for (i = 0; i < end; i++)
     {
        end_id = evas_string_char_next_get(sd->text, end_id, NULL);
        if (i < start)
          start_id = end_id;
     }
   
   if (end_id <= start_id)
      return 0;
   
   memmove(&sd->text[start_id], &sd->text[end_id], sd->char_length - end_id);
   sd->char_length -= (end_id - start_id);
   sd->unicode_length -= (end - start);
   sd->text[sd->char_length] = '\0';
   
   evas_object_text_text_set(sd->text_object, sd->text);
   
   return end - start;
}

/* Updates the position of the cursor
 * It also updates automatically the text position and the selection
 */
static void
_e_editable_cursor_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord tx, ty;
   Evas_Coord cx, cy, cw, ch;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   evas_object_geometry_get(sd->text_object, &tx, &ty, NULL, NULL);
   
   if ((sd->unicode_length <= 0) || (sd->cursor_pos <= 0))
     {
        e_editable_char_size_get(editable, &cw, &ch);
        evas_object_move(sd->cursor_object, tx, ty);
        evas_object_resize(sd->cursor_object, 1, ch);
     }
   else
     {
        if (sd->cursor_pos >= sd->unicode_length)
          {
             evas_object_text_char_pos_get(sd->text_object, sd->unicode_length - 1,
                                          &cx, &cy, &cw, &ch);
             evas_object_move(sd->cursor_object, tx + cx + cw - 1, ty + cy);
             evas_object_resize(sd->cursor_object, 1, ch);
          }
        else
          {
             evas_object_text_char_pos_get(sd->text_object, sd->cursor_pos,
                                           &cx, &cy, &cw, &ch);
             evas_object_move(sd->cursor_object, tx + cx - 1, ty + cy);
             evas_object_resize(sd->cursor_object, 1, ch);
          }
     }
   
   if (sd->cursor_timer)
   {
      evas_object_show(sd->cursor_object);
        ecore_timer_interval_set(sd->cursor_timer, E_EDITABLE_CURSOR_SHOW_DELAY);
   }
   
   _e_editable_selection_update(editable);
   _e_editable_text_position_update(editable);
}

/* Updates the selection of the editable object */
static void
_e_editable_selection_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord tx, ty;
   Evas_Coord cx, cy, cw, ch;
   Evas_Coord sx, sy, sw, sh;
   int start_pos, end_pos;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   if ((sd->cursor_pos == sd->selection_pos) ||
      (!sd->selection_visible) || (!sd->selectable))
     evas_object_hide(sd->selection_object);
   else
     {
        evas_object_geometry_get(sd->text_object, &tx, &ty, NULL, NULL);
        
        start_pos = (sd->cursor_pos <= sd->selection_pos) ?
                    sd->cursor_pos : sd->selection_pos;
        end_pos = (sd->cursor_pos >= sd->selection_pos) ?
                  sd->cursor_pos : sd->selection_pos;
        
        /* Position of the start cursor (note, the start cursor can not be at
         * the end of the editable object, and the editable object can not be
         * empty, or it would have returned before)*/
        evas_object_text_char_pos_get(sd->text_object, start_pos,
                                      &cx, &cy, &cw, &ch);
        sx = tx + cx - 1;
        sy = ty + cy;
        
        /* Position of the end cursor (note, the editable object can not be
         * empty, or it would have returned before)*/
        if (end_pos >= sd->unicode_length)
          {
             evas_object_text_char_pos_get(sd->text_object, sd->unicode_length - 1,
                                          &cx, &cy, &cw, &ch);
             sw = (tx + cx + cw - 1) - sx;
             sh = ch;
          }
        else
          {
             evas_object_text_char_pos_get(sd->text_object, end_pos,
                                           &cx, &cy, &cw, &ch);
             sw = (tx + cx - 1) - sx;
             sh = ch;
          }
        
        evas_object_move(sd->selection_object, sx, sy);
        evas_object_resize(sd->selection_object, sw, sh);
        evas_object_show(sd->selection_object);
     }
}

/* Updates the position of the text according to the position of the cursor */
static void
_e_editable_text_position_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord ox, oy, ow;
   Evas_Coord tx, ty, tw;
   Evas_Coord cx, cy, cw;
   Evas_Coord sx, sy;
   Evas_Coord offset_x = 0;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   evas_object_geometry_get(editable, &ox, &oy, &ow, NULL);
   evas_object_geometry_get(sd->text_object, &tx, &ty, &tw, NULL);
   evas_object_geometry_get(sd->cursor_object, &cx, &cy, &cw, NULL);
   evas_object_geometry_get(sd->selection_object, &sx, &sy, NULL, NULL);
   
   if (tw <= ow)
     offset_x = ox - tx;
   else if (cx < (ox + E_EDITABLE_CURSOR_MARGIN))
     offset_x = ox + E_EDITABLE_CURSOR_MARGIN - cx;
   else if ((cx + cw + E_EDITABLE_CURSOR_MARGIN) > (ox + ow))
     offset_x = (ox + ow) - (cx + cw + E_EDITABLE_CURSOR_MARGIN);
     
   if (tw > ow)
     {
        if ((tx + offset_x) > ox)
          offset_x = ox - tx;
        else if ((tx + tw + offset_x) < (ox + ow))
          offset_x = (ox + ow) - (tx + tw);
     }
   
   if (offset_x != 0)
   {
      evas_object_move(sd->text_object, tx + offset_x, ty);
      evas_object_move(sd->cursor_object, cx + offset_x, cy);
      evas_object_move(sd->selection_object, sx + offset_x, sy);
   }
}

/* Returns the position where to place the cursor according to the given coords
 * Returns -1 on failure
 */
static int
_e_editable_cursor_pos_get_from_coords(Evas_Object *editable, Evas_Coord canvas_x, Evas_Coord canvas_y)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord tx, ty, tw, th;
   Evas_Coord cx, cw;
   int pos;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return -1;
   
   evas_object_geometry_get(sd->text_object, &tx, &ty, &tw, &th);
   
   if ((canvas_y < ty) || (canvas_x < tx))
      pos = 0;
   else if ((canvas_y > (ty + th)) || (canvas_x > (tx + tw)))
      pos = sd->unicode_length;
   else
     {
        pos = evas_object_text_char_coords_get(sd->text_object,
                                               canvas_x - tx, canvas_y - ty,
                                               &cx, NULL, &cw, NULL);
        if (pos >= 0)
          {
             if ((canvas_x - tx) > (cx + (cw / 2)))
               pos++;
             if (pos > sd->unicode_length)
               pos = sd->unicode_length;
          }
        else
          pos = -1;
     }
   
   return pos;
}

/* Shows/hides the cursor on regular interval */
static int
_e_editable_cursor_timer_cb(void *data)
{
   Evas_Object *editable;
   E_Editable_Smart_Data *sd;
   
   if ((!(editable = data)) || (!(sd = evas_object_smart_data_get(editable))))
     return 1;
   
   if (evas_object_visible_get(sd->cursor_object))
     {
        evas_object_hide(sd->cursor_object);
        ecore_timer_interval_set(sd->cursor_timer, E_EDITABLE_CURSOR_HIDE_DELAY);
     }
   else
     {
        evas_object_show(sd->cursor_object);
        ecore_timer_interval_set(sd->cursor_timer, E_EDITABLE_CURSOR_SHOW_DELAY);
     }
   
   return 1;
}

/* Called when the editable object is pressed by the mouse */
static void
_e_editable_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Object *editable;
   E_Editable_Smart_Data *sd;
   Evas_Event_Mouse_Down *event;
   Evas_Coord ty, th;
   int pos;
   
   if ((!(editable = obj)) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!(event = event_info))
     return;
   
   evas_object_geometry_get(sd->text_object, NULL, &ty, NULL, &th);
   if ((event->canvas.y < ty) || (event->canvas.y > (ty + th)))
     return;
   
   pos = _e_editable_cursor_pos_get_from_coords(editable, event->canvas.x,
                                                event->canvas.y);
   if (pos >= 0)
     {
        e_editable_cursor_pos_set(editable, pos);
        if (!evas_key_modifier_is_set(event->modifiers, "Shift"))
          e_editable_selection_pos_set(editable, pos);
        
        sd->selection_dragging = 1;
     }
}

/* Called when the editable object is released by the mouse */
static void
_e_editable_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Editable_Smart_Data *sd;
   
   if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))
     return;
   sd->selection_dragging = 0;
}

/* Called when the mouse moves over the editable object */
static void
_e_editable_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Object *editable;
   E_Editable_Smart_Data *sd;
   Evas_Event_Mouse_Move *event;
   int pos;
   
   if ((!(editable = obj)) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!(event = event_info))
     return;
   
   if (sd->selection_dragging)
     {
        pos = _e_editable_cursor_pos_get_from_coords(editable,
                                                     event->cur.canvas.x,
                                                     event->cur.canvas.y);
        if (pos >= 0)
          e_editable_cursor_pos_set(editable, pos);
     }
}

/* Editable object's smart methods */

static void
_e_editable_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Editable_Smart_Data *sd;
   
   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = malloc(sizeof(E_Editable_Smart_Data));
   if (!sd)
     return;
   
   evas_object_smart_data_set(object, sd);
   
   sd->text = malloc((E_EDITABLE_BLOCK_SIZE + 1) * sizeof(char));
   sd->text[0] = '\0';
   sd->char_length = 0;
   sd->unicode_length = 0;
   sd->allocated_length = E_EDITABLE_BLOCK_SIZE;
   
   /* TODO: themability! */
   sd->font = strdup("Vera");
   sd->font_size = 10;
   sd->font_style = EVAS_TEXT_STYLE_PLAIN;
   sd->average_char_w = 0;
   sd->average_char_h = 0;
   
   sd->cursor_timer = NULL;
   sd->cursor_pos = 0;
   sd->cursor_visible = 1;
   sd->selection_pos = 0;
   sd->selection_dragging = 0;
   sd->selection_visible = 1;
   sd->selectable = 1;

   sd->clip_object = evas_object_rectangle_add(evas);
   evas_object_smart_member_add(sd->clip_object, object);
   
   sd->event_object = evas_object_rectangle_add(evas);
   evas_object_color_set(sd->event_object, 255, 255, 255, 0);
   evas_object_clip_set(sd->event_object, sd->clip_object);
   evas_object_smart_member_add(sd->event_object, object);
   
   sd->text_object = evas_object_text_add(evas);
   evas_object_text_font_set(sd->text_object, sd->font, sd->font_size);
   evas_object_text_style_set(sd->text_object, sd->font_style);
   evas_object_color_set(sd->text_object, 0, 0, 0, 255);
   evas_object_clip_set(sd->text_object, sd->clip_object);
   evas_object_smart_member_add(sd->text_object, object);
   
   sd->cursor_object = evas_object_rectangle_add(evas);
   /* TODO: themability (and use edje for this?) ! */
   evas_object_color_set(sd->cursor_object, 0, 0, 0, 255);
   evas_object_clip_set(sd->cursor_object, sd->clip_object);
   evas_object_smart_member_add(sd->cursor_object, object);
   
   sd->selection_object = evas_object_rectangle_add(evas);
   /* TODO: themability (stackint too) ! */
   evas_object_color_set(sd->selection_object, 245, 205, 109, 102);
   evas_object_clip_set(sd->selection_object, sd->clip_object);
   evas_object_smart_member_add(sd->selection_object, object);
   
   _e_editable_cursor_update(object);
   
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_editable_mouse_down_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_UP,
                                  _e_editable_mouse_up_cb, NULL);
   evas_object_event_callback_add(object, EVAS_CALLBACK_MOUSE_MOVE,
                                  _e_editable_mouse_move_cb, NULL);
}

/* Deletes the editable */
static void
_e_editable_smart_del(Evas_Object *object)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   free(sd->font);
   
   evas_object_del(sd->clip_object);
   evas_object_del(sd->event_object);
   evas_object_del(sd->text_object);
   evas_object_del(sd->cursor_object);
   evas_object_del(sd->selection_object);
   if (sd->cursor_timer)
     ecore_timer_del(sd->cursor_timer);
   
   free(sd);
   
   _e_editable_smart_use--;
   if (_e_editable_smart_use <= 0)
     {
        evas_smart_free(_e_editable_smart);
        _e_editable_smart = NULL;
     }
}

/* Moves the editable object */
static void
_e_editable_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord prev_x, prev_y;
   Evas_Coord ox, oy;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   evas_object_geometry_get(object, &prev_x, &prev_y, NULL, NULL);
   
   evas_object_move(sd->clip_object, x, y);
   evas_object_move(sd->event_object, x, y);
   
   evas_object_geometry_get(sd->text_object, &ox, &oy, NULL, NULL);
   evas_object_move(sd->text_object, ox + (x - prev_x), oy + (y - prev_y));
   
   evas_object_geometry_get(sd->cursor_object, &ox, &oy, NULL, NULL);
   evas_object_move(sd->cursor_object, ox + (x - prev_x), oy + (y - prev_y));
   
   evas_object_geometry_get(sd->selection_object, &ox, &oy, NULL, NULL);
   evas_object_move(sd->selection_object, ox + (x - prev_x), oy + (y - prev_y));
}

/* Resizes the editable object */
static void
_e_editable_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   evas_object_resize(sd->clip_object, w, h);
   evas_object_resize(sd->event_object, w, h);
}

/* Shows the editable object */
static void
_e_editable_smart_show(Evas_Object *object)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   evas_object_show(sd->clip_object);
   evas_object_show(sd->event_object);
   evas_object_show(sd->text_object);
   
   if (sd->cursor_visible)
     {
        evas_object_show(sd->cursor_object);
        if (sd->cursor_timer)
          ecore_timer_interval_set(sd->cursor_timer, E_EDITABLE_CURSOR_SHOW_DELAY);
        else
          {
             sd->cursor_timer = ecore_timer_add(E_EDITABLE_CURSOR_SHOW_DELAY,
                                                _e_editable_cursor_timer_cb,
                                                object);
          }
     }
   
   if ((sd->selectable) && (sd->selection_visible) &&
      (sd->cursor_pos != sd->selection_pos))
     evas_object_show(sd->selection_object);
}

/* Hides the editable object */
static void
_e_editable_smart_hide(Evas_Object *object)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   evas_object_hide(sd->clip_object);
   evas_object_hide(sd->event_object);
   evas_object_hide(sd->text_object);
   evas_object_hide(sd->cursor_object);
   evas_object_hide(sd->selection_object);
   
   if (sd->cursor_timer)
     {
        ecore_timer_del(sd->cursor_timer);
        sd->cursor_timer = NULL;
     }
}

/* Changes the color of the editable object */
static void
_e_editable_color_set(Evas_Object *object, int r, int g, int b, int a)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   evas_object_color_set(sd->clip_object, r, g, b, a);
}

/* Clips the editable object against "clip" */
static void
_e_editable_clip_set(Evas_Object *object, Evas_Object *clip)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   evas_object_clip_set(sd->clip_object, clip);
}

/* Unclips the editable object */
static void
_e_editable_clip_unset(Evas_Object *object)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   evas_object_clip_unset(sd->clip_object);
}

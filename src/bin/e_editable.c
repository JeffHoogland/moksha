/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

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

   int cursor_pos;
   int cursor_visible;
   int selection_pos;
   int selection_visible;
   int password_mode;
   
   char *text;
   int char_length;
   int unicode_length;
   int allocated_length;
   
   int cursor_width;
   int selection_on_fg;
   int average_char_w;
   int average_char_h;
};

/* local subsystem functions */
static int _e_editable_text_insert(Evas_Object *editable, int pos, const char *text);
static int _e_editable_text_delete(Evas_Object *editable, int start, int end);
static void _e_editable_cursor_update(Evas_Object *editable);
static void _e_editable_selection_update(Evas_Object *editable);
static void _e_editable_text_update(Evas_Object *editable);
static void _e_editable_text_position_update(Evas_Object *editable, Evas_Coord real_w);

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
   
   return evas_object_smart_add(evas, _e_editable_smart);
}

/**
 * Sets the theme group to be used by the editable object.
 * This function has to be called, or the cursor and the selection won't be
 * visible.
 *
 * @param editable an editable object
 * @param category the theme category to use for the editable object
 * @param group the theme group to use for the editable object
 */
EAPI void
e_editable_theme_set(Evas_Object *editable, const char *category, const char *group)
{
   E_Editable_Smart_Data *sd;
   char *obj_group;
   const char *data;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if ((!category) || (!group))
     return;
   
   /* Gets the theme for the text object */
   obj_group = malloc(strlen(group) + strlen("/text") + 1);
   sprintf(obj_group, "%s/text", group);
   e_theme_edje_object_set(sd->text_object, category, obj_group);
   free(obj_group);
   sd->average_char_w = -1;
   sd->average_char_h = -1;
   
   
   /* Gets the theme for the cursor */
   obj_group = malloc(strlen(group) + strlen("/cursor") + 1);
   sprintf(obj_group, "%s/cursor", group);
   e_theme_edje_object_set(sd->cursor_object, category, obj_group);
   free(obj_group);
   
   edje_object_size_min_get(sd->cursor_object, &sd->cursor_width, NULL);
   if (sd->cursor_width < 1)
     sd->cursor_width = 1;
   
   
   /* Gets the theme for the selection */
   obj_group = malloc(strlen(group) + strlen("/selection") + 1);
   sprintf(obj_group, "%s/selection", group);
   e_theme_edje_object_set(sd->selection_object, category, obj_group);
   free(obj_group);
   
   data = edje_object_data_get(sd->selection_object, "on_foreground");
   if ((data) && (strcmp(data, "1") == 0))
     {
        sd->selection_on_fg = 1;
        evas_object_stack_above(sd->selection_object, sd->text_object);
     }
   else
     {
        sd->selection_on_fg = 0;
        evas_object_stack_below(sd->selection_object, sd->text_object);
     }
   
   _e_editable_text_update(editable);
   _e_editable_cursor_update(editable);
}

/**
 * Sets whether or not the editable object is in password mode. In password
 * mode, the editable object displays '*' instead of the characters
 *
 * @param editable an editable object
 * @param password_mode 1 to turn on the password mode of the editable object,
 * 0 to turn it off
 */
EAPI void
e_editable_password_set(Evas_Object *editable, int password_mode)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (sd->password_mode == password_mode)
     return;
   
   sd->password_mode = password_mode;
   _e_editable_text_update(editable);
   _e_editable_cursor_update(editable);
}

/**
 * Gets whether or not the editable is in password mode
 *
 * @param editable an editable object
 * @return Returns 1 if the editable object is in the password mode, 0 otherwise
 */
EAPI int
e_editable_password_get(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return 0;
   return sd->password_mode;
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
   
   sd->cursor_pos = sd->unicode_length;
   sd->selection_pos = sd->unicode_length;
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
 * Gets the unicode length of the text of the editable object. The unicode
 * length is not always the length returned by strlen() since a UTF-8 char can
 * take several bytes
 *
 * @param editable an editable object
 * @return Returns the unicode length of the text of the editable object
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
   
   _e_editable_text_position_update(editable, -1);
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
   
   _e_editable_text_position_update(editable, -1);
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
        edje_object_signal_emit(sd->cursor_object, "cursor_show", "");
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
   evas_object_hide(sd->cursor_object);
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
 * Gets the position of the selection bound of the editable object
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
   return sd->selection_pos;
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
 * Shows the selection of the editable object
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
   if ((evas_object_visible_get(editable)) &&
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
 * Gets the cursor position at the coords ( @a x, @a y ). It's used to know
 * where to place the cursor or the selection bound on mouse evevents.
 *
 * @param editable an editable object
 * @param x the x coord, relative to the editable object
 * @param y the y coord, relative to the editable object
 * @return Returns the position where to place the cursor according to the
 * given coords, or -1 on failure
 */
EAPI int
e_editable_pos_get_from_coords(Evas_Object *editable, Evas_Coord x, Evas_Coord y)
{
   E_Editable_Smart_Data *sd;
   Evas_Object *text_obj;
   Evas_Coord ox, oy;
   Evas_Coord tx, ty, tw, th;
   Evas_Coord cx, cw;
   Evas_Coord canvas_x, canvas_y;
   int pos;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return -1;
   if (!(text_obj = edje_object_part_object_get(sd->text_object, "text")))
     return -1;
   
   evas_object_geometry_get(editable, &ox, &oy, NULL, NULL);
   evas_object_geometry_get(text_obj, &tx, &ty, &tw, &th);
   canvas_x = ox + x;
   canvas_y = oy + y;
   
   if ((canvas_y < ty) || (canvas_x < tx))
      pos = 0;
   else if ((canvas_y > (ty + th)) || (canvas_x > (tx + tw)))
      pos = sd->unicode_length;
   else
     {
        pos = evas_object_text_char_coords_get(text_obj,
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
   Evas_Object *text_obj;
   E_Editable_Smart_Data *sd;
   char *text = "Tout est bon dans l'abricot sauf le noyau!"
                "Wakey wakey! Eggs and Bakey!";
   const char *font, *font_source;
   Evas_Text_Style_Type style;
   int font_size;

   if (w)   *w = 0;
   if (h)   *h = 0;
   
   if ((!editable) || (!(evas = evas_object_evas_get(editable))))
     return;
   if (!(sd = evas_object_smart_data_get(editable)))
     return;
   if (!(text_obj = edje_object_part_object_get(sd->text_object, "text")))
     return;
   
   if ((sd->average_char_w <= 0) || (sd->average_char_h <= 0))
     {
        font_source = evas_object_text_font_source_get(text_obj);
        evas_object_text_font_get(text_obj, &font, &font_size);
        style = evas_object_text_style_get(text_obj);
        
        text_obj = evas_object_text_add(evas);
        evas_object_text_font_source_set(text_obj, font_source);
        evas_object_text_font_set(text_obj, font, font_size);
        evas_object_text_style_set(text_obj, style);
        evas_object_text_text_set(text_obj, text);
        evas_object_geometry_get(text_obj, NULL, NULL, &tw, &th);
        evas_object_del(text_obj);
        
        sd->average_char_w = tw / strlen(text);
        sd->average_char_h = th;
     }
   
   if (w)   *w = sd->average_char_w;
   if (h)   *h = sd->average_char_h;
}

/* Private functions */

/* A utility function to insert some text inside the editable object.
 * It doesn't update the position of the cursor, nor the selection... */
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
   
   _e_editable_text_update(editable);
   
   return unicode_length;
}

/* A utility function to delete a range of text from the editable object.
 * It doesn't update the position of the cursor, nor the selection... */
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
   
   _e_editable_text_update(editable);
   
   return end - start;
}

/* Updates the position of the cursor
 * It also updates automatically the text position and the selection */
static void
_e_editable_cursor_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Object *text_obj;
   Evas_Coord tx, ty;
   Evas_Coord cx, cy, cw, ch;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!(text_obj = edje_object_part_object_get(sd->text_object, "text")))
     return;
      
   evas_object_geometry_get(text_obj, &tx, &ty, NULL, NULL);
   
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
             evas_object_text_char_pos_get(text_obj, sd->unicode_length - 1,
                                          &cx, &cy, &cw, &ch);
             evas_object_move(sd->cursor_object, tx + cx + cw - 1, ty + cy);
             evas_object_resize(sd->cursor_object, 1, ch);
          }
        else
          {
             evas_object_text_char_pos_get(text_obj, sd->cursor_pos,
                                           &cx, &cy, &cw, &ch);
             evas_object_move(sd->cursor_object, tx + cx - 1, ty + cy);
             evas_object_resize(sd->cursor_object, 1, ch);
          }
     }
   
   if (sd->cursor_visible && evas_object_visible_get(editable))
     {
        evas_object_show(sd->cursor_object);
        edje_object_signal_emit(sd->cursor_object, "cursor_show", "");
     }
   
   _e_editable_selection_update(editable);
   _e_editable_text_position_update(editable, -1);
}

/* Updates the selection of the editable object */
static void
_e_editable_selection_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Object *text_obj;
   Evas_Coord tx, ty;
   Evas_Coord cx, cy, cw, ch;
   Evas_Coord sx, sy, sw, sh;
   int start_pos, end_pos;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   if (!(text_obj = edje_object_part_object_get(sd->text_object, "text")))
     return;
   
   if ((sd->cursor_pos == sd->selection_pos) || (!sd->selection_visible))
     evas_object_hide(sd->selection_object);
   else
     {
        evas_object_geometry_get(text_obj, &tx, &ty, NULL, NULL);
        
        start_pos = (sd->cursor_pos <= sd->selection_pos) ?
                    sd->cursor_pos : sd->selection_pos;
        end_pos = (sd->cursor_pos >= sd->selection_pos) ?
                  sd->cursor_pos : sd->selection_pos;
        
        /* Position of the start cursor (note, the start cursor can not be at
         * the end of the editable object, and the editable object can not be
         * empty, or it would have returned before)*/
        if (!evas_object_text_char_pos_get(text_obj, start_pos,
                                           &cx, &cy, &cw, &ch))
          return;
        
        sx = tx + cx - 1;
        sy = ty + cy;
        
        
        /* Position of the end cursor (note, the editable object can not be
         * empty, or it would have returned before)*/
        if (end_pos >= sd->unicode_length)
          {
             if (!evas_object_text_char_pos_get(text_obj,
                                                sd->unicode_length - 1,
                                                &cx, &cy, &cw, &ch))
               return;
             
             sw = (tx + cx + cw - 1) - sx;
             sh = ch;
          }
        else
          {
             if (!evas_object_text_char_pos_get(text_obj, end_pos,
                                                &cx, &cy, &cw, &ch))
               return;
             
             sw = (tx + cx - 1) - sx;
             sh = ch;
          }
        
        evas_object_move(sd->selection_object, sx, sy);
        evas_object_resize(sd->selection_object, sw, sh);
        evas_object_show(sd->selection_object);
     }
}

/* Updates the text of the text object of the editable object 
 * (it fills it with '*' if the editable is in password mode)
 * It does not update the position of the text */
static void
_e_editable_text_update(Evas_Object *editable)
{
   E_Editable_Smart_Data *sd;
   Evas_Coord minw, minh;
   
   if ((!editable) || (!(sd = evas_object_smart_data_get(editable))))
     return;
   
   if (sd->password_mode)
     {
        char *text;
        
        text = malloc((sd->unicode_length + 1) * sizeof(char));
        memset(text, '*', sd->unicode_length * sizeof(char));
        text[sd->unicode_length] = '\0';
        edje_object_part_text_set(sd->text_object, "text", text);
        free(text);
     }
   else
     {
        edje_object_part_text_set(sd->text_object, "text",
                                  sd->text ? sd->text : "");
     }
   
   edje_object_size_min_calc(sd->text_object, &minw, &minh);
   evas_object_resize(sd->text_object, minw, minh);
}

/* Updates the position of the text object according to the position of the
 * cursor (we make sure the cursor is visible) */
static void
_e_editable_text_position_update(Evas_Object *editable, Evas_Coord real_w)
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
   if (real_w >= 0)
      ow = real_w;
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


/* Editable object's smart methods */

static void
_e_editable_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Editable_Smart_Data *sd;
   Evas_Coord ox, oy;
   
   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = malloc(sizeof(E_Editable_Smart_Data));
   if (!sd)
     return;
   
   _e_editable_smart_use++;
   evas_object_smart_data_set(object, sd);
   evas_object_geometry_get(object, &ox, &oy, NULL, NULL);
   
   sd->text = malloc((E_EDITABLE_BLOCK_SIZE + 1) * sizeof(char));
   sd->text[0] = '\0';
   sd->char_length = 0;
   sd->unicode_length = 0;
   sd->allocated_length = E_EDITABLE_BLOCK_SIZE;
   
   sd->cursor_width = 1;
   sd->selection_on_fg = 0;
   sd->average_char_w = -1;
   sd->average_char_h = -1;
   
   sd->cursor_pos = 0;
   sd->cursor_visible = 1;
   sd->selection_pos = 0;
   sd->selection_visible = 1;
   sd->password_mode = 0;

   sd->clip_object = evas_object_rectangle_add(evas);
   evas_object_move(sd->clip_object, ox, oy);
   evas_object_smart_member_add(sd->clip_object, object);
   
   sd->event_object = evas_object_rectangle_add(evas);
   evas_object_color_set(sd->event_object, 255, 255, 255, 0);
   evas_object_clip_set(sd->event_object, sd->clip_object);
   evas_object_move(sd->event_object, ox, oy);
   evas_object_smart_member_add(sd->event_object, object);
   
   sd->text_object = edje_object_add(evas);
   evas_object_pass_events_set(sd->text_object, 1);
   evas_object_clip_set(sd->text_object, sd->clip_object);
   evas_object_move(sd->text_object, ox, oy);
   evas_object_smart_member_add(sd->text_object, object);
   
   sd->selection_object = edje_object_add(evas);
   evas_object_pass_events_set(sd->selection_object, 1);
   evas_object_clip_set(sd->selection_object, sd->clip_object);
   evas_object_move(sd->selection_object, ox, oy);
   evas_object_smart_member_add(sd->selection_object, object);
   
   sd->cursor_object = edje_object_add(evas);
   evas_object_pass_events_set(sd->cursor_object, 1);
   evas_object_clip_set(sd->cursor_object, sd->clip_object);
   evas_object_move(sd->cursor_object, ox, oy);
   evas_object_smart_member_add(sd->cursor_object, object);
   
   _e_editable_cursor_update(object);
}

/* Deletes the editable */
static void
_e_editable_smart_del(Evas_Object *object)
{
   E_Editable_Smart_Data *sd;
   
   if ((!object) || (!(sd = evas_object_smart_data_get(object))))
     return;
   
   evas_object_del(sd->clip_object);
   evas_object_del(sd->event_object);
   evas_object_del(sd->text_object);
   evas_object_del(sd->cursor_object);
   evas_object_del(sd->selection_object);
   free(sd->text);
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
   _e_editable_text_position_update(object, w);
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
        edje_object_signal_emit(sd->cursor_object, "cursor_show", "");
     }
   
   if ((sd->selection_visible) && (sd->cursor_pos != sd->selection_pos))
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

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO:
 * - implement missing _e_entry_smart_*, very easy
 * - free / delete properly
 * - implement focus and interact with theme
 */

typedef struct _E_Editable_Text_Smart_Data E_Editable_Text_Smart_Data;
typedef struct _E_Entry_Smart_Data         E_Entry_Smart_Data;

struct _E_Editable_Text_Smart_Data
{
   Evas_Object *clip;
   Evas_Object *text_object;
   Evas_Object *cursor_object;
   Evas_Object *edje_object;
   Ecore_Timer *cursor_timer;

   Evas_Bool cursor_at_the_end;
   Evas_Bool show_cursor;   
};

struct _E_Entry_Smart_Data
{
   Evas_Object *entry_object;
   Evas_Object *edje_object;
   
   void (*change_func) (void *data, Evas_Object *entry, char *key);
   void  *change_data;   
};

static Evas_Bool _e_editable_text_is_empty(Evas_Object *object);
static void      _e_editable_text_cursor_position_update(Evas_Object *object);
static void      _e_editable_text_cursor_visibility_update(Evas_Object *object);
static int       _e_editable_text_cursor_timer_cb(void *data);
static void      _e_editable_text_size_update(Evas_Object *object);

static void      _e_editable_text_smart_add(Evas_Object *object);
static void      _e_editable_text_smart_del(Evas_Object *object);
static void      _e_editable_text_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void      _e_editable_text_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void      _e_editable_text_smart_show(Evas_Object *object);
static void      _e_editable_text_smart_hide(Evas_Object *object);

static void      _e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event);

static void      _e_entry_smart_add(Evas_Object *object);
static void      _e_entry_smart_del(Evas_Object *object);
static void      _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void      _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void      _e_entry_smart_show(Evas_Object *object);
static void      _e_entry_smart_hide(Evas_Object *object);

static Evas_Smart           *e_editable_text_smart = NULL;
static Evas_Textblock_Style *e_editable_text_style = NULL;
static int                   e_editable_text_style_use_count = 0;

static Evas_Smart           *e_entry_smart = NULL;

EAPI Evas_Object *
e_editable_text_add(Evas *evas)
{
   if (!e_editable_text_smart)
     {
	e_editable_text_smart = evas_smart_new("e_editable_entry",
					       _e_editable_text_smart_add, /* add */
					       _e_editable_text_smart_del, /* del */
					       NULL, NULL, NULL, NULL, NULL,
					       _e_editable_text_smart_move, /* move */
					       _e_editable_text_smart_resize, /* resize */
					       _e_editable_text_smart_show, /* show */
					       _e_editable_text_smart_hide, /* hide */
					       NULL, /* color_set */
					       NULL, /* clip_set */
					       NULL, /* clip_unset */
					       NULL); /* data*/
     }
   return evas_object_smart_add(evas, e_editable_text_smart);
}

EAPI const char*
e_editable_text_text_get(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return "";

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   return evas_textblock_cursor_node_text_get(cursor);
}

/**
 * @brief Sets the text of the object
 * @param object an editable text object
 * @param text the text to set
 */
EAPI void
e_editable_text_text_set(Evas_Object *object, const char *text)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || (!text) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_textblock_text_markup_set(sd->text_object, text);
   sd->cursor_at_the_end = 1;
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Inserts text at the cursor position of the object
 * @param object an editable text object
 * @param text the text to insert
 */
EAPI void
e_editable_text_insert(Evas_Object *object, const char *text)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   if ((!text) || ((strlen(text) <= 1) && (text[0] < 0x20)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);

   if (sd->cursor_at_the_end)
     evas_textblock_cursor_text_append(cursor, text);
   else
     evas_textblock_cursor_text_prepend(cursor, text);
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Deletes the char placed before the cursor
 * @param object an editable text object
 */
EAPI void
e_editable_text_delete_char_before(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) ||
       (!(sd = evas_object_smart_data_get(object))) ||
       (_e_editable_text_is_empty(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);

   if ((sd->cursor_at_the_end) || (evas_textblock_cursor_char_prev(cursor)))
     evas_textblock_cursor_char_delete(cursor);

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Deletes the char placed after the cursor
 * @param object an editable text object
 */
EAPI void
e_editable_text_delete_char_after(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) ||
       (!(sd = evas_object_smart_data_get(object))) ||
       (_e_editable_text_is_empty(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);

   if (!sd->cursor_at_the_end)
     {
	if (!evas_textblock_cursor_char_next(cursor))
	  sd->cursor_at_the_end = 1;
	else
	  evas_textblock_cursor_char_prev(cursor);
	evas_textblock_cursor_char_delete(cursor);
     }

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object at the start of the text
 * @param object an editable text object
 */
EAPI void
e_editable_text_cursor_move_at_start(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) ||
       (!(sd = evas_object_smart_data_get(object))) ||
       (_e_editable_text_is_empty(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   sd->cursor_at_the_end = 0;
   evas_textblock_cursor_char_first(cursor);

   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object at the end of the text
 * @param object an editable text object
 */
EAPI void
e_editable_text_cursor_move_at_end(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) ||
       (!(sd = evas_object_smart_data_get(object))) ||
       (_e_editable_text_is_empty(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   sd->cursor_at_the_end = 1;
   evas_textblock_cursor_char_last(cursor);

   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object to the left
 * @param object an editable text object
 */
EAPI void
e_editable_text_cursor_move_left(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   if (sd->cursor_at_the_end)
     sd->cursor_at_the_end = 0;
   else
     evas_textblock_cursor_char_prev(cursor);

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object to the right
 * @param object an editable text object
 */
EAPI void
e_editable_text_cursor_move_right(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   if (!evas_textblock_cursor_char_next(cursor))
     sd->cursor_at_the_end = 1;

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Shows the cursor of the editable text object
 * @param object the editable text object
 */
EAPI void
e_editable_text_cursor_show(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   sd->show_cursor = 1;
   _e_editable_text_cursor_visibility_update(object);
}

/**
 * @brief Hides the cursor of the editable text object
 * @param object the editable text object
 */
EAPI void
e_editable_text_cursor_hide(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   sd->show_cursor = 0;
   _e_editable_text_cursor_visibility_update(object);
}

EAPI Evas_Object *
e_entry_add(Evas *evas)
{
   if(!e_entry_smart)
     e_entry_smart = evas_smart_new("e_entry",
				    _e_entry_smart_add, /* add */
				    _e_entry_smart_del, /* del */
				    NULL, NULL, NULL, NULL, NULL,
				    _e_entry_smart_move, /* move */
				    _e_entry_smart_resize, /* resize */
				    _e_entry_smart_show, /* show */
				    _e_entry_smart_hide, /* hide */
				    NULL, /* color_set */
				    NULL, /* clip_set */
				    NULL, /* clip_unset */
				    NULL); /* data*/   
   return evas_object_smart_add(evas, e_entry_smart);
}

EAPI void
e_entry_change_handler_set(Evas_Object *object, void (*func)(void *data, Evas_Object *entry, char *key), void *data)
{
   E_Entry_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   sd->change_func = func;
   sd->change_data = data;
}

EAPI void
e_entry_text_set(Evas_Object *entry, const char *text)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_text_set(sd->entry_object, text);
}

EAPI const char*
e_entry_text_get(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return "";

   return e_editable_text_text_get(sd->entry_object);
}

EAPI void
e_entry_text_insert(Evas_Object *entry, const char *text)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_insert(sd->entry_object, text);
}

EAPI void
e_entry_delete_char_before(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_delete_char_before(sd->entry_object);
}

EAPI void
e_entry_delete_char_after(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_delete_char_after(sd->entry_object);
}

EAPI void
e_entry_cursor_move_at_start(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_move_at_start(sd->entry_object);
}

EAPI void
e_entry_cursor_move_at_end(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_move_at_end(sd->entry_object);
}

EAPI void
e_entry_cursor_move_left(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_move_left(sd->entry_object);
}

EAPI void
e_entry_cursor_move_right(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_move_right(sd->entry_object);
}

EAPI void
e_entry_cursor_show(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_show(sd->entry_object);
}

EAPI void
e_entry_cursor_hide(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;

   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;

   e_editable_text_cursor_hide(sd->entry_object);
}

EAPI void
e_entry_focus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;
   
   edje_object_signal_emit(sd->edje_object, "focus_in", "");
}

EAPI void
e_entry_unfocus(Evas_Object *entry)
{
   E_Entry_Smart_Data *sd;
   
   if ((!entry) || !(sd = evas_object_smart_data_get(entry)))
     return;
   
   edje_object_signal_emit(sd->edje_object, "focus_out", "");
}

/**************************
 *
 * Private functions
 *
 **************************/

/* Returns 1 if the text of the editable object is empty */
static Evas_Bool
_e_editable_text_is_empty(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return 1;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   return (evas_textblock_cursor_node_text_length_get(cursor) <= 0);
}

/* Updates the size of the text object: to be called when the text of the object is changed */
static void
_e_editable_text_size_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   int w, h;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_textblock_size_native_get(sd->text_object, &w, &h);
   evas_object_resize(sd->text_object, w, h);
}

/* Updates the cursor position: to be called when the cursor or the object are moved */
static void
_e_editable_text_cursor_position_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;
   Evas_Coord tx, ty, tw, th, cx, cy, cw, ch, ox, oy, ow, oh;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_geometry_get(sd->text_object, &tx, &ty, &tw, &th);
   evas_object_geometry_get(object, &ox, &oy, &ow, &oh);
   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);

   if (_e_editable_text_is_empty(object))
     {
	evas_object_move(sd->cursor_object, tx, ty);
	evas_object_resize(sd->cursor_object, 1, oh);
	return;
     }
   else if (sd->cursor_at_the_end)
     {
	evas_textblock_cursor_char_last(cursor);
	evas_textblock_cursor_char_geometry_get(cursor, &cx, &cy, &cw, &ch);
	cx += tx + cw;
     }
   else
     {
	evas_textblock_cursor_char_geometry_get(cursor, &cx, &cy, &cw, &ch);
	cx += tx;
     }

   /* TODO: fix */
   if ((cx < ox + 20) && (cx - tx > 20))
     {
	evas_object_move(sd->text_object, tx + ox + 20 - cx, ty);
	cx = ox + 20;
     }
   else if (cx < ox)
     {	
	evas_object_move(sd->text_object, tx + ox - cx, ty);
	cx = ox;
     }
   else if ((cx > ox + ow - 20) && (tw - (cx - tx) > 20))
     {
	evas_object_move(sd->text_object, tx - cx + ox + ow - 20, ty);
	cx = ox + ow - 20;
     }
   else if (cx > ox + ow)
     {
	evas_object_move(sd->text_object, tx - cx + ox + ow, ty);
	cx = ox + ow;
     }

   evas_object_move(sd->cursor_object, cx, ty + cy);
   evas_object_resize(sd->cursor_object, 1, ch);   
}

/* Updates the visibility state of the cursor */
static void
_e_editable_text_cursor_visibility_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   if (!sd->show_cursor)
     {
	if (sd->cursor_timer)
	  {
	     ecore_timer_del(sd->cursor_timer);
	     sd->cursor_timer = NULL;
	  }
	evas_object_hide(sd->cursor_object);
     }
   else
     {
	if (!sd->cursor_timer)
	  {
	     sd->cursor_timer = ecore_timer_add(0.75, _e_editable_text_cursor_timer_cb, object);
	     evas_object_show(sd->cursor_object);
	  }
     }
}

/* Make the cursor blink */
// FIXME: cursor should not be a rect - shoudl be an edje. timers not needed then
static int
_e_editable_text_cursor_timer_cb(void *data)
{
   Evas_Object *object;
   E_Editable_Text_Smart_Data *sd;

   object = data;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return 1;

   if (evas_object_visible_get(sd->cursor_object))
     {
	evas_object_hide(sd->cursor_object);
	ecore_timer_interval_set(sd->cursor_timer, 0.25);
     }
   else
     {
	evas_object_show(sd->cursor_object);
	ecore_timer_interval_set(sd->cursor_timer, 0.75);
     }

   return 1;
}

/* Called when the object is created */
static void
_e_editable_text_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Editable_Text_Smart_Data *sd;
   Evas_Textblock_Cursor *cursor;

   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   if (!e_editable_text_style)
     {
	e_editable_text_style = evas_textblock_style_new();
	evas_textblock_style_set(e_editable_text_style, "DEFAULT='font=Vera font_size=10 align=left color=#000000 wrap=word'");
	e_editable_text_style_use_count = 0;
     }

   sd = malloc(sizeof(E_Editable_Text_Smart_Data));
   if (!sd) return;
   sd->show_cursor = 0;
   sd->cursor_at_the_end = 1;
   sd->cursor_timer = NULL;

   sd->text_object = evas_object_textblock_add(evas);
   evas_object_textblock_style_set(sd->text_object, e_editable_text_style);
   e_editable_text_style_use_count++;
   evas_object_smart_member_add(sd->text_object, object);

   // FIXME: cursor should not be a rect - shoudl be an edje.
   sd->clip = evas_object_rectangle_add(evas);
   evas_object_clip_set(sd->text_object, sd->clip);
   evas_object_smart_member_add(sd->clip, object);

   sd->cursor_object = evas_object_rectangle_add(evas);
   evas_object_color_set(sd->cursor_object, 0, 0, 0, 255);
   evas_object_smart_member_add(sd->cursor_object, object);

   evas_object_smart_data_set(object, sd);

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(sd->text_object);
   evas_textblock_cursor_node_first(cursor);

   evas_font_path_append (evas, PACKAGE_DATA_DIR"/data/fonts");
}

/* Called when the object is deleted */
static void
_e_editable_text_smart_del(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   if (sd->cursor_timer)
     ecore_timer_del(sd->cursor_timer);
   evas_object_del(sd->text_object);

   evas_object_del(sd->clip);
   evas_object_del(sd->cursor_object);
   free(sd);

   e_editable_text_style_use_count--;
   if ((e_editable_text_style_use_count <= 0) && (e_editable_text_style))
     {
	evas_textblock_style_free(e_editable_text_style);
	e_editable_text_style = NULL;
     }
}

/* Called when the object is moved */
static void _e_editable_text_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_move(sd->clip, x, y);
   evas_object_move(sd->text_object, x, y);
   _e_editable_text_cursor_position_update(object);
}

/* Called when the object is resized */
static void
_e_editable_text_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_resize(sd->clip, w, h);
}

/* Called when the object is shown */
static void
_e_editable_text_smart_show(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_show(sd->text_object);
   evas_object_show(sd->clip);
   _e_editable_text_cursor_visibility_update(object);
}

/* Called when the object is hidden */
static void
_e_editable_text_smart_hide(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_hide(sd->cursor_object);
   evas_object_hide(sd->text_object);
   evas_object_hide(sd->clip);
}

/* Called when the user presses a key */
static void
_e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event)
{
   E_Entry_Smart_Data *sd;
   Evas_Event_Key_Down *key_event = event;

   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;

   obj = sd->entry_object;

   if ((!strcmp(key_event->keyname, "BackSpace")) ||
       ((key_event->string) && (strlen(key_event->string) == 1) &&
	(key_event->string[0] == 0x8)))
     {
	e_editable_text_delete_char_before(obj);
	if (sd->change_func)
	  sd->change_func(sd->change_data, obj, "");
     }
   else if ((!strcmp(key_event->keyname, "Delete")) ||
	    ((key_event->string) && (strlen(key_event->string) == 1) &&
	     (key_event->string[0] == 0x4)))
     {
	e_editable_text_delete_char_after(obj);
	if (sd->change_func)
	  sd->change_func(sd->change_data, obj, "");
     }
   else if (!strcmp(key_event->keyname, "Left"))
     e_editable_text_cursor_move_left(obj);
   else if (!strcmp(key_event->keyname, "Right"))
     e_editable_text_cursor_move_right(obj);
   else if ((!strcmp(key_event->keyname, "Home")) ||
	    ((key_event->string) && (strlen(key_event->string) == 1) &&
	     (key_event->string[0] == 0x1)))
     e_editable_text_cursor_move_at_start(obj);
   else if ((!strcmp(key_event->keyname, "End")) ||
	    ((key_event->string) && (strlen(key_event->string) == 1) &&
	     (key_event->string[0] == 0x5)))
     e_editable_text_cursor_move_at_end(obj);
   else
     {	
	e_editable_text_insert(obj, key_event->string);
	if ((key_event->string) && (strcmp(key_event->keyname, "Escape")))
	  {
	     if (*(key_event->string) >= 0x20)
	       {
		  if (sd->change_func)
		    sd->change_func(sd->change_data, obj,
				    (char *)key_event->string);
	       }
	  }
     }
}

/* Called when the entry is focused */
/* we need to make this work, move then to e_entry_...? fuck it, am too sleepy */
static void
_e_editable_text_focus_cb(Evas_Object *object)
{
   if (!object)
     return;

   e_editable_text_cursor_show(object);
}

/* Called when the entry is unfocused */
/* we need to make this work */
static void
_e_editable_text_unfocus_cb(Evas_Object *object)
{
   if (!object)
     return;

   //e_editable_text_object_cursor_hide(object);
   //e_editable_text_object_cursor_move_at_start(object);
}

static void
_e_entry_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Entry_Smart_Data *sd;

   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = malloc(sizeof(E_Entry_Smart_Data));
   if (!sd) return;

   sd->change_func = NULL;
   sd->change_data = NULL;
   
   sd->entry_object = e_editable_text_add(evas);
   evas_object_smart_member_add(sd->entry_object, object);


   sd->edje_object = edje_object_add(evas);
   e_theme_edje_object_set(sd->edje_object, "base/theme/widgets",
			   "widgets/entry");

   edje_object_part_swallow(sd->edje_object, "text_area", sd->entry_object);
   evas_object_smart_member_add(sd->edje_object, object);

   evas_object_smart_data_set(object, sd);

   evas_object_event_callback_add(object, EVAS_CALLBACK_KEY_DOWN, _e_entry_key_down_cb, NULL);
}

static void
_e_entry_smart_del(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_del(sd->entry_object);
   evas_object_del(sd->edje_object);
   E_FREE(sd);
}

static void
_e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
      
   evas_object_move(sd->edje_object, x, y);
   e_entry_cursor_move_at_start(object);
}

static void
_e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_resize(sd->edje_object, w, h);
}

static void
_e_entry_smart_show(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_show(sd->edje_object);
}

static void
_e_entry_smart_hide(Evas_Object *object)
{
   E_Entry_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_hide(sd->edje_object);
}

#include "e.h"

/*
 * TODO:
 * - get text
 * - review everything, was sleepy when i wrote this
 * - look at theme / how its being set
 * - implement missing _e_entry_smart_*, very easy
 * - free / delete properly
 * - implement focus and interact with theme
 * - e style
 */

typedef struct _E_Editable_Text_Smart_Data
{
   Evas_Object *clip;
   Evas_Object *text_object;
   Evas_Object *cursor_object;
   Evas_Object *edje_object;
   Ecore_Timer *cursor_timer;
   
   Evas_Bool cursor_at_the_end;
   Evas_Bool show_cursor;
} E_Editable_Text_Smart_Data;

typedef struct _E_Entry_Smart_Data
{
   Evas_Object *entry_object;
   Evas_Object *edje_object;   
} E_Entry_Smart_Data;

static Evas_Bool _e_editable_text_is_empty(Evas_Object *object);
static void _e_editable_text_cursor_position_update(Evas_Object *object);
static void _e_editable_text_cursor_visibility_update(Evas_Object *object);
static int _e_editable_text_cursor_timer_cb(void *data);
static void _e_editable_text_size_update(Evas_Object *object);

static void _e_editable_text_smart_add(Evas_Object *object);
static void _e_editable_text_smart_del(Evas_Object *object);
static void _e_editable_text_smart_raise(Evas_Object *object);
static void _e_editable_text_smart_lower(Evas_Object *object);
static void _e_editable_text_smart_stack_above(Evas_Object *object, Evas_Object *above);
static void _e_editable_text_smart_stack_below(Evas_Object *object, Evas_Object *below);
static void _e_editable_text_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_editable_text_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_editable_text_smart_show(Evas_Object *object);
static void _e_editable_text_smart_hide(Evas_Object *object);

static Evas_Smart *e_editable_text_smart = NULL;
static Evas_Textblock_Style *_e_editable_text_style = NULL;
static int _e_editable_text_style_use_count = 0;

static void _e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event);
static void _e_entry_focus_cb(Evas_Object *object, void *data);
static void _e_entry_unfocus_cb(Evas_Object *object, void *data);

static void _e_entry_smart_add(Evas_Object *object);
static void _e_entry_smart_del(Evas_Object *object);
static void _e_entry_smart_raise(Evas_Object *object);
static void _e_entry_smart_lower(Evas_Object *object);
static void _e_entry_smart_stack_above(Evas_Object *object, Evas_Object *above);
static void _e_entry_smart_stack_below(Evas_Object *object, Evas_Object *below);
static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_entry_smart_show(Evas_Object *object);
static void _e_entry_smart_hide(Evas_Object *object);

static Evas_Smart *e_entry_smart = NULL;

Evas_Object *e_entry_add (Evas *evas)
{
   if (!e_entry_smart)
   {
      e_entry_smart = evas_smart_new("e_entry",
         _e_entry_smart_add, /* add */
         _e_entry_smart_del, /* del */
         NULL, /* layer_set */
         _e_entry_smart_raise, /* raise */
         _e_entry_smart_lower, /* lower */
         _e_entry_smart_stack_above, /* stack_above */
         _e_entry_smart_stack_below, /* stack_below */
         _e_entry_smart_move, /* move */
         _e_entry_smart_resize, /* resize */
         _e_entry_smart_show, /* show */
         _e_entry_smart_hide, /* hide */
         NULL, /* color_set */
         NULL, /* clip_set */
         NULL, /* clip_unset */
         NULL); /* data*/
   }
   return  evas_object_smart_add(evas, e_entry_smart);   
}

void
e_entry_text_set (Evas_Object *entry, const char *text)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_text_set (e_entry_sd->entry_object, text);
}

void 
e_entry_text_insert (Evas_Object *entry, const char *text)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_insert (e_entry_sd->entry_object, text);
}

void 
e_entry_delete_char_before(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_delete_char_before (e_entry_sd->entry_object);
}

void 
e_entry_delete_char_after(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_delete_char_after (e_entry_sd->entry_object);
}

void 
e_entry_cursor_move_at_start(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_move_at_start (e_entry_sd->entry_object);
}

void
e_entry_cursor_move_at_end(Evas_Object *entry)
    {
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_move_at_end (e_entry_sd->entry_object);
}

void 
e_entry_cursor_move_left(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_move_left (e_entry_sd->entry_object);
}

void 
e_entry_cursor_move_right(Evas_Object *entry)
    {
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_move_right (e_entry_sd->entry_object);
}

void 
e_entry_cursor_show(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_show (e_entry_sd->entry_object);
}

void 
e_entry_cursor_hide(Evas_Object *entry)
{
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!entry || !(e_entry_sd = evas_object_smart_data_get(entry)))
     return;
   
   e_editable_text_cursor_hide (e_entry_sd->entry_object);
}    


static void _e_entry_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Entry_Smart_Data *e_entry_sd;
   
   if (!object || !(evas = evas_object_evas_get(object)))
     return;
   
   e_entry_sd = malloc(sizeof(E_Entry_Smart_Data));
   
   e_entry_sd->entry_object = e_editable_text_add (evas);
   evas_object_smart_member_add(e_entry_sd->entry_object, object);
   
   
   e_entry_sd->edje_object = edje_object_add (evas);
   e_theme_edje_object_set(e_entry_sd->edje_object, "base/theme/widgets",
			   "widgets/entry");
      
   edje_object_part_swallow (e_entry_sd->edje_object, "text_area", e_entry_sd->entry_object);
   evas_object_smart_member_add(e_entry_sd->edje_object, object);
   
   evas_object_smart_data_set(object, e_entry_sd);
      
   evas_object_event_callback_add (object, EVAS_CALLBACK_KEY_DOWN, _e_entry_key_down_cb, NULL);
}
   
static void _e_entry_smart_del(Evas_Object *object)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_del (e_entry_sd->entry_object);
   evas_object_del (e_entry_sd->edje_object);
   E_FREE (e_entry_sd);
}

static void _e_entry_smart_raise(Evas_Object *object)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_raise(e_entry_sd->edje_object);   
}

static void _e_entry_smart_lower(Evas_Object *object)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_lower(e_entry_sd->edje_object);
}

static void _e_entry_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;   
   
   evas_object_stack_above(e_entry_sd->edje_object, above);
}

static void _e_entry_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_stack_below(e_entry_sd->edje_object, below);
}

static void _e_entry_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_move(e_entry_sd->edje_object, x, y);
}

static void _e_entry_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_resize (e_entry_sd->edje_object, w, h);
}

static void _e_entry_smart_show(Evas_Object *object)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_show (e_entry_sd->edje_object);
}

static void _e_entry_smart_hide(Evas_Object *object)
{
   E_Entry_Smart_Data *e_entry_sd;
      
   if (!object || !(e_entry_sd = evas_object_smart_data_get(object)))
     return;   
   
   evas_object_hide (e_entry_sd->edje_object);
}

/* Called when the user presses a key */
static void 
_e_entry_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event)
{
   E_Entry_Smart_Data *e_entry_sd;   
   Evas_Event_Key_Down *key_event = event;
      
   if (!obj || !(e_entry_sd = evas_object_smart_data_get(obj)))
          return;
   
   obj = e_entry_sd->entry_object;
   
   if (strcmp(key_event->keyname, "BackSpace") == 0)
     e_editable_text_delete_char_before(obj);
   else if (strcmp(key_event->keyname, "Delete") == 0)
     e_editable_text_delete_char_after(obj);
   else if (strcmp(key_event->keyname, "Left") == 0)
     e_editable_text_cursor_move_left(obj);
   else if (strcmp(key_event->keyname, "Right") == 0)
           e_editable_text_cursor_move_right(obj);
   else if (strcmp(key_event->keyname, "Home") == 0)
     e_editable_text_cursor_move_at_start(obj);
   else if (strcmp(key_event->keyname, "End") == 0)
     e_editable_text_cursor_move_at_end(obj);
   else
     e_editable_text_insert(obj, key_event->string);
}

Evas_Object *e_editable_text_add(Evas *evas)
{
   Evas_Object *o;
   
   if (!e_editable_text_smart)
   {
      e_editable_text_smart = evas_smart_new("e_entry",
         _e_editable_text_smart_add, /* add */
         _e_editable_text_smart_del, /* del */
         NULL, /* layer_set */
         _e_editable_text_smart_raise, /* raise */
         _e_editable_text_smart_lower, /* lower */
         _e_editable_text_smart_stack_above, /* stack_above */
         _e_editable_text_smart_stack_below, /* stack_below */
         _e_editable_text_smart_move, /* move */
         _e_editable_text_smart_resize, /* resize */
         _e_editable_text_smart_show, /* show */
         _e_editable_text_smart_hide, /* hide */
         NULL, /* color_set */
         NULL, /* clip_set */
         NULL, /* clip_unset */
         NULL); /* data*/
   }
   o =  evas_object_smart_add(evas, e_editable_text_smart);
   
   return o;				   				      
}

/**
 * @brief Sets the text of the object
 * @param object an editable text object
 * @param text the text to set
 */
void e_editable_text_text_set(Evas_Object *object, const char *text)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)) || !text)
      return;
   
   printf("Text set: %s\n", text);
   evas_object_textblock2_text_markup_set(editable_text_sd->text_object, text);
   editable_text_sd->cursor_at_the_end = TRUE;
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Inserts text at the cursor position of the object
 * @param object an editable text object
 * @param text the text to insert
 */
void e_editable_text_insert(Evas_Object *object, const char *text)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;
   if (!text || (strlen(text) <= 1 && *text < 0x20))
      return;
   
   printf("Insert: \"%s\", %c\n", text, *text);
   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);

   if (editable_text_sd->cursor_at_the_end)
      evas_textblock2_cursor_text_append(cursor, text);
   else
      evas_textblock2_cursor_text_prepend(cursor, text);
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Deletes the char placed before the cursor
 * @param object an editable text object
 */
void e_editable_text_delete_char_before(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)) || _e_editable_text_is_empty(object))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);

   if (editable_text_sd->cursor_at_the_end || evas_textblock2_cursor_char_prev(cursor))
      evas_textblock2_cursor_char_delete(cursor);
   
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Deletes the char placed after the cursor
 * @param object an editable text object
 */
void e_editable_text_delete_char_after(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)) || _e_editable_text_is_empty(object))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   
   if (!editable_text_sd->cursor_at_the_end)
   {
      if (!evas_textblock2_cursor_char_next(cursor))
         editable_text_sd->cursor_at_the_end = TRUE;
      else
         evas_textblock2_cursor_char_prev(cursor);
      evas_textblock2_cursor_char_delete(cursor);
   }

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object at the start of the text
 * @param object an editable text object
 */
void e_editable_text_cursor_move_at_start(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)) || _e_editable_text_is_empty(object))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   editable_text_sd->cursor_at_the_end = FALSE;
   evas_textblock2_cursor_char_first(cursor);

   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object at the end of the text
 * @param object an editable text object
 */
void e_editable_text_cursor_move_at_end(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)) || _e_editable_text_is_empty(object))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   editable_text_sd->cursor_at_the_end = TRUE;
   evas_textblock2_cursor_char_last(cursor);

   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object to the left
 * @param object an editable text object
 */
void e_editable_text_cursor_move_left(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   if (editable_text_sd->cursor_at_the_end)
      editable_text_sd->cursor_at_the_end = FALSE;
   else
      evas_textblock2_cursor_char_prev(cursor);
   
   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Moves the cursor of the editable text object to the right
 * @param object an editable text object
 */
void e_editable_text_cursor_move_right(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   if (!evas_textblock2_cursor_char_next(cursor))
      editable_text_sd->cursor_at_the_end = TRUE;

   _e_editable_text_size_update(object);
   _e_editable_text_cursor_position_update(object);
}

/**
 * @brief Shows the cursor of the editable text object
 * @param object the editable text object
 */
void e_editable_text_cursor_show(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   editable_text_sd->show_cursor = TRUE;
   _e_editable_text_cursor_visibility_update(object);
}

/**
 * @brief Hides the cursor of the editable text object
 * @param object the editable text object
 */
void e_editable_text_cursor_hide(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   
   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
     return;
   
   editable_text_sd->show_cursor = FALSE;
   _e_editable_text_cursor_visibility_update(object);
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

/**************************
 *
 * Private functions
 *
 **************************/

/* Returns TRUE if the text of the editable object is empty */
static Evas_Bool _e_editable_text_is_empty(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return TRUE;

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   return (evas_textblock2_cursor_node_text_length_get(cursor) <= 0);
}

/* Updates the size of the text object: to be called when the text of the object is changed */
static void _e_editable_text_size_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   int w, h;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_textblock2_size_native_get(editable_text_sd->text_object, &w, &h);
   evas_object_resize(editable_text_sd->text_object, w, h);
}

/* Updates the cursor position: to be called when the cursor or the object are moved */
static void _e_editable_text_cursor_position_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;
   Evas_Coord tx, ty, tw, th, cx, cy, cw, ch, ox, oy, ow, oh;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_geometry_get(editable_text_sd->text_object, &tx, &ty, &tw, &th);
   evas_object_geometry_get(object, &ox, &oy, &ow, &oh);
   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
   
   if (_e_editable_text_is_empty(object))
   {
      evas_object_move(editable_text_sd->cursor_object, ox, oy);
      evas_object_resize(editable_text_sd->cursor_object, 1, oh);
      return;
   }
   else if (editable_text_sd->cursor_at_the_end)
   {
      evas_textblock2_cursor_char_last(cursor);
      evas_textblock2_cursor_char_geometry_get(cursor, &cx, &cy, &cw, &ch);
      cx += tx + cw;
   }
   else
   {
      evas_textblock2_cursor_char_geometry_get(cursor, &cx, &cy, &cw, &ch);
      cx += tx;
   }

   /* TODO: fix */
   if ((cx < ox + 20) && (cx - tx > 20))
   {
      evas_object_move(editable_text_sd->text_object, tx + ox + 20 - cx, ty);
      cx = ox + 20;
   }
   else if (cx < ox)
   {
      evas_object_move(editable_text_sd->text_object, tx + ox - cx, ty);
      cx = ox;
   }
   else if ((cx > ox + ow - 20) && (tw - (cx - tx) > 20))
   {
      evas_object_move(editable_text_sd->text_object, tx - cx + ox + ow - 20, ty);
      cx = ox + ow - 20;
   }
   else if (cx > ox + ow)
   {
      evas_object_move(editable_text_sd->text_object, tx - cx + ox + ow, ty);
      cx = ox + ow;
   }
   
   evas_object_move(editable_text_sd->cursor_object, cx, ty + cy);
   evas_object_resize(editable_text_sd->cursor_object, 1, ch);
}

/* Updates the visibility state of the cursor */
static void _e_editable_text_cursor_visibility_update(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;
   
   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   if (!editable_text_sd->show_cursor)
   {
      if (editable_text_sd->cursor_timer)
      {
         ecore_timer_del(editable_text_sd->cursor_timer);
         editable_text_sd->cursor_timer = NULL;
      }
      evas_object_hide(editable_text_sd->cursor_object);
   }
   else
   {
      if (!editable_text_sd->cursor_timer)
      {
         editable_text_sd->cursor_timer = ecore_timer_add(0.75, _e_editable_text_cursor_timer_cb, object);
         evas_object_show(editable_text_sd->cursor_object);
      }
   }
}

/* Make the cursor blink */
static int _e_editable_text_cursor_timer_cb(void *data)
{
   Evas_Object *object;
   E_Editable_Text_Smart_Data *editable_text_sd;
   
   if (!(object = data) || !(editable_text_sd = evas_object_smart_data_get(object)))
      return 1;

   if (evas_object_visible_get(editable_text_sd->cursor_object))
   {
      evas_object_hide(editable_text_sd->cursor_object);
      ecore_timer_interval_set(editable_text_sd->cursor_timer, 0.25);
   }
   else
   {
      evas_object_show(editable_text_sd->cursor_object);
      ecore_timer_interval_set(editable_text_sd->cursor_timer, 0.75);
   }

   return 1;
}

/* Called when the object is created */
static void _e_editable_text_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Editable_Text_Smart_Data *editable_text_sd;
   Evas_Textblock_Cursor *cursor;

   if (!object || !(evas = evas_object_evas_get(object)))
      return;

   if (!_e_editable_text_style)
   {
      _e_editable_text_style = evas_textblock2_style_new();
      evas_textblock2_style_set(_e_editable_text_style, "DEFAULT='font=Vera font_size=10 align=left color=#000000 wrap=word'");
      _e_editable_text_style_use_count = 0;
   }

   editable_text_sd = malloc(sizeof(E_Editable_Text_Smart_Data));
   editable_text_sd->show_cursor = FALSE;
   editable_text_sd->cursor_at_the_end = TRUE;
   editable_text_sd->cursor_timer = NULL;

   editable_text_sd->text_object = evas_object_textblock2_add(evas);
   evas_object_textblock2_style_set(editable_text_sd->text_object, _e_editable_text_style);
   _e_editable_text_style_use_count++;
   evas_object_smart_member_add(editable_text_sd->text_object, object);

   editable_text_sd->clip = evas_object_rectangle_add(evas);
   evas_object_clip_set(editable_text_sd->text_object, editable_text_sd->clip);
   evas_object_smart_member_add(editable_text_sd->clip, object);

   editable_text_sd->cursor_object = evas_object_rectangle_add(evas);
   evas_object_color_set(editable_text_sd->cursor_object, 0, 0, 0, 255);
   evas_object_smart_member_add(editable_text_sd->cursor_object, object);

   evas_object_smart_data_set(object, editable_text_sd);

   cursor = (Evas_Textblock_Cursor *)evas_object_textblock2_cursor_get(editable_text_sd->text_object);
	evas_textblock2_cursor_node_first(cursor);
   
   evas_font_path_append (evas, PACKAGE_DATA_DIR"/data/fonts");
}

/* Called when the object is deleted */
static void _e_editable_text_smart_del(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   if (editable_text_sd->cursor_timer)
      ecore_timer_del(editable_text_sd->cursor_timer);
   evas_object_del(editable_text_sd->text_object);

   evas_object_del(editable_text_sd->clip);
   evas_object_del(editable_text_sd->cursor_object);
   free(editable_text_sd);

   _e_editable_text_style_use_count--;
   if (_e_editable_text_style_use_count <= 0 && _e_editable_text_style)
   {
      evas_textblock2_style_free(_e_editable_text_style);
      _e_editable_text_style = NULL;
   }
}

/* Called when the object is stacked above all the other objects */
static void _e_editable_text_smart_raise(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_raise(editable_text_sd->clip);
   evas_object_raise(editable_text_sd->text_object);
   evas_object_raise(editable_text_sd->cursor_object);
}

/* Called when the object is stacked below all the other objects */
static void _e_editable_text_smart_lower(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_lower(editable_text_sd->cursor_object);
   evas_object_lower(editable_text_sd->text_object);
   evas_object_lower(editable_text_sd->clip);
}

/* Called when the object is stacked above another object */
static void _e_editable_text_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !above || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_stack_above(editable_text_sd->clip, above);
   evas_object_stack_above(editable_text_sd->text_object, editable_text_sd->clip);
   evas_object_stack_above(editable_text_sd->cursor_object, editable_text_sd->text_object);
}

/* Called when the object is stacked below another object */
static void _e_editable_text_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !below || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_stack_below(editable_text_sd->cursor_object, below);
   evas_object_stack_below(editable_text_sd->text_object, editable_text_sd->cursor_object);
   evas_object_stack_below(editable_text_sd->clip, editable_text_sd->text_object);
}

/* Called when the object is moved */
static void _e_editable_text_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_move(editable_text_sd->clip, x, y);
   evas_object_move(editable_text_sd->text_object, x, y);
   _e_editable_text_cursor_position_update(object);
}

/* Called when the object is resized */
static void _e_editable_text_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;


   evas_object_resize(editable_text_sd->clip, w, h);
}

/* Called when the object is shown */
static void _e_editable_text_smart_show(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_show(editable_text_sd->text_object);
   evas_object_show(editable_text_sd->clip);
   _e_editable_text_cursor_visibility_update(object);
}

/* Called when the object is hidden */
static void _e_editable_text_smart_hide(Evas_Object *object)
{
   E_Editable_Text_Smart_Data *editable_text_sd;

   if (!object || !(editable_text_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_hide(editable_text_sd->cursor_object);
   evas_object_hide(editable_text_sd->text_object);
   evas_object_hide(editable_text_sd->clip);
}

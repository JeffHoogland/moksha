#include "e.h"

static void
e_entry_down_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Entry *entry;
   int pos;
   
   entry = _data;
   printf("%i %i\n", _x, _y);
   entry->focused = 1;
   pos = evas_text_at_position(_e, entry->text, _x, _y,
			       NULL, NULL, NULL, NULL);
   printf("%i\n", pos);
   if (pos < 0)
     {
	int tw;
	
	tw = evas_get_text_width(_e, entry->text);
	if (_x > entry->x + tw)
	  {
	     entry->cursor_pos = strlen(entry->buffer);
	  }
     }
   else
     {
	entry->cursor_pos = pos;
     }
/*   
   entry->select.start = entry->cursor_pos;
   entry->select.length = 1;
*/
   e_entry_configure(entry);
}

static void
e_entry_up_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Entry *entry;
   
   entry = _data;
}

static void
e_entry_move_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Entry *entry;
   
   entry = _data;
}

static void 
e_entry_realize(E_Entry *entry)
{
   entry->clip_box = evas_add_rectangle(entry->evas);
   entry->text = evas_add_text(entry->evas, "borzoib", 8, "");
   entry->cursor = evas_add_rectangle(entry->evas);
   entry->event_box = evas_add_rectangle(entry->evas);
   evas_set_color(entry->evas, entry->clip_box, 255, 255, 255, 255);
   evas_set_color(entry->evas, entry->event_box, 50, 100, 200, 50);
   evas_set_color(entry->evas, entry->text, 0, 0, 0, 255);
   evas_set_color(entry->evas, entry->cursor, 255, 255, 255, 100);
   evas_set_clip(entry->evas, entry->text, entry->clip_box);
   evas_set_clip(entry->evas, entry->event_box, entry->clip_box);
   evas_set_clip(entry->evas, entry->cursor, entry->clip_box);
   evas_callback_add(entry->evas, entry->event_box, CALLBACK_MOUSE_DOWN, e_entry_down_cb, entry);
   evas_callback_add(entry->evas, entry->event_box, CALLBACK_MOUSE_UP, e_entry_up_cb, entry);
   evas_callback_add(entry->evas, entry->event_box, CALLBACK_MOUSE_MOVE, e_entry_move_cb, entry);
}

static void 
e_entry_unrealize(E_Entry *entry)
{
   if (entry->event_box) evas_del_object(entry->evas, entry->event_box);
   if (entry->cursor) evas_del_object(entry->evas, entry->cursor);
   if (entry->text) evas_del_object(entry->evas, entry->text);
   if (entry->clip_box) evas_del_object(entry->evas, entry->clip_box);
}

static void 
e_entry_configure(E_Entry *entry)
{
   if (!entry->evas) return;
   if (!entry->event_box) return;
   evas_move(entry->evas, entry->clip_box, entry->x, entry->y);
   evas_resize(entry->evas, entry->clip_box, entry->w, entry->h);
   evas_move(entry->evas, entry->event_box, entry->x, entry->y);
   evas_resize(entry->evas, entry->event_box, entry->w, entry->h);
   evas_move(entry->evas, entry->text, entry->x, entry->y);
   if ((entry->buffer) && (entry->buffer[0] != 0) && (entry->focused))
     {
	int tx, ty, tw, th;
	
	if (entry->cursor_pos < strlen(entry->buffer))
	  {
	     evas_text_at(entry->evas, entry->text, entry->cursor_pos, &tx, &ty, &tw, &th);
	  }
	else
	  {
	     entry->cursor_pos = strlen(entry->buffer);
	     evas_text_at(entry->evas, entry->text, entry->cursor_pos - 1, &tx, &ty, &tw, &th);
	     tx += tw;
	     tw = 4;
	  }
	evas_move(entry->evas, entry->cursor, entry->x + tx, entry->y + ty);
	evas_resize(entry->evas, entry->cursor, tw, th);
	evas_show(entry->evas, entry->cursor);
     }
   else if (entry->focused)
     {
	int th;
	
	th = evas_get_text_height(entry->evas, entry->text);
	evas_move(entry->evas, entry->cursor, entry->x, entry->y);
	evas_resize(entry->evas, entry->cursor, 4, th);
	evas_show(entry->evas, entry->cursor);
     }
   else
     {
	evas_hide(entry->evas, entry->cursor);
     }
}

void
e_entry_init(void)
{
}

void
e_entry_free(E_Entry *entry)
{
}

E_Entry *
e_entry_new(void)
{
   E_Entry *entry;
   
   entry = NEW(E_Entry, 1);
   ZERO(entry, E_Entry, 1);
   entry->buffer=strdup("");
   return entry;
}

void
e_entry_handle_keypress(E_Entry *entry, Ev_Key_Down *e)
{
   if (!strcmp(e->key, "Up"))
     {
     }
   else if (!strcmp(e->key, "Down"))
     {
     }
   else if (!strcmp(e->key, "Left"))
     {
	entry->cursor_pos--;
	if (entry->cursor_pos < 0) 
	  entry->cursor_pos = 0;
     }
   else if (!strcmp(e->key, "Right"))
     {
	entry->cursor_pos++;
	if (entry->cursor_pos > strlen(entry->buffer)) 
	  entry->cursor_pos = strlen(entry->buffer);
     }
   else if (!strcmp(e->key, "Escape"))
     {
	entry->focused = 0;
     }
   else if (!strcmp(e->key, "BackSpace"))
     {
	char *str2;
	
	if (entry->cursor_pos > 0)
	  {
	     str2 = strdup(e_entry_get_text(entry));
	     strcpy(&(str2[entry->cursor_pos - 1]), &(entry->buffer[entry->cursor_pos]));
	     entry->cursor_pos--;
	     e_entry_set_text(entry, str2);
	     free(str2);
	  }
     }
   else if (!strcmp(e->key, "Delete"))
     {
	char *str2;
	
	if (entry->cursor_pos < strlen(entry->buffer))
	  {
	     str2 = strdup(e_entry_get_text(entry));
	     strcpy(&(str2[entry->cursor_pos]), &(entry->buffer[entry->cursor_pos + 1]));
	     e_entry_set_text(entry, str2);
	     free(str2);
	  }
     }
   else if (!strcmp(e->key, "Insert"))
     {
     }
   else if (!strcmp(e->key, "Home"))
     {
	entry->cursor_pos = 0;
     }
   else if (!strcmp(e->key, "End"))
     {
	entry->cursor_pos = strlen(entry->buffer);	
     }
   else if (!strcmp(e->key, "Prior"))
     {
	entry->cursor_pos = 0;
     }
   else if (!strcmp(e->key, "Next"))
     {
	entry->cursor_pos = strlen(entry->buffer);	
     }
   else if (!strcmp(e->key, "Return"))
     {
	entry->focused = 0;
     }
   else
     {
	char *type;
	
	printf("%s\n", e->key);
	type = e_key_press_translate_into_typeable(e);
	if (type)
	  {
	     if ((strlen(type) == 1) && (type[0] == 0x01)) /* ctrl+a */
	       {
		  entry->cursor_pos = 0;
	       }
	     else if ((strlen(type) == 1) && (type[0] == 0x05)) /* ctrl+e */
	       {
		  entry->cursor_pos = strlen(entry->buffer);
	       }
	     else if ((strlen(type) == 1) && (type[0] == 0x0b)) /* ctk+k */
	       {
		  char *str2;
		  
		  str2 = strdup(e_entry_get_text(entry));
		  str2[entry->cursor_pos] = 0;
		  e_entry_set_text(entry, str2);
		  free(str2);		  
	       }
	     else if (strlen(type) > 0)
	       {
		  char *str2;
		  
		  printf("%i: %x\n", strlen(type), (unsigned char)type[0]);
		  str2 = malloc(strlen(e_entry_get_text(entry)) + 1 + strlen(type));
		  str2[0] = 0;
		  strncat(str2, entry->buffer, entry->cursor_pos);
		  strcat(str2, type);
		  strcat(str2, &(entry->buffer[entry->cursor_pos]));
		  e_entry_set_text(entry, str2);
		  free(str2);
		  entry->cursor_pos++;
		  printf("type: ->%s<-\n", type);
	       }
	  }
     }   
   e_entry_configure(entry);
}

void
e_entry_set_evas(E_Entry *entry, Evas evas)
{
   if (entry->evas) e_entry_unrealize(entry);
   entry->evas = evas;
   e_entry_realize(entry);
   e_entry_configure(entry);
   if (entry->visible)
     {
	entry->visible = 0;
	e_entry_show(entry);
     }
}

void
e_entry_show(E_Entry *entry)
{
   if (entry->visible) return;
   entry->visible = 1;
   if (!entry->evas) return;
   evas_show(entry->evas, entry->event_box);
   evas_show(entry->evas, entry->clip_box);
   if (entry->focused) evas_show(entry->evas, entry->cursor);
   evas_show(entry->evas, entry->text);
}

void
e_entry_hide(E_Entry *entry)
{
   if (!entry->visible) return;
   entry->visible = 0;
   if (!entry->evas) return;
   evas_hide(entry->evas, entry->event_box);
   evas_hide(entry->evas, entry->clip_box);
   evas_hide(entry->evas, entry->cursor);
   evas_hide(entry->evas, entry->text);
}

void
e_entry_set_layer(E_Entry *entry, int l)
{
   evas_set_layer(entry->evas, entry->clip_box, l);
   evas_set_layer(entry->evas, entry->text, l);
   evas_set_layer(entry->evas, entry->cursor, l);
   evas_set_layer(entry->evas, entry->event_box, l);
}

void
e_entry_set_clip(E_Entry *entry, Evas_Object clip)
{
   evas_set_clip(entry->evas, entry->clip_box, clip);
}

void
e_entry_unset_clip(E_Entry *entry)
{
   evas_unset_clip(entry->evas, entry->clip_box);
}

void
e_entry_move(E_Entry *entry, int x, int y)
{
   entry->x = x;
   entry->y = y;
   e_entry_configure(entry);
}

void
e_entry_resize(E_Entry *entry, int w, int h)
{
   entry->w = w;
   entry->h = h;
   e_entry_configure(entry);
}

void
e_entry_query_max_size(E_Entry *entry, int *w, int *h)
{
}

void
e_entry_set_focus(E_Entry *entry, int focused)
{
   entry->focused = focused;
   e_entry_configure(entry);
}

void
e_entry_set_text(E_Entry *entry, const char *text)
{
   IF_FREE(entry->buffer);
   entry->buffer = strdup(text);
   evas_set_text(entry->evas, entry->text, entry->buffer);
   printf("%i %i (%s)\n", entry->cursor_pos, strlen(entry->buffer), entry->buffer);
   if (entry->cursor_pos > strlen(entry->buffer))
     entry->cursor_pos = strlen(entry->buffer);
   e_entry_configure(entry);
}

const char *
e_entry_get_text(E_Entry *entry)
{
   return entry->buffer;
}

void
e_entry_set_cursor(E_Entry *entry, int cursor_pos)
{
   entry->cursor_pos = cursor_pos;
   e_entry_configure(entry);   
}

int
e_entry_get_curosr(E_Entry *entry)
{
   return entry->cursor_pos;
}

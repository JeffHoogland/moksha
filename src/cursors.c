#include "cursors.h"
#include "config.h"
#include "util.h"

typedef struct _e_cursor E_Cursor;

struct _e_cursor
{
   char *type;
   Cursor cursor;
   time_t mod;
};

static int   cursor_change = 0;
static char *cur_cursor = NULL;
static char *prev_cursor = NULL;
static Evas_List cursors = NULL;

static void e_cursors_idle(void *data);
static void e_cursors_set(char *type);
static E_Cursor * e_cursors_find(char *type);

static void
e_cursors_idle(void *data)
{
   int change = 0;
   
   if (!cursor_change) return;
   if ((prev_cursor) && (cur_cursor) && (strcmp(prev_cursor, cur_cursor)))
     change = 1;
   if ((prev_cursor) && (!cur_cursor))
     change = 1;
   if ((!prev_cursor) && (cur_cursor))
     change = 1;
   if (change) e_cursors_set(cur_cursor);
   IF_FREE(prev_cursor);
   e_strdup(prev_cursor, cur_cursor);
   IF_FREE(cur_cursor);
   cur_cursor = NULL;
   cursor_change = 0;

   return;
   UN(data);
}

static void
e_cursors_set(char *type)
{
   e_cursors_display_in_window(0, type);
}

static E_Cursor *
e_cursors_find(char *type)
{
   Evas_List l;
   
   for (l = cursors; l; l = l->next)
     {
	E_Cursor *c;
	
	c = l->data;
	if (!strcmp(c->type, type)) 
	  {
	     char buf[PATH_MAX];
	     
	     sprintf(buf, "%s/%s.db", e_config_get("cursors"), type);
	     if (e_file_modified_time(buf) > c->mod)
	       {
		  cursors = evas_list_remove(cursors, c);
		  IF_FREE(c->type);
		  e_cursor_free(c->cursor);
		  FREE(c);
		  return NULL;
	       }
	     return c;
	  }
     }
   return NULL;
}

void
e_cursors_display_in_window(Window win, char *type)
{
   E_Cursor *c;

   if (!type) type = "Default";
   c = e_cursors_find(type);
   if (!c)
     {
	Pixmap pmap, mask;
	int hx = 0, hy = 0;
	int fr = 255, fg = 255, fb = 255;
	int br = 0, bg = 0, bb = 0;
	int w = 32, h = 32;
	int ok;
	char buf[PATH_MAX];
	Imlib_Image im;
	
	c = NEW(E_Cursor, 1);
	ZERO(c, E_Cursor, 1);
	
	e_strdup(c->type, type);
	
	sprintf(buf, "%s/%s.db", e_config_get("cursors"), type);
	c->mod = e_file_modified_time(buf);
	E_DB_INT_GET(buf, "/cursor/x", hx, ok);
	E_DB_INT_GET(buf, "/cursor/y", hy, ok);
	sprintf(buf, "%s/%s.db:/cursor/image", e_config_get("cursors"), type);
	im = imlib_load_image(buf);
	if (im)
	  {
	     DATA32 *data;
	     int x, y;
	     GC gcf, gcb;
	     int have_bg = 0, have_fg = 0;
	     
	     imlib_context_set_image(im);
	     w = imlib_image_get_width();
	     h = imlib_image_get_height();
	     pmap = e_pixmap_new(0, w, h, 1);
	     mask = e_pixmap_new(0, w, h, 1);
	     data = imlib_image_get_data_for_reading_only();
	     
	     /* figure out fg & bg */
	     if (!data) goto done;
	     for (y = 0; y < h; y++)
	       {
		  for (x = 0; x < w; x++)
		    {
		       int r, g, b, a;
		       DATA32 pix;
		       
		       pix = data[(y * w) + x];
		       r = (pix >> 16) & 0xff;
		       g = (pix >> 8 ) & 0xff;
		       b = (pix      ) & 0xff;
		       a = (pix >> 24) & 0xff;
		       
		       if (a > 127)
			 {
			    if (!have_bg)
			      {
				 br = r;
				 bg = g;
				 bb = b;
				 have_bg = 1;
			      }
			    if (!have_fg)
			      {
				 if ((have_bg) && 
				     ((br != r) || (bg != g) || (bb != b)))
				   {
				      fr = r;
				      fg = g;
				      fb = b;
				      have_fg = 1;
				      goto done;
				   }
			      }
			 }
		    }
	       }
	     done:
	     
	     /* FIXME: inefficient - using pixmaps and draw point... should */
	     /* use XImages & XShm */
	     
	     /* get some gc's set up */
	     gcb = e_gc_new(pmap);
	     gcf = e_gc_new(pmap);
	     e_gc_set_fg(gcb, 0);
	     e_gc_set_fg(gcf, 1);
	     
	     /* fill out cursor pixmap with 0's (bg)  */
	     e_fill_rectangle(pmap, gcb, 0, 0, w, h);
	     e_fill_rectangle(mask, gcb, 0, 0, w, h);
	     if (!data) goto done2;
	     for (y = 0; y < h; y++)
	       {
		  for (x = 0; x < w; x++)
		    {
		       int r, g, b, a;
		       DATA32 pix;
		       
		       pix = data[(y * w) + x];
		       r = (pix >> 16) & 0xff;
		       g = (pix >> 8 ) & 0xff;
		       b = (pix      ) & 0xff;
		       a = (pix >> 24) & 0xff;
		       
		       if (a > 127) 
			 {
			    e_draw_point(mask, gcf, x, y);
			    if ((r == fr) && (g == fg) && (b == fb))
			      e_draw_point(pmap, gcf, x, y);
			 }
		    }
	       }
	     done2:
	     /* clean up */
	     e_gc_free(gcb);
	     e_gc_free(gcf);
	     
	     imlib_image_put_back_data(data);
	     imlib_free_image();
	  }
	else
	  {
	     IF_FREE(c->type);
	     FREE(c);
	     c = NULL;
	  }
	if (c) 
	  {
	     c->cursor = e_cursor_new(pmap, mask, hx, hy, fr, fg, fb, br, bg, bb);
	     e_pixmap_free(pmap);
	     e_pixmap_free(mask);
	     cursors = evas_list_append(cursors, c);
	  }
     }
   if (c)
     e_cursor_set(win, c->cursor);
   else
     {
	if (!strcmp(type, "Default")) return;
	e_cursors_display_in_window(win, "Default");
     }
}

void
e_cursors_display(char *type)
{
   IF_FREE(cur_cursor);
   e_strdup(cur_cursor, type);
   printf("%s\n", type);
   cursor_change = 1;
}

void
e_cursors_init(void)
{
   e_event_filter_idle_handler_add(e_cursors_idle, NULL);
   e_cursors_set("Default");
}

#include "debug.h"
#include "cursors.h"
#include "config.h"
#include "util.h"
#include "file.h"

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

   D_ENTER;
   
   if (!cursor_change) D_RETURN;
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

   D_RETURN;
   UN(data);
}

static void
e_cursors_set(char *type)
{
   D_ENTER;

   e_cursors_display_in_window(0, type);

   D_RETURN;
}

static E_Cursor *
e_cursors_find(char *type)
{
   Evas_List l;
   
   D_ENTER;

   for (l = cursors; l; l = l->next)
     {
	E_Cursor *c;
	
	c = l->data;
	if (!strcmp(c->type, type)) 
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, PATH_MAX, "%s/%s.db", e_config_get("cursors"), type);
	     if (e_file_mod_time(buf) > c->mod)
	       {
		  cursors = evas_list_remove(cursors, c);
		  IF_FREE(c->type);
		  ecore_cursor_free(c->cursor);
		  FREE(c);
		  D_RETURN_(NULL);
	       }
	     D_RETURN_(c);
	  }
     }
   D_RETURN_(NULL);
}

void
e_cursors_display_in_window(Window win, char *type)
{
   E_Cursor *c;

   D_ENTER;

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
	
	snprintf(buf, PATH_MAX, "%s/%s.db", e_config_get("cursors"), type);
	c->mod = e_file_mod_time(buf);
	E_DB_INT_GET(buf, "/cursor/x", hx, ok);
	E_DB_INT_GET(buf, "/cursor/y", hy, ok);
	snprintf(buf, PATH_MAX, "%s/%s.db:/cursor/image", e_config_get("cursors"), type);
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
	     pmap = ecore_pixmap_new(0, w, h, 1);
	     mask = ecore_pixmap_new(0, w, h, 1);
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
	     gcb = ecore_gc_new(pmap);
	     gcf = ecore_gc_new(pmap);
	     ecore_gc_set_fg(gcb, 0);
	     ecore_gc_set_fg(gcf, 1);
	     
	     /* fill out cursor pixmap with 0's (bg)  */
	     ecore_fill_rectangle(pmap, gcb, 0, 0, w, h);
	     ecore_fill_rectangle(mask, gcb, 0, 0, w, h);
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
			    ecore_draw_point(mask, gcf, x, y);
			    if ((r == fr) && (g == fg) && (b == fb))
			      ecore_draw_point(pmap, gcf, x, y);
			 }
		    }
	       }
	     done2:
	     /* clean up */
	     ecore_gc_free(gcb);
	     ecore_gc_free(gcf);
	     
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
	     c->cursor = ecore_cursor_new(pmap, mask, hx, hy, fr, fg, fb, br, bg, bb);
	     ecore_pixmap_free(pmap);
	     ecore_pixmap_free(mask);
	     cursors = evas_list_append(cursors, c);
	  }
     }
   if (c)
     ecore_cursor_set(win, c->cursor);
   else
     {
	if (!strcmp(type, "Default")) D_RETURN;
	e_cursors_display_in_window(win, "Default");
     }

   D_RETURN;
}

void
e_cursors_display(char *type)
{
   D_ENTER;

   IF_FREE(cur_cursor);
   e_strdup(cur_cursor, type);
   cursor_change = 1;

   D_RETURN;
}

void
e_cursors_init(void)
{
   D_ENTER;

   ecore_event_filter_idle_handler_add(e_cursors_idle, NULL);
   e_cursors_set("Default");

   D_RETURN;
}

#include "guides.h"
#include "text.h"
#include "config.h"

static struct 
{
   int changed;
   
   struct 
     {
	struct 
	  {
	     int loc;
	     struct 
	       {
		  double x, y;
	       } align;
	     char *text;
	     char *icon;
	  } display;
	int x, y, w, h;
	int visible;
	int mode;
     } current, prev;
   
   struct {
      Window display;
      Window l, r, t, b;
   } win;
   struct {
      Evas         evas;
      Ebits_Object bg;
      E_Text      *text;
      Evas_Object  icon;
      Imlib_Image  image;
   } disp;
} guides;

static void e_guides_idle(void *data);
static void e_guides_update(void);

static void
e_guides_idle(void *data)
{
   e_guides_update();
   UN(data);
}

static void
e_guides_update(void)
{
   int max_colors = 216;
   int font_cache = 1024 * 1024;
   int image_cache = 8192 * 1024;
   char *font_dir;
   int redraw;
   
   if (!guides.changed) return;

   redraw = 0;
   if (guides.prev.visible != guides.current.visible)
     {
	if (guides.current.visible)
	  {
	     if (!guides.win.display)
	       {
		  guides.win.display = e_window_new(0, 0, 0, 1, 1);
		  guides.win.l = e_window_new(0, 0, 0, 1, 1);
		  guides.win.r = e_window_new(0, 0, 0, 1, 1);
		  guides.win.t = e_window_new(0, 0, 0, 1, 1);
		  guides.win.b = e_window_new(0, 0, 0, 1, 1);
		  redraw = 1;
	       }
	     if (!guides.disp.evas)
	       {
		  font_dir = e_config_get("fonts");
		  guides.disp.evas = evas_new();
		  evas_set_output_method(guides.disp.evas, RENDER_METHOD_IMAGE);
		  
		  guides.disp.image = imlib_create_image(1, 1);
		  imlib_context_set_image(guides.disp.image);
		  imlib_image_set_has_alpha(1);
		  imlib_image_clear();
		  
		  evas_set_output_image(guides.disp.evas, guides.disp.image);
		  evas_font_add_path(guides.disp.evas, font_dir);
		  evas_set_output_size(guides.disp.evas, 1, 1);
		  evas_set_output_viewport(guides.disp.evas, 0, 0, 1, 1);
		  evas_set_font_cache(guides.disp.evas, font_cache);
		  evas_set_image_cache(guides.disp.evas, image_cache);
	       }
	  }
	else
	  {
	     if (guides.win.display)
	       {
		  e_window_destroy(guides.win.display);
		  e_window_destroy(guides.win.l);
		  e_window_destroy(guides.win.r);
		  e_window_destroy(guides.win.t);
		  e_window_destroy(guides.win.b);
	       }
	     if (guides.disp.evas)
	       {
		  if (guides.disp.bg) ebits_free(guides.disp.bg);
		  if (guides.disp.text) e_text_free(guides.disp.text);
		  if (guides.disp.image)
		    {
		       imlib_context_set_image(guides.disp.image);
		       imlib_free_image();
		    }
		  evas_free(guides.disp.evas);
		  guides.disp.evas = NULL;
		  guides.disp.bg = NULL;
		  guides.disp.text = NULL;
		  guides.disp.icon = NULL;
		  guides.disp.image = NULL;
	       }
	  }
     }
   if (guides.current.x != guides.prev.x) redraw = 1;
   if (guides.current.y != guides.prev.y) redraw = 1;
   if (guides.current.w != guides.prev.w) redraw = 1;
   if (guides.current.h != guides.prev.h) redraw = 1;
   if (guides.current.display.loc != guides.prev.display.loc) redraw = 1;
   if (guides.current.display.align.x != guides.prev.display.align.x) redraw = 1;
   if (guides.current.display.align.y != guides.prev.display.align.y) redraw = 1;
   if (guides.current.display.text != guides.prev.display.text) redraw = 1;
   if (guides.current.display.icon != guides.prev.display.icon) redraw = 1;
   if (guides.current.mode != guides.prev.mode) redraw = 1;
   
   if ((guides.win.display) && (redraw))
     {
	int dx, dy, dw, dh;
	int iw, ih;
	double tw, th;
	char file[4096];
	
	if (!guides.disp.text)
	  {
	     guides.disp.text = e_text_new(guides.disp.evas, guides.current.display.text, "guides");
	     e_text_set_layer(guides.disp.text, 100);
	     e_text_show(guides.disp.text);
	  }
	if (!guides.disp.bg) 
	  {
	     guides.disp.bg = ebits_load(file);
	     if (guides.disp.bg) 
	       {
		  ebits_add_to_evas(guides.disp.bg, guides.disp.evas);
		  ebits_set_layer(guides.disp.bg, 0);
		  ebits_show(guides.disp.bg);
	       }
	  }
	if ((!guides.current.display.text) && (guides.disp.icon))
	  {
	     evas_del_object(guides.disp.evas, guides.disp.icon);
	     guides.disp.icon = NULL;
	  }
	if ((guides.current.display.icon) && (!guides.disp.icon))
	  {
	     guides.disp.icon = evas_add_image_from_file(guides.disp.evas, guides.current.display.icon);
	  }
	if (guides.disp.icon)
	  {
	     evas_set_image_file(guides.disp.evas, guides.disp.icon, guides.current.display.icon);
	     evas_get_image_size(guides.disp.evas, guides.disp.icon, &iw, &ih);
	  }
	e_text_set_text(guides.disp.text, guides.current.display.text);
	e_text_get_min_size(guides.disp.text, &tw, &th);
	
	if (guides.current.mode == E_GUIDES_BOX)
	  {
	     
	  }
     }
	
   if (guides.prev.visible != guides.current.visible)
     {
	if (guides.current.visible)
	  {
	     e_window_raise(guides.win.display);
	     e_window_show(guides.win.display);
	     e_window_raise(guides.win.l);
	     e_window_show(guides.win.l);
	     e_window_raise(guides.win.r);
	     e_window_show(guides.win.r);
	     e_window_raise(guides.win.t);
	     e_window_show(guides.win.t);
	     e_window_raise(guides.win.b);
	     e_window_show(guides.win.b);
	  }
     }
   guides.prev = guides.current;
}

void
e_guides_show(void)
{
   if (guides.current.visible) return;
   guides.changed = 1;
   guides.current.visible = 1;
}

void
e_guides_hide(void)
{
   if (!guides.current.visible) return;
   guides.changed = 1;
   guides.current.visible = 0;
}

void
e_guides_move(int x, int y)
{
   if ((guides.current.x == x) &&
       (guides.current.y == y)) return;
   guides.changed = 1;
   guides.current.x = x;
   guides.current.y = y;
}

void
e_guides_resize(int w, int h)
{
   if ((guides.current.w == w) &&
       (guides.current.h == h)) return;
   guides.changed = 1;
   guides.current.w = w;
   guides.current.h = h;
}

void
e_guides_display_text(char *text)
{
   if ((guides.current.display.text) && (text) &&
       (!strcmp(guides.current.display.text, text))) return;
   guides.changed = 1;
   IF_FREE(guides.current.display.text);
   guides.current.display.text = NULL;
   guides.prev.display.text = (char *)1;
   if (text)
     guides.current.display.text = strdup(text);
}

void
e_guides_display_icon(char *icon)
{
   if ((guides.current.display.icon) && (icon) &&
       (!strcmp(guides.current.display.icon, icon))) return;
   guides.changed = 1;
   IF_FREE(guides.current.display.icon);
   guides.current.display.icon = NULL;
   guides.prev.display.icon = (char *)1;
   if (icon)
     guides.current.display.icon = strdup(icon);
}

void
e_guides_set_display_location(int loc)
{
   if (guides.current.display.loc == loc) return;
   guides.changed = 1;
   guides.current.display.loc = loc;
}

void
e_guides_set_display_alignment(double x, double y)
{
   if ((guides.current.display.align.x == x) &&
       (guides.current.display.align.y == y)) return;
   guides.changed = 1;
   guides.current.display.align.x = x;
   guides.current.display.align.y = y;
}

void
e_guides_set_mode(int mode)
{
   if (guides.current.mode == mode) return;
   guides.changed = 1;
   guides.current.mode = mode;
}

void e_guides_init(void)
{
   guides.changed = 0;
   
   guides.current.display.loc = E_GUIDES_DISPLAY_LOCATION_SCREEN_MIDDLE;
   guides.current.display.text = NULL;
   guides.current.display.icon = NULL;
   guides.current.display.align.x = 0.5;
   guides.current.display.align.y = 0.5;
   guides.current.x = 0;
   guides.current.y = 0;
   guides.current.w = 0;
   guides.current.h = 0;
   guides.current.visible = 0;
   guides.current.mode = E_GUIDES_BOX;
   
   guides.prev = guides.current;
   
   guides.win.display = 0;
   guides.win.l = 0;
   guides.win.r = 0;
   guides.win.t = 0;
   guides.win.b = 0;
   
   guides.disp.evas = NULL;
   guides.disp.bg = NULL;
   guides.disp.text = NULL;
   guides.disp.icon = NULL;
   guides.disp.image = NULL;
   
   e_event_filter_idle_handler_add(e_guides_idle, NULL);
}

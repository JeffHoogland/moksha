#include "guides.h"
#include "text.h"
#include "config.h"
#include "embed.h"
#include "util.h"

static struct 
{
   int changed;
   
   struct 
     {
	struct 
	  {
	     E_Guides_Location loc;
	     struct 
	       {
		  double x, y;
	       } align;
	     char *text;
	     char *icon;
	  } display;
	int x, y, w, h;
	int visible;
	E_Guides_Mode mode;
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
   struct {
      Embed icon;
      Embed text;
   } embed;
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
		  guides.win.display = e_window_override_new(0, 0, 0, 1, 1);
		  guides.win.l = e_window_override_new(0, 0, 0, 1, 1);
		  guides.win.r = e_window_override_new(0, 0, 0, 1, 1);
		  guides.win.t = e_window_override_new(0, 0, 0, 1, 1);
		  guides.win.b = e_window_override_new(0, 0, 0, 1, 1);
		  e_window_save_under(guides.win.display);
		  e_window_save_under(guides.win.l);
		  e_window_save_under(guides.win.r);
		  e_window_save_under(guides.win.t);
		  e_window_save_under(guides.win.b);
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
		  guides.win.display = 0;
		  guides.win.l = 0;
		  guides.win.r = 0;
		  guides.win.t = 0;
		  guides.win.b = 0;
	       }
	     if (guides.disp.evas)
	       {
		  if (guides.embed.icon) e_embed_free(guides.embed.icon);
		  if (guides.embed.text) e_embed_free(guides.embed.text);
		  guides.embed.icon = NULL;
		  guides.embed.text = NULL;
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
	int dx, dy, dw, dh, sw, sh, mw, mh;
	char file[4096];
	
	if (!guides.disp.text)
	  {
	     guides.disp.text = e_text_new(guides.disp.evas, guides.current.display.text, "guides");
	     e_text_set_layer(guides.disp.text, 100);
	     e_text_show(guides.disp.text);
	  }
	if ((!guides.current.display.icon) && (guides.disp.icon))
	  {
	     evas_del_object(guides.disp.evas, guides.disp.icon);
	     guides.disp.icon = NULL;
	  }
	if ((guides.current.display.icon) && (!guides.disp.icon))
	  {
	     guides.disp.icon = evas_add_image_from_file(guides.disp.evas, guides.current.display.icon);
	     evas_show(guides.disp.evas, guides.disp.icon);
	  }
	if (guides.disp.icon)
	  evas_set_image_file(guides.disp.evas, guides.disp.icon, guides.current.display.icon);
	e_text_set_text(guides.disp.text, guides.current.display.text);
	if (!guides.disp.bg) 
	  {
	     char *dir;
	     
	     dir = e_config_get("guides");
	     sprintf(file, "%s/display.bits.db", dir);
	     guides.disp.bg = ebits_load(file);
	     if (guides.disp.bg) 
	       {
		  ebits_add_to_evas(guides.disp.bg, guides.disp.evas);
		  ebits_set_layer(guides.disp.bg, 0);
		  ebits_show(guides.disp.bg);
	       }
	  }
	
	mw = 1;
	mh = 1;
	
	if (guides.disp.bg)
	  {
	     if (!guides.embed.icon)
	       {
		  if (guides.embed.icon) e_embed_free(guides.embed.icon);
		  if (guides.embed.text) e_embed_free(guides.embed.text);
		  guides.embed.icon = e_embed_image_object(guides.disp.bg, "Icon", guides.disp.evas, guides.disp.icon);
		  guides.embed.text = e_embed_text(guides.disp.bg, "Text", guides.disp.evas, guides.disp.text, 0, 0);
	       }
	     ebits_get_real_min_size(guides.disp.bg, &mw, &mh);
	  }
	
	dw = mw;
	dh = mh;
	
	if (guides.disp.bg) 
	  {
	     ebits_move(guides.disp.bg, 0, 0);
	     ebits_resize(guides.disp.bg, dw, dh);
	  }
	if (guides.current.display.loc == E_GUIDES_DISPLAY_LOCATION_SCREEN_MIDDLE)
	  {
	     e_window_get_geometry(0, NULL, NULL, &sw, &sh);
	     dx = (int)(((double)sw - (double)dw) * guides.current.display.align.x);
	     dy = (int)(((double)sh - (double)dh) * guides.current.display.align.y);
	  }
	else if (guides.current.display.loc == E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE)
	  {
	     dx = guides.current.x + (int)(((double)guides.current.w - (double)dw) * guides.current.display.align.x);
	     dy = guides.current.y + (int)(((double)guides.current.h - (double)dh) * guides.current.display.align.y);
	  }
	
	if (guides.disp.image)
	  {
	     imlib_context_set_image(guides.disp.image);
	     imlib_free_image();
	     guides.disp.image = NULL;
	  }
	
	guides.disp.image = imlib_create_image(dw, dh);
	imlib_context_set_image(guides.disp.image);
	imlib_image_set_has_alpha(1);
	imlib_image_clear();

	evas_set_output_image(guides.disp.evas, guides.disp.image);
	evas_set_output_size(guides.disp.evas, dw, dh);
	evas_set_output_viewport(guides.disp.evas, 0, 0, dw, dh);
	evas_update_rect(guides.disp.evas, 0, 0, dw, dh);
	evas_render(guides.disp.evas);
	  {
	     Pixmap pmap, mask;
	     
             pmap = e_pixmap_new(guides.win.display, dw, dh, 0);
	     mask = e_pixmap_new(guides.win.display, dw, dh, 1);
	     
	     imlib_context_set_image(guides.disp.image);
	     
	     imlib_context_set_dither_mask(1);
	     imlib_context_set_dither(1);
	     imlib_context_set_drawable(pmap);
	     imlib_context_set_mask(mask);
	     imlib_context_set_blend(0);
	     imlib_context_set_color_modifier(NULL);
	     
	     imlib_render_image_on_drawable(0, 0);
	     e_window_set_background_pixmap(guides.win.display, pmap);
	     e_window_set_shape_mask(guides.win.display, mask);
	     e_window_clear(guides.win.display);
	     e_pixmap_free(pmap);
	     e_pixmap_free(mask);
	  }	
	e_window_move(guides.win.display, dx, dy);
	e_window_resize(guides.win.display, dw, dh);
	
	if (guides.current.mode == E_GUIDES_BOX)
	  {
	     int fr, fg, fb, fa, br, bg, bb, ba;
	     int x, y, w, h;
	     Pixmap pmap, mask;
	     Imlib_Image image;
	     
	     imlib_context_set_dither_mask(1);
	     imlib_context_set_dither(1);
	     imlib_context_set_blend(1);
	     imlib_context_set_color_modifier(NULL);
	     
	     fr = 255; fg = 255; fb = 255; fa = 255;
	     br = 0  ; bg = 0  ; bb = 0  ; ba = 255;
	     
	     x = guides.current.x;
	     y = guides.current.y + 3;
	     w = 3;
	     h = guides.current.h - 6;
	     if ((w > 0) && (h > 0))
	       {
		  image = imlib_create_image(w, h);
		  imlib_context_set_image(image);
		  imlib_image_set_has_alpha(1);
		  imlib_image_clear();
		  
		  imlib_context_set_color(fr, fg, fb, fa);
		  imlib_image_draw_line(1, 0, 1, h - 1, 0);
		  imlib_context_set_color(br, bg, bb, ba);
		  imlib_image_draw_line(0, 0, 0, h - 1, 0);
		  imlib_image_draw_line(2, 0, 2, h - 1, 0);
		  
		  pmap = e_pixmap_new(guides.win.l, w, h, 0);
		  mask = e_pixmap_new(guides.win.l, w, h, 1);
		  imlib_context_set_drawable(pmap);
		  imlib_context_set_mask(mask);
		  imlib_render_image_on_drawable(0, 0);
		  imlib_free_image();
		  e_window_move(guides.win.l, x, y);
		  e_window_resize(guides.win.l, w, h);
		  e_window_set_background_pixmap(guides.win.l, pmap);
		  e_window_set_shape_mask(guides.win.l, mask);
		  e_window_clear(guides.win.l);
		  e_pixmap_free(pmap);
		  e_pixmap_free(mask);
	       }
	     else
	       {
		  e_window_resize(guides.win.l, 0, 0);
	       }
	     
	     x = guides.current.x + guides.current.w - 3;
	     y = guides.current.y + 3;
	     w = 3;
	     h = guides.current.h - 6;
	     if ((w > 0) && (h > 0))
	       {
		  image = imlib_create_image(w, h);
		  imlib_context_set_image(image);
		  imlib_image_set_has_alpha(1);
		  imlib_image_clear();
		  
		  imlib_context_set_color(fr, fg, fb, fa);
		  imlib_image_draw_line(1, 0, 1, h - 1, 0);
		  imlib_context_set_color(br, bg, bb, ba);
		  imlib_image_draw_line(0, 0, 0, h - 1, 0);
		  imlib_image_draw_line(2, 0, 2, h - 1, 0);
		  
		  pmap = e_pixmap_new(guides.win.r, w, h, 0);
		  mask = e_pixmap_new(guides.win.r, w, h, 1);
		  imlib_context_set_drawable(pmap);
		  imlib_context_set_mask(mask);
		  imlib_render_image_on_drawable(0, 0);
		  imlib_free_image();
		  e_window_move(guides.win.r, x, y);
		  e_window_resize(guides.win.r, w, h);
		  e_window_set_background_pixmap(guides.win.r, pmap);
		  e_window_set_shape_mask(guides.win.r, mask);
		  e_window_clear(guides.win.r);
		  e_pixmap_free(pmap);
		  e_pixmap_free(mask);
	       }
	     else
	       {
		  e_window_resize(guides.win.r, 0, 0);
	       }
	     
	     x = guides.current.x;
	     y = guides.current.y;
	     w = guides.current.w;
	     h = 3;
	     if ((w > 0) && (h > 0))
	       {
		  image = imlib_create_image(w, h);
		  imlib_context_set_image(image);
		  imlib_image_set_has_alpha(1);
		  imlib_image_clear();
		  
		  imlib_context_set_color(br, bg, bb, ba);
		  imlib_image_draw_line(0, 0, w - 1, 0, 0);
		  imlib_image_draw_line(2, 2, w - 3, 2, 0);
		  imlib_image_draw_line(0, 1, 0, 2, 0);
		  imlib_image_draw_line(w - 1, 1, w - 1, 2, 0);
		  imlib_context_set_color(fr, fg, fb, fa);
		  imlib_image_draw_line(1, 1, w - 2, 1, 0);
		  imlib_image_draw_line(1, 2, 1, 2, 0);
		  imlib_image_draw_line(w - 2, 2, w - 2, 2, 0);
		  
		  pmap = e_pixmap_new(guides.win.t, w, h, 0);
		  mask = e_pixmap_new(guides.win.t, w, h, 1);
		  imlib_context_set_drawable(pmap);
		  imlib_context_set_mask(mask);
		  imlib_render_image_on_drawable(0, 0);
		  imlib_free_image();
		  e_window_move(guides.win.t, x, y);
		  e_window_resize(guides.win.t, w, h);
		  e_window_set_background_pixmap(guides.win.t, pmap);
		  e_window_set_shape_mask(guides.win.t, mask);
		  e_window_clear(guides.win.t);
		  e_pixmap_free(pmap);
		  e_pixmap_free(mask);
	       }
	     else
	       {
		  e_window_resize(guides.win.t, 0, 0);
	       }
	     
	     x = guides.current.x;
	     y = guides.current.y + guides.current.h - 3;
	     w = guides.current.w;
	     h = 3;
	     if ((w > 0) && (h > 0))
	       {
		  image = imlib_create_image(w, h);
		  imlib_context_set_image(image);
		  imlib_image_set_has_alpha(1);
		  imlib_image_clear();
		  
		  imlib_context_set_color(br, bg, bb, ba);
		  imlib_image_draw_line(0, 2, w - 1, 2, 0);
		  imlib_image_draw_line(2, 0, w - 3, 0, 0);
		  imlib_image_draw_line(0, 0, 0, 1, 0);
		  imlib_image_draw_line(w - 1, 0, w - 1, 1, 0);
		  imlib_context_set_color(fr, fg, fb, fa);
		  imlib_image_draw_line(1, 1, w - 2, 1, 0);
		  imlib_image_draw_line(1, 0, 1, 0, 0);
		  imlib_image_draw_line(w - 2, 0, w - 2, 0, 0);
		  
		  pmap = e_pixmap_new(guides.win.b, w, h, 0);
		  mask = e_pixmap_new(guides.win.b, w, h, 1);
		  imlib_context_set_drawable(pmap);
		  imlib_context_set_mask(mask);
		  imlib_render_image_on_drawable(0, 0);
		  imlib_free_image();
		  e_window_move(guides.win.b, x, y);
		  e_window_resize(guides.win.b, w, h);
		  e_window_set_background_pixmap(guides.win.b, pmap);
		  e_window_set_shape_mask(guides.win.b, mask);
		  e_window_clear(guides.win.b);
		  e_pixmap_free(pmap);
		  e_pixmap_free(mask);
	       }
	     else
	       {
		  e_window_resize(guides.win.b, 0, 0);
	       }
	  }
     }
	
   if (guides.prev.visible != guides.current.visible)
     {
	if (guides.current.visible)
	  {
	     if (guides.current.mode != E_GUIDES_OPAQUE)
	       {
		  e_window_raise(guides.win.l);
		  e_window_show(guides.win.l);
		  e_window_raise(guides.win.r);
		  e_window_show(guides.win.r);
		  e_window_raise(guides.win.t);
		  e_window_show(guides.win.t);
		  e_window_raise(guides.win.b);
		  e_window_show(guides.win.b);
	       }
	     e_window_raise(guides.win.display);
	     e_window_show(guides.win.display);
	  }
     }
   if (guides.current.mode != guides.prev.mode)
     {
	if (guides.current.mode == E_GUIDES_BOX)
	  {
	     e_window_raise(guides.win.l);
	     e_window_show(guides.win.l);
	     e_window_raise(guides.win.r);
	     e_window_show(guides.win.r);
	     e_window_raise(guides.win.t);
	     e_window_show(guides.win.t);
	     e_window_raise(guides.win.b);
	     e_window_show(guides.win.b);
	  }
	else if (guides.prev.mode == E_GUIDES_OPAQUE)
	  {
	     e_window_hide(guides.win.l);
	     e_window_hide(guides.win.r);
	     e_window_hide(guides.win.t);
	     e_window_hide(guides.win.b);
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
   e_strdup(guides.current.display.text, text);
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
   e_strdup(guides.current.display.icon, icon);
}

void
e_guides_set_display_location(E_Guides_Location loc)
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
e_guides_set_mode(E_Guides_Mode mode)
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

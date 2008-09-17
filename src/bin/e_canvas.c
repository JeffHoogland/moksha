/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_canvas_cb_flush(void *data);

/* local subsystem globals */
static Evas_List *_e_canvases = NULL;
static Ecore_Poller *_e_canvas_cache_flush_poller = NULL;

/* externally accessible functions */
EAPI void
e_canvas_add(Ecore_Evas *ee)
{
   Evas *e;
   
   _e_canvases = evas_list_prepend(_e_canvases, ee);
   e = ecore_evas_get(ee);
   evas_image_cache_set(e, e_config->image_cache * 1024);
   evas_font_cache_set(e, e_config->font_cache * 1024);
   e_path_evas_append(path_fonts, e);
   if (e_config->font_hinting == 0)
     {
	if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_BYTECODE))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_BYTECODE);
	else if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_AUTO))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_AUTO);
	else
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
     }
   else if (e_config->font_hinting == 1)
     {
	if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_AUTO))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_AUTO);
	else
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
     }
   else if (e_config->font_hinting == 2)
     evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
}

EAPI void
e_canvas_del(Ecore_Evas *ee)
{
   _e_canvases = evas_list_remove(_e_canvases, ee);
}

EAPI int
e_canvas_engine_decide(int engine)
{
   /* if use default - use it */
   if (engine == E_EVAS_ENGINE_DEFAULT)
     engine = e_config->evas_engine_default;
   /* if engine is software-16 - do we support it? */
   if (engine == E_EVAS_ENGINE_SOFTWARE_X11_16)
     {
	if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_16_X11))
	  engine = E_EVAS_ENGINE_SOFTWARE_X11;
     }
   /* if engine is gl - do we support it? */
   if (engine == E_EVAS_ENGINE_GL_X11)
     {
	/* if we dont - fall back to software x11 */
	if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_X11))
	  engine = E_EVAS_ENGINE_SOFTWARE_X11;
     }
   /* support xrender? */
   if (engine == E_EVAS_ENGINE_XRENDER_X11)
     {
	/* if we dont - fall back to software x11 */
	if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_XRENDER_X11))
	  engine = E_EVAS_ENGINE_SOFTWARE_X11;
     }
   return engine;
}

EAPI void
e_canvas_recache(void)
{
   Evas_List *l;
   
   for (l = _e_canvases; l; l = l->next)
     {
	Ecore_Evas *ee;
	Evas *e;
	
	ee = l->data;
	e = ecore_evas_get(ee);
	evas_image_cache_set(e, e_config->image_cache * 1024);
	evas_font_cache_set(e, e_config->font_cache * 1024);
     }
   edje_file_cache_set(e_config->edje_cache);
   edje_collection_cache_set(e_config->edje_collection_cache);
   if (_e_canvas_cache_flush_poller)
     {
	ecore_poller_del(_e_canvas_cache_flush_poller);
	_e_canvas_cache_flush_poller = NULL;
     }
   if (e_config->cache_flush_poll_interval > 0)
     {
	_e_canvas_cache_flush_poller = 
	  ecore_poller_add(ECORE_POLLER_CORE,
			   e_config->cache_flush_poll_interval,
			   _e_canvas_cb_flush, NULL);
     }
}

EAPI void
e_canvas_cache_flush(void)
{
   Evas_List *l;
   
   for (l = _e_canvases; l; l = l->next)
     {
	Ecore_Evas *ee;
	Evas *e;
	
	ee = l->data;
	e = ecore_evas_get(ee);
	evas_image_cache_flush(e);
	evas_font_cache_flush(e);
     }
   edje_file_cache_flush();
   edje_collection_cache_flush();
   printf("...caches flushed.\n");
}

EAPI void
e_canvas_cache_reload(void)
{
   Evas_List *l;
   
   for (l = _e_canvases; l; l = l->next)
     {
	Ecore_Evas *ee;
	Evas *e;
	
	ee = l->data;
	e = ecore_evas_get(ee);
	evas_image_cache_reload(e);
     }
}

EAPI void
e_canvas_idle_flush(void)
{
   Evas_List *l;
   
   for (l = _e_canvases; l; l = l->next)
     {
	Ecore_Evas *ee;
	Evas *e;
	
	ee = l->data;
	e = ecore_evas_get(ee);
	evas_render_idle_flush(e);
     }
}

EAPI void
e_canvas_rehint(void)
{
   Evas_List *l;
   
   for (l = _e_canvases; l; l = l->next)
     {
	Ecore_Evas *ee;
	Evas *e;
	
	ee = l->data;
	e = ecore_evas_get(ee);
	if (e_config->font_hinting == 0)
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_BYTECODE);
	else if (e_config->font_hinting == 1)
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_AUTO);
	else if (e_config->font_hinting == 2)
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
     }
}

EAPI Ecore_Evas *
e_canvas_new(int engine_hint, Ecore_X_Window win, int x, int y, int w, int h,
	     int direct_resize, int override, Ecore_X_Window *win_ret,
	     Ecore_X_Window *subwin_ret)
{
   Ecore_Evas *ee;
   int engine;
   
   engine = e_canvas_engine_decide(engine_hint);
   if (engine == E_EVAS_ENGINE_GL_X11)
     {
	ee = ecore_evas_gl_x11_new(NULL, win, x, y, w, h);
	if (ee)
	  {
	     ecore_evas_override_set(ee, override);
	     if (direct_resize) ecore_evas_gl_x11_direct_resize_set(ee, 1);
	     if (win_ret) *win_ret = ecore_evas_gl_x11_window_get(ee);
	     if (subwin_ret) *subwin_ret = ecore_evas_gl_x11_subwindow_get(ee);
	  }
	else
	  goto try2;
     }
   else if (engine == E_EVAS_ENGINE_XRENDER_X11)
     {
	ee = ecore_evas_xrender_x11_new(NULL, win, x, y, w, h);
	if (ee)
	  {
	     ecore_evas_override_set(ee, override);
	     if (direct_resize) ecore_evas_xrender_x11_direct_resize_set(ee, 1);
	     if (win_ret) *win_ret = ecore_evas_xrender_x11_window_get(ee);
	     if (subwin_ret) *subwin_ret = ecore_evas_xrender_x11_subwindow_get(ee);
	  }
	else
	  goto try2;
     }
   else if (engine == E_EVAS_ENGINE_SOFTWARE_X11_16)
     {
	ee = ecore_evas_software_x11_16_new(NULL, win, x, y, w, h);
	if (ee)
	  {
	     ecore_evas_override_set(ee, override);
	     if (direct_resize) ecore_evas_software_x11_16_direct_resize_set(ee, 1);
	     if (win_ret) *win_ret = ecore_evas_software_x11_16_window_get(ee);
	     if (subwin_ret) *subwin_ret = ecore_evas_software_x11_16_subwindow_get(ee);
	  }
	else
	  goto try2;
     }
   else
     {
	try2:
	ee = ecore_evas_software_x11_new(NULL, win, x, y, w, h);
	if (ee)
	  {
	     ecore_evas_override_set(ee, override);
	     if (direct_resize) ecore_evas_software_x11_direct_resize_set(ee, 1);
	     if (win_ret) *win_ret = ecore_evas_software_x11_window_get(ee);
	     if (subwin_ret) *subwin_ret = ecore_evas_software_x11_subwindow_get(ee);
	  }
     }
   return ee;
}

/* local subsystem functions */
static int
_e_canvas_cb_flush(void *data)
{
   e_canvas_cache_flush();
   return 1;
}


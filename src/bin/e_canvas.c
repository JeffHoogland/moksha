/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */
static Evas_List *_e_canvases = NULL;

/* externally accessible functions */
void
e_canvas_add(Ecore_Evas *ee)
{
   Evas *e;
   
   _e_canvases = evas_list_prepend(_e_canvases, ee);
   e = ecore_evas_get(ee);
   evas_image_cache_set(e, e_config->image_cache * 1024);
   evas_font_cache_set(e, e_config->font_cache * 1024);
   e_path_evas_append(path_fonts, e);
//   evas_image_cache_flush(e);
//   evas_image_cache_reload(e);
}

void
e_canvas_del(Ecore_Evas *ee)
{
   _e_canvases = evas_list_remove(_e_canvases, ee);
}

void
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
}

void
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
}

void
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

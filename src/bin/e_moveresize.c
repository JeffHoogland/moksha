/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

Ecore_Evas *_ee = NULL;
Evas_Object *_obj = NULL;

void e_resize_begin(E_Zone *zone, int w, int h)
{
   Evas_Coord ew, eh;
   char buf[40];

   if (_ee)
     {
	e_canvas_del(_ee);
	ecore_evas_free(_ee);
     }
   _ee = ecore_evas_software_x11_new(NULL, zone->container->win,
				     0, 0, 10, 10);
   ecore_evas_override_set(_ee, 1);
   ecore_evas_software_x11_direct_resize_set(_ee, 1);
   e_canvas_add(_ee);
   ecore_evas_borderless_set(_ee, 1);
   ecore_evas_layer_set(_ee, 255);
   ecore_evas_show(_ee);

   _obj = edje_object_add(ecore_evas_get(_ee));
   e_theme_edje_object_set(_obj, "base/theme/borders",
			   "widgets/border/default/resize");
   snprintf(buf, sizeof(buf), "9999x9999");
   edje_object_part_text_set(_obj, "text", buf);
   
   edje_object_size_min_calc(_obj, &ew, &eh);
   evas_object_move(_obj, 0, 0);
   evas_object_resize(_obj, ew, eh);
   evas_object_show(_obj);

   e_resize_update(w, h);
   
   ecore_evas_move(_ee, (zone->w - ew) / 2, (zone->h - eh) / 2);
   ecore_evas_resize(_ee, ew, eh);

   ecore_evas_show(_ee);
}

void e_resize_end(void)
{
   if (_obj)
     {
	evas_object_del(_obj);
	_obj = NULL;
     }
   if (_ee)
     {
	e_canvas_del(_ee);
	ecore_evas_free(_ee);
	_ee = NULL;
     }
}

void e_resize_update(int w, int h)
{
   char buf[40];

   if (!_ee) return;

   if ((w >= 0) &&
       (h >= 0))
     snprintf(buf, sizeof(buf), "%ix%i", w, h);
   else
     snprintf(buf, sizeof(buf), "%ix%i", w, h);
   edje_object_part_text_set(_obj, "text", buf);
}

void e_move_begin(E_Zone *zone, int x, int y)
{
   Evas_Coord w, h;
   char buf[40];

   if (_ee)
     {
	e_canvas_del(_ee);
	ecore_evas_free(_ee);
     }
   _ee = ecore_evas_software_x11_new(NULL, zone->container->win,
					 0, 0, 10, 10);
   ecore_evas_override_set(_ee, 1);
   ecore_evas_software_x11_direct_resize_set(_ee, 1);
   e_canvas_add(_ee);
   ecore_evas_borderless_set(_ee, 1);
   ecore_evas_layer_set(_ee, 255);

   _obj = edje_object_add(ecore_evas_get(_ee));
   e_theme_edje_object_set(_obj, "base/theme/borders",
			   "widgets/border/default/move");
   snprintf(buf, sizeof(buf), "9999 9999");
   edje_object_part_text_set(_obj, "text", buf);

   edje_object_size_min_calc(_obj, &w, &h);
   evas_object_move(_obj, 0, 0);
   evas_object_resize(_obj, w, h);
   
   ecore_evas_move(_ee, (zone->w - w) / 2, (zone->h - h) / 2);
   ecore_evas_resize(_ee, w, h);
}

void e_move_end(void)
{
   if (_obj)
     {
	evas_object_del(_obj);
	_obj = NULL;
     }
   if (_ee)
     {
	e_canvas_del(_ee);
	ecore_evas_free(_ee);
	_ee = NULL;
     }
}

void e_move_update(int x, int y)
{
   char buf[40];

   if (!_ee) return;

   evas_object_show(_obj);
   ecore_evas_show(_ee);
   snprintf(buf, sizeof(buf) - 1, "%i %i", x, y);
   edje_object_part_text_set(_obj, "text", buf);
}

void
e_moveresize_raise(void)
{
   if (_ee) ecore_evas_raise(_ee);
}

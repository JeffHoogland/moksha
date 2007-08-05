/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static E_Popup *_disp_pop = NULL;
static Evas_Object *_obj = NULL;

static int visible = 0;
static int obj_x = 0;
static int obj_y = 0;
static int obj_w = 0;
static int obj_h = 0;

EAPI void
e_resize_begin(E_Zone *zone, int w, int h)
{
   Evas_Coord ew, eh;
   char buf[40];

   _obj = NULL;
   if (_disp_pop) e_object_del(E_OBJECT(_disp_pop));

   if (e_config->resize_info_visible)
     {
	_disp_pop = e_popup_new(zone, 0, 0, 1, 1);
	if (!_disp_pop) return;
	e_popup_layer_set(_disp_pop, 255);
	_obj = edje_object_add(_disp_pop->evas);
	e_theme_edje_object_set(_obj, "base/theme/borders",
	      "e/widgets/border/default/resize");
	snprintf(buf, sizeof(buf), "9999x9999");
	edje_object_part_text_set(_obj, "e.text.label", buf);

	edje_object_size_min_calc(_obj, &ew, &eh);
	evas_object_move(_obj, 0, 0);
	evas_object_resize(_obj, ew, eh);
	evas_object_show(_obj);
	e_popup_edje_bg_object_set(_disp_pop, _obj);
     }
   
   e_resize_update(w, h);

   if (e_config->resize_info_visible)
     {
	e_popup_move_resize(_disp_pop,
	      (obj_x - _disp_pop->zone->x) +
	      ((obj_w - ew) / 2),
	      (obj_y - _disp_pop->zone->y) +
	      ((obj_h - eh) / 2),
	      ew, eh);
	e_popup_show(_disp_pop);
     }

   visible = 1;
}

EAPI void
e_resize_end(void)
{
   if (e_config->resize_info_visible)
     {
	if (_obj)
	  {
	     evas_object_del(_obj);
	     _obj = NULL;
	  }
	if (_disp_pop)
	  {
	     e_object_del(E_OBJECT(_disp_pop));
	     _disp_pop = NULL;
	  }
     }

   visible = 0;
}

EAPI void
e_resize_update(int w, int h)
{
   char buf[40];

   if (!_disp_pop) return;
   if (!visible)
     {
	evas_object_show(_obj);
	e_popup_show(_disp_pop);
	visible = 1;
     }
   snprintf(buf, sizeof(buf), "%ix%i", w, h);
   edje_object_part_text_set(_obj, "e.text.label", buf);
}

EAPI void
e_move_begin(E_Zone *zone, int x, int y)
{
   Evas_Coord ew, eh;
   char buf[40];

   _obj = NULL;
   if (_disp_pop) e_object_del(E_OBJECT(_disp_pop));

   if (e_config->move_info_visible)
     {
	_disp_pop = e_popup_new(zone, 0, 0, 1, 1);
	_obj = edje_object_add(_disp_pop->evas);
	e_theme_edje_object_set(_obj, "base/theme/borders",
	      "e/widgets/border/default/move");
	snprintf(buf, sizeof(buf), "9999 9999");
	edje_object_part_text_set(_obj, "e.text.label", buf);

	edje_object_size_min_calc(_obj, &ew, &eh);
	evas_object_move(_obj, 0, 0);
	evas_object_resize(_obj, ew, eh);
	evas_object_show(_obj);
	e_popup_edje_bg_object_set(_disp_pop, _obj);
     }

   if (e_config->move_info_visible)
     {
	e_popup_move_resize(_disp_pop,
	      (obj_x - _disp_pop->zone->x) +
	      ((obj_w - ew) / 2),
	      (obj_y - _disp_pop->zone->y) +
	      ((obj_h - eh) / 2),
	      ew, eh);
     }
}

EAPI void
e_move_end(void)
{
   if (e_config->move_info_visible)
     {
	if (_obj)
	  {
	     evas_object_del(_obj);
	     _obj = NULL;
	  }
	if (_disp_pop)
	  {
	     e_object_del(E_OBJECT(_disp_pop));
	     _disp_pop = NULL;
	  }
     }

   visible = 0;
}

EAPI void
e_move_update(int x, int y)
{
   char buf[40];

   if (!_disp_pop) return;
   if (!visible)
     {
	evas_object_show(_obj);
	e_popup_show(_disp_pop);
	visible = 1;
     }
   snprintf(buf, sizeof(buf), "%i %i", x, y);
   edje_object_part_text_set(_obj, "e.text.label", buf);
}

EAPI void
e_move_resize_object_coords_set(int x, int y, int w, int h)
{
   obj_x = x;
   obj_y = y;
   obj_w = w;
   obj_h = h;
   if ((_disp_pop) && (e_config->move_info_visible) && (visible))
     {
	e_popup_move(_disp_pop,
		     (obj_x - _disp_pop->zone->x) +
		     ((obj_w - _disp_pop->w) / 2),
		     (obj_y - _disp_pop->zone->y) +
		     ((obj_h - _disp_pop->h) / 2)
		     );
     }
}

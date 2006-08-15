/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *o_edje;
   Evas_Object *o_rect;
   E_Color *color;
};

static void
_e_wid_update(E_Widget_Data *wd)
{
   if (!wd) return;

   evas_object_color_set(wd->o_rect, wd->color->r, wd->color->g, wd->color->b, wd->color->a);
   e_widget_change(wd->obj);
}

Evas_Object *
e_widget_color_well_add(Evas *evas, E_Color *color)
{
   Evas_Object *obj, *o;
   Evas_Coord mw, mh;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);
   
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->color = color;
   wd->obj = obj;

   o = edje_object_add(evas);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/color_well");

   evas_object_show(o); 
   wd->o_edje = o;

   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);

   o = evas_object_rectangle_add(evas);
   e_widget_sub_object_add(obj, o);
   evas_object_color_set(o, color->r, color->g, color->b, color->a);
   edje_object_part_swallow(wd->o_edje, "content", o);
   evas_object_show(o);
   wd->o_rect = o;

   return obj;
}

void
e_widget_color_well_update(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   _e_wid_update(wd);
}




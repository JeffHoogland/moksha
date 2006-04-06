/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    
/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_label_add(Evas *evas, const char *label)
{
   Evas_Object *obj, *o;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/label");
   edje_object_part_text_set(o, "label", label);
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

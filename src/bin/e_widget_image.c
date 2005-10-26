/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    
/* local subsystem functions */

/* externally accessible functions */
Evas_Object *
e_widget_image_add_from_object(Evas *evas, Evas_Object *object, int minw, int minh)
{
   Evas_Object *obj;
   
   obj = e_widget_add(evas);
   
   evas_object_show(object);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, minw, minh);
   e_widget_sub_object_add(obj, object);
   e_widget_resize_object_set(obj, object);
      
   return obj;
}

Evas_Object *
e_widget_image_add_from_file(Evas *evas, char *file, int minw, int minh)
{   
   Evas_Object *obj, *o;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   o = evas_object_image_add(evas);
   evas_object_image_file_set(o, file, NULL);
   
   evas_object_show(o);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, minw, minh);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

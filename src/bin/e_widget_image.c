/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *img;
   int minw, minh;
};

static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_image_add_from_object(Evas *evas, Evas_Object *object, int minw, int minh)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   int mw = 0, mh = 0;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
      
   evas_object_show(object);
   o = e_icon_add(evas);
   e_icon_fill_inside_set(o, 1);
   e_icon_object_set(o, object);
   wd->img = o;
   e_widget_data_set(obj, wd);
   e_widget_can_focus_set(obj, 0);
   wd->minw = minw <= 0 ? minw : 0;
   wd->minh = minh <= 0 ? minh : 0;
   edje_object_size_min_get(object, &mw, &mh);
   if (minw || minh)
     e_widget_min_size_set(obj, minw, minh);
   else
     e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
      
   return obj;
}

EAPI Evas_Object *
e_widget_image_add_from_file(Evas *evas, const char *file, int minw, int minh)
{   
   Evas_Object *obj, *o, *o2;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   wd = calloc(1, sizeof(E_Widget_Data));
   
   o = e_icon_add(evas);
   e_icon_fill_inside_set(o, 1);
   o2 = evas_object_image_add(evas);
   evas_object_image_file_set(o2, file, NULL);
   e_icon_object_set(o, o2);
   
   wd->img = o;   
   evas_object_show(o);
   e_widget_data_set(obj, wd);   
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, minw, minh);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

EAPI void
e_widget_image_edje_set(Evas_Object *obj, const char *file, const char *part)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   e_icon_file_edje_set(wd->img, file, part);
   evas_object_show(wd->img);
}

EAPI void
e_widget_image_file_set(Evas_Object *obj, const char *file)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   e_icon_file_set(wd->img, file);
   evas_object_show(wd->img);
}

EAPI void
e_widget_image_file_key_set(Evas_Object *obj, const char *file, const char *key)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   e_icon_file_key_set(wd->img, file, key);
   evas_object_show(wd->img);
}

EAPI void
e_widget_image_object_set(Evas_Object *obj, Evas_Object *o)
{
   E_Widget_Data *wd;
   int mw, mh;
   
   wd = e_widget_data_get(obj);
   e_icon_object_set(wd->img, o);
   edje_object_size_min_get(o, &mw, &mh);
   if (wd->minw || wd->minh)
     e_widget_min_size_set(obj, wd->minw, wd->minh);
   else
     e_widget_min_size_set(obj, mw, mh);
   evas_object_show(wd->img);
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_frame, *img, *o_thumb;
};

static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_preview_add(Evas *evas, int minw, int minh)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));

   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/preview");
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = e_livethumb_add(evas);
   wd->img = o;
   e_livethumb_vsize_set(o, minw * 2, minh * 2);
   edje_extern_object_min_size_set(o, minw, minh);
   edje_extern_object_max_size_set(o, minw, minh);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "item", o);
   
   e_widget_data_set(obj, wd);   
   e_widget_can_focus_set(obj, 0);
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, o);
      
   return obj;
}

EAPI int
e_widget_preview_file_set(Evas_Object *obj, char *file, char *key)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (wd->o_thumb) evas_object_del(wd->o_thumb);
   wd->o_thumb = e_icon_add(e_livethumb_evas_get(wd->img));
   e_icon_file_key_set(wd->o_thumb, file, key);
   evas_object_show(wd->o_thumb);
   e_livethumb_thumb_set(wd->img, wd->o_thumb);
   return 1;
}

EAPI int
e_widget_preview_edje_set(Evas_Object *obj, char *file, char *group)
{
   E_Widget_Data *wd;
   int ret;
   
   wd = e_widget_data_get(obj);
   if (wd->o_thumb) evas_object_del(wd->o_thumb);
   wd->o_thumb = edje_object_add(e_livethumb_evas_get(wd->img));
   ret = edje_object_file_set(wd->o_thumb, file, group);
   evas_object_show(wd->o_thumb);
   e_livethumb_thumb_set(wd->img, wd->o_thumb);
   return ret;
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

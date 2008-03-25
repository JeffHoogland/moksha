/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj, *o_frame, *img, *o_thumb, *o_extern;
};

/* local subsystem functions */
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_preview_thumb_gen(void *data, Evas_Object *obj, void *event_info);

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
   if (!wd) return NULL;

   wd->obj = obj;

   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/preview");
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);

   o = e_livethumb_add(evas);
   wd->img = o;
   e_livethumb_vsize_set(o, minw * 2, minh * 2);
   edje_extern_object_min_size_set(o, minw, minh);
   edje_extern_object_max_size_set(o, minw, minh);
   evas_object_show(o);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", o);

   e_widget_data_set(obj, wd);   
   e_widget_can_focus_set(obj, 0);
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, o);

   return obj;
}

EAPI Evas *
e_widget_preview_evas_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_livethumb_evas_get(wd->img);
}

EAPI void
e_widget_preview_extern_object_set(Evas_Object *obj, Evas_Object *eobj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   wd->o_extern = eobj;
   e_livethumb_thumb_set(wd->img, wd->o_extern);
}

EAPI int
e_widget_preview_file_set(Evas_Object *obj, const char *file, const char *key)
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
e_widget_preview_thumb_set(Evas_Object *obj, const char *file, const char *key, int w, int h)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (wd->img)
     {
	e_widget_sub_object_del(obj, wd->img);
	evas_object_del(wd->img);
     }

   wd->img = e_thumb_icon_add(evas_object_evas_get(obj));
   e_widget_sub_object_add(obj, wd->img);
   if (e_util_glob_case_match(file, "*.edj"))
     {
	/* FIXME: There is probably a quicker way of doing this. */
	if (edje_file_group_exists(file, "icon"))
	  e_thumb_icon_file_set(wd->img, file, "icon");
	else if (edje_file_group_exists(file, "e/desktop/background"))
	  e_thumb_icon_file_set(wd->img, file, "e/desktop/background");
	else if (edje_file_group_exists(file, "e/init/splash"))
	  e_thumb_icon_file_set(wd->img, file, "e/init/splash");
     }
   else
     e_thumb_icon_file_set(wd->img, file, NULL);
   evas_object_smart_callback_add(wd->img, "e_thumb_gen", _e_wid_preview_thumb_gen, wd);
   e_thumb_icon_size_set(wd->img, w, h);
   e_thumb_icon_begin(wd->img);

   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->img);
   evas_object_show(wd->img);

   return 1;
}

static void
_e_wid_preview_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   E_Widget_Data *wd;

   wd = data;
   e_icon_size_get(wd->img, &w, &h);
   edje_extern_object_min_size_set(wd->img, w, h);
   edje_extern_object_max_size_set(wd->img, w, h);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->img);
   edje_object_size_min_calc(wd->o_frame, &w, &h);
   e_widget_min_size_set(wd->obj, w, h);
   evas_object_resize(wd->obj, w, h);
   evas_object_smart_callback_call(wd->obj, "preview_update", NULL);
}

EAPI int
e_widget_preview_edje_set(Evas_Object *obj, const char *file, const char *group)
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

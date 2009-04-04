/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *text;
};
   
static void _e_wid_del_hook(Evas_Object *obj); 
/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_label_add(Evas *evas, const char *label)
{
   Evas_Object *obj, *o;
   Evas_Coord mw, mh;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/label");
   wd->text = o;
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_can_focus_set(obj, 0);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
      
   return obj;
}

EAPI void
e_widget_label_text_set(Evas_Object *obj, const char *text)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   wd = e_widget_data_get(obj);
   edje_object_part_text_set(wd->text, "e.text.label", text);
   edje_object_size_min_calc(wd->text, &mw, &mh);
   edje_extern_object_min_size_set(wd->text, mw, mh);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   free(wd);
}


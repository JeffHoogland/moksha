/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_frame, *o_table;
};

static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_frametable_add(Evas *evas, char *label, int homogenous)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/frame");
   edje_object_part_text_set(o, "label", label);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = e_table_add(evas);
   wd->o_table = o;
   e_table_homogenous_set(o, homogenous);
   edje_object_part_swallow(wd->o_frame, "items", o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

EAPI void
e_widget_frametable_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h)
{
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0;
   
   wd = e_widget_data_get(obj);
   
   e_table_pack(wd->o_table, sobj, col, row, colspan, rowspan);
   e_widget_min_size_get(sobj, &mw, &mh);
   e_table_pack_options_set(sobj,
			  fill_w, fill_h, /* fill */
			  expand_w, expand_h, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  99999, 99999 /* max */
			  );
   e_table_min_size_get(wd->o_table, &mw, &mh);
   edje_extern_object_min_size_set(wd->o_table, mw, mh);
   edje_object_part_swallow(wd->o_frame, "items", wd->o_table);
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, sobj);
   evas_object_show(sobj);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

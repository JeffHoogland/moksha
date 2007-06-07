/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_frame, *o_box;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_framelist_add(Evas *evas, const char *label, int horiz)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_frame = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/frame");
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = e_box_add(evas);
   wd->o_box = o;
   e_box_orientation_set(o, horiz);
   e_box_homogenous_set(o, 0);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

EAPI void
e_widget_framelist_object_append(Evas_Object *obj, Evas_Object *sobj)
{
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0;
   
   wd = e_widget_data_get(obj);
   
   e_box_pack_end(wd->o_box, sobj);
   e_widget_min_size_get(sobj, &mw, &mh);
   e_box_pack_options_set(sobj,
			  1, 1, /* fill */
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  99999, 99999 /* max */
			  );
   e_box_min_size_get(wd->o_box, &mw, &mh);
   edje_extern_object_min_size_set(wd->o_box, mw, mh);
   edje_object_part_swallow(wd->o_frame, "e.swallow.content", wd->o_box);
   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, sobj);
   evas_object_show(sobj);
}

EAPI void
e_widget_framelist_content_align_set(Evas_Object *obj, double halign, double valign)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   e_box_align_set(wd->o_box, halign, valign);
}

EAPI void
e_widget_framelist_label_set(Evas_Object *obj, const char *label)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   edje_object_part_text_set(wd->o_frame, "e.text.label", label);
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_frame, "e,state,disabled", "e");
   else
     edje_object_signal_emit(wd->o_frame, "e,state,enabled", "e");
}

#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_box;
};

static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
/**  
 * Creates a new list widget 
 *
 * @param evas the evas pointer
 * @param  homogenous should widgets append to the list be evenly spaced out
 * @param horiz the direction the list should be displayed
 * @return the new list wdiget
 */
EAPI Evas_Object *
e_widget_list_add(Evas *evas, int homogenous, int horiz)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   o = e_box_add(evas);
   wd->o_box = o;
   e_box_orientation_set(o, horiz);
   e_box_homogenous_set(o, homogenous);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);

   return obj;
}

/**  
 * Prepend a widget to the list 
 *
 * @param obj the list widget to prepend the sub widget too
 * @param sobj the sub widget
 * @param fill DOCUMENT ME!
 * @param expand DOCUMENT ME! 
 * @param align who the sub widget to be aligned, to wards the center or sides
 * @return the new list wdiget
 */
EAPI void
e_widget_list_object_prepend(Evas_Object *obj, Evas_Object *sobj, int fill, int expand, double align)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   wd = e_widget_data_get(obj);

   e_box_pack_start(wd->o_box, sobj);
   mw = mh = 0;
   e_widget_size_min_get(sobj, &mw, &mh);
   if (e_box_orientation_get(wd->o_box) == 1)
     e_box_pack_options_set(sobj,
			    1, fill, /* fill */
			    expand, expand, /* expand */
			    0.5, align, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   else
     e_box_pack_options_set(sobj,
			    fill, 1, /* fill */
			    expand, expand, /* expand */
			    align, 0.5, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   e_box_size_min_get(wd->o_box, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   e_widget_sub_object_add(obj, sobj);
   evas_object_show(sobj);
}

/**  
 * Append a widget to the list 
 *
 * @param obj the list widget to append the sub widget too
 * @param sobj the sub widget
 * @param fill DOCUMENT ME!
 * @param expand DOCUMENT ME! 
 * @param align who the sub widget to be aligned, to wards the center or sides
 * @return the new list wdiget
 */
EAPI void
e_widget_list_object_append(Evas_Object *obj, Evas_Object *sobj, int fill, int expand, double align)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   wd = e_widget_data_get(obj);

   e_box_pack_end(wd->o_box, sobj);
   mw = mh = 0;
   e_widget_size_min_get(sobj, &mw, &mh);
   if (e_box_orientation_get(wd->o_box) == 1)
     e_box_pack_options_set(sobj,
			    1, fill, /* fill */
			    expand, expand, /* expand */
			    0.5, align, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   else
     e_box_pack_options_set(sobj,
			    fill, 1, /* fill */
			    expand, expand, /* expand */
			    align, 0.5, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   e_box_size_min_get(wd->o_box, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   e_widget_sub_object_add(obj, sobj);
   evas_object_show(sobj);
}

EAPI void
e_widget_list_object_repack(Evas_Object *obj, Evas_Object *sobj, int fill, int expand, double align)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh;

   wd = e_widget_data_get(obj);

   mw = mh = 0;
   e_widget_size_min_get(sobj, &mw, &mh);
   if (e_box_orientation_get(wd->o_box) == 1)
     e_box_pack_options_set(sobj,
			    1, fill, /* fill */
			    expand, expand, /* expand */
			    0.5, align, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   else
     e_box_pack_options_set(sobj,
			    fill, 1, /* fill */
			    expand, expand, /* expand */
			    align, 0.5, /* align */
			    mw, mh, /* min */
			    99999, 99999 /* max */
			    );
   e_box_size_min_get(wd->o_box, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
}

EAPI void
e_widget_list_homogeneous_set(Evas_Object *obj, int homogenous)
{
   E_Widget_Data *wd = e_widget_data_get(obj);
   e_box_homogenous_set(wd->o_box, homogenous);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   free(wd);
}

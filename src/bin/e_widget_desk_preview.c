/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *aspect;
   Evas_Object *table;
   Evas_List *desks;

   int w, h;
   int desk_count_x, desk_count_y;
};


static void
_e_wid_desks_free(E_Widget_Data *wd)
{
   Evas_List *l;
   for (l = wd->desks; l; l = l->next)
     {
	Evas_Object *desk = l->data;
	e_widget_sub_object_del(wd->obj, desk);
	evas_object_del(desk);
     }
   evas_list_free(wd->desks);
   wd->desks = NULL;
   if (wd->table) 
     {
	e_widget_sub_object_del(wd->obj, wd->table);
	evas_object_del(wd->table);
     }
   wd->table = NULL;
   if (wd->aspect) 
     {
	e_widget_sub_object_del(wd->obj, wd->aspect);
	evas_object_del(wd->aspect);
     }
   wd->aspect = NULL;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   _e_wid_desks_free(wd);
   free(wd);
}

/* 
 * XXX break this into num_desks_set and _reconfigure calls 
 * XXX don't completely redraw on change just add/del rows/columns and update min sizes
 */
void
e_widget_desk_preview_num_desks_set(Evas_Object *obj, int nx, int ny)
{
   E_Widget_Data *wd;
   Evas_Object *o;
   int x, y;
   int aw, ah; /* available */
   int mw, mh; /* min size for each desk */
   int tw, th; /* size to thumb at */
   E_Zone *zone;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   wd->desk_count_x = nx;
   wd->desk_count_y = ny;

   _e_wid_desks_free(wd);

   zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));

   o = e_widget_aspect_add(evas_object_evas_get(obj), zone->w * nx, zone->h * ny);
   e_widget_resize_object_set(obj, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   wd->aspect = o;

   o = e_widget_table_add(evas_object_evas_get(obj), 1);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   e_widget_aspect_child_set(wd->aspect, o); 
   wd->table = o;

   evas_object_geometry_get(wd->table, NULL, NULL, &aw, &ah);

   if (ny > nx)
     {
	mh = ah / ny;
	mw = (mh * zone->w) / zone->h;
     }
   else
     {
	mw = aw / nx;
	mh = (mw * zone->h) / zone->w;
     }
   /* this happens when aw == 0 */
   if (!mw)
     {
	mw = 10;
	mh = (mw * zone->h) / zone->w;
     }
   e_widget_aspect_aspect_set(wd->aspect, mw * nx, mh * ny);

   if (mw < 50)
     tw = 50;
   else if (mw < 150)
     tw = 150;
   else
     tw = 300;
   th = (tw * zone->h) / zone->w;

   for (y = 0; y < ny; y++)
     {
	for (x = 0; x < nx; x++)
	  {
	     Evas_Object *overlay;
	     Evas_Object *dw;
	     const char *bgfile;
	     E_Desk *desk = NULL;

	     if (x < zone->desk_x_count && y < zone->desk_y_count)
	       desk = zone->desks[x + (y * zone->desk_x_count)];
	     bgfile = e_bg_file_get(zone, desk);

	     /* wrap desks in a widget (to set min size) */
	     dw = e_widget_add(evas_object_evas_get(obj));
	     e_widget_min_size_set(dw, mw, mh);

	     o = edje_object_add(evas_object_evas_get(obj));
	     e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/deskpreview/desk");
	     e_widget_resize_object_set(dw, o);
	     e_widget_sub_object_add(dw, o);

	     evas_object_show(o);
	     e_widget_sub_object_add(dw, o);
	     overlay = o;

	     o = e_thumb_icon_add(evas_object_evas_get(obj));
	     e_icon_fill_inside_set(o, 0);
	     e_thumb_icon_file_set(o, bgfile, "e/desktop/background");
	     e_thumb_icon_size_set(o, tw, th);
	     edje_object_part_swallow(overlay, "e.swallow.content", o);
	     e_thumb_icon_begin(o);
	     evas_object_show(o);
	     e_widget_sub_object_add(dw, o);

	     e_widget_table_object_append(wd->table, dw, x, y, 1, 1, 1, 1, 1, 1);
	     wd->desks = evas_list_append(wd->desks, dw);
	  }
     }
}

Evas_Object *
e_widget_desk_preview_add(Evas *evas, int nx, int ny)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord iw, ih;

   obj = e_widget_add(evas);
   wd = calloc(1, sizeof(E_Widget_Data));

   wd->obj = obj;
   e_widget_data_set(obj, wd);
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   e_widget_desk_preview_num_desks_set(obj, nx, ny);

   return obj;
}



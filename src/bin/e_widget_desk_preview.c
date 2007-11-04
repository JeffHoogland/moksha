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

   Ecore_Event_Handler *update_handler;

   int w, h;
   int cur_x, cur_y; /* currently drawn */
   int desk_count_x, desk_count_y;
};

typedef struct _E_Widget_Desk_Data E_Widget_Desk_Data;
struct _E_Widget_Desk_Data
{
   Evas_Object *thumb;
   int container, zone;
   int x, y;
};

static void _e_wid_reconfigure(E_Widget_Data *wd);
static void _e_wid_desk_cb_config(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int  _e_wid_cb_bg_update(void *data, int type, void *event);

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   Evas_List *l;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   if (wd->update_handler) ecore_event_handler_del(wd->update_handler);
   for (l = wd->desks; l; l = l->next)
     {
	Evas_Object *o;
	E_Widget_Desk_Data *dd;
	o = l->data;
	dd = e_widget_data_get(o);
	e_thumb_icon_end(o);
     }
   evas_list_free(wd->desks);
   free(wd);
}

static void
_e_wid_desk_del_hook(Evas_Object *obj)
{
   E_Widget_Desk_Data *dd;

   dd = e_widget_data_get(obj);
   if (!dd) return;

   free(dd);
}

EAPI void
e_widget_desk_preview_num_desks_set(Evas_Object *obj, int nx, int ny)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   wd->desk_count_x = nx;
   wd->desk_count_y = ny;
   _e_wid_reconfigure(wd);
}

EAPI Evas_Object *
e_widget_deskpreview_desk_add(Evas *evas, E_Zone *zone, int x, int y, int tw, int th)
{
   Evas_Object *overlay;
   Evas_Object *obj, *o;
   const char *bgfile;
   E_Widget_Desk_Data *dd = NULL;

   bgfile = e_bg_file_get(zone->container->num, zone->num, x, y);

   /* wrap desks in a widget (to set min size) */
   obj = e_widget_add(evas);

   dd = calloc(1, sizeof(E_Widget_Desk_Data));
   e_widget_data_set(obj, dd);
   dd->container = zone->container->num;
   dd->zone = zone->num;
   dd->x = x;
   dd->y = y;

   e_widget_del_hook_set(obj, _e_wid_desk_del_hook);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets", "e/widgets/deskpreview/desk");
   e_widget_resize_object_set(obj, o);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   overlay = o;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_desk_cb_config, dd);

   o = e_thumb_icon_add(evas);
   e_icon_fill_inside_set(o, 0);
   e_thumb_icon_file_set(o, bgfile, "e/desktop/background");
   e_thumb_icon_size_set(o, tw, th);
   edje_object_part_swallow(overlay, "e.swallow.content", o);
   e_thumb_icon_begin(o);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   dd->thumb = o;

   return obj;
}

static void
_e_wid_reconfigure(E_Widget_Data *wd)
{
   Evas_List *l, *delete = NULL;
   int x, y;
   int aw, ah; /* available */
   int mw, mh; /* min size for each desk */
   int tw, th; /* size to thumb at */
   int nx, ny;
   E_Zone *zone;

   if (wd->desk_count_x == wd->cur_x && wd->desk_count_y == wd->cur_y) return;

   nx = wd->desk_count_x;
   ny = wd->desk_count_y;

   zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));

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

   for (l = wd->desks; l; l = l->next)
     {
	Evas_Object *dw = l->data;
	E_Widget_Desk_Data *dd = e_widget_data_get(dw);
	if (dd->x < nx && dd->y < ny)
	  {
	     e_widget_min_size_set(dw, mw, mh);
	     e_widget_table_object_repack(wd->table, dw, dd->x, dd->y, 1, 1, 1, 1, 1, 1);
	     e_thumb_icon_size_set(dd->thumb, tw, th);
	     e_thumb_icon_rethumb(dd->thumb); 
	  }
	else
	  {
	     delete = evas_list_append(delete, dw);
	  }
     }
   while (delete)
     {
	Evas_Object *dw = delete->data;
	e_widget_table_unpack(wd->table, dw);
	evas_object_del(dw);
	wd->desks = evas_list_remove(wd->desks, dw);
	delete = evas_list_remove_list(delete, delete);
     }

   for (y = 0; y < ny; y++)
     {
	int sx;
	if (y >= wd->cur_y) sx = 0;
	else sx = wd->cur_x;
	for (x = sx; x < nx; x++)
	  {
	     Evas_Object *dw;

	     dw = e_widget_deskpreview_desk_add(evas_object_evas_get(wd->obj), zone, x, y, tw, th);
	     e_widget_min_size_set(dw, mw, mh);

	     e_widget_table_object_append(wd->table, dw, x, y, 1, 1, 1, 1, 1, 1);
	     wd->desks = evas_list_append(wd->desks, dw);
	  }
     }

   wd->cur_x = wd->desk_count_x;
   wd->cur_y = wd->desk_count_y;
}

Evas_Object *
e_widget_desk_preview_add(Evas *evas, int nx, int ny)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   E_Zone *zone;

   obj = e_widget_add(evas);
   wd = calloc(1, sizeof(E_Widget_Data));

   wd->obj = obj;
   e_widget_data_set(obj, wd);
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
   o = e_widget_aspect_add(evas, zone->w * nx, zone->h * ny);
   e_widget_resize_object_set(wd->obj, o);
   e_widget_sub_object_add(wd->obj, o);
   evas_object_show(o);
   wd->aspect = o;

   o = e_widget_table_add(evas, 1);
   e_widget_sub_object_add(wd->obj, o);
   evas_object_show(o);
   e_widget_aspect_child_set(wd->aspect, o); 
   wd->table = o;

   e_widget_desk_preview_num_desks_set(obj, nx, ny);

   wd->update_handler = ecore_event_handler_add(E_EVENT_BG_UPDATE, _e_wid_cb_bg_update, wd);

   return obj;
}

static void 
_e_wid_desk_cb_config(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Widget_Desk_Data *dd;
   Evas_Event_Mouse_Down *ev;
   
   dd = data;
   ev = event_info;
   if (ev->button == 1) 
     {
	E_Container *con;
	char buf[256];
	
	con = e_container_current_get(e_manager_current_get());
	snprintf(buf, sizeof(buf), "%i %i %i %i",
		 dd->container, dd->zone, dd->x, dd->y);
	e_configure_registry_call("internal/desk", con, buf);
     }
}

static int
_e_wid_cb_bg_update(void *data, int type, void *event)
{
   E_Event_Bg_Update *ev;
   E_Widget_Data *wd;
   Evas_List *l;

   if (type != E_EVENT_BG_UPDATE) return 1;

   wd = data;
   ev = event;

   for (l = wd->desks; l; l = l->next)
     {
	Evas_Object *o;
	E_Widget_Desk_Data *dd;
	o = l->data;
	dd = e_widget_data_get(o);

	if (!dd) 
	  continue;

	if (((ev->container < 0) || (dd->container == ev->container)) &&
	    ((ev->zone < 0) || (dd->zone == ev->zone)) &&
	    ((ev->desk_x < 0) || (dd->x == ev->desk_x)) &&
	    ((ev->desk_y < 0) || (dd->y == ev->desk_y)))
	  {
	     const char *bgfile = e_bg_file_get(dd->container, dd->zone, dd->x, dd->y);
	     e_thumb_icon_file_set(dd->thumb, bgfile, "e/desktop/background");
	     e_thumb_icon_rethumb(dd->thumb);
	  }
     }
   return 1;
}

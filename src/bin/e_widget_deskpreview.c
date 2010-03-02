/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data 
{
   Evas_Object *obj, *table;
   Eina_List *desks;
   int w, h, dx, dy, cx, cy;
};
typedef struct _E_Widget_Desk_Data E_Widget_Desk_Data;
struct _E_Widget_Desk_Data 
{
   Evas_Object *icon, *thumb;
   int zone, con, x, y;
};

/* local function prototypes */
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_desk_del_hook(Evas_Object *obj);
static void _e_wid_reconfigure(E_Widget_Data *wd);
static void _e_wid_desk_cb_config(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_wid_cb_resize(void *data, Evas *evas, Evas_Object *obj, void *event);

EAPI Evas_Object *
e_widget_deskpreview_add(Evas *evas, int nx, int ny) 
{
   Evas_Object *obj;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);
   wd = E_NEW(E_Widget_Data, 1);
   wd->obj = obj;
   wd->dx = nx;
   wd->dy = ny;
   e_widget_data_set(obj, wd);
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   wd->table = evas_object_table_add(evas);
   evas_object_table_homogeneous_set(wd->table, EINA_TRUE);
   evas_object_table_align_set(wd->table, 0.5, 0.5);
   e_widget_resize_object_set(wd->obj, wd->table);
   evas_object_show(wd->table);
   e_widget_sub_object_add(wd->obj, wd->table);

   e_widget_deskpreview_num_desks_set(obj, wd->dx, wd->dy);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, 
                                  _e_wid_cb_resize, NULL);

   return obj;
}

EAPI void 
e_widget_deskpreview_num_desks_set(Evas_Object *obj, int nx, int ny) 
{
   E_Widget_Data *wd;

   if (!(wd = e_widget_data_get(obj))) return;
   wd->dx = nx;
   wd->dy = ny;
   _e_wid_reconfigure(wd);
}

EAPI Evas_Object *
e_widget_deskpreview_desk_add(Evas_Object *obj, E_Zone *zone, int x, int y, int w, int h) 
{
   E_Widget_Desk_Data *dd;
   const char *bgfile;

   bgfile = e_bg_file_get(zone->container->num, zone->num, x, y);

   dd = E_NEW(E_Widget_Desk_Data, 1);
   dd->con = zone->container->num;
   dd->zone = zone->num;
   dd->x = x;
   dd->y = y;

   dd->icon = e_icon_add(evas_object_evas_get(obj));
   evas_object_data_set(dd->icon, "desk_data", dd);
   e_icon_fill_inside_set(dd->icon, EINA_FALSE);
   e_icon_file_edje_set(dd->icon, bgfile, "e/desktop/background");
   evas_object_size_hint_min_set(dd->icon, w, h);
   evas_object_size_hint_max_set(dd->icon, w, h);
   evas_object_resize(dd->icon, w, h);
   evas_object_show(dd->icon);

   evas_object_event_callback_add(dd->icon, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_desk_cb_config, dd);

   return dd->icon;
}

/* local function prototypes */
static void 
_e_wid_del_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   if (!(wd = e_widget_data_get(obj))) return;

   E_FREE(wd);
}

static void 
_e_wid_desk_del_hook(Evas_Object *obj) 
{

}

static void 
_e_wid_reconfigure(E_Widget_Data *wd) 
{
   Eina_List *l, *del = NULL;
   Evas_Object *dw;
   E_Zone *zone;
   int tw, th, mw, mh, y;

   zone = e_util_zone_current_get(e_manager_current_get());

   evas_object_geometry_get(wd->table, NULL, NULL, &tw, &th);

   /* TODO: Make these values an aspect of zone */
   mw = (tw / wd->dx);
   mh = (th / wd->dy);

   EINA_LIST_FOREACH(wd->desks, l, dw) 
     {
        E_Widget_Desk_Data *dd;

        if (!(dd = evas_object_data_get(dw, "desk_data"))) continue;
        if ((dd->x < wd->dx) && (dd->y < wd->dy)) 
          {
             evas_object_size_hint_min_set(dw, mw, mh);
             evas_object_size_hint_max_set(dw, mw, mh);
             evas_object_resize(dw, mw, mh);
          }
        else
          del = eina_list_append(del, dw);
     }
   EINA_LIST_FREE(del, dw) 
     {
        evas_object_table_unpack(wd->table, dw);
        evas_object_del(dw);
        wd->desks = eina_list_remove(wd->desks, dw);
     }

   for (y = 0; y < wd->dy; y++) 
     {
        int x, sx;

        if (y >= wd->cy) sx = 0;
        else sx = wd->cx;
        for (x = sx; x < wd->dx; x++) 
          {
             Evas_Object *dw;

             dw = e_widget_deskpreview_desk_add(wd->obj, zone, x, y, mw, mh);
             evas_object_table_pack(wd->table, dw, x, y, 1, 1);
             wd->desks = eina_list_append(wd->desks, dw);
          }
     }

   wd->cx = wd->dx;
   wd->cy = wd->dy;
}

static void 
_e_wid_desk_cb_config(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   E_Widget_Desk_Data *dd;
   Evas_Event_Mouse_Down *ev;

   ev = event;
   if (!(dd = data)) return;
   if (ev->button == 1) 
     {
        E_Container *con;
        char buff[256];

        con = e_container_current_get(e_manager_current_get());
        snprintf(buff, sizeof(buff), "%i %i %i %i", 
                 dd->con, dd->zone, dd->x, dd->y);
        e_configure_registry_call("internal/desk", con, buff);
     }
}

static void 
_e_wid_cb_resize(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   E_Widget_Data *wd;

   if (!(wd = e_widget_data_get(obj))) return;
   _e_wid_reconfigure(wd);
}

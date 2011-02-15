#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data 
{
   Evas_Object *obj, *table;
   Eina_List *desks;
   int w, h, dx, dy, cx, cy;
   Ecore_Event_Handler *bg_upd_hdl;
};
typedef struct _E_Widget_Desk_Data E_Widget_Desk_Data;
struct _E_Widget_Desk_Data 
{
   Evas_Object *icon, *thumb;
   int zone, con, x, y;
};

/* local function prototypes */
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_reconfigure(E_Widget_Data *wd);
static void _e_wid_desk_cb_config(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_wid_cb_resize(void *data, Evas *evas, Evas_Object *obj, void *event);
static Eina_Bool _e_wid_cb_bg_update(void *data, int type, void *event);

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
   evas_object_table_padding_set(wd->table, 0, 0);
   evas_object_table_align_set(wd->table, 0.5, 0.5);
   e_widget_resize_object_set(wd->obj, wd->table);
   evas_object_show(wd->table);
   e_widget_sub_object_add(wd->obj, wd->table);

   e_widget_deskpreview_num_desks_set(obj, wd->dx, wd->dy);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, 
                                  _e_wid_cb_resize, NULL);

   wd->bg_upd_hdl = ecore_event_handler_add(E_EVENT_BG_UPDATE, 
                                            _e_wid_cb_bg_update, wd);
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

   dd->icon = edje_object_add(evas_object_evas_get(obj));
   e_theme_edje_object_set(dd->icon, "base/theme/widgets",
			   "e/widgets/deskpreview/desk");

   dd->thumb = e_icon_add(evas_object_evas_get(obj));
   e_icon_fill_inside_set(dd->thumb, EINA_FALSE);
   e_icon_file_edje_set(dd->thumb, bgfile, "e/desktop/background");
   evas_object_show(dd->thumb);
   edje_object_part_swallow(dd->icon, "e.swallow.content", dd->thumb); 

   evas_object_size_hint_min_set(dd->icon, w, h);
   evas_object_size_hint_max_set(dd->icon, w, h);
   evas_object_show(dd->icon);
   evas_object_data_set(dd->icon, "desk_data", dd);
   evas_object_event_callback_add(dd->icon, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_desk_cb_config, dd);

   return dd->icon;
}

/* local function prototypes */
static void 
_e_wid_del_hook(Evas_Object *obj) 
{
   E_Widget_Data *wd;
   Evas_Object *o;

   if (!(wd = e_widget_data_get(obj))) return;
   if (wd->bg_upd_hdl) ecore_event_handler_del(wd->bg_upd_hdl);

   EINA_LIST_FREE(wd->desks, o) 
     {
        E_Widget_Desk_Data *dd;

        if (!(dd = evas_object_data_get(o, "desk_data"))) continue;
	evas_object_del(dd->thumb);
        evas_object_del(o);
        E_FREE(dd);
     }

   E_FREE(wd);
}

static void 
_e_wid_reconfigure(E_Widget_Data *wd) 
{
   Eina_List *l, *ll;
   Evas_Object *dw;
   E_Zone *zone;
   int tw, th, mw, mh, y;
   E_Widget_Desk_Data *dd;
   
   zone = e_util_zone_current_get(e_manager_current_get());

   evas_object_geometry_get(wd->table, NULL, NULL, &tw, &th);

   if (wd->dy > wd->dx) 
     {
        mh = th / wd->dy;
        mw = (mh * zone->w) / zone->h;
     }
   else if (wd->dy < wd->dx) 
     {
        mw = tw / wd->dx;
        mh = (mw * zone->h) / zone->w;
     }
   else 
     {
   	mw = tw / wd->dx;
   	mh = th / wd->dy;
     }
   
   if (mw > tw/wd->dx) 
     mw = (tw * zone->h) / zone->w;
   if (mh > th/wd->dy) 
     mh = (th * zone->w) / zone->h;
   
   EINA_LIST_FOREACH_SAFE(wd->desks, l, ll, dw) 
     {
        if (!(dd = evas_object_data_get(dw, "desk_data"))) continue;
        if ((dd->x < wd->dx) && (dd->y < wd->dy)) 
          {
             evas_object_size_hint_min_set(dw, mw, mh);
             evas_object_size_hint_max_set(dw, mw, mh);
          }
        else
	  {
	     evas_object_del(dd->thumb);
	     evas_object_del(dw);
	     wd->desks = eina_list_remove(wd->desks, dw);
	     E_FREE(dd);     
	  }	
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
	     evas_object_size_hint_aspect_set(dw, EVAS_ASPECT_CONTROL_BOTH, zone->w, zone->h);
             evas_object_table_pack(wd->table, dw, x, y, 1, 1);
             wd->desks = eina_list_append(wd->desks, dw);
          }
     }

   wd->cx = wd->dx;
   wd->cy = wd->dy;
}

static void 
_e_wid_desk_cb_config(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
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
_e_wid_cb_resize(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__) 
{
   E_Widget_Data *wd;

   if (!(wd = e_widget_data_get(obj))) return;
   _e_wid_reconfigure(wd);
}

static Eina_Bool
_e_wid_cb_bg_update(void *data, int type, void *event)
{
   E_Event_Bg_Update *ev;
   E_Widget_Data *wd;
   Eina_List *l;
   Evas_Object *o;

   if (type != E_EVENT_BG_UPDATE) return ECORE_CALLBACK_PASS_ON;
   if (!(wd = data)) return ECORE_CALLBACK_PASS_ON;
   ev = event;

   EINA_LIST_FOREACH(wd->desks, l, o) 
     {
        E_Widget_Desk_Data *dd;

        if (!(dd = evas_object_data_get(o, "desk_data"))) continue;
        if (((ev->container < 0) || (dd->con == ev->container)) && 
            ((ev->zone < 0) || (dd->zone == ev->zone)) && 
            ((ev->desk_x < 0) || (dd->x == ev->desk_x)) && 
            ((ev->desk_y < 0) || (dd->y == ev->desk_y))) 
          {
             const char *bgfile;

             bgfile = e_bg_file_get(dd->con, dd->zone, dd->x, dd->y);
             e_icon_file_edje_set(dd->thumb, bgfile, "e/desktop/background");
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

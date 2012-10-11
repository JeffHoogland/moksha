#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* 'Smart' widget to wrap a pan and scroll into one */

typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* changed flag */
   Eina_Bool changed : 1;

   /* visible flag */
   Eina_Bool visible : 1;

   /* objects in this widget */
   Evas_Object *o_scroll, *o_layout;

   /* list of monitors */
   Eina_List *items;
};

/* local function prototypes */
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset(Evas_Object *obj);
static void _e_smart_reconfigure(E_Smart_Data *sd);
static void _e_smart_randr_layout_adjust(E_Smart_Data *sd, Evas_Object *obj);
static void _e_smart_randr_layout_reposition(E_Smart_Data *sd, Evas_Object *obj);
static void _e_smart_randr_changed_set(Evas_Object *obj);

static void _e_smart_cb_monitor_resized(void *data, Evas_Object *obj, void *event __UNUSED__);
static void _e_smart_cb_monitor_rotated(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__);
static void _e_smart_cb_monitor_moved(void *data, Evas_Object *obj, void *event __UNUSED__);
static void _e_smart_cb_monitor_toggled(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__);
static void _e_smart_cb_monitor_deleted(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__);

/* public functions */
Evas_Object *
e_smart_randr_add(Evas *evas)
{
   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc = 
     {
        "smart_randr", EVAS_SMART_CLASS_VERSION, 
        _e_smart_add, _e_smart_del, _e_smart_move, _e_smart_resize,
        _e_smart_show, _e_smart_hide, NULL, 
        _e_smart_clip_set, _e_smart_clip_unset, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
     };

   if (!smart)
     if (!(smart = evas_smart_class_new(&sc))) 
       return NULL;

   return evas_object_smart_add(evas, smart);
}

void 
e_smart_randr_virtual_size_set(Evas_Object *obj, Evas_Coord vw, Evas_Coord vh)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   /* set virutal size for layout */
   e_layout_virtual_size_set(sd->o_layout, vw, vh);
}

void 
e_smart_randr_monitor_add(Evas_Object *obj, Evas_Object *mon)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   /* tell monitor what layout it is in */
   e_smart_monitor_layout_set(mon, sd->o_layout);

   /* add listeners for when this monitor changes */
   evas_object_smart_callback_add(mon, "monitor_resized", 
                                  _e_smart_cb_monitor_resized, obj);
   evas_object_smart_callback_add(mon, "monitor_rotated", 
                                  _e_smart_cb_monitor_rotated, obj);
   evas_object_smart_callback_add(mon, "monitor_moved", 
                                  _e_smart_cb_monitor_moved, obj);
   evas_object_smart_callback_add(mon, "monitor_toggled", 
                                  _e_smart_cb_monitor_toggled, obj);

   /* add listener for when this monitor gets removed */
   evas_object_event_callback_add(mon, EVAS_CALLBACK_DEL, 
                                  _e_smart_cb_monitor_deleted, sd);

   /* pack this monitor into the layout */
   e_layout_pack(sd->o_layout, mon);
   e_layout_child_lower(mon);

   /* append this monitor to our list */
   sd->items = eina_list_append(sd->items, mon);

   /* reconfigure the layout */
   _e_smart_reconfigure(sd);
}

Eina_List *
e_smart_randr_monitors_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return NULL;

   return sd->items;
}

Eina_Bool 
e_smart_randr_changed_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return EINA_FALSE;

   return sd->changed;
}

/* local functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;

   if (!(sd = calloc(1, sizeof(E_Smart_Data))))
     return;

   evas = evas_object_evas_get(obj);

   /* add layout object */
   sd->o_layout = e_layout_add(evas);
   evas_object_resize(sd->o_layout, 
                      E_RANDR_12->max_size.width / 8,
                      E_RANDR_12->max_size.height / 8);
   evas_object_show(sd->o_layout);

   /* add scroll object */
   sd->o_scroll = e_scrollframe_add(evas);
   e_scrollframe_child_set(sd->o_scroll, sd->o_layout);
   evas_object_smart_member_add(sd->o_scroll, obj);

   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *mon;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   /* delete the monitors */
   EINA_LIST_FREE(sd->items, mon)
     evas_object_del(mon);

   /* delete objects */
   evas_object_del(sd->o_layout);
   evas_object_del(sd->o_scroll);

   E_FREE(sd);

   evas_object_smart_data_set(obj, NULL);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_move(sd->o_scroll, x, y);
   _e_smart_reconfigure(sd);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_resize(sd->o_scroll, w, h);
   _e_smart_reconfigure(sd);
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if (sd->visible) return;
   evas_object_show(sd->o_scroll);
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if (!sd->visible) return;
   evas_object_hide(sd->o_scroll);
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_clip_set(sd->o_scroll, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_clip_unset(sd->o_scroll);
}

static void 
_e_smart_reconfigure(E_Smart_Data *sd)
{
   Eina_List *l;
   Evas_Object *mon;

   e_layout_freeze(sd->o_layout);
   EINA_LIST_FOREACH(sd->items, l, mon)
     {
        Evas_Coord cx, cy, cw, ch;

        e_smart_monitor_crtc_geometry_get(mon, &cx, &cy, &cw, &ch);

        e_layout_child_move(mon, cx, cy);
        e_layout_child_resize(mon, cw, ch);
        e_layout_child_lower(mon);
     }
   e_layout_thaw(sd->o_layout);
}

static void 
_e_smart_randr_layout_adjust(E_Smart_Data *sd, Evas_Object *obj)
{
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o;

   if (!sd) return;

   /* get the geometry of this monitor */
   e_layout_child_geometry_get(obj, &o.x, &o.y, &o.w, &o.h);

   /* freeze layout */
   e_layout_freeze(sd->o_layout);

   /* find any monitors to the right or below this one */
   /* NB: We do not have to worry about monitors to the left or above as their 
    * size will not change because of this resize event */
   EINA_LIST_FOREACH(sd->items, l, mon)
     {
        Eina_Rectangle m;

        /* skip if it's the current monitor */
        if ((mon == obj)) continue;

        /* get the geometry of this monitor */
        e_layout_child_geometry_get(mon, &m.x, &m.y, &m.w, &m.h);

        if ((m.x >= (o.x + o.w)))
          {
             /* if this monitor is to the right, move it */
             e_layout_child_move(mon, (o.x + o.w), m.y);
          }
        else if ((m.y >= (o.y + o.h)))
          {
             /* if this monitor is below, move it */
             e_layout_child_move(mon, m.x, (o.y + o.h));
          }
        else if (eina_rectangles_intersect(&o, &m))
          {
             /* they intersect, we need to move this one */
             /* NB: This can happen when monitors get moved manually */

             if ((m.x <= (o.x + o.w)))
               e_layout_child_move(mon, (o.x + o.w), m.y);
             else if ((m.y <= (o.y + o.h)))
               e_layout_child_move(mon, m.x, (o.y + o.h));
          }
     }

   /* thaw layout to allow redraw */
   e_layout_thaw(sd->o_layout);
}

static void 
_e_smart_randr_layout_reposition(E_Smart_Data *sd, Evas_Object *obj)
{
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o;
   Evas_Coord mx, my;

   if (!sd) return;

   /* get this monitor geometry Before it was moved
    * 
    * NB: This is returned in Virtual coordinates */
   e_smart_monitor_move_geometry_get(obj, &mx, &my, NULL, NULL);

   /* get the geometry of this monitor */
   evas_object_geometry_get(obj, &o.x, &o.y, &o.w, &o.h);

   /* freeze layout */
   e_layout_freeze(sd->o_layout);

   /* find any monitors that this one intersects with */
   EINA_LIST_FOREACH(sd->items, l, mon)
     {
        Eina_Rectangle m;

        /* skip if it's the current monitor */
        if ((mon == obj)) continue;

        /* get the geometry of this monitor */
        evas_object_geometry_get(mon, &m.x, &m.y, &m.w, &m.h);

        if (eina_rectangles_intersect(&o, &m))
          {
             /* if these 2 monitors intersect, we need to move the second one 
              * as the user is currently moving the first one
              * 
              * NB: Currently, this will move This monitor to the 
              * position of the old one. This is probably not ideal */
             e_layout_child_move(mon, mx, my);
          }
     }

   /* thaw layout to allow redraw */
   e_layout_thaw(sd->o_layout);
}

static void 
_e_smart_randr_changed_set(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *mon = NULL;
   Eina_List *l = NULL;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   sd->changed = EINA_FALSE;
   EINA_LIST_FOREACH(sd->items, l, mon)
     {
        E_Smart_Monitor_Changes changes = E_SMART_MONITOR_CHANGED_NONE;

        changes = e_smart_monitor_changes_get(mon);
        if (changes > E_SMART_MONITOR_CHANGED_NONE)
          {
             sd->changed = EINA_TRUE;
             break;
          }
     }

   evas_object_smart_callback_call(obj, "changed", NULL);
}

/* callback received from the monitor object to let us know that it was 
 * resized, and we should adjust position of any adjacent monitors */
static void 
_e_smart_cb_monitor_resized(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   Evas_Object *o_randr;
   E_Smart_Data *sd;

   if (!(o_randr = data)) return;
   if (!(sd = evas_object_smart_data_get(o_randr)))
     return;

   _e_smart_randr_layout_adjust(sd, obj);

   _e_smart_randr_changed_set(o_randr);
}

/* callback received from the monitor object to let us know that it was 
 * rotated, and we should adjust position of any adjacent monitors */
static void 
_e_smart_cb_monitor_rotated(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Evas_Object *o_randr;

   if (!(o_randr = data)) return;

   _e_smart_randr_changed_set(o_randr);
}

/* callback received from the monitor object to let us know that it was 
 * moved, and we should adjust position of any adjacent monitors */
static void 
_e_smart_cb_monitor_moved(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   Evas_Object *o_randr;
   E_Smart_Data *sd;

   if (!(o_randr = data)) return;
   if (!(sd = evas_object_smart_data_get(o_randr)))
     return;

   if (e_smart_monitor_moving_get(obj)) 
     _e_smart_randr_layout_reposition(sd, obj);
   else 
     _e_smart_randr_layout_adjust(sd, obj);

   _e_smart_randr_changed_set(o_randr);
}

static void 
_e_smart_cb_monitor_toggled(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Evas_Object *o_randr;

   if (!(o_randr = data)) return;

   _e_smart_randr_changed_set(o_randr);
}

/* callback received from the monitor object to let us know that it was 
 * deleted, and we should cleanup */
static void 
_e_smart_cb_monitor_deleted(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_del(obj, "monitor_resized", 
                                  _e_smart_cb_monitor_resized);
   evas_object_smart_callback_del(obj, "monitor_rotated", 
                                  _e_smart_cb_monitor_rotated);
   evas_object_smart_callback_del(obj, "monitor_moved", 
                                  _e_smart_cb_monitor_moved);
   evas_object_smart_callback_del(obj, "monitor_toggled", 
                                  _e_smart_cb_monitor_toggled);
}

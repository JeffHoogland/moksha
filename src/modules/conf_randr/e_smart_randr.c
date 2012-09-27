#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* 'Smart' widget to wrap a pan and scroll into one */

typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
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
static void _e_smart_cb_monitor_resized(void *data, Evas_Object *obj, void *event __UNUSED__);

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
                                  _e_smart_cb_monitor_resized, sd);

   /* pack this monitor into the layout */
   e_layout_pack(sd->o_layout, mon);
   e_layout_child_lower(mon);

   /* append this monitor to our list */
   sd->items = eina_list_append(sd->items, mon);

   /* reconfigure the layout */
   _e_smart_reconfigure(sd);
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
   evas_object_smart_member_add(sd->o_layout, obj);

   /* add scroll object */
   sd->o_scroll = e_scrollframe_add(evas);
   /* e_scrollframe_custom_theme_set(sd->o_scroll, "base/theme/widgets",  */
   /*                                "e/conf/randr/main/scrollframe"); */
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

/* callback received from the monitor object to let us know that it was 
 * resized, and we should adjust position of any adjacent monitors */
static void 
_e_smart_cb_monitor_resized(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Smart_Data *sd;
   Evas_Coord w, h, nw, nh;
   Eina_List *l;
   /* E_Randr_Crtc_Info *crtc; */

   if (!(sd = data)) return;

   /* get randr output info for this monitor */
   /* crtc = e_smart_monitor_crtc_get(obj); */
   /* printf("Monitor Output Policy: %d\n", crtc->policy); */

   /* grab size of this monitor object */
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   /* convert to layout coords */
   e_layout_coord_canvas_to_virtual(sd->o_layout, w, h, &nw, &nh);

   /* freeze layout */
   e_layout_freeze(sd->o_layout);

   /* search for this monitor */
//   if ((l = eina_list_data_find_list(sd->items, obj)))
     {
        Evas_Object *mon;
        Evas_Coord mx, my;
        Eina_Rectangle rm;

        e_layout_child_geometry_get(obj, &mx, &my, NULL, NULL);
        /* printf("Monitor New Geom: %d %d %d %d\n", mx, my, nw, nh); */
        EINA_RECTANGLE_SET(&rm, mx, my, nw, nh);

        /* found monitor, check for one the will intersect */
        EINA_LIST_FOREACH(sd->items, l, mon)
          {
             /* E_Randr_Crtc_Info *cinfo; */
             Evas_Coord cx, cy, cw, ch;
             Eina_Rectangle rc;

             if ((mon == obj)) continue;

             /* cinfo = e_smart_monitor_crtc_get(mon); */
             /* printf("\tChild Policy: %d\n", cinfo->policy); */

             e_layout_child_geometry_get(mon, &cx, &cy, &cw, &ch);
             /* printf("\tNext Mon Geom: %d %d %d %d\n", cx, cy, cw, ch); */
             EINA_RECTANGLE_SET(&rc, cx, cy, cw, ch);

             if (eina_rectangles_intersect(&rm, &rc))
               {
                  printf("\nMONITORS INTERSECT !!\n");

                  /* if position of new monitor overlaps the existing one's 
                   * X position, we need to move the existing one over */
                  if ((mx + nw) >= cx)
                    cx = (mx + nw);
                  else if ((my + nh) >= cy)
                    cy = (my + nh);
 
                  /* update This monitors position based on new size 
                   * of the previous one (that was resized) */
                  /* e_layout_child_geometry_get(mon, &cx, &cy, NULL, NULL); */
                  e_layout_child_move(mon, cx, cy);
               }
          }
     }

   /* thaw layout to allow redraw */
   e_layout_thaw(sd->o_layout);
}

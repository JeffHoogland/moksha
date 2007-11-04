/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* TODO: support different preference modes ala edje */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;

   Evas_Object *child;
   int aspect_w, aspect_h;
   int aspect_preference;
   double align_x, align_y;
};

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   free(wd);
}

static void
_e_wid_reconfigure(E_Widget_Data *wd)
{
   int px, py, pw, ph;
   int cx, cy, cw, ch;
   double ap, ad;

   if (!wd->obj || !wd->child) return;

   evas_object_geometry_get(wd->obj, &px, &py, &pw, &ph);

   ap = (double)pw / ph;
   ad = (double)(wd->aspect_w) / wd->aspect_h;
   if (ap >= ad)
     {
	ch = ph;
	cw = (ch * wd->aspect_w) / wd->aspect_h;
     }
   else
     {
	cw = pw;
	ch = (cw * wd->aspect_h) / wd->aspect_w;
     }

   cx = px + (wd->align_x * (pw - cw));
   cy = py + (wd->align_y * (ph - ch));

   evas_object_resize(wd->child, cw, ch);
   evas_object_move(wd->child, cx, cy);
}

static void
_cb_reconfigure(void *data, Evas *a, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd = data;
   _e_wid_reconfigure(wd);
}

EAPI Evas_Object *
e_widget_aspect_add(Evas *evas, int w, int h)
{
   Evas_Object *obj;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);
   wd = calloc(1, sizeof(E_Widget_Data));

   wd->obj = obj;
   e_widget_data_set(obj, wd);
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _cb_reconfigure, wd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _cb_reconfigure, wd);
   wd->align_x = 0.5;
   wd->align_y = 0.5;
   wd->aspect_w = w;
   wd->aspect_h = h;
   _e_wid_reconfigure(wd);

   return obj;
}

EAPI void
e_widget_aspect_aspect_set(Evas_Object *obj, int w, int h)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;
   
   wd->aspect_w = w;
   wd->aspect_h = h;
   _e_wid_reconfigure(wd);
}

EAPI void
e_widget_aspect_align_set(Evas_Object *obj, double align_x, double align_y)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;
   
   wd->align_x = align_x;
   wd->align_y = align_y;
   _e_wid_reconfigure(wd);
}

EAPI void
e_widget_aspect_child_set(Evas_Object *obj, Evas_Object *child)
{
   E_Widget_Data *wd;
   int mw, mh;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   wd->child = child;
   e_widget_min_size_get(child, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   e_widget_sub_object_add(obj, child);
   _e_wid_reconfigure(wd);
}

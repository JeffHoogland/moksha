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

   cx = px + ((pw - cw) / 2);
   cy = py + ((ph - ch) / 2);

   evas_object_resize(wd->child, cw, ch);
   evas_object_move(wd->child, cx, cy);
}

static void
_e_wid_resize_intercept(void *data, Evas_Object *obj, int w, int h)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   evas_object_resize(obj, w, h);
   _e_wid_reconfigure(wd);
}

Evas_Object *
e_widget_aspect_add(Evas *evas, int w, int h)
{
   Evas_Object *obj;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);
   wd = calloc(1, sizeof(E_Widget_Data));

   wd->obj = obj;
   e_widget_data_set(obj, wd);
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   evas_object_intercept_resize_callback_add(obj, _e_wid_resize_intercept, wd);
   e_widget_aspect_aspect_set(obj, w, h);

   return obj;
}

void
e_widget_aspect_aspect_set(Evas_Object *obj, int w, int h)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;
   
   wd->aspect_w = w;
   wd->aspect_h = h;
   _e_wid_reconfigure(wd);
}

void
e_widget_aspect_child_set(Evas_Object *obj, Evas_Object *child)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   wd->child = child;
   _e_wid_reconfigure(wd);
}

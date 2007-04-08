/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;

struct _E_Widget_Data
{
   Evas_Object *o_edje;
   Evas_Object *o_spectrum;
   Evas_Object *o_event;

   E_Color *cv;
   E_Color_Component mode;

   int dragging;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_activate_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _e_wid_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void _e_wid_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);

static void _e_wid_mouse_handle(Evas_Object *obj, int mx, int my);
static void _e_wid_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info);

Evas_Object *
e_widget_spectrum_add(Evas *evas, E_Color_Component mode, E_Color *cv)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);

   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   
   wd->cv = cv;
   wd->mode = mode;

   o = edje_object_add(evas);
   wd->o_edje = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/spectrum");

   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);

   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   evas_object_intercept_resize_callback_add(o, _e_wid_resize, wd);
   evas_object_intercept_move_callback_add(o, _e_wid_move, wd);

   o = e_spectrum_add(evas);
   e_spectrum_color_value_set(o, cv);
   e_spectrum_mode_set(o, mode);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   wd->o_spectrum = o;
   edje_object_part_swallow(wd->o_edje, "e.swallow.content", o);
   edje_extern_object_min_size_set(o, 100, 100);

   o = evas_object_rectangle_add(evas);
   evas_object_color_set(o, 0, 0, 0, 0);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_cb_down, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_wid_cb_move, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_wid_cb_up, obj);
   wd->o_event = o;

   e_widget_spectrum_update(obj, 0);

   return obj;
}

void
e_widget_spectrum_update(Evas_Object *obj, int redraw)
{
   E_Widget_Data *wd;
   double vx, vy;
   
   wd = e_widget_data_get(obj);
   /* redraw spectrum */
   if (redraw)
     e_spectrum_update(wd->o_spectrum);

   switch (wd->mode)
     {
      case E_COLOR_COMPONENT_R:
	 vy = wd->cv->g / 255.0;
	 vx = wd->cv->b / 255.0;
	 break;
      case E_COLOR_COMPONENT_G:
	 vy = wd->cv->b / 255.0;
	 vx = wd->cv->r / 255.0;
	 break;
      case E_COLOR_COMPONENT_B:
	 vy = wd->cv->r / 255.0;
	 vx = wd->cv->g / 255.0;
	 break;
      case E_COLOR_COMPONENT_H:
	 vy = wd->cv->s;
	 vx = wd->cv->v;
	 break;
      case E_COLOR_COMPONENT_S:
	 vy = wd->cv->v;
	 vx = wd->cv->h / 360.0;
	 break;
      case E_COLOR_COMPONENT_V:
	 vy = wd->cv->h / 360.0;
	 vx = wd->cv->s;
	 break;
     case E_COLOR_COMPONENT_MAX:
	break;
     }
   edje_object_part_drag_value_set(wd->o_edje, "cursor", vx, vy);

}

void
e_widget_spectrum_mode_set(Evas_Object *obj, E_Color_Component mode)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (wd->mode == mode) return;
   wd->mode = mode;
   e_spectrum_mode_set(wd->o_spectrum, mode);
   e_widget_spectrum_update(obj, 0);
}

static void
_e_wid_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   E_Widget_Data *wd = data;
   evas_object_move(wd->o_edje, x, y);
   evas_object_move(wd->o_event, x, y);
}

static void
_e_wid_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   E_Widget_Data *wd = data;
   evas_object_resize(wd->o_edje, w, h);
   evas_object_resize(wd->o_event, w, h);
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
	edje_object_signal_emit(wd->o_edje, "e,state,focused", "e");
	evas_object_focus_set(wd->o_edje, 1);
     }
   else
     {
	edje_object_signal_emit(wd->o_edje, "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_edje, 0);
     }
}

static void 
_e_wid_activate_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_spectrum, "e,state,disabled", "e");
   else
     edje_object_signal_emit(wd->o_spectrum, "e,state,enabled", "e");
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_mouse_handle(Evas_Object *obj, int mx, int my)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;
   double vx, vy;

   wd = e_widget_data_get(obj);

   evas_object_geometry_get(wd->o_spectrum, &x, &y, &w, &h);
   vx = (mx - x) / (double)w;
   vy = (my - y) / (double)h;
   if (vx > 1) vx = 1;
   if (vx < 0) vx = 0;
   if (vy > 1) vy = 1;
   if (vy < 0) vy = 0;

   edje_object_part_drag_value_set(wd->o_edje, "e.dragable.cursor", vx, vy);

   switch (wd->mode)
     {
      case E_COLOR_COMPONENT_R:
	 wd->cv->g = vy * 255;
	 wd->cv->b = vx * 255;
	 e_color_update_rgb(wd->cv);
	 break;
      case E_COLOR_COMPONENT_G:
	 wd->cv->b = vy * 255;
	 wd->cv->r = vx * 255;
	 e_color_update_rgb(wd->cv);
	 break;
      case E_COLOR_COMPONENT_B:
	 wd->cv->r = vy * 255;
	 wd->cv->g = vx * 255;
	 e_color_update_rgb(wd->cv);
	 break;
      case E_COLOR_COMPONENT_H:
	 wd->cv->s = vy;
	 wd->cv->v = vx;
	 e_color_update_hsv(wd->cv);
	 break;
      case E_COLOR_COMPONENT_S:
	 wd->cv->v = vy;
	 wd->cv->h = vx * 360;
	 e_color_update_hsv(wd->cv);
	 break;
      case E_COLOR_COMPONENT_V:
	 wd->cv->h = vy * 360;
	 wd->cv->s = vx;
	 e_color_update_hsv(wd->cv);
	 break;
      case E_COLOR_COMPONENT_MAX:
	 break;
     }
   e_widget_change(obj);
}


static void
_e_wid_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Evas_Object *o_wid;
   E_Widget_Data *wd;

   o_wid = data;
   wd = e_widget_data_get(o_wid);
   ev = event_info;

   wd->dragging = 1;
   _e_wid_mouse_handle(o_wid, ev->canvas.x, ev->canvas.y);
}

static void
_e_wid_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Object *o_wid;
   E_Widget_Data *wd;
   
   o_wid = data;
   wd = e_widget_data_get(o_wid);
   wd->dragging = 0;
}

static void
_e_wid_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Object *o_wid;
   E_Widget_Data *wd;
   
   o_wid = data;
   wd = e_widget_data_get(o_wid);
   ev = event_info;
  
   if (wd->dragging == 1)
     {
	_e_wid_mouse_handle(o_wid, ev->cur.canvas.x, ev->cur.canvas.y);
     }
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;

struct _E_Widget_Data
{
   Evas_Object *o_cslider;
   Evas_Object *o_grad;
   Evas_Object *o_event;

   int vertical;
   int fixed;
   E_Color_Component mode;
   int valnum;
   E_Color *color;

   int dragging;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_activate_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_value_set(Evas_Object *obj, double vx);
static void _e_wid_update(E_Widget_Data *wd);
static void _e_wid_update_standard(E_Widget_Data *wd);
static void _e_wid_update_fixed(E_Widget_Data *wd);
static void _e_wid_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void _e_wid_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);

static void _e_wid_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info);

Evas_Object *
e_widget_cslider_add(Evas *evas, E_Color_Component mode, E_Color *color, int vertical, int fixed)
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

   wd->vertical = vertical;
   wd->fixed = fixed;
   wd->mode = mode;
   wd->color = color;
   
   o = edje_object_add(evas);
   wd->o_cslider = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/cslider");
   if (wd->vertical)
     edje_object_signal_emit(o, "e,state,direction,v", "e");
   else
     edje_object_signal_emit(o, "e,state,direction,h", "e");
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);

   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);

   /* add gradient obj */
   o = evas_object_gradient_add(evas);
   e_widget_sub_object_add(obj, o);

   if (wd->vertical)
     evas_object_gradient_angle_set(o, 0);
   else
     evas_object_gradient_angle_set(o, 270);

   evas_object_show(o);
   wd->o_grad = o;
   edje_object_part_swallow(wd->o_cslider, "e.swallow.content", o);
   evas_object_intercept_resize_callback_add(o, _e_wid_resize, wd);
   evas_object_intercept_move_callback_add(o, _e_wid_move, wd);
   _e_wid_update(wd);

   o = evas_object_rectangle_add(evas);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_show(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_cb_down, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_wid_cb_move, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_wid_cb_up, obj);
   wd->o_event = o;

   return obj;
}

static void
_e_wid_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   E_Widget_Data *wd = data;
   evas_object_move(wd->o_grad, x, y);
   evas_object_move(wd->o_event, x, y);
}

static void
_e_wid_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   E_Widget_Data *wd = data;
   evas_object_gradient_fill_set(o, 0, 0, w, h); 
   evas_object_resize(o, w, h);
   evas_object_resize(wd->o_event, w, h);
}

static void
_e_wid_value_set(Evas_Object *o, double vx)
{
   E_Widget_Data *wd;
   wd = e_widget_data_get(o);

   switch (wd->mode)
     {
      case E_COLOR_COMPONENT_R:
	 wd->color->r = 255 * vx;
	 e_color_update_rgb(wd->color);
	 break;
      case E_COLOR_COMPONENT_G:
	 wd->color->g = 255 * vx;
	 e_color_update_rgb(wd->color);
	 break;
      case E_COLOR_COMPONENT_B:
	 wd->color->b = 255 * vx;
	 e_color_update_rgb(wd->color);
	 break;
      case E_COLOR_COMPONENT_H:
	 wd->color->h = 360 * vx;
	 e_color_update_hsv(wd->color);
	 break;
      case E_COLOR_COMPONENT_S:
	 wd->color->s = vx;
	 e_color_update_hsv(wd->color);
	 break;
      case E_COLOR_COMPONENT_V:
	 wd->color->v = vx;
	 e_color_update_hsv(wd->color);
	 break;
      case E_COLOR_COMPONENT_MAX:
	 break;
     }

   _e_wid_update(wd);
   e_widget_change(o);
}

void
e_widget_cslider_color_value_set(Evas_Object *obj, E_Color *val)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   wd->color = val;
   _e_wid_update(wd);
}

void
e_widget_cslider_update(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   
   _e_wid_update(wd);
}

void
e_widget_cslider_mode_set(Evas_Object *obj, E_Color_Component mode)
{
   E_Widget_Data *wd;
   wd = e_widget_data_get(obj);
   if (wd->mode == mode) return;
   wd->mode = mode;
   _e_wid_update(wd);
}

static void
_e_wid_update(E_Widget_Data *wd)
{
   if (wd->fixed)
     _e_wid_update_fixed(wd);
   else
     _e_wid_update_standard(wd);
}

static void
_e_wid_update_standard(E_Widget_Data *wd)
{
   int r, g, b;
   int max, min;
   float vx;

   if (!wd->color) return;

   evas_object_gradient_clear(wd->o_grad);

   switch (wd->mode) 
     {
      case E_COLOR_COMPONENT_R:
	 evas_object_gradient_color_stop_add(wd->o_grad, 0, wd->color->g, wd->color->b, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, 255, wd->color->g, wd->color->b, 255, 1);
	 vx = wd->color->r / 255.0;
	 break;
      case E_COLOR_COMPONENT_G:
	 evas_object_gradient_color_stop_add(wd->o_grad, wd->color->r, 0, wd->color->b, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, wd->color->r, 255, wd->color->b, 255, 1);
	 vx = wd->color->g / 255.0;
	 break;
      case E_COLOR_COMPONENT_B:
	 evas_object_gradient_color_stop_add(wd->o_grad, wd->color->r, wd->color->g, 0, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, wd->color->r, wd->color->g, 255, 255, 1);
	 vx = wd->color->b / 255.0;
	 break;
      case E_COLOR_COMPONENT_H:
	 evas_color_hsv_to_rgb(0, wd->color->s, wd->color->v, &max, &min, NULL);

	 evas_object_gradient_color_stop_add(wd->o_grad, max, min, min, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, max, max, min, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, min, max, min, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, min, max, max, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, min, min, max, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, max, min, max, 255, 1);
	 evas_object_gradient_color_stop_add(wd->o_grad, max, min, min, 255, 1);
	 vx = wd->color->h / 360.0;
	 break;
      case E_COLOR_COMPONENT_S:
	 evas_color_hsv_to_rgb(wd->color->h, 0, wd->color->v, &r, &g, &b);
	 evas_object_gradient_color_stop_add(wd->o_grad, r, g, b, 255, 1);
	 evas_color_hsv_to_rgb(wd->color->h, 1, wd->color->v, &r, &g, &b);
	 evas_object_gradient_color_stop_add(wd->o_grad, r, g, b, 255, 1);
	 vx = wd->color->s;
	 break;
      case E_COLOR_COMPONENT_V:
	 evas_color_hsv_to_rgb(wd->color->h, wd->color->s, 0, &r, &g, &b);
	 evas_object_gradient_color_stop_add(wd->o_grad, r, g, b, 255, 1);
	 evas_color_hsv_to_rgb(wd->color->h, wd->color->s, 1, &r, &g, &b);
	 evas_object_gradient_color_stop_add(wd->o_grad, r, g, b, 255, 1);
	 vx = wd->color->v;
	 break;
      case E_COLOR_COMPONENT_MAX:
	 break;
     }

     edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", vx, vx);
}

void
_e_wid_update_fixed(E_Widget_Data *wd)
{
  int max, min;
  float vx;
  if (!wd) return;

  evas_object_gradient_clear(wd->o_grad);
  switch (wd->mode)
    {
     case E_COLOR_COMPONENT_R:
	evas_object_gradient_color_stop_add(wd->o_grad, 255, 0, 0, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 0, 255, 1);
	vx = wd->color->r / 255.0;
	break;
     case E_COLOR_COMPONENT_G:
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 255, 0, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 0, 255, 1);
	vx = wd->color->g / 255.0;
	break;
     case E_COLOR_COMPONENT_B:
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 255, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 0, 255, 1);
	vx = wd->color->b / 255.0;
	break;
     case E_COLOR_COMPONENT_H:
	/*
	 * Color Stops:
	 *   0 x n n
	 *  60 x x n
	 * 120 n x n
	 * 180 n x x
	 * 240 n n x
	 * 300 x n x
	 * 360 x n n
	 */
	min = 0;
	max = 255;

	evas_object_gradient_color_stop_add(wd->o_grad, max, min, min, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, max, min, max, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, min, min, max, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, min, max, max, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, min, max, min, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, max, max, min, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, max, min, min, 255, 1);
	vx = wd->color->h / 360.0;
	break;
     case E_COLOR_COMPONENT_S:
	evas_object_gradient_color_stop_add(wd->o_grad, 255, 255, 255, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 0, 255, 1);
	vx = wd->color->s; 
	break;
     case E_COLOR_COMPONENT_V:
	evas_object_gradient_color_stop_add(wd->o_grad, 255, 255, 255, 255, 1);
	evas_object_gradient_color_stop_add(wd->o_grad, 0, 0, 0, 255, 1);
	vx = wd->color->v;
	break;
     case E_COLOR_COMPONENT_MAX:
	break;
    }

  edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", vx, vx);
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
	edje_object_signal_emit(wd->o_cslider, "e,state,focused", "e");
	evas_object_focus_set(wd->o_cslider, 1);
     }
   else
     {
	edje_object_signal_emit(wd->o_cslider, "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_cslider, 0);
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
     edje_object_signal_emit(wd->o_cslider, "e,state,disabled", "e");
   else
     edje_object_signal_emit(wd->o_cslider, "e,state,enabled", "e");
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Evas_Object *o_wid;
   E_Widget_Data *wd;
   Evas_Coord ox, oy, ow, oh;
   double val;

   ev = event_info;

   o_wid = data;
   wd = e_widget_data_get(o_wid);
   wd->dragging = 1;

   evas_object_geometry_get(wd->o_grad, &ox, &oy, &ow, &oh);

   if (wd->vertical) 
     val = 1 - ((ev->canvas.y - oy) / (double)oh);
   else
     val = (ev->canvas.x - ox) / (double)ow;
   if (val > 1) val = 1;
   if (val < 0) val = 0;
   _e_wid_value_set(o_wid, val);
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
	Evas_Coord ox, oy, ow, oh;
	double val;
	evas_object_geometry_get(wd->o_grad, &ox, &oy, &ow, &oh);

	if (wd->vertical) 
	  val = 1 - ((ev->cur.canvas.y - oy) / (double)oh);
	else
	  val = (ev->cur.canvas.x - ox) / (double)ow;
	if (val > 1) val = 1;
	if (val < 0) val = 0;
	_e_wid_value_set(o_wid, val);
     }
}

#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;

struct _E_Widget_Data
{
   Evas_Object *o_cslider;
   Evas_Object *o_grad;
   Evas_Object *o_event;

   Evas_Coord x, y, w, h;

   int vertical;
   int fixed;
   E_Color_Component mode;
   int valnum;
   E_Color *color;
   E_Color *prev;

   int dragging;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_value_set(Evas_Object *obj, double vx);
static void _e_wid_update(E_Widget_Data *wd);
static void _e_wid_update_standard(E_Widget_Data *wd);
static void _e_wid_update_fixed(E_Widget_Data *wd);

static void _e_wid_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_drag_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_cb_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_cb_drag(void *data, Evas_Object *obj, const char *emission, const char *source);

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
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _e_wid_resize, wd);

   wd->vertical = vertical;
   wd->fixed = fixed;
   wd->mode = mode;
   wd->color = color;
   wd->prev = calloc(1, sizeof (E_Color));

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
   e_widget_size_min_set(obj, mw, mh);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _e_wid_move, wd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _e_wid_resize, wd);

   edje_object_signal_callback_add(wd->o_cslider, "drag,start", "*", _e_wid_cb_drag_start, obj);
   edje_object_signal_callback_add(wd->o_cslider, "drag", "*", _e_wid_cb_drag, obj);
   edje_object_signal_callback_add(wd->o_cslider, "drag,stop", "*", _e_wid_cb_drag_stop, obj);

   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);

   /* add gradient obj */
   o = evas_object_image_filled_add(evas);
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_cb_down, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_wid_cb_move, obj);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_wid_cb_up, obj);
   evas_object_color_set(o, 255, 255, 255, 255);
   wd->o_grad = o;

   edje_object_part_swallow(wd->o_cslider, "e.swallow.content", o);

   o = evas_object_rectangle_add(evas);
   evas_object_repeat_events_set(o, EINA_TRUE);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   evas_object_color_set(o, 0, 0, 0, 0);
   wd->o_event = o;

   _e_wid_update(wd);

   return obj;
}

static void
_e_wid_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Coord x, y;

   wd = data;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(wd->o_event, x, y);
   _e_wid_update(wd);
}

static void
_e_wid_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Coord w, h;
   
   wd = data;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(wd->o_event, w, h);
   _e_wid_update(wd);
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
   Evas_Coord x, y, w, h;
   Eina_Bool changed = EINA_FALSE;

   evas_object_geometry_get(wd->o_event, &x, &y, &w, &h);
   if (x != wd->x || y != wd->y || w != wd->w || h != wd->h)
     changed = EINA_TRUE;

   if (memcmp(wd->color, wd->prev, sizeof (E_Color)))
     changed = EINA_TRUE;

   if (changed)
     {
	if (wd->fixed)
	  _e_wid_update_fixed(wd);
	else
	  _e_wid_update_standard(wd);

	wd->x = x; wd->y = y;
	wd->w = w; wd->h = h;
	memcpy(wd->prev, wd->color, sizeof (E_Color));
     }
}

static void
_e_wid_gradient_set(Evas_Object *o, Eina_Bool orientation,
		    int rf, int gf, int bf,
		    int rt, int gt, int bt)
{
   unsigned int *pixels, *p;
   int x, r, g, b;
   
   if (!orientation)
     evas_object_image_size_set(o, 256, 1);
   else
     evas_object_image_size_set(o, 1, 256);
   pixels = evas_object_image_data_get(o, EINA_TRUE);
   if (pixels)
     {
        p = pixels;
        for (x = 0; x < 256; x++)
          {
             r = ((rf * (255 - x)) + (rt * x)) / 255;
             g = ((gf * (255 - x)) + (gt * x)) / 255;
             b = ((bf * (255 - x)) + (bt * x)) / 255;
             *p = 0xff000000 | (r << 16) | (g << 8) | (b);
             p++;
          }
        evas_object_image_data_set(o, pixels);
        evas_object_image_data_update_add(o, 0, 0, 256, 256);
     }
}

static void
_e_wid_gradient_range_set(Evas_Object *o, Eina_Bool orientation,
                          int rf, int gf, int bf,
                          int rt, int gt, int bt,
                          int from, int to)
{
   unsigned int *pixels, *p;
   int x, r, g, b, v, t;

   if (from < 0) from = 0;
   if (from > 255) from = 255;
   if (to < 0) to = 0;
   if (to > 256) to = 256;
   if (to <= from) return;
   if (!orientation)
     evas_object_image_size_set(o, 256, 1);
   else
     evas_object_image_size_set(o, 1, 256);
   pixels = evas_object_image_data_get(o, EINA_TRUE);
   if (pixels)
     {
        t = to - from;
        p = pixels + from;
        for (x = from; x < to; x++)
          {
             v = x - from;
             r = ((rf * (t - v)) + (rt * v)) / t;
             g = ((gf * (t - v)) + (gt * v)) / t;
             b = ((bf * (t - v)) + (bt * v)) / t;
             *p = 0xff000000 | (r << 16) | (g << 8) | (b);
             p++;
          }
        evas_object_image_data_set(o, pixels);
        evas_object_image_data_update_add(o, 0, 0, 256, 256);
     }
}

static void
_e_wid_update_standard(E_Widget_Data *wd)
{
   int r, g, b;
   int rd, gd, bd;
   int max, min;
   unsigned int i;
   float vx = 0;
   int *grad[7][3] = {
     { &max, &min, &min },
     { &max, &max, &min },
     { &min, &max, &min },
     { &min, &max, &max },
     { &min, &min, &max },
     { &max, &min, &max },
     { &max, &min, &min }
   };

   if (!wd->color) return;

   switch (wd->mode)
     {
      case E_COLOR_COMPONENT_R:
	 _e_wid_gradient_set(wd->o_grad, wd->vertical,
			     0, wd->color->g, wd->color->b,
			     255, wd->color->g, wd->color->b);
	 vx = wd->color->r / 255.0;
	 break;
      case E_COLOR_COMPONENT_G:
	 _e_wid_gradient_set(wd->o_grad, wd->vertical,
			     wd->color->r, 0, wd->color->b,
			     wd->color->r, 255, wd->color->b);
	 vx = wd->color->g / 255.0;
	 break;
      case E_COLOR_COMPONENT_B:
	 _e_wid_gradient_set(wd->o_grad, wd->vertical,
			     wd->color->r, wd->color->g, 0,
			     wd->color->r, wd->color->g, 255);
	 vx = wd->color->b / 255.0;
	 break;
      case E_COLOR_COMPONENT_H:
	 evas_color_hsv_to_rgb(0, wd->color->s, wd->color->v, &max, &min, NULL);
        for (i = 0; i < 6; i++)
          _e_wid_gradient_range_set(wd->o_grad, wd->vertical,
                                    *grad[i][0], *grad[i][1], *grad[i][2],
                                    *grad[i + 1][0], *grad[i + 1][1], *grad[i + 1][2],
                                    ((i + 0) * 256) / 6,
                                    ((i + 1) * 256) / 6);
	 vx = wd->color->h / 360.0;
	 break;
      case E_COLOR_COMPONENT_S:
	 evas_color_hsv_to_rgb(wd->color->h, 0, wd->color->v, &r, &g, &b);
	 evas_color_hsv_to_rgb(wd->color->h, 1, wd->color->v, &rd, &gd, &bd);

	 _e_wid_gradient_set(wd->o_grad, wd->vertical,
			     r, g, b,
			     rd, gd, bd);
	 vx = wd->color->s;
	 break;
      case E_COLOR_COMPONENT_V:
	 evas_color_hsv_to_rgb(wd->color->h, wd->color->s, 0, &r, &g, &b);
	 evas_color_hsv_to_rgb(wd->color->h, wd->color->s, 1, &rd, &gd, &bd);

	 _e_wid_gradient_set(wd->o_grad, wd->vertical,
			     r, g, b,
			     rd, gd, bd);
	 vx = wd->color->v;
	 break;
      default:
	 break;
     }

   edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", vx, vx);
   edje_object_message_signal_process(wd->o_cslider); /* really needed or go in infinite loop */
}

void
_e_wid_update_fixed(E_Widget_Data *wd)
{
#define GMAX 255
#define GMIN 0
   unsigned int i;
   float vx = 0;
   int grad[7][3] = {
     { GMAX, GMIN, GMIN },
     { GMAX, GMIN, GMAX },
     { GMIN, GMIN, GMAX },
     { GMIN, GMAX, GMAX },
     { GMIN, GMAX, GMIN },
     { GMAX, GMAX, GMIN },
     { GMAX, GMIN, GMIN }
   };

   if (!wd) return;

   switch (wd->mode)
    {
     case E_COLOR_COMPONENT_R:
	_e_wid_gradient_set(wd->o_grad, wd->vertical,
			    255, 0, 0,
			    0, 0, 0);
	vx = wd->color->r / 255.0;
	break;
     case E_COLOR_COMPONENT_G:
	_e_wid_gradient_set(wd->o_grad, wd->vertical,
			    0, 255, 0,
			    0, 0, 0);
	vx = wd->color->g / 255.0;
	break;
     case E_COLOR_COMPONENT_B:
	_e_wid_gradient_set(wd->o_grad, wd->vertical,
			    0, 0, 255,
			    0, 0, 0);
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
        for (i = 0; i < 6; i++)
          _e_wid_gradient_range_set(wd->o_grad, wd->vertical,
                                    grad[i][0], grad[i][1], grad[i][2],
                                    grad[i + 1][0], grad[i + 1][1], grad[i + 1][2],
                                    ((i + 0) * 256) / 6,
                                    ((i + 1) * 256) / 6);
	vx = wd->color->h / 360.0;
	break;
     case E_COLOR_COMPONENT_S:
	_e_wid_gradient_set(wd->o_grad, wd->vertical,
			    255, 255, 255,
			    0, 0, 0);
	vx = wd->color->s;
	break;
     case E_COLOR_COMPONENT_V:
	_e_wid_gradient_set(wd->o_grad, wd->vertical,
			    255, 255, 255,
			    0, 0, 0);
	vx = wd->color->v;
	break;
     default:
	break;
    }

   edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", vx, vx);

#undef GMAX
#undef GMIN
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
_e_wid_focus_steal(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_cb_drag_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *o_wid;
   E_Widget_Data *wd;
   double val, valx, valy;

   o_wid = data;
   wd = e_widget_data_get(o_wid);
   wd->dragging = 1;

   edje_object_part_drag_value_get(wd->o_cslider, "e.dragable.cursor",
                                   &valx, &valy);
   if (wd->vertical) val = valy;
   else val = valx;
   if (val > 1) val = 1;
   if (val < 0) val = 0;
   _e_wid_value_set(o_wid, val);
}

static void
_e_wid_cb_drag_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *o_wid;
   E_Widget_Data *wd;

   o_wid = data;
   wd = e_widget_data_get(o_wid);
   wd->dragging = 0;
}

static void
_e_wid_cb_drag(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *o_wid;
   E_Widget_Data *wd;

   o_wid = data;
   wd = e_widget_data_get(o_wid);

   if (wd->dragging == 1)
     {
	double val, valx, valy;

        edje_object_part_drag_value_get(wd->o_cslider, "e.dragable.cursor",
                                        &valx, &valy);
	if (wd->vertical) val = valy;
	else val = valx;
	if (val > 1) val = 1;
	if (val < 0) val = 0;
	_e_wid_value_set(o_wid, val);
     }
}

static void
_e_wid_mouse_handle(Evas_Object *obj, int mx, int my)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;
   double vx = 0.0, vy = 0.0;
   
   wd = e_widget_data_get(obj);
   
   evas_object_geometry_get(wd->o_grad, &x, &y, &w, &h);
   if (w > 1) vx = (mx - x) / (double)(w - 1);
   if (h > 1) vy = (my - y) / (double)(h - 1);
   if (vx > 1) vx = 1;
   if (vx < 0) vx = 0;
   if (vy > 1) vy = 1;
   if (vy < 0) vy = 0;
   

   if (wd->vertical)
     {
        _e_wid_value_set(obj, 1.0 - vy);
        edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", 0.5, 1.0 - vy);
     }
   else
     {
        _e_wid_value_set(obj, vx);
        edje_object_part_drag_value_set(wd->o_cslider, "e.dragable.cursor", vx, 0.5);
     }
   e_widget_change(obj);
}


static void
_e_wid_cb_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
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
_e_wid_cb_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *o_wid;
   E_Widget_Data *wd;
   
   o_wid = data;
   wd = e_widget_data_get(o_wid);
   wd->dragging = 0;
}

static void
_e_wid_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
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

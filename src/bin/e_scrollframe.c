/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_scrollframe"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object *smart_obj;
   Evas_Object *child_obj;
   Evas_Object *pan_obj;
   Evas_Object *edje_obj;
   Evas_Object *event_obj;

   E_Scrollframe_Policy hbar_flags, vbar_flags;
   
   struct {
      Evas_Coord w, h;
   } child;
   struct {
      Evas_Coord x, y;
   } step, page;

   struct {
      void (*set) (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
      void (*get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
      void (*max_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
      void (*child_size_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
   } pan_func;
   
   unsigned char hbar_visible : 1;
   unsigned char vbar_visible : 1;
   unsigned char extern_pan : 1;
}; 

/* local subsystem functions */
static void _e_smart_child_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_pan_changed_hook(void *data, Evas_Object *obj, void *event_info);
static void _e_smart_pan_pan_changed_hook(void *data, Evas_Object *obj, void *event_info);
static void _e_smart_event_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_edje_drag_v(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_smart_edje_drag_h(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_smart_scrollbar_read(E_Smart_Data *sd);
static void _e_smart_scrollbar_reset(E_Smart_Data *sd);
static int  _e_smart_scrollbar_bar_h_visibility_adjust(E_Smart_Data *sd);
static int  _e_smart_scrollbar_bar_v_visibility_adjust(E_Smart_Data *sd);
static void _e_smart_scrollbar_bar_visibility_adjust(E_Smart_Data *sd);
static void _e_smart_scrollbar_size_adjust(E_Smart_Data *sd);
static void _e_smart_reconfigure(E_Smart_Data *sd);
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_smart_clip_unset(Evas_Object *obj);
static void _e_smart_init(void);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_scrollframe_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_scrollframe_child_set(Evas_Object *obj, Evas_Object *child)
{
   Evas_Coord w, h;
   Evas_Object *o;
   
   API_ENTRY return;
   if (sd->child_obj)
     {
	e_pan_child_set(sd->pan_obj, NULL);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_FREE, _e_smart_child_del_hook);
     }
   
   sd->child_obj = child;
   if (!child) return;

   if (!sd->pan_obj)
     {
	o = e_pan_add(evas_object_evas_get(obj));
	sd->pan_obj = o;
	evas_object_smart_callback_add(o, "changed", _e_smart_pan_changed_hook, sd);
	evas_object_smart_callback_add(o, "changed", _e_smart_pan_pan_changed_hook, sd);
	evas_object_show(o);
	edje_object_part_swallow(sd->edje_obj, "item", o);
     }
   
   sd->pan_func.set = e_pan_set;
   sd->pan_func.get = e_pan_get;
   sd->pan_func.max_get = e_pan_max_get;
   sd->pan_func.child_size_get = e_pan_child_size_get;
   
   evas_object_event_callback_add(child, EVAS_CALLBACK_FREE, _e_smart_child_del_hook, sd);
   e_pan_child_set(sd->pan_obj, sd->child_obj);
   sd->pan_func.child_size_get(sd->pan_obj, &w, &h);
   sd->child.w = w;
   sd->child.h = h;
   _e_smart_scrollbar_size_adjust(sd);
   _e_smart_scrollbar_reset(sd);
}

EAPI void
e_scrollframe_extern_pan_set(Evas_Object *obj, Evas_Object *pan,
			     void (*pan_set) (Evas_Object *obj, Evas_Coord x, Evas_Coord y),
			     void (*pan_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y),
			     void (*pan_max_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y),
			     void (*pan_child_size_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y))
{
   API_ENTRY return;
   
   e_scrollframe_child_set(obj, NULL);
   if (sd->extern_pan)
     {
	if (sd->pan_obj)
	  {
	     edje_object_part_unswallow(sd->edje_obj, sd->pan_obj);
	     sd->pan_obj = NULL;
	  }
     }
   else
     {
	if (sd->pan_obj)
	  {
	     evas_object_del(sd->pan_obj);
	     sd->pan_obj = NULL;
	  }
     }
   if (!pan)
     {
	sd->extern_pan = 0;
	return;
     }

   sd->pan_obj = pan;
   sd->pan_func.set = pan_set;
   sd->pan_func.get = pan_get;
   sd->pan_func.max_get = pan_max_get;
   sd->pan_func.child_size_get = pan_child_size_get;
   sd->extern_pan = 1;
   evas_object_smart_callback_add(sd->pan_obj, "changed", _e_smart_pan_changed_hook, sd);
   evas_object_smart_callback_add(sd->pan_obj, "pan_changed", _e_smart_pan_pan_changed_hook, sd);
   edje_object_part_swallow(sd->edje_obj, "item", sd->pan_obj);
   evas_object_show(sd->pan_obj);
}

EAPI void
e_scrollframe_custom_theme_set(Evas_Object *obj, char *custom_category, char *custom_group)
{
   API_ENTRY return;
   
   e_theme_edje_object_set(sd->edje_obj, custom_category, custom_group);
}

EAPI void
e_scrollframe_child_pos_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Evas_Coord mx = 0, my = 0;
   double vx, vy;
   
   API_ENTRY return;
   sd->pan_func.max_get(sd->pan_obj, &mx, &my);
   if (mx > 0) vx = (double)x / (double)mx;
   else vx = 0.0;
   if (vx < 0.0) vx = 0.0;
   else if (vx > 1.0) vx = 1.0;
   if (my > 0) vy = (double)y / (double)my;
   else vy = 0.0;
   if (vy < 0.0) vy = 0.0;
   else if (vy > 1.0) vy = 1.0;
   edje_object_part_drag_value_set(sd->edje_obj, "vbar_bar", 0.0, vy);
   edje_object_part_drag_value_set(sd->edje_obj, "hbar_bar", vx, 0.0);
   sd->pan_func.set(sd->pan_obj, x, y);
}

EAPI void
e_scrollframe_child_pos_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   API_ENTRY return;
   sd->pan_func.get(sd->pan_obj, x, y);
}

EAPI void
e_scrollframe_child_region_show(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Coord mx = 0, my = 0, cw = 0, ch = 0, px = 0, py = 0, nx, ny;
   
   API_ENTRY return;
   sd->pan_func.max_get(sd->pan_obj, &mx, &my);
   sd->pan_func.child_size_get(sd->pan_obj, &cw, &ch);
   sd->pan_func.get(sd->pan_obj, &px, &py);
   
   nx = px;
   if (x < px) nx = x;
   else if ((x + w) > (px + (cw - mx)))
     {
	nx = x + w - (cw - mx);
	if (nx > x) nx = x;
     }
   ny = py;
   if (y < py) ny = y;
   else if ((y + h) > (py + (ch - my))) 
     {
	ny = y + h - (ch - my);
	if (ny > y) ny = y;
     }
   if ((nx == px) && (ny == py)) return;
   e_scrollframe_child_pos_set(obj, nx, ny);
}

EAPI void
e_scrollframe_child_viewport_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   edje_object_calc_force(sd->edje_obj);
   evas_object_geometry_get(sd->pan_obj, NULL, NULL, w, h);
}

EAPI void
e_scrollframe_step_size_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   API_ENTRY return;
   if (x < 1) x = 1;
   if (y < 1) y = 1;
   sd->step.x = x;
   sd->step.y = y;
   _e_smart_scrollbar_size_adjust(sd);
}

EAPI void
e_scrollframe_step_size_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   API_ENTRY return;
   if (x) *x = sd->step.x;
   if (y) *y = sd->step.y;
}

EAPI void
e_scrollframe_page_size_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   API_ENTRY return;
   sd->page.x = x;
   sd->page.y = y;
   _e_smart_scrollbar_size_adjust(sd);
}

EAPI void
e_scrollframe_page_size_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   API_ENTRY return;
   if (x) *x = sd->page.x;
   if (y) *y = sd->page.y;
}

EAPI void
e_scrollframe_policy_set(Evas_Object *obj, E_Scrollframe_Policy hbar, E_Scrollframe_Policy vbar)
{
   API_ENTRY return;
   if ((sd->hbar_flags == hbar) && (sd->vbar_flags == vbar)) return;
   sd->hbar_flags = hbar;
   sd->vbar_flags = vbar;
   _e_smart_scrollbar_size_adjust(sd);
}

EAPI void
e_scrollframe_policy_get(Evas_Object *obj, E_Scrollframe_Policy *hbar, E_Scrollframe_Policy *vbar)
{
   API_ENTRY return;
   if (hbar) *hbar = sd->hbar_flags;
   if (vbar) *vbar = sd->vbar_flags;
}

EAPI Evas_Object *
e_scrollframe_edje_object_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->edje_obj;
}

/* local subsystem functions */
static void
_e_smart_edje_drag_v(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
   _e_smart_scrollbar_read(sd);
}

static void
_e_smart_edje_drag_h(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Smart_Data *sd;
   
   sd = data;
   _e_smart_scrollbar_read(sd);
}

static void
_e_smart_child_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Smart_Data *sd;
   
   sd = data;
   sd->child_obj = NULL;
   _e_smart_scrollbar_size_adjust(sd);
   _e_smart_scrollbar_reset(sd);
}

static void
_e_smart_pan_changed_hook(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   E_Smart_Data *sd;
   
   sd = data;
   sd->pan_func.child_size_get(sd->pan_obj, &w, &h);
   if ((w != sd->child.w) || (h != sd->child.h))
     {
	sd->child.w = w;
	sd->child.h = h;
	_e_smart_scrollbar_size_adjust(sd);
     }
}

static void
_e_smart_pan_pan_changed_hook(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord x, y;
   E_Smart_Data *sd;
   
   sd = data;
   sd->pan_func.get(sd->pan_obj, &x, &y);
   e_scrollframe_child_pos_set(sd->smart_obj, x, y);
}

static void
_e_smart_event_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev;
   E_Smart_Data *sd;
   Evas_Coord x = 0, y = 0;
   
   sd = data;
   ev = event_info;
   e_scrollframe_child_pos_get(sd->smart_obj, &x, &y);
   y += ev->z * sd->step.y;
   e_scrollframe_child_pos_set(sd->smart_obj, x, y);
}

static void
_e_smart_event_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Smart_Data *sd;
   Evas_Coord x = 0, y = 0, vw = 0, vh = 0, mx = 0, my = 0;
   
   sd = data;
   ev = event_info;
   e_scrollframe_child_pos_get(sd->smart_obj, &x, &y);
   sd->pan_func.max_get(sd->pan_obj, &mx, &my);
   evas_object_geometry_get(sd->pan_obj, NULL, NULL, &vw, &vh);
   if (!strcmp(ev->keyname, "Left"))
     x -= sd->step.x;
   else if (!strcmp(ev->keyname, "Right"))
     x += sd->step.x;
   else if (!strcmp(ev->keyname, "Up"))
     y -= sd->step.y;
   else if (!strcmp(ev->keyname, "Home"))
     y = 0;
   else if (!strcmp(ev->keyname, "End"))
     y = my;
   else if (!strcmp(ev->keyname, "Down"))
     y += sd->step.y;
   else if (!strcmp(ev->keyname, "Prior"))
     {
	if (sd->page.y < 0)
	  y -= -(sd->page.y * vh) / 100;
	else
	  y -= sd->page.y;
     }
   else if (!strcmp(ev->keyname, "Next"))
     {
	if (sd->page.y < 0)
	  y += -(sd->page.y * vh) / 100;
	else
	  y += sd->page.y;
     }
   e_scrollframe_child_pos_set(sd->smart_obj, x, y);
}

static void
_e_smart_scrollbar_read(E_Smart_Data *sd)
{
   Evas_Coord x, y, mx = 0, my = 0;
   double vx, vy;
   
   edje_object_part_drag_value_get(sd->edje_obj, "vbar_bar", NULL, &vy);
   edje_object_part_drag_value_get(sd->edje_obj, "hbar_bar", &vx, NULL);
   sd->pan_func.max_get(sd->pan_obj, &mx, &my);
   x = vx * (double)mx;
   y = vy * (double)my;
   sd->pan_func.set(sd->pan_obj, x, y);
}

static void
_e_smart_scrollbar_reset(E_Smart_Data *sd)
{
   edje_object_part_drag_value_set(sd->edje_obj, "vbar_bar", 0.0, 0.0);
   edje_object_part_drag_value_set(sd->edje_obj, "hbar_bar", 0.0, 0.0);
   if ((!sd->child_obj) && (!sd->extern_pan))
     {
	edje_object_part_drag_size_set(sd->edje_obj, "vbar_bar", 1.0, 1.0);
	edje_object_part_drag_size_set(sd->edje_obj, "hbar_bar", 1.0, 1.0);
     }
   sd->pan_func.set(sd->pan_obj, 0, 0);
}

static int
_e_smart_scrollbar_bar_v_visibility_adjust(E_Smart_Data *sd)
{
   int scroll_v_vis_change = 0;
   Evas_Coord w, h, vw, vh;
   
   w = sd->child.w;
   h = sd->child.h;
   evas_object_geometry_get(sd->pan_obj, NULL, NULL, &vw, &vh);
   if (sd->vbar_visible)
     {
	if (sd->vbar_flags == E_SCROLLFRAME_POLICY_AUTO)
	  {
	     if ((sd->child_obj) || (sd->extern_pan))
	       {
		  if (h <= vh)
		    {
		       scroll_v_vis_change = 1;
		       sd->vbar_visible = 0;
		    }
	       }
	     else
	       {
		  scroll_v_vis_change = 1;
		  sd->vbar_visible = 0;
	       }
	  }
	else if (sd->vbar_flags == E_SCROLLFRAME_POLICY_OFF)
	  {
	     scroll_v_vis_change = 1;
	     sd->vbar_visible = 0;
	  }
     }
   else
     {
	if (sd->vbar_flags == E_SCROLLFRAME_POLICY_AUTO)
	  {
	     if ((sd->child_obj) || (sd->extern_pan))
	       {
		  if (h > vh)
		    {
		       scroll_v_vis_change = 1;
		       sd->vbar_visible = 1;
		    }
	       }
	  }
	else if (sd->vbar_flags == E_SCROLLFRAME_POLICY_ON)
	  {
	     scroll_v_vis_change = 1;
	     sd->vbar_visible = 1;
	  }
     }
   if (scroll_v_vis_change)
     {
	if (sd->vbar_visible)
	  edje_object_signal_emit(sd->edje_obj, "vbar", "show");
	else
	  edje_object_signal_emit(sd->edje_obj, "vbar", "hide");
	edje_object_message_signal_process(sd->edje_obj);
	_e_smart_scrollbar_size_adjust(sd);
     }
   return scroll_v_vis_change;
}

static int
_e_smart_scrollbar_bar_h_visibility_adjust(E_Smart_Data *sd)
{
   int scroll_h_vis_change = 0;
   Evas_Coord w, h, vw, vh;
   
   w = sd->child.w;
   h = sd->child.h;
   evas_object_geometry_get(sd->pan_obj, NULL, NULL, &vw, &vh);
   if (sd->hbar_visible)
     {
	if (sd->hbar_flags == E_SCROLLFRAME_POLICY_AUTO)
	  {
	     if ((sd->child_obj) || (sd->extern_pan))
	       {
		  if (w <= vw)
		    {
		       scroll_h_vis_change = 1;
		       sd->hbar_visible = 0;
		    }
	       }
	     else
	       {
		  scroll_h_vis_change = 1;
		  sd->hbar_visible = 0;
	       }
	  }
	else if (sd->hbar_flags == E_SCROLLFRAME_POLICY_OFF)
	  {
	     scroll_h_vis_change = 1;
	     sd->hbar_visible = 0;
	  }
     }
   else
     {
	if (sd->hbar_flags == E_SCROLLFRAME_POLICY_AUTO)
	  {
	     if ((sd->child_obj) || (sd->extern_pan))
	       {
		  if (w > vw)
		    {
		       scroll_h_vis_change = 1;
		       sd->hbar_visible = 1;
		    }
	       }
	  }
	else if (sd->hbar_flags == E_SCROLLFRAME_POLICY_ON)
	  {
	     scroll_h_vis_change = 1;
	     sd->hbar_visible = 1;
	  }
     }
   if (scroll_h_vis_change)
     {
	if (sd->hbar_visible)
	  edje_object_signal_emit(sd->edje_obj, "hbar", "show");
	else
	  edje_object_signal_emit(sd->edje_obj, "hbar", "hide");
	edje_object_message_signal_process(sd->edje_obj);
	_e_smart_scrollbar_size_adjust(sd);
     }
   return scroll_h_vis_change;
}

static void
_e_smart_scrollbar_bar_visibility_adjust(E_Smart_Data *sd)
{
   int changed = 0;
   
   changed |= _e_smart_scrollbar_bar_h_visibility_adjust(sd);
   changed |= _e_smart_scrollbar_bar_v_visibility_adjust(sd);
   if (changed)
     {
	_e_smart_scrollbar_bar_h_visibility_adjust(sd);
	_e_smart_scrollbar_bar_v_visibility_adjust(sd);
     }
}

static void
_e_smart_scrollbar_size_adjust(E_Smart_Data *sd)
{
   if ((sd->child_obj) || (sd->extern_pan))
     {
	Evas_Coord x, y, w, h, mx = 0, my = 0, vw = 0, vh = 0;
	double vx, vy, size;
	
	evas_object_geometry_get(sd->pan_obj, NULL, NULL, &vw, &vh);
	w = sd->child.w;
	if (w < 1) w = 1;
	size = (double)vw / (double)w;
	if (size > 1.0)
	  {
	     size = 1.0;
	     edje_object_part_drag_value_set(sd->edje_obj, "hbar_bar", 0.0, 0.0);
	  }
	edje_object_part_drag_size_set(sd->edje_obj, "hbar_bar", size, 1.0);
	
	h = sd->child.h;
	if (h < 1) h = 1;
	size = (double)vh / (double)h;
	if (size > 1.0)
	  {
	     size = 1.0;
	     edje_object_part_drag_value_set(sd->edje_obj, "vbar_bar", 0.0, 0.0);
	  }
	edje_object_part_drag_size_set(sd->edje_obj, "vbar_bar", 1.0, size);

	edje_object_part_drag_value_get(sd->edje_obj, "hbar_bar", &vx, NULL);
	edje_object_part_drag_value_get(sd->edje_obj, "vbar_bar", NULL, &vy);
	sd->pan_func.max_get(sd->pan_obj, &mx, &my);
	x = vx * mx;
	y = vy * my;
	
	edje_object_part_drag_step_set(sd->edje_obj, "hbar_bar", (double)sd->step.x / (double)w, 0.0);
	edje_object_part_drag_step_set(sd->edje_obj, "vbar_bar", 0.0, (double)sd->step.y / (double)h);
	if (sd->page.x > 0)
	  edje_object_part_drag_page_set(sd->edje_obj, "hbar_bar", (double)sd->page.x / (double)w, 0.0);
	else
	  edje_object_part_drag_page_set(sd->edje_obj, "hbar_bar", -((double)sd->page.x * ((double)vw / (double)w)) / 100.0, 0.0);
	if (sd->page.y > 0)
	  edje_object_part_drag_page_set(sd->edje_obj, "vbar_bar", 0.0, (double)sd->page.y / (double)h);
	else
	  edje_object_part_drag_page_set(sd->edje_obj, "vbar_bar", 0.0, -((double)sd->page.y * ((double)vh / (double)h)) / 100.0);
	
	sd->pan_func.set(sd->pan_obj, x, y);
     }
   else
     {
	edje_object_part_drag_size_set(sd->edje_obj, "vbar_bar", 1.0, 1.0);
	edje_object_part_drag_size_set(sd->edje_obj, "hbar_bar", 1.0, 1.0);
	sd->pan_func.set(sd->pan_obj, 0, 0);
     }
   _e_smart_scrollbar_bar_visibility_adjust(sd);
}
                    
static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->edje_obj, sd->x, sd->y);
   evas_object_resize(sd->edje_obj, sd->w, sd->h);
   evas_object_move(sd->event_obj, sd->x, sd->y);
   evas_object_resize(sd->event_obj, sd->w, sd->h);
   _e_smart_scrollbar_size_adjust(sd);
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *o;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->step.x = 32;
   sd->step.y = 32;
   sd->page.x = -50;
   sd->page.y = -50;
   sd->hbar_flags = E_SCROLLFRAME_POLICY_AUTO;
   sd->vbar_flags = E_SCROLLFRAME_POLICY_AUTO;
   sd->hbar_visible = 1;
   sd->vbar_visible = 1;
   
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);
   
   o = edje_object_add(evas_object_evas_get(obj));
   sd->edje_obj = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/scrollframe");
   edje_object_signal_callback_add(o, "drag*", "vbar_bar", _e_smart_edje_drag_v, sd);
   edje_object_signal_callback_add(o, "drag*", "hbar_bar", _e_smart_edje_drag_h, sd);
   evas_object_smart_member_add(o, obj);
   
   o = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->event_obj = o;
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, _e_smart_event_wheel, sd);
   evas_object_smart_member_add(o, obj);
   evas_object_repeat_events_set(o, 1);

   sd->pan_func.set = e_pan_set;
   sd->pan_func.get = e_pan_get;
   sd->pan_func.max_get = e_pan_max_get;
   sd->pan_func.child_size_get = e_pan_child_size_get;
   
   _e_smart_scrollbar_reset(sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   e_scrollframe_child_set(obj, NULL);
   if (!sd->extern_pan) evas_object_del(sd->pan_obj);
   evas_object_del(sd->edje_obj);
   evas_object_del(sd->event_obj);
   free(sd);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->edje_obj);
   evas_object_show(sd->event_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->edje_obj);
   evas_object_hide(sd->event_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->edje_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->edje_obj, clip);
   evas_object_clip_set(sd->event_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->edje_obj);
   evas_object_clip_unset(sd->event_obj);
}  

/* never need to touch this */

static void
_e_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new(SMART_NAME,
			     _e_smart_add,
			     _e_smart_del, 
			     NULL, NULL, NULL, NULL, NULL,
			     _e_smart_move,
			     _e_smart_resize,
			     _e_smart_show,
			     _e_smart_hide,
			     _e_smart_color_set,
			     _e_smart_clip_set,
			     _e_smart_clip_unset,
			     NULL);
}


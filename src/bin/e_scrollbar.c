/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO:
 * - add event handlers for scrolling
 * - add functions to set / get values and min / max
 */

typedef struct _E_Scrollbar_Smart_Data E_Scrollbar_Smart_Data;

struct _E_Scrollbar_Smart_Data
{
   struct {
      Evas_Object *object;
      Evas_Coord x, y, w, h;
   } edje, drag;
   
   struct {
	Evas_Coord x, y, w, h;
     } confine;
   
   int is_dragging;
   
   Evas_Coord mx, my;
      
   struct {
	double min;
	double max;
	double current;
   } value;

   E_Scrollbar_Direction direction;

   Evas_List *callbacks;
};

static void _e_scrollbar_smart_add(Evas_Object *object);
static void _e_scrollbar_smart_del(Evas_Object *object);
static void _e_scrollbar_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_scrollbar_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_scrollbar_smart_show(Evas_Object *object);
static void _e_scrollbar_smart_hide(Evas_Object *object);
static void _e_scrollbar_drag_cb(void *data, Evas_Object *object, const char *emission, const char *source);

static void _e_scrollbar_drag_mouse_move_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _e_scrollbar_drag_mouse_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);
static void _e_scrollbar_drag_mouse_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info);

static Evas_Smart *e_scrollbar_smart = NULL;

Evas_Object *
e_scrollbar_add(Evas *evas)
{
   if (!e_scrollbar_smart)
     {
	e_scrollbar_smart = evas_smart_new("e_entry",
					   _e_scrollbar_smart_add, /* add */
					   _e_scrollbar_smart_del, /* del */
					   NULL, NULL, NULL, NULL, NULL,
					   _e_scrollbar_smart_move, /* move */
					   _e_scrollbar_smart_resize, /* resize */
					   _e_scrollbar_smart_show, /* show */
					   _e_scrollbar_smart_hide, /* hide */
					   NULL, /* color_set */
					   NULL, /* clip_set */
					   NULL, /* clip_unset */
					   NULL); /* data*/
     }
   return evas_object_smart_add(evas, e_scrollbar_smart);
}

void
e_scrollbar_direction_set(Evas_Object *object, E_Scrollbar_Direction dir)
{
   E_Scrollbar_Smart_Data *sd;
      
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   if (sd->direction == dir)
     return;

   switch (dir)
     {
      case E_SCROLLBAR_HORIZONTAL:
	 e_theme_edje_object_set(sd->edje.object, 
				 "base/theme/widgets/hscrollbar",
				 "widgets/hscrollbar");
	e_theme_edje_object_set(sd->drag.object,
				"base/theme/widgets/vscrollbar",
				"widgets/hscrollbar_drag");	
	 sd->direction = dir;
	 break;

      case E_SCROLLBAR_VERTICAL:
	 e_theme_edje_object_set(sd->edje.object, 
				 "base/theme/widgets/vscrollbar",
				 "widgets/vscrollbar");
	e_theme_edje_object_set(sd->drag.object,
				"base/theme/widgets/vscrollbar",
				"widgets/vscrollbar_drag");	
	 sd->direction = dir;	
	 break;
     }
}

E_Scrollbar_Direction
e_scrollbar_direction_get(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return E_SCROLLBAR_HORIZONTAL;
   
   return sd->direction;   
}

void
e_scrollbar_callback_drag_add(Evas_Object *object, void (*func)(Evas_Object *obj, double value, void *data), void *data)
{    
   E_Scrollbar_Smart_Data *sd;
   E_Scrollbar_Drag_Handler *handler;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   handler = E_NEW(E_Scrollbar_Drag_Handler, 1);
   
   handler->data = data;
   handler->cb.drag = func;
   
   sd->callbacks = evas_list_append(sd->callbacks, handler);
}

void
e_scrollbar_value_set(Evas_Object *object, double value)
{    
   E_Scrollbar_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   sd->value.current = value;

   if (sd->direction == E_SCROLLBAR_HORIZONTAL)
     edje_object_part_drag_value_set(sd->edje.object, "drag", value, 0);
   else
     edje_object_part_drag_value_set(sd->edje.object, "drag", 0, value);
}

double
e_scrollbar_value_get(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;
   double dx, dy;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return 0.0;
   
   edje_object_part_drag_value_get(sd->edje.object, "drag", &dx, &dy);
   
   if (sd->direction == E_SCROLLBAR_HORIZONTAL)     
     sd->value.current = dx;
   else
     sd->value.current = dy;     
   
   return sd->value.current;
}

void
e_scrollbar_increments_set(Evas_Object *object, double step, double page)
{
   E_Scrollbar_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   if (sd->direction == E_SCROLLBAR_HORIZONTAL)
     {
	edje_object_part_drag_step_set(sd->edje.object, "drag", step, 0);
	edje_object_part_drag_page_set(sd->edje.object, "drag", page, 0);
     }
   else
     {
	edje_object_part_drag_step_set(sd->edje.object, "drag", 0, step);
	edje_object_part_drag_page_set(sd->edje.object, "drag", 0, page);
     }
}

void
e_scrollbar_increments_get(Evas_Object *object, double *step, double *page)
{
   E_Scrollbar_Smart_Data *sd;
   double stepx; double stepy;
   double pagex; double pagey;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   edje_object_part_drag_step_get(sd->edje.object, "drag", &stepx, &stepy);
   edje_object_part_drag_page_get(sd->edje.object, "drag", &pagex, &pagey);   
   
   if (sd->direction == E_SCROLLBAR_HORIZONTAL)
     {
	if (step) *step = stepx;
	if (page) *page = pagex;
     }
   else
     {
	if (step) *step = stepy;
	if (page) *page = pagey;
     }      
}


/************************** 
 * Private functions 
 **************************/

static void
_e_scrollbar_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(evas = evas_object_evas_get(object)))
      return;

   sd = calloc(1, sizeof(E_Scrollbar_Smart_Data));
   if (!sd) return;
   sd->is_dragging = 0;   
   sd->value.min = 0.0;
   sd->value.max = 1.0;
   sd->callbacks = NULL;
   sd->direction = E_SCROLLBAR_HORIZONTAL;
   
   sd->edje.object = edje_object_add(evas);
   sd->edje.x = 0;
   sd->edje.y = 0;
   sd->edje.w = 0;
   sd->edje.h = 0;
   e_theme_edje_object_set(sd->edje.object, "base/theme/widgets/hscrollbar",
			      "widgets/hscrollbar");
   
   sd->drag.object = edje_object_add(evas);
   sd->drag.x = 0;
   sd->drag.y = 0;
   sd->drag.w = 0;
   sd->drag.h = 0;
   e_theme_edje_object_set(sd->drag.object,
			   "base/theme/widgets/hscrollbar",
			   "widgets/hscrollbar_drag");
   
   edje_object_part_geometry_get(sd->drag.object, "confine",
				 &sd->confine.x, &sd->confine.y,
				 &sd->confine.w, sd->confine.h);
   
   evas_object_event_callback_add(sd->drag.object, EVAS_CALLBACK_MOUSE_MOVE, _e_scrollbar_drag_mouse_move_cb, sd);
   evas_object_event_callback_add(sd->drag.object, EVAS_CALLBACK_MOUSE_UP, _e_scrollbar_drag_mouse_up_cb, sd);
   evas_object_event_callback_add(sd->drag.object, EVAS_CALLBACK_MOUSE_DOWN, _e_scrollbar_drag_mouse_down_cb, sd);
   
   evas_object_data_set(sd->edje.object, "smart", object);
   evas_object_smart_member_add(sd->edje.object, object);
   evas_object_smart_member_add(sd->drag.object, object);   
   
   evas_object_smart_data_set(object, sd);
   
   evas_object_show(sd->edje.object);
   evas_object_show(sd->drag.object);
}

static void
_e_scrollbar_smart_del(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_del(sd->edje.object);
   evas_object_del(sd->drag.object);   
   free(sd);
}

/* Called when the object is moved */
static void
_e_scrollbar_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Scrollbar_Smart_Data *sd;   
   Evas_Coord we, ye, yd, wd;
   Evas_Coord cx, cy, cw, ch;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_geometry_get(sd->edje.object, NULL, &ye, &we, NULL);
   evas_object_geometry_get(sd->drag.object, NULL, &yd, &wd, NULL);
   
   evas_object_move(sd->edje.object, x, y);
   edje_object_part_geometry_get(sd->edje.object, "confine", &cx, &cy, &cw, &ch);
   evas_object_move(sd->drag.object, x + cx + abs(wd - cw)/2, y + cy);
   
   sd->edje.x = x;
   sd->edje.y = y;
   
   sd->confine.x = x + cx;
   sd->confine.y = y + cy;
   
   sd->drag.x = x + cx + abs(cw - wd)/2;
   sd->drag.y = y + cy ;   
}

/* Called when the object is resized */
static void
_e_scrollbar_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Scrollbar_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   evas_object_resize(sd->edje.object, w, h);
   sd->edje.w = w;
   sd->edje.h = h;
   edje_object_part_geometry_get(sd->edje.object, "confine", NULL, NULL,
				    &sd->confine.w, &sd->confine.h);
   evas_object_resize(sd->drag.object, sd->confine.w, 20);
   sd->drag.w = sd->confine.w;
   sd->drag.h = 20;   
}

static void
_e_scrollbar_smart_show(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   evas_object_show(sd->edje.object);
   evas_object_show(sd->drag.object);   

}

static void
_e_scrollbar_smart_hide(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   evas_object_hide(sd->edje.object);
   evas_object_hide(sd->drag.object);
}

static void
_e_scrollbar_drag_cb(void *data, Evas_Object *object, const char *emission, const char *source)
{    
   E_Scrollbar_Smart_Data *sd;
   Evas_Object *smart_object;
   Evas_List *l;
   
   if ((!object) || !(smart_object = evas_object_data_get(object, "smart")))
     return;
   
   sd = evas_object_smart_data_get(smart_object);
   if (!sd)
     return;
   
   sd->value.current = e_scrollbar_value_get(smart_object);
   
   for (l = sd->callbacks; l; l = l->next)
    {
       E_Scrollbar_Drag_Handler *handler;
       
       handler = l->data;
       handler->cb.drag(smart_object, sd->value.current, handler->data);
    }
}

static void
  _e_scrollbar_drag_mouse_down_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
      E_Scrollbar_Smart_Data *sd;
      Evas_Event_Mouse_Down *ev;
   
      ev = event_info;
      sd = data;
      sd->mx = ev->canvas.x - sd->drag.x;
      sd->my = ev->canvas.y - sd->drag.y;
      sd->is_dragging = 1;
}

static void
  _e_scrollbar_drag_mouse_up_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
      E_Scrollbar_Smart_Data *sd;
   
      sd = data;
      sd->is_dragging = 0;
}

static void
_e_scrollbar_drag_mouse_move_cb(void *data, Evas *evas, Evas_Object *object, void *event_info)
{
   E_Scrollbar_Smart_Data *sd;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord x, y, w, h;
   Evas_Coord cx, cy, cw, ch;
   
   sd = data;
   
   if(!sd->is_dragging) return;
   
   if(sd->direction = E_SCROLLBAR_VERTICAL)
     {
	if (sd->drag.y < sd->confine.y)
	  {
	     evas_object_move(sd->drag.object, sd->drag.x, sd->confine.y);
	     sd->drag.y = sd->confine.y;
	     return;
	  }
	
	if (sd->drag.y == sd->confine.y && ev->cur.canvas.y - sd->my < sd->confine.y)
	  return;
	
	if (sd->drag.y + sd->drag.h > sd->confine.y + sd->confine.h)
	  {
	     evas_object_move(sd->drag.object, sd->drag.x, sd->confine.y + sd->confine.h - sd->drag.h);
	     sd->drag.y = sd->confine.y + sd->confine.h - sd->drag.h;
	     return;
	  }
	
	if (sd->drag.y + sd->drag.h == sd->confine.y + sd->confine.h &&
	    ev->cur.canvas.y + sd->my > sd->confine.y + sd->confine.h)
	  return;
	
	evas_object_move(sd->drag.object, sd->drag.x, ev->cur.canvas.y - sd->my);
	sd->drag.y = ev->cur.canvas.y - sd->my;
     }
}

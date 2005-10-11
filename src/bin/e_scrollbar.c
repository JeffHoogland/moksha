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
   Evas_Object *edje_object;

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
static void _e_scrollbar_smart_raise(Evas_Object *object);
static void _e_scrollbar_smart_lower(Evas_Object *object);
static void _e_scrollbar_smart_stack_above(Evas_Object *object, Evas_Object *above);
static void _e_scrollbar_smart_stack_below(Evas_Object *object, Evas_Object *below);
static void _e_scrollbar_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_scrollbar_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_scrollbar_smart_show(Evas_Object *object);
static void _e_scrollbar_smart_hide(Evas_Object *object);
static void _e_scrollbar_drag_cb(void *data, Evas_Object *object, const char *emission, const char *source);
    
static Evas_Smart *e_scrollbar_smart = NULL;

Evas_Object *
e_scrollbar_add(Evas *evas)
{
   if (!e_scrollbar_smart)
     {
	e_scrollbar_smart = evas_smart_new("e_entry",
					   _e_scrollbar_smart_add, /* add */
					   _e_scrollbar_smart_del, /* del */
					   NULL, /* layer_set */
					   _e_scrollbar_smart_raise, /* raise */
					   _e_scrollbar_smart_lower, /* lower */
					   _e_scrollbar_smart_stack_above, /* stack_above */
					   _e_scrollbar_smart_stack_below, /* stack_below */
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
	 e_theme_edje_object_set(sd->edje_object, 
				 "base/theme/widgets/hscrollbar",
				 "widgets/hscrollbar");
	 sd->direction = dir;
	 break;

      case E_SCROLLBAR_VERTICAL:
	 e_theme_edje_object_set(sd->edje_object, 
				 "base/theme/widgets/vscrollbar",
				 "widgets/vscrollbar");
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
     edje_object_part_drag_value_set(sd->edje_object, "drag", value, 0);
   else
     edje_object_part_drag_value_set(sd->edje_object, "drag", 0, value);
}

double
e_scrollbar_value_get(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;
   double dx, dy;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return 0.0;
   
   edje_object_part_drag_value_get(sd->edje_object, "drag", &dx, &dy);
   
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
	edje_object_part_drag_step_set(sd->edje_object, "drag", step, 0);
	edje_object_part_drag_page_set(sd->edje_object, "drag", page, 0);
     }
   else
     {
	edje_object_part_drag_step_set(sd->edje_object, "drag", 0, step);
	edje_object_part_drag_page_set(sd->edje_object, "drag", 0, page);
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
   
   edje_object_part_drag_step_get(sd->edje_object, "drag", &stepx, &stepy);
   edje_object_part_drag_page_get(sd->edje_object, "drag", &pagex, &pagey);   
   
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
   sd->value.min = 0.0;
   sd->value.max = 1.0;
   sd->callbacks = NULL;
   sd->direction = E_SCROLLBAR_HORIZONTAL;   
   sd->edje_object = edje_object_add(evas);
   e_theme_edje_object_set(sd->edje_object, 
			   "base/theme/widgets/hscrollbar",
			   "widgets/hscrollbar");
   edje_object_signal_callback_add(sd->edje_object, "drag", "*", _e_scrollbar_drag_cb, sd);
   evas_object_data_set(sd->edje_object, "smart", object);
   evas_object_smart_member_add(sd->edje_object, object);
   
   evas_object_smart_data_set(object, sd);
}

static void
_e_scrollbar_smart_del(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_del(sd->edje_object);

   free(sd);
}

static void
_e_scrollbar_smart_raise(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_raise(sd->edje_object);
}

static void
_e_scrollbar_smart_lower(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_lower(sd->edje_object);
}

static void
_e_scrollbar_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || (!above) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_above(sd->edje_object, above);
}

/* Called when the object is stacked below another object */
static void
_e_scrollbar_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || (!below) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_below(sd->edje_object, below);
}

/* Called when the object is moved */
static void
_e_scrollbar_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_move(sd->edje_object, x, y);
}

/* Called when the object is resized */
static void
_e_scrollbar_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   evas_object_resize(sd->edje_object, w, h);
}

static void
_e_scrollbar_smart_show(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   evas_object_show(sd->edje_object);
}

static void
_e_scrollbar_smart_hide(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
      return;

   evas_object_hide(sd->edje_object);
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

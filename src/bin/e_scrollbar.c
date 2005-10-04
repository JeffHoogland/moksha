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
   } value;

   E_Scrollbar_Direction direction;
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
   E_Scrollbar_Smart_Data *scrollbar_sd;
      
   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   if (scrollbar_sd->direction == dir)
     return;

   switch (dir)
     {
      case E_SCROLLBAR_HORISONTAL:
	 e_theme_edje_object_set(scrollbar_sd->edje_object, 
				 "base/theme/widgets/hscrollbar",
				 "widgets/hscrollbar");       
	 break;

      case E_SCROLLBAR_VERTICAL:
	 e_theme_edje_object_set(scrollbar_sd->edje_object, 
				 "base/theme/widgets/vscrollbar",
				 "widgets/vscrollbar");       
	 break;
     }
}

E_Scrollbar_Direction
e_scrollbar_direction_get(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;
   
   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return E_SCROLLBAR_HORISONTAL;
   
   return scrollbar_sd->direction;   
}

/************************** 
 * Private functions 
 **************************/

static void
_e_scrollbar_smart_add(Evas_Object *object)
{
   Evas *evas;
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(evas = evas_object_evas_get(object)))
      return;

   scrollbar_sd = calloc(1, sizeof(E_Scrollbar_Smart_Data));
   if (!scrollbar_sd) return;
   scrollbar_sd->value.min = 0.0;
   scrollbar_sd->value.max = 1.0;

   scrollbar_sd->edje_object = edje_object_add(evas);
   e_theme_edje_object_set(scrollbar_sd->edje_object, 
			   "base/theme/widgets/hscrollbar",
			   "widgets/hscrollbar");

   scrollbar_sd->direction = E_SCROLLBAR_HORISONTAL;
   
   evas_object_smart_member_add(scrollbar_sd->edje_object, object);
   
   evas_object_smart_data_set(object, scrollbar_sd);
}

static void
_e_scrollbar_smart_del(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_del(scrollbar_sd->edje_object);

   free(scrollbar_sd);
}

static void
_e_scrollbar_smart_raise(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_raise(scrollbar_sd->edje_object);
}

static void
_e_scrollbar_smart_lower(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_lower(scrollbar_sd->edje_object);
}

static void
_e_scrollbar_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || (!above) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_above(scrollbar_sd->edje_object, above);
}

/* Called when the object is stacked below another object */
static void
_e_scrollbar_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || (!below) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_below(scrollbar_sd->edje_object, below);
}

/* Called when the object is moved */
static void
_e_scrollbar_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
     return;

   evas_object_move(scrollbar_sd->edje_object, x, y);
}

/* Called when the object is resized */
static void
_e_scrollbar_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_resize(scrollbar_sd->edje_object, w, h);
}

static void
_e_scrollbar_smart_show(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_show(scrollbar_sd->edje_object);
}

static void
_e_scrollbar_smart_hide(Evas_Object *object)
{
   E_Scrollbar_Smart_Data *scrollbar_sd;

   if ((!object) || !(scrollbar_sd = evas_object_smart_data_get(object)))
      return;

   evas_object_hide(scrollbar_sd->edje_object);
}


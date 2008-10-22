/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_popup_free(E_Popup *pop);
static int  _e_popup_cb_window_shape(void *data, int ev_type, void *ev);
static E_Popup *_e_popup_find_by_window(Ecore_X_Window win);
static int _e_popup_cb_mouse_down(void *data, int type, void *event);
static int _e_popup_cb_mouse_up(void *data, int type, void *event);
static int _e_popup_cb_mouse_wheel(void *data, int type, void *event);

/* local subsystem globals */
static Ecore_Event_Handler *_e_popup_window_shape_handler = NULL;
static Ecore_Event_Handler *_e_popup_mouse_down_handler = NULL;
static Ecore_Event_Handler *_e_popup_mouse_up_handler = NULL;
static Ecore_Event_Handler *_e_popup_mouse_wheel_handler = NULL;
static Eina_List *_e_popup_list = NULL;

/* externally accessible functions */

EAPI int
e_popup_init(void)
{
   _e_popup_window_shape_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,
							   _e_popup_cb_window_shape, NULL);
   _e_popup_mouse_down_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN,
							 _e_popup_cb_mouse_down, NULL);
   _e_popup_mouse_up_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
						       _e_popup_cb_mouse_up, NULL);
   _e_popup_mouse_wheel_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
							  _e_popup_cb_mouse_wheel, NULL);
   return 1;
}

EAPI int
e_popup_shutdown(void)
{
   E_FN_DEL(ecore_event_handler_del, _e_popup_window_shape_handler);
   E_FN_DEL(ecore_event_handler_del, _e_popup_mouse_down_handler);
   E_FN_DEL(ecore_event_handler_del, _e_popup_mouse_up_handler);
   E_FN_DEL(ecore_event_handler_del, _e_popup_mouse_wheel_handler);
   return 1;
}

EAPI E_Popup *
e_popup_new(E_Zone *zone, int x, int y, int w, int h)
{
   E_Popup *pop;
   
   pop = E_OBJECT_ALLOC(E_Popup, E_POPUP_TYPE, _e_popup_free);
   if (!pop) return NULL;
   pop->zone = zone;
   pop->x = x;
   pop->y = y;
   pop->w = w;
   pop->h = h;
   pop->layer = 250;
   pop->ecore_evas = e_canvas_new(e_config->evas_engine_popups, pop->zone->container->win,
				  pop->zone->x + pop->x, pop->zone->y + pop->y, pop->w, pop->h, 1, 1,
				  &(pop->evas_win), NULL);
   /* avoid excess exposes when shaped - set damage avoid to 1 */
//   ecore_evas_avoid_damage_set(pop->ecore_evas, 1);
   
   e_canvas_add(pop->ecore_evas);
   pop->shape = e_container_shape_add(pop->zone->container);
   e_container_shape_move(pop->shape, pop->zone->x + pop->x, pop->zone->y + pop->y);
   e_container_shape_resize(pop->shape, pop->w, pop->h);
   pop->evas = ecore_evas_get(pop->ecore_evas);
   e_container_window_raise(pop->zone->container, pop->evas_win, pop->layer);
   ecore_x_window_shape_events_select(pop->evas_win, 1);
   ecore_evas_name_class_set(pop->ecore_evas, "E", "_e_popup_window");
   ecore_evas_title_set(pop->ecore_evas, "E Popup");
   e_object_ref(E_OBJECT(pop->zone));
   pop->zone->popups = eina_list_append(pop->zone->popups, pop);
   _e_popup_list = eina_list_append(_e_popup_list, pop);
   return pop;
}

EAPI void
e_popup_show(E_Popup *pop)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if (pop->visible) return;
   pop->visible = 1;
   ecore_evas_show(pop->ecore_evas);
   e_container_shape_show(pop->shape);
}

EAPI void
e_popup_hide(E_Popup *pop)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if (!pop->visible) return;
   pop->visible = 0;
   ecore_evas_hide(pop->ecore_evas);
   e_container_shape_hide(pop->shape);
}

EAPI void
e_popup_move(E_Popup *pop, int x, int y)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if ((pop->x == x) && (pop->y == y)) return;
   pop->x = x;
   pop->y = y;
   ecore_evas_move(pop->ecore_evas,
		   pop->zone->x + pop->x, 
		   pop->zone->y + pop->y);
   e_container_shape_move(pop->shape,
			  pop->zone->x + pop->x, 
			  pop->zone->y + pop->y);
}

EAPI void
e_popup_resize(E_Popup *pop, int w, int h)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if ((pop->w == w) && (pop->h == h)) return;
   pop->w = w;
   pop->h = h;
   ecore_evas_resize(pop->ecore_evas, pop->w, pop->h);
   e_container_shape_resize(pop->shape, pop->w, pop->h);
}
  
EAPI void
e_popup_move_resize(E_Popup *pop, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if ((pop->x == x) && (pop->y == y) &&
       (pop->w == w) && (pop->h == h)) return;
   pop->x = x;
   pop->y = y;
   pop->w = w;
   pop->h = h;
   ecore_evas_move_resize(pop->ecore_evas,
			  pop->zone->x + pop->x, 
			  pop->zone->y + pop->y,
			  pop->w, pop->h);
   e_container_shape_move(pop->shape,
			  pop->zone->x + pop->x, 
			  pop->zone->y + pop->y);
   e_container_shape_resize(pop->shape, pop->w, pop->h);
}

EAPI void
e_popup_ignore_events_set(E_Popup *pop, int ignore)
{
   ecore_evas_ignore_events_set(pop->ecore_evas, ignore);
}

EAPI void
e_popup_edje_bg_object_set(E_Popup *pop, Evas_Object *o)
{
   const char *shape_option;
   
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   shape_option = edje_object_data_get(o, "shaped");
   if (shape_option)
     {
	if (!strcmp(shape_option, "1"))
	  pop->shaped = 1;
	else
	  pop->shaped = 0;
	if (e_config->use_composite)
	  {
	     ecore_evas_alpha_set(pop->ecore_evas, pop->shaped);
	     pop->evas_win = ecore_evas_software_x11_window_get(pop->ecore_evas);
	     e_container_window_raise(pop->zone->container, pop->evas_win, pop->layer);
	  }
	else
	  ecore_evas_shaped_set(pop->ecore_evas, pop->shaped);
     }
}

EAPI void
e_popup_layer_set(E_Popup *pop, int layer)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   pop->layer = layer;
   e_container_window_raise(pop->zone->container, pop->evas_win, pop->layer);
}

EAPI void
e_popup_idler_before(void)
{
   Eina_List *l;
   
   for (l = _e_popup_list; l; l = l->next)
     {
	E_Popup *pop;
	
	pop = l->data;
	if (pop->need_shape_export)
	  {
             Ecore_X_Rectangle *rects, *orects;
	     int num;
	     
	     rects = ecore_x_window_shape_rectangles_get(pop->evas_win, &num);
	     if (rects)
	       {
		  int changed;
		  
		  changed = 1;
		  if ((num == pop->shape_rects_num) && (pop->shape_rects))
		    {
		       int i;
		       
		       orects = pop->shape_rects;
		       changed = 0;
		       for (i = 0; i < num; i++)
			 {
			    if (rects[i].x < 0)
			      {
				 rects[i].width -= rects[i].x;
				 rects[i].x = 0;
			      }
			    if ((rects[i].x + rects[i].width) > pop->w)
			      rects[i].width = rects[i].width - rects[i].x;
			    if (rects[i].y < 0)
			      {
				 rects[i].height -= rects[i].y;
				 rects[i].y = 0;
			      }
			    if ((rects[i].y + rects[i].height) > pop->h)
			      rects[i].height = rects[i].height - rects[i].y;
			    
			    if ((orects[i].x != rects[i].x) ||
				(orects[i].y != rects[i].y) ||
				(orects[i].width != rects[i].width) ||
				(orects[i].height != rects[i].height))
			      {
				 changed = 1;
				 break;
			      }
			 }
		    }
		  if (changed)
		    {
		       E_FREE(pop->shape_rects);
		       pop->shape_rects = rects;
		       pop->shape_rects_num = num;
		       e_container_shape_rects_set(pop->shape, rects, num);
		    }
		  else
		    free(rects);
	       }
	     else
	       {
		  E_FREE(pop->shape_rects);
		  pop->shape_rects = NULL;
		  pop->shape_rects_num = 0;
		  e_container_shape_rects_set(pop->shape, NULL, 0);
	       }
	     pop->need_shape_export = 0;
	  }
	if (pop->visible)
	  e_container_shape_show(pop->shape);
     }
}

/* local subsystem functions */

static void
_e_popup_free(E_Popup *pop)
{
   E_FREE(pop->shape_rects);
   pop->shape_rects_num = 0;
   e_container_shape_hide(pop->shape);
   e_object_del(E_OBJECT(pop->shape));
   e_canvas_del(pop->ecore_evas);
   ecore_evas_free(pop->ecore_evas);
   e_object_unref(E_OBJECT(pop->zone));
   pop->zone->popups = eina_list_remove(pop->zone->popups, pop);
   _e_popup_list = eina_list_remove(_e_popup_list, pop);
   free(pop);
}

static int
_e_popup_cb_window_shape(void *data, int ev_type, void *ev)
{
   Eina_List *l;
   Ecore_X_Event_Window_Shape *e;
   
   e = ev;
   for (l = _e_popup_list; l; l = l->next)
     {
	E_Popup *pop;
	
	pop = l->data;
	if (pop->evas_win == e->win)
	  pop->need_shape_export = 1;
     }
   return 1;
}

static E_Popup *
_e_popup_find_by_window(Ecore_X_Window win)
{
   E_Popup *pop;
   Eina_List *l;
   
   for (l = _e_popup_list; l; l = l->next)
     {
	pop = l->data;
	if (pop->evas_win == win) return pop;
     }
   return NULL;
}

static int
_e_popup_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   E_Popup *pop;
   
   ev = event;
   pop = _e_popup_find_by_window(ev->event_win);
   if (pop)
     {
	Evas_Button_Flags flags = EVAS_BUTTON_NONE;
	
	e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_POPUP,
					   E_OBJECT(pop), ev);
	if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
	if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
	evas_event_feed_mouse_down(pop->evas, ev->button, flags, ev->time, NULL);
	return 0;
     }
   return 1;
}

static int
_e_popup_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   E_Popup *pop;
   
   ev = event;
   pop = _e_popup_find_by_window(ev->event_win);
   if (pop)
     {
	evas_event_feed_mouse_up(pop->evas, ev->button, EVAS_BUTTON_NONE, ev->time, NULL);
	e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_POPUP,
					 E_OBJECT(pop), ev);
	return 0;
     }
   return 1;
}

static int
_e_popup_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   E_Popup *pop;
   
   ev = event;
   pop = _e_popup_find_by_window(ev->event_win);
   if (pop)
     {
	e_bindings_wheel_event_handle(E_BINDING_CONTEXT_POPUP,
				      E_OBJECT(pop), ev);
	evas_event_feed_mouse_wheel(pop->evas, ev->direction, ev->z, ev->time, NULL);
	return 0;
     }
   return 1;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: broken when drop areas intersect 
 * (sub window has drop area on top of lower window or desktop)
 */
/*
 * TODO:
 * - Let an internal drag work with several types.
 * - Let a drag be both internal and external, or allow internal xdnd
 *   (internal xdnd is unecessary load)
 */

/* local subsystem functions */

static void _e_drag_show(E_Drag *drag);
static void _e_drag_hide(E_Drag *drag);
static void _e_drag_move(E_Drag *drag, int x, int y);
static void _e_drag_update(int x, int y);
static void _e_drag_end(int x, int y);
static void _e_drag_xdnd_end(int x, int y);
static void _e_drag_free(E_Drag *drag);

static int  _e_dnd_cb_window_shape(void *data, int type, void *event);

static int  _e_dnd_cb_mouse_up(void *data, int type, void *event);
static int  _e_dnd_cb_mouse_move(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_enter(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_leave(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_position(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_status(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_finished(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_drop(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_selection(void *data, int type, void *event);

/* local subsystem globals */

typedef struct _XDnd XDnd;

struct _XDnd
{
   int x, y;
   char *type;
   void *data;
};

static Evas_List *_event_handlers = NULL;
static Evas_List *_drop_handlers = NULL;

static Ecore_X_Window _drag_win = 0;

static Evas_List *_drag_list = NULL;
static E_Drag    *_drag_current = NULL;

static XDnd *_xdnd;

/* externally accessible functions */

EAPI int
e_dnd_init(void)
{
   Evas_List *l, *l2;
   E_Manager *man;
   E_Container *con;

   _event_handlers = evas_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
							      _e_dnd_cb_mouse_up, NULL));
   _event_handlers = evas_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
							      _e_dnd_cb_mouse_move, NULL));
   _event_handlers = evas_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,
							      _e_dnd_cb_window_shape, NULL));

   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;

	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     con = l2->data;

	     ecore_x_dnd_aware_set(con->bg_win, 1);
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_ENTER,
									_e_dnd_cb_event_dnd_enter,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_LEAVE,
									_e_dnd_cb_event_dnd_leave,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_POSITION,
									_e_dnd_cb_event_dnd_position,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_STATUS,
									_e_dnd_cb_event_dnd_status,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_FINISHED,
									_e_dnd_cb_event_dnd_finished,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_XDND_DROP,
									_e_dnd_cb_event_dnd_drop,
									con));
	     _event_handlers = evas_list_append(_event_handlers,
						ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY,
									_e_dnd_cb_event_dnd_selection,
									con));
	  }
     }
   return 1;
}

EAPI int
e_dnd_shutdown(void)
{
   Evas_List *l;

   while (_drag_list)
     {
	E_Drag *drag;

	drag = _drag_list->data;
	e_object_del(E_OBJECT(drag));
     }

   for (l = _event_handlers; l; l = l->next)
     {
	Ecore_Event_Handler *h;

	h = l->data;
	ecore_event_handler_del(h);
     }
   evas_list_free(_event_handlers);
   _event_handlers = NULL;

   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;
	e_drop_handler_del(h);
     }
   evas_list_free(_drop_handlers);
   _drop_handlers = NULL;

   return 1;
}

EAPI E_Drag*
e_drag_new(E_Container *container, int x, int y,
	   const char **types, unsigned int num_types,
	   void *data, int size,
	   void (*finished_cb)(E_Drag *drag, int dropped))
{
   E_Drag *drag;
   int i;

   /* No need to create a drag object without type */
   if (!num_types) return NULL;
   drag = E_OBJECT_ALLOC(E_Drag, E_DRAG_TYPE, _e_drag_free);
   if (!drag) return NULL;

   drag->x = x;
   drag->y = y;
   drag->w = 24;
   drag->h = 24;
   drag->layer = 250;
   drag->container = container;
   e_object_ref(E_OBJECT(drag->container));
   drag->ecore_evas = e_canvas_new(e_config->evas_engine_drag, drag->container->win,
				   drag->x, drag->y, drag->w, drag->h, 1, 1,
				   &(drag->evas_win), NULL);
   e_canvas_add(drag->ecore_evas);
   drag->shape = e_container_shape_add(drag->container);
   e_container_shape_move(drag->shape, drag->x, drag->y);
   e_container_shape_resize(drag->shape, drag->w, drag->h);

   drag->evas = ecore_evas_get(drag->ecore_evas);
   e_container_window_raise(drag->container, drag->evas_win, drag->layer);
   ecore_x_window_shape_events_select(drag->evas_win, 1);
   ecore_evas_name_class_set(drag->ecore_evas, "E", "_e_drag_window");
   ecore_evas_title_set(drag->ecore_evas, "E Drag");
   ecore_evas_ignore_events_set(drag->ecore_evas, 1);

   ecore_evas_shaped_set(drag->ecore_evas, 1);

   drag->object = evas_object_rectangle_add(drag->evas);
   evas_object_color_set(drag->object, 255, 0, 0, 255);
   evas_object_show(drag->object);

   evas_object_move(drag->object, 0, 0);
   evas_object_resize(drag->object, drag->w, drag->h);
   ecore_evas_resize(drag->ecore_evas, drag->w, drag->h);

   drag->type = E_DRAG_NONE;

   drag->types = malloc(num_types * sizeof(char *));
   for (i = 0; i < num_types; i++)
     drag->types[i] = strdup(types[i]);
   drag->num_types = num_types;
   drag->data = data;
   drag->data_size = size;
   drag->cb.finished = finished_cb;

   _drag_list = evas_list_append(_drag_list, drag);

   return drag;
}

EAPI Evas *
e_drag_evas_get(E_Drag *drag)
{
   return drag->evas;
}

EAPI void
e_drag_object_set(E_Drag *drag, Evas_Object *object)
{
   if (drag->object) evas_object_del(drag->object);
   drag->object = object;
   evas_object_resize(drag->object, drag->w, drag->h);
}

EAPI void
e_drag_resize(E_Drag *drag, int w, int h)
{
   if ((drag->w == w) && (drag->h == h)) return;
   drag->h = h;
   drag->w = w;
   evas_object_resize(drag->object, drag->w, drag->h);
   ecore_evas_resize(drag->ecore_evas, drag->w, drag->h);
   e_container_shape_resize(drag->shape, drag->w, drag->h);
}

EAPI int
e_dnd_active(void)
{
   return (_drag_win != 0);
}

EAPI int
e_drag_start(E_Drag *drag, int x, int y)
{
   Evas_List *l;
   int i;

   if (_drag_win) return 0;
   _drag_win = ecore_x_window_input_new(drag->container->win, 
					drag->container->x, drag->container->y,
					drag->container->w, drag->container->h);
   ecore_x_window_show(_drag_win);
   if (!e_grabinput_get(_drag_win, 1, _drag_win))
     {
	ecore_x_window_del(_drag_win);
	return 0;
     }

   drag->type = E_DRAG_INTERNAL;

   drag->dx = x - drag->x;
   drag->dy = y - drag->y;

   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;
	
	h->active = 0;
	for (i = 0; i < h->num_types; i++)
	  {
	     if (!strcmp(h->types[i], drag->types[0]))
	       h->active = 1;
	  }
	h->entered = 0;
     }

   _drag_current = drag;
   return 1;
}

EAPI int
e_drag_xdnd_start(E_Drag *drag, int x, int y)
{
   if (_drag_win) return 0;
   _drag_win = ecore_x_window_input_new(drag->container->win, 
					drag->container->x, drag->container->y,
					drag->container->w, drag->container->h);
   ecore_x_window_show(_drag_win);
   if (!e_grabinput_get(_drag_win, 1, _drag_win))
     {
	ecore_x_window_del(_drag_win);
	return 0;
     }

   drag->type = E_DRAG_XDND;

   drag->dx = x - drag->x;
   drag->dy = y - drag->y;

   ecore_x_dnd_aware_set(_drag_win, 1);
   ecore_x_dnd_types_set(_drag_win, drag->types, drag->num_types);
   ecore_x_dnd_begin(_drag_win, drag->data, drag->data_size);

   _drag_current = drag;
   return 1;
}

EAPI E_Drop_Handler *
e_drop_handler_add(void *data,
		   void (*enter_cb)(void *data, const char *type, void *event),
		   void (*move_cb)(void *data, const char *type, void *event),
		   void (*leave_cb)(void *data, const char *type, void *event),
		   void (*drop_cb)(void *data, const char *type, void *event),
		   const char **types, unsigned int num_types, int x, int y, int w, int h)
{
   E_Drop_Handler *handler;
   int i;

   handler = E_NEW(E_Drop_Handler, 1);
   if (!handler) return NULL;

   handler->cb.data = data;
   handler->cb.enter = enter_cb;
   handler->cb.move = move_cb;
   handler->cb.leave = leave_cb;
   handler->cb.drop = drop_cb;
   handler->num_types = num_types;
   if (num_types)
     {
	handler->types = malloc(num_types * sizeof(char *));
	for (i = 0; i < num_types; i++)
	  handler->types[i] = strdup(types[i]);
     }
   handler->x = x;
   handler->y = y;
   handler->w = w;
   handler->h = h;

   _drop_handlers = evas_list_append(_drop_handlers, handler);

   return handler;
}

EAPI void
e_drop_handler_geometry_set(E_Drop_Handler *handler, int x, int y, int w, int h)
{
   handler->x = x;
   handler->y = y;
   handler->w = w;
   handler->h = h;
}

EAPI void
e_drop_handler_del(E_Drop_Handler *handler)
{
   int i;

   _drop_handlers = evas_list_remove(_drop_handlers, handler);
   if (handler->types)
     {
	for (i = 0; i < handler->num_types; i++)
	  free(handler->types[i]);
	free(handler->types);
     }
   free(handler);
}


EAPI void
e_drag_idler_before(void)
{
   Evas_List *l;
   
   for (l = _drag_list; l; l = l->next)
     {
	E_Drag *drag;
	
	drag = l->data;
	if (drag->need_shape_export)
	  {
	     Ecore_X_Rectangle *rects, *orects;
	     int num;
	     
	     rects = ecore_x_window_shape_rectangles_get(drag->evas_win, &num);
	     if (rects)
	       {
		  int changed;
		  
		  changed = 1;
		  if ((num == drag->shape_rects_num) && (drag->shape_rects))
		    {
		       int i;
		       
		       orects = drag->shape_rects;
		       for (i = 0; i < num; i++)
			 {
			    if ((orects[i].x != rects[i].x) ||
				(orects[i].y != rects[i].y) ||
				(orects[i].width != rects[i].width) ||
				(orects[i].height != rects[i].height))
			      {
				 changed = 1;
				 break;
			      }
			 }
		       changed = 0;
		    }
		  if (changed)
		    {
		       E_FREE(drag->shape_rects);
		       drag->shape_rects = rects;
		       drag->shape_rects_num = num;
		       e_container_shape_rects_set(drag->shape, rects, num);
		    }
		  else
		    free(rects);
	       }
	     else
	       {
		  E_FREE(drag->shape_rects);
		  drag->shape_rects = NULL;
		  drag->shape_rects_num = 0;
		  e_container_shape_rects_set(drag->shape, NULL, 0);
	       }
	     drag->need_shape_export = 0;
	     if (drag->visible)
	       e_container_shape_show(drag->shape);
	  }
     }
}

/* local subsystem functions */

static void
_e_drag_show(E_Drag *drag)
{
   if (drag->visible) return;
   drag->visible = 1;
   evas_object_show(drag->object);
   ecore_evas_show(drag->ecore_evas);
   e_container_shape_show(drag->shape);
}

static void
_e_drag_hide(E_Drag *drag)
{
   if (!drag->visible) return;
   drag->visible = 0;
   evas_object_hide(drag->object);
   ecore_evas_hide(drag->ecore_evas);
   e_container_shape_hide(drag->shape);
}

static void
_e_drag_move(E_Drag *drag, int x, int y)
{
   E_Zone *zone;
   
   if (((drag->x + drag->dx) == x) && ((drag->y + drag->dy) == y)) return;

   //FIXME: I think the timer needs to be cleaned up by passing (-1, -1) someplace
   zone = e_container_zone_at_point_get(drag->container, x, y);
   if (zone) e_zone_flip_coords_handle(zone, x, y);
   
   drag->x = x - drag->dx;
   drag->y = y - drag->dy;
   ecore_evas_move(drag->ecore_evas,
		   drag->x, 
		   drag->y);
   e_container_shape_move(drag->shape,
			  drag->x, 
			  drag->y);
}

static void
_e_drag_update(int x, int y)
{
   Evas_List *l;
   E_Event_Dnd_Enter *enter_ev;
   E_Event_Dnd_Move *move_ev;
   E_Event_Dnd_Leave *leave_ev;

   if (_drag_current)
     {
	_e_drag_show(_drag_current);
	_e_drag_move(_drag_current, x, y);
     }

   enter_ev = E_NEW(E_Event_Dnd_Enter, 1);
   enter_ev->x = x;
   enter_ev->y = y;
   
   move_ev = E_NEW(E_Event_Dnd_Move, 1);
   move_ev->x = x;
   move_ev->y = y;

   leave_ev = E_NEW(E_Event_Dnd_Leave, 1);
   leave_ev->x = x;
   leave_ev->y = y;

   if ((_drag_current) && (_drag_current->types))
     {
	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;
	     
	     h = l->data;
	     
	     if (!h->active)
	       continue;
	     
	     if (E_INSIDE(x, y, h->x, h->y, h->w, h->h))
	       {
		  if (!h->entered)
		    {
		       if (h->cb.enter)
			 h->cb.enter(h->cb.data, _drag_current->types[0], enter_ev);
		       h->entered = 1;
		    }
		  if (h->cb.move)
		    h->cb.move(h->cb.data, _drag_current->types[0], move_ev);
	       }
	     else
	       {
		  if (h->entered)
		    {
		       if (h->cb.leave)
			 h->cb.leave(h->cb.data, _drag_current->types[0], leave_ev);
		       h->entered = 0;
		    }
	       }
	  }
     }
   free(enter_ev);
   free(move_ev);
   free(leave_ev);
}

static void
_e_drag_end(int x, int y)
{
   Evas_List *l;
   E_Event_Dnd_Drop *ev;
   const char *type = NULL;

   if (!_drag_current) return;

   _e_drag_hide(_drag_current);

   e_grabinput_release(_drag_win, _drag_win);
   if (_drag_current->type == E_DRAG_XDND)
     {
	e_object_del(E_OBJECT(_drag_current));
	_drag_current = NULL;
	if (!ecore_x_dnd_drop())
	  {
	     ecore_x_window_del(_drag_win);
	     _drag_win = 0;
	  }
	return;
     }

   ecore_x_window_del(_drag_win);
   _drag_win = 0;

   ev = E_NEW(E_Event_Dnd_Drop, 1);
   ev->data = _drag_current->data;
   type = _drag_current->types[0];
   ev->x = x;
   ev->y = y;

   if (ev->data)
     {
	int dropped;

	dropped = 0;
	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;

	     h = l->data;

	     if (!h->active)
	       continue;

	     if ((h->cb.drop) &&
		   E_INSIDE(x, y, h->x, h->y, h->w, h->h))
	       {
		  h->cb.drop(h->cb.data, type, ev);
		  dropped = 1;
	       }
	  }
	if (_drag_current->cb.finished)
	  _drag_current->cb.finished(_drag_current, dropped);
	e_object_del(E_OBJECT(_drag_current));
	_drag_current = NULL;
     }
   else
     {
	/* Just leave */
	E_Event_Dnd_Leave *leave_ev;

	leave_ev = E_NEW(E_Event_Dnd_Leave, 1);
	/* FIXME: We don't need x and y in leave */
	leave_ev->x = 0;
	leave_ev->y = 0;

	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;

	     h = l->data;

	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, type, leave_ev);
		  h->entered = 0;
	       }
	  }
     }

   free(ev);
}

static void
_e_drag_xdnd_end(int x, int y)
{
   Evas_List *l;
   E_Event_Dnd_Drop *ev;
   const char *type = NULL;

   if (!_xdnd) return;

   ev = E_NEW(E_Event_Dnd_Drop, 1);
   ev->data = _xdnd->data;
   type = _xdnd->type;
   ev->x = x;
   ev->y = y;

   if (ev->data)
     {
	int dropped;

	dropped = 0;
	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;

	     h = l->data;

	     if (!h->active)
	       continue;

	     if ((h->cb.drop) &&
		 E_INSIDE(x, y, h->x, h->y, h->w, h->h))
	       {
		  h->cb.drop(h->cb.data, type, ev);
		  dropped = 1;
	       }
	  }
     }
   else
     {
	/* Just leave */
	E_Event_Dnd_Leave *leave_ev;

	leave_ev = E_NEW(E_Event_Dnd_Leave, 1);
	/* FIXME: We don't need x and y in leave */
	leave_ev->x = 0;
	leave_ev->y = 0;

	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;

	     h = l->data;

	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, type, leave_ev);
		  h->entered = 0;
	       }
	  }
     }

   free(ev);
}

static void
_e_drag_free(E_Drag *drag)
{
   int i;

   _drag_list = evas_list_remove(_drag_list, drag);

   E_FREE(drag->shape_rects);
   drag->shape_rects_num = 0;
   e_object_unref(E_OBJECT(drag->container));
   e_container_shape_hide(drag->shape);
   e_object_del(E_OBJECT(drag->shape));
   evas_object_del(drag->object);
   e_canvas_del(drag->ecore_evas);
   ecore_evas_free(drag->ecore_evas);
   for (i = 0; i < drag->num_types; i++)
     free(drag->types[i]);
   free(drag->types);
   free(drag);
}


static int
_e_dnd_cb_window_shape(void *data, int ev_type, void *ev)
{
   Evas_List *l;
   Ecore_X_Event_Window_Shape *e;
   
   e = ev;
   for (l = _drag_list; l; l = l->next)
     {
	E_Drag *drag;
	
	drag = l->data;
	if (drag->evas_win == e->win)
	  drag->need_shape_export = 1;
     }
   return 1;
}

static int
_e_dnd_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;

   ev = event;
   if (ev->win != _drag_win) return 1;

   _e_drag_end(ev->x, ev->y);

   return 1;
}

static int
_e_dnd_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   if (ev->win != _drag_win) return 1;

   _e_drag_update(ev->x, ev->y);
   return 1;
}

static int
_e_dnd_cb_event_dnd_enter(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Enter *ev;
   E_Container *con;
   Evas_List *l;
   int i, j;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   if (ev->source == _drag_win) return 1;
   printf("Xdnd enter\n");
   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;

	h->active = 0;
	h->entered = 0;
     }
   for (i = 0; i < ev->num_types; i++)
     {
	/* FIXME: Maybe we want to get something else then files dropped? */
	if (!strcmp("text/uri-list", ev->types[i]))
	  {
	     _xdnd = E_NEW(XDnd, 1);
	     _xdnd->type = strdup("text/uri-list");
	     for (l = _drop_handlers; l; l = l->next)
	       {
		  E_Drop_Handler *h;

		  h = l->data;

		  h->active = 0;
		  for (j = 0; j < h->num_types; j++)
		    {
		       if (!strcmp(h->types[j], _xdnd->type))
			 h->active = 1;
		    }

		  h->entered = 0;
	       }
	     break;
	  }
#if 0
	else if (!strcmp("text/x-moz-url", ev->types[i]))
	  {
	     _xdnd = E_NEW(XDnd, 1);
	     _xdnd->type = strdup("text/x-moz-url");
	     for (l = _drop_handlers; l; l = l->next)
	       {
		  E_Drop_Handler *h;

		  h = l->data;

		  h->active = !strcmp(h->type, "enlightenment/x-file");
		  h->entered = 0;
	       }
	     break;
	  }
#endif
     }
   return 1;
}

static int
_e_dnd_cb_event_dnd_leave(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Leave *ev;
   E_Container *con;
   E_Event_Dnd_Leave *leave_ev;
   Evas_List *l;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   if (ev->source == _drag_win) return 1;
   printf("Xdnd leave\n");

   leave_ev = E_NEW(E_Event_Dnd_Leave, 1);
   /* FIXME: We don't need x and y in leave */
   leave_ev->x = 0;
   leave_ev->y = 0;

   if (_xdnd)
     {
	for (l = _drop_handlers; l; l = l->next)
	  {
	     E_Drop_Handler *h;

	     h = l->data;

	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, _xdnd->type, leave_ev);
		  h->entered = 0;
	       }
	  }

	free(_xdnd->type);
	free(_xdnd);
	_xdnd = NULL;
     }
   free(leave_ev);
   return 1;
}

static int
_e_dnd_cb_event_dnd_position(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Position *ev;
   E_Container *con;
   Ecore_X_Rectangle rect;
   Evas_List *l;

   int active;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   if (ev->source == _drag_win) return 1;

   rect.x = 0;
   rect.y = 0;
   rect.width = 0;
   rect.height = 0;

   active = 0;
   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;
	if (h->active)
	  active = 1;
     }
   if (!active)
     {
	ecore_x_dnd_send_status(0, 0, rect, ECORE_X_DND_ACTION_PRIVATE);
     }
   else
     {
	_e_drag_update(ev->position.x, ev->position.y);
	ecore_x_dnd_send_status(1, 0, rect, ECORE_X_DND_ACTION_PRIVATE);
     }
   return 1;
}

static int
_e_dnd_cb_event_dnd_status(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Status *ev;

   ev = event;
   if (ev->win != _drag_win) return 1;

   return 1;
}

static int
_e_dnd_cb_event_dnd_finished(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Finished *ev;

   /*
    * TODO:
    * - Check action
    * - Do something if not completed
    */

   ev = event;
   if (ev->win != _drag_win) return 1;
   printf("Xdnd finished\n");

   if (!ev->completed)
     printf("FIXME: XDnd not completed, need to delay deleting _drag_win!!\n");

   ecore_x_window_del(_drag_win);
   _drag_win = 0;
   return 1;
}

static int
_e_dnd_cb_event_dnd_drop(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Drop *ev;
   E_Container *con;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   if (ev->source == _drag_win) return 1;
   printf("Xdnd drop\n");

   ecore_x_selection_xdnd_request(ev->win, _xdnd->type);

   _xdnd->x = ev->position.x;
   _xdnd->y = ev->position.y;
   return 1;
}

static int
_e_dnd_cb_event_dnd_selection(void *data, int type, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
   E_Container *con;
   int i;

   ev = event;
   con = data;
   if ((con->bg_win != ev->win) ||
       (ev->selection != ECORE_X_SELECTION_XDND)) return 1;
   printf("Xdnd selection\n");

   if (!strcmp("text/uri-list", _xdnd->type))
     {
	Ecore_X_Selection_Data_Files   *files;
	Evas_List *l = NULL;

	files = ev->data;
	for (i = 0; i < files->num_files; i++)
	  l = evas_list_append(l, files->files[i]), printf("file: %s\n", files->files[i]);
	_xdnd->data = l;
	_e_drag_xdnd_end(_xdnd->x, _xdnd->y);
	evas_list_free(l);
     }
   else if (!strcmp("text/x-moz-url", _xdnd->type))
     {
	/* FIXME: Create a ecore x parser for this type */
	Ecore_X_Selection_Data *data;
	Evas_List *l = NULL;
	char file[PATH_MAX];
	char *text;
	int i, size;

	data = ev->data;
	text = (char *)data->data;
	size = MIN(data->length, PATH_MAX - 1);
	/* A moz url _shall_ contain a space */
	/* FIXME: The data is two-byte unicode. Somewhere it
	 * is written that the url and the text is separated by
	 * a space, but it seems like they are separated by
	 * newline
	 */
	for (i = 0; i < size; i++)
	  {
	     file[i] = text[i];
	     printf("'%d-%c' ", text[i], text[i]);
	     /*
	     if (text[i] == ' ')
	       {
		  file[i] = '\0';
		  break;
	       }
	       */
	  }
	printf("\n");
	file[i] = '\0';
	printf("file: %d \"%s\"\n", i, file);
	l = evas_list_append(l, file);

	_xdnd->data = l;
	_e_drag_xdnd_end(_xdnd->x, _xdnd->y);
	evas_list_free(l);
     }
   else
     {
	_e_drag_xdnd_end(_xdnd->x, _xdnd->y);
     }
   /* FIXME: When to execute this? It could be executed in ecore_x after getting
    * the drop property... */
   ecore_x_dnd_send_finished();
   free(_xdnd->type);
   free(_xdnd);
   _xdnd = NULL;
   return 1;
}

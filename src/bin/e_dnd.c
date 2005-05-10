/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

static void _e_drag_free(E_Drag *drag);

static int  _e_dnd_cb_mouse_up(void *data, int type, void *event);
static int  _e_dnd_cb_mouse_move(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_enter(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_leave(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_position(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_drop(void *data, int type, void *event);
static int  _e_dnd_cb_event_dnd_selection(void *data, int type, void *event);

/* local subsystem globals */

static Evas_List *_event_handlers = NULL;
static Evas_List *_drop_handlers = NULL;

static Ecore_X_Window _drag_win = 0;

static Evas_List *_draggies = NULL;
static E_Drag *_drag_current = NULL;

/* externally accessible functions */

int
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

int
e_dnd_shutdown(void)
{
   Evas_List *l;

   for (l = _draggies; l;)
     {
	E_Drag *drag;

	drag = l->data;
	l = l->next;
	e_object_del(E_OBJECT(drag));
     }
   evas_list_free(_draggies);
   _draggies = NULL;

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

E_Drag*
e_drag_new(E_Container *container,
	   const char *type, void *data,
	   void (*finished_cb)(E_Drag *drag, int dropped),
	   const char *icon_path, const char *icon)
{
   E_Drag *drag;

   drag = E_OBJECT_ALLOC(E_Drag, E_DRAG_TYPE, _e_drag_free);
   if (!drag) return NULL;

   drag->x = 0;
   drag->y = 0;
   drag->w = 24;
   drag->h = 24;
   drag->layer = 250;
   drag->container = container;
   e_object_ref(E_OBJECT(drag->container));
   if (e_canvas_engine_decide(e_config->evas_engine_drag) ==
       E_EVAS_ENGINE_GL_X11)
     {
	drag->ecore_evas = ecore_evas_gl_x11_new(NULL,
						 drag->container->win,
						 drag->x, drag->y,
						 drag->w, drag->h);
	ecore_evas_gl_x11_direct_resize_set(drag->ecore_evas, 1);
	drag->evas_win = ecore_evas_gl_x11_window_get(drag->ecore_evas);
     }
   else
     {
	drag->ecore_evas = ecore_evas_software_x11_new(NULL,
						      drag->container->win,
						      drag->x, drag->y,
						      drag->w, drag->h);
	ecore_evas_software_x11_direct_resize_set(drag->ecore_evas, 1);
	drag->evas_win = ecore_evas_software_x11_window_get(drag->ecore_evas);
     }
   e_canvas_add(drag->ecore_evas);
   drag->shape = e_container_shape_add(drag->container);
   e_container_shape_move(drag->shape, drag->x, drag->y);
   e_container_shape_resize(drag->shape, drag->w, drag->h);

   drag->evas = ecore_evas_get(drag->ecore_evas);
   e_container_window_raise(drag->container, drag->evas_win, drag->layer);
   ecore_x_window_shape_events_select(drag->evas_win, 1);
   ecore_evas_name_class_set(drag->ecore_evas, "E", "_e_drag_window");
   ecore_evas_title_set(drag->ecore_evas, "E Drag");

   ecore_evas_shaped_set(drag->ecore_evas, 1);

   drag->object = edje_object_add(drag->evas);
   edje_object_file_set(drag->object, icon_path, icon);
   evas_object_show(drag->object);

   evas_object_move(drag->object, 0, 0);
   evas_object_resize(drag->object, drag->w, drag->h);
   ecore_evas_resize(drag->ecore_evas, drag->w, drag->h);
   
   drag->type = strdup(type);
   drag->data = data;
   drag->cb.finished = finished_cb;

   return drag;
}

void
e_drag_show(E_Drag *drag)
{
   if (drag->visible) return;
   drag->visible = 1;
   ecore_evas_show(drag->ecore_evas);
   e_container_shape_show(drag->shape);
}

void
e_drag_hide(E_Drag *drag)
{
   if (!drag->visible) return;
   drag->visible = 0;
   ecore_evas_hide(drag->ecore_evas);
   e_container_shape_hide(drag->shape);
}

void
e_drag_move(E_Drag *drag, int x, int y)
{
   if ((drag->x == x) && (drag->y == y)) return;
   drag->x = x;
   drag->y = y;
   ecore_evas_move(drag->ecore_evas,
		   drag->x, 
		   drag->y);
   e_container_shape_move(drag->shape,
			  drag->x, 
			  drag->y);
}

void
e_drag_resize(E_Drag *drag, unsigned int w, unsigned int h)
{
   if ((drag->w == w) && (drag->h == h)) return;
   drag->h = h;
   drag->w = w;
   ecore_evas_resize(drag->ecore_evas, drag->w, drag->h);
   e_container_shape_resize(drag->shape, drag->w, drag->h);
}

int
e_dnd_active(void)
{
   return (_drag_win != 0);
}

void
e_drag_start(E_Drag *drag)
{
   Evas_List *l;

   _drag_win = ecore_x_window_input_new(drag->container->win, 
					drag->container->x, drag->container->y,
					drag->container->w, drag->container->h);
   ecore_x_window_show(_drag_win);
   ecore_x_pointer_confine_grab(_drag_win);
   ecore_x_keyboard_grab(_drag_win);

   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;
	
	h->active = !strcmp(h->type, drag->type);
	h->entered = 0;
     }

   _drag_current = drag;
}

void
e_drag_update(int x, int y)
{
   Evas_List *l;
   E_Event_Dnd_Enter *enter_ev;
   E_Event_Dnd_Move *move_ev;
   E_Event_Dnd_Leave *leave_ev;

   e_drag_show(_drag_current);
   e_drag_move(_drag_current, x, y);

   enter_ev = E_NEW(E_Event_Dnd_Enter, 1);
   enter_ev->x = x;
   enter_ev->y = y;
   
   move_ev = E_NEW(E_Event_Dnd_Move, 1);
   move_ev->x = x;
   move_ev->y = y;

   leave_ev = E_NEW(E_Event_Dnd_Leave, 1);
   leave_ev->x = x;
   leave_ev->y = y;

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
		    h->cb.enter(h->data, _drag_current->type, enter_ev);
		  h->entered = 1;
	       }
	     if (h->cb.move)
	       h->cb.move(h->data, _drag_current->type, move_ev);
	  }
	else
	  {
	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->data, _drag_current->type, leave_ev);
		  h->entered = 0;
	       }
	  }
     }

   free(enter_ev);
   free(move_ev);
   free(leave_ev);
}

void
e_drag_end(int x, int y)
{
   Evas_List *l;
   E_Event_Dnd_Drop *ev;
   int dropped;

   e_drag_hide(_drag_current);

   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_window_del(_drag_win);
   _drag_win = 0;

   ev = E_NEW(E_Event_Dnd_Drop, 1);
   ev->data = _drag_current->data;
   ev->x = x;
   ev->y = y;

   dropped = 0;
   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;

	if (!h->active)
	  continue;
	
	if ((h->cb.drop)
	    && E_INSIDE(x, y, h->x, h->y, h->w, h->h))
	  {
	     h->cb.drop(h->data, _drag_current->type, ev);
	     dropped = 1;
	  }
     }
   if (_drag_current->cb.finished)
     _drag_current->cb.finished(_drag_current, dropped);
   e_object_del(E_OBJECT(_drag_current));
   _drag_current = NULL;

   free(ev);
}

E_Drop_Handler *
e_drop_handler_add(void *data,
		   void (*enter_cb)(void *data, const char *type, void *event),
		   void (*move_cb)(void *data, const char *type, void *event),
		   void (*leave_cb)(void *data, const char *type, void *event),
		   void (*drop_cb)(void *data, const char *type, void *event),
		   const char *type, int x, int y, int w, int h)
{
   E_Drop_Handler *handler;

   handler = E_NEW(E_Drop_Handler, 1);
   if (!handler) return NULL;

   handler->data = data;
   handler->cb.enter = enter_cb;
   handler->cb.move = move_cb;
   handler->cb.leave = leave_cb;
   handler->cb.drop = drop_cb;
   handler->type = strdup(type);
   handler->x = x;
   handler->y = y;
   handler->w = w;
   handler->h = h;

   _drop_handlers = evas_list_append(_drop_handlers, handler);

   return handler;
}

void
e_drop_handler_del(E_Drop_Handler *handler)
{
   free(handler->type);
   free(handler);
}

/* local subsystem functions */

static void
_e_drag_free(E_Drag *drag)
{
   _draggies = evas_list_remove(_draggies, drag);

   e_object_unref(E_OBJECT(drag->container));
   evas_object_del(drag->object);
   e_canvas_del(drag->ecore_evas);
   ecore_evas_free(drag->ecore_evas);
   free(drag->type);
   free(drag);
}

static int
_e_dnd_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;

   ev = event;
   if (ev->win != _drag_win) return 1;

   e_drag_end(ev->x, ev->y);

   return 1;
}

static int
_e_dnd_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   if (ev->win != _drag_win) return 1;

   e_drag_update(ev->x, ev->y);
   return 1;
}

static int
_e_dnd_cb_event_dnd_enter(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Enter *ev;
   E_Container *con;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   printf("Xdnd enter\n");
   return 1;
}

static int
_e_dnd_cb_event_dnd_leave(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Leave *ev;
   E_Container *con;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   printf("Xdnd leave\n");
   return 1;
}

static int
_e_dnd_cb_event_dnd_position(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Position *ev;
   E_Container *con;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   printf("Xdnd pos\n");

#if 0
   for (l = _drop_handlers; l; l = l->next)
     {
	E_Drop_Handler *h;

	h = l->data;
	
	if ((x >= h->x) && (x < h->x + h->w) && (y >= h->y) && (y < h->y + h->h)
	    && (!strcmp(h->type, drag_type)))
	  {
	     h->func(h->data, drag_type, ev);
	  }
     }
#endif

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
   printf("Xdnd drop\n");
   return 1;
}

static int
_e_dnd_cb_event_dnd_selection(void *data, int type, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
   E_Container *con;

   ev = event;
   con = data;
   if (con->bg_win != ev->win) return 1;
   printf("Xdnd selection\n");
   return 1;
}

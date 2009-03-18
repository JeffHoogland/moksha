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
static void _e_drag_coords_update(E_Drop_Handler *h, int *dx, int *dy, int *dw, int *dh);
static Ecore_X_Window _e_drag_win_get(E_Drop_Handler *h, int xdnd);
static int  _e_drag_win_matches(E_Drop_Handler *h, Ecore_X_Window win, int xdnd);
static void _e_drag_win_show(E_Drop_Handler *h);
static void _e_drag_win_hide(E_Drop_Handler *h);
static int _e_drag_update(Ecore_X_Window root, int x, int y, Ecore_X_Atom action);
static void _e_drag_end(Ecore_X_Window root, int x, int y);
static void _e_drag_xdnd_end(Ecore_X_Window root, int x, int y);
static void _e_drag_free(E_Drag *drag);

static int  _e_dnd_cb_window_shape(void *data, int type, void *event);

static int  _e_dnd_cb_key_down(void *data, int type, void *event);
static int  _e_dnd_cb_key_up(void *data, int type, void *event);
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

static Eina_List *_event_handlers = NULL;
static Eina_List *_drop_handlers = NULL;
static Eina_Hash *_drop_win_hash = NULL;

static Ecore_X_Window _drag_win = 0;
static Ecore_X_Window _drag_win_root = 0;

static Eina_List *_drag_list = NULL;
static E_Drag    *_drag_current = NULL;

static XDnd *_xdnd = NULL;

static Eina_Hash *_drop_handlers_responsives;
static Ecore_X_Atom _action;

/* externally accessible functions */

EAPI int
e_dnd_init(void)
{
   _drop_win_hash = eina_hash_string_superfast_new(NULL);
   _drop_handlers_responsives = eina_hash_string_superfast_new(NULL);

   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
							      _e_dnd_cb_mouse_up, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
							      _e_dnd_cb_mouse_move, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,
							      _e_dnd_cb_window_shape, NULL));

   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_ENTER,
							      _e_dnd_cb_event_dnd_enter, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_LEAVE,
							      _e_dnd_cb_event_dnd_leave, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_POSITION,
							      _e_dnd_cb_event_dnd_position, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_STATUS,
							      _e_dnd_cb_event_dnd_status, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_FINISHED,
							      _e_dnd_cb_event_dnd_finished, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_XDND_DROP,
							      _e_dnd_cb_event_dnd_drop, NULL));
   _event_handlers = eina_list_append(_event_handlers,
				      ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY,
							      _e_dnd_cb_event_dnd_selection, NULL));
   _event_handlers = eina_list_append(_event_handlers,
                                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                              _e_dnd_cb_key_down, NULL));
   _event_handlers = eina_list_append(_event_handlers,
                                      ecore_event_handler_add(ECORE_EVENT_KEY_UP,
                                                              _e_dnd_cb_key_up, NULL));

   _action = ECORE_X_ATOM_XDND_ACTION_PRIVATE;
   return 1;
}

EAPI int
e_dnd_shutdown(void)
{
   Ecore_Event_Handler *h;

   while (_drag_list)
     {
	E_Drag *drag;

	drag = _drag_list->data;
	e_object_del(E_OBJECT(drag));
     }

   EINA_LIST_FREE(_event_handlers, h)
     ecore_event_handler_del(h);

   eina_hash_free(_drop_win_hash);
   eina_list_free(_drop_handlers);
   _drop_handlers = NULL;

   eina_hash_free(_drop_handlers_responsives);

   return 1;
}

EAPI E_Drag*
e_drag_new(E_Container *container, int x, int y,
	   const char **types, unsigned int num_types,
	   void *data, int size,
	   void *(*convert_cb)(E_Drag *drag, const char *type),
	   void (*finished_cb)(E_Drag *drag, int dropped))
{
   E_Drag *drag;
   int i;

   /* No need to create a drag object without type */
   if ((!types) || (!num_types)) return NULL;
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
   if (e_config->use_composite)
     {
	ecore_evas_alpha_set(drag->ecore_evas, 1);
	if (e_config->evas_engine_drag == ECORE_EVAS_ENGINE_SOFTWARE_16_X11)
	  drag->evas_win = ecore_evas_software_x11_window_get(drag->ecore_evas);
	else if (e_config->evas_engine_drag == ECORE_EVAS_ENGINE_XRENDER_X11)
	  drag->evas_win = ecore_evas_xrender_x11_window_get(drag->ecore_evas);
     }
   else
     {
	/* avoid excess exposes when shaped - set damage avoid to 1 */
	ecore_evas_avoid_damage_set(drag->ecore_evas, 1);
	ecore_evas_shaped_set(drag->ecore_evas, 1);
	ecore_x_window_shape_events_select(drag->evas_win, 1);
     }
   
   e_canvas_add(drag->ecore_evas);
   drag->shape = e_container_shape_add(drag->container);
   e_container_shape_move(drag->shape, drag->x, drag->y);
   e_container_shape_resize(drag->shape, drag->w, drag->h);

   drag->evas = ecore_evas_get(drag->ecore_evas);
   e_container_window_raise(drag->container, drag->evas_win, drag->layer);

   ecore_evas_name_class_set(drag->ecore_evas, "E", "_e_drag_window");
   ecore_evas_title_set(drag->ecore_evas, "E Drag");
   ecore_evas_ignore_events_set(drag->ecore_evas, 1);

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
   drag->cb.convert = convert_cb;
   drag->cb.finished = finished_cb;

   _drag_list = eina_list_append(_drag_list, drag);

   ecore_x_window_shadow_tree_flush();
   
   _drag_win_root = drag->container->manager->root;

   drag->cb.key_down = NULL;
   drag->cb.key_up = NULL;
   
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
e_drag_move(E_Drag *drag, int x, int y)
{
   if ((drag->x == x) && (drag->y == y)) return;
   drag->x = x;
   drag->y = y;
   drag->xy_update = 1;
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
   const Eina_List *l;
   E_Drop_Handler *h;

   if (_drag_win) return 0;
   _drag_win = ecore_x_window_input_new(drag->container->win, 
					drag->container->x, drag->container->y,
					drag->container->w, drag->container->h);
   _drag_win_root = drag->container->manager->root;
   ecore_x_window_show(_drag_win);
   if (!e_grabinput_get(_drag_win, 1, _drag_win))
     {
	ecore_x_window_del(_drag_win);
	return 0;
     }

   drag->type = E_DRAG_INTERNAL;

   drag->dx = x - drag->x;
   drag->dy = y - drag->y;

   EINA_LIST_FOREACH(_drop_handlers, l, h)
     {
	int i, j;

	h->active = 0;
	h->active_type = NULL;
	for (i = 0; i < h->num_types; i++)
	  {
	     for (j = 0; j < drag->num_types; j++)
	       {
		  if (!strcmp(h->types[i], drag->types[j]))
		    {
		       h->active = 1;
		       h->active_type = h->types[i];
		       break;
		    }
	       }
	     if (h->active) break;
	  }
	h->entered = 0;
     }

   _drag_current = drag;
   return 1;
}

EAPI int
e_drag_xdnd_start(E_Drag *drag, int x, int y)
{
   Ecore_X_Atom actions[] = {ECORE_X_DND_ACTION_MOVE, ECORE_X_DND_ACTION_PRIVATE, 
			     ECORE_X_DND_ACTION_COPY, ECORE_X_DND_ACTION_ASK,
			     ECORE_X_DND_ACTION_LINK};
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
   ecore_x_dnd_actions_set(_drag_win, actions, 5);
   ecore_x_dnd_begin(_drag_win, drag->data, drag->data_size);

   _drag_current = drag;
   return 1;
}

EAPI E_Drop_Handler *
e_drop_handler_add(E_Object *obj,
		   void *data,
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

   handler->obj = obj;
   handler->entered = 0;
   
   _drop_handlers = eina_list_append(_drop_handlers, handler);

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

EAPI int
e_drop_inside(E_Drop_Handler *handler, int x, int y)
{
   int dx, dy, dw, dh;

   _e_drag_coords_update(handler, &dx, &dy, &dw, &dh);
   return E_INSIDE(x, y, dx, dy, dw, dh);
}

EAPI void
e_drop_handler_del(E_Drop_Handler *handler)
{
   int i;

   _drop_handlers = eina_list_remove(_drop_handlers, handler);
   if (handler->types)
     {
	for (i = 0; i < handler->num_types; i++)
	  free(handler->types[i]);
	free(handler->types);
     }
   free(handler);
}

EAPI int
e_drop_xdnd_register_set(Ecore_X_Window win, int reg)
{
   const char *id;

   id = e_util_winid_str_get(win);
   if (reg)
     {
	if (!eina_hash_find(_drop_win_hash, id))
	  {
	     ecore_x_dnd_aware_set(win, 1);
	     eina_hash_add(_drop_win_hash, id, (void *)1);
	  }
     }
   else
     {
	ecore_x_dnd_aware_set(win, 0);
	eina_hash_del(_drop_win_hash, id, (void *) 1);
     }
   return 1;
}

EAPI void
e_drag_idler_before(void)
{
   const Eina_List *l;
   E_Drag *drag;

   EINA_LIST_FOREACH(_drag_list, l, drag)
     {
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
        if (drag->xy_update)
	  {
	     ecore_evas_move(drag->ecore_evas, drag->x, drag->y);
	     e_container_shape_move(drag->shape, drag->x, drag->y);
	     drag->xy_update = 0;
	  }
     }
}

EAPI void
e_drop_handler_responsive_set(E_Drop_Handler *handler)
{
   Ecore_X_Window hwin = _e_drag_win_get(handler, 1);
   const char *wid = e_util_winid_str_get(hwin);

   eina_hash_add(_drop_handlers_responsives, wid, (void *)handler);
}

EAPI int
e_drop_handler_responsive_get(E_Drop_Handler *handler)
{
   Ecore_X_Window hwin = _e_drag_win_get(handler, 1);
   const char *wid = e_util_winid_str_get(hwin);

   return eina_hash_find(_drop_handlers_responsives, wid) == (void *)handler;
}

EAPI void
e_drop_handler_action_set(Ecore_X_Atom action)
{
   _action = action;
}

EAPI Ecore_X_Atom
e_drop_handler_action_get()
{
   return _action;
}

EAPI void
e_drag_key_down_cb_set(E_Drag *drag, void (*func)(E_Drag *drag, Ecore_Event_Key *e))
{
   drag->cb.key_down = func;
}

EAPI void
e_drag_key_up_cb_set(E_Drag *drag, void (*func)(E_Drag *drag, Ecore_Event_Key *e))
{
   drag->cb.key_up = func;
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

   zone = e_container_zone_at_point_get(drag->container, x, y);
   if (zone) e_zone_flip_coords_handle(zone, x, y);
   
   drag->x = x - drag->dx;
   drag->y = y - drag->dy;
   drag->xy_update = 1;
}

static void
_e_drag_coords_update(E_Drop_Handler *h, int *dx, int *dy, int *dw, int *dh)
{
   int px = 0, py = 0;
   
   *dx = h->x;
   *dy = h->y;
   *dw = h->w;
   *dh = h->h;
   if (h->obj)
     {
	switch (h->obj->type)
	  {
	   case E_GADCON_TYPE:
	     e_gadcon_canvas_zone_geometry_get((E_Gadcon *)(h->obj), &px, &py, NULL, NULL);
	     break;
	   case E_GADCON_CLIENT_TYPE:
	     evas_object_geometry_get(((E_Gadcon_Client *)(h->obj))->o_box, dx, dy, dw, dh);
	     e_gadcon_canvas_zone_geometry_get(((E_Gadcon_Client *)(h->obj))->gadcon, &px, &py, NULL, NULL);
	     break;
	   case E_WIN_TYPE:
	     px = ((E_Win *)(h->obj))->x;
	     py = ((E_Win *)(h->obj))->y;
	     break;
	   case E_ZONE_TYPE:
// zone based drag targets are in a container thus their coords should be
// screen-relative as containers just cover the screen
//	     px = ((E_Zone *)(h->obj))->x;
//	     py = ((E_Zone *)(h->obj))->y;
	     break;
	   case E_BORDER_TYPE:
	     px = ((E_Border *)(h->obj))->x + ((E_Border *)(h->obj))->fx.x;
	     py = ((E_Border *)(h->obj))->y + ((E_Border *)(h->obj))->fx.y;
	     break;
	   case E_POPUP_TYPE:
	     px = ((E_Popup *)(h->obj))->x;
	     py = ((E_Popup *)(h->obj))->y;
	     break;
	     /* FIXME: add more types as needed */
	   default:
	     break;
	  }
     }
   *dx += px;
   *dy += py;
}

static Ecore_X_Window
_e_drag_win_get(E_Drop_Handler *h, int xdnd)
{
   Ecore_X_Window hwin = 0;
   
   if (h->obj)
     {
	switch (h->obj->type)
	  {
	   case E_GADCON_TYPE:
	     if (xdnd) hwin = e_gadcon_xdnd_window_get((E_Gadcon *)(h->obj));
	     else hwin = e_gadcon_dnd_window_get((E_Gadcon *)(h->obj));
	     break;
	   case E_GADCON_CLIENT_TYPE:
	     if (xdnd) hwin = e_gadcon_xdnd_window_get(((E_Gadcon_Client *)(h->obj))->gadcon);
	     else hwin = e_gadcon_dnd_window_get(((E_Gadcon_Client *)(h->obj))->gadcon);
	     break;
	   case E_WIN_TYPE:
	     hwin = ((E_Win *)(h->obj))->evas_win;
	     break;
	   case E_ZONE_TYPE:
	     /* Not quite sure about this, probably need to set up 
	      * E_Container to pass DND events from event_win to bg_win. */
	     // hwin = ((E_Zone *)(h->obj))->container->event_win;
	     hwin = ((E_Zone *)(h->obj))->container->bg_win;
	     break;
	   case E_BORDER_TYPE:
	     hwin = ((E_Border *)(h->obj))->event_win;
	     break;
	   case E_POPUP_TYPE:
	     hwin = ((E_Popup *)(h->obj))->evas_win;
	     break;
	     /* FIXME: add more types as needed */
	   default:
	     break;
	  }
     }

   return hwin;
}

static int
_e_drag_win_matches(E_Drop_Handler *h, Ecore_X_Window win, int xdnd)
{
   Ecore_X_Window hwin = _e_drag_win_get(h, xdnd);

   if (win == hwin) return 1;
   return 0;
}

static void
_e_drag_win_show(E_Drop_Handler *h)
{
   E_Shelf *shelf;

   if (h->obj)
     {
	switch (h->obj->type)
	  {
	   case E_GADCON_TYPE:
	     shelf = e_gadcon_shelf_get((E_Gadcon *)(h->obj));
	     if (shelf) e_shelf_toggle(shelf, 1);
	     break;
	   case E_GADCON_CLIENT_TYPE:
	     shelf = e_gadcon_shelf_get(((E_Gadcon_Client *)(h->obj))->gadcon);
	     if (shelf) e_shelf_toggle(shelf, 1);
	     break;
	     /* FIXME: add more types as needed */
	   default:
	     break;
	  }
     }
}

static void
_e_drag_win_hide(E_Drop_Handler *h)
{
   E_Shelf *shelf;

   if (h->obj)
     {
	switch (h->obj->type)
	  {
	   case E_GADCON_TYPE:
	      shelf = e_gadcon_shelf_get((E_Gadcon *)(h->obj));
	      if (shelf) e_shelf_toggle(shelf, 0);
	      break;
	   case E_GADCON_CLIENT_TYPE:
	      shelf = e_gadcon_shelf_get(((E_Gadcon_Client *)(h->obj))->gadcon);
	      if (shelf) e_shelf_toggle(shelf, 0);
	      break;
	     /* FIXME: add more types as needed */
	   default:
	     break;
	  }
     }
}

static int
_e_drag_update(Ecore_X_Window root, int x, int y, Ecore_X_Atom action)
{
   const Eina_List *l;
   E_Event_Dnd_Enter enter_ev;
   E_Event_Dnd_Move move_ev;
   E_Event_Dnd_Leave leave_ev;
   int dx, dy, dw, dh;
   Ecore_X_Window win, ignore_win[2];
   int responsive = 0;
   static struct 
     {
	Ecore_X_Window root, win;
     } cache = {0, 0};

//   double t1 = ecore_time_get(); ////
   if (_drag_current && !_xdnd)
     {
	ignore_win[0] = _drag_current->evas_win;
	ignore_win[1] = _drag_win;
	/* FIXME: this is nasty. every x mouse event we go back to x and do
	 * a whole bunch of round-trips narrowing down the toplevel window
	 * which contains the mouse */
	if (cache.win != root)
	  {
	     /* A little bit of caching to avoid trips to X. */
	     cache.win = root;
	     cache.root = ecore_x_window_root_get(root);
	  }
	win = ecore_x_window_shadow_tree_at_xy_with_skip_get(cache.root, x, y, ignore_win, 2);
//	win = ecore_x_window_at_xy_with_skip_get(x, y, ignore_win, 2);
     }
   else
     win = root;

   if (_drag_current)
     {
	_e_drag_show(_drag_current);
	_e_drag_move(_drag_current, x, y);
     }

   if (_drag_current)
     {
	E_Drop_Handler *h;
	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active) continue;
	     _e_drag_coords_update(h, &dx, &dy, &dw, &dh);
	     enter_ev.x = x - dx;
	     enter_ev.y = y - dy;
	     enter_ev.data = NULL;
	     enter_ev.action = action;
	     move_ev.x = x - dx;
	     move_ev.y = y - dy;
	     move_ev.action = action;
	     leave_ev.x = x - dx;
	     leave_ev.y = y - dy;
	     
	     if (E_INSIDE(x, y, dx, dy, dw, dh) && 
		 _e_drag_win_matches(h, win, 0))
	       {
		  if (e_drop_handler_responsive_get(h)) responsive = 1;

		  if (!h->entered)
		    {
		       _e_drag_win_show(h);
		       if (h->cb.enter)
			 {
			    if (_drag_current->cb.convert)
			      {
				 enter_ev.data = _drag_current->cb.convert(_drag_current,
									   h->active_type);
			      }
			    else
			      enter_ev.data = _drag_current->data;
			    h->cb.enter(h->cb.data, h->active_type, &enter_ev);
			 }
		       h->entered = 1;
		    }
		  if (h->cb.move)
		    h->cb.move(h->cb.data, h->active_type, &move_ev);
	       }
	     else
	       {
		  if (h->entered)
		    {
		       if (h->cb.leave)
			 h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		       _e_drag_win_hide(h);
		       h->entered = 0;
		    }
	       }
	  }
     }
   else if (_xdnd)
     {
	E_Drop_Handler *h;
	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active) continue;
	     _e_drag_coords_update(h, &dx, &dy, &dw, &dh);
	     enter_ev.x = x - dx;
	     enter_ev.y = y - dy;
	     enter_ev.action = action;
	     move_ev.x = x - dx;
	     move_ev.y = y - dy;
	     move_ev.action = action;
	     leave_ev.x = x - dx;
	     leave_ev.y = y - dy;
	     if (E_INSIDE(x, y, dx, dy, dw, dh) && _e_drag_win_matches(h, win, 1))
	       {
		  if (e_drop_handler_responsive_get(h)) responsive = 1;

		  if (!h->entered)
		    {
		       if (h->cb.enter)
			 h->cb.enter(h->cb.data, h->active_type, &enter_ev);
		       h->entered = 1;
		    }
		  if (h->cb.move)
		    h->cb.move(h->cb.data, h->active_type, &move_ev);
	       }
	     else
	       {
		  if (h->entered)
		    {
		       if (h->cb.leave)
			 h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		       h->entered = 0;
		    }
	       }
	  }
     }

   return responsive;
//   double t2 = ecore_time_get() - t1; ////
//   printf("DND UPDATE %3.7f\n", t2); ////
}

static void
_e_drag_end(Ecore_X_Window root, int x, int y)
{
   E_Zone *zone;
   const Eina_List *l;
   E_Event_Dnd_Drop ev;
   int dx, dy, dw, dh;
   Ecore_X_Window win, ignore_win[2];
   E_Drag *tmp;

   if (!_drag_current) return;
   ignore_win[0] = _drag_current->evas_win;
   ignore_win[1] = _drag_win;
   /* this is nasty - but necessary to get the window stacking */
   win = ecore_x_window_shadow_tree_at_xy_with_skip_get(root, x, y, ignore_win, 2);
//   win = ecore_x_window_at_xy_with_skip_get(x, y, ignore_win, 2);
   zone = e_container_zone_at_point_get(_drag_current->container, x, y);
   /* Pass -1, -1, so that it is possible to drop at the edge. */
   if (zone) e_zone_flip_coords_handle(zone, -1, -1);

   _e_drag_hide(_drag_current);

   e_grabinput_release(_drag_win, _drag_win);
   if (_drag_current->type == E_DRAG_XDND)
     {
	int dropped;

	if (!(dropped = ecore_x_dnd_drop()))
	  {
	     ecore_x_window_del(_drag_win);
	     _drag_win = 0;
	  }
	if (_drag_current->cb.finished)
	  _drag_current->cb.finished(_drag_current, dropped);

	if (_drag_current && !_xdnd)
	  {
	     tmp = _drag_current;
	     _drag_current = NULL;
	     e_object_del(E_OBJECT(tmp));
	  }
	return;
     }

   ecore_x_window_del(_drag_win);
   _drag_win = 0;

   if (_drag_current->data)
     {
	E_Drop_Handler *h;
	int dropped = 0;

	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active) continue;
	     _e_drag_coords_update(h, &dx, &dy, &dw, &dh);
	     ev.x = x - dx;
	     ev.y = y - dy;
	     if ((_e_drag_win_matches(h, win, 0)) &&
		 ((h->cb.drop) &&
		  (E_INSIDE(x, y, dx, dy, dw, dh))))
	       {
		  if (_drag_current->cb.convert)
		    {
		       ev.data = _drag_current->cb.convert(_drag_current,
							   h->active_type);
		    }
		  else
		    ev.data = _drag_current->data;
		  h->cb.drop(h->cb.data, h->active_type, &ev);
		  dropped = 1;
	       }
	  }
	if (_drag_current->cb.finished)
	  _drag_current->cb.finished(_drag_current, dropped);

	tmp = _drag_current;
	_drag_current = NULL;
	e_object_del(E_OBJECT(tmp));
     }
   else
     {
	/* Just leave */
	E_Event_Dnd_Leave leave_ev;
	E_Drop_Handler *h;

	/* FIXME: We don't need x and y in leave */
	leave_ev.x = 0;
	leave_ev.y = 0;

	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		  h->entered = 0;
	       }
	  }
     }
}

static void
_e_drag_xdnd_end(Ecore_X_Window win, int x, int y)
{
   const Eina_List *l;
   E_Event_Dnd_Drop ev;
   int dx, dy, dw, dh;

   if (!_xdnd) return;

   ev.data = _xdnd->data;

   if (ev.data)
     {
	E_Drop_Handler *h;
	int dropped = 0;

	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active) continue;
	     _e_drag_coords_update(h, &dx, &dy, &dw, &dh);
	     ev.x = x - dx;
	     ev.y = y - dy;
	     if (_e_drag_win_matches(h, win, 1) && h->cb.drop 
		 && E_INSIDE(x, y, dx, dy, dw, dh))
	       {
		  h->cb.drop(h->cb.data, h->active_type, &ev);
		  dropped = 1;
	       }
	  }
     }
   else
     {
	/* Just leave */
	E_Event_Dnd_Leave leave_ev;
	E_Drop_Handler *h;

	/* FIXME: We don't need x and y in leave */
	leave_ev.x = 0;
	leave_ev.y = 0;

	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		  h->entered = 0;
	       }
	  }
     }
}

static void
_e_drag_free(E_Drag *drag)
{
   int i;

   if (drag == _drag_current)
     {
	const Eina_List *l;
	E_Event_Dnd_Leave leave_ev;
	E_Drop_Handler *h;

	e_grabinput_release(_drag_win, _drag_win);
	ecore_x_window_del(_drag_win);
	_drag_win = 0;
	_drag_win_root = 0;

	leave_ev.x = 0;
	leave_ev.y = 0;
	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if ((h->active) && (h->entered))
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		  _e_drag_win_hide(h);
	       }
	  }
	if (drag->cb.finished)
	  drag->cb.finished(drag, 0);
     }

   _drag_current = NULL;

   _drag_list = eina_list_remove(_drag_list, drag);

   E_FREE(drag->shape_rects);
   drag->shape_rects_num = 0;
   e_object_unref(E_OBJECT(drag->container));
   e_container_shape_hide(drag->shape);
   e_object_del(E_OBJECT(drag->shape));
   evas_object_del(drag->object);
   e_canvas_del(drag->ecore_evas);
   ecore_evas_free(drag->ecore_evas);
   for (i = 0; i < drag->num_types; i++) free(drag->types[i]);
   free(drag->types);
   free(drag);
   ecore_x_window_shadow_tree_flush();
}


static int
_e_dnd_cb_window_shape(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Shape *e = ev;
   const Eina_List *l;
   E_Drag *drag;

   EINA_LIST_FOREACH(_drag_list, l, drag)
     {
	if (drag->evas_win == e->win)
	  drag->need_shape_export = 1;
     }
   return 1;
}

static int
_e_dnd_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->window != _drag_win) return 1;

   if (!_drag_current) return 1;

   if (_drag_current->cb.key_down)
     _drag_current->cb.key_down(_drag_current, ev);
   
   return 1;
}

static int
_e_dnd_cb_key_up(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->window != _drag_win) return 1;

   if (!_drag_current) return 1;

   if (_drag_current->cb.key_up)
     _drag_current->cb.key_up(_drag_current, ev);
   
   return 1;
}

static int
_e_dnd_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (ev->window != _drag_win) return 1;

   _e_drag_end(_drag_win_root, ev->x, ev->y);

   return 1;
}

static int
_e_dnd_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev;

   ev = event;
   if (ev->window != _drag_win) return 1;

   if (!_xdnd) _e_drag_update(_drag_win_root, ev->x, ev->y, ECORE_X_ATOM_XDND_ACTION_PRIVATE);
   
   return 1;
}

static int
_e_dnd_cb_event_dnd_enter(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Enter *ev;
   E_Drop_Handler *h;
   const char *id;
   const Eina_List *l;
   int i, j;

   ev = event;
   id = e_util_winid_str_get(ev->win);
   if (!eina_hash_find(_drop_win_hash, id)) return 1;

   EINA_LIST_FOREACH(_drop_handlers, l, h)
     {
	h->active = 0;
	h->active_type = NULL;
	h->entered = 0;
     }
   for (i = 0; i < ev->num_types; i++)
     {
	/* FIXME: Maybe we want to get something else then files dropped? */
	if (!strcmp("text/uri-list", ev->types[i]))
	  {
	     _xdnd = E_NEW(XDnd, 1);
	     _xdnd->type = strdup("text/uri-list");
	     EINA_LIST_FOREACH(_drop_handlers, l, h)
	       {
		  h->active = 0;
		  h->active_type = NULL;
		  for (j = 0; j < h->num_types; j++)
		    {
		       if (!strcmp(h->types[j], _xdnd->type))
			 {
			    h->active = 1;
			    h->active_type = _xdnd->type;
			    break;
			 }
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
	     EINA_LIST_FOREACH(_drop_handlers, l, h)
	       {
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
   E_Event_Dnd_Leave leave_ev;
   const char *id;
   const Eina_List *l;

   ev = event;

   id = e_util_winid_str_get(ev->win);
   if (!eina_hash_find(_drop_win_hash, id)) return 1;

   leave_ev.x = 0;
   leave_ev.y = 0;

   if (_xdnd)
     {
	E_Drop_Handler *h;
	EINA_LIST_FOREACH(_drop_handlers, l, h)
	  {
	     if (!h->active)
	       continue;

	     if (h->entered)
	       {
		  if (h->cb.leave)
		    h->cb.leave(h->cb.data, h->active_type, &leave_ev);
		  h->entered = 0;
	       }
	  }

	free(_xdnd->type);
	free(_xdnd);
	_xdnd = NULL;
     }
   return 1;
}

static int
_e_dnd_cb_event_dnd_position(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Position *ev;
   Ecore_X_Rectangle rect;
   Ecore_X_Action action;
   const char *id;
   const Eina_List *l;
   E_Drop_Handler *h;

   int active;
   int responsive;

   ev = event;
//   double t1 = ecore_time_get(); ////
   id = e_util_winid_str_get(ev->win);
   if (!eina_hash_find(_drop_win_hash, id))
     {
//	double t2 = ecore_time_get() - t1; ////
//	printf("DND POS EV 1 %3.7f\n", t2); ////
	return 1;
     }

   rect.x = 0;
   rect.y = 0;
   rect.width = 0;
   rect.height = 0;

   action = ev->action;

   active = 0;
   EINA_LIST_FOREACH(_drop_handlers, l, h)
     {
	if (h->active)
	  {
	     active = 1;
	     break;
	  }
     }
   if (!active)
     {
	ecore_x_dnd_send_status(0, 0, rect, ECORE_X_DND_ACTION_PRIVATE);
     }
   else
     {
	responsive = _e_drag_update(ev->win, ev->position.x, ev->position.y, ev->action);
	if (responsive)
	  {
	     ecore_x_dnd_send_status(1, 0, rect, _action);
	  }
	else
	  {
	     ecore_x_dnd_send_status(1, 0, rect, ECORE_X_ATOM_XDND_ACTION_PRIVATE);
	  }
     }
//   double t2 = ecore_time_get() - t1; ////
//   printf("DND POS EV 2 %3.7f\n", t2); ////
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

   ev = event;

   if (!ev->completed)
     return 1;

   if (_drag_current)
     {
	E_Drag *tmp;

	tmp = _drag_current;
	_drag_current = NULL;
	e_object_del(E_OBJECT(tmp));
     }

   e_grabinput_release(_drag_win, _drag_win);
   ecore_x_window_del(_drag_win);
   _drag_win = 0;

   return 1;
}

static int
_e_dnd_cb_event_dnd_drop(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Drop *ev;
   const char *id;

   ev = event;
   id = e_util_winid_str_get(ev->win);
   if (!eina_hash_find(_drop_win_hash, id)) return 1;

   ecore_x_selection_xdnd_request(ev->win, _xdnd->type);

   _xdnd->x = ev->position.x;
   _xdnd->y = ev->position.y;

   return 1;
}

static int
_e_dnd_cb_event_dnd_selection(void *data, int type, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
   const char *id;
   int i;

   ev = event;
   id = e_util_winid_str_get(ev->win);
   if (!eina_hash_find(_drop_win_hash, id)) return 1;
   if (ev->selection != ECORE_X_SELECTION_XDND) return 1;

   if (!strcmp("text/uri-list", _xdnd->type))
     {
	Ecore_X_Selection_Data_Files   *files;
	Eina_List *l = NULL;

	files = ev->data;
	for (i = 0; i < files->num_files; i++)
	  l = eina_list_append(l, files->files[i]);
	_xdnd->data = l;
	_e_drag_xdnd_end(ev->win, _xdnd->x, _xdnd->y);
	eina_list_free(l);
     }
   else if (!strcmp("text/x-moz-url", _xdnd->type))
     {
	/* FIXME: Create a ecore x parser for this type */
	Ecore_X_Selection_Data *data;
	Eina_List *l = NULL;
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
//	printf("\n");
	file[i] = '\0';
//	printf("file: %d \"%s\"\n", i, file);
	l = eina_list_append(l, file);

	_xdnd->data = l;
	_e_drag_xdnd_end(ev->win, _xdnd->x, _xdnd->y);
	eina_list_free(l);
     }
   else
     {
	_e_drag_xdnd_end(ev->win, _xdnd->x, _xdnd->y);
     }
   /* FIXME: When to execute this? It could be executed in ecore_x after getting
    * the drop property... */
   ecore_x_dnd_send_finished();
   free(_xdnd->type);
   free(_xdnd);
   _xdnd = NULL;
   return 1;
}

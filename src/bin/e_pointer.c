/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO
 * - Make fallback user controlable.
 * - Define the allowed signals?
 */

typedef struct _E_Pointer_Stack E_Pointer_Stack;

struct _E_Pointer_Stack
{
   void          *obj;
   const char    *type;
};

static Evas_List *_e_pointers = NULL;
static Evas_List *handlers = NULL;

static void _e_pointer_canvas_add(E_Pointer *p);
static void _e_pointer_canvas_del(E_Pointer *p);
static void _e_pointer_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info);
static void _e_pointer_free(E_Pointer *p);
static void _e_pointer_stack_free(E_Pointer_Stack *elem);
static int  _e_pointer_type_set(E_Pointer *p, const char *type);
static void _e_pointer_active_handle(E_Pointer *p);

static int _e_pointer_cb_mouse_down(void *data, int type, void *event);
static int _e_pointer_cb_mouse_up(void *data, int type, void *event);
static int _e_pointer_cb_mouse_move(void *data, int type, void *event);
static int _e_pointer_cb_mouse_wheel(void *data, int type, void *event);
static int _e_pointer_cb_idle_timer_pre(void *data);
static int _e_pointer_cb_idle_timer_wait(void *data);
static int _e_pointer_cb_idle_poller(void *data);

/* externally accessible functions */
EAPI int
e_pointer_init(void)
{
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_pointer_cb_mouse_down, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_pointer_cb_mouse_up, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE, _e_pointer_cb_mouse_move, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL, _e_pointer_cb_mouse_wheel, NULL));
   return 1;
}

EAPI int
e_pointer_shutdown(void)
{
   while (handlers)
     {
	Ecore_Event_Handler *h;
	
	h = handlers->data;
	handlers = evas_list_remove_list(handlers, handlers);
	ecore_event_handler_del(h);
     }
   
   return 1;
}

EAPI E_Pointer *
e_pointer_window_new(Ecore_X_Window win, int filled)
{
   E_Pointer *p = NULL;

   p = E_OBJECT_ALLOC(E_Pointer, E_POINTER_TYPE, _e_pointer_free);
   if (!p) return NULL;
   
   if (e_config->use_e_cursor)
     {
	p->e_cursor = 1;
	p->win = win;
	/* Init the cursor object */
	if (ecore_x_cursor_color_supported_get()) p->color = 1;
	else p->color = 0;
     }
   else
     {
	p->e_cursor = 0;
	p->win = win;
     }
   ecore_x_cursor_size_set(e_config->cursor_size * 3 / 4);
   if (filled) e_pointer_type_push(p, p, "default");
   _e_pointers = evas_list_append(_e_pointers, p); 
   return p;
}

EAPI void
e_pointers_size_set(int size)
{
   Evas_List *l;

   for (l = _e_pointers; l; l = l->next)
     {
	E_Pointer *p;
	Evas_Engine_Info_Buffer *einfo;

	p = l->data;
	if (p->evas)
	  {
	     p->w = p->h = size;
	     evas_output_size_set(p->evas, p->w, p->h);
	     evas_output_viewport_set(p->evas, 0, 0, p->w, p->h);

	     p->pixels = realloc(p->pixels, p->w * p->h * sizeof(int));

	     einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(p->evas);
	     if (einfo)
	       {
		  einfo->info.dest_buffer = p->pixels;
		  einfo->info.dest_buffer_row_bytes = p->w * sizeof(int);
		  evas_engine_info_set(p->evas, (Evas_Engine_Info *)einfo);
	       }

	     evas_object_move(p->pointer_object, 0, 0);
	     evas_object_resize(p->pointer_object, p->w, p->h);
	  }
	else
	  {
	     const char *type;
	     
	     ecore_x_cursor_size_set(e_config->cursor_size * 3 / 4);
	     type = p->type;
	     if (type)
	       {
		  p->type = NULL;
		  _e_pointer_type_set(p, type);
		  evas_stringshare_del(type);
	       }
	  }
     }
}

EAPI void
e_pointer_type_push(E_Pointer *p, void *obj, const char *type)
{
   E_Pointer_Stack *stack;

   p->e_cursor = e_config->use_e_cursor;
   
   _e_pointer_type_set(p, type);
   
   p->obj = obj;

   stack = E_NEW(E_Pointer_Stack, 1);
   if (stack)
     {
	stack->type = evas_stringshare_add(p->type);
	stack->obj = p->obj;
	p->stack = evas_list_prepend(p->stack, stack);
     }

}

EAPI void
e_pointer_type_pop(E_Pointer *p, void *obj, const char *type)
{
   Evas_List *l;
   E_Pointer_Stack *stack;

   for (l = p->stack; l; l = l->next)
     {
	stack = l->data;

	if ((stack->obj == obj) &&
	    ((!type) || (!strcmp(stack->type, type))))
	  {
	     _e_pointer_stack_free(stack);
	     p->stack = evas_list_remove_list(p->stack, l);
	     if (type) break;
	  }
     }

   if (!p->stack)
     {
	if (p->evas) _e_pointer_canvas_del(p);
	ecore_x_window_cursor_set(p->win, 0);
	if (p->type) evas_stringshare_del(p->type);
	p->type = NULL;
	return;
     }

   stack = p->stack->data;
   _e_pointer_type_set(p, stack->type);

   if (p->type) evas_stringshare_del(p->type);
   p->type = evas_stringshare_add(stack->type);
   p->obj = stack->obj;

   /* try the default cursor next time */
   p->e_cursor = e_config->use_e_cursor;
}

EAPI void
e_pointer_idler_before(void)
{
   Evas_List *l;

   for (l = _e_pointers; l; l = l->next)
     {
	E_Pointer *p;
	Evas_List *updates;

	p = l->data;
	if (!p->e_cursor) continue;
	if (!p->evas) continue;

	updates = evas_render_updates(p->evas);
	if ((updates) || (p->hot.update))
	  {
	     Ecore_X_Cursor cur;

	     cur = ecore_x_cursor_new(p->win, p->pixels, p->w, p->h, p->hot.x, p->hot.y);
	     ecore_x_window_cursor_set(p->win, cur);
	     ecore_x_cursor_free(cur);
	     evas_render_updates_free(updates);
	     p->hot.update = 0;
	  }
     }
}

/* local subsystem functions */
static void
_e_pointer_canvas_add(E_Pointer *p)
{
   Evas_Engine_Info_Buffer *einfo;
   Evas_Object *o;
   int rmethod;
   
   p->w = e_config->cursor_size;
   p->h = e_config->cursor_size;
   
   /* create evas */
   p->evas = evas_new();
   if (!p->evas)
     {
	e_object_del(E_OBJECT(p));
	return;
     }
   rmethod = evas_render_method_lookup("buffer");
   evas_output_method_set(p->evas, rmethod);
   evas_output_size_set(p->evas, p->w, p->h);
   evas_output_viewport_set(p->evas, 0, 0, p->w, p->h);

   p->pixels = malloc(p->w * p->h * sizeof(int));
   if (!p->pixels)
     {
	evas_free(p->evas);
	p->evas = NULL;
	return;
     }
   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(p->evas);
   if (!einfo)
     {
	free(p->pixels);
	p->pixels = NULL;
	evas_free(p->evas);
	p->evas = NULL;
	return;
     }
   einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
   einfo->info.dest_buffer = p->pixels;
   einfo->info.dest_buffer_row_bytes = p->w * sizeof(int);
   einfo->info.use_color_key = 0;
   einfo->info.alpha_threshold = 0;
   einfo->info.func.new_update_region = NULL;
   einfo->info.func.free_update_region = NULL;
   evas_engine_info_set(p->evas, (Evas_Engine_Info *)einfo);
   
   /* set the pointer edje */
   o = edje_object_add(p->evas);
   p->pointer_object = o;
   /* Create the hotspot object */
   o = evas_object_rectangle_add(p->evas);
   evas_object_color_set(o, 0, 0, 0, 0);
   p->hot_object = o;   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
				  _e_pointer_cb_move, p);
   /* init edje */
   evas_object_move(p->pointer_object, 0, 0);
   evas_object_resize(p->pointer_object, p->w, p->h);
   evas_object_show(p->pointer_object);
}

static void
_e_pointer_canvas_del(E_Pointer *p)
{
   if (p->pointer_object) evas_object_del(p->pointer_object);
   if (p->hot_object) evas_object_del(p->hot_object);
   if (p->evas) evas_free(p->evas);
   if (p->pixels) free(p->pixels);
   p->pointer_object = NULL;
   p->hot_object = NULL;
   p->evas = NULL;
   p->pixels = NULL;
}

static void
_e_pointer_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   E_Pointer *p;
   Evas_Coord x, y;

   p = data;
   if (!p->e_cursor) return;
   edje_object_part_geometry_get(p->pointer_object, "e.swallow.hotspot",
				 &x, &y, NULL, NULL);
   printf("@@@@@@@@@@@@@@@@@@@@@@ HOT CHANGE -> %i %i\n", x, y);
   if ((p->hot.x != x) || (p->hot.y != y))
     {
	p->hot.x = x;
	p->hot.y = y;
	p->hot.update = 1;
     }
}

static void
_e_pointer_free(E_Pointer *p)
{
   _e_pointers = evas_list_remove(_e_pointers, p);

   _e_pointer_canvas_del(p);
   
   while (p->stack)
     {
	_e_pointer_stack_free(p->stack->data);
	p->stack = evas_list_remove_list(p->stack, p->stack);
     }

   if (p->type) evas_stringshare_del(p->type);
   
   if (p->idle_timer) ecore_timer_del(p->idle_timer);
   if (p->idle_poller) ecore_poller_del(p->idle_poller);

   p->type = NULL;
   p->idle_timer = NULL;
   p->idle_poller = NULL;
   free(p);
}

static void
_e_pointer_stack_free(E_Pointer_Stack *elem)
{
   if (elem->type) evas_stringshare_del(elem->type);
   free(elem);
}

static int
_e_pointer_type_set(E_Pointer *p, const char *type)
{
   /* Check if this pointer is already set */
   if ((p->type) && (!strcmp(p->type, type))) return 1;

   if (p->type) evas_stringshare_del(p->type);
   p->type = evas_stringshare_add(type);
   
   if (p->e_cursor)
     {
	Evas_Object *o;
	char cursor[1024];
	Evas_Coord x, y;
	
	if (!p->evas) _e_pointer_canvas_add(p);
	o = p->pointer_object;
	if (p->color)
	  {
	     snprintf(cursor, sizeof(cursor), "e/pointer/enlightenment/%s/color", type);
	     if (!e_theme_edje_object_set(o, "base/theme/pointer", cursor))
	       goto fallback;
	  }
	else
	  {
	     snprintf(cursor, sizeof(cursor), "e/pointer/enlightenment/%s/mono", type);
	     if (!e_theme_edje_object_set(o, "base/theme/pointer", cursor))
	       goto fallback;
	  }
	edje_object_part_swallow(p->pointer_object, "e.swallow.hotspot", p->hot_object);
	edje_object_part_geometry_get(p->pointer_object, "e.swallow.hotspot", 
				      &x, &y, NULL, NULL);
	printf("@@@@@@@@@@@@@@@@@@@@@@ HOT @ -> %i %i\n", x, y);
	if ((p->hot.x != x) || (p->hot.y != y))
	  {
	     p->hot.x = x;
	     p->hot.y = y;
	  }
	p->hot.update = 1;
	return 1;
     }
   fallback:
     {
	Ecore_X_Cursor cursor = 0;
	
	if (p->evas) _e_pointer_canvas_del(p);
	if (!strcmp(type, "move"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_FLEUR);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
#if 0
	else if (!strcmp(type, "resize"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_SIZING);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
#endif
	else if (!strcmp(type, "resize_tl"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_LEFT_CORNER);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_t"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_SIDE);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_tr"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_RIGHT_CORNER);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_r"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_RIGHT_SIDE);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_br"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_b"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_SIDE);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_bl"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_LEFT_CORNER);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "resize_l"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_SIDE);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "entry"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_XTERM);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "default"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_PTR);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else if (!strcmp(type, "plus"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_PLUS);
	     if (!cursor) printf("X Cursor for %s is missing\n", type);
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	else
	  {
	     printf("Unknown pointer type: %s\n", type);
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_ARROW);
	     if (!cursor) printf("X Cursor for default is missing\n");
	     ecore_x_window_cursor_set(p->win, cursor);
	  }
	if (cursor) ecore_x_cursor_free(cursor);
     }
   return 1;
}

static void
_e_pointer_active_handle(E_Pointer *p)
{
   /* we got some mouse event - if there was an idle timer emit an active
    * signal as we WERE idle, NOW we are active */
   if (p->idle_timer)
     {
	ecore_timer_del(p->idle_timer);
	p->idle_timer = NULL;
     }
   if (p->idle_poller)
     {
	ecore_poller_del(p->idle_poller);
	p->idle_poller = NULL;
     }
   if (p->idle)
     {
	if (p->pointer_object)
	  edje_object_signal_emit(p->pointer_object, "e,state,mouse,active", "e");
	p->idle = 0;
     }
   if (e_powersave_mode_get() >= E_POWERSAVE_MODE_MEDIUM) return;
   /* and scedule a pre-idle check in 1 second if no more events happen */
   p->idle_timer = ecore_timer_add(1.0, _e_pointer_cb_idle_timer_pre, p);
}

static int
_e_pointer_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   Evas_List *l;
   E_Pointer *p;
                                     
   ev = event;
   for (l = _e_pointers; l; l = l->next)
     {
	p = l->data;
	_e_pointer_active_handle(p);
	if (e_powersave_mode_get() < E_POWERSAVE_MODE_EXTREME)
	  {
	     if (p->pointer_object)
	       edje_object_signal_emit(p->pointer_object, "e,action,mouse,down", "e");
	  }
     }
   return 1;
}

static int
_e_pointer_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   Evas_List *l;
   E_Pointer *p;
                                     
   ev = event;
   for (l = _e_pointers; l; l = l->next)
     {
	p = l->data;
	_e_pointer_active_handle(p);
	if (e_powersave_mode_get() < E_POWERSAVE_MODE_EXTREME)
	  {
	     if (p->pointer_object)
	       edje_object_signal_emit(p->pointer_object, "e,action,mouse,up", "e");
	  }
     }
   return 1;
}

static int
_e_pointer_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;
   Evas_List *l;
   E_Pointer *p;
                                     
   ev = event;
   for (l = _e_pointers; l; l = l->next)
     {
	p = l->data;
	_e_pointer_active_handle(p);
	if (e_powersave_mode_get() < E_POWERSAVE_MODE_HIGH)
	  {
	     if (p->pointer_object)
	       edje_object_signal_emit(p->pointer_object, "e,action,mouse,move", "e");
	  }
     }
   return 1;
}

static int
_e_pointer_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   Evas_List *l;
   E_Pointer *p;
                                     
   ev = event;
   for (l = _e_pointers; l; l = l->next)
     {
	p = l->data;
	_e_pointer_active_handle(p);
	if (e_powersave_mode_get() < E_POWERSAVE_MODE_EXTREME)
	  {
	     if (p->pointer_object)
	       edje_object_signal_emit(p->pointer_object, "e,action,mouse,wheel", "e");
	  }
     }
   return 1;
}

static int
_e_pointer_cb_idle_timer_pre(void *data)
{
   E_Pointer *p;
   int x, y;
   
   p = data;
   ecore_x_pointer_xy_get(p->win, &x, &y);
   p->x = x;
   p->y = y;
   p->idle_timer = ecore_timer_add(4.0, _e_pointer_cb_idle_timer_wait, p);
   return 0;
}

static int
_e_pointer_cb_idle_timer_wait(void *data)
{
   E_Pointer *p;

   p = data;
   if (e_powersave_mode_get() >= E_POWERSAVE_MODE_MEDIUM)
     {
	if (p->idle_poller)
	  ecore_poller_del(p->idle_poller);
	p->idle_poller = NULL;
	p->idle_timer = NULL;
	return 0;
     }
   if (!p->idle_poller)
     p->idle_poller = ecore_poller_add(ECORE_POLLER_CORE, 64,
				       _e_pointer_cb_idle_poller, p);
   p->idle_timer = NULL;
   return 0;
}

static int
_e_pointer_cb_idle_poller(void *data)
{
   E_Pointer *p;
   int x, y;
   
   p = data;
   if (e_powersave_mode_get() >= E_POWERSAVE_MODE_MEDIUM)
     {
	p->idle_poller = NULL;
	return 0;
     }
   /* check if pointer actually moved since the 1 second post-mouse move idle
    * pre-timer that fetches the position */
   ecore_x_pointer_xy_get(p->win, &x, &y);
   if ((x != p->x) || (y != p->y))
     {
	/* it moved - so we are not idle yet - record position and wait 
	 * 4 secons more */
	p->x = x;
	p->y = y;
	if (p->idle)
	  {
	     if (p->pointer_object)
	       edje_object_signal_emit(p->pointer_object, "e,state,mouse,active", "e");
	     p->idle = 0;
	  }
	/* use poller to check from now on */
	return 1;
     }
   /* we are idle - report it if not idle before */
   if (!p->idle)
     {
	if (p->pointer_object)
	  edje_object_signal_emit(p->pointer_object, "e,state,mouse,idle", "e");
	p->idle = 1;
     }
   return 1;
}

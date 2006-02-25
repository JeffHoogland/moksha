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
   char          *type;
   unsigned char  e_cursor : 1;
};

static Evas_List *_e_pointers = NULL;

static void _e_pointer_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info);
static void _e_pointer_free(E_Pointer *p);
static void _e_pointer_stack_free(E_Pointer_Stack *elem);
static int  _e_pointer_type_set(E_Pointer *p, const char *type);

/* externally accessible functions */
EAPI E_Pointer *
e_pointer_window_new(Ecore_X_Window win)
{
   E_Pointer *p = NULL;

   if (e_config->use_e_cursor)
     {
	Evas_Engine_Info_Buffer *einfo;
	Evas_Object *o;
	int rmethod;

	rmethod = evas_render_method_lookup("buffer");
	if (!rmethod) return NULL;

	p = E_OBJECT_ALLOC(E_Pointer, E_POINTER_TYPE, _e_pointer_free);
	if (!p) return NULL;
	p->e_cursor = 1;
	p->win = win;

	p->w = e_config->cursor_size;
	p->h = e_config->cursor_size;

	/* create evas */
	p->evas = evas_new();
	if (!p->evas)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }
	evas_output_method_set(p->evas, rmethod);
	evas_output_size_set(p->evas, p->w, p->h);
	evas_output_viewport_set(p->evas, 0, 0, p->w, p->h);

	p->pixels = malloc(p->w * p->h * sizeof(int));
	if (!p->pixels)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }

	einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(p->evas);
	if (!einfo)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
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
	if (!o)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }
	p->pointer_object = o;

	/* Create the hotspot object */
	o = evas_object_rectangle_add(p->evas);
	if (!o)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }
	p->hot_object = o;
	evas_object_event_callback_add(o,
				       EVAS_CALLBACK_MOVE,
				       _e_pointer_cb_move, p);

	/* Init the cursor object */
	if (ecore_x_cursor_color_supported_get())
	  p->color = 1;
	else
	  p->color = 0;

	/* init edje */
	evas_object_move(p->pointer_object, 0, 0);
	evas_object_resize(p->pointer_object, p->w, p->h);
	evas_object_show(p->pointer_object);

	ecore_x_cursor_size_set(e_config->cursor_size * 3 / 4);
	e_pointer_type_push(p, p, "default");

	_e_pointers = evas_list_append(_e_pointers, p);
     }
   else
     {
	p = E_OBJECT_ALLOC(E_Pointer, E_POINTER_TYPE, _e_pointer_free);
	if (!p) return NULL;
	p->e_cursor = 0;
	p->win = win;

	ecore_x_cursor_size_set(e_config->cursor_size * 3 / 4);
	e_pointer_type_push(p, p, "default");

	_e_pointers = evas_list_append(_e_pointers, p);
     }
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
	if (p->e_cursor)
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
	     char *type;

	     ecore_x_cursor_size_set(e_config->cursor_size * 3 / 4);
	     type = p->type;
	     p->type = NULL;
	     _e_pointer_type_set(p, type);
	     p->type = type;
	  }
     }
}

EAPI void
e_pointer_type_push(E_Pointer *p, void *obj, const char *type)
{
   E_Pointer_Stack *stack;

   p->e_cursor = e_config->use_e_cursor;
   if (!_e_pointer_type_set(p, type))
     {
	p->e_cursor = 0;
	if (!_e_pointer_type_set(p, type))
	  return;
     }

   if (p->type) evas_stringshare_del(p->type);
   p->type = evas_stringshare_add(type);
   p->obj = obj;

   stack = E_NEW(E_Pointer_Stack, 1);
   if (stack)
     {
	stack->obj = p->obj;
	stack->type = evas_stringshare_add(p->type);
	stack->e_cursor = p->e_cursor;
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
	printf("BUG: No pointer on the stack!\n");
	return;
     }

   stack = p->stack->data;
   if ((stack->obj == p->obj) &&
       (!strcmp(stack->type, p->type)))
     {
	/* We already use the top pointer */
	return;
     }

   p->e_cursor = stack->e_cursor;
   if (!_e_pointer_type_set(p, stack->type))
     {
	p->e_cursor = !p->e_cursor;
	if (!_e_pointer_type_set(p, stack->type))
	  {
	     printf("BUG: Can't set cursor!\n");
	     return;
	  }
     }

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
_e_pointer_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   E_Pointer *p;
   Evas_Coord x, y;

   p = data;
   if (!p->e_cursor) return;
   evas_object_geometry_get(p->hot_object, &x, &y, NULL, NULL);
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

   /* free evas */
   if (p->pointer_object) evas_object_del(p->pointer_object);
   if (p->hot_object) evas_object_del(p->hot_object);
   if (p->evas) evas_free(p->evas);
   if (p->pixels) free(p->pixels);

   while (p->stack)
     {
	_e_pointer_stack_free(p->stack->data);
	p->stack = evas_list_remove_list(p->stack, p->stack);
     }

   if (p->type) evas_stringshare_del(p->type);
   free(p);
}

static void
_e_pointer_stack_free(E_Pointer_Stack *elem)
{
   if (elem->type) 
     evas_stringshare_del(elem->type);
   free(elem);
}

static int
_e_pointer_type_set(E_Pointer *p, const char *type)
{
   /* Check if this pointer is already set */
   if ((p->type) && (!strcmp(p->type, type))) return 1;

   if (p->e_cursor)
     {
	Evas_Object *o;
	char cursor[1024];

	o = p->pointer_object;
	if (p->color)
	  {
	     snprintf(cursor, sizeof(cursor), "pointer/enlightenment/%s/color", type);
	     if (!e_theme_edje_object_set(o, "base/theme/pointer", cursor))
	       return 0;
	  }
	else
	  {
	     snprintf(cursor, sizeof(cursor), "pointer/enlightenment/%s/mono", type);
	     if (!e_theme_edje_object_set(o, "base/theme/pointer", cursor))
	       return 0;
	  }
	edje_object_part_swallow(p->pointer_object, "hotspot", p->hot_object);
	p->hot.update = 1;
     }
   else
     {
	Ecore_X_Cursor cursor = 0;
	
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
	else if (!strcmp(type, "default"))
	  {
	     cursor = ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_PTR);
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

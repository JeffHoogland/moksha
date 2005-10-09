/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO
 * - Make fallback user controlable.
 */

static Evas_List *_e_pointers = NULL;

static void _e_pointer_cb_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info);
static void _e_pointer_free(E_Pointer *p);

/* externally accessible functions */
E_Pointer *
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

	p->type = strdup("default");
	if (!p->type)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }

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
	if (ecore_x_cursor_color_supported_get())
	  {
	     if (!e_theme_edje_object_set(o,
					  "base/theme/pointer",
					  "pointer/enlightenment/default/color"))
	       {
		  /* fallback on x cursor */
		  p->e_cursor = 0;
		  free(p->type);
		  p->type = strdup("");
		  free(p->evas);
		  p->evas = NULL;
		  free(p->pixels);
		  p->evas = NULL;
		  e_pointer_type_set(p, "default");
		  return p;
	       }
	     p->color = 1;
	  }
	else
	  {
	     if (!e_theme_edje_object_set(o,
					  "base/theme/pointer",
					  "pointer/enlightenment/default/mono"))
	       {
		  /* fallback on x cursor */
		  p->e_cursor = 0;
		  free(p->type);
		  p->type = strdup("");
		  free(p->evas);
		  p->evas = NULL;
		  free(p->pixels);
		  p->evas = NULL;
		  e_pointer_type_set(p, "default");
		  return p;
	       }
	     p->color = 0;
	  }

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
	edje_object_part_swallow(p->pointer_object, "hotspot", o);

	/* init edje */
	evas_object_move(p->pointer_object, 0, 0);
	evas_object_resize(p->pointer_object, p->w, p->h);
	evas_object_show(p->pointer_object);

	_e_pointers = evas_list_append(_e_pointers, p);
     }
   else
     {
	p = E_OBJECT_ALLOC(E_Pointer, E_POINTER_TYPE, _e_pointer_free);
	if (!p) return NULL;
	p->e_cursor = 0;

	p->type = strdup("default");
	if (!p->type)
	  {
	     e_object_del(E_OBJECT(p));
	     return NULL;
	  }

	p->win = win;

	ecore_x_window_cursor_set(win,
				  ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_PTR));

	_e_pointers = evas_list_append(_e_pointers, p);
     }
   return p;
}

void
e_pointers_size_set(int size)
{
   Evas_List *l;

   for (l = _e_pointers; l; l = l->next)
     {
	E_Pointer *p;
	Evas_Engine_Info_Buffer *einfo;

	p = l->data;
	if (!p->e_cursor) continue;

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
}

void
e_pointer_type_set(E_Pointer *p, const char *type)
{
   if (!strcmp(p->type, type)) return;

   if (p->e_cursor)
     {
	Evas_Object *o;
	char         cursor[1024];

	o = p->pointer_object;
	if (p->color)
	  {
	     snprintf(cursor, sizeof(cursor), "pointer/enlightenment/%s/color", type);
	     if (!e_theme_edje_object_set(o,
					  "base/theme/pointer",
					  cursor))
	       {
		  /* fallback on x cursor */
		  p->e_cursor = 0;
		  e_pointer_type_set(p, type);
		  return;
	       }
	  }
	else
	  {
	     snprintf(cursor, sizeof(cursor), "pointer/enlightenment/%s/mono", type);
	     if (!e_theme_edje_object_set(o,
					  "base/theme/pointer",
					  cursor))
	       {
		  /* fallback on x cursor */
		  p->e_cursor = 0;
		  e_pointer_type_set(p, type);
		  return;
	       }
	  }
	edje_object_part_swallow(p->pointer_object, "hotspot", p->hot_object);
	p->hot.update = 1;
     }
   else
     {
	if (!strcmp(type, "move"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_FLEUR));
	  }
	else if (!strcmp(type, "resize_tl"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_LEFT_CORNER));
	  }
	else if (!strcmp(type, "resize_t"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_SIDE));
	  }
	else if (!strcmp(type, "resize_tr"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_TOP_RIGHT_CORNER));
	  }
	else if (!strcmp(type, "resize_r"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_RIGHT_SIDE));
	  }
	else if (!strcmp(type, "resize_br"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER));
	  }
	else if (!strcmp(type, "resize_b"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_SIDE));
	  }
	else if (!strcmp(type, "resize_bl"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_BOTTOM_LEFT_CORNER));
	  }
	else if (!strcmp(type, "resize_l"))
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_SIDE));
	  }
	else
	  {
	     ecore_x_window_cursor_set(p->win,
				       ecore_x_cursor_shape_get(ECORE_X_CURSOR_LEFT_PTR));
	  }
     }
   /* try the default cursor next time */
   p->e_cursor = e_config->use_e_cursor;

   if (p->type) free(p->type);
   p->type = strdup(type);
}

void
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
	free(p->stack->data);
	p->stack = evas_list_remove_list(p->stack, p->stack);
     }

   if (p->type) free(p->type);
   free(p);
}

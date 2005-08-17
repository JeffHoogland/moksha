/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Evas_List *_e_pointers = NULL;

static void _e_pointer_free(E_Pointer *p);

/* externally accessible functions */
E_Pointer *
e_pointer_window_set(Ecore_X_Window win)
{
   Evas_Engine_Info_Buffer *einfo;
   E_Pointer *p;
   int rmethod;
   Evas_Coord w, h;

   rmethod = evas_render_method_lookup("buffer");
   if (!rmethod) return NULL;

   p = E_OBJECT_ALLOC(E_Pointer, E_POINTER_TYPE, _e_pointer_free);
   if (!p) return NULL;

   p->win = win;

   p->w = 10;
   p->h = 10;

   /* create evas */
   p->evas = evas_new();
   evas_output_method_set(p->evas, rmethod);
   evas_output_size_set(p->evas, p->w, p->h);
   evas_output_viewport_set(p->evas, 0, 0, p->w, p->h);
   
   p->pixels = calloc(p->w * p->h, sizeof(int));
   
   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(p->evas);
   if (einfo)
     {
	einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
	einfo->info.dest_buffer = p->pixels;
	einfo->info.dest_buffer_row_bytes = p->w * sizeof(int);
	einfo->info.use_color_key = 0;
	einfo->info.alpha_threshold = 0;
	einfo->info.func.new_update_region = NULL;
	einfo->info.func.free_update_region = NULL;
	evas_engine_info_set(p->evas, (Evas_Engine_Info *)einfo);
     }

   /* set the pointer edje */
   p->evas_object = edje_object_add(p->evas);
   if (ecore_x_cursor_color_supported_get())
     {
	if (!e_theme_edje_object_set(p->evas_object,
				     "base/theme/pointer",
				     "pointer/enlightenment/default"))
	  {
	     /* error */
	     printf("ERROR: No default theme for pointer!\n");
	     if (!e_theme_edje_object_set(p->evas_object,
					  "base/theme/pointer",
					  "pointer/enlightenment/mono"))
	       {
		  /* error */
		  printf("ERROR: No mono theme for pointer!\n");
	       }
	  }
     }
   else
     {
	if (!e_theme_edje_object_set(p->evas_object,
				     "base/theme/pointer",
				     "pointer/enlightenment/mono"))
	  {
	     /* error */
	     printf("ERROR: No mono theme for pointer!\n");
	  }
     }
   edje_object_calc_force(p->evas_object);
   edje_object_size_min_calc(p->evas_object, &w, &h);
   if ((w == 0) || (h == 0))
     {
	/* error */
	printf("The size of the pointer is 0!\n");
	w = h = 10;
     }
   p->w = w;
   p->h = h;

   /* resize evas */
   evas_output_size_set(p->evas, p->w, p->h);
   evas_output_viewport_set(p->evas, 0, 0, p->w, p->h);
   evas_damage_rectangle_add(p->evas, 0, 0, p->w, p->h);
   
   free(p->pixels);
   p->pixels = calloc(p->w * p->h, sizeof(int));
   
   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(p->evas);
   if (einfo)
     {
	einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
	einfo->info.dest_buffer = p->pixels;
	einfo->info.dest_buffer_row_bytes = p->w * sizeof(int);
	einfo->info.use_color_key = 0;
	einfo->info.alpha_threshold = 0;
	einfo->info.func.new_update_region = NULL;
	einfo->info.func.free_update_region = NULL;
	evas_engine_info_set(p->evas, (Evas_Engine_Info *)einfo);
     }

   /* init edje */
   evas_object_move(p->evas_object, 0, 0);
   evas_object_resize(p->evas_object, p->w, p->h);
   evas_object_show(p->evas_object);

   _e_pointers = evas_list_append(_e_pointers, p);
   return p;
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
	updates = evas_render_updates(p->evas);
	if (updates)
	  {
	     Ecore_X_Cursor cur;
	     Evas_Coord w, h;

	     evas_render_updates_free(updates);

	     /* TODO: Resize evas if pointer changes */
	     evas_object_geometry_get(p->evas_object, NULL, NULL, &w, &h);
	     cur = ecore_x_cursor_new(p->win, p->pixels, p->w, p->h, 0, 0);
	     ecore_x_window_cursor_set(p->win, cur);
	     ecore_x_cursor_free(cur);
	  }
     }
}

/* local subsystem functions */
static void
_e_pointer_free(E_Pointer *p)
{

   _e_pointers = evas_list_remove(_e_pointers, p);

   /* create evas */
   if (p->evas_object) evas_object_del(p->evas_object);
   if (p->evas) evas_free(p->evas);

   free(p->pixels);
   free(p);
}

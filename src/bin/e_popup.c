#include "e.h"

/* local subsystem functions */
static void _e_popup_free(E_Popup *pop);
static Eina_Bool _e_popup_idle_enterer(void *data);
static Eina_Bool _e_popup_cb_window_shape(void *data, int ev_type, void *ev);

/* local subsystem globals */
static Ecore_Event_Handler *_e_popup_window_shape_handler = NULL;
static Eina_List *_e_popup_list = NULL;
static Eina_Hash *_e_popup_hash = NULL;

/* externally accessible functions */

EINTERN int
e_popup_init(void)
{
   _e_popup_window_shape_handler = 
     ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,
			     _e_popup_cb_window_shape, NULL);
   if (!_e_popup_hash) _e_popup_hash = eina_hash_string_superfast_new(NULL);
   return 1;
}

EINTERN int
e_popup_shutdown(void)
{
   if (_e_popup_hash)
     {
        eina_hash_free(_e_popup_hash);
        _e_popup_hash = NULL;
     }
   E_FN_DEL(ecore_event_handler_del, _e_popup_window_shape_handler);
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
				  &(pop->evas_win));
   if (!pop->ecore_evas)
     {
	free(pop);
	return NULL;
     }
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
   eina_hash_add(_e_popup_hash, e_util_winid_str_get(pop->evas_win), pop);
   return pop;
}

EAPI void
e_popup_alpha_set(E_Popup *pop, Eina_Bool alpha)
{
   ecore_evas_alpha_set(pop->ecore_evas, alpha);
}

EAPI Eina_Bool
e_popup_alpha_get(E_Popup *pop)
{
   return ecore_evas_alpha_get(pop->ecore_evas);
}

EAPI void
e_popup_name_set(E_Popup *pop, const char *name)
{
   if (eina_stringshare_replace(&pop->name, name))
     ecore_evas_name_class_set(pop->ecore_evas, "E", pop->name);
}

EAPI void
e_popup_show(E_Popup *pop)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if (pop->visible) return;
   pop->visible = 1;
   if ((pop->shaped) && (!e_config->use_composite))
     {
        ecore_evas_move(pop->ecore_evas,
                        pop->zone->container->manager->w,
                        pop->zone->container->manager->h);
        ecore_evas_show(pop->ecore_evas);
        if (pop->idle_enterer) ecore_idle_enterer_del(pop->idle_enterer);
        pop->idle_enterer = ecore_idle_enterer_add(_e_popup_idle_enterer, pop);
     }
   else
     {
        ecore_evas_show(pop->ecore_evas);
	if (!(pop->shaped && e_config->use_composite))
	  e_container_shape_show(pop->shape);
     }
}

EAPI void
e_popup_hide(E_Popup *pop)
{
   E_OBJECT_CHECK(pop);
   E_OBJECT_TYPE_CHECK(pop, E_POPUP_TYPE);
   if (!pop->visible) return;
   if (pop->idle_enterer) ecore_idle_enterer_del(pop->idle_enterer);
   pop->idle_enterer = NULL;
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
             eina_hash_del(_e_popup_hash, e_util_winid_str_get(pop->evas_win), pop);
	     pop->evas_win = ecore_evas_software_x11_window_get(pop->ecore_evas);
             eina_hash_add(_e_popup_hash, e_util_winid_str_get(pop->evas_win), pop);
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
   E_Popup *pop;

   EINA_LIST_FOREACH(_e_popup_list, l, pop)
     {
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
			    if ((rects[i].x + (int) rects[i].width) > pop->w)
			      rects[i].width = rects[i].width - rects[i].x;
			    if (rects[i].y < 0)
			      {
				 rects[i].height -= rects[i].y;
				 rects[i].y = 0;
			      }
			    if ((rects[i].y + (int) rects[i].height) > pop->h)
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
	if ((pop->visible) && (!pop->idle_enterer) &&
	    (!pop->shaped && e_config->use_composite))
	  e_container_shape_show(pop->shape);
     }
}

EAPI E_Popup *
e_popup_find_by_window(Ecore_X_Window win)
{
   E_Popup *pop;

   pop = eina_hash_find(_e_popup_hash, e_util_winid_str_get(win));
   if ((pop) && (pop->evas_win != win))
     return NULL;
   return pop;
}

/* local subsystem functions */

static void
_e_popup_free(E_Popup *pop)
{
   if (pop->idle_enterer) ecore_idle_enterer_del(pop->idle_enterer);
   pop->idle_enterer = NULL;
   E_FREE(pop->shape_rects);
   pop->shape_rects_num = 0;
   e_container_shape_hide(pop->shape);
   e_object_del(E_OBJECT(pop->shape));
   e_canvas_del(pop->ecore_evas);
   ecore_evas_free(pop->ecore_evas);
   e_object_unref(E_OBJECT(pop->zone));
   pop->zone->popups = eina_list_remove(pop->zone->popups, pop);
   _e_popup_list = eina_list_remove(_e_popup_list, pop);
   eina_hash_del(_e_popup_hash, e_util_winid_str_get(pop->evas_win), pop);
   if (pop->name) eina_stringshare_del(pop->name);
   free(pop);
}

static Eina_Bool
_e_popup_idle_enterer(void *data)
{
   E_Popup *pop;

   if (!(pop = data)) return ECORE_CALLBACK_CANCEL;
   ecore_evas_move(pop->ecore_evas,
		   pop->zone->x + pop->x,
		   pop->zone->y + pop->y);
   e_container_shape_show(pop->shape);
   pop->idle_enterer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_popup_cb_window_shape(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   E_Popup *pop;
   Ecore_X_Event_Window_Shape *e;

   e = ev;
   pop = e_popup_find_by_window(e->win);
   if (pop) pop->need_shape_export = 1;
   return ECORE_CALLBACK_PASS_ON;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * * fix shape callbacks to be able to be safely deleted
 * * remove duplicate bd->layer -> layers code
 *
 */

/* local subsystem functions */
static void _e_container_free(E_Container *con);

static void _e_container_shape_del(E_Container_Shape *es);
static void _e_container_shape_free(E_Container_Shape *es);
static void _e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch);
static void _e_container_resize_handle(E_Container *con);
static void _e_container_event_container_resize_free(void *data, void *ev);

int E_EVENT_CONTAINER_RESIZE = 0;
static int container_count;

/* externally accessible functions */
int
e_container_init(void)
{
   E_EVENT_CONTAINER_RESIZE = ecore_event_type_new();
   container_count = 0;
   return 1;
}

int
e_container_shutdown(void)
{
   return 1;
}

E_Container *
e_container_new(E_Manager *man)
{
   E_Container *con;
   E_Zone *zone;
   Evas_Object *o;
   char name[40];
   Evas_List *l, *screens;
   int i;
   Ecore_X_Window mwin;
   
   con = E_OBJECT_ALLOC(E_Container, E_CONTAINER_TYPE, _e_container_free);
   if (!con) return NULL;
   con->manager = man;
   con->manager->containers = evas_list_append(con->manager->containers, con);
   con->w = con->manager->w;
   con->h = con->manager->h;
   if (e_config->use_virtual_roots)
     {
	con->win = ecore_x_window_override_new(con->manager->win, con->x, con->y, con->w, con->h);
	ecore_x_icccm_title_set(con->win, "Enlightenment Container");
	ecore_x_netwm_name_set(con->win, "Enlightenment Container");
	ecore_x_window_raise(con->win);
     }
   else
     {
	con->win = con->manager->win;
     }
   if (e_canvas_engine_decide(e_config->evas_engine_container) ==
       E_EVAS_ENGINE_GL_X11)
     {
	con->bg_ecore_evas = ecore_evas_gl_x11_new(NULL, con->win, 0, 0, con->w, con->h);
	ecore_evas_gl_x11_direct_resize_set(con->bg_ecore_evas, 1);
	ecore_evas_override_set(con->bg_ecore_evas, 1);
	con->bg_win = ecore_evas_gl_x11_window_get(con->bg_ecore_evas);
     }
   else
     {
	con->bg_ecore_evas = ecore_evas_software_x11_new(NULL, con->win, 0, 0, con->w, con->h);
	ecore_evas_software_x11_direct_resize_set(con->bg_ecore_evas, 1);
	ecore_evas_override_set(con->bg_ecore_evas, 1);
	con->bg_win = ecore_evas_software_x11_window_get(con->bg_ecore_evas);
     }
   e_canvas_add(con->bg_ecore_evas);
   con->bg_evas = ecore_evas_get(con->bg_ecore_evas);
   ecore_evas_name_class_set(con->bg_ecore_evas, "E", "Background_Window");
   ecore_evas_title_set(con->bg_ecore_evas, "Enlightenment Background");
   ecore_evas_avoid_damage_set(con->bg_ecore_evas, 1);
   ecore_x_window_lower(con->bg_win);

   o = evas_object_rectangle_add(con->bg_evas);
   con->bg_blank_object = o;
   evas_object_layer_set(o, -100);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, con->w, con->h);
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_name_set(o, "desktop/background");
   evas_object_data_set(o, "e_container", con);
   evas_object_show(o);
   
   e_pointer_container_set(con);

   con->num = evas_list_count(con->manager->containers) - 1;
   snprintf(name, sizeof(name), _("Container %d"), con->num);
   con->name = strdup(name);

   /* init layers */
   for (i = 0; i < 7; i++)
     {
	con->layers[i].win = ecore_x_window_input_new(con->win, 0, 0, 1, 1);

	if (i > 0)
	  ecore_x_window_configure(con->layers[i].win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   con->layers[i - 1].win, ECORE_X_WINDOW_STACK_ABOVE);
	else
	  ecore_x_window_raise(con->layers[i].win);
     }

   /* Put init win on top */
   mwin = e_init_window_get();
   if (mwin)
     ecore_x_window_configure(mwin,
			      ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			      ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			      0, 0, 0, 0, 0,
			      con->layers[6].win, ECORE_X_WINDOW_STACK_ABOVE);


   /* Put menu win on top */
   mwin = e_menu_grab_window_get();
   if (mwin)
     ecore_x_window_configure(mwin,
			      ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			      ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			      0, 0, 0, 0, 0,
			      con->layers[6].win, ECORE_X_WINDOW_STACK_ABOVE);

   /* Put background win at the bottom */
   ecore_x_window_configure(con->bg_win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[0].win, ECORE_X_WINDOW_STACK_BELOW);

   screens = (Evas_List *)e_xinerama_screens_get();
   if (screens)
     {
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;
	     
	     scr = l->data;
	     zone = e_zone_new(con, scr->screen, scr->x, scr->y, scr->w, scr->h);
	  }
     }
   else
     {
	zone = e_zone_new(con, 0, 0, 0, con->w, con->h);
     }
   con->gadman = e_gadman_new(con);
   
   return con;
}
        
void
e_container_show(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   if (con->visible) return;
   ecore_evas_show(con->bg_ecore_evas);
   ecore_x_window_configure(con->bg_win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[0].win, ECORE_X_WINDOW_STACK_BELOW);
   if (con->win != con->manager->win)
     ecore_x_window_show(con->win);
   ecore_x_icccm_state_set(con->bg_win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
   con->visible = 1;
}
        
void
e_container_hide(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   if (!con->visible) return;
   ecore_evas_hide(con->bg_ecore_evas);
   if (con->win != con->manager->win)
     ecore_x_window_hide(con->win);
   con->visible = 0;
}

E_Container *
e_container_current_get(E_Manager *man)
{
   Evas_List *l;
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);

   for (l = man->containers; l; l = l->next)
     {
	E_Container *con = l->data;
	if (con->visible)
	  return con;
     }
   /* If noone is available, return the first */
   if (!man->containers)
     return NULL;
   l = man->containers;
   return (E_Container *)l->data;
}

void
e_container_move(E_Container *con, int x, int y)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   if ((x == con->x) && (y == con->y)) return;
   con->x = x;
   con->y = y;
   if (con->win != con->manager->win)
     ecore_x_window_move(con->win, con->x, con->y);
   evas_object_move(con->bg_blank_object, con->x, con->y);
}
        
void
e_container_resize(E_Container *con, int w, int h)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   if ((w == con->w) && (h == con->h)) return;
   con->w = w;
   con->h = h;
   if (con->win != con->manager->win)
     ecore_x_window_resize(con->win, con->w, con->h);
   ecore_evas_resize(con->bg_ecore_evas, con->w, con->h);
   evas_object_resize(con->bg_blank_object, con->w, con->h);
   _e_container_resize_handle(con);
}

void
e_container_move_resize(E_Container *con, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   if ((x == con->x) && (y == con->y) && (w == con->w) && (h == con->h)) return;
   con->x = x;
   con->y = y;
   con->w = w;
   con->h = h;
   if (con->win != con->manager->win)
     ecore_x_window_move_resize(con->win, con->x, con->y, con->w, con->h);
   ecore_evas_resize(con->bg_ecore_evas, con->w, con->h);
   evas_object_move(con->bg_blank_object, con->x, con->y);
   evas_object_resize(con->bg_blank_object, con->w, con->h);
   _e_container_resize_handle(con);
}

void
e_container_raise(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
#if 0
   if (con->win != con->manager->win)
     {
	ecore_x_window_raise(con->win);
     }
   else
     {
	ecore_x_window_lower(con->bg_win);
     }
#endif
}

void
e_container_lower(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
#if 0
   if (con->win != con->manager->win)
     ecore_x_window_lower(con->win);
   else
     {
	ecore_x_window_lower(con->bg_win);
     }
#endif
}

E_Zone *
e_container_zone_at_point_get(E_Container *con, int x, int y)
{
   Evas_List *l;
   
   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;
	
	zone = l->data;
	if ((E_SPANS_COMMON(zone->x, zone->w, x, 1)) &&
	    (E_SPANS_COMMON(zone->y, zone->h, y, 1)))
	  return zone;
     }
   return NULL;
}

E_Zone *
e_container_zone_number_get(E_Container *con, int num)
{
   Evas_List *l;
   
   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;
	
	zone = l->data;
	if (zone->num == num)
	  return zone;
     }
   return NULL;
}

E_Container_Shape *
e_container_shape_add(E_Container *con)
{
   E_Container_Shape *es;
   
   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, 0);
   
   es = E_OBJECT_ALLOC(E_Container_Shape, E_CONTAINER_SHAPE_TYPE, _e_container_shape_free);
   E_OBJECT_DEL_SET(es, _e_container_shape_del);
   es->con = con;
   con->shapes = evas_list_append(con->shapes, es);
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_ADD);
   return es;
}

void
e_container_shape_show(E_Container_Shape *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (es->visible) return;
   es->visible = 1;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_SHOW);
}

void
e_container_shape_hide(E_Container_Shape *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (!es->visible) return;
   es->visible = 0;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_HIDE);
}

void
e_container_shape_move(E_Container_Shape *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if ((es->x == x) && (es->y == y)) return;
   es->x = x;
   es->y = y;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_MOVE);
}

void
e_container_shape_resize(E_Container_Shape *es, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   if ((es->w == w) && (es->h == h)) return;
   es->w = w;
   es->h = h;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_RESIZE);
}

Evas_List *
e_container_shape_list_get(E_Container *con)
{
   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   return con->shapes;
}

void
e_container_shape_geometry_get(E_Container_Shape *es, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (x) *x = es->x;
   if (y) *y = es->y;
   if (w) *w = es->w;
   if (h) *h = es->h;
}

E_Container *
e_container_shape_container_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(es, E_CONTAINER_SHAPE_TYPE, NULL);
   return es->con;
}

void
e_container_shape_change_callback_add(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data)
{
   E_Container_Shape_Callback *cb;
   
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   cb = calloc(1, sizeof(E_Container_Shape_Callback));
   if (!cb) return;
   cb->func = func;
   cb->data = data;
   con->shape_change_cb = evas_list_append(con->shape_change_cb, cb);
}

void
e_container_shape_change_callback_del(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data)
{
   Evas_List *l;

   /* FIXME: if we call this from within a callback we are in trouble */
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   for (l = con->shape_change_cb; l; l = l->next)
     {
	E_Container_Shape_Callback *cb;
	
	cb = l->data;
	if ((cb->func == func) && (cb->data == data))
	  {
	     con->shape_change_cb = evas_list_remove_list(con->shape_change_cb, l);
	     free(cb);
	     return;
	  }
     }
}

Evas_List *
e_container_shape_rects_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(es, E_CONTAINER_SHAPE_TYPE, NULL);
   return es->shape;
}

void
e_container_shape_rects_set(E_Container_Shape *es, Ecore_X_Rectangle *rects, int num)
{
   Evas_List *l;
   int i;
   int changed = 1;
   E_Rect *r;
   
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);

   if (num == evas_list_count(es->shape))
     {
	for (i = 0, l = es->shape; (i < num) && (l); i++, l = l->next)
	  {
	     r = l->data;
	     if ((r->x != rects[i].x) || (r->y != rects[i].y) ||
		 (r->w != rects[i].width) || (r->h != rects[i].height))
	       {
		  changed = 1;
		  break;
	       }
	  }
	changed = 0;
     }
   if (!changed)
     {
	return;
     }
   if (es->shape)
     {
	for (l = es->shape; l; l = l->next)
	  free(l->data);
	evas_list_free(es->shape);
	es->shape = NULL;
     }
   if (rects)
     {
	for (i = 0; i < num; i++)
	  {
	     r = malloc(sizeof(E_Rect));
	     if (r)
	       {
		  r->x = rects[i].x;
		  r->y = rects[i].y;
		  r->w = rects[i].width;
		  r->h = rects[i].height;
		  es->shape = evas_list_append(es->shape, r);
	       }
	  }
     }
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_RECTS);
}

void
e_container_shape_solid_rect_set(E_Container_Shape *es, int x, int y, int w, int h)
{
   es->solid_rect.x = x;
   es->solid_rect.y = y;
   es->solid_rect.w = w;
   es->solid_rect.h = h;
}

void
e_container_shape_solid_rect_get(E_Container_Shape *es, int *x, int *y, int *w, int *h)
{
   if (x) *x = es->solid_rect.x;
   if (y) *y = es->solid_rect.y;
   if (w) *w = es->solid_rect.w;
   if (h) *h = es->solid_rect.h;
}

/* layers
 * 0 = desktop
 * 50 = below
 * 100 = normal
 * 150 = above
 * 200 = fullscreen
 * 999 = internal on top windows for E
 */
int
e_container_borders_count(E_Container *con)
{
   /* FIXME: This could be stored and not calculated */
   int num, i;

   num = 0;
   for (i = 0; i < num; i++)
     num += evas_list_count(con->layers[i].clients);

   return num;
}

void
e_container_border_add(E_Border *bd)
{
   int pos;

   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   bd->zone->container->layers[pos].clients =
      evas_list_append(bd->zone->container->layers[pos].clients, bd);
}

void
e_container_border_remove(E_Border *bd)
{
   int i;

   /* FIXME: Could revert to old behaviour, ->layer is consistent
    * with pos now. */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }
}

void
e_container_window_raise(E_Container *con, Ecore_X_Window win, int layer)
{
   int pos;

   if (layer == 0) pos = 1;
   else if ((layer > 0) && (layer <= 50)) pos = 2;
   else if ((layer > 50) && (layer <= 100)) pos = 3;
   else if ((layer > 100) && (layer <= 150)) pos = 4;
   else if ((layer > 150) && (layer <= 200)) pos = 5;
   else pos = 6;

   ecore_x_window_configure(win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[pos].win, ECORE_X_WINDOW_STACK_BELOW);
}

void
e_container_window_lower(E_Container *con, Ecore_X_Window win, int layer)
{
   int pos;

   if (layer == 0) pos = 0;
   else if ((layer > 0) && (layer <= 50)) pos = 1;
   else if ((layer > 50) && (layer <= 100)) pos = 2;
   else if ((layer > 100) && (layer <= 150)) pos = 3;
   else if ((layer > 150) && (layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[pos].win, ECORE_X_WINDOW_STACK_ABOVE);
}

void
e_container_border_raise(E_Border *bd)
{
   int pos, i;

   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   if (bd->layer == 0) pos = 1;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 2;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 3;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 4;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 5;
   else pos = 6;

   ecore_x_window_configure(bd->win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    bd->zone->container->layers[pos].win, ECORE_X_WINDOW_STACK_BELOW);

   bd->zone->container->layers[pos - 1].clients =
      evas_list_append(bd->zone->container->layers[pos - 1].clients, bd);

   e_hints_client_stacking_set();
}

void
e_container_border_lower(E_Border *bd)
{
   int pos, i;
   
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(bd->win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    bd->zone->container->layers[pos].win, ECORE_X_WINDOW_STACK_ABOVE);

   bd->zone->container->layers[pos].clients =
      evas_list_prepend(bd->zone->container->layers[pos].clients, bd);

   e_hints_client_stacking_set();
}

void
e_container_border_stack_above(E_Border *bd, E_Border *above)
{
   int pos;

   /* Remove from old layer */
   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   bd->zone->container->layers[pos].clients =
      evas_list_remove(bd->zone->container->layers[pos].clients, bd);

   /* Add to new layer */
   bd->layer = above->layer;

   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(bd->win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    above->win, ECORE_X_WINDOW_STACK_ABOVE);

   bd->zone->container->layers[pos].clients =
      evas_list_append_relative(bd->zone->container->layers[pos].clients, bd, above);
}

void
e_container_border_stack_below(E_Border *bd, E_Border *below)
{
   int pos;

   /* Remove from old layer */
   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   bd->zone->container->layers[pos].clients =
      evas_list_remove(bd->zone->container->layers[pos].clients, bd);

   /* Add to new layer */
   bd->layer = below->layer;

   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(bd->win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    below->win, ECORE_X_WINDOW_STACK_BELOW);

   bd->zone->container->layers[pos].clients =
      evas_list_prepend_relative(bd->zone->container->layers[pos].clients, bd, below);
}

E_Border_List *
e_container_border_list_first(E_Container *con)
{
   E_Border_List *list;
   list = E_NEW(E_Border_List, 1);
   if (!list) return NULL;
   list->container = con;
   e_object_ref(E_OBJECT(con));
   list->layer = 0;
   list->clients = list->container->layers[list->layer].clients;
   while ((list->layer < 6) && (!list->clients))
     list->clients = list->container->layers[++list->layer].clients;
   return list;
}

E_Border_List *
e_container_border_list_last(E_Container *con)
{
   E_Border_List *list;
   list = E_NEW(E_Border_List, 1);
   if (!list) return NULL;
   list->container = con;
   e_object_ref(E_OBJECT(con));
   list->layer = 6;
   if (list->container->layers[list->layer].clients)
     list->clients = list->container->layers[list->layer].clients->last;
   while ((list->layer > 0) && (!list->clients))
     {
	list->layer--;
	if (list->container->layers[list->layer].clients)
	  list->clients = list->container->layers[list->layer].clients->last;
     }
   return list;
}

E_Border *
e_container_border_list_next(E_Border_List *list)
{
   E_Border *bd;

   if (!list->clients) return NULL;

   bd = list->clients->data;

   list->clients = list->clients->next;
   while ((list->layer < 6) && (!list->clients))
     list->clients = list->container->layers[++list->layer].clients;
   return bd;
}

E_Border *
e_container_border_list_prev(E_Border_List *list)
{
   E_Border *bd;

   if (!list->clients) return NULL;
   
   bd = list->clients->data;

   list->clients = list->clients->prev;
   while ((list->layer > 0) && (!list->clients))
     {
	list->layer--;
	if (list->container->layers[list->layer].clients)
	  list->clients = list->container->layers[list->layer].clients->last;
     }
   return bd;
}

void
e_container_border_list_free(E_Border_List *list)
{
   e_object_unref(E_OBJECT(list->container));
   free(list);
}

void
e_container_all_freeze(void)
{
   Evas_List *managers, *l;
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	Evas_List *ll;
	E_Manager *man;
	
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     E_Container *con;
	     
	     con = ll->data;
	     evas_event_freeze(con->bg_evas);
	  }
     }
}

void
e_container_all_thaw(void)
{
   Evas_List *managers, *l;
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	Evas_List *ll;
	E_Manager *man;
	
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     E_Container *con;
	     
	     con = ll->data;
	     evas_event_thaw(con->bg_evas);
	  }
     }
}

/* local subsystem functions */
static void
_e_container_free(E_Container *con)
{
   Evas_List *l, *tmp;
   int i;

   if (con->gadman) e_object_del(E_OBJECT(con->gadman));
   /* We can't use e_object_del here, because border adds a ref to itself
    * when it is removed, and the ref is never unref'ed */
   for (i = 0; i < 7; i++)
     {
	for (l = con->layers[i].clients; l;)
	  {
	     tmp = l;
	     l = l->next;
	     e_object_free(E_OBJECT(tmp->data));
	  }
     }
   for (l = con->zones; l;)
     {
	tmp = l;
	l = l->next;
	e_object_del(E_OBJECT(tmp->data));
     }
   con->manager->containers = evas_list_remove(con->manager->containers, con);
   e_canvas_del(con->bg_ecore_evas);
   ecore_evas_free(con->bg_ecore_evas);
   if (con->manager->win != con->win)
     {
	ecore_x_window_del(con->win);
     }
   if (con->name)
     free(con->name);
   free(con);
}
   
static void
_e_container_shape_del(E_Container_Shape *es)
{
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_DEL);
}

static void
_e_container_shape_free(E_Container_Shape *es)
{
   Evas_List *l;

   es->con->shapes = evas_list_remove(es->con->shapes, es);
   for (l = es->shape; l; l = l->next)
     free(l->data);
   evas_list_free(es->shape);
   free(es);
}

static void
_e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch)
{
   Evas_List *l;
   
   for (l = es->con->shape_change_cb; l; l = l->next)
     {
	E_Container_Shape_Callback *cb;
	
	cb = l->data;
	cb->func(cb->data, es, ch);
     }
}

static void
_e_container_resize_handle(E_Container *con)
{
   E_Event_Container_Resize *ev;
   Evas_List *l, *screens;
   int i;
   
   ev = calloc(1, sizeof(E_Event_Container_Resize));
   ev->container = con;

   e_xinerama_update();
   
   screens = (Evas_List *)e_xinerama_screens_get();
   if (screens)
     {
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;
	     E_Zone *zone;
	     
	     scr = l->data;
	     zone = e_container_zone_number_get(con, scr->screen);
	     if (zone)
	       {
		  e_zone_move(zone, scr->x, scr->y);
		  e_zone_resize(zone, scr->w, scr->h);
	       }
	     else
	       {
		  zone = e_zone_new(con, scr->screen, scr->x, scr->y, scr->w, scr->h);
	       }
	     /* FIXME: what if a zone exists for a screen that doesn't exist?
	      *        not sure this will ever happen...
	      */
	  }
     }
   else
     {
	E_Zone *zone;
	
	zone = e_container_zone_number_get(con, 0);
	e_zone_move(zone, 0, 0);
	e_zone_resize(zone, con->w, con->h);
     }
   
   e_gadman_container_resize(con->gadman);
   e_object_ref(E_OBJECT(con));
   ecore_event_add(E_EVENT_CONTAINER_RESIZE, ev, _e_container_event_container_resize_free, NULL);
#if 0
   /* FIXME: This is wrong, we should only move/resize to save things from
    * disappearing!
    */
   for (i = 0; i < 7; i++)
     {
	for (l = con->layers[i].clients; l; l = l->next)
	  {
	     E_Border *bd;

	     bd = l->data;

	     if (bd->w > bd->zone->w)
	       e_border_resize(bd, bd->zone->w, bd->h);
	     if ((bd->x + bd->w) > (bd->zone->x + bd->zone->w))
	       e_border_move(bd, bd->zone->x + bd->zone->w - bd->w, bd->y);

	     if (bd->h > bd->zone->h)
	       e_border_resize(bd, bd->w, bd->zone->h);
	     if ((bd->y + bd->h) > (bd->zone->y + bd->zone->h))
	       e_border_move(bd, bd->x, bd->zone->y + bd->zone->h - bd->h);
	  }
     }
#endif
}

static void
_e_container_event_container_resize_free(void *data, void *ev)
{
   E_Event_Container_Resize *e;
   
   e = ev;
   e_object_unref(E_OBJECT(e->container));
   free(e);
}

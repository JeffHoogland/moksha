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

static E_Container *_e_container_find_by_event_window(Ecore_X_Window win);
static void _e_container_modifiers_update(Evas *evas, int modifiers);

static int  _e_container_cb_mouse_in(void *data, int type, void *event);
static int  _e_container_cb_mouse_out(void *data, int type, void *event);
static int  _e_container_cb_mouse_down(void *data, int type, void *event);
static int  _e_container_cb_mouse_up(void *data, int type, void *event);
static int  _e_container_cb_mouse_move(void *data, int type, void *event);
static int  _e_container_cb_mouse_wheel(void *data, int type, void *event);

static void _e_container_shape_del(E_Container_Shape *es);
static void _e_container_shape_free(E_Container_Shape *es);
static void _e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch);
static void _e_container_resize_handle(E_Container *con);
static void _e_container_event_container_resize_free(void *data, void *ev);

EAPI int E_EVENT_CONTAINER_RESIZE = 0;
static int container_count;
static Evas_List *handlers = NULL;

/* externally accessible functions */
EAPI int
e_container_init(void)
{
   E_EVENT_CONTAINER_RESIZE = ecore_event_type_new();
   container_count = 0;

   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, _e_container_cb_mouse_in, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, _e_container_cb_mouse_out, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_container_cb_mouse_down, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_container_cb_mouse_up, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE, _e_container_cb_mouse_move, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL, _e_container_cb_mouse_wheel, NULL));
   return 1;
}

EAPI int
e_container_shutdown(void)
{
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = evas_list_remove_list(handlers, handlers);
     }
   return 1;
}

EAPI E_Container *
e_container_new(E_Manager *man)
{
   E_Container *con;
   E_Zone *zone;
   Evas_Object *o;
   char name[40];
   Evas_List *l, *screens;
   int i;
   Ecore_X_Window mwin;
   static int container_num = 0;

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
     con->win = con->manager->win;

   con->bg_ecore_evas = e_canvas_new(e_config->evas_engine_container, con->win,
				     0, 0, con->w, con->h, 1, 1,
				     &(con->bg_win), NULL);
   e_canvas_add(con->bg_ecore_evas);
   con->event_win = ecore_x_window_input_new(con->bg_win, 0, 0, con->w, con->h);
   ecore_x_window_show(con->event_win);
   con->bg_evas = ecore_evas_get(con->bg_ecore_evas);
   ecore_evas_name_class_set(con->bg_ecore_evas, "E", "Background_Window");
   ecore_evas_title_set(con->bg_ecore_evas, "Enlightenment Background");
   if (getenv("REDRAW_DEBUG"))
     ecore_evas_avoid_damage_set(con->bg_ecore_evas, !atoi(getenv("REDRAW_DEBUG")));
   else
     ecore_evas_avoid_damage_set(con->bg_ecore_evas, ECORE_EVAS_AVOID_DAMAGE_BUILT_IN);
   ecore_x_window_lower(con->bg_win);

   o = evas_object_rectangle_add(con->bg_evas);
   con->bg_blank_object = o;
   evas_object_layer_set(o, -100);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, con->w, con->h);
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_name_set(o, "e/desktop/background");
   evas_object_data_set(o, "e_container", con);
   evas_object_show(o);

   con->num = container_num;
   container_num++;
   snprintf(name, sizeof(name), _("Container %d"), con->num);
   con->name = evas_stringshare_add(name);

   /* init layers */
   for (i = 0; i < 7; i++)
     {
	con->layers[i].win = ecore_x_window_input_new(con->win, 0, 0, 1, 1);
	ecore_x_window_lower(con->layers[i].win);

	if (i > 0)
	  ecore_x_window_configure(con->layers[i].win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   con->layers[i - 1].win, ECORE_X_WINDOW_STACK_ABOVE);
     }

   /* Put init win on top */
   if (man->initwin)
     ecore_x_window_configure(man->initwin,
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
	     zone = e_zone_new(con, scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
	  }
     }
   else
     zone = e_zone_new(con, 0, 0, 0, 0, con->w, con->h);
   return con;
}

EAPI void
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

EAPI void
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

EAPI E_Container *
e_container_current_get(E_Manager *man)
{
   Evas_List *l;
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);

   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;
        
        if (!(con = l->data)) continue;
	if (con->visible) return con;
     }

   /* If noone is available, return the first */
   if (!man->containers) return NULL;
   l = man->containers;
   return (E_Container *)l->data;
}

EAPI E_Container *
e_container_number_get(E_Manager *man, int num)
{
   Evas_List *l;

   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;

	con = l->data;
	if (con->num == num) return con;
     }
   return NULL;
}

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
e_container_raise(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
}

EAPI void
e_container_lower(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
}

EAPI E_Zone *
e_container_zone_at_point_get(E_Container *con, int x, int y)
{
   Evas_List *l = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;

	zone = l->data;
	if (E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
	  return zone;
     }
   return NULL;
}

EAPI E_Zone *
e_container_zone_number_get(E_Container *con, int num)
{
   Evas_List *l = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;

	zone = l->data;
	if (zone->num == num) return zone;
     }
   return NULL;
}

EAPI E_Zone *
e_container_zone_id_get(E_Container *con, int id)
{
   Evas_List *l = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   for (l = con->zones; l; l = l->next)
     {
	E_Zone *zone;

	zone = l->data;
	if (zone->id == id) return zone;
     }
   return NULL;
}

EAPI E_Container_Shape *
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

EAPI void
e_container_shape_show(E_Container_Shape *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (es->visible) return;
   es->visible = 1;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_SHOW);
}

EAPI void
e_container_shape_hide(E_Container_Shape *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (!es->visible) return;
   es->visible = 0;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_HIDE);
}

EAPI void
e_container_shape_move(E_Container_Shape *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if ((es->x == x) && (es->y == y)) return;
   es->x = x;
   es->y = y;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_MOVE);
}

EAPI void
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

EAPI Evas_List *
e_container_shape_list_get(E_Container *con)
{
   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   return con->shapes;
}

EAPI void
e_container_shape_geometry_get(E_Container_Shape *es, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (x) *x = es->x;
   if (y) *y = es->y;
   if (w) *w = es->w;
   if (h) *h = es->h;
}

EAPI E_Container *
e_container_shape_container_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(es, E_CONTAINER_SHAPE_TYPE, NULL);
   return es->con;
}

EAPI void
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

EAPI void
e_container_shape_change_callback_del(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data)
{
   Evas_List *l = NULL;

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

EAPI Evas_List *
e_container_shape_rects_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(es, E_CONTAINER_SHAPE_TYPE, NULL);
   return es->shape;
}

EAPI void
e_container_shape_rects_set(E_Container_Shape *es, Ecore_X_Rectangle *rects, int num)
{
   Evas_List *l = NULL;
   int i;
   E_Rect *r;

   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (es->shape)
     {
	for (l = es->shape; l; l = l->next)
	  free(l->data);
	evas_list_free(es->shape);
	es->shape = NULL;
     }
   if ((rects) && (num == 1) &&
       (rects[0].x == 0) &&
       (rects[0].y == 0) &&
       (rects[0].width == es->w) &&
       (rects[0].height == es->h))
     {
	/* do nothing */
     }
   else if (rects)
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

EAPI void
e_container_shape_solid_rect_set(E_Container_Shape *es, int x, int y, int w, int h)
{
   es->solid_rect.x = x;
   es->solid_rect.y = y;
   es->solid_rect.w = w;
   es->solid_rect.h = h;
}

EAPI void
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
EAPI int
e_container_borders_count(E_Container *con)
{
   return con->clients;
}

EAPI void
e_container_border_add(E_Border *bd)
{
   int pos = 0;

   if (!bd->zone) return;
   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   bd->zone->container->clients++;
   bd->zone->container->layers[pos].clients =
      evas_list_append(bd->zone->container->layers[pos].clients, bd);
   e_hints_client_list_set();
}

EAPI void
e_container_border_remove(E_Border *bd)
{
   int i;

   if (!bd->zone) return;
   /* FIXME: Could revert to old behaviour, ->layer is consistent
    * with pos now. */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }
   bd->zone->container->clients--;
   bd->zone = NULL;
   e_hints_client_list_set();
}

EAPI void
e_container_window_raise(E_Container *con, Ecore_X_Window win, int layer)
{
   int pos = 0;

   if (layer <= 0) pos = 0;
   else if ((layer > 0) && (layer <= 50)) pos = 1;
   else if ((layer > 50) && (layer <= 100)) pos = 2;
   else if ((layer > 100) && (layer <= 150)) pos = 3;
   else if ((layer > 150) && (layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[pos + 1].win, ECORE_X_WINDOW_STACK_BELOW);
}

EAPI void
e_container_window_lower(E_Container *con, Ecore_X_Window win, int layer)
{
   int pos = 0;

   if (layer <= 0) pos = 0;
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

EAPI E_Border *
e_container_border_raise(E_Border *bd)
{
   E_Border *above = NULL;
   Evas_List *l;
   int pos = 0, i;

   if (!bd->zone) return NULL;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   if (bd->layer <= 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   ecore_x_window_configure(bd->win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    bd->zone->container->layers[pos + 1].win, ECORE_X_WINDOW_STACK_BELOW);

   bd->zone->container->layers[pos].clients =
      evas_list_append(bd->zone->container->layers[pos].clients, bd);

   /* Find the window below this one */
   l = evas_list_find_list(bd->zone->container->layers[pos].clients, bd);
   if (l->prev)
     above = l->prev->data;
   else
     {
	/* Need to check the layers below */
	for (i = pos - 2; i >= 0; i--)
	  {
	     if ((bd->zone->container->layers[i].clients) &&
		 (l = evas_list_last(bd->zone->container->layers[i].clients)))
	       {
		  above = l->data;
		  break;
	       }
	  }
     }

   e_hints_client_stacking_set();
   return above;
}

EAPI E_Border *
e_container_border_lower(E_Border *bd)
{
   E_Border *below = NULL;
   Evas_List *l;
   int pos = 0, i;

   if (!bd->zone) return NULL;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   if (bd->layer <= 0) pos = 0;
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

   /* Find the window above this one */
   l = evas_list_find_list(bd->zone->container->layers[pos].clients, bd);
   if (l->next)
     below = l->next->data;
   else
     {
	/* Need to check the layers above */
	for (i = pos + 1; i < 7; i++)
	  {
	     if (bd->zone->container->layers[i].clients)
	       {
		  below = bd->zone->container->layers[i].clients->data;
		  break;
	       }
	  }
     }

   e_hints_client_stacking_set();
   return below;
}

EAPI void
e_container_border_stack_above(E_Border *bd, E_Border *above)
{
   int pos = 0, i;

   if (!bd->zone) return;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   bd->layer = above->layer;

   if (bd->layer <= 0) pos = 0;
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

EAPI void
e_container_border_stack_below(E_Border *bd, E_Border *below)
{
   int pos = 0, i;

   if (!bd->zone) return;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   evas_list_remove(bd->zone->container->layers[i].clients, bd);
     }

   /* Add to new layer */
   bd->layer = below->layer;

   if (bd->layer <= 0) pos = 0;
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

EAPI E_Border_List *
e_container_border_list_first(E_Container *con)
{
   E_Border_List *list = NULL;

   if (!(list = E_NEW(E_Border_List, 1))) return NULL;
   list->container = con;
   e_object_ref(E_OBJECT(con));
   list->layer = 0;
   list->clients = list->container->layers[list->layer].clients;
   while ((list->layer < 6) && (!list->clients))
     list->clients = list->container->layers[++list->layer].clients;
   return list;
}

EAPI E_Border_List *
e_container_border_list_last(E_Container *con)
{
   E_Border_List *list = NULL;

   if (!(list = E_NEW(E_Border_List, 1))) return NULL;
   list->container = con;
   e_object_ref(E_OBJECT(con));
   list->layer = 6;
   if (list->container->layers[list->layer].clients)
     list->clients = evas_list_last(list->container->layers[list->layer].clients);
   while ((list->layer > 0) && (!list->clients))
     {
	list->layer--;
	if (list->container->layers[list->layer].clients)
	  list->clients = evas_list_last(list->container->layers[list->layer].clients);
     }
   return list;
}

EAPI E_Border *
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

EAPI E_Border *
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
	  list->clients = evas_list_last(list->container->layers[list->layer].clients);
     }
   return bd;
}

EAPI void
e_container_border_list_free(E_Border_List *list)
{
   e_object_unref(E_OBJECT(list->container));
   free(list);
}

EAPI void
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

EAPI void
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
   Evas_List *l;

   ecore_x_window_del(con->event_win);
   /* We can't use e_object_del here, because border adds a ref to itself
    * when it is removed, and the ref is never unref'ed */
/* FIXME: had to disable this as it was freeing already freed items during
 * looping (particularly remember/lock config dialogs). this is just
 * disabled until we put in some special handling for this
 *
   int i;

   for (i = 0; i < 7; i++)
     {
	for (l = con->layers[i].clients; l;)
	  {
	     tmp = l;
	     l = l->next;
	     e_object_free(E_OBJECT(tmp->data));
	  }
     }
 */   
   l = con->zones;
   con->zones = NULL;
   while (l)
     {
	e_object_del(E_OBJECT(l->data));
	l = evas_list_remove_list(l, l);
     }
   con->manager->containers = evas_list_remove(con->manager->containers, con);
   e_canvas_del(con->bg_ecore_evas);
   ecore_evas_free(con->bg_ecore_evas);
   if (con->manager->win != con->win)
     {
	ecore_x_window_del(con->win);
     }
   if (con->name) evas_stringshare_del(con->name);
   free(con);
}

static E_Container *
_e_container_find_by_event_window(Ecore_X_Window win)
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
	     if (con->event_win == win) return con;
	  }
     }
   return NULL;
}

static void
_e_container_modifiers_update(Evas *evas, int modifiers)
{
   if (modifiers & ECORE_X_MODIFIER_SHIFT)
     evas_key_modifier_on(evas, "Shift");
   else
     evas_key_modifier_off(evas, "Shift");
   if (modifiers & ECORE_X_MODIFIER_CTRL)
     evas_key_modifier_on(evas, "Control");
   else
     evas_key_modifier_off(evas, "Control");
   if (modifiers & ECORE_X_MODIFIER_ALT)
     evas_key_modifier_on(evas, "Alt");
   else
     evas_key_modifier_off(evas, "Alt");
   if (modifiers & ECORE_X_MODIFIER_WIN)
     {
	evas_key_modifier_on(evas, "Super");
	evas_key_modifier_on(evas, "Hyper");
     }
   else
     {
	evas_key_modifier_off(evas, "Super");
	evas_key_modifier_off(evas, "Hyper");
     }
   if (modifiers & ECORE_X_LOCK_SCROLL)
     evas_key_lock_on(evas, "Scroll_Lock");
   else
     evas_key_lock_off(evas, "Scroll_Lock");
   if (modifiers & ECORE_X_LOCK_NUM)
     evas_key_lock_on(evas, "Num_Lock");
   else
     evas_key_lock_off(evas, "Num_Lock");
   if (modifiers & ECORE_X_LOCK_CAPS)
     evas_key_lock_on(evas, "Caps_Lock");
   else
     evas_key_lock_off(evas, "Caps_Lock");
}

static int
_e_container_cb_mouse_in(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_In *ev;
   E_Border *bd;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
	bd = e_border_focused_get();
	if (bd) e_focus_event_mouse_out(bd);
	_e_container_modifiers_update(con->bg_evas, ev->modifiers);
	evas_event_feed_mouse_in(con->bg_evas, ev->time, NULL);
     }
   return 1;
}

static int
_e_container_cb_mouse_out(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
	_e_container_modifiers_update(con->bg_evas, ev->modifiers);
	if (ev->mode == ECORE_X_EVENT_MODE_GRAB)
	  evas_event_feed_mouse_cancel(con->bg_evas, ev->time, NULL);
        evas_event_feed_mouse_out(con->bg_evas, ev->time, NULL);
     }
   return 1;
}

static int
_e_container_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
        Evas_Button_Flags flags = EVAS_BUTTON_NONE;

	e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_CONTAINER,
					   E_OBJECT(con), ev);
	if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
	if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
	_e_container_modifiers_update(con->bg_evas, ev->modifiers);
	evas_event_feed_mouse_down(con->bg_evas, ev->button, flags, ev->time, NULL);
     }
   return 1;
}

static int
_e_container_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
        evas_event_feed_mouse_up(con->bg_evas, ev->button, EVAS_BUTTON_NONE, ev->time, NULL);
	_e_container_modifiers_update(con->bg_evas, ev->modifiers);
	e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_CONTAINER,
					 E_OBJECT(con), ev);
     }
   return 1;
}

static int
_e_container_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
	_e_container_modifiers_update(con->bg_evas, ev->modifiers);
        evas_event_feed_mouse_move(con->bg_evas, ev->x, ev->y, ev->time, NULL);
     }
   return 1;
}

static int
_e_container_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
	if (!e_bindings_wheel_event_handle(E_BINDING_CONTEXT_CONTAINER,
					   E_OBJECT(con), ev))
	  {
	     _e_container_modifiers_update(con->bg_evas, ev->modifiers);
	     evas_event_feed_mouse_wheel(con->bg_evas, ev->direction, ev->z, ev->time, NULL);
	  }
     }
   return 1;
}

static void
_e_container_shape_del(E_Container_Shape *es)
{
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_DEL);
}

static void
_e_container_shape_free(E_Container_Shape *es)
{
   Evas_List *l = NULL;

   es->con->shapes = evas_list_remove(es->con->shapes, es);
   for (l = es->shape; l; l = l->next)
     free(l->data);
   evas_list_free(es->shape);
   free(es);
}

static void
_e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch)
{
   Evas_List *l = NULL;

   if ((!es) || (!es->con) || (!es->con->shape_change_cb)) return;
   for (l = es->con->shape_change_cb; l; l = l->next)
     {
	E_Container_Shape_Callback *cb;

	if (!(cb = l->data)) continue;
	cb->func(cb->data, es, ch);
     }
}

static void
_e_container_resize_handle(E_Container *con)
{
   E_Event_Container_Resize *ev;
   Evas_List *l, *screens, *zones = NULL;
   int i;

   ev = calloc(1, sizeof(E_Event_Container_Resize));
   ev->container = con;
   e_object_ref(E_OBJECT(con));

   e_xinerama_update();
   screens = (Evas_List *)e_xinerama_screens_get();

   if (screens)
     {
	for (l = con->zones; l; l = l->next)
	  zones = evas_list_append(zones, l->data);
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;
	     E_Zone *zone;

	     scr = l->data;
	     zone = e_container_zone_id_get(con, scr->escreen);
	     if (zone)
	       {
		  e_zone_move_resize(zone, scr->x, scr->y, scr->w, scr->h);
		  e_shelf_zone_move_resize_handle(zone);	
		  zones = evas_list_remove(zones, zone);
	       }
	     else
	       {
		  Evas_List *ll;

		  zone = e_zone_new(con, scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
		  /* find any shelves configured for this zone and add them in */
		  for (ll = e_config->shelves; ll; ll = ll->next)
		    {
		       E_Config_Shelf *cf_es;

		       cf_es = ll->data;
		       if (e_util_container_zone_id_get(cf_es->container, cf_es->zone) == zone)
			 e_shelf_config_new(zone, cf_es);
		    }
	       }
	  }
	if (zones)
	  {
	     E_Zone *spare_zone = NULL;
	     Evas_List *ll;

	     for (ll = con->zones; ll; ll = ll->next)
	       {
		  spare_zone = ll->data;
		  if (evas_list_find(zones, spare_zone))
		    spare_zone = NULL;
		  else break;
	       }
	     while (zones)
	       {
		  E_Zone *zone;
		  Evas_List *shelves, *ll, *del_shelves;
		  E_Border_List *bl;
		  E_Border *bd;

		  zone = zones->data;
		  /* delete any shelves on this zone */
		  shelves = e_shelf_list();
		  del_shelves = NULL;
		  for (ll = shelves; ll; ll = ll->next)
		    {
		       E_Shelf *es;

		       es = ll->data;
		       if (es->zone == zone)
			 del_shelves = evas_list_append(del_shelves, es);
		    }
		  while (del_shelves)
		    {
		       e_object_del(E_OBJECT(del_shelves->data));
		       del_shelves = evas_list_remove_list(del_shelves, del_shelves);
		    }
		  bl = e_container_border_list_first(zone->container);
		  while ((bd = e_container_border_list_next(bl)))
		    {
		       if (bd->zone == zone)
			 {
			    if (spare_zone) e_border_zone_set(bd, spare_zone);
			    else
			      printf("EEEK! should not be here - but no\n"
				     "spare zones exist to move this\n"
				     "window to!!! help!\n");
			 }
		    }
		  e_container_border_list_free(bl);
		  e_object_del(E_OBJECT(zone));
		  zones = evas_list_remove_list(zones, zones);
	       }
	  }
     }
   else
     {
	E_Zone *zone;

	zone = e_container_zone_number_get(con, 0);
	e_zone_move_resize(zone, 0, 0, con->w, con->h);
	e_shelf_zone_move_resize_handle(zone);	
     }

   ecore_event_add(E_EVENT_CONTAINER_RESIZE, ev, _e_container_event_container_resize_free, NULL);

   for (i = 0; i < 7; i++)
     {
	Evas_List *tmp = NULL;

	/* Make temporary list as e_border_res_change_geometry_restore
	 * rearranges the order. */
	for (l = con->layers[i].clients; l; l = l->next)
	     tmp = evas_list_append(tmp, l->data);

	for (l = tmp; l; l = l->next)
	  {
	     E_Border *bd;

	     bd = l->data;
	     e_border_res_change_geometry_save(bd);
	     e_border_res_change_geometry_restore(bd);
	  }

	tmp = evas_list_free(tmp);
     }
}

static void
_e_container_event_container_resize_free(void *data, void *ev)
{
   E_Event_Container_Resize *e;

   e = ev;
   e_object_unref(E_OBJECT(e->container));
   free(e);
}

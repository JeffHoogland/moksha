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

static Eina_Bool  _e_container_cb_mouse_in(void *data, int type, void *event);
static Eina_Bool  _e_container_cb_mouse_out(void *data, int type, void *event);
static Eina_Bool  _e_container_cb_mouse_down(void *data, int type, void *event);
static Eina_Bool  _e_container_cb_mouse_up(void *data, int type, void *event);
static Eina_Bool  _e_container_cb_mouse_move(void *data, int type, void *event);
static Eina_Bool  _e_container_cb_mouse_wheel(void *data, int type, void *event);

static void _e_container_shape_del(E_Container_Shape *es);
static void _e_container_shape_free(E_Container_Shape *es);
static void _e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch);
static void _e_container_resize_handle(E_Container *con);
static void _e_container_event_container_resize_free(void *data, void *ev);

EAPI int E_EVENT_CONTAINER_RESIZE = 0;
static Eina_List *handlers = NULL;

/* externally accessible functions */
EINTERN int
e_container_init(void)
{
   E_EVENT_CONTAINER_RESIZE = ecore_event_type_new();

   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, _e_container_cb_mouse_in, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, _e_container_cb_mouse_out, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_container_cb_mouse_down, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, _e_container_cb_mouse_up, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _e_container_cb_mouse_move, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, _e_container_cb_mouse_wheel, NULL));
   return 1;
}

EINTERN int
e_container_shutdown(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
   return 1;
}

EAPI E_Container *
e_container_new(E_Manager *man)
{
   E_Container *con;
   Evas_Object *o;
   char name[40];
   Eina_List *l, *screens;
   int i;
   Ecore_X_Window mwin;
   static int container_num = 0;

   con = E_OBJECT_ALLOC(E_Container, E_CONTAINER_TYPE, _e_container_free);
   if (!con) return NULL;
   con->manager = man;
   con->manager->containers = eina_list_append(con->manager->containers, con);
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

   if (!e_config->null_container_win)
      con->bg_ecore_evas = e_canvas_new(e_config->evas_engine_container, con->win,
                                        0, 0, con->w, con->h, 1, 1,
                                        &(con->bg_win));
   else
      con->bg_ecore_evas = e_canvas_new(e_config->evas_engine_container, con->win,
                                        0, 0, 1, 1, 1, 1,
                                        &(con->bg_win));
   e_canvas_add(con->bg_ecore_evas);
   con->event_win = ecore_x_window_input_new(con->win, 0, 0, con->w, con->h);
   ecore_x_window_show(con->event_win);
   con->bg_evas = ecore_evas_get(con->bg_ecore_evas);
   ecore_evas_name_class_set(con->bg_ecore_evas, "E", "Background_Window");
   ecore_evas_title_set(con->bg_ecore_evas, "Enlightenment Background");
   if (!getenv("EVAS_RENDER_MODE"))
     {
        int have_comp = 0;
        Eina_List *l;
        E_Config_Module *em;
        
        // FIXME: major hack. checking in advance for comp. eventully comp
        // will be rolled into e17 core and this won't be needed
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!strcmp(em->name, "comp"))
               {
                  have_comp = 1;
                  break;
               }
          }
        if (!have_comp)
          {
             if (getenv("REDRAW_DEBUG"))
               ecore_evas_avoid_damage_set(con->bg_ecore_evas, !atoi(getenv("REDRAW_DEBUG")));
             else
               ecore_evas_avoid_damage_set(con->bg_ecore_evas, ECORE_EVAS_AVOID_DAMAGE_BUILT_IN);
          }
     }
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
   con->name = eina_stringshare_add(name);

   /* create a scratch window for putting stuff into */
   con->scratch_win = ecore_x_window_override_new(con->win, 0, 0, 7, 7);
   
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

   screens = (Eina_List *)e_xinerama_screens_get();
   if (screens)
     {
	E_Screen *scr;
	EINA_LIST_FOREACH(screens, l, scr)
	  {
	     e_zone_new(con, scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
	  }
     }
   else
     e_zone_new(con, 0, 0, 0, 0, con->w, con->h);
   return con;
}

EAPI void
e_container_show(E_Container *con)
{
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);

   if (con->visible) return;
   if (!e_config->null_container_win)
      ecore_evas_show(con->bg_ecore_evas);
   ecore_x_window_configure(con->bg_win,
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
			    ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    con->layers[0].win, ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_configure(con->event_win,
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
   Eina_List *l;
   E_Container *con;
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);

   EINA_LIST_FOREACH(man->containers, l, con)
     {
        if (!con) continue;
	if (con->visible) return con;
     }

   /* If no one is available, return the first */
   if (!man->containers) return NULL;
   l = man->containers;
   return (E_Container *)eina_list_data_get(l);
}

EAPI E_Container *
e_container_number_get(E_Manager *man, int num)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   EINA_LIST_FOREACH(man->containers, l, con)
     {
	if ((int) con->num == num) return con;
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
   ecore_x_window_resize(con->event_win, con->w, con->h);
   if (!e_config->null_container_win)
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
   ecore_x_window_move_resize(con->event_win, con->x, con->y, con->w, con->h);
   if (!e_config->null_container_win)
      ecore_evas_resize(con->bg_ecore_evas, con->w, con->h);
   evas_object_move(con->bg_blank_object, con->x, con->y);
   evas_object_resize(con->bg_blank_object, con->w, con->h);
   _e_container_resize_handle(con);
}

EAPI void
e_container_raise(E_Container *con __UNUSED__)
{
//   E_OBJECT_CHECK(con);
//   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
}

EAPI void
e_container_lower(E_Container *con __UNUSED__)
{
//   E_OBJECT_CHECK(con);
//   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
}

EAPI E_Zone *
e_container_zone_at_point_get(E_Container *con, int x, int y)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   EINA_LIST_FOREACH(con->zones, l, zone)
     {
	if (E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
	  return zone;
     }
   return NULL;
}

EAPI E_Zone *
e_container_zone_number_get(E_Container *con, int num)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   EINA_LIST_FOREACH(con->zones, l, zone)
     {
	if ((int) zone->num == num) return zone;
     }
   return NULL;
}

EAPI E_Zone *
e_container_zone_id_get(E_Container *con, int id)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   EINA_LIST_FOREACH(con->zones, l, zone)
     {
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
   con->shapes = eina_list_append(con->shapes, es);
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

EAPI Eina_List *
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
   con->shape_change_cb = eina_list_append(con->shape_change_cb, cb);
}

EAPI void
e_container_shape_change_callback_del(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data)
{
   Eina_List *l = NULL;
   E_Container_Shape_Callback *cb = NULL;

   /* FIXME: if we call this from within a callback we are in trouble */
   E_OBJECT_CHECK(con);
   E_OBJECT_TYPE_CHECK(con, E_CONTAINER_TYPE);
   EINA_LIST_FOREACH(con->shape_change_cb, l, cb)
     {
	if ((cb->func == func) && (cb->data == data))
	  {
	     con->shape_change_cb = eina_list_remove_list(con->shape_change_cb, l);
	     free(cb);
	     return;
	  }
     }
}

EAPI Eina_List *
e_container_shape_rects_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(es, E_CONTAINER_SHAPE_TYPE, NULL);
   return es->shape;
}

EAPI void
e_container_shape_rects_set(E_Container_Shape *es, Ecore_X_Rectangle *rects, int num)
{
   int i;
   E_Rect *r;

   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_CONTAINER_SHAPE_TYPE);
   if (es->shape)
     {
	E_FREE_LIST(es->shape, free);
	es->shape = NULL;
     }
   if ((rects) && (num == 1) &&
       (rects[0].x == 0) &&
       (rects[0].y == 0) &&
       ((int) rects[0].width == es->w) &&
       ((int) rects[0].height == es->h))
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
		  es->shape = eina_list_append(es->shape, r);
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
      eina_list_append(bd->zone->container->layers[pos].clients, bd);
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
	   eina_list_remove(bd->zone->container->layers[i].clients, bd);
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
   Eina_List *l;
   int pos = 0, i;

   if (!bd->zone) return NULL;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   eina_list_remove(bd->zone->container->layers[i].clients, bd);
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
      eina_list_append(bd->zone->container->layers[pos].clients, bd);

   /* Find the window below this one */
   l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd);
   if (eina_list_prev(l))
     above = eina_list_data_get(eina_list_prev(l));
   else
     {
	/* Need to check the layers below */
	for (i = pos - 1; i >= 0; i--)
	  {
	     if ((bd->zone->container->layers[i].clients) &&
		 (l = eina_list_last(bd->zone->container->layers[i].clients)))
	       {
		  above = eina_list_data_get(l);
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
   Eina_List *l;
   int pos = 0, i;

   if (!bd->zone) return NULL;
   /* Remove from old layer */
   for (i = 0; i < 7; i++)
     {
	bd->zone->container->layers[i].clients =
	   eina_list_remove(bd->zone->container->layers[i].clients, bd);
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
      eina_list_prepend(bd->zone->container->layers[pos].clients, bd);

   /* Find the window above this one */
   l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd);
   if (eina_list_next(l))
     below = eina_list_data_get(eina_list_next(l));
   else
     {
	/* Need to check the layers above */
	for (i = pos + 1; i < 7; i++)
	  {
	     if (bd->zone->container->layers[i].clients)
	       {
		  below = eina_list_data_get(bd->zone->container->layers[i].clients);
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
	   eina_list_remove(bd->zone->container->layers[i].clients, bd);
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
      eina_list_append_relative(bd->zone->container->layers[pos].clients, bd, above);
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
	   eina_list_remove(bd->zone->container->layers[i].clients, bd);
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
      eina_list_prepend_relative(bd->zone->container->layers[pos].clients, bd, below);
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
     list->clients = eina_list_last(list->container->layers[list->layer].clients);
   while ((list->layer > 0) && (!list->clients))
     {
	list->layer--;
	if (list->container->layers[list->layer].clients)
	  list->clients = eina_list_last(list->container->layers[list->layer].clients);
     }
   return list;
}

EAPI E_Border *
e_container_border_list_next(E_Border_List *list)
{
   E_Border *bd;

   if (!list->clients) return NULL;

   bd = eina_list_data_get(list->clients);

   list->clients = eina_list_next(list->clients);
   while ((list->layer < 6) && (!list->clients))
     list->clients = list->container->layers[++list->layer].clients;
   return bd;
}

EAPI E_Border *
e_container_border_list_prev(E_Border_List *list)
{
   E_Border *bd;

   if (!list->clients) return NULL;

   bd = eina_list_data_get(list->clients);

   list->clients = eina_list_prev(list->clients);
   while ((list->layer > 0) && (!list->clients))
     {
	list->layer--;
	if (list->container->layers[list->layer].clients)
	  list->clients = eina_list_last(list->container->layers[list->layer].clients);
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
   Eina_List *l, *ll;
   E_Manager *man;
   E_Container *con;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	EINA_LIST_FOREACH(man->containers, ll, con)
	  {
	     evas_event_freeze(con->bg_evas);
	  }
     }
}

EAPI void
e_container_all_thaw(void)
{
   Eina_List *l, *ll;
   E_Manager *man;
   E_Container *con;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	EINA_LIST_FOREACH(man->containers, ll, con)
	  {
	     evas_event_thaw(con->bg_evas);
	  }
     }
}

/* local subsystem functions */
static void
_e_container_free(E_Container *con)
{
   Eina_List *l;
   int i;

   ecore_x_window_free(con->scratch_win);
   ecore_x_window_free(con->event_win);
   /* We can't use e_object_del here, because border adds a ref to itself
    * when it is removed, and the ref is never unref'ed */
   for (i = 0; i < 7; i++)
     {
        ecore_x_window_free(con->layers[i].win);
/* FIXME: had to disable this as it was freeing already freed items during
 * looping (particularly remember/lock config dialogs). this is just
 * disabled until we put in some special handling for this
 *
	EINA_LiST_FOREACH(con->layers[i].clients, l, tmp)
	  {
	     e_object_free(E_OBJECT(tmp));
	  }
 */   
     }
   l = con->zones;
   con->zones = NULL;
   E_FREE_LIST(l, e_object_del);
   con->manager->containers = eina_list_remove(con->manager->containers, con);
   e_canvas_del(con->bg_ecore_evas);
   ecore_evas_free(con->bg_ecore_evas);
   if (con->manager->win != con->win)
     {
	ecore_x_window_free(con->win);
     }
   if (con->name) eina_stringshare_del(con->name);
   free(con);
}

static E_Container *
_e_container_find_by_event_window(Ecore_X_Window win)
{
   Eina_List *l, *ll;
   E_Manager *man;
   E_Container *con;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
	EINA_LIST_FOREACH(man->containers, ll, con)
	  {
	     if (con->event_win == win) return con;
	  }
     }
   return NULL;
}

static Eina_Bool
_e_container_cb_mouse_in(void *data __UNUSED__, int type __UNUSED__, void *event)
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
	ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
	evas_event_feed_mouse_in(con->bg_evas, ev->time, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_container_cb_mouse_out(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_win);
   if (con)
     {
	ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
	if (ev->mode == ECORE_X_EVENT_MODE_GRAB)
	  evas_event_feed_mouse_cancel(con->bg_evas, ev->time, NULL);
        evas_event_feed_mouse_out(con->bg_evas, ev->time, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_container_cb_mouse_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_window);
   if (con)
     {
        Evas_Button_Flags flags = EVAS_BUTTON_NONE;

	e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_CONTAINER,
					   E_OBJECT(con), ev);
	if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
	if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
	ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
	evas_event_feed_mouse_down(con->bg_evas, ev->buttons, flags, ev->timestamp, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_container_cb_mouse_up(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_window);
   if (con)
     {
        evas_event_feed_mouse_up(con->bg_evas, ev->buttons, EVAS_BUTTON_NONE, ev->timestamp, NULL);
	ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
	e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_CONTAINER,
					 E_OBJECT(con), ev);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_container_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_window);
   if (con)
     {
	ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
        evas_event_feed_mouse_move(con->bg_evas, ev->x, ev->y, ev->timestamp, NULL);
     }
   return 1;
}

static Eina_Bool
_e_container_cb_mouse_wheel(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Wheel *ev;
   E_Container *con;

   ev = event;
   con = _e_container_find_by_event_window(ev->event_window);
   if (con)
     {
	if (!e_bindings_wheel_event_handle(E_BINDING_CONTEXT_CONTAINER,
					   E_OBJECT(con), ev))
	  {
	     ecore_event_evas_modifier_lock_update(con->bg_evas, ev->modifiers);
	     evas_event_feed_mouse_wheel(con->bg_evas, ev->direction, ev->z, ev->timestamp, NULL);
	  }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_container_shape_del(E_Container_Shape *es)
{
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_DEL);
}

static void
_e_container_shape_free(E_Container_Shape *es)
{
   es->con->shapes = eina_list_remove(es->con->shapes, es);
   E_FREE_LIST(es->shape, free);
   free(es);
}

static void
_e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch)
{
   Eina_List *l = NULL;
   E_Container_Shape_Callback *cb = NULL;

   if ((!es) || (!es->con) || (!es->con->shape_change_cb)) return;
   EINA_LIST_FOREACH(es->con->shape_change_cb, l, cb)
     {
	if (!cb) continue;
	cb->func(cb->data, es, ch);
     }
}

static void
_e_container_resize_handle(E_Container *con)
{
   E_Event_Container_Resize *ev;
   Eina_List *l, *screens, *zones = NULL;
   E_Zone *zone;
   E_Screen *scr;
   int i;

   ev = calloc(1, sizeof(E_Event_Container_Resize));
   ev->container = con;
   e_object_ref(E_OBJECT(con));

   e_xinerama_update();
   screens = (Eina_List *)e_xinerama_screens_get();

   if (screens)
     {
	EINA_LIST_FOREACH(con->zones, l, zone)
	  zones = eina_list_append(zones, zone);
	EINA_LIST_FOREACH(screens, l, scr)
	  {
	     zone = e_container_zone_id_get(con, scr->escreen);
	     if (zone)
	       {
		  e_zone_move_resize(zone, scr->x, scr->y, scr->w, scr->h);
		  e_shelf_zone_move_resize_handle(zone);	
		  zones = eina_list_remove(zones, zone);
	       }
	     else
	       {
		  Eina_List *ll;
  		  E_Config_Shelf *cf_es;

		  zone = e_zone_new(con, scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
		  /* find any shelves configured for this zone and add them in */
		  EINA_LIST_FOREACH(e_config->shelves, ll, cf_es)
		    {
		       if (e_util_container_zone_id_get(cf_es->container, cf_es->zone) == zone)
			 e_shelf_config_new(zone, cf_es);
		    }
	       }
	  }
	if (zones)
	  {
	     E_Zone *spare_zone = NULL;
	     Eina_List *ll;

	     EINA_LIST_FOREACH(con->zones, ll, spare_zone)
	       {
		  if (eina_list_data_find(zones, spare_zone))
		    spare_zone = NULL;
		  else break;
	       }
	     EINA_LIST_FREE(zones, zone)
	       {
		  Eina_List *shelves, *ll, *del_shelves;
		  E_Shelf *es;
		  E_Border_List *bl;
		  E_Border *bd;

		  /* delete any shelves on this zone */
		  shelves = e_shelf_list();
		  del_shelves = NULL;
		  EINA_LIST_FOREACH(shelves, ll, es)
		    {
		       if (es->zone == zone)
			 del_shelves = eina_list_append(del_shelves, es);
		    }
		  E_FREE_LIST(del_shelves, e_object_del);
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
	Eina_List *tmp = NULL;
	E_Border *bd;

	/* Make temporary list as e_border_res_change_geometry_restore
	 * rearranges the order. */
	EINA_LIST_FOREACH(con->layers[i].clients, l, bd)
	     tmp = eina_list_append(tmp, bd);

	EINA_LIST_FOREACH(tmp, l, bd)
	  {
	     e_border_res_change_geometry_save(bd);
	     e_border_res_change_geometry_restore(bd);
	  }

	eina_list_free(tmp);
     }
}

static void
_e_container_event_container_resize_free(void *data __UNUSED__, void *ev)
{
   E_Event_Container_Resize *e;

   e = ev;
   e_object_unref(E_OBJECT(e->container));
   free(e);
}

#include "e.h"

/* TODO List:
 * 
 * * fix shape callbacks to be able to be safely deleted
 */

/* local subsystem functions */
static void _e_container_free(E_Container *con);

static void _e_container_cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_container_cb_bg_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_container_cb_bg_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_container_cb_bg_ecore_evas_resize(Ecore_Evas *ee);

static void _e_container_shape_del(E_Container_Shape *es);
static void _e_container_shape_free(E_Container_Shape *es);
static void _e_container_shape_change_call(E_Container_Shape *es, E_Container_Shape_Change ch);

/* externally accessible functions */
int
e_container_init(void)
{
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
   Ecore_Event_Handler *h;
   
   con = E_OBJECT_ALLOC(E_Container, _e_container_free);
   if (!con) return NULL;
   con->manager = man;
   e_object_ref(E_OBJECT(con->manager));
   con->manager->containers = evas_list_append(con->manager->containers, con);
   con->w = con->manager->w;
   con->h = con->manager->h;
   con->win = ecore_x_window_override_new(con->manager->win, con->x, con->y, con->w, con->h);
   ecore_x_icccm_title_set(con->win, "Enlightenment Container");
   con->bg_ecore_evas = ecore_evas_software_x11_new(NULL, con->win, 0, 0, con->w, con->h);
   e_canvas_add(con->bg_ecore_evas);
   con->bg_evas = ecore_evas_get(con->bg_ecore_evas);
   con->bg_win = ecore_evas_software_x11_window_get(con->bg_ecore_evas);
   ecore_evas_name_class_set(con->bg_ecore_evas, "E", "Background_Window");
   ecore_evas_title_set(con->bg_ecore_evas, "Enlightenment Background");
   ecore_evas_avoid_damage_set(con->bg_ecore_evas, 1);
   ecore_evas_show(con->bg_ecore_evas);
   e_path_evas_append(path_fonts, con->bg_evas);
   
   e_pointer_container_set(con);
   
   if (1) /* for now ALWAYS on - but later maybe a config option */
     {
	Evas_Object *o;

	o = evas_object_rectangle_add(con->bg_evas);
	con->bg_blank_object = 0;
	evas_object_layer_set(o, -100);
	evas_object_move(o, 0, 0);
	evas_object_resize(o, con->w, con->h);
	evas_object_color_set(o, 255, 255, 255, 255);
	evas_object_show(o);
	
	o = edje_object_add(con->bg_evas);
	con->bg_object = o;
	evas_object_layer_set(o, -1);
	evas_object_name_set(o, "desktop/background");
	evas_object_data_set(o, "e_container", con);
	evas_object_move(o, 0, 0);
	evas_object_resize(o, con->w, con->h);
	edje_object_file_set(o,
			     e_config->desktop_default_background,
			     "desktop/background");
	evas_object_show(o);
	
	o = evas_object_rectangle_add(con->bg_evas);
	con->bg_event_object = 0;
	evas_object_move(o, 0, 0);
	evas_object_resize(o, con->w, con->h);
	evas_object_color_set(o, 255, 255, 255, 0);
	evas_object_show(o);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_container_cb_bg_mouse_down, con);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,   _e_container_cb_bg_mouse_up,   con);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_container_cb_bg_mouse_move, con);
	
	ecore_evas_callback_resize_set(con->bg_ecore_evas, _e_container_cb_bg_ecore_evas_resize);
     }
   return con;
}
        
void
e_container_show(E_Container *con)
{
   E_OBJECT_CHECK(con);
   if (con->visible) return;
   ecore_x_window_show(con->win);
   con->visible = 1;
}
        
void
e_container_hide(E_Container *con)
{
   E_OBJECT_CHECK(con);
   if (!con->visible) return;
   ecore_x_window_hide(con->win);
   con->visible = 0;
}

void
e_container_move(E_Container *con, int x, int y)
{
   E_OBJECT_CHECK(con);
   if ((x == con->x) && (y == con->y)) return;
   con->x = x;
   con->y = y;
   ecore_x_window_move(con->win, con->x, con->y);
}
        
void
e_container_resize(E_Container *con, int w, int h)
{
   E_OBJECT_CHECK(con);
   if ((w == con->w) && (h == con->h)) return;
   con->w = w;
   con->h = h;
   ecore_x_window_resize(con->win, con->w, con->h);
   ecore_evas_resize(con->bg_ecore_evas, con->w, con->h);
}  

void
e_container_move_resize(E_Container *con, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(con);
   if ((x == con->x) && (y == con->y) && (w == con->w) && (h == con->h)) return;
   con->x = x;
   con->y = y;
   con->w = w;
   con->h = h;
   ecore_x_window_move_resize(con->win, con->x, con->y, con->w, con->h);
   ecore_evas_resize(con->bg_ecore_evas, con->w, con->h);
}

void
e_container_raise(E_Container *con)
{
   E_OBJECT_CHECK(con);
   ecore_x_window_raise(con->win);
}

void
e_container_lower(E_Container *con)
{
   E_OBJECT_CHECK(con);
   ecore_x_window_lower(con->win);
}

void
e_container_bg_reconfigure(E_Container *con)
{
   Evas_Object *o;
   
   E_OBJECT_CHECK(con);
   o = con->bg_object;
   evas_object_hide(o);
   edje_object_file_set(o,
			e_config->desktop_default_background,
			"desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_show(o);
}


Evas_List *
e_container_clients_list_get(E_Container *con)
{
    E_OBJECT_CHECK(con);
    return con->clients;
}
   

E_Container_Shape *
e_container_shape_add(E_Container *con)
{
   E_Container_Shape *es;
   
   E_OBJECT_CHECK_RETURN(con, NULL);
   
   es = E_OBJECT_ALLOC(E_Container_Shape, _e_container_shape_free);
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
   if (es->visible) return;
   es->visible = 1;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_SHOW);
}

void
e_container_shape_hide(E_Container_Shape *es)
{
   E_OBJECT_CHECK(es);
   if (!es->visible) return;
   es->visible = 0;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_HIDE);
}

void
e_container_shape_move(E_Container_Shape *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   if ((es->x == x) && (es->y == y)) return;
   es->x = x;
   es->y = y;
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_MOVE);
}

void
e_container_shape_resize(E_Container_Shape *es, int w, int h)
{
   E_OBJECT_CHECK(es);
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
   return con->shapes;
}

void
e_container_shape_geometry_get(E_Container_Shape *es, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK(es);
   if (x) *x = es->x;
   if (y) *y = es->y;
   if (w) *w = es->w;
   if (h) *h = es->h;
}

E_Container *
e_container_shape_container_get(E_Container_Shape *es)
{
   E_OBJECT_CHECK_RETURN(es, NULL);
   return es->con;
}

void
e_container_shape_change_callback_add(E_Container *con, void (*func) (void *data, E_Container_Shape *es, E_Container_Shape_Change ch), void *data)
{
   E_Container_Shape_Callback *cb;
   
   E_OBJECT_CHECK(con);
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
   return es->shape;
}





/* local subsystem functions */
static void
_e_container_free(E_Container *con)
{
   while (con->clients) e_object_del(E_OBJECT(con->clients->data));
   con->manager->containers = evas_list_remove(con->manager->containers, con);
   e_canvas_del(con->bg_ecore_evas);
   ecore_evas_free(con->bg_ecore_evas);
   ecore_x_window_del(con->win);
   e_object_unref(E_OBJECT(con->manager));
   free(con);
}
   
static void
_e_container_cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Container *con;
   Evas_Event_Mouse_Down *ev;
   
   ev = (Evas_Event_Mouse_Down *)event_info;
   con = data;
   if (ev->button == 1)
     {
	char buf[4096];
	char *homedir;
	
	homedir = e_user_homedir_get();
	if (homedir)
	  {
	     E_Menu *m;
	     
	     snprintf(buf, sizeof(buf), "%s/.e/e/applications/favorite", homedir);
	     m = e_int_menus_apps_new(buf, 1);
	     e_menu_activate_mouse(m, con, ev->output.x, ev->output.y, 1, 1,
				   E_MENU_POP_DIRECTION_DOWN);
	     e_util_container_fake_mouse_up_all_later(con);
	     free(homedir);
	  }
     }
   else if (ev->button == 2)
     {
	static E_Menu *m = NULL;
	static E_Menu *m1 = NULL;
	static E_Menu *m2 = NULL;
	
	if (!m)
	  {
	     E_Menu_Item *mi;
	     
	     m1 = e_menu_new();
	     mi = e_menu_item_new(m1);
	     e_menu_item_label_set(mi, "Submenu 1 Item 1");
	     mi = e_menu_item_new(m1);
	     e_menu_item_label_set(mi, "Submenu 1 Item 2");
	     mi = e_menu_item_new(m1);
	     e_menu_item_label_set(mi, "Submenu 1 Item 3");
	     
	     m2 = e_menu_new();
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "Flimstix");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "Shub Shub");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "Gah I thought I'd just make this long");
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "And more");
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "Getting stenchy");
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "Ich bin ein Fisch");
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "PONG");
	     mi = e_menu_item_new(m2);
	     e_menu_item_label_set(mi, "The last word");

	     m = e_menu_new();
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "First Menu Item");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "Short");
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "A very long menu item is here to test with");
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "There is no spoon!");
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "Icon: Pants On.");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "First Submenu");
	     e_menu_item_submenu_set(mi, m1);
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "Other side of a separator");
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, "A Submenu");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     e_menu_item_submenu_set(mi, m2);
	     mi = e_menu_item_new(m);
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_label_set(mi, "Check 1");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m);
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_label_set(mi, "Check 2");
	     mi = e_menu_item_new(m);
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_label_set(mi, "Check 3");
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     e_menu_item_label_set(mi, "Radio 1 Group 1");
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     e_menu_item_label_set(mi, "Radio 2 Group 1");
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     e_menu_item_label_set(mi, "Radio 3 Group 1");
	     e_menu_item_icon_file_set(mi, 
				       e_path_find(path_images, "e.png"));
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 2);
	     e_menu_item_label_set(mi, "Radio 1 Group 2");
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 2);
	     e_menu_item_label_set(mi, "Radio 2 Group 2");
	  }
	e_menu_activate_mouse(m, con, ev->output.x, ev->output.y, 1, 1, 
			      E_MENU_POP_DIRECTION_DOWN);
	/* fake the up event as we will now grab the mouse to the menu */
	e_util_container_fake_mouse_up_all_later(con);
     }
   else if (ev->button == 3)
     {
	E_Menu *m;
	
	m = e_int_menus_about_new();
	e_menu_activate_mouse(m, con, ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(con);
     }
}

static void
_e_container_cb_bg_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Container *con;
   Evas_Event_Mouse_Up *ev;
   
   ev = (Evas_Event_Mouse_Up *)event_info;      
   con = data;
}

static void
_e_container_cb_bg_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Container *con;
   Evas_Event_Mouse_Move *ev;
   
   ev = (Evas_Event_Mouse_Move *)event_info;   
   con = data;
/*   printf("move %i %i\n", ev->cur.output.x, ev->cur.output.y); */
}

static void
_e_container_cb_bg_ecore_evas_resize(Ecore_Evas *ee)
{
   Evas *evas;
   Evas_Object *o;
   E_Container *con;
   Evas_Coord w, h;
   
   evas = ecore_evas_get(ee);
   evas_output_viewport_get(evas, NULL, NULL, &w, &h);
   o = evas_object_name_find(evas, "desktop/background");
   con = evas_object_data_get(o, "e_container");
   evas_object_resize(con->bg_object, w, h);
   evas_object_resize(con->bg_event_object, w, h);
}

static void
_e_container_shape_del(E_Container_Shape *es)
{
   _e_container_shape_change_call(es, E_CONTAINER_SHAPE_DEL);
}

static void
_e_container_shape_free(E_Container_Shape *es)
{
   es->con->shapes = evas_list_remove(es->con->shapes, es);
   while (es->shape)
     {
	E_Rect *r;
	
	r = es->shape->data;
	es->shape = evas_list_remove_list(es->shape, es->shape);
	free(r);
     }
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

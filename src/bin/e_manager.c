/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_manager_free(E_Manager *man);

static int _e_manager_cb_window_show_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_configure(void *data, int ev_type, void *ev);
static int _e_manager_cb_key_up(void *data, int ev_type, void *ev);
static int _e_manager_cb_key_down(void *data, int ev_type, void *ev);
static int _e_manager_cb_frame_extents_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_ping(void *data, int ev_type, void *ev);
static int _e_manager_cb_screensaver_notify(void *data, int ev_type, void *ev);
static int _e_manager_cb_client_message(void *data, int ev_type, void *ev);

static Evas_Bool _e_manager_frame_extents_free_cb(const Evas_Hash *hash __UNUSED__,
						  const char *key __UNUSED__,
						  void *data, void *fdata __UNUSED__);
static E_Manager *_e_manager_get_for_root(Ecore_X_Window root);
#if 0 /* use later - maybe */
static int _e_manager_cb_window_destroy(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_hide(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_reparent(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_create(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_configure_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_gravity(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_stack(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_stack_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_property(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_colormap(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_shape(void *data, int ev_type, void *ev);
static int _e_manager_cb_client_message(void *data, int ev_type, void *ev);
#endif

/* local subsystem globals */

typedef struct _Frame_Extents Frame_Extents;

struct _Frame_Extents
{
   int l, r, t, b;
};

static Evas_List *managers = NULL;
static Evas_Hash *frame_extents = NULL;
    
/* externally accessible functions */
EAPI int
e_manager_init(void)
{
   ecore_x_screensaver_event_listen_set(1);
   return 1;
}

EAPI int
e_manager_shutdown(void)
{
   Evas_List *l;

   l = managers;
   managers = NULL;
   while (l)
     {
	e_object_del(E_OBJECT(l->data));
	l = evas_list_remove_list(l, l);
     }
   if (frame_extents)
     {
	evas_hash_foreach(frame_extents, _e_manager_frame_extents_free_cb, NULL);
	evas_hash_free(frame_extents);
	frame_extents = NULL;
     }
   return 1;
}

EAPI Evas_List *
e_manager_list(void)
{
   return managers;
}

EAPI E_Manager *
e_manager_new(Ecore_X_Window root, int num)
{
   E_Manager *man;
   Ecore_Event_Handler *h;

   if (!ecore_x_window_manage(root)) return NULL;
   man = E_OBJECT_ALLOC(E_Manager, E_MANAGER_TYPE, _e_manager_free);
   if (!man) return NULL;
   managers = evas_list_append(managers, man);
   man->root = root;
   man->num = num;
   ecore_x_window_size_get(man->root, &(man->w), &(man->h));
   if (e_config->use_virtual_roots)
     {
	Ecore_X_Window mwin;
	
	man->win = ecore_x_window_override_new(man->root, man->x, man->y, man->w, man->h);
	ecore_x_icccm_title_set(man->win, "Enlightenment Manager");
	ecore_x_netwm_name_set(man->win, "Enlightenment Manager");
	mwin = e_menu_grab_window_get();
	ecore_x_window_raise(man->win);
     }
   else
     {
	man->win = man->root;
     }

   /* FIXME: this handles 1 screen only - not multihead. multihead randr
    * and xinerama are complex oin terms of interaction, so for now only
    * really have this work in single head. the randr module kept this
    * as a list, and i might move it to be the same too, but for now, keep
    * it as is
    */
   if (e_config->display_res_restore)
     {
        Ecore_X_Screen_Size size;
	Ecore_X_Screen_Refresh_Rate rate;
	
	size.width = e_config->display_res_width;
	size.height = e_config->display_res_height;
	rate.rate = e_config->display_res_hz;
	ecore_x_randr_screen_refresh_rate_set(man->root, size, rate);
	if (e_config->display_res_rotation)
	  ecore_x_randr_screen_rotation_set(man->root,
					    e_config->display_res_rotation);
     }
   
   h = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST, _e_manager_cb_window_show_request, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_manager_cb_window_configure, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN, _e_manager_cb_key_down, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_KEY_UP, _e_manager_cb_key_up, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_FRAME_EXTENTS_REQUEST, _e_manager_cb_frame_extents_request, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_PING, _e_manager_cb_ping, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_SCREENSAVER_NOTIFY, _e_manager_cb_screensaver_notify, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);
   h = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _e_manager_cb_client_message, man);
   if (h) man->handlers = evas_list_append(man->handlers, h);

   man->pointer = e_pointer_window_new(man->root, 1);

   return man;
}

EAPI void
e_manager_manage_windows(E_Manager *man)
{
   Ecore_X_Window *windows;
   int wnum;

   /* a manager is designated for each root. lets get all the windows in 
      the managers root */
   windows = ecore_x_window_children_get(man->root, &wnum);
   if (windows)
     {
	int i;
	const char *atom_names[] =
	  {
	     "_XEMBED_INFO",
	       "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
	       "KWM_DOCKWINDOW"
	  };
	Ecore_X_Atom atoms[3];
        Ecore_X_Atom atom_xmbed, atom_kde_netwm_systray, atom_kwm_dockwindow,
	  atom_window;
	unsigned char *data = NULL;
	int count;
	
	atom_window = ECORE_X_ATOM_WINDOW;
	ecore_x_atoms_get(atom_names, 3, atoms);
	atom_xmbed = atoms[0];
	atom_kde_netwm_systray = atoms[1];
	atom_kwm_dockwindow = atoms[2];
	for (i = 0; i < wnum; i++)
	  {
	     Ecore_X_Window_Attributes att;
	     unsigned int ret_val, deskxy[2];
	     int ret;

	     ecore_x_window_attributes_get(windows[i], &att);
	     if ((att.override) || (att.input_only))
	       {
		  if (att.override)
		    {
		       char *wname = NULL, *wclass = NULL;
		       
		       ecore_x_icccm_name_class_get(windows[i], 
						    &wname, &wclass);
		       if ((wname) && (wclass) &&
			   (!strcmp(wname, "E")) &&
			   (!strcmp(wclass, "Init_Window")))
			 {
			    free(wname);
			    free(wclass);
			    man->initwin = windows[i];
			 }
		       else
			 {
			    if (wname) free(wname);
			    if (wclass) free(wclass);
			    continue;
			 }
		    }
		  else
		    continue;
	       }
	     if (!ecore_x_window_prop_property_get(windows[i],
						   atom_xmbed,
						   atom_xmbed, 32,
						   &data, &count))
	       data = NULL;
	     if (!data)
	       {
		  if (!ecore_x_window_prop_property_get(windows[i],
							atom_kde_netwm_systray,
							atom_xmbed, 32,
							&data, &count))
		    data = NULL;
	       }
	     if (!data)
	       {
		  if (!ecore_x_window_prop_property_get(windows[i],
							atom_kwm_dockwindow,
							atom_kwm_dockwindow, 32,
							&data, &count))
		    data = NULL;
	       }
	     if (data)
	       {
		  free(data);
		  data = NULL;
		  continue;
	       }
	     ret = ecore_x_window_prop_card32_get(windows[i],
						  E_ATOM_MANAGED,
						  &ret_val, 1);

	     /* we have seen this window before */
	     if ((ret > -1) && (ret_val == 1))
	       {
		  E_Container  *con = NULL;
		  E_Zone       *zone = NULL;
		  E_Desk       *desk = NULL;
		  E_Border     *bd = NULL;
		  unsigned int  id;

		  /* get all information from window before it is 
		   * reset by e_border_new */
		  ret = ecore_x_window_prop_card32_get(windows[i],
						       E_ATOM_CONTAINER,
						       &id, 1);
		  if (ret == 1)
		    con = e_container_number_get(man, id);
		  if (!con)
		    con = e_container_current_get(man);
		  
		  ret = ecore_x_window_prop_card32_get(windows[i],
						       E_ATOM_ZONE,
						       &id, 1);
		  if (ret == 1)
		    zone = e_container_zone_number_get(con, id);
		  if (!zone)
		    zone = e_zone_current_get(con);
		  ret = ecore_x_window_prop_card32_get(windows[i],
						       E_ATOM_DESK,
						       deskxy, 2);
		  if (ret == 2)
		    desk = e_desk_at_xy_get(zone,
					    deskxy[0],
					    deskxy[1]);

		    {
		       bd = e_border_new(con, windows[i], 1, 0);
		       if (bd)
			 {
			    /* FIXME:
			     * It's enough to set the desk, the zone will
			     * be set according to the desk */
			    if (zone) e_border_zone_set(bd, zone);
			    if (desk) e_border_desk_set(bd, desk);
			 }
		    }
	       }
	     else if ((att.visible) && (!att.override) &&
		      (!att.input_only))
	       {
		  /* We have not seen this window, and X tells us it
		   * should be seen */
		  E_Container *con;
		  E_Border *bd;
		  
		  con = e_container_current_get(man);
		  bd = e_border_new(con, windows[i], 1, 0);
		  if (bd) e_border_show(bd);
	       }
	  }
	free(windows);
     }
}

EAPI void
e_manager_show(E_Manager *man)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->visible) return;
   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;
	
	con = l->data;
	e_container_show(con);
     }
   if (man->root != man->win)
     {
	Ecore_X_Window mwin;
	
	mwin = e_menu_grab_window_get();
	if (!mwin) mwin = man->initwin;
	if (!mwin)
	  ecore_x_window_raise(man->win);
	else
	  ecore_x_window_configure(man->win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   mwin, ECORE_X_WINDOW_STACK_BELOW);
	ecore_x_window_show(man->win);
     }
   man->visible = 1;
}

EAPI void
e_manager_hide(E_Manager *man)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->visible) return;
   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;
	
	con = l->data;
	e_container_hide(con);
     }
   if (man->root != man->win)
     ecore_x_window_hide(man->win);
   man->visible = 0; 
}

EAPI void
e_manager_move(E_Manager *man, int x, int y)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if ((x == man->x) && (y == man->y)) return;
   if (man->root != man->win)
     {
	man->x = x;
	man->y = y;
	ecore_x_window_move(man->win, man->x, man->y);
     }
}

EAPI void
e_manager_resize(E_Manager *man, int w, int h)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if ((w == man->w) && (h == man->h)) return;
   man->w = w;
   man->h = h;
   if (man->root != man->win)
     ecore_x_window_resize(man->win, man->w, man->h);
	
   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;
	
	con = l->data;
	e_container_resize(con, man->w, man->h);
     }

   ecore_x_netwm_desk_size_set(man->root, man->w, man->h);
}

EAPI void
e_manager_move_resize(E_Manager *man, int x, int y, int w, int h)
{
   Evas_List *l;
   
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if ((x == man->x) && (y == man->y) && (w == man->w) && (h == man->h)) return;
   if (man->root != man->win)
     {
	man->x = x;
	man->y = y;
     }
   man->w = w;
   man->h = h;
   ecore_x_window_move_resize(man->win, man->x, man->y, man->w, man->h);

   for (l = man->containers; l; l = l->next)
     {
	E_Container *con;
	
	con = l->data;
	e_container_resize(con, man->w, man->h);
     }
}

EAPI void
e_manager_raise(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     {
	Ecore_X_Window mwin;
	
	mwin = e_menu_grab_window_get();
	if (!mwin) mwin = man->initwin;
	if (!mwin)
	  ecore_x_window_raise(man->win);
	else
	  ecore_x_window_configure(man->win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   mwin, ECORE_X_WINDOW_STACK_BELOW);
     }
}

EAPI void
e_manager_lower(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     ecore_x_window_lower(man->win);
}

EAPI E_Manager *
e_manager_current_get(void)
{
   Evas_List *l;
   E_Manager *man;
   int x, y;
   
   if (!managers) return NULL;
   for (l = managers; l; l = l->next)
     {
	man = l->data;
	ecore_x_pointer_xy_get(man->win, &x, &y);
	if (x == -1 && y == -1)
	  continue;
	if (E_INSIDE(x, y, man->x, man->y, man->w, man->h))
	  return man;
     }
   return managers->data;
}

EAPI E_Manager *
e_manager_number_get(int num)
{
   Evas_List *l;
   E_Manager *man;
   
   if (!managers) return NULL;
   for (l = managers; l; l = l->next)
     {
	man = l->data;
	if (man->num == num)
	  return man;
     }
   return NULL;
}

EAPI void
e_managers_keys_grab(void)
{
   Evas_List *l;

   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	e_bindings_key_grab(E_BINDING_CONTEXT_ANY, man->root);
     }
}

EAPI void
e_managers_keys_ungrab(void)
{
   Evas_List *l;
   
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	e_bindings_key_ungrab(E_BINDING_CONTEXT_ANY, man->root);
     }
}

/* local subsystem functions */
static void
_e_manager_free(E_Manager *man)
{
   Evas_List *l;

   while (man->handlers)
     {
	Ecore_Event_Handler *h;
   
	h = man->handlers->data;
	man->handlers = evas_list_remove_list(man->handlers, man->handlers);
	ecore_event_handler_del(h);
     }
   l = man->containers;
   man->containers = NULL;
   while (l)
     {
	e_object_del(E_OBJECT(l->data));
	l = evas_list_remove_list(l, l);
     }
   if (man->root != man->win)
     {
	ecore_x_window_del(man->win);
     }
   if (man->pointer) e_object_del(E_OBJECT(man->pointer));
   managers = evas_list_remove(managers, man);   
   free(man);
}

static int
_e_manager_cb_window_show_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Show_Request *e;
   
   man = data;
   e = ev;
   if (e_stolen_win_get(e->win)) return 1;
   if (ecore_x_window_parent_get(e->win) != man->root)
     return 1;  /* try other handlers for this */
   
     {
	E_Container *con;
	E_Border *bd;
	
	con = e_container_current_get(man);
	if (!e_border_find_by_client_window(e->win))
	  {
	     bd = e_border_new(con, e->win, 0, 0);
	     if (!bd)
	       ecore_x_window_show(e->win);
	  }
     }
   return 1;
}

static int
_e_manager_cb_window_configure(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Configure *e;
   
   man = data;
   e = ev;
   if (e->win != man->root) return 1;
   e_manager_resize(man, e->w, e->h);
   return 1;
}

static int
_e_manager_cb_key_down(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Key_Down *e;
   
   man = data;
   e = ev;

   if (e->event_win != man->root) return 1;
   if (e->root_win != man->root) man = _e_manager_get_for_root(e->root_win);
   if (e_bindings_key_down_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), ev))
     return 0;
   return 1;
}

static int
_e_manager_cb_key_up(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Key_Up *e;
   
   man = data;
   e = ev;

   if (e->event_win != man->root) return 1;
   if (e->root_win != man->root) man = _e_manager_get_for_root(e->root_win);
   if (e_bindings_key_up_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), ev))
     return 0;
   return 1;
}

static int
_e_manager_cb_frame_extents_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Container *con;
   Ecore_X_Event_Frame_Extents_Request *e;
   Ecore_X_Window_Type type;
   Ecore_X_MWM_Hint_Decor decor;
   Ecore_X_Window_State *state;
   Frame_Extents *extents;
   const char *border, *signal, *key;
   int ok;
   unsigned int i, num;
   
   man = data;
   con = e_container_current_get(man);
   e = ev;

   if (ecore_x_window_parent_get(e->win) != man->root) return 1;

   /* TODO:
    * * We need to check if we remember this window, and border locking is set
    */
   border = "default";
   key = border;
   ok = ecore_x_mwm_hints_get(e->win, NULL, &decor, NULL);
   if ((ok) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_ALL)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_TITLE)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_BORDER)))
     {
	border = "borderless";
	key = border;
     }

   ok = ecore_x_netwm_window_type_get(e->win, &type);
   if ((ok) &&
       ((type == ECORE_X_WINDOW_TYPE_DESKTOP) || 
	(type == ECORE_X_WINDOW_TYPE_DOCK)))
     {
	border = "borderless";
	key = border;
     }

   signal = NULL;
   ecore_x_netwm_window_state_get(e->win, &state, &num);
   if (state)
     {
	int maximized = 0;
	int fullscreen = 0;

	for (i = 0; i < num; i++)
	  {
	     switch (state[i])
	       {
		case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
		  maximized++;
		  break;
		case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
		  maximized++;
		  break;
		case ECORE_X_WINDOW_STATE_FULLSCREEN:
		  fullscreen = 1;
		  border = "borderless";
		  key = border;
		  break;
		case ECORE_X_WINDOW_STATE_SHADED:
		case ECORE_X_WINDOW_STATE_SKIP_TASKBAR:
		case ECORE_X_WINDOW_STATE_SKIP_PAGER:
		case ECORE_X_WINDOW_STATE_HIDDEN:
		case ECORE_X_WINDOW_STATE_ICONIFIED:
		case ECORE_X_WINDOW_STATE_MODAL:
		case ECORE_X_WINDOW_STATE_STICKY:
		case ECORE_X_WINDOW_STATE_ABOVE:
		case ECORE_X_WINDOW_STATE_BELOW:
		case ECORE_X_WINDOW_STATE_DEMANDS_ATTENTION:
		case ECORE_X_WINDOW_STATE_UNKNOWN:
		  break;
	       }
	  }
	if ((maximized == 2) &&
	    (e_config->maximize_policy == E_MAXIMIZE_FULLSCREEN))
	  {
	     signal = "e,action,maximize,fullscreen";
	     key = "maximize,fullscreen";
	  }
	free(state);
     }

   extents = evas_hash_find(frame_extents, key);
   if (!extents)
     {
	extents = E_NEW(Frame_Extents, 1);
	if (extents)
	  {
	     Evas_Object *o;
	     char buf[1024];

	     o = edje_object_add(con->bg_evas);
	     snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", border);
	     ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
	     if (ok)
	       {
		  Evas_Coord x, y, w, h;

		  if (signal)
		    {
		       edje_object_signal_emit(o, signal, "e");
		       edje_object_message_signal_process(o);
		    }

		  evas_object_resize(o, 1000, 1000);
		  edje_object_calc_force(o);
		  edje_object_part_geometry_get(o, "e.swallow.client", 
						&x, &y, &w, &h);
		  extents->l = x;
		  extents->r = 1000 - (x + w);
		  extents->t = y;
		  extents->b = 1000 - (y + h);
	       }
	     else
	       {
		  extents->l = 0;
		  extents->r = 0;
		  extents->t = 0;
		  extents->b = 0;
	       }
	     evas_object_del(o);
	     frame_extents = evas_hash_add(frame_extents, key, extents);
	  }
     }

   if (extents)
     ecore_x_netwm_frame_size_set(e->win, extents->l, extents->r, extents->t, extents->b);

   return 1;
}

static int
_e_manager_cb_ping(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Border *bd;
   Ecore_X_Event_Ping *e;
   
   man = data;
   e = ev;

   if (e->win != man->root) return 1;

   bd = e_border_find_by_client_window(e->event_win);
   if (!bd) return 1;

   bd->ping_ok = 1;
   return 1;
}

static int
_e_manager_cb_screensaver_notify(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Screensaver_Notify *e;
   
   man = data;
   e = ev;
   
   if (e->on)
     {
	if (e_config->desklock_autolock_screensaver)
	  e_desklock_show();
     }
   return 1;
}

static int
_e_manager_cb_client_message(void *data, int ev_type, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Client_Message *e;
   E_Border *bd;
   
   man = data;
   e = ev;
   
   if (e->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
	bd = e_border_find_by_client_window(e->win);
	if ((bd) && (!bd->focused))
	  {
#if 0 /* notes */	     
	     if (e->data.l[0] == 0 /* 0 == old, 1 == client, 2 == pager */)
	       {
		  // FIXME: need config for the below - what to do given each
		  //  request (either do nothng, make app look urgent/want
		  //  attention or actiually flip to app as below is the
		  //  current default)
		  // if 0 == just make app demand attention somehow
		  // if 1 == just make app demand attention somehow
		  // if 2 == activate window as below
	       }
	     timestamp = e->data.l[1];
	     requestor_id e->data.l[2];
#endif
	     if (bd->iconic)
	       {
		  if (e_config->clientlist_warp_to_iconified_desktop == 1)
		    e_desk_show(bd->desk);
		  
		  if (!bd->lock_user_iconify)
		    e_border_uniconify(bd);
	       }
	     if (!bd->iconic) e_desk_show(bd->desk);
	     if (!bd->lock_user_stacking) e_border_raise(bd);
	     if (!bd->lock_focus_out)
	       {  
		  if (e_config->focus_policy != E_FOCUS_CLICK)
		    ecore_x_pointer_warp(bd->zone->container->win,
					 bd->x + (bd->w / 2), bd->y + (bd->h / 2));
		  e_border_focus_set(bd, 1, 1);
	       }
	  }
     }
   
   return 1;
}

static Evas_Bool
_e_manager_frame_extents_free_cb(const Evas_Hash *hash __UNUSED__, const char *key __UNUSED__,
				 void *data, void *fdata __UNUSED__)
{
   free(data);
   return 1;
}

static E_Manager *
_e_manager_get_for_root(Ecore_X_Window root)
{
   Evas_List *l;
   E_Manager *man;

   if (!managers) return NULL;
   for (l = managers; l; l = l->next)
     {
	man = l->data;
	if (man->root == root)
	  return man;
     }
   return managers->data;
}


#if 0 /* use later - maybe */
static int _e_manager_cb_window_destroy(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_hide(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_reparent(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_create(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_configure_request(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_configure(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_gravity(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_stack(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_stack_request(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_property(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_colormap(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_shape(void *data, int ev_type, void *ev){return 1;}
#endif

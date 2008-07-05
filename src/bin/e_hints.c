/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

Ecore_X_Atom _QTOPIA_SOFT_MENU = 0;
Ecore_X_Atom _QTOPIA_SOFT_MENUS = 0;

EAPI void
e_hints_init(void)
{
   Ecore_X_Window *roots = NULL;
   int num;

   _QTOPIA_SOFT_MENU = ecore_x_atom_get("_QTOPIA_SOFT_MENU");
   _QTOPIA_SOFT_MENUS = ecore_x_atom_get("_QTOPIA_SOFT_MENUS");
   roots = ecore_x_window_root_list(&num);
   if (roots)
     {
	Ecore_X_Atom supported[41];
	int supported_num, i;

	supported_num = 0;
	/* Set what hints we support */
	/* Root Window Properties (and Related Messages) */
	supported[supported_num++] = ECORE_X_ATOM_NET_CLIENT_LIST;
	supported[supported_num++] = ECORE_X_ATOM_NET_CLIENT_LIST_STACKING;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_NUMBER_OF_DESKTOPS, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_DESKTOP_GEOMETRY, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_DESKTOP_VIEWPORT, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_CURRENT_DESKTOP, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_DESKTOP_NAMES, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_ACTIVE_WINDOW;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WORKAREA, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_SUPPORTING_WM_CHECK;
	supported[supported_num++] = ECORE_X_ATOM_NET_VIRTUAL_ROOTS;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_DESKTOP_LAYOUT, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_SHOWING_DESKTOP, 1);*/

	/* Other Root Wsupported_numndow Messages */
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_CLOSE_WINDOW, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_MOVERESIZE_WINDOW, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_MOVERESIZE;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_RESTACK_WINDOW, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_REQUEST_FRAME_EXTENTS;

	/* Application Window Properties */
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_NAME;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_VISIBLE_NAME;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_ICON_NAME;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_VISIBLE_ICON_NAME;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_DESKTOP, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_DESKTOP;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_DOCK;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_MENU;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_UTILITY;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_SPLASH;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_DIALOG;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_WINDOW_TYPE_NORMAL;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_MODAL;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_STICKY;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_MAXIMIZED_VERT;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_MAXIMIZED_HORZ;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_SHADED;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_SKIP_TASKBAR;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_SKIP_PAGER;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_HIDDEN;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_FULLSCREEN;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_ABOVE;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STATE_BELOW;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_STATE_DEMANDS_ATTENTION, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ALLOWED_ACTIONS, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_MOVE, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_RESIZE, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_MINIMIZE, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_SHADE, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_STICK, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_MAXIMIZE_HORZ, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_MAXIMIZE_VERT, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_FULLSCREEN, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_CHANGE_DESKTOP, 1);*/
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ACTION_CLOSE, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STRUT;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_STRUT_PARTIAL;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_ICON_GEOMETRY, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_ICON;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_PID;
	/*ecore_x_netwm_supported(roots[supported_num], ECORE_X_ATOM_NET_WM_HANDLED_ICONS, 1);*/
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_USER_TIME;
	supported[supported_num++] = ECORE_X_ATOM_NET_FRAME_EXTENTS;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_PING;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_SYNC_REQUEST;
	supported[supported_num++] = ECORE_X_ATOM_NET_WM_SYNC_REQUEST_COUNTER;

	for (i = 0; i < num; i++)
	  {
	     Ecore_X_Window win, twin;
	     int nwins;
	     char *name;
	     double ts;
	     
	     /* check for previous netwm wm and wait for it to die */
	     ts = ecore_time_get();
	     nwins = ecore_x_window_prop_window_get(roots[i], 
						    ECORE_X_ATOM_NET_SUPPORTING_WM_CHECK, 
						    &win, 1);
	     if (nwins > 0)
	       {
		  for (;;)
		    {
		       nwins = ecore_x_window_prop_window_get(win, 
							      ECORE_X_ATOM_NET_SUPPORTING_WM_CHECK, 
							      &twin, 1);
		       if (nwins < 1) break;
		       if (twin != win) break;
		       if (ecore_x_netwm_name_get(win, &name))
			 {
			    if (strcmp(name, "Enlightenment"))
			      {
				 free(name);
				 break;
			      }
			    free(name);
			 }
		       ecore_x_sync();
		       if ((ecore_time_get() - ts) > 2.0)
			 {
			    e_error_message_show(_("A previous instance of Enlightenment is still active\n"
						   "on this screen. Aborting startup.\n"));
			    exit(-1);
			 }
		    }
	       }

	     /* TODO: Remember this window and close it on shutdown */
	     win = ecore_x_window_new(roots[i], -200, -200, 5, 5);
/*	     
 * I don't FUCKING believe it. if we PRETEND we are Kwin - java is happy.
 * why? it expects a double reparenting wm then. java insists on finding this
 * out when it should be irrelevant! stupid code! I can't believe the time we
 * just wasted hunting a bug that wasn't and that is due to sheer stupid
 * coding (in java's awt layer that swing also uses).
 */
/* Now for more stupidity... Openoffice.org will change its look and feel 
 * depending on what wm it thinks there is... so if we pretend to be Kwin...
 * it tries to use kde preferences, if found.
 */
/* I have disabled this now by pretending to be E16 with e16 comms. this
 * means java plays nice and uses our FRAME property.. but we had to do other
 * evil stuff as java EXPECTS all this at REPARENT time... i've deferred
 * reparenting... i hate java!
 */
/*	     ecore_x_netwm_wm_identify(roots[i], win, "KWin");*/
	     ecore_x_netwm_wm_identify(roots[i], win, "Enlightenment");
/* this makes openoffice.org read gtk settings so it doesnt look like shit */
	     e_hints_openoffice_gnome_fake(roots[i]);

	     ecore_x_netwm_supported_set(roots[i], supported, supported_num);
	  }
        free(roots);
     }
}

/* 
 * This is here so we don't have to pretend to be Kwin anymore - we pretend
 * to do old e16 style ipc. in fact we just ignore it... but set up the
 * window port anyway
 */
EAPI void
e_hints_e16_comms_pretend(E_Manager *man)
{
   Ecore_X_Window win;
   Ecore_X_Atom enlightenment_comms, enlightenment_version, string;
   char buf[256];
   
   enlightenment_comms = ecore_x_atom_get("ENLIGHTENMENT_COMMS");
   enlightenment_version = ecore_x_atom_get("ENLIGHTENMENT_VERSION");
   string = ecore_x_atom_get("STRING");
   
   win = ecore_x_window_input_new(man->root, -100, -100, 1, 1);

   /* to help detect this is NOT e16 */
   snprintf(buf, sizeof(buf), "Enlightenment %s", VERSION);
   ecore_x_window_prop_property_set(win, enlightenment_version, string, 8, buf, strlen(buf));
   ecore_x_window_prop_property_set(man->root, enlightenment_version, string, 8, buf, strlen(buf));
   
   snprintf(buf, sizeof(buf), "WINID %8x", (int)win);
   ecore_x_window_prop_property_set(win, enlightenment_comms, string, 8, buf, 14);
   
   ecore_x_window_prop_property_set(man->root, enlightenment_comms, string, 8, buf, 14);
}

EAPI void
e_hints_manager_init(E_Manager *man)
{
   /* Set desktop count, desktop names and workarea */
   int			i = 0, num = 0;
   unsigned int		*areas = NULL;
   Evas_List		*cl;
   Ecore_X_Window	*vroots = NULL;
   /* FIXME: Desktop names not yet implemented */
/*   char			**names; */

   e_hints_e16_comms_pretend(man);

   num = evas_list_count(man->containers);

   vroots = calloc(num, sizeof(Ecore_X_Window));
   if (!vroots) return;
   
/*   names = calloc(num, sizeof(char *));*/
   
   areas = calloc(4 * num, sizeof(unsigned int));
   if (!areas)
     {
	free(vroots);
	return;
     }
   
   for (cl = man->containers; cl; cl = cl->next)
     {
	E_Container *c;

	c = cl->data;
	areas[4 * i] = c->x;
	areas[4 * i + 1] = c->y;
	areas[4 * i + 2] = c->w;
	areas[4 * i + 3] = c->h;
	vroots[i++] = c->win;
     }

#if 0
   ecore_x_netwm_desk_count_set(man->root, num);
#endif
   if (e_config->use_virtual_roots)
     {
	ecore_x_netwm_desk_roots_set(man->root, vroots, num);
     }
#if 0
   /* No need for workarea without desktops */
   ecore_x_netwm_desk_workareas_set(man->root, num, areas);
#endif

   free(vroots);
   free(areas);
}

/* FIXME, this should set the list in map order, not stack order */
EAPI void
e_hints_client_list_set(void)
{
   Evas_List *ml = NULL, *cl = NULL;
   unsigned int i = 0, num = 0;
   E_Manager *m;
   E_Container *c;
   E_Border_List *bl;
   E_Border *b;
   Ecore_X_Window *clients = NULL;
   Ecore_X_Window *clients_focus = NULL;

   /* Get client count by adding client lists on all containers */
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	for (cl = m->containers; cl; cl = cl->next)
	  {
	     c = cl->data;
	     num += e_container_borders_count(c);
	  }
     }
   
   clients = calloc(num, sizeof(Ecore_X_Window));
   if (!clients)
      return;

   /* Fetch window IDs and add to array */
   if (num > 0)
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     i = 0;
	     for (cl = m->containers; cl; cl = cl->next)
	       {
		  c = cl->data;
		  bl = e_container_border_list_first(c);
		  while ((b = e_container_border_list_next(bl)))
		    clients[i++] = b->client.win;
		  e_container_border_list_free(bl);
	       }
	     if (i > 0)
	       {
		  ecore_x_netwm_client_list_stacking_set(m->root, clients, i);
		  ecore_x_netwm_client_list_set(m->root, clients, i);
	       }
	     else
	       {
		  ecore_x_netwm_client_list_set(m->root, NULL, 0);
		  ecore_x_netwm_client_list_stacking_set(m->root, NULL, 0);
	       }
	  }
     }
   else
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_set(m->root, NULL, 0);
	     ecore_x_netwm_client_list_stacking_set(m->root, NULL, 0);
	  }
     }
   E_FREE(clients);
}

/* Client list is already in stacking order, so this function is nearly
 * identical to the previous one */
EAPI void
e_hints_client_stacking_set(void)
{
   Evas_List *ml = NULL, *cl = NULL;
   unsigned int i = 0, num = 0;
   E_Manager *m;
   E_Container *c;
   E_Border_List *bl;
   E_Border *b;
   Ecore_X_Window *clients = NULL;

   /* Get client count */
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	m = ml->data;
	for (cl = m->containers; cl; cl = cl->next)
	  {
	     c = cl->data;
	     num += e_container_borders_count(c);
	  }
     }

   if (num > 0)
     {
	clients = calloc(num, sizeof(Ecore_X_Window));
	if (!clients) return;
	
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     for (cl = m->containers; cl; cl = cl->next)
	       {
		  c = cl->data;
		  bl = e_container_border_list_first(c);
		  while ((b = e_container_border_list_next(bl)))
		    {
		       if (i >= num)
			 {
			    e_error_message_show("e_hints.c: e_hints_client_stacking_set()\n"
						 "\n"
						 "Window list size greater than window count.\n"
						 "This is really bad.\n"
						 "Please report this.\n");
			    break;
			 }
		       clients[i++] = b->win;
		    }
		  e_container_border_list_free(bl);
	       }
	  }
	if (i < num)
	  {
	     e_error_message_show("e_hints.c: e_hints_client_stacking_set()\n"
				  "\n"
				  "Window list size less than window count.\n"
				  "This is strange, but not harmful.\n"
				  "Please report this.\n");
	  }
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_stacking_set(m->root, clients, num);
	  }
	E_FREE(clients);
     }
   else
     {
	for (ml = e_manager_list(); ml; ml = ml->next)
	  {
	     m = ml->data;
	     ecore_x_netwm_client_list_stacking_set(m->root, NULL, 0);
	  }
     }
}

EAPI void
e_hints_active_window_set(E_Manager *man, E_Border *bd)
{
   E_OBJECT_CHECK(man);
   if (bd)
     ecore_x_netwm_client_active_set(man->root, bd->client.win);
   else
     ecore_x_netwm_client_active_set(man->root, 0);
}

EAPI void
e_hints_window_init(E_Border *bd)
{
   E_Remember *rem = NULL;

   if (bd->remember)
     rem = bd->remember;

   if (bd->client.icccm.state == ECORE_X_WINDOW_STATE_HINT_NONE)
     {
	if (bd->client.netwm.state.hidden)
	  bd->client.icccm.state = ECORE_X_WINDOW_STATE_HINT_ICONIC;
	else
	  bd->client.icccm.state = ECORE_X_WINDOW_STATE_HINT_NORMAL;
     }

   if ((rem) && (rem->prop.layer))
     {
	bd->layer = rem->prop.layer;
	e_border_layer_set(bd, bd->layer);
     }
   else
     {
	if (!bd->lock_client_stacking)
	  {
	     if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DESKTOP)
	       e_border_layer_set(bd, 0);
	     else if (bd->client.netwm.state.stacking == E_STACKING_BELOW)
	       e_border_layer_set(bd, 50);
	     else if (bd->client.netwm.state.stacking == E_STACKING_ABOVE)
	       e_border_layer_set(bd, 150);
	     else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK)
	       e_border_layer_set(bd, 150);
	     else
	       e_border_layer_set(bd, 100);
	  }
	else
	  e_border_raise(bd);
     }

   if ((bd->parent) && (e_config->transient.layer))
     e_border_layer_set(bd, bd->parent->layer);

#if 0
   /* Ignore this, E has incompatible desktop setup */
   if (ecore_x_netwm_desktop_get(bd->client.win, &bd->client.netwm.desktop))
     {
	if (bd->client.netwm.desktop == 0xffffffff)
	  {
	     e_border_stick(bd);
	  }
	else if (bd->client.netwm.desktop < (bd->zone->desk_x_count * bd->zone->desk_y_count))
	  {
	     E_Desk *desk;

	     desk = e_desk_at_pos_get(bd->zone, bd->client.netwm.desktop);
	     if (desk)
	       e_border_desk_set(bd, desk);
	  }
	else
	  {
	     /* Update netwm desktop with current desktop */
	     e_hints_window_desktop_set(bd);
	  }
     }
   else
     {
	/* Update netwm desktop with current desktop */
	e_hints_window_desktop_set(bd);
     }
#endif

     {
	char *str = NULL;
	
	if ((ecore_x_netwm_startup_id_get(bd->client.win, &str) && (str)) ||
	    ((bd->client.icccm.client_leader > 0) && 
	     ecore_x_netwm_startup_id_get(bd->client.icccm.client_leader, &str) && (str))
	    )
	  {
	     if (!strncmp(str, "E_START|", 8))
	       {
		  int id;

		  id = atoi(str + 8);
		  if (id > 0) bd->client.netwm.startup_id = id;
	       }
	     free(str);
	  }
     }
   /* It's ok not to have fetch flag, should only be set on startup
    * and not changed. */
   if (!ecore_x_netwm_pid_get(bd->client.win, &bd->client.netwm.pid))
     {
        if (bd->client.icccm.client_leader)
	  {
	     if (!ecore_x_netwm_pid_get(bd->client.icccm.client_leader, &bd->client.netwm.pid))
	       bd->client.netwm.pid = -1;
	  }
	else
	  bd->client.netwm.pid = -1;
     }

   if (bd->client.netwm.state.sticky)
     {
	if (!bd->lock_client_sticky)
	  e_border_stick(bd);
	else
	  e_hints_window_sticky_set(bd, 0);
     }
   if (bd->client.netwm.state.shaded)
     {
	if (!bd->lock_client_shade)
	  e_border_shade(bd, e_hints_window_shade_direction_get(bd));
	else
	  e_hints_window_shaded_set(bd, 0);
     }
   if ((bd->client.netwm.state.maximized_v) && (bd->client.netwm.state.maximized_h))
     {
	if (!bd->lock_client_maximize)
	  {
	     e_hints_window_size_get(bd);
	     e_border_maximize(bd, e_config->maximize_policy);
	  }
	else
	  e_hints_window_maximized_set(bd, 0, 0);
     }
   else if (bd->client.netwm.state.maximized_h)
     {
	if (!bd->lock_client_maximize)
	  {
	     e_hints_window_size_get(bd);
	     e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_HORIZONTAL);
	  }
	else
	  e_hints_window_maximized_set(bd, 0, 0);
     }
   else if (bd->client.netwm.state.maximized_v)
     {
	if (!bd->lock_client_maximize)
	  {
	     e_hints_window_size_get(bd);
	     e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_VERTICAL);
	  }
	else
	  e_hints_window_maximized_set(bd, 0, 0);
     }
   if (bd->client.netwm.state.fullscreen)
     {
	if (!bd->lock_client_fullscreen)
	  {
	     e_hints_window_size_get(bd);
	     e_border_fullscreen(bd, e_config->fullscreen_policy);
	  }
	else
	  e_hints_window_fullscreen_set(bd, 0);
     }
   if ((bd->client.icccm.state == ECORE_X_WINDOW_STATE_HINT_ICONIC) &&
       (bd->client.netwm.state.hidden))
     {
	if (!bd->lock_client_iconify)
	  e_border_iconify(bd);
	else
	  e_hints_window_visible_set(bd);
     }
   else if ((bd->parent) && (e_config->transient.iconify) && (bd->parent->iconic))
     e_border_iconify(bd);
   /* If a window isn't iconic, and is one the current desk,
    * show it! */
   else if (bd->desk == e_desk_current_get(bd->zone))
     e_border_show(bd);
   /* e hints */
/*   
   if (bd->client.e.state.centered)
     {
	e_border_move(bd,
		      bd->zone->x + (bd->zone->w - bd->w) / 2,
		      bd->zone->y + (bd->zone->h - bd->h) / 2);
		      
     }
 */
   /* Update stacking */
   e_hints_client_list_set();
   e_hints_client_stacking_set();
}

EAPI void
e_hints_window_state_set(E_Border *bd)
{
   Ecore_X_Window_State state[10];
   int num = 0;

   if (bd->client.netwm.state.modal)
     state[num++] = ECORE_X_WINDOW_STATE_MODAL;
   if (bd->client.netwm.state.sticky)
     state[num++] = ECORE_X_WINDOW_STATE_STICKY;
   if (bd->client.netwm.state.maximized_v)
     state[num++] = ECORE_X_WINDOW_STATE_MAXIMIZED_VERT;
   if (bd->client.netwm.state.maximized_h)
     state[num++] = ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ;
   if (bd->client.netwm.state.shaded)
     state[num++] = ECORE_X_WINDOW_STATE_SHADED;
   if (bd->client.netwm.state.skip_taskbar)
     state[num++] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   if (bd->client.netwm.state.skip_pager)
     state[num++] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   if (bd->client.netwm.state.hidden)
     state[num++] = ECORE_X_WINDOW_STATE_HIDDEN;
   if (bd->client.netwm.state.fullscreen)
     state[num++] = ECORE_X_WINDOW_STATE_FULLSCREEN;

   switch (bd->client.netwm.state.stacking)
     {
      case E_STACKING_ABOVE:
	 state[num++] = ECORE_X_WINDOW_STATE_ABOVE;
	 break;
      case E_STACKING_BELOW:
	 state[num++] = ECORE_X_WINDOW_STATE_BELOW;
	 break;
      case E_STACKING_NONE:
      default:
	 break;
     }
   ecore_x_netwm_window_state_set(bd->client.win, state, num);
}

EAPI void
e_hints_allowed_action_set(E_Border *bd)
{
   Ecore_X_Action action[10];
   int num = 0;

   if (bd->client.netwm.action.move)
     action[num++] = ECORE_X_ACTION_MOVE;
   if (bd->client.netwm.action.resize)
     action[num++] = ECORE_X_ACTION_RESIZE;
   if (bd->client.netwm.action.minimize)
     action[num++] = ECORE_X_ACTION_MINIMIZE;
   if (bd->client.netwm.action.shade)
     action[num++] = ECORE_X_ACTION_SHADE;
   if (bd->client.netwm.action.stick)
     action[num++] = ECORE_X_ACTION_STICK;
   if (bd->client.netwm.action.maximized_h)
     action[num++] = ECORE_X_ACTION_MAXIMIZE_HORZ;
   if (bd->client.netwm.action.maximized_v)
     action[num++] = ECORE_X_ACTION_MAXIMIZE_VERT;
   if (bd->client.netwm.action.fullscreen)
     action[num++] = ECORE_X_ACTION_FULLSCREEN;
   if (bd->client.netwm.action.change_desktop)
     action[num++] = ECORE_X_ACTION_CHANGE_DESKTOP;
   if (bd->client.netwm.action.close)
     action[num++] = ECORE_X_ACTION_CLOSE;

   ecore_x_netwm_allowed_action_set(bd->client.win, action, num);
}

EAPI void
e_hints_window_type_set(E_Border *bd)
{
   ecore_x_netwm_window_type_set(bd->client.win, bd->client.netwm.type);
}

EAPI void
e_hints_window_type_get(E_Border *bd)
{
   Ecore_X_Window_Type *types = NULL;
   int num, i;
   
   num = ecore_x_netwm_window_types_get(bd->client.win, &types);
   if (bd->client.netwm.extra_types)
     {
	free(bd->client.netwm.extra_types);
	bd->client.netwm.extra_types = NULL;
	bd->client.netwm.extra_types_num = 0;
     }
   if (num == 0) 
     {
	bd->client.netwm.type = ECORE_X_WINDOW_TYPE_UNKNOWN;
     }
   else
     {
	bd->client.netwm.type = types[0];
	if (num > 1)
	  {
	     bd->client.netwm.extra_types = 
	       malloc((num - 1) * sizeof(Ecore_X_Window_Type));
	     if (bd->client.netwm.extra_types)
	       {
		  for (i = 1; i < num; i++)
		    bd->client.netwm.extra_types[i - 1] = types[i];
		  bd->client.netwm.extra_types_num = num - 1;
	       }
	  }
	free(types);
     }
}

EAPI void
e_hints_window_state_update(E_Border *bd, Ecore_X_Window_State state,
			    Ecore_X_Window_State_Action action)
{
   switch (state)
     {
      case ECORE_X_WINDOW_STATE_ICONIFIED:
	 if (action != ECORE_X_WINDOW_STATE_ACTION_ADD) return;
	 if (bd->client.icccm.state == ECORE_X_WINDOW_STATE_HINT_ICONIC) return;
	 if (bd->lock_client_iconify) return;
	 e_border_iconify(bd);
	 break;
      case ECORE_X_WINDOW_STATE_MODAL:
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       if (bd->client.netwm.state.modal)
		 {
		    bd->client.netwm.state.modal = 0;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       if (!bd->client.netwm.state.modal)
		 {
		    bd->client.netwm.state.modal = 1;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       bd->client.netwm.state.modal = !bd->client.netwm.state.modal;
	       bd->client.netwm.update.state = 1;
	       bd->changed = 1;
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_STICKY:
	 if (bd->lock_client_sticky) return;
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       e_border_unstick(bd);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       e_border_stick(bd);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->sticky)
		 e_border_unstick(bd);
	       else
		 e_border_stick(bd);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
	 if (bd->lock_client_maximize) return;
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       if (bd->maximized & E_MAXIMIZE_VERTICAL)
		 e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       if (!(bd->maximized & E_MAXIMIZE_VERTICAL))
		 e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_VERTICAL);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->maximized & E_MAXIMIZE_VERTICAL)
		 e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
	       else
		 e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_VERTICAL);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
	 if (bd->lock_client_maximize) return;
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       if (bd->maximized & E_MAXIMIZE_HORIZONTAL)
		 e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       if (!(bd->maximized & E_MAXIMIZE_HORIZONTAL))
		 e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_HORIZONTAL);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->maximized & E_MAXIMIZE_HORIZONTAL)
		 e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
	       else
		 e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | E_MAXIMIZE_HORIZONTAL);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_SHADED:
	 if (bd->lock_client_shade) return;
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       e_border_unshade(bd, e_hints_window_shade_direction_get(bd));
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       e_border_shade(bd, e_hints_window_shade_direction_get(bd));
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->shaded)
		 e_border_unshade(bd, e_hints_window_shade_direction_get(bd));
	       else
		 e_border_shade(bd, e_hints_window_shade_direction_get(bd));
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_SKIP_TASKBAR:
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       if (bd->client.netwm.state.skip_taskbar)
		 {
		    bd->client.netwm.state.skip_taskbar = 0;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       if (!bd->client.netwm.state.skip_taskbar)
		 {
		    bd->client.netwm.state.skip_taskbar = 1;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       bd->client.netwm.state.skip_taskbar = !bd->client.netwm.state.skip_taskbar;
	       bd->client.netwm.update.state = 1;
	       bd->changed = 1;
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_SKIP_PAGER:
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       if (bd->client.netwm.state.skip_pager)
		 {
		    bd->client.netwm.state.skip_pager = 0;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       if (!bd->client.netwm.state.skip_pager)
		 {
		    bd->client.netwm.state.skip_pager = 1;
		    bd->client.netwm.update.state = 1;
		    bd->changed = 1;
		 }
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       bd->client.netwm.state.skip_pager = !bd->client.netwm.state.skip_pager;
	       bd->client.netwm.update.state = 1;
	       bd->changed = 1;
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_HIDDEN:
	 /* Ignore */
	 break;
      case ECORE_X_WINDOW_STATE_FULLSCREEN:
	 if (bd->lock_client_fullscreen) return;
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       e_border_unfullscreen(bd);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       e_border_fullscreen(bd, e_config->fullscreen_policy);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->fullscreen)
		 e_border_unfullscreen(bd);
	       else
		 e_border_fullscreen(bd, e_config->fullscreen_policy);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_ABOVE:
	 if (bd->lock_client_stacking) return;
	 /* FIXME: Should this require that BELOW is set to 0 first, or just
	  * do it? */
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       e_border_layer_set(bd, 100);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       e_border_layer_set(bd, 150);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->layer == 150)
		 e_border_layer_set(bd, 100);
	       else
		 e_border_layer_set(bd, 150);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_BELOW:
	 if (bd->lock_client_stacking) return;
	 /* FIXME: Should this require that ABOVE is set to 0 first, or just
	  * do it? */
	 switch (action)
	   {
	    case ECORE_X_WINDOW_STATE_ACTION_REMOVE:
	       e_border_layer_set(bd, 100);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_ADD:
	       e_border_layer_set(bd, 50);
	       break;
	    case ECORE_X_WINDOW_STATE_ACTION_TOGGLE:
	       if (bd->layer == 50)
		 e_border_layer_set(bd, 100);
	       else
		 e_border_layer_set(bd, 50);
	       break;
	   }
	 break;
      case ECORE_X_WINDOW_STATE_DEMANDS_ATTENTION:
	 /* FIXME */
	 break;
      case ECORE_X_WINDOW_STATE_UNKNOWN:
	 /* Ignore */
	 break;
     }
}

EAPI void
e_hints_window_state_get(E_Border *bd)
{
   unsigned int i, num;
   Ecore_X_Window_State *state;

   bd->client.netwm.state.modal = 0;
   bd->client.netwm.state.sticky = 0;
   bd->client.netwm.state.maximized_v = 0;
   bd->client.netwm.state.maximized_h = 0;
   bd->client.netwm.state.shaded = 0;
   bd->client.netwm.state.skip_taskbar = 0;
   bd->client.netwm.state.skip_pager = 0;
   bd->client.netwm.state.hidden = 0;
   bd->client.netwm.state.fullscreen = 0;
   bd->client.netwm.state.stacking = 0;

   ecore_x_netwm_window_state_get(bd->client.win, &state, &num);
   if (state)
     {
	for (i = 0; i < num; i++)
	  {
	     switch (state[i])
	       {
		case ECORE_X_WINDOW_STATE_ICONIFIED:
		  /* Ignore */
		  break;
		case ECORE_X_WINDOW_STATE_MODAL:
		  bd->client.netwm.state.modal = 1;
		  break;
		case ECORE_X_WINDOW_STATE_STICKY:
		  bd->client.netwm.state.sticky = 1;
		  break;
		case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
		  bd->client.netwm.state.maximized_v = 1;
		  break;
		case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
		  bd->client.netwm.state.maximized_h = 1;
		  break;
		case ECORE_X_WINDOW_STATE_SHADED:
		  bd->client.netwm.state.shaded = 1;
		  break;
		case ECORE_X_WINDOW_STATE_SKIP_TASKBAR:
		  bd->client.netwm.state.skip_taskbar = 1;
		  break;
		case ECORE_X_WINDOW_STATE_SKIP_PAGER:
		  bd->client.netwm.state.skip_pager = 1;
		  break;
		case ECORE_X_WINDOW_STATE_HIDDEN:
		  bd->client.netwm.state.hidden = 1;
		  break;
		case ECORE_X_WINDOW_STATE_FULLSCREEN:
		  bd->client.netwm.state.fullscreen = 1;
		  break;
		case ECORE_X_WINDOW_STATE_ABOVE:
		  bd->client.netwm.state.stacking = E_STACKING_ABOVE;
		  break;
		case ECORE_X_WINDOW_STATE_BELOW:
		  bd->client.netwm.state.stacking = E_STACKING_BELOW;
		  break;
		case ECORE_X_WINDOW_STATE_DEMANDS_ATTENTION:
		  /* FIXME */
		  break;
		case ECORE_X_WINDOW_STATE_UNKNOWN:
		  /* Ignore */
		  break;
	       }
	  }
	free(state);
     }
}

EAPI void
e_hints_allowed_action_update(E_Border *bd, Ecore_X_Action action)
{
   switch (action)
     {
      case ECORE_X_ACTION_MOVE:
	 break;
      case ECORE_X_ACTION_RESIZE:
	 break;
      case ECORE_X_ACTION_MINIMIZE:
	 break;
      case ECORE_X_ACTION_SHADE:
	 break;
      case ECORE_X_ACTION_STICK:
	 break;
      case ECORE_X_ACTION_MAXIMIZE_HORZ:
	 break;
      case ECORE_X_ACTION_MAXIMIZE_VERT:
	 break;
      case ECORE_X_ACTION_FULLSCREEN:
	 break;
      case ECORE_X_ACTION_CHANGE_DESKTOP:
	 break;
      case ECORE_X_ACTION_CLOSE:
	 break;
     }
}

EAPI void
e_hints_allowed_action_get(E_Border *bd)
{
   Ecore_X_Action *action;
   unsigned int    i;
   unsigned int    num;

   bd->client.netwm.action.move = 0;
   bd->client.netwm.action.resize = 0;
   bd->client.netwm.action.minimize = 0;
   bd->client.netwm.action.shade = 0;
   bd->client.netwm.action.stick = 0;
   bd->client.netwm.action.maximized_h = 0;
   bd->client.netwm.action.maximized_v = 0;
   bd->client.netwm.action.fullscreen = 0;
   bd->client.netwm.action.change_desktop = 0;
   bd->client.netwm.action.close = 0;

   ecore_x_netwm_allowed_action_get(bd->client.win, &action, &num);
   if (action)
     {
	for (i = 0; i < num; i++)
	  {
	     switch (action[i])
	       {
		case ECORE_X_ACTION_MOVE:
		  bd->client.netwm.action.move = 1;
		  break;
		case ECORE_X_ACTION_RESIZE:
		  bd->client.netwm.action.resize = 1;
		  break;
		case ECORE_X_ACTION_MINIMIZE:
		  bd->client.netwm.action.minimize = 1;
		  break;
		case ECORE_X_ACTION_SHADE:
		  bd->client.netwm.action.shade = 1;
		  break;
		case ECORE_X_ACTION_STICK:
		  bd->client.netwm.action.stick = 1;
		  break;
		case ECORE_X_ACTION_MAXIMIZE_HORZ:
		  bd->client.netwm.action.maximized_h = 1;
		  break;
		case ECORE_X_ACTION_MAXIMIZE_VERT:
		  bd->client.netwm.action.maximized_v = 1;
		  break;
		case ECORE_X_ACTION_FULLSCREEN:
		  bd->client.netwm.action.fullscreen = 1;
		  break;
		case ECORE_X_ACTION_CHANGE_DESKTOP:
		  bd->client.netwm.action.change_desktop = 1;
		  break;
		case ECORE_X_ACTION_CLOSE:
		  bd->client.netwm.action.close = 1;
		  break;
	       }
	  }
	free(action);
     }
}




EAPI void
e_hints_window_visible_set(E_Border *bd)
{
   if (bd->client.icccm.state != ECORE_X_WINDOW_STATE_HINT_NORMAL)
     {
	ecore_x_icccm_state_set(bd->client.win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
	bd->client.icccm.state = ECORE_X_WINDOW_STATE_HINT_NORMAL;
     }
   if (bd->client.netwm.state.hidden)
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.hidden = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_iconic_set(E_Border *bd)
{
   if (bd->client.icccm.state != ECORE_X_WINDOW_STATE_HINT_ICONIC)
     {
	ecore_x_icccm_state_set(bd->client.win, ECORE_X_WINDOW_STATE_HINT_ICONIC);
	bd->client.icccm.state = ECORE_X_WINDOW_STATE_HINT_ICONIC;
     }
   if (!bd->client.netwm.state.hidden)
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.hidden = 1;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_hidden_set(E_Border *bd)
{
   if (bd->client.icccm.state != ECORE_X_WINDOW_STATE_HINT_WITHDRAWN)
     {
	ecore_x_icccm_state_set(bd->client.win, ECORE_X_WINDOW_STATE_HINT_WITHDRAWN);
	bd->client.icccm.state = ECORE_X_WINDOW_STATE_HINT_WITHDRAWN;
     }
   if (bd->client.netwm.state.hidden)
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.hidden = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_shaded_set(E_Border *bd, int on)
{
   if ((!bd->client.netwm.state.shaded) && (on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.shaded = 1;
	bd->changed = 1;
     }
   else if ((bd->client.netwm.state.shaded) && (!on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.shaded = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_shade_direction_set(E_Border *bd, E_Direction dir)
{
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_SHADE_DIRECTION, &dir, 1);
}

EAPI E_Direction
e_hints_window_shade_direction_get(E_Border *bd)
{
   int ret;
   E_Direction dir;

   ret = ecore_x_window_prop_card32_get(bd->client.win,
	 E_ATOM_SHADE_DIRECTION,
	 &dir, 1);
   if (ret == 1)
     return dir;

   return E_DIRECTION_UP;
}

EAPI void
e_hints_window_size_set(E_Border *bd)
{
   unsigned int sizes[4];

   sizes[0] = bd->x;
   sizes[1] = bd->y;
   sizes[2] = bd->w;
   sizes[3] = bd->h;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_BORDER_SIZE, sizes, 4);
}

EAPI void
e_hints_window_size_unset(E_Border *bd)
{
   ecore_x_window_prop_property_del(bd->client.win, E_ATOM_BORDER_SIZE);
}

EAPI int
e_hints_window_size_get(E_Border *bd)
{
   int ret;
   unsigned int sizes[4];

   memset(sizes, 0, sizeof(sizes));
   ret = ecore_x_window_prop_card32_get(bd->client.win, E_ATOM_BORDER_SIZE,
				        sizes, 4);
   if (ret != 4)
     return 0;

   bd->x = sizes[0];
   bd->y = sizes[1];
   bd->w = sizes[2];
   bd->h = sizes[3];

   return 1;
}

EAPI void
e_hints_window_maximized_set(E_Border *bd, int horizontal, int vertical)
{

   if ((horizontal) && (!bd->client.netwm.state.maximized_h))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.maximized_h = 1;
	bd->changed = 1;
     }
   else if ((!horizontal) && (bd->client.netwm.state.maximized_h))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.maximized_h = 0;
	bd->changed = 1;
     }
   if ((vertical) && (!bd->client.netwm.state.maximized_v))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.maximized_v = 1;
	bd->changed = 1;
     }
   else if ((!vertical) && (bd->client.netwm.state.maximized_v))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.maximized_v = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_fullscreen_set(E_Border *bd, int on)
{
   if ((!bd->client.netwm.state.fullscreen) && (on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.fullscreen = 1;
	bd->changed = 1;
     }
   else if ((bd->client.netwm.state.fullscreen) && (!on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.fullscreen = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_sticky_set(E_Border *bd, int on)
{
   if ((!bd->client.netwm.state.sticky) && (on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.sticky = 1;
	bd->changed = 1;
     }
   else if ((bd->client.netwm.state.sticky) && (!on))
     {
	bd->client.netwm.update.state = 1;
	bd->client.netwm.state.sticky = 0;
	bd->changed = 1;
     }
}

EAPI void
e_hints_window_stacking_set(E_Border *bd, E_Stacking stacking)
{
   if (bd->client.netwm.state.stacking == stacking) return;
   bd->client.netwm.update.state = 1;
   bd->client.netwm.state.stacking = stacking;
   bd->changed = 1;
}

EAPI void
e_hints_window_desktop_set(E_Border *bd)
{
   /* This function is only called when really changing desktop,
    * so just set the property and don't care about the roundtrip.
    */
   unsigned int deskpos[2];

   /* if valgrind complains here it is complaining bd->client.netwm.desktop
    * is an uninitialised variable - but it isn't. it can't be. its part of
    * a calloc()'d struct and thus has to have been set to 0. hell even
    * e_border.c explicitly sets it to 0 on creation of the border object.
    */
   deskpos[0] = bd->desk->x;
   deskpos[1] = bd->desk->y;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_DESK, deskpos, 2);

#if 0
   ecore_x_netwm_desktop_set(bd->client.win, current);
#endif
   bd->client.netwm.desktop = (bd->desk->y * bd->zone->desk_x_count) + bd->desk->x;
}

EAPI void
e_hints_window_e_state_get(E_Border *bd)
{
   /* Remember to update the count if we add more states! */
   Ecore_X_Atom state[1];
   int num;

   memset(state, 0, sizeof(state));
   num = ecore_x_window_prop_card32_get(bd->client.win, E_ATOM_WINDOW_STATE, state, 1);
   if (num)
     {
	int i;
	for (i = 0; i < num; i++)
	  {
	     if (state[i] == E_ATOM_WINDOW_STATE_CENTERED)
	       bd->client.e.state.centered = 1;
	  }
     }
}

EAPI void
e_hints_window_e_state_set(E_Border *bd)
{
   /* TODO */
}

EAPI void
e_hints_window_qtopia_soft_menu_get(E_Border *bd)
{
   unsigned int val;
   
   if (ecore_x_window_prop_card32_get(bd->client.win, _QTOPIA_SOFT_MENU, &val, 1))
     bd->client.qtopia.soft_menu = val;
   else
     bd->client.qtopia.soft_menu = 0;
}

EAPI void
e_hints_window_qtopia_soft_menus_get(E_Border *bd)
{
   unsigned int val;

   if (ecore_x_window_prop_card32_get(bd->client.win, _QTOPIA_SOFT_MENUS, &val, 1))
     bd->client.qtopia.soft_menus = val;
   else
     bd->client.qtopia.soft_menus = 0;
}

EAPI void
e_hints_openoffice_gnome_fake(Ecore_X_Window root)
{
   Ecore_X_Atom gnome_atom, string_atom;
   const char *string = "GNOME_SM_PROXY";
   
   gnome_atom = ecore_x_atom_get("GNOME_SM_PROXY");
   string_atom = ecore_x_atom_get("STRING");
   ecore_x_window_prop_property_set(root, gnome_atom, string_atom, 
				    8, (void *)string, strlen(string));
}

EAPI void
e_hints_openoffice_kde_fake(Ecore_X_Window root)
{
   Ecore_X_Window win2;
   
   win2 = ecore_x_window_new(root, -20, -20, 1, 1);
   ecore_x_netwm_wm_identify(root, win2, "KWin");
}


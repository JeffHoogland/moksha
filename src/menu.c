#include "debug.h"
#include "menu.h"
#include "config.h"
#include "util.h"
#include "math.h"

static Evas_List    open_menus = NULL;	/* List of all open menus */
static Evas_List    menus = NULL;
static E_Menu_Item *curr_selected_item = NULL;	/* Currently selected item */
static Window       menu_event_win = 0;	/* Window which originated event */
static int          screen_w, screen_h;	/* Screen width and height */
static int          mouse_x, mouse_y;	/* Mouse coordinates */
static int          keyboard_nav = 0;	/* If non-zero, navigating with keyboard */

static void         e_idle(void *data);
static void         e_wheel(Ecore_Event * ev);
static void         e_key_down(Ecore_Event * ev);
static void         e_key_up(Ecore_Event * ev);
static void         e_mouse_down(Ecore_Event * ev);
static void         e_mouse_up(Ecore_Event * ev);
static void         e_mouse_move(Ecore_Event * ev);
static void         e_mouse_in(Ecore_Event * ev);
static void         e_mouse_out(Ecore_Event * ev);
static void         e_window_expose(Ecore_Event * ev);
static void         e_menu_item_select(E_Menu_Item * mi);
static void         e_menu_item_unselect(E_Menu_Item * mi);

static void
e_scroller_timer(int val, void *data)
{
   Evas_List           l;
   int                 ok = 0;
   int                 resist = 5;
   int                 scroll_speed = 12;
   static double       last_time = 0.0;
   double              t;

   /* these two lines... */
   E_CFG_INT(cfg_resist, "settings", "/menu/scroll/resist", 5);
   E_CFG_INT(cfg_scroll_speed, "settings", "/menu/scroll/speed", 12);

   D_ENTER;

   /* and these 2 should do exactly what tom wants - see e.h */
   E_CONFIG_INT_GET(cfg_resist, resist);
   E_CONFIG_INT_GET(cfg_scroll_speed, scroll_speed);

   t = ecore_get_time();
   if (val != 0)
      scroll_speed = (int)(((t - last_time) / 0.02) * (double)scroll_speed);
   last_time = t;

   ok = 0;
   if (mouse_x >= (screen_w - resist))
     {
	int                 scroll = 0;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if ((m->current.x + m->current.w) > screen_w)
		scroll = m->current.x + m->current.w - screen_w;
	  }
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_menu_scroll_all_by(-scroll, 0);
	     ok = 1;
	  }
     }
   else if (mouse_x < resist)
     {
	int                 scroll = 0;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if (m->current.x < 0)
		scroll = -m->current.x;
	  }
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_menu_scroll_all_by(scroll, 0);
	     ok = 1;
	  }
     }
   if (mouse_y >= (screen_h - resist))
     {
	int                 scroll = 0;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if ((m->current.y + m->current.h) > screen_h)
		scroll = m->current.y + m->current.h - screen_h;
	  }
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_menu_scroll_all_by(0, -scroll);
	     ok = 1;
	  }
     }
   else if (mouse_y < resist)
     {
	int                 scroll = 0;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if (m->current.y < 0)
		scroll = -m->current.y;
	  }
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_menu_scroll_all_by(0, scroll);
	     ok = 1;
	  }
     }
   if ((ok) && (open_menus))
      ecore_add_event_timer("menu_scroller", 0.02, e_scroller_timer, val + 1,
			    NULL);

   D_RETURN;
   UN(data);
}

static void
e_idle(void *data)
{
   Evas_List           l;

   D_ENTER;

   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	e_menu_update_base(m);
     }
   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	e_menu_update_shows(m);
     }
   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	e_menu_update_hides(m);
     }
   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	e_menu_update_finish(m);
     }
   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	if (m->first_expose)
	   evas_render(m->evas);
     }
 again:
   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	if (m->delete_me)
	  {
	     e_object_unref(E_OBJECT(m));
	     goto again;
	  }
     }

   e_db_runtime_flush();

   D_RETURN;
   UN(data);
}

/**
 * e_wheel - Handle mouse wheel events
 *
 * @ev: Pointer to event.
 */
static void
e_wheel(Ecore_Event * ev)
{
   Ecore_Event_Wheel  *e;

   D_ENTER;

   e = ev->event;
   if (e->win == menu_event_win)
     {
     }

   D_RETURN;
}

/**
 * e_key_down - Handle key down events
 *
 * @ev: Pointer to event.
 */
static void
e_key_down(Ecore_Event * ev)
{
   Ecore_Event_Key_Down *e;
   int                 ok;

   D_ENTER;

   e = ev->event;
   ok = 0;
   if (e->win == menu_event_win)
      ok = 1;
   else
     {
	Evas_List           l;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if ((e->win == m->win.main) || (e->win == m->win.evas))
	       {
		  ok = 1;
		  break;
	       }
	  }
     }
   if (ok)
     {
	Evas_List           l;
	E_Menu             *m = NULL;
	E_Menu_Item        *mi = NULL;

	for (l = open_menus; l; l = l->next)
	  {
	     m = l->data;
	     if (m->selected)
	       {
		  mi = m->selected;
		  break;
	       }
	  }
	if (!strcmp(e->key, "Up"))
	  {
	     keyboard_nav = 1;
	     e_menu_select(0, -1);
	  }
	else if (!strcmp(e->key, "Down"))
	  {
	     keyboard_nav = 1;
	     e_menu_select(0, 1);
	  }
	else if (!strcmp(e->key, "Left"))
	  {
	     keyboard_nav = 1;
	     e_menu_select(-1, 0);
	  }
	else if (!strcmp(e->key, "Right"))
	  {
	     keyboard_nav = 1;
	     e_menu_select(1, 0);
	  }
	else if (!strcmp(e->key, "Escape"))
	  {
	     keyboard_nav = 1;
	     for (l = menus; l; l = l->next)
	       {
		  m = l->data;

		  if (m->current.visible)
		     e_menu_hide(m);
	       }
	  }
	else if (!strcmp(e->key, "Return"))
	  {
	     keyboard_nav = 1;
	     if (mi)
	       {
		  e_menu_callback_item(m, mi);
		  mi->selected = 0;
		  mi->menu->selected = NULL;
	       }
	     for (l = menus; l; l = l->next)
	       {
		  m = l->data;

		  if (m->current.visible)
		     e_menu_hide(m);
	       }
	  }
	else
	  {
	  }
     }

   D_RETURN;
}

/**
 * e_key_up - Handle key up events
 *
 * @ev: Pointer to event.
 */
static void
e_key_up(Ecore_Event * ev)
{
   Ecore_Event_Key_Up *e;

   D_ENTER;

   e = ev->event;
   {
   }

   D_RETURN;
}

/**
 * e_mouse_down - Handle mouse down events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Down *e;

   D_ENTER;

   e = ev->event;
   {
   }

   D_RETURN;
}

/**
 * e_mouse_up - Handle mouse up events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;

   D_ENTER;

   e = ev->event;
   keyboard_nav = 0;
   if (e->win == menu_event_win)
     {
	if (open_menus)
	  {
	     E_Menu             *m;

	     m = open_menus->data;
	     if ((e->time - m->time) > 200)
	       {
		  Evas_List           l;

		  for (l = open_menus; l; l = l->next)
		    {
		       m = l->data;
		       /* Ensure that the item is actually selected and
		        * that the mouse pointer really is over it: */
		       if (m->selected)
			 {
				/* Get the dimensions of the selection for use in
				 * the test */
				double s_x, s_y, s_w, s_h;
				evas_get_geometry(m->evas, m->selected->obj_entry, 
						&s_x, &s_y, &s_w, &s_h);
			    if (INTERSECTS(m->current.x + rint(s_x),
					   m->current.y + rint(s_y),
					   rint(s_w),
					   rint(s_h),
					   mouse_x, mouse_y, 0,
					   0))
			      {
				 e_menu_callback_item(m, m->selected);
			      }
			    e_menu_item_unselect(m->selected);
			    break;
			 }
		    }
		  for (l = menus; l; l = l->next)
		    {
		       m = l->data;

		       if (m->current.visible)
			  e_menu_hide(m);
		    }
	       }
	  }
     }

   D_RETURN;
}

/**
 * e_mouse_move - Handle mouse move events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move *e;

   D_ENTER;

   e = ev->event;
   keyboard_nav = 0;
   if (e->win == menu_event_win)
     {
	Evas_List           l;

	mouse_x = e->rx;
	mouse_y = e->ry;
	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     evas_event_move(m->evas,
			     e->rx - m->current.x, e->ry - m->current.y);
	  }
     }
   else
     {
	Evas_List           l;

	mouse_x = e->rx;
	mouse_y = e->ry;
	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;

	     evas_event_move(m->evas,
			     e->rx - m->current.x, e->ry - m->current.y);
	  }
     }
   e_scroller_timer(0, NULL);

   D_RETURN;
}

/**
 * e_mouse_in - Handle mouse enter events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_in(Ecore_Event * ev)
{
   Ecore_Event_Window_Enter *e;

   D_ENTER;

   e = ev->event;
   keyboard_nav = 0;
   if (e->win == menu_event_win)
     {
     }

   D_RETURN;
}

/**
 * e_mouse_out - Handle mouse leave events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_out(Ecore_Event * ev)
{
   Ecore_Event_Window_Leave *e;

   D_ENTER;

   e = ev->event;
   keyboard_nav = 0;
   if (e->win == menu_event_win)
     {
     }
   else
     {
	Evas_List           l;

	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if ((e->win == m->win.main) || (e->win == m->win.evas))
	       {
		  evas_event_move(m->evas, -99999999, -99999999);
	       }
	  }
     }

   D_RETURN;
}

/**
 * e_window_expose - Handle window expose events
 *
 * @ev: Pointer to event.
 */
static void
e_window_expose(Ecore_Event * ev)
{
   Ecore_Event_Window_Expose *e;

   D_ENTER;

   e = ev->event;
   {
      Evas_List           l;

      for (l = open_menus; l; l = l->next)
	{
	   E_Menu             *m;

	   m = l->data;
	   if (e->win == m->win.evas)
	     {
		m->first_expose = 1;
		evas_update_rect(m->evas, e->x, e->y, e->w, e->h);
		break;
	     }
	}
   }

   D_RETURN;
}

/**
 * e_menu_item_unselect - Unselect a menu item.
 *
 * @mi: Pointer to the menu item to be unselected.
 */
static void
e_menu_item_unselect(E_Menu_Item * mi)
{
   D_ENTER;
   if ((mi) && (mi->menu->selected == mi))
     {
	mi->menu->selected = curr_selected_item = NULL;

	mi->selected = 0;
	mi->menu->redo_sel = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

/**
 * e_menu_item_select - Select a menu item.
 *   NOTE - Assumes only one item can be selected at once,
 *          and unselects any previously selected menu item.
 *
 * @mi: Pointer to the menu item to be selected.
 */
static void
e_menu_item_select(E_Menu_Item * mi)
{
   D_ENTER;

   //  e_menu_item_unselect(curr_selected_item);
   if (mi)
     {
	mi->menu->selected = mi;
	mi->selected = 1;
	mi->menu->redo_sel = 1;
	mi->menu->changed = 1;
	curr_selected_item = mi;
     }

   D_RETURN;
}

/**
 * e_menu_item_in_cb - Callback for when mouse enters a specific menu item.
 *   Attached by e_item_realize().  Selects menu item.
 *
 * @_data: Pointer to actual menu item structure.
 * @_e: Evas
 * @_o: Evas object
 * @_b: ?????
 * @x: ?????
 * @y: ?????
 */
static void
e_menu_item_in_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Menu_Item        *mi;

   D_ENTER;

   mi = _data;
   e_menu_item_select(mi);
   e_menu_hide_submenus(mi->menu);
   if (mi->submenu && mi->submenu->entries)
     {
	e_menu_move_to(mi->submenu,
		       mi->menu->current.x + mi->menu->current.w,
		       mi->menu->current.y + mi->y - mi->menu->border.t);
	e_menu_show(mi->submenu);
     }

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_b);
   UN(_x);
   UN(_y);
}

/**
 * e_menu_item_out_cb - Callback for when mouse leaves a specific menu item.
 *   Attached by e_item_realize().  Unselects menu item.
 *
 * @_data: Pointer to actual menu item structure.
 * @_e: Evas
 * @_o: Evas object
 * @_b: ?????
 * @x: ?????
 * @y: ?????
 */
static void
e_menu_item_out_cb(void *_data, Evas _e, Evas_Object _o, int _b, int _x, int _y)
{
   E_Menu_Item        *mi;

   D_ENTER;

   mi = _data;
   e_menu_item_unselect(mi);

   D_RETURN;
   UN(_e);
   UN(_o);
   UN(_b);
   UN(_x);
   UN(_y);
}

void
e_menu_callback_item(E_Menu * m, E_Menu_Item * mi)
{
   D_ENTER;

   if (mi->func_select)
      mi->func_select(m, mi, mi->func_select_data);

   D_RETURN;
}

void
e_menu_hide_callback(E_Menu * m, void (*func) (E_Menu * m, void *data),
		     void *data)
{
   m->func_hide = func;
   m->func_hide_data = data;
}

void
e_menu_item_set_callback(E_Menu_Item * mi,
			 void (*func) (E_Menu * m, E_Menu_Item * mi,
				       void *data), void *data)
{
   D_ENTER;

   mi->func_select = func;
   mi->func_select_data = data;

   D_RETURN;
}

/**
 * e_menu_hide_submenus - Hide all menus except @menus_after.
 *   Assumes all menus after @menus_after in the list open_menus
 *   are submenus of @menus_after.
 *
 * @menus_after: All menus after this are hidden.
 */
void
e_menu_hide_submenus(E_Menu * menus_after)
{
   Evas_List           l;

   D_ENTER;

   /* Loop thru all open menus: */
   for (l = open_menus; l; l = l->next)
     {
	/* Found submenu, so now hide all remaining menus: */
	if (l->data == menus_after)
	  {
	     l = l->next;
	     for (; l; l = l->next)
	       {
		  E_Menu             *m;

		  m = l->data;
		  e_menu_hide(m);
	       }
	     break;
	  }
     }

   D_RETURN;
}

/**
 * e_menu_select - Attempt to select the menu entry @dx entries across,
 *   and @dy entries down.
 *
 * @dx: Horizontal offset of new menu entry.
 * @dy: Vertical offset of new menu entry.
 */
void
e_menu_select(int dx, int dy)
{
   Evas_List           l, ll;
   int                 done = 0;

   D_ENTER;

   /* Loop through all open menus, tile done or reached end */
   for (l = open_menus; (l) && (!done); l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	/* If this is the selected menu: */
	if (m->selected)
	  {
	     /* Go through the menu entries: */
	     for (ll = m->entries; (ll) && (!done); ll = ll->next)
	       {
		  E_Menu_Item        *mi;

		  mi = ll->data;
		  /* Found the currently selected entry: */
		  if (mi->selected)
		    {
		       /* Vertical movement, up and down menu: */
		       if (dy != 0)
			 {
			    int                 ok = 0;

			    /* Only go up or down if entry exists to do so, */
			    /* and skip over separators: */
			    if (dy < 0)
			      {
				 for (; ll->prev; ll = ll->prev)
				   {
				      mi = ll->prev->data;
				      if (!mi->separator)
					{
					   ok = 1;
					   break;
					}
				   }
			      }
			    else if (dy > 0)
			      {
				 for (; ll->next; ll = ll->next)
				   {
				      mi = ll->next->data;
				      if (!mi->separator)
					{
					   ok = 1;
					   break;
					}
				   }
			      }

			    if (ok)
			      {
				 /* Unselect old and select new item: */
				 e_menu_item_unselect(m->selected);
				 e_menu_item_select(mi);
				 e_menu_hide_submenus(mi->menu);

				 /* If submenu, display it: */
				 if (mi->submenu && mi->submenu->entries)
				   {
				      e_menu_move_to(mi->submenu,
						     mi->menu->current.x +
						     mi->menu->current.w,
						     mi->menu->current.y +
						     mi->y -
						     mi->menu->border.t);
				      e_menu_show(mi->submenu);
				   }
			      }
			 }
		       done = 1;
		    }
	       }
	     /* Horizontal movement, into and out of submenus: */
	     if (dx != 0)
	       {
		  int                 ok = 0;

		  /* Only carry on if appropriate submenus exist: */
		  if ((dx < 0) && (l->prev))
		     ok = 1;
		  else if ((dx > 0) && (l->next))
		     ok = 1;
		  if (ok)
		    {
		       E_Menu_Item        *mi = NULL;
		       E_Menu             *mm;

		       /* Moving out of a submenu: */
		       if (dx < 0)
			 {
			    Evas_List           ll;

			    mm = l->prev->data;
			    for (ll = mm->entries; (ll) && (!mi); ll = ll->next)
			      {
				 E_Menu_Item        *mmi;

				 mmi = ll->data;
				 if (mmi->submenu == m)
				    mi = mmi;
			      }
			 }
		       /* Moving into a submenu: */
		       else
			 {
			    mm = l->next->data;
			    if (mm->entries)
			       mi = mm->entries->data;
			 }
		       if (mi)
			 {
			    /* Unselect old and select new item: */
			    e_menu_item_unselect(m->selected);
			    e_menu_item_select(mi);
			    e_menu_hide_submenus(mi->menu);

			    /* If new entry is a submenu, display it: */
			    if (mi->submenu && mi->submenu->entries)
			      {
				 e_menu_move_to(mi->submenu,
						mi->menu->current.x +
						mi->menu->current.w,
						mi->menu->current.y + mi->y -
						mi->menu->border.t);
				 e_menu_show(mi->submenu);
			      }
			    e_menu_update_visibility(mm);
			 }
		    }
		  done = 1;
	       }
	     e_menu_update_visibility(m);
	  }
     }
   /* If opened a new submenu, position it and display it: */
   if (!done)
     {
	if (open_menus)
	  {
	     E_Menu             *m;
	     E_Menu_Item        *mi;

	     m = open_menus->data;
	     mi = m->entries->data;
	     e_menu_item_select(mi);
	     if (mi->submenu && mi->submenu->entries)
	       {
		  e_menu_move_to(mi->submenu,
				 mi->menu->current.x + mi->menu->current.w,
				 mi->menu->current.y + mi->y -
				 mi->menu->border.t);
		  e_menu_show(mi->submenu);
	       }
	  }
     }

   D_RETURN;
}

void
e_menu_init(void)
{
   D_ENTER;

   ecore_window_get_geometry(0, NULL, NULL, &screen_w, &screen_h);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN, e_mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP, e_mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE, e_mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_IN, e_mouse_in);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_OUT, e_mouse_out);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, e_window_expose);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN, e_key_down);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_UP, e_key_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_WHEEL, e_wheel);
   ecore_event_filter_idle_handler_add(e_idle, NULL);

   D_RETURN;
}

void
e_menu_event_win_show(void)
{
   D_ENTER;

   /* create it */
   if (!menu_event_win)
     {
	menu_event_win = ecore_window_input_new(0, 0, 0, screen_w, screen_h);
	ecore_window_set_events(menu_event_win,
				XEV_MOUSE_MOVE | XEV_BUTTON | XEV_IN_OUT |
				XEV_KEY);
	ecore_window_show(menu_event_win);
	ecore_keyboard_grab(menu_event_win);
	ecore_grab_mouse(menu_event_win, 1, 0);
     }
   /* raise it */
   if (menu_event_win)
      ecore_window_raise(menu_event_win);
   if ((!ecore_grab_window_get()) || (!ecore_keyboard_grab_window_get()))
     {
	Evas_List           l;

	for (l = menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     if (m->current.visible)
		e_menu_hide(m);
	  }
	e_menu_event_win_hide();
     }

   D_RETURN;
}

void
e_menu_event_win_hide(void)
{
   D_ENTER;

   /* destroy it */
   if (menu_event_win)
     {
	ecore_ungrab_mouse();
	ecore_keyboard_ungrab();
	ecore_window_destroy(menu_event_win);
	menu_event_win = 0;
     }

   D_RETURN;
}

/**
 * e_menu_set_background - Sets the background of menu @m
 *   Sets background of menu using the default theme background,
 *   base.bits.db
 *
 * @m: Menu to set background on.
 */
void
e_menu_set_background(E_Menu * m)
{
   char               *menus;
   char                buf[PATH_MAX];
   char               *part;
   int                 pl, pr, pt, pb;

   D_ENTER;

   menus = e_config_get("menus");

   part = "base.bits.db";
   snprintf(buf, PATH_MAX, "%s%s", menus, part);
   if ((m->bg_file) && (!strcmp(m->bg_file, buf)))
      D_RETURN;

   IF_FREE(m->bg_file);
   m->bg_file = strdup(buf);

   if (m->bg)
      ebits_free(m->bg);
   m->bg = ebits_load(m->bg_file);
   if (m->bg)
      ebits_set_color_class(m->bg, "Menu BG", 100, 200, 255, 255);

   pl = pr = pt = pb = 0;
   if (m->bg)
     {
	ebits_get_insets(m->bg, &pl, &pr, &pt, &pb);
	ebits_add_to_evas(m->bg, m->evas);
	ebits_move(m->bg, 0, 0);
	ebits_show(m->bg);
	ebits_set_layer(m->bg, 0);
     }
   m->current.w += ((pl + pr) - (m->border.l + m->border.r));
   m->current.h += ((pt + pb) - (m->border.t + m->border.b));
   m->border.l = pl;
   m->border.r = pr;
   m->border.t = pt;
   m->border.b = pb;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_set_sel(E_Menu * m, E_Menu_Item * mi)
{
   char               *menus;
   char                buf[PATH_MAX];
   int                 pl, pr, pt, pb;
   int                 has_sub = 0;
   int                 selected = 0;

   D_ENTER;

   menus = e_config_get("menus");
   if (!mi->separator)
     {
	selected = mi->selected;
	if (mi->submenu)
	   has_sub = 1;
	snprintf(buf, PATH_MAX, "%sselected-%i.submenu-%i.bits.db", menus,
		 selected, has_sub);
	if ((mi->bg_file) && (!strcmp(mi->bg_file, buf)))
	   D_RETURN;
     }
   IF_FREE(mi->bg_file);
   if (!mi->separator)
      mi->bg_file = strdup(buf);
   else
      mi->bg_file = NULL;
   if (mi->bg)
      ebits_free(mi->bg);
   if (mi->bg_file)
      mi->bg = ebits_load(mi->bg_file);
   if (mi->bg)
      ebits_set_color_class(mi->bg, "Menu BG", 100, 200, 255, 255);

   pl = pr = pt = pb = 0;
   if (mi->bg)
     {
	ebits_get_insets(mi->bg, &pl, &pr, &pt, &pb);
	ebits_add_to_evas(mi->bg, m->evas);
	ebits_set_layer(mi->bg, 1);
     }
   if (m->sel_border.l < pl)
     {
	m->sel_border.l = pl;
	m->recalc_entries = 1;
     }
   if (m->sel_border.r < pr)
     {
	m->sel_border.r = pr;
	m->recalc_entries = 1;
     }
   if (m->sel_border.t < pt)
     {
	m->sel_border.t = pt;
	m->recalc_entries = 1;
     }
   if (m->sel_border.b < pb)
     {
	m->sel_border.b = pb;
	m->recalc_entries = 1;
     }
   m->redo_sel = 1;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_set_sep(E_Menu * m, E_Menu_Item * mi)
{
   char               *menus;
   char                buf[PATH_MAX];
   int                 pl, pr, pt, pb, minx, miny;

   D_ENTER;

   menus = e_config_get("menus");
   snprintf(buf, PATH_MAX, "%sseparator.bits.db", menus);
   if ((mi->sep_file) && (!strcmp(mi->sep_file, buf)))
      D_RETURN;

   IF_FREE(mi->sep_file);
   mi->sep_file = strdup(buf);

   if (mi->sep)
      ebits_free(mi->sep);
   mi->sep = ebits_load(mi->sep_file);
   if (mi->sep)
      ebits_set_color_class(mi->sep, "Menu BG", 100, 200, 255, 255);

   pl = pr = pt = pb = 0;
   minx = 0;
   miny = 0;
   if (mi->sep)
     {
	ebits_get_insets(mi->sep, &pl, &pr, &pt, &pb);
	ebits_add_to_evas(mi->sep, m->evas);
	ebits_set_layer(mi->sep, 1);
	ebits_get_min_size(mi->sep, &minx, &miny);
     }
   if (mi->size.min.w < minx)
      mi->size.min.w = minx;
   if (mi->size.min.h < miny)
      mi->size.min.h = miny;
   m->redo_sel = 1;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_set_state(E_Menu * m, E_Menu_Item * mi)
{
   char               *menus;
   char                buf[PATH_MAX];
   int                 on;
   int                 pl, pr, pt, pb, minx, miny;

   D_ENTER;

   menus = e_config_get("menus");
   on = mi->on;
   if (mi->check)
      snprintf(buf, PATH_MAX, "%scheck-%i.bits.db", menus, on);
   else
      snprintf(buf, PATH_MAX, "%sradio-%i.bits.db", menus, on);
   if ((mi->state_file) && (!strcmp(mi->state_file, buf)))
      D_RETURN;

   IF_FREE(mi->state_file);
   mi->state_file = strdup(buf);

   if (mi->state)
      ebits_free(mi->state);
   mi->state = ebits_load(mi->state_file);
   if (mi->state)
      ebits_set_color_class(mi->state, "Menu BG", 100, 200, 255, 255);

   pl = pr = pt = pb = 0;
   minx = 0;
   miny = 0;
   if (mi->state)
     {
	ebits_get_insets(mi->state, &pl, &pr, &pt, &pb);
	ebits_add_to_evas(mi->state, m->evas);
	ebits_set_layer(mi->state, 2);
	ebits_get_min_size(mi->state, &minx, &miny);
     }
   if (mi->size.min.w < minx)
      mi->size.min.w = minx;
   if (mi->size.min.h < miny)
      mi->size.min.h = miny;
   m->redo_sel = 1;
   m->changed = 1;

   D_RETURN;
}

static void
e_menu_cleanup(E_Menu * m)
{
   Evas_List           l;

   D_ENTER;

   for (l = m->entries; l; l = l->next)
     {
	E_Menu_Item        *mi;

	mi = l->data;
	e_menu_item_unrealize(m, mi);
	IF_FREE(mi->str);
	IF_FREE(mi->icon);
	free(mi);
     }
   m->entries = evas_list_free(m->entries);
   m->selected = NULL;
   IF_FREE(m->bg_file);
   evas_free(m->evas);
   ecore_window_destroy(m->win.main);
   menus = evas_list_remove(menus, m);
   open_menus = evas_list_remove(open_menus, m);

   /* Call the destructor of the base class */
   e_object_cleanup(E_OBJECT(m));
   m = NULL;

   D_RETURN;
}

E_Menu             *
e_menu_new(void)
{
   E_Menu             *m;
   int                 max_colors = 216;
   int                 font_cache = 1024 * 1024;
   int                 image_cache = 8192 * 1024;
   char               *font_dir;

   D_ENTER;

   font_dir = e_config_get("fonts");

   m = NEW(E_Menu, 1);
   ZERO(m, E_Menu, 1);

   e_object_init(E_OBJECT(m), (E_Cleanup_Func) e_menu_cleanup);

   m->win.main = ecore_window_override_new(0, 0, 0, 1, 1);
   m->evas = evas_new_all(ecore_display_get(),
			  m->win.main,
			  0, 0, 1, 1,
			  RENDER_METHOD_ALPHA_SOFTWARE,
			  max_colors, font_cache, image_cache, font_dir);
   /* aaaaaaaaah. this makes building the menu fast - moves the mouse far */
   /* far far far far away so callbacks and events arent triggerd as we */
   /* create objects that ofter hang around 0,0 - the default place for */
   /* the pointer to be... this means my 2000 entry menu works  and comes up */
   /* pretty damn fast - considering i creating it when i click :) - problem */
   /* you can't fit 2000 entires into a window in X - since the limit is */
   /* 65536x65536 fo X - the other problem is i can only really draw in */
   /* the first 32768x32768 pixels of the window - thus limiting the menu */
   /* size to well - 32768x32768 - normally ok - but in extremes not so */
   /* good. We *COULD* do a workaround that meant we did the menu scrolling */
   /* within the evas and faked a menu window that only gets as big as the */
   /* screen - an then re-render it all - but well.. it's an extreme and */
   /* for now i think people will just have to live with a maximum menu size */
   /* of 32768x32768... didums! */
   evas_event_move(m->evas, -999999999, -99999999);

   m->win.evas = evas_get_window(m->evas);
   ecore_window_set_events(m->win.evas,
			   XEV_EXPOSE | XEV_MOUSE_MOVE | XEV_BUTTON | XEV_IN_OUT
			   | XEV_KEY);
   ecore_window_set_events(m->win.main, XEV_IN_OUT | XEV_KEY);
   ecore_window_show(m->win.evas);

   e_menu_set_background(m);

   m->current.w = m->border.l + m->border.r;
   m->current.h = m->border.t + m->border.b;
   m->changed = 1;

   menus = evas_list_prepend(menus, m);

   D_RETURN_(m);
}

void
e_menu_hide(E_Menu * m)
{
   D_ENTER;

   if (m->selected)
     {
	m->selected->selected = 0;
     }
   m->selected = NULL;
   m->redo_sel = 1;
   m->current.visible = 0;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_show(E_Menu * m)
{
   D_ENTER;

   m->current.visible = 1;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_move_to(E_Menu * m, int x, int y)
{
   D_ENTER;

   m->current.x = x;
   m->current.y = y;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_show_at_mouse(E_Menu * m, int x, int y, Time t)
{
   D_ENTER;

   D("show at mouse\n");
   m->current.x = x;
   m->current.y = y;
   m->time = t;
   D("show menu\n") e_menu_show(m);

   D_RETURN;
}

void
e_menu_add_item(E_Menu * m, E_Menu_Item * mi)
{
   D_ENTER;

   m->entries = evas_list_append(m->entries, mi);
   m->recalc_entries = 1;
   m->changed = 1;
   mi->menu = m;
   e_menu_item_realize(m, mi);

   D_RETURN;
}

void
e_menu_del_item(E_Menu * m, E_Menu_Item * mi)
{
   D_ENTER;

   m->entries = evas_list_remove(m->entries, mi);
   m->recalc_entries = 1;
   m->changed = 1;
   e_menu_item_unrealize(m, mi);
   IF_FREE(mi->str);
   IF_FREE(mi->icon);
   if (mi->menu->selected == mi)
      mi->menu->selected = NULL;
   FREE(mi);
   mi->menu = NULL;

   D_RETURN;
}

void
e_menu_item_update(E_Menu * m, E_Menu_Item * mi)
{
   int                 tx, ty, tw, th, ix, iy, iw, ih, rx, ry, rw, rh;
   double              dtw, dth;

   D_ENTER;

   if (mi->sep)
     {
	ebits_move(mi->sep, mi->x, mi->y);
	ebits_resize(mi->sep, mi->size.w + m->sel_border.l + m->sel_border.r,
		     mi->size.h);
	ebits_show(mi->sep);
     }
   else
     {
	rx = 0;
	ry = 0;
	rh = 0;
	rw = m->size.state;
	if (mi->state)
	  {
	     ebits_get_min_size(mi->state, &rw, &rh);
	     rx = 0;
	     ry = ((mi->size.h - rh) / 2);
	     ebits_move(mi->state, m->sel_border.l + mi->x + rx,
			m->sel_border.t + mi->y + ry);
	     ebits_resize(mi->state, rw, rh);
	  }

	tx = 0;
	ty = 0;
	tw = 0;
	th = 0;
	if (mi->obj_text)
	  {
	     evas_get_geometry(m->evas, mi->obj_text, NULL, NULL, &dtw, &dth);
	     tw = (int)dtw;
	     th = (int)dth;
	  }

	ix = 0;
	iy = 0;
	iw = 0;
	ih = 0;
	if (mi->obj_icon)
	  {
	     int                 sh;

	     evas_get_image_size(m->evas, mi->obj_icon, &iw, &ih);
	     sh = th;
	     if (rh > th)
		sh = rh;
	     if ((mi->scale_icon) && (ih > sh) && (mi->str))
	       {
		  iw = (iw * sh) / ih;
		  ih = sh;
	       }
	     if (m->size.state)
		ix = rx + m->size.state + m->pad.state;
	     ix += ((m->size.icon - iw) / 2);
	     iy = ((mi->size.h - ih) / 2);
	     evas_move(m->evas, mi->obj_icon, m->sel_border.l + mi->x + ix,
		       m->sel_border.t + mi->y + iy);
	     evas_resize(m->evas, mi->obj_icon, iw, ih);
	     evas_set_image_fill(m->evas, mi->obj_icon, 0, 0, iw, ih);
	  }

	if (mi->obj_text)
	  {
	     if (m->size.state)
		tx = rx + m->size.state + m->pad.state;
	     if (m->size.icon)
		tx += m->size.icon + m->pad.icon;
	     ty = ((mi->size.h - th) / 2);
	     evas_move(m->evas, mi->obj_text, m->sel_border.l + mi->x + tx,
		       m->sel_border.t + mi->y + ty);
	  }

	if (mi->obj_entry)
	  {
	     evas_move(m->evas, mi->obj_entry, mi->x, mi->y);
	     evas_resize(m->evas, mi->obj_entry,
			 mi->size.w + m->sel_border.l + m->sel_border.r,
			 mi->size.h + m->sel_border.t + m->sel_border.b);
	  }
	if (mi->state)
	  {
	     ebits_show(mi->state);
	  }
     }

   D_RETURN;
}

void
e_menu_item_unrealize(E_Menu * m, E_Menu_Item * mi)
{
   D_ENTER;

   if (mi->bg)
      ebits_free(mi->bg);
   mi->bg = NULL;
   IF_FREE(mi->bg_file);
   mi->bg_file = NULL;
   if (mi->obj_entry)
      evas_del_object(m->evas, mi->obj_entry);
   mi->obj_entry = NULL;
   if (mi->obj_text)
      evas_del_object(m->evas, mi->obj_text);
   mi->obj_text = NULL;
   if (mi->obj_icon)
      evas_del_object(m->evas, mi->obj_icon);
   mi->obj_icon = NULL;
   if (mi->state)
      ebits_free(mi->state);
   mi->state = NULL;
   IF_FREE(mi->state_file);
   mi->state_file = NULL;
   if (mi->sep)
      ebits_free(mi->sep);
   mi->sep = NULL;
   IF_FREE(mi->sep_file);
   mi->sep_file = NULL;

   D_RETURN;
}

void
e_menu_item_realize(E_Menu * m, E_Menu_Item * mi)
{
   double              tw, th;
   int                 iw, ih, rw, rh;

   D_ENTER;

   if (mi->separator)
     {
	e_menu_set_sep(m, mi);
     }
   else
     {
	if (mi->str)
	  {
	     mi->obj_text = evas_add_text(m->evas, "borzoib", 8, mi->str);
	     evas_set_color(m->evas, mi->obj_text, 0, 0, 0, 255);
	     evas_show(m->evas, mi->obj_text);
	     evas_set_layer(m->evas, mi->obj_text, 10);
	  }
	if (mi->icon)
	  {
	     mi->obj_icon = evas_add_image_from_file(m->evas, mi->icon);
	     evas_show(m->evas, mi->obj_icon);
	     evas_set_layer(m->evas, mi->obj_icon, 10);
	  }
	mi->obj_entry = evas_add_rectangle(m->evas);
	evas_set_layer(m->evas, mi->obj_entry, 11);
	evas_set_color(m->evas, mi->obj_entry, 0, 0, 0, 0);
	evas_show(m->evas, mi->obj_entry);
	tw = 0;
	th = 0;
	if (mi->obj_text)
	   evas_get_geometry(m->evas, mi->obj_text, NULL, NULL, &tw, &th);
	iw = 0;
	ih = 0;
	if (mi->obj_icon)
	   evas_get_image_size(m->evas, mi->obj_icon, &iw, &ih);
	rw = 0;
	rh = 0;
	if (mi->state)
	   ebits_get_min_size(mi->state, &rw, &rh);
	mi->size.min.w = (int)tw + rw;
	if (rh > th)
	   th = (double)rh;
	if (((!mi->scale_icon) && (ih > th)) || ((!mi->str) && (ih > th)))
	   th = (double)ih;
	mi->size.min.h = (int)th;
	evas_callback_add(m->evas, mi->obj_entry, CALLBACK_MOUSE_IN,
			  e_menu_item_in_cb, mi);
	evas_callback_add(m->evas, mi->obj_entry, CALLBACK_MOUSE_OUT,
			  e_menu_item_out_cb, mi);
	e_menu_set_sel(m, mi);
	if ((mi->radio) || (mi->check))
	   e_menu_set_state(m, mi);
     }

   D_RETURN;
}

E_Menu_Item        *
e_menu_item_new(char *str)
{
   E_Menu_Item        *mi;

   D_ENTER;

   mi = NEW(E_Menu_Item, 1);
   ZERO(mi, E_Menu_Item, 1);

   if (str)
      mi->str = strdup(str);

   D_RETURN_(mi);
}

void
e_menu_obscure_outside_screen(E_Menu * m)
{
   D_ENTER;

   /* obscure stuff outside the screen boundaries - optimizes rendering */
   evas_clear_obscured_rects(m->evas);
   evas_add_obscured_rect(m->evas,
			  -m->current.x - 100000,
			  -m->current.y - 100000, 200000 + screen_w, 100000);
   evas_add_obscured_rect(m->evas,
			  -m->current.x - 100000,
			  -m->current.y - 100000, 100000, 200000 + screen_h);
   evas_add_obscured_rect(m->evas,
			  -m->current.x - 100000,
			  screen_h - m->current.y, 200000 + screen_w, 100000);
   evas_add_obscured_rect(m->evas,
			  screen_w - m->current.x,
			  -m->current.y - 100000, 100000, 200000 + screen_h);

   D_RETURN;
}

void
e_menu_scroll_all_by(int dx, int dy)
{
   Evas_List           l;

   D_ENTER;

   for (l = menus; l; l = l->next)
     {
	E_Menu             *m;

	m = l->data;
	if (m->current.visible)
	  {
	     m->current.x += dx;
	     m->current.y += dy;
	     m->changed = 1;
	  }
     }
   if (!keyboard_nav)
     {
	for (l = open_menus; l; l = l->next)
	  {
	     E_Menu             *m;

	     m = l->data;
	     evas_event_move(m->evas,
			     mouse_x - m->current.x, mouse_y - m->current.y);
	  }
     }

   D_RETURN;
}

void
e_menu_update_visibility(E_Menu * m)
{
   E_Menu_Item        *mi;

   D_ENTER;
   mi = m->selected;
   if (mi)
     {
	/* if the entry is off screen - scroll so it's on screen */
	if (m->current.x < 0)
	   e_menu_scroll_all_by(-m->current.x, 0);
	else if ((m->current.x + m->current.w) > screen_w)
	  {
	     e_menu_scroll_all_by(screen_w - (m->current.x + m->current.w), 0);
	  }
	if ((m->current.y + m->sel_border.t + mi->y) < 0)
	  {
	     if (m->current.y < -(screen_h / 4))
		e_menu_scroll_all_by(0, screen_h / 4);
	     else
		e_menu_scroll_all_by(0, -m->current.y);
	  }
	else if ((m->current.y + m->sel_border.t + mi->y + mi->size.h) >
		 screen_h)
	  {
	     if ((m->current.y + m->current.h - screen_h) < (screen_h / 4))
		e_menu_scroll_all_by(0,
				     -(m->current.y + m->current.h - screen_h));
	     else
		e_menu_scroll_all_by(0, -(screen_h / 4));
	  }
     }

   D_RETURN;
}

void
e_menu_update_base(E_Menu * m)
{
   int                 size_changed = 0;
   int                 location_changed = 0;

   D_ENTER;

   if (!m->changed)
      D_RETURN;

   if (m->recalc_entries)
     {
	Evas_List           l;
	int                 max_w, max_h;
	int                 i;

	max_w = 0;
	max_h = 0;
	for (l = m->entries; l; l = l->next)
	  {
	     E_Menu_Item        *mi;

	     mi = l->data;
	     if (mi->size.min.h > max_h)
		max_h = mi->size.min.h;
	  }
	m->size.state = 0;
	m->size.icon = 0;
	m->size.text = 0;
	for (l = m->entries; l; l = l->next)
	  {
	     E_Menu_Item        *mi;
	     int                 iw, ih, rw, rh;
	     double              tw, th;

	     mi = l->data;
	     if (!mi->separator)
	       {
		  tw = 0;
		  th = 0;
		  if (mi->obj_text)
		     evas_get_geometry(m->evas, mi->obj_text, NULL, NULL, &tw,
				       &th);
		  iw = 0;
		  ih = 0;
		  if (mi->obj_icon)
		     evas_get_image_size(m->evas, mi->obj_icon, &iw, &ih);
		  rw = 0;
		  rh = 0;
		  if (mi->state)
		     ebits_get_min_size(mi->state, &rw, &rh);
		  if (m->size.text < tw)
		     m->size.text = tw;
		  if (m->size.state < rw)
		     m->size.state = rw;
		  if ((mi->scale_icon) && (iw > 0) && (ih > 0) && (mi->str))
		    {
		       int                 iiw;

		       iiw = iw;
		       if (ih > (int)th)
			  iiw = (iw * (int)th) / ih;
		       if (m->size.icon < iiw)
			  m->size.icon = iiw;
		    }
		  else if (m->size.icon < iw)
		     m->size.icon = iw;
	       }
	  }
	max_w = m->size.state;
	if (m->size.state)
	   max_w += m->pad.state;
	max_w += m->size.icon;
	if (m->size.icon)
	   max_w += m->pad.icon;
	max_w += m->size.text;

	i = m->border.t;
	for (l = m->entries; l; l = l->next)
	  {
	     E_Menu_Item        *mi;

	     mi = l->data;
	     mi->size.w = max_w;
	     if (mi->separator)
		mi->size.h = mi->size.min.h;
	     else
		mi->size.h = max_h;
	     mi->x = m->border.l;
	     mi->y = i;
	     if (!mi->separator)
		i += m->sel_border.t + m->sel_border.b;
	     if (mi->separator)
		i += mi->size.h;
	     else
		i += max_h;
	     e_menu_item_update(m, mi);
	  }
	m->current.w =
	   m->border.l + m->border.r + max_w + m->sel_border.l +
	   m->sel_border.r;
	m->current.h = m->border.b + i;

	m->recalc_entries = 0;
     }
   if (m->redo_sel)
     {
	Evas_List           l;

	for (l = m->entries; l; l = l->next)
	  {
	     E_Menu_Item        *mi;

	     mi = l->data;
	     e_menu_set_sel(m, mi);
	     if (mi)
	       {
		  if (mi->bg)
		    {
		       ebits_move(mi->bg, mi->x, mi->y);
		       ebits_resize(mi->bg,
				    mi->size.w + m->sel_border.l +
				    m->sel_border.r,
				    mi->size.h + m->sel_border.t +
				    m->sel_border.b);
		       ebits_show(mi->bg);
		    }
	       }
	  }
	m->redo_sel = 0;
     }

   if ((m->current.x != m->previous.x) || (m->current.y != m->previous.y))
      location_changed = 1;
   if ((m->current.w != m->previous.w) || (m->current.h != m->previous.h))
      size_changed = 1;

   if ((location_changed) && (size_changed))
     {
	ecore_window_move_resize(m->win.main, m->current.x, m->current.y,
				 m->current.w, m->current.h);
	e_menu_obscure_outside_screen(m);
     }
   else if (location_changed)
     {
	ecore_window_move(m->win.main, m->current.x, m->current.y);
	e_menu_obscure_outside_screen(m);
     }
   else if (size_changed)
     {
	ecore_window_resize(m->win.main, m->current.w, m->current.h);
     }
   if (size_changed)
     {
	ecore_window_resize(m->win.evas, m->current.w, m->current.h);
	evas_set_output_size(m->evas, m->current.w, m->current.h);
	evas_set_output_viewport(m->evas, 0, 0, m->current.w, m->current.h);
	if (m->bg)
	   ebits_resize(m->bg, m->current.w, m->current.h);
     }

   D_RETURN;
}

void
e_menu_update_finish(E_Menu * m)
{
   D_ENTER;

   if (!m->changed)
      D_RETURN;
   m->previous = m->current;
   m->changed = 0;

   D_RETURN;
}

void
e_menu_update_shows(E_Menu * m)
{
   D_ENTER;

   if (!m->changed)
      D_RETURN;
   if (m->current.visible != m->previous.visible)
     {
	if (m->current.visible)
	  {
	     ecore_window_raise(m->win.main);
	     e_menu_event_win_show();
	     ecore_window_show(m->win.main);
	     if (!open_menus)
		keyboard_nav = 0;
	     open_menus = evas_list_append(open_menus, m);
	  }
     }

   D_RETURN;
}

void
e_menu_update_hides(E_Menu * m)
{
   D_ENTER;

   if (!m->changed)
      D_RETURN;
   if (m->current.visible != m->previous.visible)
     {
	if (!m->current.visible)
	  {
	     if (m->selected)
	       {
		  E_Menu_Item        *mi;

		  mi = m->selected;
		  mi->selected = 0;
		  e_menu_set_sel(m, mi);
		  if (mi)
		    {
		       if (mi->bg)
			 {
			    ebits_move(mi->bg, mi->x, mi->y);
			    ebits_resize(mi->bg,
					 mi->size.w + m->sel_border.l +
					 m->sel_border.r,
					 mi->size.h + m->sel_border.t +
					 m->sel_border.b);
			    ebits_show(mi->bg);
			 }
		    }
		  m->redo_sel = 1;
		  m->changed = 1;
		  m->selected = NULL;
	       }
	     open_menus = evas_list_remove(open_menus, m);
	     ecore_window_hide(m->win.main);
	     if (!open_menus)
		e_menu_event_win_hide();
	     if (m->func_hide)
		m->func_hide(m, m->func_hide_data);
	  }
     }

   D_RETURN;
}

void
e_menu_update(E_Menu * m)
{
   D_ENTER;

   e_menu_update_base(m);
   e_menu_update_shows(m);
   e_menu_update_hides(m);
   e_menu_update_finish(m);

   D_RETURN;
}

void
e_menu_item_set_icon(E_Menu_Item * mi, char *icon)
{
   D_ENTER;

   IF_FREE(mi->icon);
   mi->icon = NULL;
   if (icon)
      mi->icon = strdup(icon);
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_text(E_Menu_Item * mi, char *text)
{
   D_ENTER;

   IF_FREE(mi->str);
   mi->str = NULL;
   if (text)
      mi->str = strdup(text);
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_separator(E_Menu_Item * mi, int sep)
{
   D_ENTER;

   mi->separator = sep;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_radio(E_Menu_Item * mi, int radio)
{
   D_ENTER;

   mi->radio = radio;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_check(E_Menu_Item * mi, int check)
{
   D_ENTER;

   mi->check = check;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_state(E_Menu_Item * mi, int state)
{
   D_ENTER;

   mi->on = state;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->redo_sel = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_submenu(E_Menu_Item * mi, E_Menu * submenu)
{
   D_ENTER;

   if (mi->submenu)
      e_menu_hide(mi->submenu);
   mi->submenu = submenu;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->redo_sel = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_item_set_scale_icon(E_Menu_Item * mi, int scale)
{
   D_ENTER;

   mi->scale_icon = scale;
   if (mi->menu)
     {
	mi->menu->recalc_entries = 1;
	mi->menu->changed = 1;
     }

   D_RETURN;
}

void
e_menu_set_padding_icon(E_Menu * m, int pad)
{
   D_ENTER;

   m->pad.icon = pad;
   m->recalc_entries = 1;
   m->changed = 1;

   D_RETURN;
}

void
e_menu_set_padding_state(E_Menu * m, int pad)
{
   D_ENTER;

   m->pad.state = pad;
   m->recalc_entries = 1;
   m->changed = 1;

   D_RETURN;
}

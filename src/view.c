#include "e.h"

static Evas_List views = NULL;

static void e_idle(void *data);
static void e_wheel(Eevent * ev);
static void e_key_down(Eevent * ev);
static void e_key_up(Eevent * ev);
static void e_mouse_down(Eevent * ev);
static void e_mouse_up(Eevent * ev);
static void e_mouse_move(Eevent * ev);
static void e_mouse_in(Eevent * ev);
static void e_mouse_out(Eevent * ev);
static void e_window_expose(Eevent * ev);
static void e_view_handle_fs(EfsdEvent *ev);

static void
e_idle(void *data)
{
   Evas_List l;

   printf("view idle.\n");
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	e_view_update(v);
     }
   return;
   UN(data);
}

static void 
e_wheel(Eevent * ev)
{
   Ev_Wheel           *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;	
     }
}

static void
e_key_down(Eevent * ev)
{
   Ev_Key_Down          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;	
	if ((e->win == v->win.base) ||
	    (e->win == v->win.main))
	  {
	     if (!strcmp(e->key, "Up"))
	       {
	       }
	     else if (!strcmp(e->key, "Down"))
	       {
	       }
	     else if (!strcmp(e->key, "Left"))
	       {
	       }
	     else if (!strcmp(e->key, "Right"))
	       {
	       }
	     else if (!strcmp(e->key, "Escape"))
	       {
	       }
	     else
	       {
		  char *type;
		  
		  type = e_key_press_translate_into_typeable(e);
		  if (type)
		    {
		    }
	       }
	     return;
	  }
     }
}

static void
e_key_up(Eevent * ev)
{
   Ev_Key_Up          *e;
   Evas_List l;
   
   e = ev->event;
   return;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
     }
}

static void
e_mouse_down(Eevent * ev)
{
   Ev_Mouse_Down          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     /* FIXME: */
	     /* normally would handle selection in evasa object callbacks */
	     /* but for now it's handled here */
	     if (e->button == 1)
	       {
		  v->selection.on = 1;
		  v->selection.start_x = e->x;
		  v->selection.start_y = e->y;
		  v->selection.x = e->x;
		  v->selection.y = e->y;
		  v->selection.w = 1;
		  v->selection.h = 1;
		  if (!v->selection.obj_rect)
		    {
		       v->selection.obj_rect = evas_add_rectangle(v->evas);
		       v->selection.obj_l1 = evas_add_line(v->evas);
		       v->selection.obj_l2 = evas_add_line(v->evas);
		       v->selection.obj_l3 = evas_add_line(v->evas);
		       v->selection.obj_l4 = evas_add_line(v->evas);
		       evas_set_color(v->evas, v->selection.obj_rect, 255, 255, 255, 100);
		       evas_set_color(v->evas, v->selection.obj_l1, 0, 0, 0, 200);
		       evas_set_color(v->evas, v->selection.obj_l2, 0, 0, 0, 200);
		       evas_set_color(v->evas, v->selection.obj_l3, 0, 0, 0, 200);
		       evas_set_color(v->evas, v->selection.obj_l4, 0, 0, 0, 200);
		       evas_set_layer(v->evas, v->selection.obj_rect, 100);
		       evas_set_layer(v->evas, v->selection.obj_l1, 100);
		       evas_set_layer(v->evas, v->selection.obj_l2, 100);
		       evas_set_layer(v->evas, v->selection.obj_l3, 100);
		       evas_set_layer(v->evas, v->selection.obj_l4, 100);
		    }
		  e_view_update_selection(v, e->x, e->y);
	       }
	     evas_event_button_down(v->evas, e->x, e->y, e->button);
	     return;
	  }
     }
}

static void
e_mouse_up(Eevent * ev)
{
   Ev_Mouse_Up          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     /* FIXME: temporary for now- should only do this if its a deskop */
	     /* view and desktops accept focus on click. */
	     /*	e_focus_to_window(e->win); */
	     if ((v->selection.w < 6) && (v->selection.h < 6))
	       {
		  if (e->button == 1)
		    {
		       static E_Build_Menu *buildmenu = NULL;
		       
		       if (!buildmenu)
			 {
			    char *apps_menu_db;
		       
			    apps_menu_db = e_config_get("apps_menu");
			    if (apps_menu_db) buildmenu = e_build_menu_new_from_db(apps_menu_db);
			 }
		       if (buildmenu)
			 {
			    static E_Menu *menu = NULL;
			    menu = buildmenu->menu;
			    if (menu)
			      e_menu_show_at_mouse(menu, e->rx, e->ry, e->time);
			 }
		    }
		  else if (e->button == 3)
		    {
		       static E_Menu *menu = NULL;
		       
		       if (!menu)
			 {
			    E_Menu_Item *menuitem;
			    
			    menu = e_menu_new();
			    menu->pad.icon = 2;
			    menu->pad.state = 2;
			    menuitem = e_menu_item_new("Enlightenment "VERSION);
			    menuitem->icon = strdup(PACKAGE_DATA_DIR"/data/images/e_logo.png");
			    e_menu_add_item(menu, menuitem);
			 }
		       if (menu)
			 e_menu_show_at_mouse(menu, e->rx, e->ry, e->time);
		    }
	       }
	     if (e->button == 1)
	       {
		  v->selection.on = 0;
		  e_view_update_selection(v, e->x, e->y);
	       }
	     evas_event_button_up(v->evas, e->x, e->y, e->button);
	  }
     }
}

static void
e_mouse_move(Eevent * ev)
{
   Ev_Mouse_Move          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     e_view_update_selection(v, e->x, e->y);
	     evas_event_move(v->evas, e->x, e->y);
	     return;
	  }
     }
}

static void
e_mouse_in(Eevent * ev)
{
   Ev_Window_Enter          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     /* FIXME: temporary for now- should only do this if its a deskop */
	     /* view and desktops accept focus on mouse. */
	     e_focus_to_window(e->win);
	     evas_event_enter(v->evas);
	     return;
	  }
     }
}

static void
e_mouse_out(Eevent * ev)
{
   Ev_Window_Leave          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     evas_event_leave(v->evas);
	     return;
	  }
     }
}

static void
e_window_expose(Eevent * ev)
{
   Ev_Window_Expose          *e;
   Evas_List l;
   
   e = ev->event;
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (e->win == v->win.main)
	  {
	     if (!(v->pmap))
	       evas_update_rect(v->evas, e->x, e->y, e->w, e->h);
	     v->changed = 1;
	     return;
	  }
     }
}

int
e_view_filter_file(E_View *v, char *file)
{
   if (file[0] == '.') return 0;
   return 1;
}

void
e_view_update_selection(E_View *v, int x, int y)
{
   if (v->selection.on)
     {
	if (x < v->selection.start_x)
	  {
	     v->selection.w = (-(x - v->selection.start_x)) + 1;
	     v->selection.x = x;
	  }
	else
	  {
	     v->selection.w = (x - v->selection.start_x) + 1;
	     v->selection.x = v->selection.start_x;		       
	  }
	if (y < v->selection.start_y)
	  {
	     v->selection.h = (-(y - v->selection.start_y)) + 1;
	     v->selection.y = y;
		    }
	else
	  {
	     v->selection.h = (y - v->selection.start_y) + 1;
	     v->selection.y = v->selection.start_y;		       
	  }
	evas_move(v->evas, v->selection.obj_rect, v->selection.x, v->selection.y);
	evas_resize(v->evas, v->selection.obj_rect, v->selection.w, v->selection.h);
	evas_set_line_xy(v->evas, v->selection.obj_l1, v->selection.x, v->selection.y, v->selection.x + v->selection.w - 1, v->selection.y);
	evas_set_line_xy(v->evas, v->selection.obj_l2, v->selection.x, v->selection.y, v->selection.x, v->selection.y + v->selection.h - 1);
	evas_set_line_xy(v->evas, v->selection.obj_l3, v->selection.x, v->selection.y + v->selection.h - 1, v->selection.x + v->selection.w - 1, v->selection.y + v->selection.h - 1);
	evas_set_line_xy(v->evas, v->selection.obj_l4, v->selection.x + v->selection.w - 1, v->selection.y, v->selection.x + v->selection.w - 1, v->selection.y + v->selection.h - 1);
	evas_show(v->evas, v->selection.obj_rect);
	evas_show(v->evas, v->selection.obj_l1);
	evas_show(v->evas, v->selection.obj_l2);
	evas_show(v->evas, v->selection.obj_l3);
	evas_show(v->evas, v->selection.obj_l4);
     }
   else
     {
	evas_hide(v->evas, v->selection.obj_rect);
	evas_hide(v->evas, v->selection.obj_l1);
	evas_hide(v->evas, v->selection.obj_l2);
	evas_hide(v->evas, v->selection.obj_l3);
	evas_hide(v->evas, v->selection.obj_l4);
     }
}

void
e_view_file_added(int id, char *file)
{
   E_Icon *icon;
   E_View *v;
   char *realfile;
   char buf[4096];

   /* if we get a path - ignore it - its not a file in the a dir */
   printf("e_view_file_added(%i, \"%s\");\n", id, file);
   if (!file) return;
   if (file[0] == '/') return;
   v = e_view_find_by_monitor_id(id);
   if (!v) return;
   /* filter files here */
   if (!e_view_filter_file(v, file)) return;
   icon = e_icon_new();
   icon->file = strdup(file);
   icon->x = rand()%(v->size.w - 60);
   icon->y = rand()%(v->size.h - 60);
   icon->changed = 1;
   icon->visible = 1;
   icon->icon = strdup(PACKAGE_DATA_DIR"/data/icons/file/default.db:/icon/normal");
   e_view_add_icon(v, icon);
   v->changed = 1;
}

void
e_view_file_deleted(int id, char *file)
{
   E_Icon *icon;
   E_View *v;
   char *realfile;

   printf("e_view_file_deleted(%i, \"%s\");\n", id, file);
   v = e_view_find_by_monitor_id(id);
   if (!v) return;
}

static void
e_view_handle_fs(EfsdEvent *ev)
{
   switch (ev->type)
     {
      case FILECHANGE:
	switch (ev->efsd_filechange_event.changecode)
	  {
	   case FAMCreated:
	     e_view_file_added(ev->efsd_filechange_event.id, 
			       ev->efsd_filechange_event.file);
	     break;
	   case FAMExists:
	     e_view_file_added(ev->efsd_filechange_event.id, 
			       ev->efsd_filechange_event.file);
	     break;
	   case FAMDeleted:
	     e_view_file_deleted(ev->efsd_filechange_event.id, 
				 ev->efsd_filechange_event.file);
	     break;
	   case FAMChanged:
	     printf("FAMChanged: %i %s\n",
		    ev->efsd_filechange_event.id,
		    ev->efsd_filechange_event.file);	     
	     break;
	   case FAMMoved:
	     printf("FAMMoved: %i %s\n",
		    ev->efsd_filechange_event.id,
		    ev->efsd_filechange_event.file);	     
	     break;
	   case FAMEndExist:
	     printf("FAMEndExist: %i %s\n",
		    ev->efsd_filechange_event.id,
		    ev->efsd_filechange_event.file);	     
	     break;
	   default:
	     break;
	  }
	break;
      case REPLY:
	switch (ev->efsd_reply_event.command.type)
	  {
	   case REMOVE:
	     break;
	   case MOVE:
	     break;
	   case SYMLINK:
	     break;
	   case LISTDIR:
	     break;
	   case MAKEDIR:
	     break;
	   case CHMOD:
	     break;
	   case STAT:
	     break;
	   case CLOSE:
	     break;
	   case SETMETA:
	     break;
	   case GETMETA:
	     break;
	   case STARTMON:
	     printf("Startmon event %i\n",
		    ev->efsd_reply_event.command.efsd_file_cmd.id);	     
	     break;
	   case STOPMON:
	     break;
	   default:
	     break;
	  }
	break;
      default:
	break;
     }
}

E_View *
e_view_find_by_monitor_id(int id)
{
   Evas_List l;
   
   for (l = views; l; l = l->next)
     {
	E_View *v;
	
	v = l->data;
	if (v->monitor_id == id) return v;
     }
   return NULL;
}

void
e_view_free(E_View *v)
{
   views = evas_list_remove(views, v);
   FREE(v);
}

E_View *
e_view_new(void)
{
   E_View *v;
   
   v = NEW(E_View, 1);
   ZERO(v, E_View, 1);
   OBJ_INIT(v, e_view_free);

#if 1   
   /* ONLY alpha software can be "backing stored" */
   v->options.render_method = RENDER_METHOD_ALPHA_SOFTWARE;
   v->options.back_pixmap = 1;
#else
   v->options.render_method = RENDER_METHOD_BASIC_HARDWARE;
   v->options.back_pixmap = 0;
#endif
   views = evas_list_append(views, v);
   return v;   
}

void
e_view_set_background(E_View *v)
{
   v->changed = 1;
}

void
e_view_set_dir(E_View *v, char *dir)
{
   /* stop monitoring old dir */
   IF_FREE(v->dir);
   v->dir = e_file_real(dir);
   /* start monitoring new dir */
   v->monitor_id = efsd_start_monitor(e_fs_get_connection(), v->dir);
   printf("%i %s\n", v->monitor_id, v->dir);
   v->is_listing = 1;
   v->changed = 1;
}

void
e_view_scroll(E_View *v, int dx, int dy)
{
   v->changed = 1;
}

void
e_view_add_icon(E_View *v, E_Icon *icon)
{
   if (icon->view) return;
   icon->view = v;
   e_icon_realize(icon);
   v->changed = 1;
}

void
e_view_del_icon(E_View *v, E_Icon *icon)
{
   if (!icon->view) return;
   e_icon_unrealize(icon);
   icon->view = NULL;
   icon->changed = 1;
   v->changed = 1;
}

void
e_view_realize(E_View *v)
{
   int max_colors = 216;
   int font_cache = 1024 * 1024;
   int image_cache = 8192 * 1024;
   char *font_dir;
   
   if (v->evas) return;
   v->win.base = e_window_override_new(0, 
				       v->location.x, v->location.y, 
				       v->size.w, v->size.h);   
   font_dir = e_config_get("fonts");
   v->evas = evas_new_all(e_display_get(),
			  v->win.base,
			  0, 0, v->size.w, v->size.h,
			  v->options.render_method,
			  max_colors,
			  font_cache,
			  image_cache,
			  font_dir);
   v->win.main = evas_get_window(v->evas);
   e_add_child(v->win.base, v->win.main);   
   if (v->options.back_pixmap)
     {
	v->pmap = e_pixmap_new(v->win.main, v->size.w, v->size.h, 0);
	evas_set_output(v->evas, e_display_get(), v->pmap,
			evas_get_visual(v->evas), 
			evas_get_colormap(v->evas));
        e_window_set_background_pixmap(v->win.main, v->pmap);
     }
   if (v->bg)
     {
	v->bg->geom.w = v->size.w;
	v->bg->geom.h = v->size.h;
	e_background_realize(v->bg, v->evas);
     }
   e_window_set_events(v->win.main, 
		       XEV_EXPOSE | XEV_MOUSE_MOVE | 
		       XEV_BUTTON | XEV_IN_OUT | XEV_KEY);
   e_window_show(v->win.main);
     {
	char *dir;
	
	dir = v->dir;
	v->dir = NULL;
	e_view_set_dir(v, dir);
	IF_FREE(dir);
     }
   v->changed = 1;
}

void
e_view_unrealize(E_View *v)
{
   if (!v->evas) return;
}

void
e_view_update(E_View *v)
{
   if (!v->changed) return;
   
   if (v->options.back_pixmap)
     {
	Imlib_Updates up;
	
	up = evas_render_updates(v->evas);
	if (up)
	  {
	     Imlib_Updates u;
	     
	     for (u = up; u; u = imlib_updates_get_next(u))
	       {
		  int x, y, w, h;
		  
		  imlib_updates_get_coordinates(u, &x, &y, &w, &h);
		  e_window_clear_area(v->win.main, x, y, w, h);
	       }
	     imlib_updates_free(up);
	  }
     }
   else
     evas_render(v->evas);
/*   v->changed = 0;*/
}

void
e_view_init(void)
{
   e_event_filter_handler_add(EV_MOUSE_DOWN,               e_mouse_down);
   e_event_filter_handler_add(EV_MOUSE_UP,                 e_mouse_up);
   e_event_filter_handler_add(EV_MOUSE_MOVE,               e_mouse_move);
   e_event_filter_handler_add(EV_MOUSE_IN,                 e_mouse_in);
   e_event_filter_handler_add(EV_MOUSE_OUT,                e_mouse_out);
   e_event_filter_handler_add(EV_WINDOW_EXPOSE,            e_window_expose);
   e_event_filter_handler_add(EV_KEY_DOWN,                 e_key_down);
   e_event_filter_handler_add(EV_KEY_UP,                   e_key_up);
   e_event_filter_handler_add(EV_MOUSE_WHEEL,              e_wheel);
   e_event_filter_idle_handler_add(e_idle, NULL);
   e_fs_add_event_handler(e_view_handle_fs);
}

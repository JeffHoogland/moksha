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

static void
e_idle(void *data)
{
   Evas_List l;
   
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
	  }
     }
}

static void
e_key_up(Eevent * ev)
{
   Ev_Key_Up          *e;
   Evas_List l;
   
   e = ev->event;
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
	     evas_event_button_down(v->evas, e->x, e->y, e->button);
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
	     evas_event_move(v->evas, e->x, e->y);
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
	     evas_event_enter(v->evas);
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
	  }
     }
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
   
   v->options.render_method = RENDER_METHOD_ALPHA_SOFTWARE;
   v->options.back_pixmap = 1;
   
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
   IF_FREE(v->directory);
   v->directory = strdup(dir);
   /* start monitoring new dir */
   
     {
	/* bad hack- lets just add some dummy stuff for testing */
	int i;
	char *files[8] = {
	   "The first file",
	   "Some more things",
	   "K is a FISH!",
	   "Possum Eyes",
	   "Nasty Bums",
	   "BLUMFRUB!",
	   "Oh lookie here now!",
	   "Last one...."
	};
	for (i = 0; i < 8; i++)
	  {
	     E_Icon *icon;
	     
	     icon = e_icon_new();
	     icon->file = strdup(files[i]);
	     e_view_add_icon(v, icon);
	  }
     }   
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
   icon->changed = 1;
   v->changed = 1;
}

void
e_view_del_icon(E_View *v, E_Icon *icon)
{
   if (!icon->view) return;
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
   e_window_set_events(v->win.main, 
		       XEV_EXPOSE | XEV_MOUSE_MOVE | 
		       XEV_BUTTON | XEV_IN_OUT | XEV_KEY);
   e_window_show(v->win.main);
   e_view_set_dir(v, v->directory);
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
   v->changed = 0;
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
}

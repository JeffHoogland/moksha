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
	evas_render(v->evas);
     }
}

static void 
e_wheel(Eevent * ev)
{
   Ev_Wheel           *e;
   
   e = ev->event;
}

static void
e_key_down(Eevent * ev)
{
   Ev_Key_Down          *e;
   
   e = ev->event;
}

static void
e_key_up(Eevent * ev)
{
   Ev_Key_Up          *e;
   
   e = ev->event;
}

static void
e_mouse_down(Eevent * ev)
{
   Ev_Mouse_Down          *e;
   
   e = ev->event;
}

static void
e_mouse_up(Eevent * ev)
{
   Ev_Mouse_Up          *e;
   
   e = ev->event;
}

static void
e_mouse_move(Eevent * ev)
{
   Ev_Mouse_Move          *e;
   
   e = ev->event;
}

static void
e_mouse_in(Eevent * ev)
{
   Ev_Window_Enter          *e;
   
   e = ev->event;
}

static void
e_mouse_out(Eevent * ev)
{
   Ev_Window_Leave          *e;
   
   e = ev->event;
}

static void
e_window_expose(Eevent * ev)
{
   Ev_Window_Expose          *e;
   
   e = ev->event;
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
   
   
   
   views = evas_list_append(views, v);  
   return v;   
}

void
e_view_realize(void)
{
}

void
e_view_unrealize(void)
{
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

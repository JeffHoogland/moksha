/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_focus_cb_window_focus_in(void *data, int ev_type, void *ev);
static int _e_focus_cb_window_focus_out(void *data, int ev_type, void *ev);
static int _e_focus_cb_mouse_button_down(void *data, int ev_type, void *ev);
static int _e_focus_raise_timer(void* data);

/* local subsystem globals */
static Ecore_Event_Handler *_e_focus_focus_in_handler = NULL;
static Ecore_Event_Handler *_e_focus_focus_out_handler = NULL;
static Ecore_Event_Handler *_e_focus_mouse_down_handler = NULL;

/* externally accessible functions */
int
e_focus_init(void)
{
   _e_focus_focus_in_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _e_focus_cb_window_focus_in, NULL);
   _e_focus_focus_out_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _e_focus_cb_window_focus_out, NULL);
//   _e_focus_mouse_down_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_DOWN, 
//	 _e_focus_cb_mouse_button_down, NULL);


   return 1;
}

int
e_focus_shutdown(void)
{
   E_FN_DEL(ecore_event_handler_del, _e_focus_focus_in_handler);
   E_FN_DEL(ecore_event_handler_del, _e_focus_focus_out_handler);
   E_FN_DEL(ecore_event_handler_del, _e_focus_mouse_down_handler);
   return 1;
}

void
e_focus_idler_before(void)
{
   return;
}

int
e_focus_event_mouse_in(E_Border* bd)
{
   /* If focus follows mouse */
   if( e_config->focus_policy & E_FOCUS_FOLLOW_MOUSE)
     e_border_focus_set(bd, 1, 1);
   
   bd->raise_timer = NULL;
   if (e_config->focus_policy & E_FOCUS_AUTORAISE)
     {
	if (e_config->raise_timer == 0)
	  e_border_raise(bd);
	else
	  bd->raise_timer = ecore_timer_add((double)e_config->raise_timer / 10.0, 
					    _e_focus_raise_timer, bd);
     }
   return 0;
}

int
e_focus_event_mouse_out(E_Border* bd)
{
   /* If focus follows mouse */
   if (e_config->focus_policy & E_FOCUS_FOLLOW_MOUSE)
     e_border_focus_set(bd, 0, 1);

   if (bd->raise_timer != NULL)
     {
	ecore_timer_del(bd->raise_timer);
	bd->raise_timer = NULL;
     }
   return 0;
}

int
e_focus_event_mouse_down(E_Border* bd)
{
   if (!(e_config->focus_policy & E_FOCUS_FOLLOW_MOUSE))
     {
	e_border_focus_set(bd, 1, 1);
	e_border_raise(bd);
     }
   return 0;
}

int
e_focus_event_mouse_up(E_Border* bd)
{
   return 0;
}


/* local subsystem functions */
static int
_e_focus_cb_window_focus_in(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Focus_In *e;

   e = ev;
#if 0
   const char *modes[] = {
	"ECORE_X_EVENT_MODE_NORMAL",
	"ECORE_X_EVENT_MODE_WHILE_GRABBED",
	"ECORE_X_EVENT_MODE_GRAB",
	"ECORE_X_EVENT_MODE_UNGRAB"
   };
   const char *details[] = {
	"ECORE_X_EVENT_DETAIL_ANCESTOR",
	"ECORE_X_EVENT_DETAIL_VIRTUAL",
	"ECORE_X_EVENT_DETAIL_INFERIOR",
	"ECORE_X_EVENT_DETAIL_NON_LINEAR",
	"ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL",
	"ECORE_X_EVENT_DETAIL_POINTER",
	"ECORE_X_EVENT_DETAIL_POINTER_ROOT",
	"ECORE_X_EVENT_DETAIL_DETAIL_NONE"
   };

   if (e_border_find_by_client_window(e->win))
     {
	printf("BI 0x%x md=%s dt=%s\n",
	       e->win,
	       modes[e->mode],
	       details[e->detail]);
     }
   else
     {
	printf("FI 0x%x md=%s dt=%s\n",
	       e->win,
	       modes[e->mode],
	       details[e->detail]);
     }
#endif
   return 1;
}

static int
_e_focus_cb_window_focus_out(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Focus_Out *e;

   e = ev;
#if 0
   const char *modes[] = {
	"ECORE_X_EVENT_MODE_NORMAL",
	"ECORE_X_EVENT_MODE_WHILE_GRABBED",
	"ECORE_X_EVENT_MODE_GRAB",
	"ECORE_X_EVENT_MODE_UNGRAB"
   };
   const char *details[] = {
	"ECORE_X_EVENT_DETAIL_ANCESTOR",
	"ECORE_X_EVENT_DETAIL_VIRTUAL",
	"ECORE_X_EVENT_DETAIL_INFERIOR",
	"ECORE_X_EVENT_DETAIL_NON_LINEAR",
	"ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL",
	"ECORE_X_EVENT_DETAIL_POINTER",
	"ECORE_X_EVENT_DETAIL_POINTER_ROOT",
	"ECORE_X_EVENT_DETAIL_DETAIL_NONE"
   };

   if (e_border_find_by_client_window(e->win))
     {
	printf("BO 0x%x md=%s dt=%s\n",
	       e->win,
	       modes[e->mode],
	       details[e->detail]);
     }
   else
     {
	printf("FO 0x%x md=%s dt=%s\n",
	       e->win,
	       modes[e->mode],
	       details[e->detail]);
     }
#endif
   return 1;
}


static int
_e_focus_cb_mouse_button_down(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Mouse_Button_Down* e;
   E_Border* bd = NULL;

   e = ev;

   fprintf(stderr,"%s, %p\n",__FUNCTION__, e->win);

   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     bd = e_border_find_by_window(e->win);
   if (!bd)
     bd = e_border_find_by_frame_window(e->win);

   if (!bd)
     bd = e_border_find_by_client_window(e->event_win);
   if (!bd)
     bd = e_border_find_by_window(e->event_win);
   if (!bd)
     bd = e_border_find_by_frame_window(e->event_win);

   fprintf(stderr,"bd = %p\n", bd);

   if (bd)
     e_focus_event_mouse_down(bd);

   return 1;
}
     
static int
_e_focus_raise_timer(void* data)
{
   e_border_raise((E_Border*)data);
   ((E_Border*)data)->raise_timer = NULL;
   return 0;
}


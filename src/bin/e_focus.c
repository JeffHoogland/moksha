#include "e.h"

/* local subsystem functions */
static int _e_focus_cb_idle(void *data);
static int _e_focus_cb_window_focus_in(void *data, int ev_type, void *ev);
static int _e_focus_cb_window_focus_out(void *data, int ev_type, void *ev);

/* local subsystem globals */
static Ecore_Event_Handler *_e_focus_focus_in_handler = NULL;
static Ecore_Event_Handler *_e_focus_focus_out_handler = NULL;

/* externally accessible functions */
int
e_focus_init(void)
{
   _e_focus_focus_in_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _e_focus_cb_window_focus_in, NULL);
   _e_focus_focus_out_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _e_focus_cb_window_focus_out, NULL);
   return 1;
}

int
e_focus_shutdown(void)
{
   E_FN_DEL(ecore_event_handler_del, _e_focus_focus_in_handler);
   E_FN_DEL(ecore_event_handler_del, _e_focus_focus_out_handler);
   return 1;
}

void
e_focus_idler_before(void)
{
   return;
}



/* local subsystem functions */
static int
_e_focus_cb_window_focus_in(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Focus_In *e;
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
   
   e = ev;
   printf("FI 0x%x md=%s dt=%s\n", 
	  e->win,
	  modes[e->mode],
	  details[e->detail]);
   return 1;
}

static int
_e_focus_cb_window_focus_out(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Focus_Out *e;
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
   
   e = ev;
   printf("FO 0x%x md=%s dt=%s\n", 
	  e->win,
	  modes[e->mode],
	  details[e->detail]);
   return 1;
}

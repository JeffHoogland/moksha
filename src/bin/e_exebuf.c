/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

static int _e_exebuf_cb_key_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_up(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_wheel(void *data, int type, void *event);

/* local subsystem globals */
static E_Popup *exebuf = NULL;
static Evas_Object *bg_object = NULL;
static Evas_List *handlers = NULL;
static Ecore_X_Window input_window = 0;

/* externally accessible functions */
int
e_exebuf_init(void)
{
   return 1;
}

int
e_exebuf_shutdown(void)
{
   e_exebuf_hide();
   return 1;
}

int
e_exebuf_show(E_Zone *zone)
{
   Evas_Object *o;
   int x, y, w, h;
   
   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);
   
   if (exebuf) return 0;

   input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
   ecore_x_window_show(input_window);
   e_grabinput_get(input_window, 0, input_window);

   x = zone->x + 20;
   y = zone->y + 20 + ((zone->h - 20 - 20 - 20) / 2);
   w = zone->w - 20 - 20;
   h = 20;
   
   exebuf = e_popup_new(zone, x, y, w, h); 
   if (!exebuf) return 0;
   e_popup_layer_set(exebuf, 255);
   evas_event_freeze(exebuf->evas);
   o = edje_object_add(exebuf->evas);
   bg_object = o;
   e_theme_edje_object_set(o, "base/theme/exebuf",
			   "widgets/exebuf/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(exebuf, o);

   evas_event_thaw(exebuf->evas);

   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_KEY_DOWN, _e_exebuf_cb_key_down, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_exebuf_cb_mouse_down, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_exebuf_cb_mouse_up, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_WHEEL, _e_exebuf_cb_mouse_wheel, NULL));
   
   e_popup_show(exebuf);
   return 1;
}

void
e_exebuf_hide(void)
{
   if (!exebuf) return;
   
   evas_event_freeze(exebuf->evas);
   e_popup_hide(exebuf);
   evas_object_del(bg_object);
   bg_object = NULL;
   evas_event_thaw(exebuf->evas);
   e_object_del(E_OBJECT(exebuf));
   exebuf = NULL;
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = evas_list_remove_list(handlers, handlers);
     }
   ecore_x_window_del(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;
}

/* local subsystem functions */

static int
_e_exebuf_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   if      (!strcmp(ev->keysymbol, "Up"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "Down"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "Left"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "Right"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "Return"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "Escape"))
     e_exebuf_hide();
   return 1;
}

static int
_e_exebuf_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   return 1;
}

static int
_e_exebuf_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   return 1;
}

static int
_e_exebuf_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   if (ev->z < 0) /* up */
     {
	int i;
	
	for (i = ev->z; i < 0; i++) e_exebuf_hide();
     }
   else if (ev->z > 0) /* down */
     {
	int i;
	
	for (i = ev->z; i > 0; i--) e_exebuf_hide();
     }
   return 1;
}

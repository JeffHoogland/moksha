/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Evas_List *event_handlers = NULL;
static Evas_List *dnd_handlers = NULL;

static Ecore_X_Window drag_win;
static Ecore_Evas  *drag_ee = NULL;
static Evas_Object *drag_obj;

static char *drag_type;
static void *drag_data;

static int  _e_dnd_cb_mouse_up(void *data, int type, void *event);
static int  _e_dnd_cb_mouse_move(void *data, int type, void *event);

int
e_dnd_init(void)
{
   Evas_List *l, *l2;
   E_Manager *man;
   E_Container *con;

   event_handlers = evas_list_append(event_handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_dnd_cb_mouse_up, NULL));
   event_handlers = evas_list_append(event_handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE, _e_dnd_cb_mouse_move, NULL));

   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;

	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     con = l2->data;

#if 0
	     e_hints_window_visible_set(con->bg_win);
	     ecore_x_dnd_aware_set(con->bg_win, 1);
	     ecore_event_handler_add(ECORE_X_EVENT_XDND_DROP, _e_dnd_cb_event_dnd_drop , eb);
	     ecore_event_handler_add(ECORE_X_EVENT_XDND_POSITION, _e_dnd_cb_event_dnd_position, eb);
	     ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY, _e_dnd_cb_event_dnd_selection, eb);
#endif
	  }
     }
   return 1;
}

int
e_dnd_shutdown(void)
{
   Evas_List *l;
   
   for (l = event_handlers; l; l = l->next)
     {
	Ecore_Event_Handler *h;

	h = l->data;
	ecore_event_handler_del(h);
     }
   evas_list_free(event_handlers);
   event_handlers = NULL;

   for (l = dnd_handlers; l; l = l->next)
     {
	E_DND_Handler *h;

	h = l->data;
	e_dnd_handler_del(h);
     }
   evas_list_free(dnd_handlers);
   dnd_handlers = NULL;

   return 1;
}

void
e_drag_start(E_Zone *zone, const char *type, void *data, const char *icon_path, const char *icon)
{
   int w, h;

   drag_win = ecore_x_window_input_new(zone->container->win, 
				       zone->x, zone->y,
				       zone->w, zone->h);
   ecore_x_window_show(drag_win);
   ecore_x_pointer_confine_grab(drag_win);
   ecore_x_keyboard_grab(drag_win);

   if (drag_ee)
     {
	e_canvas_del(drag_ee);
	ecore_evas_free(drag_ee);
     }

   drag_ee = ecore_evas_software_x11_new(NULL, zone->container->manager->win,
					 0, 0, 10, 10);
   ecore_evas_override_set(drag_ee, 1);
   ecore_evas_software_x11_direct_resize_set(drag_ee, 1);
   ecore_evas_shaped_set(drag_ee, 1);
   e_canvas_add(drag_ee);
   ecore_evas_borderless_set(drag_ee, 1);
   ecore_evas_layer_set(drag_ee, 255);

   drag_obj = edje_object_add(ecore_evas_get(drag_ee));
   edje_object_file_set(drag_obj, icon_path, icon);

   w = h = 24;
   evas_object_move(drag_obj, 0, 0);
   evas_object_resize(drag_obj, w, h);
   
   ecore_evas_resize(drag_ee, w, h);

   drag_type = strdup(type);
   drag_data = data;
}

void
e_drag_update(int x, int y)
{
   int w, h;

   if (!drag_ee) return;
   
   evas_object_geometry_get(drag_obj, NULL, NULL, &w, &h);
   evas_object_show(drag_obj);
   ecore_evas_show(drag_ee);

   ecore_evas_move(drag_ee, x - (w / 2), y - (h / 2));
}

void
e_drag_end(int x, int y)
{
   Evas_List *l;

   printf("drag_end\n");

   evas_object_del(drag_obj);
   if (drag_ee)
     {
	e_canvas_del(drag_ee);
	ecore_evas_free(drag_ee);
	drag_ee = NULL;
     }
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_window_del(drag_win);

   for (l = dnd_handlers; l; l = l->next)
     {
	E_DND_Handler *h;

	h = l->data;
	
	printf("pos: %d %d\n", x, y);
	printf("pos: %d %d %d %d", h->x, h->y, h->w, h->y);
	printf("data: %s %s\n", h->type, drag_type);
	if ((x >= h->x) && (x < h->x + h->w) && (y >= h->y) && (y < h->y + h->h)
	    && (!strcmp(h->type, drag_type)))
	  {
	     h->func(h->data, drag_type, drag_data);
	  }
     }

   free(drag_type);
}

E_DND_Handler *
e_dnd_handler_add(void *data, void (*func)(void *data, const char *type, void *drop), const char *type, int x, int y, int w, int h)
{
   E_DND_Handler *handler;

   handler = E_NEW(E_DND_Handler, 1);
   if (!handler) return NULL;

   handler->data = data;
   handler->func = func;
   handler->type = strdup(type);
   handler->x = x;
   handler->y = y;
   handler->w = w;
   handler->h = h;

   dnd_handlers = evas_list_append(dnd_handlers, handler);

   return handler;
}

void
e_dnd_handler_del(E_DND_Handler *handler)
{
   free(handler->type);
   free(handler);
}

static int
_e_dnd_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;

   ev = event;
   if (ev->win != drag_win) return 1;

   e_drag_end(ev->x, ev->y);

   return 1;
}

static int
_e_dnd_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   if (ev->win != drag_win) return 1;

   e_drag_update(ev->x, ev->y);
   return 1;
}


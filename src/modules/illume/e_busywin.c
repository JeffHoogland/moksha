#include "e.h"
#include "e_busywin.h"
#include "e_cfg.h"

/* internal calls */

E_Busywin *_e_busywin_new(E_Zone *zone, const char *themedir);
static void _e_busywin_free(E_Busywin *esw);
static int _e_busywin_cb_animate(void *data);
static void _e_busywin_slide(E_Busywin *esw, int out, double len);
static int _e_busywin_cb_mouse_up(void *data, int type, void *event);
static int _e_busywin_cb_zone_move_resize(void *data, int type, void *event);
static void _e_busywin_cb_item_sel(void *data, void *data2);

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

/* state */
static Eina_List *busywins = NULL;

/* called from the module core */
EAPI int
e_busywin_init(void)
{
   return 1;
}

EAPI int
e_busywin_shutdown(void)
{
   return 1;
}

EAPI E_Busywin *
e_busywin_new(E_Zone *zone, const char *themedir)
{
   E_Busywin *esw;
   Evas_Coord mw, mh;
   int x, y;
   Evas_Object *o;
     
   esw = E_OBJECT_ALLOC(E_Busywin, E_BUSYWIN_TYPE, _e_busywin_free);
   if (!esw) return NULL;
   
   esw->zone = zone;
   if (themedir) esw->themedir = evas_stringshare_add(themedir);
   
   esw->clickwin = ecore_x_window_input_new(zone->container->win,
					    zone->x, zone->y, zone->w, zone->h);
   esw->popup = e_popup_new(esw->zone, -1, -1, 1, 1);
   ecore_x_window_configure(esw->clickwin, 
			    ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING|ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
			    0, 0, 0, 0, 0,
			    esw->popup->evas_win,
			    ECORE_X_WINDOW_STACK_BELOW);
   e_popup_layer_set(esw->popup, 250);
   
   esw->base_obj = _theme_obj_new(esw->popup->evas,
				  esw->themedir,
				  "e/modules/busywin/base/default");

   edje_object_size_min_calc(esw->base_obj, &mw, &mh);
   
   x = zone->x;
   y = zone->y + zone->h;
   mw = zone->w;
   
   e_popup_move_resize(esw->popup, x, y, mw, mh);

   evas_object_resize(esw->base_obj, esw->popup->w, esw->popup->h);
   e_popup_edje_bg_object_set(esw->popup, esw->base_obj);
   evas_object_show(esw->base_obj);
   
   e_popup_show(esw->popup);

   busywins = eina_list_append(busywins, esw);

   esw->handlers = eina_list_append
     (esw->handlers,
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
			      _e_busywin_cb_mouse_up, esw));
   esw->handlers = eina_list_append
     (esw->handlers,
      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
			      _e_busywin_cb_zone_move_resize, esw));
   
   return esw;
}

EAPI E_Busywin_Handle *
e_busywin_push(E_Busywin *esw, const char *message, const char *icon)
{
   E_Busywin_Handle *h;
   
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK_RETURN(esw, E_BUSYWIN_TYPE, NULL);
   h = calloc(1, sizeof(E_Busywin_Handle));
   h->busywin = esw;
   if (message) h->message = evas_stringshare_add(message);
   if (icon) h->icon = evas_stringshare_add(icon);
   esw->handles = eina_list_prepend(esw->handles, h);
   edje_object_part_text_set(esw->base_obj, "e.text.label", h->message);
   /* FIXME: handle icon... */
   _e_busywin_slide(esw, 1, (double)illume_cfg->sliding.busywin.duration / 1000.0);
   return h;
}

EAPI void
e_busywin_pop(E_Busywin *esw, E_Busywin_Handle *handle)
{
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_BUSYWIN_TYPE);
   if (!eina_list_data_find(esw->handles, handle)) return;
   esw->handles = eina_list_remove(esw->handles, handle);
   if (handle->message) evas_stringshare_del(handle->message);
   if (handle->icon) evas_stringshare_del(handle->icon);
   free(handle);
   if (esw->handles)
     {
	handle = esw->handles->data;
	edje_object_part_text_set(esw->base_obj, "e.text.label", 
				  handle->message);
     }
   else
     {
	_e_busywin_slide(esw, 0, (double)illume_cfg->sliding.busywin.duration / 1000.0);
     }
}


/* internal calls */
static void
_e_busywin_free(E_Busywin *esw)
{
   busywins = eina_list_remove(busywins, esw);
   while (esw->handlers)
     {
	if (esw->handlers->data)
	  ecore_event_handler_del(esw->handlers->data);
	esw->handlers = eina_list_remove_list(esw->handlers, esw->handlers);
     }
   if (esw->animator) ecore_animator_del(esw->animator);
   if (esw->themedir) evas_stringshare_del(esw->themedir);
   ecore_x_window_free(esw->clickwin);
   e_object_del(E_OBJECT(esw->popup));
   free(esw);
}

static int
_e_busywin_cb_animate(void *data)
{
   E_Busywin *esw;
   double t, v;
   
   esw = data;
   t = ecore_loop_time_get() - esw->start;
   if (t > esw->len) t = esw->len;
   if (esw->len > 0.0)
     {
	v = t / esw->len;
	v = 1.0 - v;
	v = v * v * v * v;
	v = 1.0 - v;
     }
   else
     {
	t = esw->len;
	v = 1.0;
     }
   esw->adjust = (esw->adjust_target * v) + (esw->adjust_start  * (1.0 - v));
   e_popup_move(esw->popup, 
		esw->zone->x, 
		esw->zone->y + esw->zone->h - esw->adjust);
   if (t == esw->len)
     {
	esw->animator = NULL;
	if (esw->out)
	  edje_object_signal_emit(esw->base_obj, "e,state,out,end", "e");
	else
	  edje_object_signal_emit(esw->base_obj, "e,state,in,end", "e");
	return 0;
     }
   return 1;
}

static void
_e_busywin_slide(E_Busywin *esw, int out, double len)
{
   if (out == esw->out) return;
   esw->start = ecore_loop_time_get();
   esw->len = len;
   esw->out = out;
   esw->adjust_start = esw->adjust;
   if (esw->out) esw->adjust_target = esw->popup->h;
   else esw->adjust_target = 0;
   if (esw->out)
     {
	edje_object_signal_emit(esw->base_obj, "e,state,out,begin", "e");
	ecore_x_window_configure(esw->clickwin, 
				 ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING|ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				 0, 0, 0, 0, 0,
				 esw->popup->evas_win,
				 ECORE_X_WINDOW_STACK_BELOW);
//	ecore_x_window_show(esw->clickwin);
     }
   else
     {
	edje_object_signal_emit(esw->base_obj, "e,state,in,begin", "e");
//	ecore_x_window_hide(esw->clickwin);
     }
   if (len <= 0.0)
     {
	_e_busywin_cb_animate(esw);
	return;
     }
   if (!esw->animator)
     esw->animator = ecore_animator_add(_e_busywin_cb_animate, esw);
}

static int
_e_busywin_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Busywin *esw;
   
   ev = event;
   esw = data;
   if (ev->window == esw->clickwin)
     {
	if (esw->out) _e_busywin_slide(esw, 0, (double)illume_cfg->sliding.busywin.duration / 1000.0);
	else _e_busywin_slide(esw, 1, (double)illume_cfg->sliding.busywin.duration / 1000.0);
     }
   return 1;
}

static int
_e_busywin_cb_zone_move_resize(void *data, int type, void *event)
{
   E_Event_Zone_Move_Resize *ev;
   E_Busywin *esw;
   
   ev = event;
   esw = data;
   if (esw->zone == ev->zone)
     {
	e_popup_move_resize(esw->popup, 
			    esw->zone->x, 
			    esw->zone->y + esw->zone->h - esw->adjust,
			    esw->zone->w, esw->popup->h);
	evas_object_resize(esw->base_obj, esw->popup->w, esw->popup->h);
     }
   return 1;
}






static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}


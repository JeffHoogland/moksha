#include <e.h>
#include "e_slipwin.h"

EAPI int E_EVENT_SLIPWIN_DEL = 0;

/* internal calls */

E_Slipwin *_e_slipwin_new(E_Zone *zone, const char *themedir);
static void _e_slipwin_free(E_Slipwin *ess);
static int _e_slipwin_cb_animate(void *data);
static void _e_slipwin_slide(E_Slipwin *ess, int out, double len);
static int _e_slipwin_cb_mouse_up(void *data, int type, void *event);
static int _e_slipwin_cb_zone_move_resize(void *data, int type, void *event);
static int _e_slipwin_cb_zone_del(void *data, int type, void *event);
static void _e_slipwin_event_simple_free(void *data, void *ev);
static void _e_slipwin_object_del_attach(void *o);
static void _e_slipwin_cb_item_sel(void *data, void *data2);

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

/* state */
static Eina_List *slipwins = NULL;

/* called from the module core */
EAPI int
e_slipwin_init(void)
{
   E_EVENT_SLIPWIN_DEL = ecore_event_type_new();
   return 1;
}

EAPI int
e_slipwin_shutdown(void)
{
   return 1;
}

EAPI E_Slipwin *
e_slipwin_new(E_Zone *zone, const char *themedir)
{
   E_Slipwin *esw;
   Evas_Coord mw, mh;
   int x, y;
   Evas_Object *o;
     
   esw = E_OBJECT_ALLOC(E_Slipwin, E_SLIPWIN_TYPE, _e_slipwin_free);
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
   e_popup_layer_set(esw->popup, 220);
   
   esw->base_obj = _theme_obj_new(esw->popup->evas,
				  esw->themedir,
				  "e/modules/slipwin/base/default");

   esw->focused_border = e_border_focused_get();

   o = e_scrollframe_add(esw->popup->evas);
   edje_object_part_swallow(esw->base_obj, "e.swallow.content", o);
   evas_object_show(o);
   esw->scrollframe_obj = o;
      
   edje_object_size_min_calc(esw->base_obj, &mw, &mh);
   
   x = zone->x;
   y = zone->y + zone->h;
   mw = zone->w;
   
   mh = 300;
   
   e_popup_move_resize(esw->popup, x, y, mw, mh);

   evas_object_resize(esw->base_obj, esw->popup->w, esw->popup->h);
   e_popup_edje_bg_object_set(esw->popup, esw->base_obj);
   evas_object_show(esw->base_obj);
   
   e_popup_show(esw->popup);

   slipwins = eina_list_append(slipwins, esw);

   esw->handlers = eina_list_append
     (esw->handlers,
      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
			      _e_slipwin_cb_mouse_up, esw));
   
   e_object_del_attach_func_set(E_OBJECT(esw), _e_slipwin_object_del_attach);
   
   return esw;
}

EAPI void
e_slipwin_show(E_Slipwin *esw)
{
   Evas_Object *o;
   Evas_Coord mw, mh, vw, vh, w, h;
   Eina_List *borders, *l;
   int i, selnum;
   
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_SLIPWIN_TYPE);

   /* FIXME: build window list and free old onw if needed */
   if (esw->ilist_obj)
     {
	evas_object_del(esw->ilist_obj);
	esw->ilist_obj = NULL;
     }
   while (esw->borders)
     {
	e_object_unref(E_OBJECT(esw->borders->data));
	esw->borders = eina_list_remove_list(esw->borders, esw->borders);
     }
   
   o = e_ilist_add(esw->popup->evas);
   e_ilist_selector_set(o, 1);
   e_ilist_freeze(o);
   
   borders = e_border_client_list();
   i = 0;
   selnum = -1;
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	const char *title;
	
	bd = l->data;
	if (e_object_is_del(E_OBJECT(bd))) continue;
	if ((!bd->client.icccm.accepts_focus) &&
	    (!bd->client.icccm.take_focus)) continue;
	if (bd->client.netwm.state.skip_taskbar) continue;
	if (bd->user_skip_winlist) continue;
	
	e_object_ref(E_OBJECT(bd));
	title = "???";
	if (bd->client.netwm.name) title = bd->client.netwm.name;
	else if (bd->client.icccm.title) title = bd->client.icccm.title;
	e_ilist_append(o, NULL/*icon*/, title, 0, _e_slipwin_cb_item_sel,
		       NULL, esw, bd);
	esw->borders = eina_list_append(esw->borders, bd);
	if (bd == e_border_focused_get()) selnum = i;
	i++;
     }
   e_ilist_thaw(o);
   
   e_ilist_min_size_get(o, &mw, &mh);
   
   evas_object_resize(o, mw, mh);
   e_scrollframe_child_set(esw->scrollframe_obj, o);
   
   e_scrollframe_child_viewport_size_get(esw->scrollframe_obj, &vw, &vh);
   edje_object_part_geometry_get(esw->scrollframe_obj, "e.swallow.content", NULL, NULL, &vw, &vh);
//   evas_object_geometry_get(esw->scrollframe_obj, NULL, NULL, &w, &h);
   if (mw > vw) mw = mw + (w - vw);
   else if (mw < vw) evas_object_resize(o, vw, mh);
   
   if (selnum >= 0) e_ilist_selected_set(o, selnum);

   evas_object_show(o);
   esw->ilist_obj = o;
   
   edje_extern_object_min_size_set(esw->scrollframe_obj, mw, mh);
   printf("min size %ix%i\n", mw, mh);
   edje_object_part_swallow(esw->base_obj, "e.swallow.content", esw->scrollframe_obj);
   edje_object_size_min_calc(esw->base_obj, &mw, &mh);
   
   edje_extern_object_min_size_set(esw->scrollframe_obj, 0, 0);
   edje_object_part_swallow(esw->base_obj, "e.swallow.content", esw->scrollframe_obj);
   
   mw = esw->zone->w;
   if (mh > esw->zone->h) mh = esw->zone->h;
   e_popup_resize(esw->popup, mw, mh);

   evas_object_resize(esw->base_obj, esw->popup->w, esw->popup->h);
   
   printf("sw: %ix%i\n", esw->popup->w, esw->popup->h);
   
   _e_slipwin_slide(esw, 1, 1.0);
}

EAPI void
e_slipwin_hide(E_Slipwin *esw)
{
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_SLIPWIN_TYPE);

   _e_slipwin_slide(esw, 0, 1.0);
}

EAPI void
e_slipwin_border_select_callback_set(E_Slipwin *esw, void (*func) (void *data, E_Slipwin *ess, E_Border *bd), const void *data)
{
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_SLIPWIN_TYPE);
   esw->callback.func = func;
   esw->callback.data = data;
}

/* internal calls */
static void
_e_slipwin_free(E_Slipwin *esw)
{
   slipwins = eina_list_remove(slipwins, esw);
   while (esw->handlers)
     {
	if (esw->handlers->data)
	  ecore_event_handler_del(esw->handlers->data);
	esw->handlers = eina_list_remove_list(esw->handlers, esw->handlers);
     }
   if (esw->animator) ecore_animator_del(esw->animator);
   if (esw->themedir) evas_stringshare_del(esw->themedir);
   ecore_x_window_del(esw->clickwin);
   e_object_del(E_OBJECT(esw->popup));
   free(esw);
}

static int
_e_slipwin_cb_animate(void *data)
{
   E_Slipwin *esw;
   double t, v;
   
   esw = data;
   t = ecore_time_get() - esw->start;
   if (t > esw->len) t = esw->len;
   v = t / esw->len;
   v = 1.0 - v;
   v = v * v * v * v;
   v = 1.0 - v;
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
	  {
	     edje_object_signal_emit(esw->base_obj, "e,state,in,end", "e");
	     if (esw->ilist_obj)
	       {
		  evas_object_del(esw->ilist_obj);
		  esw->ilist_obj = NULL;
	       }
	     while (esw->borders)
	       {
		  e_object_unref(E_OBJECT(esw->borders->data));
		  esw->borders = eina_list_remove_list(esw->borders, esw->borders);
	       }
	  }
	return 0;
     }
   return 1;
}

static void
_e_slipwin_slide(E_Slipwin *esw, int out, double len)
{
   if (out == esw->out) return;
   if (len == 0.0) len = 0.000001; /* FIXME: handle len = 0.0 case */
   
   if (!esw->animator)
     esw->animator = ecore_animator_add(_e_slipwin_cb_animate, esw);
   esw->start = ecore_time_get();
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
	ecore_x_window_show(esw->clickwin);
     }
   else
     {
	edje_object_signal_emit(esw->base_obj, "e,state,in,begin", "e");
	ecore_x_window_hide(esw->clickwin);
     }
}

static int
_e_slipwin_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   E_Slipwin *esw;
   
   ev = event;
   esw = data;
   if (ev->win == esw->clickwin)
     {
	if (esw->out) _e_slipwin_slide(esw, 0, 1.0);
	else _e_slipwin_slide(esw, 1, 1.0);
     }
   return 1;
}

static int
_e_slipwin_cb_zone_move_resize(void *data, int type, void *event)
{
   E_Event_Zone_Move_Resize *ev;
   E_Slipwin *esw;
   
   ev = event;
   esw = data;
   if (esw->zone == ev->zone)
     {
	/* FIXME: handle new size pants */
     }
   return 1;
}

static int
_e_slipwin_cb_zone_del(void *data, int type, void *event)
{
   E_Event_Zone_Del *ev;
   E_Slipwin *esw;
   
   ev = event;
   esw = data;
   if (esw->zone == ev->zone)
     {
	e_object_del(E_OBJECT(esw));
     }
   return 1;
}
				       
static void
_e_slipwin_event_simple_free(void *data, void *ev)
{
   struct _E_Event_Slipwin_Simple *e;
   
   e = ev;
   e_object_unref(E_OBJECT(e->slipwin));
   free(e);
}

static void
_e_slipwin_object_del_attach(void *o)
{
   E_Slipwin *esw;
   E_Event_Slipwin_Del *ev;

   if (e_object_is_del(E_OBJECT(o))) return;
   esw = o;
   ev = calloc(1, sizeof(E_Event_Slipwin_Del));
   ev->slipwin = esw;
   e_object_ref(E_OBJECT(esw));
   ecore_event_add(E_EVENT_SLIPWIN_DEL, ev, 
		   _e_slipwin_event_simple_free, NULL);
}

static void
_e_slipwin_cb_item_sel(void *data, void *data2)
{
   E_Slipwin *esw;
   
   esw = data;
   if (esw->callback.func) esw->callback.func(esw->callback.data, esw, data2);
   e_slipwin_hide(esw);
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


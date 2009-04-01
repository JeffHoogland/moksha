/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_simplelock.h"

/**************************** private data ******************************/
typedef struct _E_Simplelock_Data	      E_Simplelock_Data;

struct _E_Simplelock_Data
{
   E_Popup        *popup;
   Evas_Object    *base_obj;
   Ecore_X_Window  win;
   E_Zone         *zone;
};

static void _e_action_simplelock_cb(E_Object *obj, const char *params);
static int _e_simplelock_cb_key_down(void *data, int type, void *event);
static int _e_simplelock_cb_key_up(void *data, int type, void *event);

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

static E_Module       *mod = NULL;

static Eina_List      *locks = NULL;
static Ecore_X_Window  grab_win;
static Eina_List      *handlers = NULL;

/***********************************************************************/
static void
_e_action_simplelock_cb(E_Object *obj, const char *params)
{
   if (locks)
     e_simplelock_hide();
   else
     e_simplelock_show();
}

static int
_e_simplelock_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;
   E_Action *act;
   Eina_List *l;
   E_Config_Binding_Key *bind;
   E_Binding_Modifier mod;
      
   ev = event;
   if (ev->event_window != grab_win) return 1;
   for (l = e_config->key_bindings; l; l = l->next)
     {
	bind = l->data;
	
	if (bind->action && strcmp(bind->action,"simple_lock")) continue;
	
	mod = 0;
	
	if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
	
	if (bind->key && (!strcmp(bind->key, ev->keyname)) && 
	    ((bind->modifiers == mod) || (bind->any_mod)))
	  {
	     act = e_action_find(bind->action);
	     
	     if (!act) continue;
	     e_simplelock_hide();
	  }
     }
   return 0;
}

static int
_e_simplelock_cb_key_up(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;
      
   ev = event;
   if (ev->event_window != grab_win) return 1;
   return 0;
}

static int
_e_simplelock_cb_zone_move_resize(void *data, int type, void *event)
{
   E_Event_Zone_Move_Resize *ev;
   Eina_List *l;
      
   ev = event;
   for (l = locks; l; l = l->next)
     {
	E_Simplelock_Data *esl;
	
	esl = l->data;
	if (esl->zone == ev->zone)
	  {
	     Evas_Coord minw, minh, mw, mh, ww, hh;
	     
	     edje_object_size_min_get(esl->base_obj, &minw, &minh);
	     edje_object_size_min_calc(esl->base_obj, &mw, &mh);
	     ww = esl->zone->w;
	     if (minw == 1) ww = mw;
	     hh = esl->zone->h;
	     if (minh == 1) hh = mh;
	     e_popup_move_resize(esl->popup, 
				 esl->zone->x + ((esl->zone->w - ww) / 2), 
				 esl->zone->y + ((esl->zone->h - hh) / 2), 
				 ww, hh);
	     evas_object_resize(esl->base_obj, esl->popup->w, esl->popup->h);
	     ecore_x_window_move_resize(esl->win,
					esl->zone->x, esl->zone->y,
					esl->zone->w, esl->zone->h);
	  }
     }
   return 1;
}

/***********************************************************************/

EAPI int
e_simplelock_init(E_Module *m)
{
   E_Action *act;

   mod = m;
   
   act = e_action_add("simple_lock");
   if (act)
     {
	act->func.go = _e_action_simplelock_cb;
	e_action_predef_name_set("Desktop", "Desktop Simple Lock",
				 "simple_lock", NULL, NULL, 0);
     }
   return 1;
}

EAPI int
e_simplelock_shutdown(void)
{
   e_simplelock_hide();
   e_action_predef_name_del("Desktop", "Simple Lock");
   e_action_del("simple_lock");
   return 1;
}

EAPI int
e_simplelock_show(void)
{
   Eina_List	     *managers, *l, *l2, *l3;

   if (locks) return 1;
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  E_Simplelock_Data *esl;
		  E_Zone *zone;
		  Evas_Coord mw, mh, minw = 0, minh = 0;
		  int ww, hh;
		  
		  zone = l3->data;
		  esl = E_NEW(E_Simplelock_Data, 1);
		  esl->zone = zone;
		  esl->win = ecore_x_window_input_new(zone->container->win,
						      zone->x, zone->y,
						      zone->w, zone->h);
		  ecore_x_window_show(esl->win);
		  if (!grab_win) grab_win = esl->win;
		  esl->popup = e_popup_new(zone, -1, -1, 1, 1);
		  e_popup_layer_set(esl->popup, 250);
		  esl->base_obj = _theme_obj_new(esl->popup->evas,
						 e_module_dir_get(mod),
						 "e/modules/simplelock/base/default");
		  edje_object_size_min_get(esl->base_obj, &minw, &minh);
		  edje_object_part_text_set(esl->base_obj, 
					    "e.text.label", "LOCKED");
		  edje_object_size_min_calc(esl->base_obj, &mw, &mh);
		  ww = zone->w;
		  if (minw == 1) ww = mw;
		  hh = zone->h;
		  if (minh == 1) hh = mh;
		  e_popup_move_resize(esl->popup, 
				      zone->x + ((zone->w - ww) / 2), 
				      zone->y + ((zone->h - hh) / 2), 
				      ww, hh);
		  evas_object_resize(esl->base_obj, esl->popup->w, esl->popup->h);
		  e_popup_edje_bg_object_set(esl->popup, esl->base_obj);
		  evas_object_show(esl->base_obj);
		  
		  e_popup_show(esl->popup);
		  
		  locks = eina_list_append(locks, esl);
	       }
	  }
     }
   if (!e_grabinput_get(grab_win, 0, grab_win))
     {
	e_simplelock_hide();
	return 0;
     }
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _e_simplelock_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_UP, _e_simplelock_cb_key_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_ZONE_MOVE_RESIZE, _e_simplelock_cb_zone_move_resize, NULL));
   return 1;
}

EAPI void
e_simplelock_hide(void)
{
   Ecore_Event_Handler *handle;

   if (!locks) return;
   e_grabinput_release(grab_win, grab_win);
   while (locks)
     {
	E_Simplelock_Data *esl;
	
	esl = locks->data;
	e_object_del(E_OBJECT(esl->popup));
	ecore_x_window_del(esl->win);
	free(esl);
	locks = eina_list_remove_list(locks, locks);
     }
   grab_win = 0;

   EINA_LIST_FREE(handlers, handle)
     ecore_event_handler_del(handle);
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

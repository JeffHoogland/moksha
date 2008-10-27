#include "e.h"
#include "e_flaunch.h"
#include "e_cfg.h"

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

static void
_e_flaunch_cb_button_select(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Flaunch_App *fla;
   
   fla = data;
   if (fla->callback.func) fla->callback.func((void *)fla->callback.data);
}

static E_Flaunch_App *
_e_fluanch_button_add(E_Flaunch *fl, const char *label, int expander, void (*func) (void *data), const void *data)
{
   E_Flaunch_App *fla;
   Evas_Object *o;

   fla = E_NEW(E_Flaunch_App, 1);
   if (!fla) return NULL;
   if (expander)
     o =  _theme_obj_new(fl->zone->container->bg_evas,
			 fl->themedir,
			 "e/modules/flaunch/button/default");
   else
     o =  _theme_obj_new(fl->zone->container->bg_evas,
			 fl->themedir,
			 "e/modules/flaunch/button/start");
   edje_object_part_text_set(o, "e.text.label", label);
   fla->flaunch = fl;
   fla->obj = o;
   fla->callback.func = func;
   if (!data) data = fla;
   fla->callback.data = data;
   edje_object_signal_callback_add(o, "e,action,do,select", "", 
				   _e_flaunch_cb_button_select, fla);
   return fla;
}

static void
_e_flaunch_button_del(E_Flaunch_App *fla)
{
   evas_object_del(fla->obj);
   if (fla->desktop) evas_stringshare_del(fla->desktop);
   free(fla);
}

static void
_e_fluanch_cb_app_button(void *data)
{
   E_Flaunch_App *fla;
   Efreet_Desktop *desktop;
   
   fla = data;
   if (!fla->desktop) return;
   desktop = efreet_util_desktop_file_id_find(fla->desktop);
   if (!desktop) return;
   if (fla->flaunch->desktop_run_func) fla->flaunch->desktop_run_func(desktop);
}

static E_Flaunch_App *
_e_flaunch_app_add(E_Flaunch *fl, const char *deskfile)
{
   E_Flaunch_App *fla;
   Efreet_Desktop *desktop;
   const char *label = "";
   
   desktop = efreet_util_desktop_file_id_find(deskfile);
   if (desktop) label = desktop->name;
   fla = _e_fluanch_button_add(fl, label, 1, _e_fluanch_cb_app_button, NULL);
   if (deskfile) fla->desktop = evas_stringshare_add(deskfile);
   fl->apps = eina_list_append(fl->apps, fla);
   e_box_pack_end(fl->app_box_obj, fla->obj);
   e_box_pack_options_set(fla->obj, 1, 1, 1, 1, 0.5, 0.5, 0, 0, 9999, 9999);
   evas_object_show(fla->obj);
   return fla;
}

static void
_e_flaunch_apps_clear(E_Flaunch *fl)
{
   while (fl->apps)
     {
	_e_flaunch_button_del(fl->apps->data);
	fl->apps = eina_list_remove_list(fl->apps, fl->apps);
     }
}

static void
_e_flaunch_apps_populate(E_Flaunch *fl)
{
   E_Flaunch_App *fla;
   Ecore_List *bar_desktops;
   int num = 0, max, count;
   
   // FIXME: 3 should become config here
   max = 3;
   // for now just look for any apps in "category" 'Bar' and add the first 3
   // FIXME: category should be configurable... 
   bar_desktops = efreet_util_desktop_category_list("Bar");
   if (bar_desktops)
     {
	Efreet_Desktop *desktop;
	
	count = ecore_list_count(bar_desktops);
	if (count < max)
	  {
	     int i;
	     
	     for (i = 0; i < (max - count) / 2; i++)
	       {
		  _e_flaunch_app_add(fl, "");
		  num++;
	       }
	  }
	ecore_list_first_goto(bar_desktops);
	while ((desktop = ecore_list_next(bar_desktops)))
	  {
	     if (desktop->orig_path)
	       {
		  const char *dname;
		  
		  dname = ecore_file_file_get(desktop->orig_path);
		  if (dname)
		    {
		       _e_flaunch_app_add(fl, dname);
		       num++;
		    }
	       }
	     if (num >= max) break;
	  }
     }
   while (num < max)
     {
	_e_flaunch_app_add(fl, "");
	num++;
     }
}

static void
_e_fluanch_cb_home_button(void *data)
{
   printf("FIXME: this 'start' button has no defined behavior\n");
}

static void
_e_flaunch_free(E_Flaunch *fl)
{
   if (fl->repopulate_timer) ecore_timer_del(fl->repopulate_timer);
   _e_flaunch_apps_clear(fl);
   _e_flaunch_button_del(fl->start_button);
   evas_stringshare_del(fl->themedir);
   evas_object_del(fl->app_box_obj);
   evas_object_del(fl->box_obj);
   while (fl->handlers)
     {
	ecore_event_handler_del(fl->handlers->data);
	fl->handlers = eina_list_remove_list(fl->handlers, fl->handlers);
     }
   free(fl);
}

static int
_e_flaunch_cb_zone_move_resize(void *data, int type, void *event)
{       
   E_Event_Zone_Move_Resize *ev;
   E_Flaunch *fl;
   
   ev = event;
   fl = data;
   if (fl->zone == ev->zone)
     {
	evas_object_move(fl->box_obj, fl->zone->x, fl->zone->y + fl->zone->h - fl->height);
	evas_object_resize(fl->box_obj, fl->zone->w, fl->height);
     }
   return 1;
}

static int
_e_flaunch_cb_delayed_repopulate(void *data)
{
   E_Flaunch *fl;
   
   fl = data;
   _e_flaunch_apps_clear(fl);
   _e_flaunch_apps_populate(fl);
   fl->repopulate_timer = NULL;
   return 0;
}

static int
_e_flaunch_cb_desktop_list_change(void *data, int type, void *event)
{       
   E_Flaunch *fl;
   
   fl = data;
   if (fl->repopulate_timer) ecore_timer_del(fl->repopulate_timer);
   fl->repopulate_timer = ecore_timer_add(0.5, _e_flaunch_cb_delayed_repopulate, fl);
   return 1;
}

static int
_e_flaunch_cb_desktop_change(void *data, int type, void *event)
{       
   E_Flaunch *fl;
   
   fl = data;
   if (fl->repopulate_timer) ecore_timer_del(fl->repopulate_timer);
   fl->repopulate_timer = ecore_timer_add(0.5, _e_flaunch_cb_delayed_repopulate, fl);
   return 1;
}

EAPI int
e_flaunch_init(void)
{
   return 1;
}

EAPI int
e_flaunch_shutdown(void)
{
   return 1;
}

EAPI E_Flaunch *
e_flaunch_new(E_Zone *zone, const char *themedir)
{
   E_Flaunch *fl;
   Evas_Object *o;
   
   fl = E_OBJECT_ALLOC(E_Flaunch, E_FLAUNCH_TYPE, _e_flaunch_free);
   if (!fl) return NULL;
   fl->zone = zone;
   
   fl->themedir = evas_stringshare_add(themedir);

   // FIXME: height should become config
   fl->height = 30 * e_scale;
   
   o = e_box_add(fl->zone->container->bg_evas);
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 0);
   fl->box_obj = o;
     
   fl->start_button = _e_fluanch_button_add(fl, "*", 0, 
					    _e_fluanch_cb_home_button, fl);
   e_box_pack_end(fl->box_obj, fl->start_button->obj);
   e_box_pack_options_set(fl->start_button->obj, 1, 1, 0, 1, 0.5, 0.5, fl->height, 0, fl->height, 9999);
   evas_object_show(fl->start_button->obj);
   
   o = e_box_add(fl->zone->container->bg_evas);
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   fl->app_box_obj = o;

   e_box_pack_end(fl->box_obj, fl->app_box_obj);
   e_box_pack_options_set(fl->app_box_obj, 1, 1, 1, 1, 0.5, 0.5, 0, 0, 9999, 9999);
   
   _e_flaunch_apps_populate(fl);
   
   evas_object_move(fl->box_obj, fl->zone->x, fl->zone->y + fl->zone->h - fl->height);
   evas_object_resize(fl->box_obj, fl->zone->w, fl->height);
   evas_object_show(fl->app_box_obj);
   evas_object_layer_set(fl->box_obj, 10);
   evas_object_show(fl->box_obj);

   fl->handlers = eina_list_append
     (fl->handlers, ecore_event_handler_add
      (E_EVENT_ZONE_MOVE_RESIZE, _e_flaunch_cb_zone_move_resize, fl));
   fl->handlers = eina_list_append
     (fl->handlers, ecore_event_handler_add
      (EFREET_EVENT_DESKTOP_LIST_CHANGE, _e_flaunch_cb_desktop_list_change, fl));
   fl->handlers = eina_list_append
     (fl->handlers, ecore_event_handler_add
      (EFREET_EVENT_DESKTOP_CHANGE, _e_flaunch_cb_desktop_change, fl));
   
   return fl;
}

EAPI void
e_flaunch_desktop_exec_callabck_set(E_Flaunch *fl, void (*func) (Efreet_Desktop *desktop))
{
   fl->desktop_run_func = func;
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

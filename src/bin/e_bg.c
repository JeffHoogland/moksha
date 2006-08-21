/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_bg_signal(void *data, Evas_Object *obj, const char *emission, const char *source);

/* local subsystem globals */

/* externally accessible functions */
EAPI void
e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition)
{
   Evas_Object *o;
   Evas_List *l, *ll, *entries;
   int ok;
   const char *bgfile = "";
   const char *trans = "";
   
   if (transition == E_BG_TRANSITION_START) trans = e_config->transition_start;
   else if (transition == E_BG_TRANSITION_DESK) trans = e_config->transition_desk;
   else if (transition == E_BG_TRANSITION_CHANGE) trans = e_config->transition_change;
   if ((!trans) || (strlen(trans) < 1)) transition = E_BG_TRANSITION_NONE;

   ok = 0;
   for (l = e_config->desktop_backgrounds; l; l = l->next)
     {
	E_Config_Desktop_Background *cfbg;
	E_Desk *desk;
	
	cfbg = l->data;
	if ((cfbg->container >= 0) && (zone->container->num != cfbg->container)) continue;
	if ((cfbg->zone >= 0) && (zone->num != cfbg->zone)) continue;
	desk = e_desk_current_get(zone);
	if (!desk) continue;
	if ((cfbg->desk_x >= 0) && (cfbg->desk_x != desk->x)) continue;
	if ((cfbg->desk_y >= 0) && (cfbg->desk_y != desk->y)) continue;
	entries = edje_file_collection_list(cfbg->file);
	if (entries)
	  {
	     for (ll = entries; ll; ll = ll->next)
	       {
		  if (!strcmp(ll->data, "desktop/background"))
		    {
		       bgfile = cfbg->file;
		       ok = 1;
		       break;
		    }
	       }
	     edje_file_collection_list_free(entries);
	  }
	break;
     }
   if (!ok)
     {
	entries = edje_file_collection_list(e_config->desktop_default_background);
	if (entries)
	  {
	     for (ll = entries; ll; ll = ll->next)
	       {
		  if (!strcmp(ll->data, "desktop/background"))
		    {
		       bgfile = e_config->desktop_default_background;
		       ok = 1;
		       break;
		    }
	       }
	     edje_file_collection_list_free(entries);
	  }
	if (!ok)
	  {
	     bgfile = e_theme_edje_file_get("base/theme/background", "desktop/background");
	  }
     }
   if (zone->bg_object)
     {
	const char *pfile = "";
	
	edje_object_file_get(zone->bg_object, &pfile, NULL);
	if (!e_util_strcmp(pfile, bgfile))
	  return;
     }
   
   if (transition == E_BG_TRANSITION_NONE)
     {
	if (zone->bg_object)
	  {
	     evas_object_del(zone->bg_object);
	     zone->bg_object = NULL;
	  }
     }
   else
     {
	char buf[4096];
	
	if (zone->bg_object)
	  {
	     if (zone->prev_bg_object)
	       evas_object_del(zone->prev_bg_object);
	     zone->prev_bg_object = zone->bg_object;
	     if (zone->transition_object)
	       evas_object_del(zone->transition_object);
	     zone->transition_object = NULL;
	     zone->bg_object = NULL;
	  }
	o = edje_object_add(zone->container->bg_evas);
	zone->transition_object = o;
	evas_object_data_set(o, "e_zone", zone);
	snprintf(buf, sizeof(buf), "transitions/%s", trans);
	e_theme_edje_object_set(o, "base/theme/transitions", buf);
	edje_object_signal_callback_add(o, "done", "*", _e_bg_signal, zone);
	evas_object_move(o, zone->x, zone->y);
	evas_object_resize(o, zone->w, zone->h);
	evas_object_layer_set(o, -1);
	evas_object_clip_set(o, zone->bg_clip_object);
	evas_object_show(o);
     }
   o = edje_object_add(zone->container->bg_evas);
   zone->bg_object = o;
   evas_object_data_set(o, "e_zone", zone);
   evas_object_move(o, zone->x, zone->y);
   evas_object_resize(o, zone->w, zone->h);
   edje_object_file_set(o, bgfile, "desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, zone->bg_clip_object);
   evas_object_show(o);
   
   if (transition != E_BG_TRANSITION_NONE)
     {
	edje_object_part_swallow(zone->transition_object, "bg_prev",
				 zone->prev_bg_object);
	edje_object_part_swallow(zone->transition_object, "bg_new",
				 zone->bg_object);
	edje_object_signal_emit(zone->transition_object, "go", "");
     }
}

EAPI void
e_bg_add(int container, int zone, int desk_x, int desk_y, char *file)
{
   E_Config_Desktop_Background *cfbg;
   
   e_bg_del(container, zone, desk_x, desk_y);
   cfbg = E_NEW(E_Config_Desktop_Background, 1);
   cfbg->container = container;
   cfbg->zone = zone;
   cfbg->desk_x = desk_x;
   cfbg->desk_y = desk_y;
   cfbg->file = evas_stringshare_add(file);
   e_config->desktop_backgrounds = evas_list_append(e_config->desktop_backgrounds, cfbg);
}

EAPI void
e_bg_del(int container, int zone, int desk_x, int desk_y)
{
   Evas_List *l;
   
   for (l = e_config->desktop_backgrounds; l; l = l->next)
     {
	E_Config_Desktop_Background *cfbg;
	
	cfbg = l->data;
	if ((cfbg->container == container) && (cfbg->zone == zone) &&
	    (cfbg->desk_x == desk_x) && (cfbg->desk_y == desk_y))
	  {
	     e_config->desktop_backgrounds = evas_list_remove_list(e_config->desktop_backgrounds, l);
	     if (cfbg->file) evas_stringshare_del(cfbg->file);
	     free(cfbg);
	     break;
	  }
     }
}

EAPI void
e_bg_update(void)
{
   Evas_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   
   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     for (lll = con->zones; lll; lll = lll->next)
	       {
		  zone = lll->data;
		  e_zone_bg_reconfigure(zone);
	       }
	  }
     }
}

/* local subsystem functions */

static void
_e_bg_signal(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Zone *zone;
   
   zone = data;

   if (zone->prev_bg_object)
     {
	evas_object_del(zone->prev_bg_object);
	zone->prev_bg_object = NULL;
     }
   if (zone->transition_object)
     {
	evas_object_del(zone->transition_object);
	zone->transition_object = NULL;
     }
   evas_object_move(zone->bg_object, zone->x, zone->y);
   evas_object_resize(zone->bg_object, zone->w, zone->h);
   evas_object_layer_set(zone->bg_object, -1);
   evas_object_clip_set(zone->bg_object, zone->bg_clip_object);
   evas_object_show(zone->bg_object);
}

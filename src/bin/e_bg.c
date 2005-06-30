/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_bg_signal(void *data, Evas_Object *obj, const char *emission, const char *source);

/* local subsystem globals */

/* externally accessible functions */
void
e_bg_zone_update(E_Zone *zone, E_Bg_Transition transition)
{
   Evas_Object *o;
   Evas_List *l;
   int ok;
   char *trans = "";
   
   if (transition == E_BG_TRANSITION_START) trans = e_config->transition_start;
   else if (transition == E_BG_TRANSITION_DESK) trans = e_config->transition_desk;
   else if (transition == E_BG_TRANSITION_CHANGE) trans = e_config->transition_change;
   if (strlen(trans) < 1) transition = E_BG_TRANSITION_NONE;
   
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

   ok = 0;
   for (l = e_config->desktop_backgrounds; l; l = l->next)
     {
	E_Config_Desktop_Background *cfbg;
	E_Desk *desk;
	
	cfbg = l->data;
	if ((cfbg->container >= 0) &&
	    (zone->container->num != cfbg->container)) continue;
	if ((cfbg->zone >= 0) &&
	    (zone->num != cfbg->zone)) continue;
	if ((!cfbg->desk) || (strlen(cfbg->desk) == 0)) continue;
	desk = e_desk_current_get(zone);
	if (!desk) continue;
	if (strcmp(cfbg->desk, desk->name)) continue;
	ok = edje_object_file_set(o, cfbg->file,
				  "desktop/background");
	break;
     }
   if (!ok)
     {
	if (!edje_object_file_set(o, e_config->desktop_default_background,
				  "desktop/background"))
	  e_theme_edje_object_set(o, "base/theme/background",
				  "desktop/background");
     }

   evas_object_layer_set(o, -1);
   evas_object_clip_set(o, zone->bg_clip_object);
   evas_object_show(o);
   
   if (zone->prev_bg_object)
     {
	const char *pfile =  "", *pgroup = "", *file = "", *group = "";
	
	edje_object_file_get(zone->prev_bg_object, &pfile, &pgroup);
	edje_object_file_get(zone->bg_object, &file, &group);
	if ((pfile) && (file) && (!strcmp(pfile, file)) &&
	    (pgroup) && (group) && (!strcmp(pgroup, group)))
	  {
	     evas_object_del(zone->prev_bg_object);
	     zone->prev_bg_object = NULL;
	     return;
	  }
     }
   
   if (transition != E_BG_TRANSITION_NONE)
     {
	edje_object_part_swallow(zone->transition_object, "bg_prev",
				 zone->prev_bg_object);
	edje_object_part_swallow(zone->transition_object, "bg_new",
				 zone->bg_object);
	edje_object_signal_emit(zone->transition_object, "go", "");
     }
}

void
e_bg_add(int container, int zone, char *desk, char *file)
{
   E_Config_Desktop_Background *cfbg;
   
   e_bg_del(container, zone, desk);
   cfbg = E_NEW(E_Config_Desktop_Background, 1);
   cfbg->container = container;
   cfbg->zone = zone;
   cfbg->desk = strdup(desk);
   cfbg->file = strdup(file);
   e_config->desktop_backgrounds = evas_list_append(e_config->desktop_backgrounds, cfbg);
}

void
e_bg_del(int container, int zone, char *desk)
{
   Evas_List *l;
   
   for (l = e_config->desktop_backgrounds; l; l = l->next)
     {
	E_Config_Desktop_Background *cfbg;
	
	cfbg = l->data;
	if ((cfbg->container == container) && (cfbg->zone == zone) &&
	    (!strcmp(cfbg->desk, desk)))
	  {
	     e_config->desktop_backgrounds = evas_list_remove_list(e_config->desktop_backgrounds, l);
	     IF_FREE(cfbg->desk);
	     IF_FREE(cfbg->file);
	     free(cfbg);
	     break;
	  }
     }
}

void
e_bg_update(void)
{
   Evas_List *l, *ll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   
   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     zone = e_zone_current_get(con);
	     e_zone_bg_reconfigure(zone);
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

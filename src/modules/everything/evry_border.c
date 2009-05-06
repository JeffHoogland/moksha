#include "e.h"
#include "e_mod_main.h"

static Evry_Plugin plugin;
static Evry_Action act_close;

static int  _evry_border_fetch(char *string);
static int  _evry_border_action(Evry_Item *item);
static void _evry_border_cleanup(void);
static void _evry_border_item_add(E_Border *bd, int prio);
static int  _evry_border_cb_sort(const void *data1, const void *data2);
static void _evry_border_item_icon_get(Evry_Item *it, Evas *e);

EAPI int
evry_border_init(void)
{
   plugin.name = "Windows";
   plugin.type = "BORDER";
   plugin.need_query = 0;
   plugin.fetch  = &_evry_border_fetch;
   plugin.action = &_evry_border_action;
   plugin.cleanup = &_evry_border_cleanup;
   plugin.icon_get = &_evry_border_item_icon_get;
   plugin.candidates = NULL;   
   evry_plugin_add(&plugin);

   /* act_close.name = "Close";
    * act_close.type = "BORDER";
    * evry_action_add(&act_close); */
   
   return 1;
}

EAPI int
evry_border_shutdown(void)
{
   evry_plugin_remove(&plugin);
   /* evry_action_remove(&act_close); */
   
   return 1;
}

static int
_evry_border_action(Evry_Item *item)
{
   E_Border *bd;
   E_Zone *zone;
   
   bd = (E_Border *)item->data;
   zone = e_util_zone_current_get(e_manager_current_get());
   
   if (bd->desk != (e_desk_current_get(zone)))
     e_desk_show(bd->desk);
   
   if (bd->shaded)
     e_border_unshade(bd, E_DIRECTION_UP);

   if (bd->iconic)
     e_border_uniconify(bd);
   else
     e_border_raise(bd);

   /* e_border_focus_set(bd, 1, 1); */
   e_border_focus_set_with_pointer(bd);
   
   return 1;
}

static void
_evry_border_cleanup(void)
{
   Evry_Item *it;

   EINA_LIST_FREE(plugin.candidates, it)
     {
	if (it->data) e_object_unref(E_OBJECT(it->data));
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static int
_evry_border_fetch(char *string)
{
   E_Manager *man;
   E_Zone *zone;
   
   char match1[4096];
   char match2[4096];
   Eina_List *list;
   E_Border *bd;
   E_Border_List *bl;

   _evry_border_cleanup(); 

   man = e_manager_current_get();
   zone = e_util_zone_current_get(man);
   
   if (string)
     {
	snprintf(match1, sizeof(match1), "%s*", string);
	snprintf(match2, sizeof(match2), "*%s*", string);
     }
   
   bl = e_container_border_list_first(e_container_current_get(man));
   while ((bd = e_container_border_list_next(bl)))
     {
	if (zone == bd->zone)
	  {
	     if (!string)
	       _evry_border_item_add(bd, 1);
	     else if (bd->client.icccm.name && e_util_glob_case_match(bd->client.icccm.name, match1))
	       _evry_border_item_add(bd, 1);
	     else  if (e_util_glob_case_match(e_border_name_get(bd), match1))
	       _evry_border_item_add(bd, 1);
	     else if (bd->client.icccm.name && e_util_glob_case_match(bd->client.icccm.name, match2))
	       _evry_border_item_add(bd, 2);
	     else if (e_util_glob_case_match(e_border_name_get(bd), match2))
	       _evry_border_item_add(bd, 2);
	  }
     }
   e_container_border_list_free(bl);

   if (eina_list_count(plugin.candidates) > 0)
     {
	plugin.candidates = eina_list_sort(plugin.candidates,
					   eina_list_count(plugin.candidates),
					   _evry_border_cb_sort);
	return 1;
     }

   return 0;
}

static void
_evry_border_item_icon_get( Evry_Item *it, Evas *e)
{ 
   it->o_icon = e_border_icon_add(((E_Border *)it->data), e); 
}

static void
_evry_border_item_add(E_Border *bd, int prio)
{
   Evry_Item *it;   

   it = calloc(1, sizeof(Evry_Item));
   e_object_ref(E_OBJECT(bd));
   it->data = bd;
   it->priority = prio;
   it->label = eina_stringshare_add(e_border_name_get(bd));
   it->o_icon = NULL; //e_border_icon_add(bd, evry_evas_get()); 
	     
   plugin.candidates = eina_list_append(plugin.candidates, it);
}


// TODO sort by focus history and name?
static int
_evry_border_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}

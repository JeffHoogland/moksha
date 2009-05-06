#include "e.h"
#include "e_mod_main.h"

static Evry_Plugin plugin;

static int  _evry_plug_config_fetch(char *string);
static int  _evry_plug_config_action(Evry_Item *item);
static void _evry_plug_config_cleanup(void);
static void _evry_plug_config_item_add(E_Configure_It *eci, int prio);
static int  _evry_plug_config_cb_sort(const void *data1, const void *data2);
static void _evry_plug_config_item_icon_get(Evry_Item *it, Evas *e);

EAPI int
evry_plug_config_init(void)
{
   plugin.name = "Settings";
   plugin.fetch  = &_evry_plug_config_fetch;
   plugin.action = &_evry_plug_config_action;
   plugin.cleanup = &_evry_plug_config_cleanup;
   plugin.icon_get = &_evry_plug_config_item_icon_get;
   plugin.candidates = NULL;   
   evry_plugin_add(&plugin);
   
   return 1;
}

EAPI int
evry_plug_config_shutdown(void)
{
   evry_plugin_remove(&plugin);
   
   return 1;
}

static int
_evry_plug_config_action(Evry_Item *item)
{
   E_Configure_It *eci, *eci2;
   E_Container *con;
   E_Configure_Cat *ecat;
   Eina_List *l, *ll;
   char buf[1024];
   int found = 0;
   
   eci = item->data;
   con = e_container_current_get(e_manager_current_get());

   for (l = e_configure_registry; l; l = l->next)
     {
	ecat = l->data;
	for (ll = ecat->items; ll; ll = ll->next)
	  {
	     eci2 = ll->data;
	     if (eci == eci2)
	       {
		  found = 1;
		  snprintf(buf, sizeof(buf), "%s/%s",
			   ecat->cat, 
			   eci->item);
	       }
	  }
     }

   printf("path: %s\n", buf);
   
   
   if (found)
     e_configure_registry_call(buf, con, NULL);   
   
   return 1;
}

static void
_evry_plug_config_cleanup(void)
{
   Evry_Item *it;

   EINA_LIST_FREE(plugin.candidates, it)
     {
	/* if (it->data)  */
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static int
_evry_plug_config_fetch(char *string)
{
   E_Manager *man;
   E_Zone *zone;
   
   char match1[4096];
   char match2[4096];
   Eina_List *l, *ll;
   E_Configure_Cat *ecat;
   E_Configure_It *eci;
   
   _evry_plug_config_cleanup(); 

   snprintf(match1, sizeof(match1), "%s*", string);
   snprintf(match2, sizeof(match2), "*%s*", string);

   for (l = e_configure_registry; l; l = l->next)
     {
	ecat = l->data;
	if ((ecat->pri >= 0) && (ecat->items))
	  {
	     for (ll = ecat->items; ll; ll = ll->next)
	       {
		  eci = ll->data;
		  if (eci->pri >= 0)
		    {
		       if (e_util_glob_case_match(eci->label, match1))
			 _evry_plug_config_item_add(eci, 1);
		       else if (e_util_glob_case_match(eci->label, match2))
			 _evry_plug_config_item_add(eci, 2);
		       else if (e_util_glob_case_match(ecat->label, match1))
			 _evry_plug_config_item_add(eci, 3);
		       else if (e_util_glob_case_match(ecat->label, match2))
			 _evry_plug_config_item_add(eci, 4);
		    }
	       }
	  }
     }
   
   if (eina_list_count(plugin.candidates) > 0)
     {
	plugin.candidates =
	  eina_list_sort(plugin.candidates,
			 eina_list_count(plugin.candidates),
			 _evry_plug_config_cb_sort);
	return 1;
     }

   return 0;
}

static void
_evry_plug_config_item_icon_get(Evry_Item *it, Evas *e)
{
   E_Configure_It *eci = it->data;
   Evas_Object *o;
   
   if (eci->icon) 
     {
	o = e_icon_add(e);
	if (!e_util_icon_theme_set(o, eci->icon))
	  {
	     evas_object_del(o);
	     o = e_util_icon_add(eci->icon, e);
	  }
     }

   it->o_icon = o;
}

static void
_evry_plug_config_item_add(E_Configure_It *eci, int prio)
{
   Evry_Item *it;   

   it = calloc(1, sizeof(Evry_Item));
   it->data = eci;
   it->priority = prio;
   it->label = eina_stringshare_add(eci->label);
   it->o_icon = NULL; 
	     
   plugin.candidates = eina_list_append(plugin.candidates, it);
}


// TODO sort name?
static int
_evry_plug_config_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}

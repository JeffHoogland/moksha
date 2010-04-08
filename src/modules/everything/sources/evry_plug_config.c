#include "Evry.h"

static Evry_Plugin *p;
static Evry_Action *act;


static void
_cleanup(Evry_Plugin *p)
{
   EVRY_PLUGIN_ITEMS_FREE(p);
}

static void
_item_add(Evry_Plugin *p, E_Configure_It *eci, int match, int prio)
{
   Evry_Item *it;

   it = evry_item_new(NULL, p, eci->label, NULL);
   it->data = eci;
   it->priority = prio;
   it->fuzzy_match = match;

   EVRY_PLUGIN_ITEM_APPEND(p, it);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Eina_List *l, *ll;
   E_Configure_Cat *ecat;
   E_Configure_It *eci;
   int match;

   _cleanup(p);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	if ((ecat->pri < 0) || (!ecat->items)) continue;
	if (!strcmp(ecat->cat, "system")) continue;

	EINA_LIST_FOREACH(ecat->items, ll, eci)
	  {
	     if (eci->pri >= 0)
	       {
		  if ((match = evry_fuzzy_match(eci->label, input)))
		    _item_add(p, eci, match, 0);
		  else if ((match = evry_fuzzy_match(ecat->label, input)))
		    _item_add(p, eci, match, 1);
	       }
	  }
     }

   if (eina_list_count(p->items) > 0)
     {
	p->items = evry_fuzzy_match_sort(p->items);
	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   E_Configure_It *eci = it->data;

   if (eci->icon)
     {
	if (!(o = evry_icon_theme_get(eci->icon, e)))
	  {
	     o = e_util_icon_add(eci->icon, e);
	  }
     }

   return o;
}

static int
_action(Evry_Action *act)
{
   E_Configure_It *eci, *eci2;
   E_Container *con;
   E_Configure_Cat *ecat;
   Eina_List *l, *ll;
   char buf[1024];
   int found = 0;

   eci = act->item1->data;
   con = e_container_current_get(e_manager_current_get());

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	if (found) break;

	EINA_LIST_FOREACH(ecat->items, ll, eci2)
	  {
	     if (eci == eci2)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s",
			   ecat->cat,
			   eci->item);
		  found = 1;
		  break;
	       }
	  }
     }

   if (found)
     e_configure_registry_call(buf, con, NULL);

   return EVRY_ACTION_FINISHED;
}

static Eina_Bool
_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p = evry_plugin_new(NULL, "Settings", type_subject, NULL, "E_SETTINGS", 0, NULL, NULL,
		       NULL, _cleanup, _fetch, NULL, _item_icon_get, NULL, NULL);

   act = evry_action_new("Show Dialog", "E_SETTINGS", NULL, NULL, "preferences-advanced",
			 _action, NULL, NULL, NULL, NULL);

   evry_plugin_register(p, 10);

   evry_action_register(act, 0);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   EVRY_PLUGIN_FREE(p);

   evry_action_free(act);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

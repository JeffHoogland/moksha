/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

typedef struct _Settings_Item Settings_Item;

struct _Settings_Item
{
  Evry_Item base;
  
  E_Configure_Cat *ecat;
  E_Configure_It *eci;  
};

static Evry_Plugin *p;
static Evry_Action *act;
static Eina_List *items = NULL;


static void
_finish(Evry_Plugin *p)
{
   Evry_Item *it;
   
   EVRY_PLUGIN_ITEMS_CLEAR(p);
   
   EINA_LIST_FREE(items, it)
     evry_item_free(it);
}

static Evas_Object *
_icon_get(Evry_Item *item, Evas *e)
{
   Evas_Object *o;
   Settings_Item *it = (Settings_Item *) item;
   
   if (it->eci->icon &&
       ((o = evry_icon_theme_get(it->eci->icon, e)) ||
	(o = e_util_icon_add(it->eci->icon, e))))
     return o;

   return NULL;
}

static Evry_Plugin *
_begin(Evry_Plugin *p, const Evry_Item *item __UNUSED__)
{
   Settings_Item *it;
   Eina_List *l, *ll;
   E_Configure_Cat *ecat;
   E_Configure_It *eci;

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	if ((ecat->pri < 0) || (!ecat->items)) continue;
	if (!strcmp(ecat->cat, "system")) continue;

	EINA_LIST_FOREACH(ecat->items, ll, eci)
	  {
	     if (eci->pri < 0) continue;

	     it = EVRY_ITEM_NEW(Settings_Item, p, eci->label, _icon_get, NULL);
	     it->eci = eci;
	     it->ecat = ecat;
	     EVRY_ITEM_DETAIL_SET(it, ecat->label);
   
	     items = eina_list_append(items, it);
	  }
     }
   return p;
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (!input) return 0;
   
   return EVRY_PLUGIN_ITEMS_ADD(p, items, input, 1, 1);
}

static int
_action(Evry_Action *act)
{
   char buf[1024];
   Settings_Item *it;

   it = (Settings_Item *) act->it1.item;

   snprintf(buf, sizeof(buf), "%s/%s", it->ecat->cat, it->eci->item);

   e_configure_registry_call(buf, e_container_current_get(e_manager_current_get()), NULL);

   return EVRY_ACTION_FINISHED;
}

static Eina_Bool
_plugins_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Settings"), "preferences-desktop",
		       evry_type_register("E_SETTINGS"),
		       _begin, _finish, _fetch, NULL);

   evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 10);

   act = EVRY_ACTION_NEW(N_("Show Dialog"),
			 evry_type_register("E_SETTINGS"), 0,
			 "preferences-advanced", _action, NULL);

   evry_action_register(act, 0);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(p);

   evry_action_free(act);
}


/***************************************************************************/

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "everything-settings"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   module = m;

   if (e_datastore_get("everything_loaded"))
     active = _plugins_init();

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   module = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/***************************************************************************/

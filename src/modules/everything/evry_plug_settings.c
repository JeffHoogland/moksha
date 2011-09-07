#include "e.h"
#include "evry_api.h"

typedef struct _Plugin Plugin;
typedef struct _Settings_Item Settings_Item;

struct _Plugin
{
  Evry_Plugin base;
  Eina_List  *items;
  Eina_List  *categories;
  Eina_Bool   parent;
};

struct _Settings_Item
{
  Evry_Item base;
  
  E_Configure_Cat *ecat;
  E_Configure_It *eci;  
};

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;
static Evry_Plugin *p;
static Evry_Action *act;
static Evry_Type E_SETTINGS;

static void
_finish(Evry_Plugin *plugin)
{
   Settings_Item *it;
   GET_PLUGIN(p, plugin);
     
   EVRY_PLUGIN_ITEMS_CLEAR(p);
   
   EINA_LIST_FREE(p->items, it)
     EVRY_ITEM_FREE(it);

   EINA_LIST_FREE(p->categories, it)
     EVRY_ITEM_FREE(it);

   E_FREE(p);
}

static Evas_Object *
_icon_get(Evry_Item *item, Evas *e __UNUSED__)
{
   Settings_Item *it = (Settings_Item *) item;

   if (!item->icon)
     {
	if (it->eci && it->eci->icon)
	  item->icon = eina_stringshare_add(it->eci->icon); 
	else if (it->ecat->icon)
	  item->icon = eina_stringshare_add(it->ecat->icon);
     }
   
   return NULL;
}

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p;
   Eina_List *l;
   Settings_Item *it, *it2;

   if (!CHECK_TYPE(item, E_SETTINGS))
     return NULL;

   it = (Settings_Item *) item;
     
   EVRY_PLUGIN_INSTANCE(p, plugin);
   p->parent = EINA_TRUE;
   
   GET_PLUGIN(parent, item->plugin);
   
   EINA_LIST_FOREACH(parent->items, l, it2)
     {
	if (it2->ecat == it->ecat)
	  {
	     EVRY_ITEM_REF(it2);
	     p->items = eina_list_append(p->items, it2);
	  }
     }
   
   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);
   
   return EVRY_PLUGIN(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   int len = input ? strlen(input) : 0;
   
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);
   
   if ((!p->parent) && (len < plugin->config->min_query))
     return 0;
   
   if (!p->categories && !p->items)
     {
	Settings_Item *it;
	Eina_List *l, *ll;
	E_Configure_Cat *ecat;
	E_Configure_It *eci;

	EINA_LIST_FOREACH(e_configure_registry, l, ecat)
	  {
	     if ((ecat->pri < 0) || (!ecat->items)) continue;
	     if (!strcmp(ecat->cat, "system")) continue;

	     it = EVRY_ITEM_NEW(Settings_Item, p, ecat->label, _icon_get, NULL);
	     it->ecat = ecat;
	     EVRY_ITEM(it)->browseable = EINA_TRUE;
	     p->categories = eina_list_append(p->categories, it);
	
	     EINA_LIST_FOREACH(ecat->items, ll, eci)
	       {
		  if (eci->pri < 0) continue;

		  it = EVRY_ITEM_NEW(Settings_Item, p, eci->label, _icon_get, NULL);
		  it->eci = eci;
		  it->ecat = ecat;
		  EVRY_ITEM_DETAIL_SET(it, ecat->label);
   
		  p->items = eina_list_append(p->items, it);
	       }
	  }
     }

   EVRY_PLUGIN_ITEMS_ADD(p, p->categories, input, 1, 1);

   if (input || p->parent)
     EVRY_PLUGIN_ITEMS_ADD(p, p->items, input, 1, 1);

   return EVRY_PLUGIN_HAS_ITEMS(p);
}

static int
_action_check(Evry_Action *act __UNUSED__, const Evry_Item *item)
{
   return !!(((Settings_Item*)item)->eci);
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

static int
_plugins_init(const Evry_API *_api)
{   
   evry = _api;
   
   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   E_SETTINGS = evry->type_register("E_SETTINGS");
   
   p = EVRY_PLUGIN_BASE("Settings", "configure", E_SETTINGS, _begin, _finish, _fetch);
   p->browse = &_browse;
   evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 10);

   act = EVRY_ACTION_NEW("Show Dialog", E_SETTINGS, 0,
			 "preferences-advanced", _action, _action_check);

   evry->action_register(act, 0);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(p);

   EVRY_ACTION_FREE(act);
}


/***************************************************************************/

Eina_Bool
evry_plug_settings_init(E_Module *m)
{
   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   e_module_delayed_set(m, 1);

   return EINA_TRUE;
}

void
evry_plug_settings_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);
}

void
evry_plug_settings_save(void){}

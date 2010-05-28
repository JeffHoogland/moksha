/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e_mod_main.h"


typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  const char *input;
  Eina_List *items;
};

static Eina_List *plugins = NULL;
static const char _module_icon[] = "start";
static Evry_Type COLLECTION_PLUGIN;


static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *item)
{
   Evry_Plugin *inst;
   Evry_Plugin *pp;

   Plugin_Config *pc;

   if (!CHECK_TYPE(item, COLLECTION_PLUGIN))
     return NULL;

   pc = item->data;
   pp = pc->plugin;

   GET_PLUGIN(p, item->plugin);

   if (pp->begin && (inst = pp->begin(pp, NULL)))
     {
	inst->config = pc;
	return inst;
     }

   return NULL;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   Evry_Plugin *pp;
   Plugin_Config *pc;
   Evry_Item *it;
   Eina_List *l;
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   EINA_LIST_FOREACH(plugin->config->plugins, l, pc)
     {
	if (!pc->enabled)
	  continue;

	if ((pp = evry_plugin_find(pc->name)))
	  {
	     GET_ITEM(itp, pp);
	     it = EVRY_ITEM_NEW(Evry_Item, EVRY_PLUGIN(p), itp->label, NULL, NULL);
	     if (itp->icon) it->icon = eina_stringshare_ref(itp->icon);
	     pc->plugin = pp;
	     it->data = pc;
	     it->browseable = EINA_TRUE;
	     p->base.items = eina_list_append(p->base.items, it);
	  }
     }

   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   Evry_Plugin *inst;

   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_FREE(p);

   IF_RELEASE(p->input);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);

   return !!(p->base.items);
}

Eina_Bool
evry_plug_collection_init(void)
{
   Evry_Plugin *p;
   Plugin_Config *pc, *pcc;
   Eina_List *l;
   char path[4096];
   char title[4096];

   COLLECTION_PLUGIN = evry_type_register("COLLECTION_PLUGIN");

   e_configure_registry_category_add
     ("extensions", 80, _("Extensions"), NULL, "preferences-extensions");

   EINA_LIST_FOREACH(evry_conf->collections, l, pc)
     {
	p = EVRY_PLUGIN_NEW(Evry_Plugin, N_(pc->name),
			    _module_icon, COLLECTION_PLUGIN,
			    _begin, _finish, _fetch, NULL);
	p->browse = &_browse;
	p->config = pc;
	pc->plugin = p;
	if (evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 1))
	  {
	     p->config->aggregate = EINA_FALSE;
	  }

	snprintf(path, sizeof(path), "extensions/everything-%s", p->name);

	snprintf(title, sizeof(title), "Everything %s", p->name);
	
	e_configure_registry_item_add
	  (path, 110, title, NULL, NULL/*icon*/, evry_collection_conf_dialog);

	p->config_path = eina_stringshare_add(path);
	
	plugins = eina_list_append(plugins, p);
     }

   return EINA_TRUE;
}

void
evry_plug_collection_shutdown(void)
{
   Evry_Plugin *p;
   
   EINA_LIST_FREE(plugins, p)
     {
	e_configure_registry_item_del(p->config_path);
	eina_stringshare_del(p->config_path);
	EVRY_PLUGIN_FREE(p);
     }
   
}

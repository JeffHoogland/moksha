#include "e_mod_main.h"


typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;

  Eina_List *plugins;
};

static Eina_List *plugins = NULL;
static const char _module_icon[] = "preferences-plugin";
static Evry_Type COLLECTION_PLUGIN;
static Plugin_Config plugin_config;

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *item)
{
   Evry_Plugin *inst;
   Evry_Plugin *pp;

   Plugin_Config *pc;

   if (!CHECK_TYPE(item, COLLECTION_PLUGIN))
     return NULL;

   if (item->plugin != plugin)
     return NULL;

   pc = item->data;
   pp = pc->plugin;

   if (pp->begin && (inst = pp->begin(pp, NULL)))
     {
	if (!strcmp(plugin->name, "Plugins"))
	  inst->config = &plugin_config;
	else
	  inst->config = pc;

	return inst;
     }

   return NULL;
}

static Evry_Item *
_add_item(Plugin *p, Plugin_Config *pc)
{
   Evry_Plugin *pp;
   Evry_Item *it = NULL;

   if (pc->enabled && (pp = evry_plugin_find(pc->name)))
     {
	pc->plugin = pp;

	GET_ITEM(itp, pp);
	it = EVRY_ITEM_NEW(Evry_Item, EVRY_PLUGIN(p), itp->label, NULL, NULL);
	if (itp->icon) it->icon = eina_stringshare_ref(itp->icon);
	it->icon_get = itp->icon_get;
	it->data = pc;
	it->browseable = EINA_TRUE;
	it->detail = eina_stringshare_ref(EVRY_ITEM(p)->label);
	p->plugins = eina_list_append(p->plugins, it);
     }
   return it;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Plugin_Config *pc;
   Eina_List *l;
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   EINA_LIST_FOREACH(plugin->config->plugins, l, pc)
     _add_item(p, pc);

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin_all(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Plugin_Config *pc;
   Eina_List *l;
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   EINA_LIST_FOREACH(evry_conf->conf_subjects, l, pc)
     {
	if (!strcmp(pc->name, "All") ||
	    !strcmp(pc->name, "Actions") ||
	    !strcmp(pc->name, "Calculator") ||
	    !strcmp(pc->name, "Plugins"))
	  continue;

     _add_item(p, pc);
     }

   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   Evry_Item *it;

   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->plugins, it)
     EVRY_ITEM_FREE(it);
   
   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EVRY_PLUGIN_ITEMS_ADD(p, p->plugins, input, 1, 0);

   return !!(plugin->items);
}

static Evry_Plugin *
_add_plugin(const char *name)
{
   Evry_Plugin *p;
   char path[4096];
   char title[4096];

   p = EVRY_PLUGIN_BASE(name, _module_icon, COLLECTION_PLUGIN,
			_begin, _finish, _fetch);
   p->browse = &_browse;

   snprintf(path, sizeof(path), "launcher/everything-%s", p->name);

   snprintf(title, sizeof(title), "%s: %s", _("Everything Plugin"), p->base.label);

   e_configure_registry_item_params_add
     (path, 110, title, NULL, p->base.icon, evry_collection_conf_dialog, p->name);

   p->config_path = eina_stringshare_add(path);

   plugins = eina_list_append(plugins, p);

   return p;
}

Eina_Bool
evry_plug_collection_init(void)
{
   Evry_Plugin *p;
   Plugin_Config *pc;
   Eina_List *l;

   plugin_config.min_query = 0;
   plugin_config.top_level = EINA_TRUE;
   plugin_config.aggregate = EINA_FALSE;
   plugin_config.view_mode = VIEW_MODE_DETAIL;

   COLLECTION_PLUGIN = evry_type_register("COLLECTION_PLUGIN");

   p = _add_plugin("Plugins");
   p->begin = &_begin_all;
   if (evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 100))
     {
	p->config->aggregate = EINA_TRUE;
	p->config->top_level = EINA_TRUE;
	p->config->view_mode = VIEW_MODE_THUMB;
     }

   EINA_LIST_FOREACH(evry_conf->collections, l, pc)
     {
	p = _add_plugin(pc->name); 
	p->config = pc;
	pc->plugin = p;

	if (evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 1))
	  p->config->aggregate = EINA_FALSE;
     }

   return EINA_TRUE;
}

void
evry_plug_collection_shutdown(void)
{
   Evry_Plugin *p;

   EINA_LIST_FREE(plugins, p)
     {
	if (p->config_path)
	  {
	     e_configure_registry_item_del(p->config_path);
	     eina_stringshare_del(p->config_path);
	  }

	EVRY_PLUGIN_FREE(p);
     }
}

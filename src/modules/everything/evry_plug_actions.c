#include "e_mod_main.h"

/* action selector plugin: provides list of actions registered for
   candidate types provided by current plugin */

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;
  Eina_List *actions;
};


static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);
   Evry_Action *act;

   EINA_LIST_FREE(p->actions, act)
     if (act->cleanup) act->cleanup(act);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   PLUGIN(p, plugin);
   Evry_Action *act;
   Eina_List *l;
   const char *type;
   int changed = 0;
   
   if (plugin->type == type_action)
     {
	if (!it) return NULL;
	type = it->plugin->type_out;
	if (!type) return NULL;
     }

   EINA_LIST_FOREACH(evry_conf->actions, l, act)
     {
	if ((!act->type_in1) || ((act->type_in1 == type) &&
				 (!act->check_item || act->check_item(act, it))))
	  {
	     act->item1 = it;

	     act->base.plugin = plugin;
	     
	     if (!eina_list_data_find_list(p->actions, act))
	       {
		  changed = 1;
		  p->actions = eina_list_append(p->actions, act);
	       }
	     continue;
	  }
	changed = 1;
	p->actions = eina_list_remove(p->actions, act);
     }

   if (!p->actions) return NULL;
   
   return plugin;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->fuzzy_match || it2->fuzzy_match)
     {
	if (it1->fuzzy_match && !it2->fuzzy_match)
	  return -1;

	if (!it1->fuzzy_match && it2->fuzzy_match)
	  return 1;

	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);

   return 0;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Eina_List *l;
   Evry_Item *it;
   int match;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FOREACH(p->actions, l, it)
     {
	match = evry_fuzzy_match(it->label, input);

	if (!input || match)
	  {
	     it->fuzzy_match = match;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	  }
     }

   if (!plugin->items) return 0;

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   ACTION_GET(act, it);
   Evas_Object *o = NULL;

   if (act->icon_get)
     {
	o = act->icon_get(act, e);
     }

   if ((!o) && it->icon)
     {
	o = evry_icon_theme_get(it->icon, e);
     }

   return o;
}

Evry_Plugin *
evry_plug_actions_new(Evry_Selector *sel, int type)
{
   Evry_Plugin *plugin;

   plugin = EVRY_PLUGIN_NEW(Plugin, N_("Actions"), type, "", "",
		       _begin, _cleanup, _fetch, _icon_get, NULL);

   PLUGIN(p, plugin);
   p->selector = sel;
     
   evry_plugin_register(plugin, 2);
   return plugin;
}

/***************************************************************************/

EAPI void
evry_action_register(Evry_Action *act, int priority)
{
   EVRY_ITEM(act)->priority = priority;

   evry_conf->actions = eina_list_append(evry_conf->actions, act);
   /* TODO sorting, initialization, etc */
}

EAPI void
evry_action_unregister(Evry_Action *act)
{
   evry_conf->actions = eina_list_remove(evry_conf->actions, act);
   /* cleanup */
}

static void
_action_free_cb(Evry_Item *it)
{
   ACTION_GET(act, it);
   
   if (act->name)     eina_stringshare_del(act->name);
   if (act->type_in1) eina_stringshare_del(act->type_in1);
   if (act->type_in2) eina_stringshare_del(act->type_in2);

   E_FREE(act);
}

EAPI Evry_Action *
evry_action_new(const char *name, const char *label,
		const char *type_in1, const char *type_in2,
		const char *icon,
		int  (*action) (Evry_Action *act),
		int (*check_item) (Evry_Action *act, const Evry_Item *it))
{
   Evry_Action *act = E_NEW(Evry_Action, 1);

   evry_item_new(EVRY_ITEM(act), NULL, label, _action_free_cb);
   act->base.icon = icon;

   act->name = eina_stringshare_add(name);
   
   act->type_in1 = (type_in1 ? eina_stringshare_add(type_in1) : NULL);
   act->type_in2 = (type_in2 ? eina_stringshare_add(type_in2) : NULL);

   act->action = action;
   act->check_item = check_item;

   return act;
}

EAPI void
evry_action_free(Evry_Action *act)
{
   evry_action_unregister(act);

   evry_item_free(EVRY_ITEM(act));
}

#include "e_mod_main.h"


typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;
};

inline static int
_is_action(const Evry_Item *it)
{
   return (it->plugin->name == action_selector);
}

static int
_cb_sort_recent(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->usage && it2->usage)
     return (it1->usage > it2->usage ? -1 : 1);
   if (it1->usage && !it2->usage)
     return -1;
   if (it2->usage && !it1->usage)
     return 1;

   if (_is_action(it1) || _is_action(it2))
     {
	if (_is_action(it1) && _is_action(it2))
	  return (it1->priority - it2->priority);
	else if (_is_action(it1))
	  return ((it1->plugin->config->priority + it1->priority) -
		  (it2->plugin->config->priority));
	else
	  return ((it1->plugin->config->priority) -
		  (it2->plugin->config->priority + it2->priority));
     }

  return -1;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->usage && it2->usage)
     return (it1->usage > it2->usage ? -1 : 1);
   if (it1->usage && !it2->usage)
     return -1;
   if (it2->usage && !it1->usage)
     return 1;

   if (_is_action(it1) || _is_action(it2))
     {
	if (_is_action(it1) && _is_action(it2))
	  return (it1->priority - it2->priority);
	else if (_is_action(it1))
	  return ((it1->plugin->config->priority + it1->priority)
		  - it2->plugin->config->priority);
	else
	  return (it1->plugin->config->priority -
		  (it1->plugin->config->priority + it2->priority));
     }

   if ((it1->plugin == it2->plugin) &&
       (it1->priority - it2->priority))
     return (it1->priority - it2->priority);

   if (it1->fuzzy_match || it2->fuzzy_match)
     {
	if (it1->fuzzy_match && !it2->fuzzy_match)
	  return -1;

	if (!it1->fuzzy_match && it2->fuzzy_match)
	  return 1;

	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   if (it1->plugin->config->priority - it2->plugin->config->priority)
     return (it1->plugin->config->priority - it2->plugin->config->priority);

   if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);

   return strcasecmp(it1->label, it2->label);
}

static int refs = 0;

static inline Eina_List *
_add_item(Plugin *p, Eina_List *items, Evry_Item *it)
{
   evry_item_ref(it);
   items = eina_list_append(items, it);
   EVRY_PLUGIN_ITEM_APPEND(p, it);

   return items;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin *) plugin;
   Evry_Plugin *pp;
   Evry_State *s;
   Eina_List *l, *ll, *lp;
   Evry_Item *it;
   int i, cnt = 0;
   Eina_List *items = NULL;
   const char *context = NULL;

   EVRY_PLUGIN_ITEMS_FREE(p);
   
   s = p->selector->state;

   if (!s || !s->cur_plugins || !s->cur_plugins->next)
     return 0;

   for (i = 1; i < 3; i++)
     {
	Evry_Item *item;
	if (p->selector == selectors[i])
	  {
	     item = selectors[i-1]->state->cur_item;
	     context = item->context;
	  }	
     }

   /* first is aggregator itself */
   lp = s->cur_plugins->next;

   if (input)
     {
	EINA_LIST_FOREACH(lp, l, pp)
	  {
	     if (!pp->aggregate) continue;
	     
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (!it->fuzzy_match)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->fuzzy_match || p->selector == selectors[2])
		    items = _add_item(p, items, it);
	       }
	  }
     }

   /* always append items of action or object selector */
   if ((!input) &&
       ((p->selector == selectors[1]) ||
	(p->selector == selectors[2])))
     {
   	EINA_LIST_FOREACH(lp, l, pp)
   	  {
	     if (!pp->aggregate) continue;
	     
  	     for (cnt = 0, ll = pp->items; ll && cnt < 50; ll = ll->next, cnt++)
   	       {
   		  if (!eina_list_data_find_list(items, ll->data))
   		    {
   		       it = ll->data;
   		       it->fuzzy_match = 0;
		       items = _add_item(p, items, it);
   		    }
   	       }
   	  }
     }

   EINA_LIST_FOREACH(lp, l, pp)
     {
	if (!pp->aggregate) continue;
	
	EINA_LIST_FOREACH(pp->items, ll, it)
	  {
	     if (evry_history_item_usage_set(p->selector->history, it, input, context) &&
		 (!eina_list_data_find_list(items, it)))
	       {
		  items = _add_item(p, items, it);
		  continue;
	       }
	  }
     }

   if (lp && ((eina_list_count(lp) == 2) || (!EVRY_PLUGIN(p)->items)))
     {
	pp = lp->data;

	if (pp->aggregate)
	  {
	     EINA_LIST_FOREACH(pp->items, l, it)
	       {
		  if (!eina_list_data_find_list(items, it))
		    {
		       it->fuzzy_match = 0;
		       items = _add_item(p, items, it);
		    }
	       }
	  }
     }

   if (items) eina_list_free(items);

   if (input)
     {
	EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);
     }
   else
     {
	EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort_recent);
     }

   return 1;
}

static int
_action(Evry_Plugin *plugin, const Evry_Item *it)
{
   if (it->plugin && it->plugin->action)
     return it->plugin->action(it->plugin, it);

   return 0;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   EVRY_PLUGIN_ITEMS_FREE(plugin);
}

static Evas_Object *
_icon_get(Evry_Plugin *plugin, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;

   if (it->plugin)
     {
	if (it->plugin->icon_get)
	  o = it->plugin->icon_get(it->plugin, it, e);
	else if  (it->plugin->icon)
	  o = evry_icon_theme_get(it->plugin->icon, e);
     }

   return o;
}

static void
_plugin_free(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);

   E_FREE(p);
}

Evry_Plugin *
evry_plug_aggregator_new(Evry_Selector *selector)
{
   Plugin *p;
   Plugin_Config *pc;

   p = E_NEW(Plugin, 1);
   evry_plugin_new(EVRY_PLUGIN(p), "All", 0, "", "", 0, NULL, NULL,
		   NULL, _cleanup, _fetch, _action, _icon_get, _plugin_free);

   pc = E_NEW(Plugin_Config, 1);
   pc->enabled = 1;
   pc->priority = -1;
   EVRY_PLUGIN(p)->config = pc;

   p->selector = selector;

   return EVRY_PLUGIN(p);
}

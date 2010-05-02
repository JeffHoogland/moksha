#include "e_mod_main.h"

#define MAX_ITEMS 50

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;
};

static int
_cb_sort_recent(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   /* sort actions matching the subtype always before those matching type*/
   if ((it1->type == EVRY_TYPE_ACTION) &&
       (it2->type == EVRY_TYPE_ACTION))
     {
	const Evry_Action *act1 = data1;
	const Evry_Action *act2 = data2;

	if (act1->remember_context || act2->remember_context)
	  {
	     if (act1->remember_context && !act2->remember_context)
	       return -1;
	     if (!act1->remember_context && act2->remember_context)
	       return 1;
	  }

	if (act1->it1.item && act2->it1.item)
	  {
	     if ((act1->it1.type == act1->it1.item->type) &&
		 (act2->it1.type != act2->it1.item->type))
	       return -1;

	     if ((act1->it1.type != act1->it1.item->type) &&
		 (act2->it1.type == act2->it1.item->type))
	       return 1;
	  }
     }

   if (it1->usage > 0 || it2->usage > 0)
     {	
	return (it1->usage > it2->usage ? -1 : 1);
     }

   if (it1->type != EVRY_TYPE_ACTION &&
       it2->type != EVRY_TYPE_ACTION)
     {
	if ((it1->plugin == it2->plugin) &&
	    (it1->priority - it2->priority))
	  {
	     return (it1->priority - it2->priority);
	  }
	else if (it1->plugin->config->priority -
		 it2->plugin->config->priority)
	  {
	     return (it1->plugin->config->priority -
		     it2->plugin->config->priority);
	  }
     }
   
   return strcmp(it1->label, it2->label);

  return 1;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if ((it1->type == EVRY_TYPE_ACTION) &&
       (it2->type == EVRY_TYPE_ACTION))
     {
	const Evry_Action *act1 = data1;
	const Evry_Action *act2 = data2;

	if (act1->remember_context || act2->remember_context)
	  {
	     if (act1->remember_context && !act2->remember_context)
	       return -1;
	     if (!act1->remember_context && act2->remember_context)
	       return 1;
	  }

	if (act1->it1.item && act2->it1.item)
	  {
	     if ((act1->it1.type == act1->it1.item->type) &&
		 (act2->it1.type != act2->it1.item->type))
	       return -1;

	     if ((act1->it1.type != act1->it1.item->type) &&
		 (act2->it1.type == act2->it1.item->type))
	       return 1;
	  }
     }

   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
	if (it2->fuzzy_match <= 0)
	  return -1;

	if (it1->fuzzy_match <= 0)
	  return 1;

	if (abs (it1->fuzzy_match - it2->fuzzy_match) > 5)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   if (it1->usage > 0 || it2->usage > 0)
     {	
	return (it1->usage > it2->usage ? -1 : 1);
     }
   
   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
	if (it2->fuzzy_match <= 0)
	  return -1;
	if (it1->fuzzy_match <= 0)
	  return 1;

	return (it1->fuzzy_match - it2->fuzzy_match);
     }

   if (it1->type != EVRY_TYPE_ACTION &&
       it2->type != EVRY_TYPE_ACTION)
     {
	if ((it1->plugin == it2->plugin) &&
	    (it1->priority - it2->priority))
	  {
	     return (it1->priority - it2->priority);
	  }
	else if (it1->plugin->config->priority -
		 it2->plugin->config->priority)
	  {
	     return (it1->plugin->config->priority -
		     it2->plugin->config->priority);
	  }
     }
   
   return strcasecmp(it1->label, it2->label);
}

static inline Eina_List *
_add_item(Plugin *p, Eina_List *items, Evry_Item *it)
{
   /* remove duplicates provided by different plugins */
   if (it->id)
     {
	Eina_List *_l;
	Evry_Item *_it;

	EINA_LIST_FOREACH(p->base.items, _l, _it)
	  {
	     if ((it->plugin->name != _it->plugin->name) &&
		 (it->type == _it->type) &&
		 (it->id == _it->id))
	       return items;
	  }
     }

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
   Eina_List *l, *ll, *lp = NULL;
   Evry_Item *it;
   int i, cnt = 0;
   Eina_List *items = NULL;
   const char *context = NULL;
   if (input && !input[0]) input = NULL;

   EVRY_PLUGIN_ITEMS_FREE(p);

   s = p->selector->state;
   if (!s || !s->cur_plugins)
     return 0;

   /* get current 'context' ... */
   for (i = 1; i < 3; i++)
     {
	if (p->selector == selectors[i])
	  {
	     it = selectors[i-1]->state->cur_item;
	     if (it) context = it->context;
	  }
     }

   /* filter all to be shown in aggregator */
   EINA_LIST_FOREACH(s->cur_plugins, l, pp)
     {
	if (!pp->aggregate || pp == plugin) continue;
	lp = eina_list_append(lp, pp);
     }

   if (!lp) return 0;

   /* if there is only one plugin append all items */
   if (!lp->next)
     {
       pp = lp->data;

       EINA_LIST_FOREACH(pp->items, l, it)
	 {
	    if (it->usage >= 0)
	      evry_history_item_usage_set(p->selector->history, it, input, context);
	    it->fuzzy_match = evry_fuzzy_match(it->label, input);
	    items = _add_item(p, items, it);
	 }
     }
   /* if there is input append all items that match or have
      fuzzy_match set to -1 */
   else if (input)
     {
       EINA_LIST_FOREACH(lp, l, pp)
	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (it->fuzzy_match == 0)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->fuzzy_match || p->selector == selectors[2])
		    {
		       if (it->usage >= 0)
			 evry_history_item_usage_set(p->selector->history, it, input, context);

		       items = _add_item(p, items, it);
		    }
	       }
	  }
     }
   /* always append items of action or object selector */
   else if ((!input) &&
	    ((p->selector == selectors[1]) ||
	     (p->selector == selectors[2])))
     {
       EINA_LIST_FOREACH(lp, l, pp)
   	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
   	       {
		  if (it->usage >= 0)
		    evry_history_item_usage_set(p->selector->history, it, NULL, context);
		  it->fuzzy_match = 0;
		  items = _add_item(p, items, it);
   	       }
   	  }
     }
   /* no input: append all items that are in history */
   else
     {
       EINA_LIST_FOREACH(lp, l, pp)
	 {
	   EINA_LIST_FOREACH(pp->items, ll, it)
	     {
		if ((it->usage >= 0) &&
		    (evry_history_item_usage_set(p->selector->history, it, input, context)) &&
		    (!eina_list_data_find_list(items, it)))
		 {
		   items = _add_item(p, items, it);
		 }
	     }
	 }
     }

   if (eina_list_count(items) <  MAX_ITEMS)
     {
	EINA_LIST_FOREACH(lp, l, pp)
	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
   		  if (!eina_list_data_find_list(items, it))
		    items = _add_item(p, items, it);
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

   cnt = 0;
   EINA_LIST_FOREACH_SAFE(p->base.items, l, ll, it)
     {
	if (cnt++ < MAX_ITEMS) continue;
	evry_item_free(it);
	p->base.items = eina_list_remove_list(p->base.items, l);
     }

   return 1;
}

static void
_finish(Evry_Plugin *plugin)
{
   Evry_Item *it;
   EINA_LIST_FREE(plugin->items, it)
     evry_item_free(it);
}


static void
_free(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   
   _finish(plugin);

   free(p);
}

Evry_Plugin *
evry_plug_aggregator_new(Evry_Selector *sel, int type)
{
   Evry_Plugin *p;

   p = EVRY_PLUGIN_NEW(Plugin, N_("All"), NULL, 0, NULL, _finish, _fetch, _free);

   p->history = EINA_FALSE;
   evry_plugin_register(p, type, -1);

   GET_PLUGIN(pa, p);
   pa->selector = sel;

   return p;
}

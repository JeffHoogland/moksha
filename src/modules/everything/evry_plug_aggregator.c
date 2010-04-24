#include "e_mod_main.h"

#define MAX_ITEMS 50

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

   if (it1->plugin == it2->plugin)
     return (it1->priority - it2->priority);

   return strcmp(it1->label, it2->label);

  return 1;
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

   if (it1->fuzzy_match > 0 || it2->fuzzy_match > 0)
     {
	if (it2->fuzzy_match <= 0)
	  return -1;

	if (it1->fuzzy_match <= 0)
	  return 1;

	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }

   if (it1->plugin->config->priority - it2->plugin->config->priority)
     return (it1->plugin->config->priority - it2->plugin->config->priority);

   return strcasecmp(it1->label, it2->label);
}

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
   Eina_List *l, *ll, *lll, *lp;
   Evry_Item *it, *it2;
   int i, cnt = 0;
   Eina_List *items = NULL;
   const char *context = NULL;
   if (input && !input[0]) input = NULL;

   plugin->changed = 1;

   EVRY_PLUGIN_ITEMS_FREE(p);

   s = p->selector->state;
   if (!s || !s->cur_plugins)
     return 0;

   /* get current 'context' ... */
   for (i = 1; i < 3; i++)
     {
	Evry_Item *item;
	if (p->selector == selectors[i])
	  {
	     item = selectors[i-1]->state->cur_item;
	     context = item->context;
	  }
     }

   if (eina_list_data_find_list(s->cur_plugins, plugin))
     lp = s->cur_plugins->next;
   else
     lp = s->cur_plugins;

   if (lp && lp->data && lp->data == plugin)
     lp = s->cur_plugins->next;

   if ((lp) && (!lp->next))
     {
       pp = lp->data;

       if (pp->aggregate);
	 {
	   EINA_LIST_FOREACH(pp->items, l, it)
	     {
	       evry_history_item_usage_set(p->selector->history, it, input, context);
	       it->fuzzy_match = evry_fuzzy_match(it->label, input);
	       items = _add_item(p, items, it);
	     }
	 }
     }
   else if (input)
     {
       EINA_LIST_FOREACH(lp, l, pp)
	  {
	     if (!pp->aggregate) continue;

	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (it->fuzzy_match == 0)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->fuzzy_match || p->selector == selectors[2])
		    {
		      evry_history_item_usage_set(p->selector->history, it, input, context);
		      items = _add_item(p, items, it);
		    }
	       }
	  }
     }
   else if ((!input) &&
	    ((p->selector == selectors[1]) ||
	     (p->selector == selectors[2])))
     {
       /* always append items of action or object selector */
       EINA_LIST_FOREACH(lp, l, pp)
   	  {
	     if (!pp->aggregate) continue;
	     cnt = 0;
	     EINA_LIST_FOREACH(pp->items, ll, it)
   	       {
		  if (cnt++ == MAX_ITEMS) break;
   		  if (!eina_list_data_find_list(items, it))
   		    {
		      evry_history_item_usage_set(p->selector->history, it, NULL, context);
		      it->fuzzy_match = 0;
		       items = _add_item(p, items, it);
   		    }
   	       }
   	  }
     }
   else
     {
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

   /* remove duplicates provided by different plugins */
   /* EINA_LIST_FOREACH_SAFE(p->base.items, l, ll, it)
    *   {
    *     EINA_LIST_FOREACH(l->next, lll, it2)
    * 	  {
    * 	     if ((it->plugin->name != it2->plugin->name) &&
    * 		 (it->plugin->type_out == it2->plugin->type_out) &&
    * 		 (it->id == it2->id))
    * 	       {
    * 		  p->base.items = eina_list_remove_list(p->base.items, l);
    * 		  evry_item_free(it);
    * 		  break;
    * 	       }
    * 	  }
    *   } */

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
   Evry_Item *it;
   EINA_LIST_FREE(plugin->items, it)
     evry_item_free(it);
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

Evry_Plugin *
evry_plug_aggregator_new(Evry_Selector *sel, int type)
{
   Plugin *p;

   p = E_NEW(Plugin, 1);
   EVRY_PLUGIN_NEW(EVRY_PLUGIN(p), N_("All"), type, "", "",
		   NULL, _cleanup, _fetch, _icon_get, NULL);

   EVRY_PLUGIN(p)->action = &_action;
   EVRY_PLUGIN(p)->history = EINA_FALSE;

   evry_plugin_register(EVRY_PLUGIN(p), -1);
   p->selector = sel;

   return EVRY_PLUGIN(p);
}

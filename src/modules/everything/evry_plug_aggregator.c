#include "e_mod_main.h"

#define MAX_ITEMS 50

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;

  Evry_Item *warning;
};

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
   char buf[128];
   Evry_Selector *sel = p->selector;

   if (input && !input[0]) input = NULL;

   EVRY_PLUGIN_ITEMS_FREE(p);

   s = sel->state;
   if (!s) return 0;

   if (!s->cur_plugins)
     {
	/* 'text' and 'actions' are always loaded */
	if ((sel == selectors[0]) &&
	    (eina_list_count(s->plugins) == 2))
	  {
	     evry_item_ref(p->warning);
	     EVRY_PLUGIN_ITEM_APPEND(p, p->warning);
	  }
	return 1;
     }

   /* get current items' context ... */
   for (i = 1; i < 3; i++)
     {
	if (sel == selectors[i])
	  {
	     it = selectors[i-1]->state->cur_item;
	     if (it) context = it->context;
	  }
     }

   /* filter all to be shown in aggregator */
   EINA_LIST_FOREACH(s->cur_plugins, l, pp)
     {
	if (!pp->config->aggregate || pp == plugin)
	  continue;
	if (!sel->states->next && !pp->config->top_level)
	  continue;
	lp = eina_list_append(lp, pp);
     }

   /* show non-top-level plugins as item */
   if (!s->trigger_active && !sel->states->next)
     {
	EINA_LIST_FOREACH(s->plugins, l, pp)
	  {
	     GET_ITEM(it, pp);

	     if (pp->config->top_level || pp == plugin)
	       continue;

	     if (!pp->items)
	       continue;

	     it->fuzzy_match = evry_fuzzy_match(it->label, input);

	     it->hi = NULL;
	     evry_history_item_usage_set(it, NULL, NULL);

	     snprintf(buf, sizeof(buf), "%d %s", eina_list_count(pp->items), _("Items"));
	     if (it->detail)
	       eina_stringshare_del(it->detail);
	     it->detail = eina_stringshare_add(buf);

	     items = _add_item(p, items, it);
	  }
     }

   if (!lp && !items) return 0;

   /* if there is only one plugin append all items */
   if (lp && !lp->next)
     {
       pp = lp->data;

       EINA_LIST_FOREACH(pp->items, l, it)
	 {
	    if (it->usage >= 0)
	      evry_history_item_usage_set(it, input, context);
	    if (it->fuzzy_match == 0)
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

		  if (it->fuzzy_match || sel == selectors[2])
		    {
		       if (it->usage >= 0)
			 evry_history_item_usage_set(it, input, context);

		       items = _add_item(p, items, it);
		    }
	       }
	  }
     }
   /* always append items of action or object selector */
   else if ((!input) &&
	    ((sel == selectors[1]) ||
	     (sel == selectors[2])))
     {
       EINA_LIST_FOREACH(lp, l, pp)
   	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
   	       {
		  if (it->usage >= 0)
		    evry_history_item_usage_set(it, NULL, context);
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
		    (evry_history_item_usage_set(it, input, context)) &&
		    (!eina_list_data_find_list(items, it)))
		 {
		    it->fuzzy_match = 0;
		    items = _add_item(p, items, it);
		 }
	     }
	 }
     }

   if (lp && lp->next && eina_list_count(items) <  MAX_ITEMS)
     {
	EINA_LIST_FOREACH(lp, l, pp)
	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
   		  if (!eina_list_data_find_list(items, it))
		    {
		       it->usage = 0;
		       if (it->fuzzy_match == 0)
			 it->fuzzy_match = evry_fuzzy_match(it->label, input);

		       items = _add_item(p, items, it);
		    }
	       }
	  }
     }

   /* EINA_LIST_FOREACH(items, l, it)
    *   {
    * 	if(CHECK_TYPE(it, EVRY_TYPE_FILE))
    * 	  printf("%d %1.20f %s\n", it->fuzzy_match, it->usage, it->label);
    *   } */
   
   if (items) eina_list_free(items);
   if (lp) eina_list_free(lp);

   EVRY_PLUGIN_ITEMS_SORT(p, evry_items_sort_func);

   EINA_LIST_FOREACH_SAFE(p->base.items, l, ll, it)
     {
	if (cnt++ < MAX_ITEMS)
	  {
	     evry_item_ref(it);
	     continue;
	  }
	p->base.items = eina_list_remove_list(p->base.items, l);
     }

   return 1;
}

static void
_finish(Evry_Plugin *p)
{
   EVRY_PLUGIN_ITEMS_FREE(p);
}

static void
_free(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   _finish(plugin);

   evry_item_free(p->warning);

   free(p);
}

Evry_Plugin *
evry_plug_aggregator_new(Evry_Selector *sel, int type)
{
   Evry_Plugin *p;

   p = EVRY_PLUGIN_NEW(Plugin, N_("All"), NULL, 0, NULL, _finish, _fetch, _free);
   p->history = EINA_FALSE;
   if (evry_plugin_register(p, type, -1))
     {
	p->config->view_mode = VIEW_MODE_THUMB;
     }

   GET_PLUGIN(pa, p);
   pa->selector = sel;

   pa->warning = evry_item_new(NULL, p, N_("No plugins loaded"), NULL, NULL);
   pa->warning->type = EVRY_TYPE_NONE;

   return p;
}

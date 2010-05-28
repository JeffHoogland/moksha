#include "e_mod_main.h"

#define MAX_ITEMS 50

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  int type;
  Evry_Window *win;

  Evry_Item *warning;
};

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin *) plugin;
   Evry_Plugin *pp;
   Evry_State *s;
   Eina_List *l, *ll, *lp = NULL;
   Evry_Item *it, *it2;
   int i, cnt = 0;
   Eina_List *items = NULL;
   const char *context = NULL;
   char buf[128];
   Evry_Selector *sel = p->win->selectors[p->type];

   if (input && !input[0]) input = NULL;

   EVRY_PLUGIN_ITEMS_FREE(p);

   s = sel->state;
   if (!s) return 0;

   if (!s->cur_plugins)
     {
	/* 'text' and 'actions' are always loaded */
	if ((sel == p->win->selectors[0]) &&
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
	if (sel == p->win->selectors[i])
	  {
	     it = p->win->selectors[i-1]->state->cur_item;
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

	     it->hi = NULL;
	     it->usage = 0;
	     it->fuzzy_match = 0;

	     /* if (input)
	      *   {
	      * 	  evry_history_item_usage_set(it, NULL, NULL);
	      * 	  it->usage /= 100.0;
	      * 	  it->fuzzy_match = ;
	      *   } */

	     IF_RELEASE(it->detail);
	     snprintf(buf, sizeof(buf), "%d %s", eina_list_count(pp->items), _("Items"));
	     it->detail = eina_stringshare_add(buf);

	     items = eina_list_append(items, it);
	  }

	/* only one plugin: show items */
	if (eina_list_count(s->cur_plugins) == 1 && items)
	  {
	     pp = eina_list_data_get(items);
	     eina_list_free(items);
	     items = NULL;

	     EINA_LIST_FOREACH(pp->items, l, it)
	       {
		  if (it->usage >= 0)
		    evry_history_item_usage_set(it, input, context);
		  if (it->fuzzy_match == 0)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  items = eina_list_append(items, it);
	       }
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

	    items = eina_list_append(items, it);
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

		  if (it->fuzzy_match || sel == p->win->selectors[2])
		    {
		       if (it->usage >= 0)
			 evry_history_item_usage_set(it, input, context);

		       items = eina_list_append(items, it);
		    }
	       }
	  }
     }
   /* always append items of action or object selector */
   else if ((!input) &&
	    ((sel == p->win->selectors[1]) ||
	     (sel == p->win->selectors[2])))
     {
       EINA_LIST_FOREACH(lp, l, pp)
   	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
   	       {
		  if (it->usage >= 0)
		    evry_history_item_usage_set(it, NULL, context);
		  it->fuzzy_match = 0;

		  items = eina_list_append(items, it);
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
		    items = eina_list_append(items, it);
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
		       it->usage = -1;
		       if (it->fuzzy_match == 0)
			 it->fuzzy_match = evry_fuzzy_match(it->label, input);

		       items = eina_list_append(items, it);
		    }
	       }
	  }
     }

   items = eina_list_sort(items, -1, evry_items_sort_func);

   EINA_LIST_FOREACH(items, l, it)
     {
	/* remove duplicates provided by different plugins */
	if (it->id)
	  {
	     EINA_LIST_FOREACH(p->base.items, ll, it2)
	       {
		  if ((it->plugin->name != it2->plugin->name) &&
		      (it->type == it2->type) &&
		      (it->id == it2->id))
		    break;
	       }
	  }

	if (!it->id || !it2)
	  {
	     evry_item_ref(it);
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	  }

	if (cnt++ > MAX_ITEMS)
	  break;

     }

   if (items) eina_list_free(items);
   if (lp) eina_list_free(lp);

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
evry_aggregator_new(Evry_Window *win, int type)
{
   Evry_Plugin *p;

   p = EVRY_PLUGIN_NEW(Plugin, N_("All"), NULL, 0, NULL, _finish, _fetch, _free);

   if (evry_plugin_register(p, type, -1))
     {
	if (type == EVRY_PLUGIN_SUBJECT)
	  p->config->view_mode = VIEW_MODE_THUMB;
     }

   GET_PLUGIN(pa, p);
   pa->win = win;
   pa->type = type;

   pa->warning = evry_item_new(NULL, p, N_("No plugins loaded"), NULL, NULL);
   pa->warning->type = EVRY_TYPE_NONE;

   return p;
}

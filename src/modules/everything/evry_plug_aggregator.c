#include "e_mod_main.h"

//TODO min input for items not in history

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
   int i, top_level = 0, subj_sel = 0, inp_len = 0, cnt = 0;
   Eina_List *items = NULL;
   const char *context = NULL;
   char buf[128];
   Evry_Selector *sel = p->win->selectors[p->type];

   if (input && input[0])
     inp_len = strlen(input);
   else
     input = NULL;

   EVRY_PLUGIN_ITEMS_FREE(p);

   s = sel->state;
   if (!s) return 0;

   if (sel == p->win->selectors[0])
     subj_sel = 1;

   if (!sel->states->next)
     top_level = 1;

   /* get current items' context ... */
   for (i = 1; i < 3; i++)
     {
	if (sel == p->win->selectors[i])
	  {
	     it = p->win->selectors[i-1]->state->cur_item;
	     if (it) context = it->context;
	  }
     }

   /* collect plugins to be shown in aggregator */
   EINA_LIST_FOREACH(s->cur_plugins, l, pp)
     {
	/* dont show in aggregator */
	if (!pp->config->aggregate)
	  continue;
	/* do not recurse */
	if (pp == plugin)
	  continue;
	/* dont show plugin in top-level */
	if (top_level && !pp->config->top_level)
	  continue;
	lp = eina_list_append(lp, pp);
     }

   /* show non-top-level plugins as item */
   if (top_level && (!s->trigger_active))
     {
	EINA_LIST_FOREACH(s->plugins, l, pp)
	  {
	     int min_fuzz = 0;
	     double max_usage = 0.0;

	     if (pp->config->top_level)
	       continue;

	     if (pp == plugin)
	       continue;

	     if (!pp->items)
	       continue;

	     /* give plugin item the highest priority of its items */
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (it->usage >= 0)
		    evry_history_item_usage_set(it, input, context);

		  if (it->usage && (it->usage > max_usage))
		    max_usage = it->usage;

		  if (it->fuzzy_match == 0)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if ((!min_fuzz) || ((it->fuzzy_match > 0) &&
				      (it->fuzzy_match < min_fuzz)))
		    min_fuzz = it->fuzzy_match;
	       }

	     GET_ITEM(it, pp);

	     it->hi = NULL;
	     /* TODO get better usage estimate */
	     evry_history_item_usage_set(it, NULL, NULL);
	     it->usage /= 100.0;

	     if ((it->usage && max_usage) && (it->usage < max_usage))
	       it->usage = max_usage;
	     it->fuzzy_match = min_fuzz;

	     IF_RELEASE(it->detail);
	     snprintf(buf, sizeof(buf), "%d %s", eina_list_count(pp->items), _("Items"));
	     it->detail = eina_stringshare_add(buf);

	     items = eina_list_append(items, it);
	  }

	/* only one plugin: show items */
	if ((eina_list_count(s->cur_plugins)) == 1 && items &&
	    (pp = eina_list_data_get(items)) && (pp->config->aggregate))
	  {
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

#if 0
	/* append all plugins as items (which were not added above) */
	if (inp_len >= plugin->config->min_query)
	  {
	     EINA_LIST_FOREACH(s->plugins, l, pp)
	       {
		  if (!strcmp(pp->name, "Actions"))
		    continue;

		  /* items MUST only conatin plugins here ! */
		  EINA_LIST_FOREACH(items, ll, pp2)
		    if (pp2->name == pp->name) break;
		  if (pp2)
		    continue;

		  GET_ITEM(it, pp);

		  if ((!input) ||
		      (it->fuzzy_match = evry_fuzzy_match(it->label, input)))
		    {
		       evry_history_item_usage_set(it, input, NULL);
		       it->usage /= 100.0;

		       EVRY_ITEM(pp)->plugin->state = s;
		       items = eina_list_append(items, pp);
		    }
	       }
	  }
#endif
	if (!lp && (eina_list_count(items) < 2))
	  {
	     if (items) eina_list_free(items);
	     return 0;
	  }
     }

   if (!lp && !items)
     return 0;

   /* if there is only one plugin append all items */
   if (lp && !lp->next)
     {
	pp = eina_list_data_get(lp);

	EINA_LIST_FOREACH(pp->items, l, it)
	  {
	     if (it->usage >= 0)
	       evry_history_item_usage_set(it, input, context);

	     if (it->fuzzy_match == 0)
	       it->fuzzy_match = evry_fuzzy_match(it->label, input);

	     items = eina_list_append(items, it);
	  }
     }
   /* if there is input append all items that match */
   else if (input)
     {
       EINA_LIST_FOREACH(lp, l, pp)
	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (it->fuzzy_match == 0)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->usage >= 0)
		    evry_history_item_usage_set(it, input, context);

		  if ((subj_sel) && (top_level) &&
		      (!it->usage) && (inp_len < plugin->config->min_query))
		    continue;

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
		if ((!subj_sel) || (it->usage < 0) || 
		    (evry_history_item_usage_set(it, input, context)))
		 {
		    it->fuzzy_match = 0;
		    items = eina_list_append(items, it);
		 }
	     }
	 }
     }

   if ((!top_level) && (eina_list_count(items) <  MAX_ITEMS))
     {
   	EINA_LIST_FOREACH(lp, l, pp)
   	  {
   	     EINA_LIST_FOREACH(pp->items, ll, it)
   	       {
   		  if (eina_list_data_find_list(items, it))
		    continue;

		  items = eina_list_append(items, it);
   	       }
   	  }
     }

   items = eina_list_sort(items, -1, evry_items_sort_func);

   EINA_LIST_FOREACH(items, l, it)
     {
	/* remove duplicates provided by different plugins. e.g.
	   files / places and tracker can find the same files */
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

   if (lp) eina_list_free(lp);

   if (items)
     {
	eina_list_free(items);
     }
   /* 'text' and 'actions' are always loaded */
   else if ((subj_sel) && (eina_list_count(s->plugins) == 2))
     {
	evry_item_ref(p->warning);
	EVRY_PLUGIN_ITEM_APPEND(p, p->warning);
     }

   return !!(p->base.items);
}

static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_FREE(p);

   E_FREE(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it __UNUSED__)
{
   Plugin *p;

   GET_PLUGIN(base, plugin);
   EVRY_PLUGIN_INSTANCE(p, plugin);

   p->type = base->type;
   p->win  = base->win;
   p->warning = base->warning;

   return EVRY_PLUGIN(p);
}

static void
_free(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   evry_item_free(p->warning);

   free(p);
}

Evry_Plugin *
evry_aggregator_new(Evry_Window *win, int type)
{
   Evry_Plugin *p;

   p = EVRY_PLUGIN_NEW(Plugin, "All", NULL, 0, _begin, _finish, _fetch, _free);

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

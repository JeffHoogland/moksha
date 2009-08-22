#include "e_mod_main.h"


typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;
};


static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

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

   return 0;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin *) plugin;
   Evry_Plugin *pp;
   Evry_State *s;
   Eina_List *l, *ll;
   Evry_Item *it;
   int cnt = 0;
   Eina_List *items = NULL;

   s = p->selector->state;

   EINA_LIST_FREE(p->base.items, it)
     evry_item_free(it);

   EINA_LIST_FOREACH(s->cur_plugins, l, pp)
     cnt += eina_list_count(pp->items);


   if (input[0])
     {
	EINA_LIST_FOREACH(s->cur_plugins, l, pp)
	  {
	     EINA_LIST_FOREACH(pp->items, ll, it)
	       {
		  if (!it->fuzzy_match)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->fuzzy_match || p->selector == selectors[2])
		    {
		       evry_item_ref(it);
		       items = eina_list_append(items, it);
		       p->base.items = eina_list_append(p->base.items, it);
		    }
	       }
	  }
     }

   if (!input[0] || eina_list_count(items) < 20)
     {
	EINA_LIST_FOREACH(s->cur_plugins, l, pp)
   	  {
   	     for (cnt = 0, ll = pp->items; ll && cnt < 15; ll = ll->next, cnt++)
   	       {
		  if (!items || !eina_list_data_find_list(items, ll->data))
		    {
		       it = ll->data;

		       evry_item_ref(it);
		       it->fuzzy_match = 0;
		       p->base.items = eina_list_append(p->base.items, it);
		    }
   	       }
   	  }
     }

   if (items) eina_list_free(items);

   if (input[0])
     p->base.items = eina_list_sort
       (p->base.items, eina_list_count(p->base.items), _cb_sort);

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
_icon_get(Evry_Plugin *p, const Evry_Item *it, Evas *e)
{
   if (it->plugin && it->plugin->icon_get)
     return it->plugin->icon_get(it->plugin, it, e);

   return NULL;
}

Evry_Plugin *
evry_plug_aggregator_new(Evry_Selector *selector)
{
   Plugin *p;
   Plugin_Config *pc;

   p = E_NEW(Plugin, 1);
   evry_plugin_new(&p->base, "All", 0, "", "", 0, NULL, NULL,
		   NULL, _cleanup, _fetch, _action, _icon_get,
		   NULL, NULL);

   pc = E_NEW(Plugin_Config, 1);
   pc->enabled = 1;
   pc->priority = -1;
   p->base.config = pc;

   p->selector = selector;

   return &p->base;
}

void
evry_plug_aggregator_free(Evry_Plugin *plugin)
{
   Plugin *p = (Plugin *) plugin;

   evry_plugin_free(&p->base, 0);
   E_FREE(p);
}

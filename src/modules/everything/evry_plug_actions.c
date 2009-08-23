#include "e_mod_main.h"

/* action selector plugin: provides list of actions registered for
   candidate types provided by current plugin */


static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   Evry_Selector *sel = selectors[1];

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   if (sel->actions) eina_list_free(sel->actions);
   sel->actions = NULL;
}

static Evry_Plugin *
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_Selector *sel = selectors[1];

   _cleanup(p);

   if (!it) return NULL;

   const char *type = it->plugin->type_out;

   if (!type) return NULL;

   EINA_LIST_FOREACH(evry_conf->actions, l, act)
     {
	if (act->type_in1 && (act->type_in1 == type) &&
	    (!act->check_item || act->check_item(act, it)))
	  {
	     act->item1 = it;
	     sel->actions = eina_list_append(sel->actions, act);
	  }
     }

   if (!sel->actions) return NULL;

   return p;
}

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
_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_Item *it;
   Evry_Selector *sel = selectors[1];
   int match = 0;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   EINA_LIST_FOREACH(sel->actions, l, act)
     {
	if (input)
	  match = evry_fuzzy_match(act->name, input);

	if (!input || match)
	  {
	     it = evry_item_new(NULL, p, act->name, NULL);
	     it->fuzzy_match = match;
	     it->data = act;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	  }
     }

   if (input)
     EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   if (p->items) return 1;

   return 0;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   Evry_Action *act = it->data;

   if (!act) return NULL;

   if (act->icon_get)
     o = act->icon_get(act, e);
   else if (act->icon)
     o = evry_icon_theme_get(act->icon, e);

   return o;
}

Evry_Plugin *
evry_plug_actions_new(void)
{
   Evry_Plugin *p;

   p = evry_plugin_new(NULL, "Select Action", type_action, "", "", 0, NULL, NULL,
		       _begin, _cleanup, _fetch, NULL, _icon_get, NULL, NULL);

   evry_plugin_register(p, 2);

   return p;
}

void
evry_plug_actions_free(Evry_Plugin *plugin)
{
   evry_plugin_free(plugin, 1);
}

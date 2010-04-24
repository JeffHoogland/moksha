#include "e_mod_main.h"

/* action selector plugin: provides list of actions registered for
   candidate types provided by current plugin */


static void
_cleanup(Evry_Plugin *p)
{
   Evry_Action *act;
   Evry_Selector *sel;

   if (p->type == type_subject)
     sel = selectors[0];
   else
     sel = selectors[1];

   EVRY_PLUGIN_ITEMS_FREE(p);

   EINA_LIST_FREE(sel->cur_actions, act)
     if (act->cleanup) act->cleanup(act);
}

static Evry_Plugin *
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   Evry_Action *act;
   Eina_List *l;
   const char *type;
   Evry_Selector *sel;

   if (p->type == type_subject)
     sel = selectors[0];
   else
     sel = selectors[1];

   _cleanup(p);

   if (p->type == type_action)
     {
	if (!it) return NULL;
	type = it->plugin->type_out;
	if (!type) return NULL;
     }

   EINA_LIST_FOREACH(evry_conf->actions, l, act)
     {
	if ((!act->type_in1) ||
	    ((act->type_in1 == type) && (!act->check_item || act->check_item(act, it))))
	  {
	     act->item1 = it;

	     /* FIXME where is this used ?*/
	     /* if (act->type_out && act->intercept && !(act->intercept(act)))
	      *   continue;; */

	     sel->cur_actions = eina_list_append(sel->cur_actions, act);
	  }
     }

   if (!sel->cur_actions) return NULL;

   return p;
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
_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_Item *it;
   Evry_Selector *sel;
   int match = 0;

   if (p->type == type_subject)
     sel = selectors[0];
   else
     sel = selectors[1];

   EVRY_PLUGIN_ITEMS_FREE(p);

   EINA_LIST_FOREACH(sel->cur_actions, l, act)
     {
	if (input)
	  match = evry_fuzzy_match(act->name, input);

	if (!input || match)
	  {
	     it = evry_item_new(NULL, p, act->label, NULL);
	     it->fuzzy_match = match;
	     it->data = act;
	     it->priority = act->priority;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	  }
     }

   if (!p->items) return 0;

   if (input)
     EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
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
evry_plug_actions_new(int type)
{
   Evry_Plugin *p;

   p = evry_plugin_new(NULL, action_selector, _("Actions"), type, "", "",
		       _begin, _cleanup, _fetch, _icon_get, NULL);

   evry_plugin_register(p, 2);

   return p;
}

void
evry_plug_actions_free(Evry_Plugin *plugin)
{
   evry_plugin_free(plugin, 1);
}

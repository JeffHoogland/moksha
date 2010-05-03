#include "e_mod_main.h"

/* action selector plugin: provides list of actions registered for
   candidate types provided by current plugin */

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Evry_Selector *selector;
  Eina_List *actions;
  Eina_Bool parent;
  Evry_Action *action;
};

static Evry_Plugin *_base_plug = NULL;

static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   Evry_Action *act;

   if (p->parent)
     {
	EINA_LIST_FREE(p->actions, act);
	E_FREE(p);
     }
   else
     {
	EINA_LIST_FREE(p->actions, act);
     }
}

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *it)
{
   Evry_Action *act;
   Plugin *p;
   Eina_List *l;

   if (!CHECK_TYPE(it, EVRY_TYPE_ACTION))
     return NULL;

   act = EVRY_ACTN(it);

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;
   p->actions = act->fetch(act);
   p->parent = EINA_TRUE;
   p->action = act;

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Evry_Action *act;
   Eina_List *l;

   GET_PLUGIN(p, plugin);

   EINA_LIST_FOREACH(evry_conf->actions, l, act)
     {
	if (((!act->it1.type) ||
	     (CHECK_TYPE(it, act->it1.type)) ||
	     (CHECK_SUBTYPE(it, act->it1.type))) &&
	    (!act->check_item || act->check_item(act, it)))
	  {
	     act->base.plugin = plugin;

	     if (!eina_list_data_find_list(p->actions, act))
	       {
		  act->it1.item = it;
		  p->actions = eina_list_append(p->actions, act);
	       }
	     continue;
	  }
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
   const Evry_Action *act1 = data1;
   const Evry_Action *act2 = data2;

   if (act1->remember_context || act2->remember_context)
     {
	if (act1->remember_context && !act2->remember_context)
	  return -1;
	if (!act1->remember_context && act2->remember_context)
	  return 1;
     }

   /* sort type match before subtype match */
   if (act1->it1.item && act2->it1.item)
     {
	if ((act1->it1.type == act1->it1.item->type) &&
	    (act2->it1.type != act2->it1.item->type))
	  return -1;

	if ((act1->it1.type != act1->it1.item->type) &&
	    (act2->it1.type == act2->it1.item->type))
	  return 1;
     }

   if (it1->fuzzy_match || it2->fuzzy_match)

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
   GET_PLUGIN(p, plugin);
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

Evry_Plugin *
evry_plug_actions_new(Evry_Selector *sel, int type)
{
   Evry_Plugin *plugin;

   if (type == EVRY_PLUGIN_SUBJECT)
     {
	plugin = EVRY_PLUGIN_NEW(Plugin, N_("Actions"), NULL, 0, NULL, _finish, _fetch, NULL);
     }
   else if (type == EVRY_PLUGIN_ACTION)
     {
	plugin = EVRY_PLUGIN_NEW(Plugin, N_("Actions"), NULL, 0, _begin, _finish, _fetch, NULL);
     }
   else return NULL;

   plugin->browse = &_browse;

   GET_PLUGIN(p, plugin);
   p->selector = sel;

   evry_plugin_register(plugin, type, 2);
   return plugin;
}

/***************************************************************************/

int evry_plug_actions_init()
{
   _base_plug = evry_plugin_new(NULL, _("Actions"), NULL, NULL,
				EVRY_TYPE_ACTION, NULL, NULL, NULL, NULL);

   return 1;
}

void evry_plug_actions_shutdown()
{
   Evry_Item *it;

   evry_plugin_free(_base_plug);

   /* bypass unregister, because it modifies the list */
   EINA_LIST_FREE(evry_conf->actions, it)
     evry_item_free(it);
}


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
   /* finish */
}

static void
_action_free_cb(Evry_Item *it)
{
   GET_ACTION(act, it);

   if (act->name)     eina_stringshare_del(act->name);

   E_FREE(act);
}

EAPI Evry_Action *
evry_action_new(const char *name, const char *label,
		Evry_Type type_in1, Evry_Type type_in2,
		const char *icon,
		int  (*action) (Evry_Action *act),
		int (*check_item) (Evry_Action *act, const Evry_Item *it))
{
   Evry_Action *act = EVRY_ITEM_NEW(Evry_Action, _base_plug, label, NULL, _action_free_cb);
   act->base.icon = icon;

   act->name = eina_stringshare_add(name);

   act->it1.type = type_in1;
   act->it2.type = type_in2;

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

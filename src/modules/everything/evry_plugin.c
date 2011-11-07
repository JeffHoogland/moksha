#include "e_mod_main.h"

static Eina_List *actions = NULL;

int
evry_plugins_init(void)
{
   return 1;
}

void
evry_plugins_shutdown(void)
{
   Evry_Action *act;

   EINA_LIST_FREE(actions, act)
     evry_action_free(act);
}


static int
_evry_cb_plugin_sort(const void *data1, const void *data2)
{
   const Plugin_Config *pc1 = data1;
   const Plugin_Config *pc2 = data2;

   return pc1->priority - pc2->priority;
}

void
_evry_plugin_free(Evry_Item *it)
{
   GET_EVRY_PLUGIN(p, it);

   evry_plugin_unregister(p);

   DBG("%s", p->name);
   if (p->config) p->config->plugin = NULL;
   if (p->name) eina_stringshare_del(p->name);

   if (p->free)
     p->free(p);
   else
     E_FREE(p);
}

Evry_Plugin *
evry_plugin_new(Evry_Plugin *base, const char *name, const char *label,
		const char *icon, Evry_Type item_type,
		Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item),
		void (*finish) (Evry_Plugin *p),
		int  (*fetch) (Evry_Plugin *p, const char *input),
		void (*cb_free) (Evry_Plugin *p))
{
   Evry_Plugin *p;
   Evry_Item *it;

   if (base)
     p = base;
   else
     p = E_NEW(Evry_Plugin, 1);

   it = evry_item_new(EVRY_ITEM(p), NULL, label, NULL, _evry_plugin_free);
   it->plugin = p;
   it->browseable = EINA_TRUE;
   it->type  = EVRY_TYPE_PLUGIN;
   if (item_type)
     it->subtype = item_type;
   if (icon)
     it->icon = eina_stringshare_add(icon);

   p->name   = eina_stringshare_add(name);
   p->begin  = begin;
   p->finish = finish;
   p->fetch  = fetch;

   p->async_fetch = EINA_FALSE;
   p->history     = EINA_TRUE;

   p->free = cb_free;

   return p;
}

void
evry_plugin_free(Evry_Plugin *p)
{
   evry_item_free(EVRY_ITEM(p));
}

static int
_evry_plugin_action_browse(Evry_Action *act)
{
   Evry_Plugin *p;
   Eina_List *plugins = NULL;
   Evry_Selector *sel;

   GET_ITEM(it, act->it1.item);
   GET_EVRY_PLUGIN(pp, EVRY_ITEM(act)->data);

   if (!it->plugin || !it->plugin->state)
     return 0;

   sel = it->plugin->state->selector;

   evry_selectors_switch(sel->win, -1, EINA_TRUE);

   if ((p = pp->begin(pp, it)))
     {
	plugins = eina_list_append(plugins, p);

	if (!evry_state_push(sel, plugins))
	  eina_list_free(plugins);
     }

   return 0;
}

int
evry_plugin_register(Evry_Plugin *p, int type, int priority)
{
   Eina_List *l;
   Plugin_Config *pc;
   Eina_List *conf;
   int new_conf = 0;

   if ((type < 0) || (type > 2))
     return 0;

   if (type == EVRY_PLUGIN_SUBJECT)
     conf = evry_conf->conf_subjects;
   else if (type == EVRY_PLUGIN_ACTION)
     conf = evry_conf->conf_actions;
   else if (type == EVRY_PLUGIN_OBJECT)
     conf = evry_conf->conf_objects;

   EINA_LIST_FOREACH(conf, l, pc)
     if (pc->name && p->name && !strcmp(pc->name, p->name))
       break;

   /* check if module of same name is already loaded */
   /* if ((pc) && (pc->plugin))
    *   return 0; */
   
   /* collection plugin sets its own config */
   if (!pc && p->config)
     {
	conf = eina_list_append(conf, p->config);
	pc = p->config;
     }
   else if (!pc)
     {
	new_conf = 1;
	pc = E_NEW(Plugin_Config, 1);
	pc->name = eina_stringshare_add(p->name);
	pc->enabled = 1;
	pc->priority = priority ? priority : 100;
	pc->view_mode = VIEW_MODE_NONE;
	pc->aggregate = EINA_TRUE;
	pc->top_level = EINA_TRUE;

	conf = eina_list_append(conf, pc);
     }
   if (pc->trigger && strlen(pc->trigger) == 0)
     {
	eina_stringshare_del(pc->trigger);
	pc->trigger = NULL;
     }

   p->config = pc;
   pc->plugin = p;

   conf = eina_list_sort(conf, -1, _evry_cb_plugin_sort);

   if (type == EVRY_PLUGIN_SUBJECT)
     evry_conf->conf_subjects = conf;
   else if (type == EVRY_PLUGIN_ACTION)
     evry_conf->conf_actions = conf;
   else if (type == EVRY_PLUGIN_OBJECT)
     evry_conf->conf_objects = conf;
   
   if ((type == EVRY_PLUGIN_SUBJECT) && (p->name && strcmp(p->name, "All")))
     {
	char buf[256];
	snprintf(buf, sizeof(buf), _("Show %s Plugin"), p->name);

	e_action_predef_name_set(_("Everything Launcher"), buf,
				 "everything", p->name, NULL, 1);
     }

   if (p->input_type)
     {
	Evry_Action *act;
	char buf[256];
	snprintf(buf, sizeof(buf), _("Browse %s"), EVRY_ITEM(p)->label);

	act = EVRY_ACTION_NEW(buf, p->input_type, 0, EVRY_ITEM(p)->icon,
			      _evry_plugin_action_browse, NULL);
	EVRY_ITEM(act)->icon_get = EVRY_ITEM(p)->icon_get;
	EVRY_ITEM(act)->data = p;
	evry_action_register(act, 1);
	actions = eina_list_append(actions, act);
     }

   return new_conf;
}

void
evry_plugin_unregister(Evry_Plugin *p)
{
   DBG("%s", p->name);
   Eina_List *l = evry_conf->conf_subjects;

   if (l && eina_list_data_find_list(l, p->config))
     {
	char buf[256];
   	snprintf(buf, sizeof(buf), _("Show %s Plugin"), p->name);

   	e_action_predef_name_del(_("Everything"), buf);
     }
}

Evry_Plugin *
evry_plugin_find(const char *name)
{
   Plugin_Config *pc = NULL;
   Eina_List *l;
   const char *n = eina_stringshare_add(name);

   EINA_LIST_FOREACH(evry_conf->conf_subjects, l, pc)
     {
	if (!pc->plugin) continue;
	if (pc->name == n)
	  break;
     }

   eina_stringshare_del(n);

   if (!pc) return NULL;

   return pc->plugin;
}

#include "e.h"
#include "e_mod_main.h"

static Evry_Plugin *p;
static Evry_Action *act;


static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);
}

static void
_item_add(Evry_Plugin *p, E_Configure_It *eci, int prio)
{
   Evry_Item *it;

   it = evry_item_new(p, eci->label);
   it->data[0] = eci;
   it->priority = prio;

   p->items = eina_list_append(p->items, it);
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;

   it1 = data1;
   it2 = data2;

   /* TODO sort by name? */
   return (it1->priority - it2->priority);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   char match1[128];
   char match2[128];
   Eina_List *l, *ll;
   E_Configure_Cat *ecat;
   E_Configure_It *eci;

   _cleanup(p);

   snprintf(match1, sizeof(match1), "%s*", input);
   snprintf(match2, sizeof(match2), "*%s*", input);

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	if ((ecat->pri < 0) || (!ecat->items)) continue;
	if (!strcmp(ecat->cat, "system")) continue;
	
	EINA_LIST_FOREACH(ecat->items, ll, eci)
	  {
	     if (eci->pri >= 0)
	       {
		  if (e_util_glob_case_match(eci->label, match1))
		    _item_add(p, eci, 1);
		  else if (e_util_glob_case_match(eci->label, match2))
		    _item_add(p, eci, 2);
		  else if (e_util_glob_case_match(ecat->label, match1))
		    _item_add(p, eci, 3);
		  else if (e_util_glob_case_match(ecat->label, match2))
		    _item_add(p, eci, 4);
	       }
	  }
     }

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   E_Configure_It *eci = it->data[0];

   if (eci->icon)
     {
	o = e_icon_add(e);
	if (!evry_icon_theme_set(o, eci->icon))
	  {
	     evas_object_del(o);
	     o = e_util_icon_add(eci->icon, e);
	  }
     }

   return o;
}

static int
_action(Evry_Action *act, Evry_Item *it, Evry_Item *it2 __UNUSED__, const char *input __UNUSED__)
{
   E_Configure_It *eci, *eci2;
   E_Container *con;
   E_Configure_Cat *ecat;
   Eina_List *l, *ll;
   char buf[1024];
   int found = 0;

   eci = it->data[0];
   con = e_container_current_get(e_manager_current_get());

   EINA_LIST_FOREACH(e_configure_registry, l, ecat)
     {
	if (found) break;
	
	EINA_LIST_FOREACH(ecat->items, ll, eci2)
	  {
	     if (eci == eci2)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s",
			   ecat->cat,
			   eci->item);
		  found = 1;
		  break;
	       }
	  }
     }

   if (found)
     e_configure_registry_call(buf, con, NULL);

   return EVRY_ACTION_FINISHED;
}

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Settings";
   p->type = type_subject;
   p->type_in  = "NONE";
   p->type_out = "E_SETTINGS";
   p->fetch = &_fetch;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;

   evry_plugin_register(p);

   act = E_NEW(Evry_Action, 1);
   act->name = "Show Dialog";
   act->is_default = EINA_TRUE;
   act->type_in1 = "E_SETTINGS";
   act->action = &_action;
   act->icon = "preferences-advanced";
   evry_action_register(act);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p);
   evry_action_unregister(act);
   E_FREE(p);
   E_FREE(act);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

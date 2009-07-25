#include "e.h"
#include "e_mod_main.h"

static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _cleanup(Evry_Plugin *p);
static void _item_add(Evry_Plugin *p, E_Border *bd, int prio);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin *p;


EAPI int
evry_plug_border_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Windows";
   p->type_in  = "NONE";
   p->type_out = "BORDER";
   p->need_query = 0;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   evry_plugin_register(p);

   return 1;
}

EAPI int
evry_plug_border_shutdown(void)
{
   evry_plugin_unregister(p);
   E_FREE(p);

   return 1;
}

static int
_action(Evry_Plugin *p __UNUSED__, Evry_Item *it, const char *input __UNUSED__)
{
   E_Border *bd;
   E_Zone *zone;

   if (!it) return EVRY_ACTION_CONTINUE;

   bd = (E_Border *)it->data[0];
   zone = e_util_zone_current_get(e_manager_current_get());

   if (bd->desk != (e_desk_current_get(zone)))
     e_desk_show(bd->desk);

   if (bd->shaded)
     e_border_unshade(bd, E_DIRECTION_UP);

   if (bd->iconic)
     e_border_uniconify(bd);
   else
     e_border_raise(bd);

   /* e_border_focus_set(bd, 1, 1); */
   e_border_focus_set_with_pointer(bd);

   return EVRY_ACTION_FINISHED;
}

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	/* if (it->data[0]) e_object_unref(E_OBJECT(it->data[0])); */
	if (it->label) eina_stringshare_del(it->label);
	E_FREE(it);
     }
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   E_Manager *man;
   E_Zone *zone;

   char match1[4096];
   char match2[4096];
   E_Border *bd;
   E_Border_List *bl;

   _cleanup(p);

   man = e_manager_current_get();
   zone = e_util_zone_current_get(man);

   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }

   bl = e_container_border_list_first(e_container_current_get(man));
   while ((bd = e_container_border_list_next(bl)))
     {
	if (zone == bd->zone)
	  {
	     if (!input)
	       _item_add(p, bd, 1);
	     else if (bd->client.icccm.name &&
		      e_util_glob_case_match(bd->client.icccm.name, match1))
	       _item_add(p, bd, 1);
	     else  if (e_util_glob_case_match(e_border_name_get(bd), match1))
	       _item_add(p, bd, 1);
	     else if (bd->client.icccm.name &&
		      e_util_glob_case_match(bd->client.icccm.name, match2))
	       _item_add(p, bd, 2);
	     else if (e_util_glob_case_match(e_border_name_get(bd), match2))
	       _item_add(p, bd, 2);
	  }
     }
   e_container_border_list_free(bl);

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	return 1;
     }

   return 0;
}

static void
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e)
{
   it->o_icon = e_border_icon_add(((E_Border *)it->data[0]), e);
}

static void
_item_add(Evry_Plugin *p, E_Border *bd, int prio)
{
   Evry_Item *it;

   it = E_NEW(Evry_Item, 1);
   /* e_object_ref(E_OBJECT(bd)); */
   it->data[0] = bd;
   it->priority = prio;
   it->label = eina_stringshare_add(e_border_name_get(bd));

   p->items = eina_list_append(p->items, it);
}

/* TODO sort by focus history and name? */
static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;

   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}


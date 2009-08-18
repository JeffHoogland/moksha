#include "e.h"
#include "e_mod_main.h"

static Evry_Plugin *p;

/* XXX handle border remove events */

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);
}

static void
_item_free(Evry_Item *it)
{
   /* if (it->data[0]) e_object_unref(E_OBJECT(it->data[0])); */
}

static void
_item_add(Evry_Plugin *p, E_Border *bd, int match, int *prio)
{
   Evry_Item *it;

   it = evry_item_new(p, e_border_name_get(bd), &_item_free);

   /* e_object_ref(E_OBJECT(bd)); */
   it->data[0] = bd;
   it->fuzzy_match = match;
   it->priority = *prio;

   *prio += 1;

   p->items = eina_list_append(p->items, it);
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

   return (it1->priority - it2->priority);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   E_Zone *zone;
   E_Border *bd;
   Eina_List *l;
   int prio = 0;
   int m1, m2;

   _cleanup(p);

   zone = e_util_zone_current_get(e_manager_current_get());

   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
	if (zone == bd->zone)
	  {
	     if (!input)
	       _item_add(p, bd, 0, &prio);
	     else
	       {
		  m1 = evry_fuzzy_match(e_border_name_get(bd), input);

		  if (bd->client.icccm.name)
		    {
		       m2 = evry_fuzzy_match(bd->client.icccm.name, input);
		       if (!m1 || (m2 && m2 < m1))
			 m1 = m2;
		    }

		  if (bd->desktop)
		    {
		       m2 = evry_fuzzy_match(bd->desktop->name, input);
		       if (!m1 || (m2 && m2 < m1))
			 m1 = m2;
		    }

		  if (m1)
		    _item_add(p, bd, m1, &prio);
	       }
	  }
     }

   if (p->items)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);

	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   E_Border *bd = it->data[0];

   if (bd->internal)
     {
	o = edje_object_add(e);
	if (!bd->internal_icon) 
	  e_util_edje_icon_set(o, "enlightenment/e");
	else
	  {
	     if (!bd->internal_icon_key)
	       {
		  char *ext;
		  ext = strrchr(bd->internal_icon, '.');
		  if ((ext) && ((!strcmp(ext, ".edj"))))
		    {
		       if (!edje_object_file_set(o, bd->internal_icon, "icon"))
			 e_util_edje_icon_set(o, "enlightenment/e");	       
		    }
		  else if (ext)
		    {
		       evas_object_del(o);
		       o = e_icon_add(e);
		       e_icon_file_set(o, bd->internal_icon);
		    }
		  else 
		    {
		       if (!e_util_edje_icon_set(o, bd->internal_icon))
			 e_util_edje_icon_set(o, "enlightenment/e"); 
		    }
	       }
	     else
	       {
		  edje_object_file_set(o, bd->internal_icon,
				       bd->internal_icon_key);
	       }
	  }
	return o;
     }
   
   if (!o && bd->desktop)
     o = e_util_desktop_icon_add(bd->desktop, 128, e);

   if (!o && bd->client.netwm.icons)
     {
	int i, size, tmp, found = 0;
	o = e_icon_add(e);

	size = bd->client.netwm.icons[0].width;

	for (i = 1; i < bd->client.netwm.num_icons; i++)
	  {
	     if ((tmp = bd->client.netwm.icons[i].width) > size)
	       {
		  size = tmp;
		  found = i;
	       }
	  }

	e_icon_data_set(o, bd->client.netwm.icons[found].data,
			bd->client.netwm.icons[found].width,
			bd->client.netwm.icons[found].height);
	e_icon_alpha_set(o, 1);
	return o;
     }

   if (!o)
     o = e_border_icon_add(bd, e);

   return o;
}

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Windows";
   p->type = type_subject;
   p->type_in  = "NONE";
   p->type_out = "BORDER";
   p->fetch = &_fetch;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   evry_plugin_register(p, 2);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p);
   E_FREE(p);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

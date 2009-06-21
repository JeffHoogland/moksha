#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  const char *mime;
  Eina_List *apps;
  Evry_Item *candidate;
};

static Evry_Plugin *_plug_file_open_new();
static void _plug_file_open_free(Evry_Plugin *p);
static int  _plug_file_open_fetch(Evry_Plugin *p, char *string);
static int  _plug_file_open_begin(Evry_Plugin *p, Evry_Item *item);
static int  _plug_file_open_action(Evry_Plugin *p, Evry_Item *item);
static void _plug_file_open_cleanup(Evry_Plugin *p);
static void _plug_file_open_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, int prio);
static int  _plug_file_open_cb_sort(const void *data1, const void *data2);
static void _plug_file_open_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin_Class class;

EAPI int
evry_plug_file_open_init(void)
{
   class.name = "Open With...";
   class.type_in  = "FILE";
   class.type_out = "NONE";
   class.need_query = 0;
   class.new = &_plug_file_open_new;
   class.free = &_plug_file_open_free;
   evry_plugin_register(&class);
   
   return 1;
}

EAPI int
evry_plug_file_open_shutdown(void)
{
   evry_plugin_unregister(&class);
   
   return 1;
}

static Evry_Plugin *
_plug_file_open_new()
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->class = &class;
   p->begin = &_plug_file_open_begin;
   p->fetch = &_plug_file_open_fetch;
   p->action = &_plug_file_open_action;
   p->cleanup = &_plug_file_open_cleanup;
   p->icon_get = &_plug_file_open_item_icon_get;
   p->items = NULL;   

   Inst *inst = E_NEW(Inst, 1);
   p->priv = inst;

   return p;
}

static void
_plug_file_open_free(Evry_Plugin *p)
{
   Inst *inst = p->priv;
   
   _plug_file_open_cleanup(p);
   if (inst->apps) eina_list_free(inst->apps);

   E_FREE(inst);
   E_FREE(p);
}



static int
_plug_file_open_begin(Evry_Plugin *p, Evry_Item *it)
{
   Inst *inst;
   
   _plug_file_open_cleanup(p);

   if (!it || !it->uri) return 0;
   inst = p->priv;
   inst->candidate = it;
   
   if (!it->mime)
     inst->mime = efreet_mime_type_get(it->uri);
   else
     inst->mime = it->mime;
   
   if (!inst->mime) return 0;

   /* if (!strcmp(mime, "Folder"))
    *   {
    * 	apps = 
    *   }
    * else */
     {
	inst->apps = efreet_util_desktop_mime_list(inst->mime);
     }
   
   return 1;
}

static int
_plug_file_open_action(Evry_Plugin *p, Evry_Item *it)
{
   E_Zone *zone;
   Efreet_Desktop *desktop;
   Eina_List *files = NULL;
   Inst *inst = p->priv;
   
   desktop = it->data[0];
   files = eina_list_append(files, inst->candidate->uri);
   zone = e_util_zone_current_get(e_manager_current_get());
   e_exec(zone, desktop, NULL, files, NULL /*"everything"*/);
   
   return 1;
}

static void
_plug_file_open_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static int
_plug_file_open_fetch(Evry_Plugin *p, char *string)
{
   Eina_List *l;
   Efreet_Desktop *desktop;
   char match1[4096];
   char match2[4096];
   Inst *inst = p->priv;
   
   _plug_file_open_cleanup(p);

   if (!string)
     {
	EINA_LIST_FOREACH(inst->apps, l, desktop)
	  _plug_file_open_item_add(p, desktop, 1);
     }
   else
     {
	snprintf(match1, sizeof(match1), "%s*", string);
	snprintf(match2, sizeof(match2), "*%s*", string);

	EINA_LIST_FOREACH(inst->apps, l, desktop)
	  {
	     if (desktop->name)
	       {		  
		  if (e_util_glob_case_match(desktop->name, match1))
		    _plug_file_open_item_add(p, desktop, 1);
		  else if (e_util_glob_case_match(desktop->name, match2))
		    _plug_file_open_item_add(p, desktop, 2);
		  else if (desktop->comment)
		    {
		       if (e_util_glob_case_match(desktop->comment, match1))
			 _plug_file_open_item_add(p, desktop, 3);
		       else if (e_util_glob_case_match(desktop->comment, match2))
			 _plug_file_open_item_add(p, desktop, 4);
		    }
	       }
	  }
     }
   
   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items,
				       eina_list_count(p->items),
				       _plug_file_open_cb_sort);
	return 1;
     }

   return 0;
}

static void
_plug_file_open_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   it->o_icon = e_util_desktop_icon_add((Efreet_Desktop *)it->data[0], 24, e);
}

static void
_plug_file_open_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, int prio)
{
   Evry_Item *it;
   Evry_App *app;

   it = calloc(1, sizeof(Evry_Item));
   it->data[0] = desktop;
   it->priority = prio;
   it->label = eina_stringshare_add(desktop->name);

   p->items = eina_list_append(p->items, it);
}

// TODO sort by focus history and name?
static int
_plug_file_open_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}

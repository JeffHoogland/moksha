#include "Evry.h"


#define MAX_ITEMS 100

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;

  const char *directory;
  /* all files of directory */
  Eina_List  *items;
  /* current list of files */
  Eina_List  *cur;
  Eina_Bool command;
  Ecore_Idler *idler;
};

static Evry_Plugin *p1;
static Evry_Plugin *p2;
static Evry_Action *act;


static Evry_Item *
_item_add(Plugin *p, const char *directory, const char *file)
{
   Evry_Item *it = NULL;
   char buf[4096];

   it = evry_item_new(&p->base, file, NULL);
   if (!it) return NULL;

   snprintf(buf, sizeof(buf), "%s/%s", directory, file);
   it->uri = eina_stringshare_add(buf);

   return it;
}

static void
_item_fill(Evry_Item *it)
{
   const char *mime;

   if (it->mime) return;

   if (ecore_file_is_dir(it->uri))
     {
	it->mime = eina_stringshare_add("x-directory/normal");
	it->browseable = EINA_TRUE;
	return;
     }

   /* if ((ext = strrchr(it->label, '.')))
    *   {
    * 	if (!strcmp(ext, ".desktop") || !strcmp(ext, ".directory"))
    * 	  {
    * 	     Efreet_Desktop *desktop;
    * 	     desktop = efreet_desktop_new(it->uri);
    * 	     if (!desktop) return;
    * 	     eina_stringshare_del(it->label);
    * 	     it->label = eina_stringshare_add(desktop->name);
    * 	     it->mime = eina_stringshare_add("None");
    * 	     efreet_desktop_free(desktop);
    * 	     return;
    * 	  }
    *   } */

   if ((mime = efreet_mime_type_get(it->uri)))
     {
	it->mime = eina_stringshare_add(mime);
	return;
     }

   it->mime = eina_stringshare_add("None");
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->browseable && !it2->browseable)
     return -1;

   if (!it1->browseable && it2->browseable)
     return 1;

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

   return strcasecmp(it1->label, it2->label);
}

static int
_dirbrowse_idler(void *data)
{
   Plugin *p = data;
   Eina_List *l;
   Evry_Item *it;
   int cnt = 10;

   EINA_LIST_FOREACH(p->items, l, it)
     {
	if (!it->mime)
	  {
	     _item_fill(it);
	     cnt--;
	  }
	if (cnt == 0) break;
     }

   l = p->base.items;

   p->base.items = eina_list_sort(l, eina_list_count(l), _cb_sort);
   evry_plugin_async_update(&p->base, EVRY_ASYNC_UPDATE_ADD);

   if (cnt > 0)
     {
	p->idler = NULL;
	return 0;
     }

   return 1;
}

static void
_read_directory(Plugin *p)
{
   char *file;
   Eina_List *files;
   Evry_Item *it;

   files = ecore_file_ls(p->directory);

   EINA_LIST_FREE(files, file)
     {
	it = NULL;

	if (file[0] == '.')
	  {
	     free(file);
	     continue;
	  }

	it = _item_add(p, p->directory, file);

	if (it)
	  p->items = eina_list_append(p->items, it);

	free(file);
     }

   p->idler = ecore_idler_add(_dirbrowse_idler, p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p;

   /* is FILE ? */
   if (it && (it->plugin->type_out == plugin->type_in))
     {
	if (!it->uri || !ecore_file_is_dir(it->uri))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	
	p->directory = eina_stringshare_add(it->uri);
     }
   else   
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->directory = eina_stringshare_add(e_user_homedir_get());
     }
   
   _read_directory(p);

   return &p->base;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it;

   if (p->directory)
     eina_stringshare_del(p->directory);

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   if (p->idler)
     ecore_idler_del(p->idler);

   if (plugin->items)
     eina_list_free(plugin->items);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it;
   Eina_List *l;
   int cnt = 0;

   if (!p->command)
     {
	if (p->base.items) eina_list_free(p->base.items);
	p->base.items = NULL;
     }

   /* input is command ? */
   if (input)
     {
   	if (!strncmp(input, "/", 1))
   	  {
   	     if (p->command) return 1;

   	     it = evry_item_new(&p->base, "/", NULL);
   	     if (!it) return 0;
   	     it->uri = eina_stringshare_add("/");
   	     p->base.items = eina_list_append(p->base.items, it);
   	     p->command = EINA_TRUE;
   	     return 1;
   	  }
   	else if (!strncmp(input, "..", 2))
   	  {
   	     char *end;
   	     char dir[4096];
   	     char *tmp;
   	     int prio = 0;

   	     if (p->command) return 1;

   	     if (!strcmp(p->directory, "/")) return 0;

   	     snprintf(dir, 4096, "%s", p->directory);
   	     end = strrchr(dir, '/');

   	     while (end != dir)
   	       {
   		  tmp = strdup(dir);
   		  snprintf(dir, (end - dir) + 1, "%s", tmp);
   	          it = evry_item_new(&p->base, dir, NULL);
   		  if (!it) break;
   		  it->uri = eina_stringshare_add(dir);
   		  it->priority = prio;
		  p->base.items = eina_list_append(p->base.items, it);
   		  end = strrchr(dir, '/');
   		  free(tmp);
   		  prio--;
   	       }

   	     it = evry_item_new(&p->base, "/", NULL);
   	     if (!it) return 0;
   	     it->uri = eina_stringshare_add("/");
   	     it->priority = prio;
	     p->base.items = eina_list_append(p->base.items, it);
   	     p->command = EINA_TRUE;
   	     return 1;
   	  }
     }

   if (p->command)
     {
   	p->command = EINA_FALSE;
   	EINA_LIST_FREE(p->base.items, it)
   	  evry_item_free(it);
     }

   EINA_LIST_FOREACH(p->items, l, it)
     {
	if (input)
	  {
	     int match;
	     if ((match = evry_fuzzy_match(it->label, input)))
	       it->fuzzy_match = match;
	     else
	       it = NULL;
	  }

	if (it)
	  {
   	     p->base.items = eina_list_append(p->base.items, it);
	     if (cnt++ >= MAX_ITEMS) break;
	  }
     }

   if (p->base.items)
     {
	p->base.items = eina_list_sort
	  (p->base.items, eina_list_count(p->base.items), _cb_sort);

	return 1;
     }

   return 0;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;

   if (!it->mime)
     _item_fill((Evry_Item *)it);

   if (!it->mime) return NULL;

   if (it->browseable)
     o = evry_icon_theme_get("folder", e);
   else
     o = evry_icon_mime_get(it->mime, e);

   return o;
}


static int
_open_folder_check(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   return (it->browseable && e_action_find("fileman"));
}

static int
_open_folder_action(Evry_Action *act)
{
   E_Action *action = e_action_find("fileman");
   char *path;
   Eina_List *m;

   if (!action) return 0;

   m = e_manager_list();

   if (!act->item1->browseable)
     {
	path = ecore_file_dir_get(act->item1->uri);
	if (!path) return 0;
	action->func.go(E_OBJECT(m->data), path);
	free(path);
     }
   else
     {
	action->func.go(E_OBJECT(m->data), act->item1->uri);
     }

   return 1;
}


static Eina_Bool
_init(void)
{
   p1 = evry_plugin_new(NULL, "Files", type_subject, "FILE", "FILE", 0, NULL, NULL,
			_begin, _cleanup, _fetch, NULL, _icon_get,
			NULL, NULL);

   p2 = evry_plugin_new(NULL, "Files", type_object, "FILE", "FILE", 0, NULL, NULL,
			_begin, _cleanup, _fetch, NULL, _icon_get,
			NULL, NULL);

   evry_plugin_register(p1, 3);
   evry_plugin_register(p2, 1);

   act = evry_action_new("Open Folder (EFM)", "FILE", NULL, NULL, "folder-open",
			 _open_folder_action, _open_folder_check, NULL, NULL, NULL);

   evry_action_register(act);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_free(p1, 1);
   evry_plugin_free(p2, 1);

   evry_action_free(act);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

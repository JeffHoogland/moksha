#include "Evry.h"


#define MAX_ITEMS 100

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;

  const char *directory;
  /* all files of directory */
  Eina_List  *files;
  /* current list of files */
  Eina_List  *cur;
  Eina_Bool command;
  Ecore_Idle_Enterer *idler;
};

static Evry_Plugin *p1;
static Evry_Plugin *p2;
static Evry_Action *act;




static void
_item_fill(Evry_Item_File *file)
{
   const char *mime;

   if (file->mime) return;

   if (ecore_file_is_dir(file->uri))
     {
	file->mime = eina_stringshare_add("x-directory/normal");
	EVRY_ITEM(file)->browseable = EINA_TRUE;
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

   if ((mime = efreet_mime_type_get(file->uri)))
     {
	file->mime = eina_stringshare_add(mime);
	return;
     }

   file->mime = eina_stringshare_add("None");
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
   Evry_Item_File *file;
   int cnt = 20;

   if (!p->idler) return 0;
   
   EINA_LIST_FOREACH(p->files, l, file)
     {
	if (!file->mime)
	  {
	     _item_fill(file);
	     cnt--;
	  }
	if (cnt == 0) break;
     }

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);

   if (cnt > 0)
     {
	p->idler = NULL;
	return 0;
     }

   e_util_wakeup();
   
   return 1;
}

static void
_item_free(Evry_Item *it)
{
   ITEM_FILE(file, it);
   if (file->uri) eina_stringshare_del(file->uri);
   if (file->mime) eina_stringshare_del(file->mime);

   E_FREE(file);
}

static void
_read_directory(Plugin *p)
{
   char *filename;
   Eina_List *files;
   char buf[4096];
   Evry_Item_File *file;

   files = ecore_file_ls(p->directory);

   EINA_LIST_FREE(files, filename)
     {
	if (filename[0] == '.')
	  {
	     free(filename);
	     continue;
	  }

	file = E_NEW(Evry_Item_File, 1);
	if (!file) return;

	evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), filename, _item_free);

	/* TODO one could have a function uri() instead that puts
	   together dir and file name when needed */
	snprintf(buf, sizeof(buf), "%s/%s", p->directory, filename);
	file->uri = eina_stringshare_add(buf);

	if (file)
	  p->files = eina_list_append(p->files, file);

	free(filename);
     }

   p->idler = ecore_idle_enterer_before_add(_dirbrowse_idler, p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   /* is FILE ? */
   if (it && (it->plugin->type_out == plugin->type_in))
     {
	ITEM_FILE(file, it);

	if (!file->uri || !ecore_file_is_dir(file->uri))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;

	p->directory = eina_stringshare_add(file->uri);
     }
   else
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->directory = eina_stringshare_add(e_user_homedir_get());
     }

   _read_directory(p);

   return EVRY_PLUGIN(p);
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);

   Evry_Item_File *file;

   if (p->directory)
     eina_stringshare_del(p->directory);

   EINA_LIST_FREE(p->files, file)
     evry_item_free(EVRY_ITEM(file));

   if (p->idler)
     ecore_idle_enterer_del(p->idler);
   p->idler = NULL;
   
   EVRY_PLUGIN_ITEMS_CLEAR(p);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Evry_Item_File *file;
   Eina_List *l;
   int cnt = 0;
   int match;

   if (!p->command)
     EVRY_PLUGIN_ITEMS_CLEAR(p);

   /* input is command ? */
   if (input)
     {
   	if (!strncmp(input, "/", 1))
   	  {
	     if (p->command) return 1;

	     file = E_NEW(Evry_Item_File, 1);
	     if (file)
	       {
		  evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), "/", _item_free);
		  file->uri = eina_stringshare_add("/");
		  EVRY_PLUGIN_ITEM_APPEND(p, file);
		  p->command = EINA_TRUE;
		  return 1;
	       }
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

		  file = E_NEW(Evry_Item_File, 1);
		  if (file)
		    {
		       evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), dir, _item_free);
		       file->uri = eina_stringshare_add(dir);
		       EVRY_ITEM(file)->priority = prio;
		       EVRY_PLUGIN_ITEM_APPEND(p, file);
		    }
   		  end = strrchr(dir, '/');
   		  free(tmp);
   		  prio--;
   	       }

	     file = E_NEW(Evry_Item_File, 1);
	     if (file)
	       {
		  evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), "/", _item_free);
		  file->uri = eina_stringshare_add("/");
		  EVRY_ITEM(file)->priority = prio;
		  EVRY_PLUGIN_ITEM_APPEND(p, file);
	       }

   	     p->command = EINA_TRUE;
   	     return 1;
   	  }
     }

   if (p->command)
     {
   	p->command = EINA_FALSE;
	EVRY_PLUGIN_ITEMS_FREE(p);
     }

   EINA_LIST_FOREACH(p->files, l, file)
     {
	if (input)
	  {
	     if ((match = evry_fuzzy_match(EVRY_ITEM(file)->label, input)))
	       EVRY_ITEM(file)->fuzzy_match = match;
	     else
	       file = NULL;
	  }

	if (file)
	  {
	     EVRY_PLUGIN_ITEM_APPEND(p, file);

	     if (cnt++ >= MAX_ITEMS) break;
	  }
     }

   if (!EVRY_PLUGIN(p)->items)
     return 0;

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   ITEM_FILE(file, it);

   if (!file->mime)
     _item_fill(file);

   if (!file->mime) return NULL;

   if (it->browseable)
     o = evry_icon_theme_get("folder", e);
   else
     o = evry_icon_mime_get(file->mime, e);

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

   ITEM_FILE(file, act->item1);

   if (!act->item1->browseable)
     {
	path = ecore_file_dir_get(file->uri);
	if (!path) return 0;
	action->func.go(E_OBJECT(m->data), path);
	free(path);
     }
   else
     {
	action->func.go(E_OBJECT(m->data), file->uri);
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
   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);

   evry_action_free(act);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

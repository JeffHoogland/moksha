#include "e.h"
#include "e_mod_main.h"

typedef struct _State State;

struct _State
{
  const char *directory;
  Eina_List  *items;
};

static int  _begin(Evry_Item *it);
static int  _fetch(const char *input);
static int  _action(Evry_Item *it, const char *input);
static void _cleanup(void);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Item *it, Evas *e);
static void _list_free(void);
static Evry_Item *_item_fill(const char *directory, const char *file);

static Evry_Plugin *p;

EAPI int
evry_plug_dir_browse_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Browse Files";
   p->type_in  = "NONE|FILE";
   p->type_out = "FILE";
   p->prio = 2;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;

   evry_plugin_register(p);

   return 1;
}

EAPI int
evry_plug_dir_browse_shutdown(void)
{
   evry_plugin_unregister(p);
   E_FREE(p);
   
   return 1;
}

static int
_begin(Evry_Item *it)
{
   State *s;
   
   if (it)
     {
	if (!it->uri || !ecore_file_is_dir(it->uri))
	  return 0;

	s = E_NEW(State, 1);     
	s->directory = eina_stringshare_add(it->uri);
     }
   else
     {
	s = E_NEW(State, 1);     
	s->directory = eina_stringshare_add(e_user_homedir_get());	
     }

   p->states = eina_list_prepend(p->states, s);
   p->items = NULL;
   
   return 1;
}

static int
_action(Evry_Item *it, const char *input)
{   
   return 0;
}

static void
_list_free()
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	if (it->uri) eina_stringshare_del(it->uri);
	if (it->mime) eina_stringshare_del(it->mime);

	free(it);
     }
}


static void
_cleanup()
{
   State *s;
   
   if (!p->states) return;

   s = p->states->data;
   
   _list_free();
   
   eina_stringshare_del(s->directory);
   
   E_FREE(s);

   p->states = eina_list_remove_list(p->states, p->states);

   if (p->states)
     {
	s = p->states->data;
	p->items = s->items;
     }
}

/* based on directory-watcher from drawer module  */
static int
_fetch(const char *input)
{
   Eina_List *files;
   char *file;
   Evry_Item *it;
   char match1[4096];
   char match2[4096];
   State *s = p->states->data;
   
   _list_free();
   
   files = ecore_file_ls(s->directory);

   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }

   EINA_LIST_FREE(files, file)
     {
	it = NULL;
	
	if (file[0] == '.')
	  {
	     free(file);
	     continue;
	  }
	
	if (input)
	  {
	     if (e_util_glob_case_match(file, match1))
	       {
		  it  = _item_fill(s->directory, file);
		  it->priority += 1;
	       }
	     else if (e_util_glob_case_match(file, match2))
	       {
		  it = _item_fill(s->directory, file);
	       }
	  }
	else
	  {
	     it  = _item_fill(s->directory, file);
	  }
	
	if (it)
	  p->items = eina_list_append(p->items, it);

	free(file);
     }

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items),
				  _cb_sort);
	s->items = p->items;
	
	return 1;
     }

   return 0;
}

/* based on directory-watcher from drawer module  */
static Evry_Item *
_item_fill(const char *directory, const char *file)
{
   Evry_Item *it = NULL;
   char buf[4096];
   const char *mime, *file_path;

   it = E_NEW(Evry_Item, 1);

   snprintf(buf, sizeof(buf), "%s/%s", directory, file);
   
   if ((e_util_glob_case_match(file, "*.desktop")) ||
       (e_util_glob_case_match(file, "*.directory")))
     {
	Efreet_Desktop *desktop;

	desktop = efreet_desktop_new(buf);
	if (!desktop) return NULL;
	it->label = eina_stringshare_add(desktop->name);
	efreet_desktop_free(desktop);
     }
   else
     it->label = eina_stringshare_add(file);

   file_path = eina_stringshare_add(buf);

   it->uri = file_path;
   
   mime = efreet_mime_globs_type_get(file_path);
   if (mime)
     {
	it->mime = eina_stringshare_add(mime);
	it->priority = 0;
     }
   else if (ecore_file_is_dir(file_path))
     {
	it->mime = eina_stringshare_add("Folder");
	it->priority = 1;
     }
   else if ((mime = efreet_mime_type_get(file_path)))
     {
	it->mime = eina_stringshare_add(mime);
	it->priority = 0;
     }
   else	
     {
	it->mime = eina_stringshare_add("None");
	it->priority = 0;
     }
   
   return it;
}

static void
_item_icon_get(Evry_Item *it, Evas *e)
{
   char *item_path;

   if (!it->mime) return;
   
   if (!strcmp(it->mime, "Folder"))
     {
	it->o_icon = edje_object_add(e);
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/folder");
     }
   else
     {
	item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 32);

	if (item_path)
	  it->o_icon = e_util_icon_add(item_path, e);
	else
	  {
	     it->o_icon = edje_object_add(e);
	     e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/fileman/file");
	  }
     }
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   if (it2->priority - it1->priority) 
     return (it2->priority - it1->priority);
   else
     return strcasecmp(it1->label, it2->label);
}

#include "e.h"
#include "e_mod_main.h"

typedef struct _State State;

struct _State
{
  const char *directory;
  Eina_List  *items;
  int command;
};

static int  _begin(Evry_Plugin *p, Evry_Item *it);
static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *it, const char *input);
static void _cleanup(Evry_Plugin *p);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static void _list_free(Evry_Plugin *p);
static void _item_fill(Evry_Item *it, Evas *evas);
static Evry_Item *_item_add(const char *directory, const char *file);
static void _realize(Evry_Plugin *p, Evas *e);

static Evry_Plugin *p;
static Eina_List *stack = NULL;


EAPI int
evry_plug_dir_browse_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Browse Files";
   p->type_in  = "NONE|FILE";
   p->type_out = "FILE";
   /* p->trigger = "/"; */
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->realize_items = &_realize;
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
_begin(Evry_Plugin *p, Evry_Item *it)
{
   State *s;

   if (stack)
     {
	s = stack->data;
	if (s->command) evry_clear_input();
     }
   
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

   stack = eina_list_prepend(stack, s);
   p->items = NULL;

   return 1;
}

static int
_action(Evry_Plugin *p, Evry_Item *it, const char *input)
{
   return EVRY_ACTION_OTHER;
}

static void
_list_free(Evry_Plugin *p)
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
_cleanup(Evry_Plugin *p)
{
   State *s;

   if (!stack) return;

   s = stack->data;

   _list_free(p);

   eina_stringshare_del(s->directory);

   E_FREE(s);

   stack = eina_list_remove_list(stack, stack);

   if (stack)
     {
	s = stack->data;
	p->items = s->items;
     }
}

/* based on directory-watcher from drawer module  */
static int
_fetch(Evry_Plugin *p, const char *input)
{
   Eina_List *files;
   char *file;
   const char *directory = NULL;
   Evry_Item *it;
   char match1[4096];
   char match2[4096];
   State *s = stack->data;

   _list_free(p);

   /* input is command ? */
   if (input)
     {
	if (!strncmp(input, "/", 1))
	  {
	     it = E_NEW(Evry_Item, 1);
	     it->uri = eina_stringshare_add("/");
	     it->label = eina_stringshare_add("/");
	     p->items = eina_list_append(p->items, it);
	     s->command = 1;
	     return 1;
	     
	  }
	else if (!strncmp(input, "..", 2))
	  {
	     char *end;
	     char dir[4096];
	     char *tmp;
	     int prio = 0;

	     if (!strcmp(s->directory, "/")) return 0;
	     
	     snprintf(dir, 4096, "%s", s->directory);
	     end = strrchr(dir, '/');	     

	     while (end != dir)
	       {
		  tmp = strdup(dir);
		  snprintf(dir, (end - dir) + 1, "%s", tmp);
	          it = E_NEW(Evry_Item, 1);
		  it->uri = eina_stringshare_add(dir);
		  it->label = eina_stringshare_add(dir);
		  it->priority = prio;
		  p->items = eina_list_append(p->items, it);
		  end = strrchr(dir, '/');
		  free(tmp);
		  prio--;
	       }

	     it = E_NEW(Evry_Item, 1);
	     it->uri = eina_stringshare_add("/");
	     it->label = eina_stringshare_add("/");
	     it->priority = prio;
	     p->items = eina_list_append(p->items, it);

	     s->command = 1;

	     return 1;
	  }
     }
   
   if (!directory)
     directory = s->directory;

   files = ecore_file_ls(directory);

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
		  it  = _item_add(directory, file);
		  it->priority = 1;
	       }
	     else if (e_util_glob_case_match(file, match2))
	       {
		  it = _item_add(directory, file);
		  it->priority = 0;
	       }
	  }
	else
	  {
	     it = _item_add(directory, file);
	  }
	
	if (it)
	  p->items = eina_list_append(p->items, it);

	free(file);
     }

   s->items = p->items;
   
   if (p->items) return 1;
   
   return 0;
}

static Evry_Item *
_item_add(const char *directory, const char *file)
{
   Evry_Item *it = NULL;
   char buf[4096];
   
   it = E_NEW(Evry_Item, 1);

   snprintf(buf, sizeof(buf), "%s/%s", directory, file);
   it->uri = eina_stringshare_add(buf);
   it->label = eina_stringshare_add(file);

   return it;
}

static void
_realize(Evry_Plugin *p, Evas *e)
{
   Eina_List *l;
   Evry_Item *it;
   State *s = stack->data;
   
   EINA_LIST_FOREACH(p->items, l, it)
     _item_fill(it, e);

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	s->items = p->items;
     }
}


/* based on directory-watcher from drawer module  */
static void
_item_fill(Evry_Item *it, Evas *e)
{
   const char *mime;

   if  (it->mime) return;
   
   if ((e_util_glob_case_match(it->label, "*.desktop")) ||
       (e_util_glob_case_match(it->label, "*.directory")))
     {
	Efreet_Desktop *desktop;

	desktop = efreet_desktop_new(it->uri);
	if (!desktop) return;
	it->label = eina_stringshare_add(desktop->name);
	efreet_desktop_free(desktop);
     }

   mime = efreet_mime_globs_type_get(it->uri);
   if (mime)
     {
	it->mime = eina_stringshare_add(mime);
     }
   else if (ecore_file_is_dir(it->uri))
     {
	it->mime = eina_stringshare_add("Folder");
	it->priority += 1;
	it->o_icon = edje_object_add(e);
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/folder");
     }
   else if ((mime = efreet_mime_type_get(it->uri)))
     {
	it->mime = eina_stringshare_add(mime);
     }
   else
     {
	it->mime = eina_stringshare_add("None");
     }

   if (strcmp(it->mime, "Folder"))
     {
	char *item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 32);

	if (item_path)
	  it->o_icon = e_util_icon_add(item_path, e);
	else
	  {
	     it->o_icon = edje_object_add(e);
	     e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/fileman/file");
	  }
     }
}

static void
_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
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

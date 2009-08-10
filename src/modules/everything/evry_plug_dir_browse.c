#include "e.h"
#include "e_mod_main.h"

#define MAX_ITEMS 100

typedef struct _State State;

struct _State
{
  const char *directory;
  /* all files of directory */
  Eina_List  *items;
  /* current list of files */
  Eina_List  *cur;
  int command;
};

static int  _begin(Evry_Plugin *p, Evry_Item *it);
static int  _fetch(Evry_Plugin *p, const char *input);
static void _cleanup(Evry_Plugin *p);
static int  _cb_sort(const void *data1, const void *data2);
static Evas_Object *_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static void _item_fill(Evry_Item *it);
static Evry_Item *_item_add(const char *directory, const char *file);
static void _realize(Evry_Plugin *p, Evas *e);
static int  _dirbrowse_idler(void *data);

static Eina_Bool _init(void);
static void _shutdown(void);
EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

static Evry_Plugin *p;
static Evry_Plugin *p2;
static Ecore_Idler *idler = NULL;

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Files";
   p->type = type_subject;
   p->type_in  = "NONE|FILE";
   p->type_out = "FILE";
   p->browseable = EINA_TRUE;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->cleanup = &_cleanup;
   p->realize_items = &_realize;
   p->icon_get = &_item_icon_get;

   evry_plugin_register(p);

   p2 = E_NEW(Evry_Plugin, 1);
   p2->name = "Files";
   p2->type = type_object;
   p2->type_in  = "NONE|FILE";
   p2->type_out = "FILE";
   p2->browseable = EINA_TRUE;
   p2->begin = &_begin;
   p2->fetch = &_fetch;
   p2->cleanup = &_cleanup;
   p2->realize_items = &_realize;
   p2->icon_get = &_item_icon_get;

   evry_plugin_register(p2);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p);
   evry_plugin_unregister(p2);
   E_FREE(p);
   E_FREE(p2);
}

static int
_begin(Evry_Plugin *p, Evry_Item *it)
{
   State *s;
   char *file;
   Eina_List *files;
   Eina_List *stack = p->private;

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
	p->items = NULL;
     }
   else
     {
	s = E_NEW(State, 1);
	s->directory = eina_stringshare_add(e_user_homedir_get());
	p->items = NULL;
     }
   
   files = ecore_file_ls(s->directory);

   EINA_LIST_FREE(files, file)
     {
	it = NULL;

	if (file[0] == '.')
	  {
	     free(file);
	     continue;
	  }

	it = _item_add(s->directory, file);
	
	if (it)
	  s->items = eina_list_append(s->items, it);

	free(file);
     }

   if (idler)
     ecore_idler_del(idler);

   idler = ecore_idler_add(_dirbrowse_idler, p);
   
   stack = eina_list_prepend(stack, s);

   p->private = stack;
   
   return 1;
}

static void
_cleanup(Evry_Plugin *p)
{
   State *s;
   Evry_Item *it;
   Eina_List *stack = p->private;
   
   if (!stack) return;
   
   s = stack->data;

   if (s->directory) eina_stringshare_del(s->directory);

   EINA_LIST_FREE(s->items, it)
     {
	if (it->uri) eina_stringshare_del(it->uri);
	if (it->mime) eina_stringshare_del(it->mime);
	evry_item_free(it);
     }
   
   if (idler)
     {
	ecore_idler_del(idler);
	idler = NULL;
     }
   
   E_FREE(s);

   eina_list_free(p->items);
   p->items = NULL;
   
   stack = eina_list_remove_list(stack, stack);
   p->private = stack;
   if (stack)
     {
	s = stack->data;
	
	p->items = s->cur;
     }
}

/* based on directory-watcher from drawer module  */
static int
_fetch(Evry_Plugin *p, const char *input)
{
   const char *directory = NULL;
   Evry_Item *it;
   Eina_List *l;
   char match1[4096];
   char match2[4096];
   int cnt = 0;
   Eina_List *stack = p->private;
   State *s = stack->data;

   if (p->items) eina_list_free(p->items);
   p->items = NULL;
   
   /* input is command ? */
   if (input)
     {
	/* XXX free s->items?  */
	if (!strncmp(input, "/", 1))
	  {
	     it = evry_item_new(p, "/");
	     if (!it) return 0;
	     it->uri = eina_stringshare_add("/");
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
	          it = evry_item_new(p, dir);
		  if (!it) return 0;
		  it->uri = eina_stringshare_add(dir);
		  it->priority = prio;
		  p->items = eina_list_append(p->items, it);
		  end = strrchr(dir, '/');
		  free(tmp);
		  prio--;
	       }

	     it = evry_item_new(p, "/");
	     if (!it) return 0;
	     it->uri = eina_stringshare_add("/");
	     it->priority = prio;
	     p->items = eina_list_append(p->items, it);
	     s->command = 1;

	     return 1;
	  }
     }
   
   if (!directory)
     directory = s->directory;

   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }

   EINA_LIST_FOREACH(s->items, l, it)
     {
	if (input)
	  {
	     /* TODO fix sort priority */
	     if (e_util_glob_case_match(it->label, match1))
	       it->priority += 2;
	     else if (e_util_glob_case_match(it->label, match2))
	       it->priority += 1;
	     else it = NULL;
	  }

	if (it)
	  {
	     p->items = eina_list_append(p->items, it);
	     cnt++;
	     if (cnt >= MAX_ITEMS) break;
	  }
     }

   if (p->items)
     {
	s->cur = p->items;
	return 1;
     }
   
   return 0;
}

static Evry_Item *
_item_add(const char *directory, const char *file)
{
   Evry_Item *it = NULL;
   char buf[4096];
   
   it = evry_item_new(p, file);
   if (!it) return NULL;
   
   snprintf(buf, sizeof(buf), "%s/%s", directory, file);
   it->uri = eina_stringshare_add(buf);

   return it;
}

static void
_realize(Evry_Plugin *p, Evas *e)
{
   Eina_List *l;
   Evry_Item *it;
   Eina_List *stack = p->private;
   State *s = stack->data;
   
   /* EINA_LIST_FOREACH(p->items, l, it)
    *   _item_fill(it, e); */

   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	s->cur = p->items;
     }
}


/* based on directory-watcher from drawer module  */
static void
_item_fill(Evry_Item *it)
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
	it->browseable = EINA_TRUE;
	it->priority += 1;
     }
   else if ((mime = efreet_mime_type_get(it->uri)))
     {
	it->mime = eina_stringshare_add(mime);
     }
   else
     {
	it->mime = eina_stringshare_add("None");
     }
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;   
   char *item_path;

   if (!it->mime)
     _item_fill(it);

   if (!it->mime) return NULL;

   if (!strcmp(it->mime, "Folder"))
     {
	o = e_icon_add(e); 
	evry_icon_theme_set(o, "folder");
     }
   else
     {
	item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 64);

	if (item_path)
	  o = e_util_icon_add(item_path, e);
	if (!o)
	  {
	     o = e_icon_add(e); 
	     evry_icon_theme_set(o, "none");
	  }
     }
   return o;   
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

static int
_dirbrowse_idler(void *data)
{
   Evry_Plugin *p = data;
   State *s = ((Eina_List *)p->private)->data;
   int cnt = 10;
   Eina_List *l;
   Evry_Item *it;

   /* printf("dirbrowse idler\n"); */
   
   if (!idler) return 0;
   
   EINA_LIST_FOREACH(s->items, l, it)
     {
	if (!it->mime)
	  {
	     _item_fill(it);
	     cnt--;
	  }
	if (cnt == 0) break;
     }

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_CLEAR);
   
   if (eina_list_count(p->items) > 0)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	s->cur = p->items;
     }

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);
   
   if (cnt > 0)
     {
	idler = NULL;
	return 0;
     }

   return 1;
}

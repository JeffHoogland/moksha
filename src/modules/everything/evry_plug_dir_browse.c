#include "Evry.h"


#define MAX_ITEMS 100

typedef struct _State State;

struct _State
{
  const char *directory;
  /* all files of directory */
  Eina_List  *items;
  /* current list of files */
  Eina_List  *cur;
  Eina_Bool command;
};

static Evry_Plugin *p1;
static Evry_Plugin *p2;
static Ecore_Idler *idler = NULL;


static Evry_Item *
_item_add(Evry_Plugin *p, const char *directory, const char *file)
{
   Evry_Item *it = NULL;
   char buf[4096];

   it = evry_item_new(p, file, NULL);
   if (!it) return NULL;

   snprintf(buf, sizeof(buf), "%s/%s", directory, file);
   it->uri = eina_stringshare_add(buf);

   return it;
}

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
	it->mime = eina_stringshare_add("x-directory/normal");
	it->browseable = EINA_TRUE;
	it->priority = 1;
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
   Evry_Plugin *p = data;
   State *s = ((Eina_List *)p->private)->data;
   int cnt = 10;
   Eina_List *l;
   Evry_Item *it;

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

   if (!s->command)
     {
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
     }

   return 1;
}

static void
_push_directory(Evry_Plugin *p, State *s)
{
   char *file;
   Eina_List *files;
   Evry_Item *it;
   Eina_List *stack = p->private;

   /* previous states items are saved in s->items !*/
   p->items = NULL;

   files = ecore_file_ls(s->directory);

   EINA_LIST_FREE(files, file)
     {
	it = NULL;

	if (file[0] == '.')
	  {
	     free(file);
	     continue;
	  }

	it = _item_add(p, s->directory, file);

	if (it)
	  s->items = eina_list_append(s->items, it);

	free(file);
     }

   if (idler)
     ecore_idler_del(idler);

   idler = ecore_idler_add(_dirbrowse_idler, p);

   stack = eina_list_prepend(stack, s);
   p->private = stack;
}

static int
_begin(Evry_Plugin *p, const Evry_Item *item __UNUSED__)
{
   State *s;

   s = E_NEW(State, 1);
   s->directory = eina_stringshare_add(e_user_homedir_get());
   _push_directory(p, s);

   return 1;
}

static int
_browse(Evry_Plugin *p, const Evry_Item *it_file)
{
   State *s;

   if (!it_file || !it_file->uri || !ecore_file_is_dir(it_file->uri))
     return 0;

   s = E_NEW(State, 1);
   s->directory = eina_stringshare_add(it_file->uri);
   _push_directory(p, s);

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
     evry_item_free(it);

   if (idler)
     {
	ecore_idler_del(idler);
	idler = NULL;
     }

   E_FREE(s);

   if (p->items) eina_list_free(p->items);
   p->items = NULL;

   stack = eina_list_remove_list(stack, stack);
   p->private = stack;

   if (stack)
     {
	s = stack->data;
	p->items = s->cur;
     }
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Item *it;
   Eina_List *l;
   int cnt = 0;
   State *s = ((Eina_List *)p->private)->data;

   if (!s->command)
     {
	if (p->items) eina_list_free(p->items);
	p->items = NULL;
     }

   /* input is command ? */
   if (input)
     {
	/* XXX free s->items?  */
	if (!strncmp(input, "/", 1))
	  {
	     if (s->command) return 1;

	     it = evry_item_new(p, "/", NULL);
	     if (!it) return 0;
	     it->uri = eina_stringshare_add("/");
	     p->items = eina_list_append(p->items, it);
	     s->cur = p->items;
	     s->command = EINA_TRUE;
	     return 1;

	  }
	else if (!strncmp(input, "..", 2))
	  {
	     char *end;
	     char dir[4096];
	     char *tmp;
	     int prio = 0;

	     if (s->command) return 1;

	     if (!strcmp(s->directory, "/")) return 0;

	     snprintf(dir, 4096, "%s", s->directory);
	     end = strrchr(dir, '/');

	     while (end != dir)
	       {
		  tmp = strdup(dir);
		  snprintf(dir, (end - dir) + 1, "%s", tmp);
	          it = evry_item_new(p, dir, NULL);
		  if (!it) return 0; /* free stuff !!!*/
		  it->uri = eina_stringshare_add(dir);
		  it->priority = prio;
		  p->items = eina_list_append(p->items, it);
		  end = strrchr(dir, '/');
		  free(tmp);
		  prio--;
	       }

	     it = evry_item_new(p, "/", NULL);
	     if (!it) return 0;
	     it->uri = eina_stringshare_add("/");
	     it->priority = prio;
	     p->items = eina_list_append(p->items, it);
	     s->cur = p->items;
	     s->command = EINA_TRUE;

	     return 1;
	  }
     }

   if (s->command)
     {
	EINA_LIST_FREE(p->items, it)
	  evry_item_free(it);

	s->command = EINA_FALSE;
     }

   EINA_LIST_FOREACH(s->items, l, it)
     {
	if (input)
	  {
	     int match;
	     if ((match = evry_fuzzy_match(it->label, input)))
	       it->priority = match;
	     else
	       it = NULL;
	  }

	if (it)
	  {
	     p->items = eina_list_prepend(p->items, it);
	     cnt++;
	     if (cnt >= MAX_ITEMS) break;
	  }
     }

   s->cur = p->items;
   if (p->items)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	s->cur = p->items;
	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   char *icon_path;

   if (!it->mime)
     _item_fill((Evry_Item *)it);

   if (!it->mime) return NULL;

   if (it->browseable)
     {
	o = e_icon_add(e);
	evry_icon_theme_set(o, "folder");
     }
   else
     {
	icon_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 64);

	if (icon_path)
	  {
	     o = e_util_icon_add(icon_path, e);
	     free(icon_path);
	  }
	  if (!o)
	  {
	     o = e_icon_add(e);
	     evry_icon_theme_set(o, "none");
	  }
     }
   return o;
}

static Eina_Bool
_init(void)
{
   p1 = E_NEW(Evry_Plugin, 1);
   p1->name = "Files";
   p1->type = type_subject;
   p1->type_in  = "NONE|FILE";
   p1->type_out = "FILE";
   p1->browseable = EINA_TRUE;
   p1->begin = &_begin;
   p1->browse = &_browse;
   p1->fetch = &_fetch;
   p1->cleanup = &_cleanup;
   p1->icon_get = &_item_icon_get;
   evry_plugin_register(p1);

   p2 = E_NEW(Evry_Plugin, 1);
   p2->name = "Files";
   p2->type = type_object;
   p2->type_in  = "NONE|FILE";
   p2->type_out = "FILE";
   p2->browseable = EINA_TRUE;
   p2->begin = &_begin;
   p2->browse = &_browse;
   p2->fetch = &_fetch;
   p2->cleanup = &_cleanup;
   p2->icon_get = &_item_icon_get;
   evry_plugin_register(p2);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p1);
   evry_plugin_unregister(p2);
   E_FREE(p1);
   E_FREE(p2);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

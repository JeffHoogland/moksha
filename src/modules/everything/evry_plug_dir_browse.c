#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  const char *directory;
};

static Evry_Plugin *_plug_new();
static void _plug_free(Evry_Plugin *p);
static int  _begin(Evry_Plugin *p, Evry_Item *item);
static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _cleanup(Evry_Plugin *p);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static void _list_free(Evry_Plugin *p);
static Evry_Item *_item_fill(const char *directory, const char *file);

static Evry_Plugin_Class class;

EAPI int
evry_plug_dir_browse_init(void)
{
   class.name = "Browse Files";
   class.type_in  = "NONE|FILE";
   class.type_out = "FILE";
   class.prio = 2;
   class.new = &_plug_new;
   class.free = &_plug_free;
   evry_plugin_register(&class);

   return 1;
}

EAPI int
evry_plug_dir_browse_shutdown(void)
{
   evry_plugin_unregister(&class);
   
   return 1;
}

static Evry_Plugin *
_plug_new()
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->class = &class;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   p->items = NULL;   

   Inst *inst = E_NEW(Inst, 1);
   inst->directory = eina_stringshare_add(e_user_homedir_get());
   p->priv = inst;

   return p;
}

static void
_plug_free(Evry_Plugin *p)
{
   _cleanup(p);

   Inst *inst = p->priv;
   eina_stringshare_del(inst->directory);
   E_FREE(inst);
   E_FREE(p);
}


static int
_action(Evry_Plugin *p, Evry_Item *item, const char *input)
{   
   return 0;
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
	if (it->o_icon) evas_object_del(it->o_icon);

	free(it);
     }
}


static void
_cleanup(Evry_Plugin *p)
{
   _list_free(p);
}

static int
_begin(Evry_Plugin *p, Evry_Item *item)
{
   Inst *inst = p->priv;
   
   if (item->uri && ecore_file_is_dir(item->uri))
     {
	eina_stringshare_del(inst->directory);
	inst->directory = eina_stringshare_add(item->uri);
	
	return 1;
     }

   return 0;
}

/* based on directory-watcher from drawer module  */
static int
_fetch(Evry_Plugin *p, const char *input)
{
   Eina_List *files;
   char *file;
   Evry_Item *it;
   char match1[4096];
   char match2[4096];
   Inst *inst = p->priv;
   
   _list_free(p);
   
   files = ecore_file_ls(inst->directory);

   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }

   EINA_LIST_FREE(files, file)
     {
	if ((file[0] == '.') ||
	    (input &&
	     (!e_util_glob_case_match(file, match1)) &&
	     (!e_util_glob_case_match(file, match2))))
	  goto end;
	it  = _item_fill(inst->directory, file);
	if (it)
	  p->items = eina_list_append(p->items, it);

     end:
	free(file);

     }
    
   return 1;
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

   it->uri = eina_stringshare_add(buf);
   
   if ((e_util_glob_case_match(buf, "*.desktop")) ||
       (e_util_glob_case_match(buf, "*.directory")))
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

   mime = e_fm_mime_filename_get(file_path);
   if (mime)
     {
	it->mime = eina_stringshare_add(mime);
	it->priority = 2;
     }
   else if (ecore_file_is_dir(file_path))
     {
	it->mime = eina_stringshare_add("Folder");
	it->priority = 1;
     }
   else
     {
	it->mime = eina_stringshare_add("None");
	it->priority = 2;
     }
   
   return it;
}

static void
_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   char *item_path;

   if (!it->mime) return;
   
   if (!strcmp(it->mime, "Folder"))
     {
	it->o_icon = edje_object_add(e);
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/fileman/folder");
     }
   else
     {
	item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 32);

	if (item_path)
	  it->o_icon = e_util_icon_add(item_path, e);
     }
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   
   it1 = data1;
   it2 = data2;

   return (it1->priority - it2->priority);
}

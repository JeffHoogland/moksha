/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

#define IMPORT_STRETCH 0
#define IMPORT_TILE 1
#define IMPORT_CENTER 2
#define IMPORT_SCALE_ASPECT_IN 3
#define IMPORT_SCALE_ASPECT_OUT 4

typedef struct _Plugin Plugin;
typedef struct _Import Import;
typedef struct _Item Item;

struct _Plugin
{
  Evry_Plugin base;
  Plugin *prev;
  Eina_List *items;
  const Evry_Item_File *file;
};

struct _Import
{
  const char *file;
  int method;
  int external;
  int quality;

  Ecore_Exe *exe;
  Ecore_Event_Handler *exe_handler;
  char *tmpf;
  char *fdest;
};

struct _Item
{
  Evry_Item base;
  const char *icon;
  int method;
};


static void _import_edj_gen(Import *import);
static int _import_cb_edje_cc_exit(void *data, int type, void *event);
static Import *import = NULL;

static Evry_Plugin *_plug;

static void
_item_free(Evry_Item *item)
{
   Item *it = (Item*) item;
   E_FREE(it);
}

static void
_item_add(Plugin *p, const char *name, int method, const char *icon)
{
   Item *item = E_NEW(Item, 1);
   evry_item_new(EVRY_ITEM(item), EVRY_PLUGIN(p), name, _item_free);

   item->icon = icon;
   item->method = method;

   p->items = eina_list_append(p->items, EVRY_ITEM(item));
}

static Evas_Object *
_item_icon_get(Evry_Plugin *plugin, const Evry_Item *item, Evas *e)
{
   return evry_icon_theme_get(((Item*)item)->icon, e);
}

static Evas_Object *
_icon_get(Evry_Plugin *plugin, const Evry_Item *it, Evas *e)
{
   return evry_icon_theme_get("preferences-desktop-wallpaper", e);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   PLUGIN(p, plugin);

   if (!item) return NULL;
   
   /* is FILE ? */
   if (item->plugin->type_out == plugin->type_in)
     {
	Evry_Item *it;
	ITEM_FILE(file, item);

	if (!file->mime || (strncmp(file->mime, "image/", 6)))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->file = file;

	it = evry_item_new(NULL, EVRY_PLUGIN(p), _("Set as Wallpaper"), NULL);
	it->browseable = EINA_TRUE;

	p->items = eina_list_append(p->items, it);

	return EVRY_PLUGIN(p);
     }
   else if (item->plugin->type_out == plugin->type_out)
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->base.icon_get = &_item_icon_get;
	p->prev = (Plugin*) item->plugin;

	_item_add(p, _("Stretch"), IMPORT_STRETCH,
		  "enlightenment/wallpaper_stretch");
	_item_add(p, _("Center"), IMPORT_CENTER,
		  "enlightenment/wallpaper_center");
	_item_add(p, _("Tile"), IMPORT_TILE,
		  "enlightenment/wallpaper_tile");
	_item_add(p, _("Within"), IMPORT_SCALE_ASPECT_IN,
		  "enlightenment/wallpaper_scale_aspect_in");
	_item_add(p, _("Fill"), IMPORT_SCALE_ASPECT_OUT,
		  "enlightenment/wallpaper_stretch");

	return EVRY_PLUGIN(p);
     }

   return NULL;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);
   Evry_Item *it;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Evry_Item *it = NULL;
   Eina_List *l;
   int match = 0;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FOREACH(p->items, l, it)
     if (!input || (match = evry_fuzzy_match(it->label, input)))
       {
	  it->fuzzy_match = match;
	  EVRY_PLUGIN_ITEM_APPEND(p, it);
       }

   if (input)
     plugin->items = evry_fuzzy_match_sort(plugin->items);

   return 1;
}

static int
_action(Evry_Plugin *plugin, const Evry_Item *item)
{
   PLUGIN(p, plugin);

   if (p->prev && p->prev->file)
     {
	if (import)
	  {
	     if (import->exe_handler)
	       ecore_event_handler_del(import->exe_handler);
	     E_FREE(import);
	  }

	Item *it = (Item*) item;
	import = E_NEW(Import, 1);
	import->method = it->method;
	import->file = p->prev->file->path;
	import->quality = 100;
	import->external = 0;
	_import_edj_gen(import);

	return 1;
     }

   return 0;
}

static Eina_Bool
_plugins_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   _plug = EVRY_PLUGIN_NEW(NULL, N_("Wallpaper"), type_action, "FILE", "",
			    _begin, _cleanup, _fetch, _icon_get, NULL);
   
   EVRY_PLUGIN(_plug)->icon = "preferences-desktop-wallpaper";
   EVRY_PLUGIN(_plug)->action = &_action;
   
   evry_plugin_register(_plug, 10);
   
   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(_plug);

   if (import)
     {
	if (import->exe_handler)
	  ecore_event_handler_del(import->exe_handler);
	E_FREE(import);
     }
}

/* taken from e_int_config_wallpaper_import.c */
static void
_import_edj_gen(Import *import)
{
   Ecore_Evas *ee = ecore_evas_buffer_new(100, 100);
   Evas *evas = ecore_evas_get(ee);
   Evas_Object *img;
   int fd, num = 1;
   int w = 0, h = 0;
   const char *file, *locale;
   char buf[4096], cmd[4096], tmpn[4096], ipart[4096], enc[128];
   char *imgdir = NULL, *fstrip;
   int cr = 255, cg = 255, cb = 255, ca = 255;
   FILE *f;
   size_t len, off;

   file = ecore_file_file_get(import->file);
   fstrip = ecore_file_strip_ext(file);
   if (!fstrip) return;
   len = e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s.edj", fstrip);
   if (len >= sizeof(buf)) return;
   off = len - (sizeof(".edj") - 1);
   while (ecore_file_exists(buf))
     {
	snprintf(buf + off, sizeof(buf) - off, "-%d.edj", num);
	num++;
     }
   free(fstrip);
   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0)
     {
	printf("Error Creating tmp file: %s\n", strerror(errno));
	return;
     }
   close(fd);

   f = fopen(tmpn, "w");
   if (!f)
     {
	printf("Cannot open %s for writing\n", tmpn);
	return;
     }

   imgdir = ecore_file_dir_get(import->file);
   if (!imgdir) ipart[0] = '\0';
   else
     {
	snprintf(ipart, sizeof(ipart), "-id %s", e_util_filename_escape(imgdir));
	free(imgdir);
     }

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, import->file, NULL);
   evas_object_image_size_get(img, &w, &h);
   evas_object_del(img);
   ecore_evas_free(ee);

   printf("w%d h%d\n", w, h);
   
   if (import->external)
     {
	fstrip = strdup(e_util_filename_escape(import->file));
	snprintf(enc, sizeof(enc), "USER");
     }
   else
     {
	fstrip = strdup(e_util_filename_escape(file));
	if (import->quality == 100)
	  snprintf(enc, sizeof(enc), "COMP");
	else
	  snprintf(enc, sizeof(enc), "LOSSY %i", import->quality);
     }
   switch (import->method)
     {
      case IMPORT_STRETCH:
	 fprintf(f,
		 "images { image: \"%s\" %s; }\n"
		 "collections {\n"
		 "group { name: \"e/desktop/background\";\n"
		 "data { item: \"style\" \"0\"; }\n"
		 "max: %i %i;\n"
		 "parts {\n"
		 "part { name: \"bg\"; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "image { normal: \"%s\"; scale_hint: STATIC; }\n"
		 "} } } } }\n"
		 , fstrip, enc, w, h, fstrip);
	 break;
      case IMPORT_TILE:
	 fprintf(f,
		 "images { image: \"%s\" %s; }\n"
		 "collections {\n"
		 "group { name: \"e/desktop/background\";\n"
		 "data { item: \"style\" \"1\"; }\n"
		 "max: %i %i;\n"
		 "parts {\n"
		 "part { name: \"bg\"; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "image { normal: \"%s\"; }\n"
		 "fill { size {\n"
		 "relative: 0.0 0.0;\n"
		 "offset: %i %i;\n"
		 "} } } } } } }\n"
		 , fstrip, enc, w, h, fstrip, w, h);
	 break;
      case IMPORT_CENTER:
	 fprintf(f,
		 "images { image: \"%s\" %s; }\n"
		 "collections {\n"
		 "group { name: \"e/desktop/background\";\n"
		 "data { item: \"style\" \"2\"; }\n"
		 "max: %i %i;\n"
		 "parts {\n"
		 "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "color: %i %i %i %i;\n"
		 "} }\n"
		 "part { name: \"bg\"; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "min: %i %i; max: %i %i;\n"
		 "image { normal: \"%s\"; }\n"
		 "} } } } }\n"
		 , fstrip, enc, w, h, cr, cg, cb, ca, w, h, w, h, fstrip);
	 break;
      case IMPORT_SCALE_ASPECT_IN:
	 locale = e_intl_language_get();
	 setlocale(LC_NUMERIC, "C");
	 fprintf(f,
		 "images { image: \"%s\" %s; }\n"
		 "collections {\n"
		 "group { name: \"e/desktop/background\";\n"
		 "data { item: \"style\" \"3\"; }\n"
		 "max: %i %i;\n"
		 "parts {\n"
		 "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "color: %i %i %i %i;\n"
		 "} }\n"
		 "part { name: \"bg\"; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "aspect: %1.9f %1.9f; aspect_preference: BOTH;\n"
		 "image { normal: \"%s\";  scale_hint: STATIC; }\n"
		 "} } } } }\n"
		 , fstrip, enc, w, h, cr, cg, cb, ca, (double)w / (double)h, (double)w / (double)h, fstrip);
	 setlocale(LC_NUMERIC, locale);
	 break;
      case IMPORT_SCALE_ASPECT_OUT:
	 locale = e_intl_language_get();
	 setlocale(LC_NUMERIC, "C");
	 fprintf(f,
		 "images { image: \"%s\" %s; }\n"
		 "collections {\n"
		 "group { name: \"e/desktop/background\";\n"
		 "data { item: \"style\" \"4\"; }\n"
		 "max: %i %i;\n"
		 "parts {\n"
		 "part { name: \"bg\"; mouse_events: 0;\n"
		 "description { state: \"default\" 0.0;\n"
		 "aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
		 "image { normal: \"%s\";  scale_hint: STATIC; }\n"
		 "} } } } }\n"
		 , fstrip, enc, w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
	 setlocale(LC_NUMERIC, locale);
	 break;
      default:
	 /* won't happen */
	 break;
     }
   free(fstrip);

   fclose(f);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s",
	    ipart, tmpn, e_util_filename_escape(buf));

   import->tmpf = strdup(tmpn);
   import->fdest = strdup(buf);
   import->exe_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
			     _import_cb_edje_cc_exit, import);
   import->exe = ecore_exe_run(cmd, NULL);
}

static int
_import_cb_edje_cc_exit(void *data, int type, void *event)
{
   Import *import;
   Ecore_Exe_Event_Del *ev;
   char *fdest;
   int r = 1;

   ev = event;
   import = data;

   if (!ev->exe) return 1;
     
   if (ev->exe != import->exe) return 1;

   if (ev->exit_code != 0)
     {
	e_util_dialog_show(_("Picture Import Error"),
			   _("Enlightenment was unable to import the picture<br>"
			     "due to conversion errors."));
	r = 0;
     }


   fdest = strdup(import->fdest);
   if (r)
     {
	e_bg_default_set(fdest);
	e_bg_update();
     }

   E_FREE(fdest);

   return 0;
}

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "everything-wallpaper"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   module = m;

   if (e_datastore_get("everything_loaded"))
     active = _plugins_init();
   
   e_module_delayed_set(m, 1); 

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   module = NULL;
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/**/
/***************************************************************************/


#include "Evry.h"

#define IMPORT_STRETCH 0
#define IMPORT_TILE 1
#define IMPORT_CENTER 2
#define IMPORT_SCALE_ASPECT_IN 3
#define IMPORT_SCALE_ASPECT_OUT 4

typedef struct _Plugin Plugin;
typedef struct _Import Import;
typedef struct _Item_Data Item_Data;

struct _Plugin
{
  Evry_Plugin base;
  Plugin *prev;
  Eina_List *items;
  const Evry_Item *item;
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
  Evas *evas;
};

struct _Item_Data
{
  const char *icon;
  int method;
};


static void _import_edj_gen(Import *import);
static int _import_cb_edje_cc_exit(void *data, int type, void *event);


static Evry_Plugin *plugin;

static void
_item_free(Evry_Item *it)
{
   Item_Data *id = it->data[0];
   E_FREE(id);
}

static void
_item_add(Plugin *p, const char *name, int method, const char *icon)
{
   Evry_Item *it;
   Item_Data *id;

   it = evry_item_new(&p->base, name, _item_free);
   id = E_NEW(Item_Data, 1);
   id->icon = icon;
   id->method = method;
   it->data[0] = id;
   p->items = eina_list_append(p->items, it);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it;

   /* is FILE ? */
   if (item && item->plugin->type_out == p->base.type_in)
     {
	if (!item->mime || (strncmp(item->mime, "image/", 6)))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->item = item;

	it = evry_item_new(&p->base, _("Set as Wallpaper"), NULL);

	it->browseable = EINA_TRUE;
	p->items = eina_list_append(p->items, it);

	return &p->base;
     }
   else if (item->plugin->type_out == plugin->type_out)
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
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

	return &p->base;
     }

   return NULL;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it;

   if (p->base.items) eina_list_free(p->base.items);
   p->base.items = NULL;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it = NULL;
   Eina_List *l;

   if (p->base.items) eina_list_free(p->base.items);
   p->base.items = NULL;

   EINA_LIST_FOREACH(p->items, l, it)
     plugin->items = eina_list_append(plugin->items, it);

   return 1;
}

static int
_action(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = (Plugin*) plugin;

   if (p->prev)
     {
	const Evry_Item *it_file;
	Item_Data *id;
	Import *import;

	import = E_NEW(Import, 1);
	it_file = p->prev->item;
	id = it->data[0];
	import->method = id->method;
	import->file = it_file->uri;
	import->quality = 100;
	import->external = 0;
	import->evas = evas_object_evas_get(it->o_bg); /* Eeek! */
	_import_edj_gen(import);

	return 1;
     }

   return 0;
}

static Evas_Object *
_icon_get(Evry_Plugin *plugin, const Evry_Item *it, Evas *e)
{
   /* Plugin *p = (Plugin*) plugin; */
   Evas_Object *o = NULL;

   if (it->data[0])
     {
	const char *icon = ((Item_Data*) it->data[0])->icon;

	o = evry_icon_theme_get(icon, e);
     }
   else
     {
	o = evry_icon_theme_get("preferences-desktop-wallpaper", e);
     }

   return o;
}

static Eina_Bool
_init(void)
{
   plugin = evry_plugin_new(NULL, "Wallpaper", type_action, "FILE", "",
			    0, "preferences-desktop-wallpaper", NULL,
			    _begin, _cleanup, _fetch, _action, _icon_get,
			    NULL, NULL);

   evry_plugin_register(plugin, 10);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_free(plugin, 1);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);



/* taken from e_int_config_wallpaper_import.c */
static void
_import_edj_gen(Import *import)
{
   Evas *evas = import->evas;

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
   E_FREE(import);

   return 0;
}

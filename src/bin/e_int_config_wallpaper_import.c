#include "e.h"

#define IMPORT_STRETCH 0
#define IMPORT_TILE 1
#define IMPORT_CENTER 2
#define IMPORT_SCALE_ASPECT_IN 3
#define IMPORT_SCALE_ASPECT_OUT 4

typedef struct _Import Import;

struct _Import 
{
   E_Config_Dialog *parent;
   E_Config_Dialog_Data *cfdata;
   
   Evas_Object *bg_obj;
   Evas_Object *box_obj;
   Evas_Object *event_obj;
   Evas_Object *content_obj;
   Evas_Object *fsel_obj;
   
   Evas_Object *ok_obj;
   Evas_Object *close_obj;

   Evas_Object *fill_stretch_obj;
   Evas_Object *fill_center_obj;
   Evas_Object *fill_tile_obj;
   Evas_Object *fill_within_obj;
   Evas_Object *fill_fill_obj;
   Evas_Object *quality_obj;
   Evas_Object *frame_fill_obj;
   Evas_Object *frame_quality_obj;
   
   E_Win *win;
   
   Ecore_Exe *exe;
   Ecore_Event_Handler *exe_handler;
   char *tmpf;
   char *fdest;
};

struct _E_Config_Dialog_Data 
{
   char *file;   
   int method;
   int quality;
};

static Ecore_Event_Handler *_import_edje_cc_exit_handler = NULL;

static void _import_opt_disabled_set(Import *import, int disabled);
static void _import_cb_sel_change(void *data, Evas_Object *obj);
static void _import_edj_gen(Import *import);
static int _import_cb_edje_cc_exit(void *data, int type, void *event);
static void _import_cb_delete(E_Win *win);
static void _import_cb_resize(E_Win *win);
static void _import_cb_close(void *data, void *data2);
static void _import_cb_ok(void *data, void *data2);
static void _import_cb_wid_on_focus(void *data, Evas_Object *obj);
static void _import_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);

/* FIXME: save previous dev/dir and restore it to browse that dir */
/* FIXME: rememebr size and maybe location? */

EAPI E_Win *
e_int_config_wallpaper_import(E_Config_Dialog *parent)
{
   Evas *evas;
   E_Win *win;
   Import *import;
   Evas_Object *o, *of, *ofm, *ord, *ot;
   E_Radio_Group *rg;
   Evas_Coord w, h;
   E_Config_Dialog_Data *cfdata;
   Evas_Modifier_Mask mask;
   
   import = E_NEW(Import, 1);
   if (!import) return NULL;
   
   win = e_win_new(parent->con);
   if (!win) 
     { 
	free(import);
	return NULL;	
     }
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->method = IMPORT_STRETCH;
   cfdata->quality = 90;
   import->cfdata = cfdata;
   import->win = win;
   
   evas = e_win_evas_get(win);
   
   import->parent = parent;

   e_win_title_set(win, _("Select a Picture..."));
   e_win_delete_callback_set(win, _import_cb_delete);
   e_win_resize_callback_set(win, _import_cb_resize);
   e_win_dialog_set(win, 1);
   e_win_name_class_set(win, "E", "_dialog");

   o = edje_object_add(evas);
   import->bg_obj = o;
   e_theme_edje_object_set(o, "base/theme/dialog", "widgets/dialog/main");
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   o = e_widget_list_add(evas, 1, 1);
   e_widget_on_focus_hook_set(o, _import_cb_wid_on_focus, import);
   import->box_obj = o;
   edje_object_part_swallow(import->bg_obj, "buttons_swallow", o);

  o = evas_object_rectangle_add(evas);
   import->event_obj = o;
   mask = 0;
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = evas_key_modifier_mask_get(evas, "Shift");
   evas_object_key_grab(o, "Tab", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "Return", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "KP_Enter", mask, ~mask, 0);
   mask = 0;
   evas_object_key_grab(o, "Space", mask, ~mask, 0);
   mask = 0;
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _import_cb_key_down, import);
   
   o = e_widget_list_add(evas, 0, 0);   
   import->content_obj = o;

   ofm = e_widget_fsel_add(evas, "~/", "/", NULL, NULL,
			   _import_cb_sel_change, import,
			   _import_cb_sel_change, import, 0
			   );
   import->fsel_obj = ofm;
   e_widget_list_object_append(o, ofm, 1, 1, 0.5);

   ot = e_widget_table_add(evas, 0);
   
   of = e_widget_frametable_add(evas, _("Fill and Stretch Options"), 1);
   import->frame_fill_obj = of;
   rg = e_widget_radio_group_new(&cfdata->method);
   ord = e_widget_radio_icon_add(evas, _("Stretch"), "enlightenment/wallpaper_stretch", 24, 24, IMPORT_STRETCH, rg);
   import->fill_stretch_obj = ord;
   e_widget_frametable_object_append(of, ord, 0, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Center"), "enlightenment/wallpaper_center", 24, 24, IMPORT_CENTER, rg);
   import->fill_center_obj = ord;
   e_widget_frametable_object_append(of, ord, 1, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Tile"), "enlightenment/wallpaper_tile", 24, 24, IMPORT_TILE, rg);
   import->fill_tile_obj = ord;
   e_widget_frametable_object_append(of, ord, 2, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Within"), "enlightenment/wallpaper_scale_aspect_in", 24, 24, IMPORT_SCALE_ASPECT_IN, rg);
   import->fill_within_obj = ord;
   e_widget_frametable_object_append(of, ord, 3, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_icon_add(evas, _("Fill"), "enlightenment/wallpaper_scale_aspect_out", 24, 24, IMPORT_SCALE_ASPECT_OUT, rg);
   import->fill_fill_obj = ord;
   e_widget_frametable_object_append(of, ord, 4, 0, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 0);
   
   of = e_widget_frametable_add(evas, _("File Quality"), 0);
   import->frame_quality_obj = of;
   ord = e_widget_slider_add(evas, 1, 0, _("%3.0f%%"), 0.0, 100.0, 1.0, 0, NULL, &(cfdata->quality), 150);
   import->quality_obj = ord;
   e_widget_frametable_object_append(of, ord, 0, 0, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 0);
   
   e_widget_list_object_append(o, ot, 0, 0, 0.5);
   
   e_widget_min_size_get(o, &w, &h);
   edje_extern_object_min_size_set(o, w, h);
   edje_object_part_swallow(import->bg_obj, "content_swallow", o);
   evas_object_show(o);
   
   import->ok_obj = e_widget_button_add(evas, _("OK"), NULL, _import_cb_ok, win, cfdata);
   e_widget_list_object_append(import->box_obj, import->ok_obj, 1, 0, 0.5);

   import->close_obj = e_widget_button_add(evas, _("Cancel"), NULL, _import_cb_close, win, NULL);
   e_widget_list_object_append(import->box_obj, import->close_obj, 1, 0, 0.5);
   
   e_win_centered_set(win, 1);
   
   o = import->box_obj;
   e_widget_min_size_get(o, &w, &h);
   edje_extern_object_min_size_set(o, w, h);
   edje_object_part_swallow(import->bg_obj, "buttons_swallow", o);
   
   edje_object_size_min_calc(import->bg_obj, &w, &h);
   evas_object_resize(import->bg_obj, w, h + 64);
   e_win_resize(win, w, h + 64);
   e_win_size_min_set(win, w, h);
   e_win_size_max_set(win, 99999, 99999);
   e_win_show(win);
   
   if (!e_widget_focus_get(import->bg_obj))
     e_widget_focus_set(import->box_obj, 1);
   
   win->data = import;
   
   _import_opt_disabled_set(import, 1);
   return win;
}

EAPI void
e_int_config_wallpaper_del(E_Win *win)
{
   Import *import;
   
   import = win->data;
   if (import->exe_handler) ecore_event_handler_del(import->exe_handler);
   import->exe_handler = NULL;
   if (import->tmpf) unlink(import->tmpf);
   E_FREE(import->tmpf);
   E_FREE(import->fdest);
   import->exe = NULL;
   e_object_del(E_OBJECT(import->win));
   e_int_config_wallpaper_import_done(import->parent);
   if (import) free(import);
}

static void
_import_opt_disabled_set(Import *import, int disabled)
{
   e_widget_disabled_set(import->fill_stretch_obj, disabled);
   e_widget_disabled_set(import->fill_center_obj, disabled);
   e_widget_disabled_set(import->fill_tile_obj, disabled);
   e_widget_disabled_set(import->fill_within_obj, disabled);
   e_widget_disabled_set(import->fill_fill_obj, disabled);
   e_widget_disabled_set(import->quality_obj, disabled);
   e_widget_disabled_set(import->frame_fill_obj, disabled);
   e_widget_disabled_set(import->frame_quality_obj, disabled);
}

static void 
_import_cb_sel_change(void *data, Evas_Object *obj)
{
   Import *import;
   const char *path, *p = NULL;
   
   import = data;
   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path) import->cfdata->file = strdup(path);
   if (path) p = strrchr(path, '.');
   if ((!p) || (!strcasecmp(p, ".edj")))
     _import_opt_disabled_set(import, 1);
   else
     _import_opt_disabled_set(import, 0);
}

static void 
_import_edj_gen(Import *import)
{
   Evas *evas;
   Evas_Object *img;
   int fd, num = 1;
   int w = 0, h = 0;
   const char *file;
   char buf[4096], cmd[4096], tmpn[4096], ipart[4096], enc[128];
   char *imgdir = NULL, *homedir, *fstrip;
   int cr = 255, cg = 255, cb = 255, ca = 255;
   FILE *f;
   
   evas = e_win_evas_get(import->win);
   file = ecore_file_get_file(import->cfdata->file);
   homedir = e_user_homedir_get();
   if (!homedir) return;
   fstrip = ecore_file_strip_ext(file);
   if (!fstrip)
     {
	free(homedir);
	return;
     }
   snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds/%s.edj", homedir, fstrip);
   while (ecore_file_exists(buf))
     {
	snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds/%s-%i.edj", homedir, fstrip, num);
	num++;
     }
   free(fstrip);
   free(homedir);
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
	printf("Cannot open %s for writting\n", tmpn);
	return;
     }
   
   imgdir = ecore_file_get_dir(import->cfdata->file);
   if (!imgdir) ipart[0] = '\0';
   else
     {
	snprintf(ipart, sizeof(ipart), "-id %s", imgdir);
	free(imgdir);
     }

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, import->cfdata->file, NULL);
   evas_object_image_size_get(img, &w, &h);
   evas_object_del(img);   
   
   fstrip = strdup(e_util_filename_escape(file));
   if (import->cfdata->quality == 100)
     snprintf(enc, sizeof(enc), "COMP");
   else
     snprintf(enc, sizeof(enc), "LOSSY %i", import->cfdata->quality);
   switch (import->cfdata->method) 
     {
      case IMPORT_STRETCH:
	fprintf(f, 
		"images { image: \"%s\" %s; }\n"
		"collections {\n"
		"group { name: \"desktop/background\";\n"
		"max: %i %i;\n"
		"parts {\n"
		"part { name: \"bg\"; mouse_events: 0;\n"
		"description { state: \"default\" 0.0;\n"
		"image { normal: \"%s\"; }\n"
		"} } } } }\n"
		, fstrip, enc, w, h, fstrip);
	break;
      case IMPORT_TILE:
	fprintf(f, 
		"images { image: \"%s\" %s; }\n"
		"collections {\n"
		"group { name: \"desktop/background\";\n"
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
		"group { name: \"desktop/background\";\n"
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
	fprintf(f, 
		"images { image: \"%s\" %s; }\n"
		"collections {\n"
		"group { name: \"desktop/background\";\n"
		"max: %i %i;\n"
		"parts {\n"
		"part { name: \"col\"; type: RECT; mouse_events: 0;\n"
		"description { state: \"default\" 0.0;\n"
		"color: %i %i %i %i;\n"
		"} }\n"
		"part { name: \"bg\"; mouse_events: 0;\n"
		"description { state: \"default\" 0.0;\n"
		"aspect: %1.9f %1.9f; aspect_preference: BOTH;\n"
		"image { normal: \"%s\"; }\n"
		"} } } } }\n"
		, fstrip, enc, w, h, cr, cg, cb, ca, (double)w / (double)h, (double)w / (double)h, fstrip);
	break;
      case IMPORT_SCALE_ASPECT_OUT:
	fprintf(f, 
		"images { image: \"%s\" %s; }\n"
		"collections {\n"
		"group { name: \"desktop/background\";\n"
		"max: %i %i;\n"
		"parts {\n"
		"part { name: \"bg\"; mouse_events: 0;\n"
		"description { state: \"default\" 0.0;\n"
		"aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
		"image { normal: \"%s\"; }\n"
		"} } } } }\n"
		, fstrip, enc, w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
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
   import->exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _import_cb_edje_cc_exit, import);
   import->exe = ecore_exe_run(cmd, NULL);   
}

static int
_import_cb_edje_cc_exit(void *data, int type, void *event)
{
   Import *import;
   Ecore_Exe_Event_Del *ev;
   
   ev = event;
   import = data;
   if (ev->exe != import->exe) return 1;

   if (ev->exit_code != 0)
     {
	e_util_dialog_show(_("Picture Import Error"),
			   _("Enlightenment was unable to import the picture<br>"
			     "due to conversion errors."));
     }
   
   e_int_config_wallpaper_update(import->parent, import->fdest);

   e_int_config_wallpaper_del(import->win);
   return 0;
}

static void 
_import_cb_delete(E_Win *win) 
{
   e_int_config_wallpaper_del(win);
}

static void 
_import_cb_resize(E_Win *win) 
{
   Import *import;
   
   import = win->data;
   if (!import) return;
   evas_object_resize(import->bg_obj, win->w, win->h);
}

static void 
_import_cb_close(void *data, void *data2) 
{
   E_Win *win;
   
   win = data;
   e_int_config_wallpaper_del(win);
}

static void 
_import_cb_ok(void *data, void *data2) 
{
   Import *import;
   E_Win *win;
   const char *path;
   
   win = data;
   import = win->data;
   if (!import) return;
   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path) import->cfdata->file = strdup(path);
   if (import->cfdata->file)
     {
	_import_edj_gen(import);
	e_win_hide(win);
	return;
     }
   e_int_config_wallpaper_del(win);
}

static void 
_import_cb_wid_on_focus(void *data, Evas_Object *obj) 
{
   Import *import;
   
   import = data;
   if (obj == import->content_obj)
     e_widget_focused_object_clear(import->box_obj);
   else if (import->content_obj)
     e_widget_focused_object_clear(import->content_obj);
}

static void 
_import_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event) 
{
   Evas_Event_Key_Down *ev;
   Import *import;

   ev = event;
   import = data;
   if (!strcmp(ev->keyname, "Tab"))
     {
	if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(import->win)), "Shift"))
	  {
	     if (e_widget_focus_get(import->box_obj))
	       {
		  if (!e_widget_focus_jump(import->box_obj, 0))
		    {
		       e_widget_focus_set(import->content_obj, 0);
		       if (!e_widget_focus_get(import->content_obj))
			 e_widget_focus_set(import->box_obj, 0);
		    }
	       }
	     else
	       {
		  if (!e_widget_focus_jump(import->content_obj, 0))
		    e_widget_focus_set(import->box_obj, 0);
	       }
	  }
	else
	  {
	     if (e_widget_focus_get(import->box_obj))
	       {
		  if (!e_widget_focus_jump(import->box_obj, 1))
		    {
		       e_widget_focus_set(import->content_obj, 1);
		       if (!e_widget_focus_get(import->content_obj))
			 e_widget_focus_set(import->box_obj, 1);
		    }
	       }
	     else
	       {
		  if (!e_widget_focus_jump(import->content_obj, 1))
		    e_widget_focus_set(import->box_obj, 1);
	       }
	  }
     }
   else if (((!strcmp(ev->keyname, "Return")) || 
	     (!strcmp(ev->keyname, "KP_Enter")) || 
	     (!strcmp(ev->keyname, "space"))))
     {
	Evas_Object *o = NULL;
	
	if ((import->content_obj) && (e_widget_focus_get(import->content_obj)))
	  o = e_widget_focused_object_get(import->content_obj);
	else
	  o = e_widget_focused_object_get(import->box_obj);
	if (o) e_widget_activate(o);
     }   
}

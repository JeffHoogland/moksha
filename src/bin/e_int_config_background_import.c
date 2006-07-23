#include "e.h"

#define E_BG_SCALE 0
#define E_BG_TILE 1
#define E_BG_CENTER 2

/* Personally I hate having to define this twice, but Tileing needs a fill */
#define IMG_EDC_TMPL_TILE \
"images {\n" \
"  image: \"%s\" LOSSY 90;\n" \
"}\n" \
"collections {\n" \
"  group {\n" \
"    name: \"desktop/background\";\n" \
"    max: %d %d;\n" \
"    parts {\n" \
"      part {\n" \
"        name: \"background_image\";\n" \
"        type: IMAGE;\n" \
"        mouse_events: 0;\n" \
"        description {\n" \
"          state: \"default\" 0.0;\n" \
"          visible: 1;\n" \
"          rel1 {\n" \
"            relative: 0.0 0.0;\n" \
"            offset: 0 0;\n" \
"          }\n" \
"          rel2 {\n" \
"            relative: 1.0 1.0;\n" \
"            offset: -1 -1;\n" \
"          }\n" \
"          image {\n" \
"            normal: \"%s\";\n" \
"          }\n" \
"          fill {\n" \
"            size {\n" \
"              relative: 0.0 0.0;\n" \
"              offset: %d %d;\n" \
"            }\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"    }\n" \
"  }\n" \
"}\n"

#define IMG_EDC_TMPL \
"images {\n" \
"  image: \"%s\" LOSSY 90;\n" \
"}\n" \
"collections {\n" \
"  group {\n" \
"    name: \"desktop/background\";\n" \
"    max: %d %d;\n" \
"    parts {\n" \
"      part {\n" \
"        name: \"background_image\";\n" \
"        type: IMAGE;\n" \
"        mouse_events: 0;\n" \
"        description {\n" \
"          state: \"default\" 0.0;\n" \
"          visible: 1;\n" \
"          rel1 {\n" \
"            relative: 0.0 0.0;\n" \
"            offset: 0 0;\n" \
"          }\n" \
"          rel2 {\n" \
"            relative: 1.0 1.0;\n" \
"            offset: -1 -1;\n" \
"          }\n" \
"          image {\n" \
"            normal: \"%s\";\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"    }\n" \
"  }\n" \
"}\n"

static Ecore_Event_Handler *_edj_exe_exit_handler = NULL;

static void         _efm_cb_selchange(void *data, Evas_Object *obj);
static void         _bg_edj_gen                (Evas *evas, char *filename, int method);
static int          _edj_exe_exit_cb           (void *data, int type, void *event);

static void _import_cb_delete(E_Win *win);
static void _import_cb_resize(E_Win *win);
static void _import_cb_close(void *data, void *data2);
static void _import_cb_ok(void *data, void *data2);
static void _import_cb_wid_on_focus(void *data, Evas_Object *obj);
static void _import_cb_key_down(void *data, Evas *e, Evas_Object *obj, void *event);

typedef struct _Bg_Import_Window Bg_Import_Window;
struct _Bg_Import_Window 
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
   
   E_Win *win;
};

struct _E_Config_Dialog_Data 
{
   char *file;   
   int method;
};

EAPI E_Win *
e_int_config_background_import(E_Config_Dialog *parent)
{
   Evas *evas;
   E_Win *win;
   Bg_Import_Window *import;
   Evas_Object *o, *of, *ofm, *ord;
   E_Radio_Group *rg;
   Evas_Coord w, h;
   E_Config_Dialog_Data *cfdata;
   Evas_Modifier_Mask mask;
   
   import = E_NEW(Bg_Import_Window, 1);
   if (!import) return NULL;
   
   win = e_win_new(parent->con);
   if (!win) 
     { 
	free(import);
	return NULL;	
     }
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->method = 0;
   import->cfdata = cfdata;
   import->win = win;
   
   evas = e_win_evas_get(win);
   
   import->parent = parent;

   e_win_title_set(win, _("Import An Image"));
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
			   _efm_cb_selchange, import,
			   _efm_cb_selchange, import
			   );
   import->fsel_obj = ofm;
   e_widget_list_object_append(o, ofm, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("Options"), 0);
   rg = e_widget_radio_group_new(&cfdata->method);
   ord = e_widget_radio_add(evas, _("Center Image"), E_BG_CENTER, rg);
   e_widget_frametable_object_append(of, ord, 0, 0, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_add(evas, _("Scale Image"), E_BG_SCALE, rg);
   e_widget_frametable_object_append(of, ord, 0, 1, 1, 1, 1, 0, 1, 0);
   ord = e_widget_radio_add(evas, _("Tile Image"), E_BG_TILE, rg);
   e_widget_frametable_object_append(of, ord, 0, 2, 1, 1, 1, 0, 1, 0);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   
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
   evas_object_resize(import->bg_obj, w + 64, h + 128);
   e_win_resize(win, w + 64, h + 128);
   e_win_size_min_set(win, w, h);
   e_win_size_max_set(win, 99999, 99999);
   e_win_show(win);
   
   if (!e_widget_focus_get(import->bg_obj))
     e_widget_focus_set(import->box_obj, 1);
   
   win->data = import;
   return win;
}

static void 
_efm_cb_selchange(void *data, Evas_Object *obj)
{
   Bg_Import_Window *import;
   const char *path;
   
   import = data;
   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path) import->cfdata->file = strdup(path);
}

static void 
_bg_edj_gen(Evas *evas, char *filename, int method) 
{
   Evas_Object *img;
   int fd = 0;
   int w, h;
   const char *file;
   char buff[4096], cmd[4096];
   char ipart[4096];
   char *imgdir = NULL;
   static char tmpn[4096];
   FILE *out = NULL;
   Ecore_Exe *x;
   
   if (!filename) return;
   file = ecore_file_get_file(filename);
   
   snprintf(buff, sizeof(buff), "%s/.e/e/backgrounds/%s.edj", 
	    e_user_homedir_get(), 
	    ecore_file_strip_ext(file));
   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0) 
     {
	printf("Error Creating tmp file: %s\n", strerror(errno));
	return;
     }
   close(fd);
   
   out = fopen(tmpn, "w");
   if (!out) 
     {
	printf("Cannot open %s for writting\n", tmpn);
	return;
     }
   
   imgdir = ecore_file_get_dir(filename);
   if (!imgdir) ipart[0] = '\0';
   if (imgdir) 
     {
	snprintf(ipart, sizeof(ipart), "-id %s", imgdir);
	free(imgdir);
     }

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, filename, NULL);
   evas_object_image_size_get(img, &w, &h);
   evas_object_del(img);   
   
   switch (method) 
     {
      case E_BG_CENTER:
	fprintf(out, IMG_EDC_TMPL, file, w, h, file);
	break;
      case E_BG_TILE:
	fprintf(out, IMG_EDC_TMPL_TILE, file, w, h, file, w, h);
	break;
      case E_BG_SCALE:
	fprintf(out, IMG_EDC_TMPL, file, w, h, file);
	break;
     }

   fclose(out);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s", 
	    ipart, 
	    tmpn, 
	    e_util_filename_escape(buff));

   _edj_exe_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edj_exe_exit_cb, NULL);
   x = ecore_exe_run(cmd, tmpn);   
}

static int
_edj_exe_exit_cb(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Ecore_Exe *x;
   
   ev = event;
   if (!ev->exe) return 1;
   
   x = ev->exe;
   if (!x) return 1;
   
   x = NULL;
   ecore_event_handler_del(_edj_exe_exit_handler);
   _edj_exe_exit_handler = NULL;
   
   unlink(data);   
   return 0;
}

static void 
_import_cb_delete(E_Win *win) 
{
   Bg_Import_Window *import;
   
   import = win->data;
   if (import) free(import);
   e_object_del(E_OBJECT(win));
}

static void 
_import_cb_resize(E_Win *win) 
{
   Bg_Import_Window *import;
   
   import = win->data;
   if (!import) return;
   evas_object_resize(import->bg_obj, win->w, win->h);
}

static void 
_import_cb_close(void *data, void *data2) 
{
   Bg_Import_Window *import;
   E_Win *win;
   
   win = data;   
   import = win->data;
   if (import) free(import);
   e_object_del(E_OBJECT(win));
}

static void 
_import_cb_ok(void *data, void *data2) 
{
   E_Fm_File *f;
   Evas *evas;
   E_Win *win;
   Bg_Import_Window *import;
   const char *path;
   
   win = data;   
   import = win->data;

   path = e_widget_fsel_selection_path_get(import->fsel_obj);
   E_FREE(import->cfdata->file);
   if (path) import->cfdata->file = strdup(path);
   if (import->cfdata->file)
     {
	evas = e_win_evas_get(win);
	_bg_edj_gen(evas, import->cfdata->file, import->cfdata->method);
     }
}

static void 
_import_cb_wid_on_focus(void *data, Evas_Object *obj) 
{
   Bg_Import_Window *import;
   
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
   Bg_Import_Window *dia;

   ev = event;
   dia = data;
   if (!strcmp(ev->keyname, "Tab"))
     {
	if (evas_key_modifier_is_set(evas_key_modifier_get(e_win_evas_get(dia->win)), "Shift"))
	  {
	     if (e_widget_focus_get(dia->box_obj))
	       {
		  if (!e_widget_focus_jump(dia->box_obj, 0))
		    {
		       e_widget_focus_set(dia->content_obj, 0);
		       if (!e_widget_focus_get(dia->content_obj))
			 e_widget_focus_set(dia->box_obj, 0);
		    }
	       }
	     else
	       {
		  if (!e_widget_focus_jump(dia->content_obj, 0))
		    e_widget_focus_set(dia->box_obj, 0);
	       }
	  }
	else
	  {
	     if (e_widget_focus_get(dia->box_obj))
	       {
		  if (!e_widget_focus_jump(dia->box_obj, 1))
		    {
		       e_widget_focus_set(dia->content_obj, 1);
		       if (!e_widget_focus_get(dia->content_obj))
			 e_widget_focus_set(dia->box_obj, 1);
		    }
	       }
	     else
	       {
		  if (!e_widget_focus_jump(dia->content_obj, 1))
		    e_widget_focus_set(dia->box_obj, 1);
	       }
	  }
     }
   else if (((!strcmp(ev->keyname, "Return")) || 
	     (!strcmp(ev->keyname, "KP_Enter")) || 
	     (!strcmp(ev->keyname, "space"))))
     {
	Evas_Object *o = NULL;
	
	if ((dia->content_obj) && (e_widget_focus_get(dia->content_obj)))
	  o = e_widget_focused_object_get(dia->content_obj);
	else
	  o = e_widget_focused_object_get(dia->box_obj);
	if (o) e_widget_activate(o);
     }   
}

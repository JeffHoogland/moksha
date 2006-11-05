/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

struct _E_Config_Dialog_Data
{
   E_App eap;

   char         *name; /* app name */
   char         *generic; /* generic app name */
   char         *comment; /* a longer description */
   char         *exe; /* command to execute, NULL if directory */
   char         *exe_params; /* command params to execute, NULL if directory */

   char         *win_name; /* window name */
   char         *win_class; /* window class */
   char         *win_title; /* window title */
   char         *win_role; /* window role */

   char         *icon_class; /* icon_class */
   char         *icon_path;  /* icon path */
   char         *image; /* used when we're saving a image into the eap */

   int    startup_notify;
   int    wait_exit;

   char  *exec;
   int    icon_theme;
   E_App_Edit *editor;
   Evas_Object *themed;
};

/* local subsystem functions */

static void           _e_eap_edit_free(E_App_Edit *editor);
static void          *_e_eap_edit_create_data(E_Config_Dialog *cfd);
static void           _e_eap_edit_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int            _e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int            _e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static Evas_Object   *_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data);
static Evas_Object   *_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data);
static void           _e_eap_editor_cb_icon_select(void *data1, void *data2);
static void           _e_eap_edit_select_cb(void *data, Evas_Object *obj);
static void           _e_eap_edit_cb_icon_select_ok(void *data, E_Dialog *dia);
static void           _e_eap_edit_cb_icon_select_cancel(void *data, E_Dialog *dia);
static void           _cb_files_icon_theme_changed(void *data, Evas_Object *obj, void *event_info);
static void           _e_eap_editor_icon_show(E_Config_Dialog_Data *cfdata);

#define IFADD(src, dst) if (src) dst = evas_stringshare_add(src); else dst = NULL
#define IFDEL(src) if (src) evas_stringshare_del(src);  src = NULL;
#define IFDUP(src, dst) if (src) dst = strdup(src); else dst = NULL
#define IFFREE(src) if (src) free(src);  src = NULL;

/* externally accessible functions */

EAPI E_App_Edit *
e_eap_edit_show(E_Container *con, E_App *a)
{
   E_Config_Dialog_View *v;
   E_App_Edit *editor;

   if (!con) return NULL;

   editor = E_OBJECT_ALLOC(E_App_Edit, E_EAP_EDIT_TYPE, _e_eap_edit_free);
   if (!editor) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v)
     {
	e_object_del(E_OBJECT(editor));
	return NULL;
     }

   editor->img = NULL;
   editor->eap = a;
   e_object_ref(E_OBJECT(editor->eap));

   /* methods */
   v->create_cfdata           = _e_eap_edit_create_data;
   v->free_cfdata             = _e_eap_edit_free_data;
   v->basic.apply_cfdata      = _e_eap_edit_basic_apply_data;
   v->basic.create_widgets    = _e_eap_edit_basic_create_widgets;
   v->advanced.apply_cfdata   = _e_eap_edit_advanced_apply_data;
   v->advanced.create_widgets = _e_eap_edit_advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   editor->cfd = e_config_dialog_new(con,
				     _("Application Editor"), 
				     "E", "_eap_editor_dialog",
				     "enlightenment/applications", 0, 
				     v, editor);
   return editor;
}

/* local subsystem functions */
static void
_e_eap_edit_free(E_App_Edit *editor)
{
   if (!editor) return;
   E_OBJECT_CHECK(editor);
   E_OBJECT_TYPE_CHECK(editor, E_EAP_EDIT_TYPE);
   if (editor->img) evas_object_del(editor->img);
   if (editor->fsel_dia) e_object_del(E_OBJECT(editor->fsel_dia));
   if (editor->cfd)
     {
	E_Object *obj;

	obj = E_OBJECT(editor->cfd);
	editor->cfd = NULL;
	e_object_del(obj);
     }
   if (editor->eap)
      {
         /* This frees up the temp file created by the border menu "Create Icon". */
         if (editor->eap->tmpfile) ecore_file_unlink(editor->eap->image);
         editor->eap->tmpfile = 0;
         IFDEL(editor->eap->image);
         editor->eap->width = 0;
         editor->eap->height = 0;
         e_object_unref(E_OBJECT(editor->eap));
         editor->eap = NULL;
      }
   e_object_del(E_OBJECT(editor));
}

static void *
_e_eap_edit_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->editor = cfd->data;
   /*- COMMON -*/
   IFADD(cfdata->editor->eap->path, cfdata->eap.path);
   IFDUP(cfdata->editor->eap->name, cfdata->name);
   IFDUP(cfdata->editor->eap->exe, cfdata->exe);
   IFDUP(cfdata->editor->eap->exe_params, cfdata->exe_params);
   cfdata->exec = ecore_desktop_merge_command((char *)cfdata->editor->eap->exe, (char *)cfdata->editor->eap->exe_params);
   IFDUP(cfdata->editor->eap->image, cfdata->image);
   cfdata->eap.height = cfdata->editor->eap->height;
   cfdata->eap.width = cfdata->editor->eap->width;
   IFADD(cfdata->editor->eap->icon_theme, cfdata->eap.icon_theme);
   IFADD(cfdata->editor->eap->icon, cfdata->eap.icon);
   IFDUP(cfdata->editor->eap->icon_class, cfdata->icon_class);
   IFDUP(cfdata->editor->eap->icon_path, cfdata->icon_path);
   cfdata->eap.icon_type = cfdata->editor->eap->icon_type;
   /*- ADVANCED -*/
   IFDUP(cfdata->editor->eap->generic, cfdata->generic);
   IFDUP(cfdata->editor->eap->comment, cfdata->comment);
   IFDUP(cfdata->editor->eap->win_name, cfdata->win_name);
   IFDUP(cfdata->editor->eap->win_class, cfdata->win_class);
   IFDUP(cfdata->editor->eap->win_title, cfdata->win_title);
   IFDUP(cfdata->editor->eap->win_role, cfdata->win_role);
   cfdata->startup_notify = cfdata->editor->eap->startup_notify;
   cfdata->wait_exit = cfdata->editor->eap->wait_exit;

   if (!cfdata->icon_path)
     {
        IFDUP(cfdata->image, cfdata->icon_path);
     }
   if (!cfdata->icon_path)
      cfdata->icon_theme = 1;
   /* Save it for later. */
   IFDUP(cfdata->icon_path, cfdata->image);
   return cfdata;
}

static void
_e_eap_edit_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   IFDEL(cfdata->eap.path);
   IFFREE(cfdata->name);
   IFFREE(cfdata->exe);
   IFFREE(cfdata->exe_params);
   IFFREE(cfdata->exec);
   IFFREE(cfdata->image);
   IFDEL(cfdata->eap.icon_theme);
   IFDEL(cfdata->eap.icon);
   IFFREE(cfdata->icon_class);
   IFFREE(cfdata->icon_path);
   IFFREE(cfdata->generic);
   IFFREE(cfdata->comment);
   IFFREE(cfdata->win_name);
   IFFREE(cfdata->win_class);
   IFFREE(cfdata->win_title);
   IFFREE(cfdata->win_role);

   if (cfdata->editor)
     {
	E_Object *obj;
	
	obj = E_OBJECT(cfdata->editor);
	cfdata->editor = NULL;
	e_object_del(obj);
     }

   free(cfdata);
}

static int
_e_eap_edit_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_App_Edit *editor;
   E_App *eap;

   editor = cfdata->editor;
   eap = editor->eap;

   /* FIXME: should NULL out any blank things, and sanity check the entire lot. */

   e_app_fields_empty(eap);

   IFADD(cfdata->eap.path, eap->path);
   IFADD(cfdata->name, eap->name);
   if (cfdata->exec)
      {
         char *exe;

         exe = strchr(cfdata->exec, ' ');
	 if (exe)
	    {
	       *exe = '\0';
               eap->exe_params = evas_stringshare_add(++exe);
	       *exe = ' ';
	    }
         eap->exe = evas_stringshare_add(cfdata->exec);
      }

   IFADD(cfdata->eap.icon, eap->icon);
   IFADD(cfdata->icon_class, eap->icon_class);
   IFADD(cfdata->eap.icon_theme, eap->icon_theme);
   if (cfdata->icon_theme)
     {
        IFDEL(eap->icon_path);
        IFDEL(eap->icon_theme);
     }
   else
     {
        IFADD(cfdata->icon_path, eap->icon_path);
	/* Check if it's still the same old temporary image. */
	if ((eap->icon_path) && (cfdata->editor->eap->image))
	  {
	    if (strcmp(eap->icon_path, cfdata->editor->eap->image) == 0)
	      {
                 IFDEL(eap->icon_path);
	      }
	  }
	/* Move the temporary image to a proper place. */
	if ((!eap->icon_path) && (cfdata->editor->eap->image))
	  {
	     char file[PATH_MAX];

/* FIXME: eap->image was created by the border menu "Create Icon" and it's the
 * path to a temporary file.  This was fine for .eaps as the file got saved 
 * into the .eap.  For .desktops, we need to copy this file into ~/.e/e/icons 
 * and find a decent name for it.  A decent name for it is whatever the icon 
 * search algo will find quickly.  On the other hand, this goes into icon_path, 
 * which is checked first.  The original temp name includes win_name and a time 
 * stamp.
 *
 * I'm going to just copy the existing filename for now, and leave the issue of 
 * a proper name until later.
 */
             snprintf(file, PATH_MAX, "%s/.e/e/icons/%s", e_user_homedir_get(), ecore_file_get_file(cfdata->editor->eap->image));
	     ecore_file_mv(cfdata->editor->eap->image, file);
             IFADD(file, eap->icon_path);
             IFDEL(eap->icon_theme);
	  }
     }
   if ((!cfdata->icon_theme) && (eap->icon_path))
      eap->icon_type = E_APP_ICON_PATH;
   else
      eap->icon_type = E_APP_ICON_UNKNOWN;

   /* FIXME: hardcoded until the eap editor provides fields to change it */
   if (cfdata->eap.width) eap->width = cfdata->eap.width;
   else eap->width = 128;
   if (cfdata->eap.height) eap->height = cfdata->eap.height;
   else eap->height = 128;

   IFADD(cfdata->generic, eap->generic);
   IFADD(cfdata->comment, eap->comment);
   IFADD(cfdata->win_name, eap->win_name);
   IFADD(cfdata->win_class, eap->win_class);
   IFADD(cfdata->win_title, eap->win_title);
   IFADD(cfdata->win_role, eap->win_role);
   eap->startup_notify = cfdata->startup_notify;
   eap->wait_exit = cfdata->wait_exit;

   if ((eap->name) && (eap->exe))
      e_app_fields_save(eap);

   return 1;
}

static int
_e_eap_edit_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return _e_eap_edit_basic_apply_data(cfd, cfdata);
}


static Evas_Object *
_e_eap_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;

   editor = cfdata->editor;
   editor->evas = evas;
   eap = editor->eap;

   if (cfdata->themed)
     cfdata->themed = NULL;
   
   ol = e_widget_table_add(evas, 0);

   o = e_widget_frametable_add(evas, _("Icon"), 0);

   _e_eap_editor_icon_show(cfdata);

// when flipping from advanced to basic - this will already be destroyed.
//   if (editor->img_widget) evas_object_del(editor->img_widget);
   editor->img_widget = e_widget_button_add(evas, "", NULL,
					    _e_eap_editor_cb_icon_select, cfdata, editor);
   if (editor->img)
      e_widget_button_icon_set(editor->img_widget, editor->img);
   e_widget_min_size_set(editor->img_widget, 48, 48);
   e_widget_frametable_object_append(o, editor->img_widget,
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   e_widget_table_object_append(ol, o, 0, 0, 1, 1, 1 ,1, 1, 1);

   o = e_widget_frametable_add(evas, _("Basic Info"), 0);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Name")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(cfdata->name));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Executable")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->exec)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 1, 0, 1, 1, 1 ,1, 1, 1);

   return ol;
}

static Evas_Object *
_e_eap_edit_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_App_Edit *editor;
   E_App *eap;
   Evas_Object *ol, *o;
   Evas_Object *entry;

   editor = cfdata->editor;
   eap = editor->eap;

   ol = _e_eap_edit_basic_create_widgets(cfd, evas, cfdata);

   o = e_widget_frametable_add(evas, _("General"), 0);

   /*- general info -*/
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Generic Info")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(cfdata->generic));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Comment")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->comment)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 1 ,1, 1, 1);


   /*- window info -*/
   o = e_widget_frametable_add(evas, _("Window"), 0);

   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Name")),
				     0, 0, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(cfdata->win_name));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Class")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->win_class)),
				     1, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Title")),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->win_title)),
				     1, 2, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Window Role")),
				     0, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_entry_add(evas, &(cfdata->win_role)),
				     1, 3, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 0, 2, 1, 1, 1 ,1, 1, 1);

   /*- icon info -*/
   o = e_widget_frametable_add(evas, _("Icon Theme"), 0);

   cfdata->themed = e_widget_check_add(evas, _("Use Icon Theme"), &(cfdata->icon_theme));
   evas_object_smart_callback_add(cfdata->themed, "changed",
				  _cb_files_icon_theme_changed, cfdata);
   e_widget_frametable_object_append(o, cfdata->themed,
				     1, 0, 1, 1,
				     1, 1, 1, 1);

   e_widget_frametable_object_append(o, e_widget_label_add(evas, _("Icon Class")),
				     0, 1, 1, 1,
				     1, 1, 1, 1);

   entry = e_widget_entry_add(evas, &(cfdata->icon_class));
   e_widget_min_size_set(entry, 100, 1);
   e_widget_frametable_object_append(o, entry,
				     1, 1, 1, 1,
				     1, 1, 1, 1);

   e_widget_table_object_append(ol, o, 1, 1, 1, 1, 1 ,1, 1, 1);


   /*- misc info -*/
   o = e_widget_frametable_add(evas, _("Misc"), 0);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Startup Notify"), &(cfdata->startup_notify)),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   e_widget_frametable_object_append(o, e_widget_check_add(evas, _("Wait Exit"), &(cfdata->wait_exit)),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   e_widget_table_object_append(ol, o, 1, 2, 1, 1, 1 ,1, 1, 1);

   return ol;
}

static void
_e_eap_editor_cb_icon_select(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   E_App_Edit *editor;
   char *dir = NULL;

   editor = data2;
   cfdata = data1;

   if (editor->fsel_dia) return;

   dia = e_dialog_new(cfdata->editor->cfd->con, "E", "_eap_icon_select_dialog");
   if (!dia) return;
   e_dialog_title_set(dia, _("Select an Icon"));
   dia->data = cfdata;

   if (cfdata->icon_path)
     dir = ecore_file_get_dir(cfdata->icon_path);
   
   if (dir)
     {
        o = e_widget_fsel_add(dia->win->evas, dir, "/", NULL, NULL,
		              _e_eap_edit_select_cb, cfdata,
		              NULL, cfdata, 1);
	free(dir);
     }
   else
     {
        o = e_widget_fsel_add(dia->win->evas, "~/", "/", NULL, NULL,
			      _e_eap_edit_select_cb, cfdata,
			      NULL, cfdata, 1);
     }
   evas_object_show(o);
   editor->fsel = o;
   e_widget_min_size_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);

   /* buttons at the bottom */
   e_dialog_button_add(dia, _("OK"), NULL, _e_eap_edit_cb_icon_select_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, _e_eap_edit_cb_icon_select_cancel, cfdata);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   e_win_resize(dia->win, 475, 341);
   editor->fsel_dia = dia;
}

static void
_e_eap_edit_select_cb(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
}

static void
_e_eap_edit_cb_icon_select_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   const char *file;

   cfdata = data;
   file = e_widget_fsel_selection_path_get(cfdata->editor->fsel);
   if (file)
     {
        IFFREE(cfdata->image);
        IFDUP(file, cfdata->image);
        if (cfdata->themed)
           e_widget_check_checked_set(cfdata->themed, 0);
	else
	  {
	     cfdata->icon_theme = 0;
             IFFREE(cfdata->icon_path);
             IFDUP(file, cfdata->icon_path);
	  }
        _cb_files_icon_theme_changed(cfdata, NULL, NULL);
     }

   _e_eap_edit_cb_icon_select_cancel(data, dia);
}

static void
_e_eap_edit_cb_icon_select_cancel(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_object_del(E_OBJECT(dia));
   cfdata->editor->fsel_dia = NULL;
}

static void
_cb_files_icon_theme_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   _e_eap_editor_icon_show(cfdata);
   if (cfdata->editor->img)
      e_widget_button_icon_set(cfdata->editor->img_widget, cfdata->editor->img);
}

static void
_e_eap_editor_icon_show(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->editor->img)
     {
	evas_object_del(cfdata->editor->img);
	cfdata->editor->img = NULL;
     }

   IFFREE(cfdata->icon_path);
   if (!cfdata->icon_theme)
     {
        IFDUP(cfdata->image, cfdata->icon_path);
     }

   IFDEL(cfdata->eap.icon_class);
   IFDEL(cfdata->eap.icon_path);
   IFADD(cfdata->icon_class, cfdata->eap.icon_class);
   IFADD(cfdata->icon_path, cfdata->eap.icon_path);
   if ((!cfdata->icon_theme) && (cfdata->eap.icon_path))
      cfdata->eap.icon_type = E_APP_ICON_PATH;
   else
      cfdata->eap.icon_type = E_APP_ICON_UNKNOWN;
   cfdata->editor->img = e_app_icon_add(cfdata->editor->evas, &(cfdata->eap));
}

#include "e.h"

struct _E_Config_Dialog_Data
{
   Efreet_Desktop *desktop;
   int type; /* desktop type */

   char *name; /* app name (e.g. Firefox) */
   char *generic_name; /* generic app name (e.g. Web Browser) */
   char *comment; /* a longer description */
   char *exec; /* command to execute */
   char *try_exec; /* executable to test for an apps existence */
   char *url; /* url to open */

   char *startup_wm_class; /* window class */
   char *categories; /* list of category names that app is in */
   char *mimes; /* list of mimes this app can handle */
   char *icon;  /* absolute path to file or icon name */

   int startup_notify;
   int terminal;
   int show_in_menus;

   E_Desktop_Edit *editor;

   char *orig_path; /* informational only */
   Evas_Object *orig_path_entry; /* to set when info changes */
   Evas_Object *icon_entry; /* to set when icon changes */

   /* speed up check_changed tests */
   Eina_Bool changed_categories;
   Eina_Bool changed_mimes;
   Eina_Bool edited_categories;
   Eina_Bool edited_mimes;
};

/* local subsystem functions */

static int          _e_desktop_edit_view_create(E_Desktop_Edit *editor, E_Container *con);
static void         _e_desktop_edit_free(E_Desktop_Edit *editor);
static void        *_e_desktop_edit_create_data(E_Config_Dialog *cfd);
static void         _e_desktop_edit_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int          _e_desktop_edit_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static int          _e_desktop_edit_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *data);
static Evas_Object *_e_desktop_edit_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *data);
static void         _e_desktop_editor_cb_icon_select(void *data1, void *data2);
static void         _e_desktop_edit_cb_icon_select_destroy(void *obj);
static void         _e_desktop_edit_cb_icon_select_ok(void *data, E_Dialog *dia);
static void         _e_desktop_edit_cb_icon_select_cancel(void *data, E_Dialog *dia);
static void         _e_desktop_editor_icon_update(E_Config_Dialog_Data *cfdata);
static void         _e_desktop_editor_cb_exec_select(void *data1, void *data2);
static void         _e_desktop_edit_cb_exec_select_destroy(void *obj);
static void         _e_desktop_edit_cb_exec_select_ok(void *data, E_Dialog *dia);
static void         _e_desktop_edit_cb_exec_select_cancel(void *data, E_Dialog *dia);
static void         _e_desktop_editor_exec_update(E_Config_Dialog_Data *cfdata);
static void         _e_desktop_edit_select_cb(void *data, Evas_Object *obj);
static void         _e_desktop_editor_icon_entry_changed(void *data, Evas_Object *obj);

#define IFADD(src, dst) if (src) dst = eina_stringshare_add(src); else dst = NULL
#define IFDEL(src) if (src) eina_stringshare_del(src);  src = NULL;
#define IFDUP(src, dst) if (src) dst = strdup(src); else dst = NULL
#define IFFREE(src) if (src) free(src);  src = NULL;

/* externally accessible functions */

EAPI Efreet_Desktop *
e_desktop_border_create(E_Border *bd)
{
   Efreet_Desktop *desktop = NULL;
   const char *desktop_dir, *icon_dir;
   const char *bname, *bclass, *btitle;
   char path[PATH_MAX];

   bname = bd->client.icccm.name;
   if ((bname) && (bname[0] == 0)) bname = NULL;
   bclass = bd->client.icccm.class;
   if ((bclass) && (bclass[0] == 0)) bclass = NULL;
   btitle = e_border_name_get(bd);
   
   desktop_dir = e_user_desktop_dir_get();

   if ((!desktop_dir) || (!e_util_dir_check(desktop_dir))) return NULL;

   icon_dir = e_user_icon_dir_get();
   if ((!icon_dir) || (!e_util_dir_check(icon_dir))) return NULL;

   if (bname) 
     {
	snprintf(path, sizeof(path), "%s/%s.desktop", desktop_dir, bname);
	desktop = efreet_desktop_empty_new(path);
     }
   else
     {
        int i;
	
	for (i = 1; i < 65536; i++)
	  {
	     snprintf(path, sizeof(path), "%s/_new_app-%i.desktop",
		      desktop_dir, i);
	     if (!ecore_file_exists(path))
	       {
		  desktop = efreet_desktop_empty_new(path);
		  break;
	       }
	  }
	if (!desktop)
	  {
	     snprintf(path, sizeof(path), "%s/_rename_me-%i.desktop",
		      desktop_dir, (int)ecore_time_get());
	     desktop = efreet_desktop_empty_new(NULL);
	  }
     }

   if (!desktop)
     {
	//XXX out of memory?
	return NULL;
     }
   if (bclass) desktop->name = strdup(bclass);
   else if (bname) desktop->name = strdup(bname);
   else if (btitle) desktop->name = strdup(btitle);
   
   if (btitle) desktop->comment = strdup(btitle);
   
   if (bclass) desktop->startup_wm_class = strdup(bclass);
   if (bd->client.icccm.command.argc > 0)
     // FIXME this should concat the entire argv array together
     desktop->exec = strdup(bd->client.icccm.command.argv[0]);
   else if (bname)
     desktop->exec = strdup(bname); 

// disable this   
//   if (bd->client.netwm.startup_id > 0) desktop->startup_notify = 1;
   if (bd->client.netwm.icons)
     {
	/* FIXME
	 * - Find the icon with the best size
	 * - Should use mkstemp
	 */
	snprintf(path, sizeof(path), "%s/%s-%.6f.png", icon_dir, bname, ecore_time_get());
	if (e_util_icon_save(&(bd->client.netwm.icons[0]), path))
	  desktop->icon = strdup(path);
	else
	  fprintf(stderr, "Could not save file from ARGB: %s\n", path);
     }
   return desktop;
}

EAPI E_Desktop_Edit *
e_desktop_border_edit(E_Container *con, E_Border *bd)
{
   E_Desktop_Edit *editor;
   int new_desktop = 0;

   if (!con) return NULL;
   editor = E_OBJECT_ALLOC(E_Desktop_Edit, E_DESKTOP_EDIT_TYPE, _e_desktop_edit_free);
   if (!editor) return NULL;
   if (bd->desktop)
     editor->desktop = bd->desktop;

   /* the border does not yet have a desktop entry. add one and pre-populate
      it with values from the border */
   if (!editor->desktop)
     {
	editor->desktop = e_desktop_border_create(bd);
	if ((editor->desktop) && (editor->desktop->icon))
	  editor->tmp_image_path = strdup(editor->desktop->icon);
	new_desktop = 1;
     }

#if 0
   if ((!bname) && (!bclass))
     {
	e_util_dialog_show(_("Incomplete Window Properties"),
			   _("The window you are creating an icon for<br>"
			     "does not contain window name and class<br>"
			     "properties, so the needed properties for<br>"
			     "the icon so that it will be used for this<br>"
			     "window cannot be guessed. You will need to<br>"
			     "use the window title instead. This will only<br>"
			     "work if the window title is the same at<br>"
			     "the time the window starts up, and does not<br>"
			     "change."));
     }
#endif 
   if (!_e_desktop_edit_view_create(editor, con))
     {
	e_object_del(E_OBJECT(editor));
	editor = NULL;
     }
   else 
	e_config_dialog_changed_set(editor->cfd, new_desktop);

   return editor;
}

EAPI E_Desktop_Edit *
e_desktop_edit(E_Container *con, Efreet_Desktop *desktop)
{
   E_Desktop_Edit *editor;

   if (!con) return NULL;
   editor = E_OBJECT_ALLOC(E_Desktop_Edit, E_DESKTOP_EDIT_TYPE, _e_desktop_edit_free);
   if (!editor) return NULL;
   if (desktop) editor->desktop = desktop;
   if (!_e_desktop_edit_view_create(editor, con))
     {
	e_object_del(E_OBJECT(editor));
	editor = NULL;
     }
   return editor;
}

static int
_e_desktop_edit_view_create(E_Desktop_Edit *editor, E_Container *con)
{
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return 0;

   /* view methods */
   v->create_cfdata           = _e_desktop_edit_create_data;
   v->free_cfdata             = _e_desktop_edit_free_data;
   v->basic.apply_cfdata      = _e_desktop_edit_basic_apply_data;
   v->basic.create_widgets    = _e_desktop_edit_basic_create_widgets;
   v->basic.check_changed     = _e_desktop_edit_basic_check_changed;

   editor->cfd = 
     e_config_dialog_new(con, _("Desktop Entry Editor"), "E", 
			 "applications/new_application",
			 "preferences-applications", 0, v, editor);
   
   if (!editor->cfd)
     {
	E_FREE(v);
	return 0;
     }
   
   return 1;
}

/* local subsystem functions */
static void
_e_desktop_edit_free(E_Desktop_Edit *editor)
{
   if (!editor) return;
   E_OBJECT_CHECK(editor);
   E_OBJECT_TYPE_CHECK(editor, E_EAP_EDIT_TYPE);

   IFFREE(editor->tmp_image_path);
   E_FREE(editor);
}

/**
 * Populates the config dialog's data from the .desktop file passed in 
 */
static void *
_e_desktop_edit_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Desktop *desktop = NULL; 
   char path[PATH_MAX];

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->editor = cfd->data;

   /* 
    * If we can't write to the file on it's current location,
    * we always save to the user's applications dir.
    * If the file is already there, then edit it directly. Otherwise, create
    * a new empty desktop entry there. 
    *
    * cfdata->editor->desktop is the the desktop passed in
    * cfdata->desktop is the desktop to save
    * desktop is the desktop to load
    */
   path[0] = '\0';
   if (cfdata->editor->desktop) 
     {
	char dir[PATH_MAX];
	const char *file;

	if (ecore_file_can_write (cfdata->editor->desktop->orig_path))
	  cfdata->desktop = efreet_desktop_uncached_new(cfdata->editor->desktop->orig_path);
	else
	  {
	     snprintf(dir, sizeof(dir), "%s/applications", efreet_data_home_get());
	     if (!strncmp(dir, cfdata->editor->desktop->orig_path, strlen(dir)))
	       cfdata->desktop = efreet_desktop_uncached_new(cfdata->editor->desktop->orig_path);
	     else
	       {
	          /* file not in user's dir, so create new desktop that points there */
	          if (!ecore_file_exists(dir)) ecore_file_mkdir(dir);
	          file = ecore_file_file_get(cfdata->editor->desktop->orig_path);
	          snprintf(path, sizeof(path), "%s/%s", dir, file);
	          /*
	           * if a file already exists in the user dir with this name, we
	           * fetch the pointer to it, so the caches stay consistent (this
	           * probably will never return non null, since the ui shouldn't
	           * provide a means to edit a file in a system dir when one 
	           * exists in the user's
	           */
	          cfdata->desktop = efreet_desktop_uncached_new(path);
	       }
	  }
	desktop = cfdata->editor->desktop;
     }

   if (!cfdata->desktop)
	cfdata->desktop = efreet_desktop_empty_new(path);

   if (!desktop) desktop = cfdata->desktop;

   IFDUP(desktop->name, cfdata->name);
   IFDUP(desktop->generic_name, cfdata->generic_name);
   IFDUP(desktop->comment, cfdata->comment);
   IFDUP(desktop->exec, cfdata->exec);
   IFDUP(desktop->try_exec, cfdata->try_exec);
   IFDUP(desktop->url, cfdata->url);
   IFDUP(desktop->startup_wm_class, cfdata->startup_wm_class);
   IFDUP(desktop->orig_path, cfdata->orig_path);

   if (desktop->categories)
     cfdata->categories = efreet_desktop_string_list_join(desktop->categories);
   if (desktop->mime_types)
     cfdata->mimes = efreet_desktop_string_list_join(desktop->mime_types);
   
   IFDUP(desktop->icon, cfdata->icon);

   cfdata->startup_notify = desktop->startup_notify;
   cfdata->terminal = desktop->terminal;
   cfdata->show_in_menus = !desktop->no_display;

   if (cfdata->exec && *cfdata->exec)
     cfdata->type = 0;
   else if (cfdata->url && *cfdata->url)
     cfdata->type = 1;

   return cfdata;
}

/**
 * Frees the config dialog data
 */
static void
_e_desktop_edit_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->editor->tmp_image_path) 
     {
	if ((!cfdata->desktop) || (!cfdata->editor->saved) || 
	    (!cfdata->desktop->icon) ||
	    (strcmp(cfdata->editor->tmp_image_path, cfdata->desktop->icon)))
	  {
	     ecore_file_unlink(cfdata->editor->tmp_image_path);
	  }
     }
   if (cfdata->desktop) efreet_desktop_free(cfdata->desktop);

   IFFREE(cfdata->name);
   IFFREE(cfdata->generic_name);
   IFFREE(cfdata->comment);
   IFFREE(cfdata->exec);
   IFFREE(cfdata->try_exec);
   IFFREE(cfdata->url);
   IFFREE(cfdata->startup_wm_class);
   IFFREE(cfdata->categories);
   IFFREE(cfdata->icon);
   IFFREE(cfdata->mimes);
   IFFREE(cfdata->orig_path);

   if (cfdata->editor->icon_fsel_dia) 
     e_object_del(E_OBJECT(cfdata->editor->icon_fsel_dia));

   e_object_del(E_OBJECT(cfdata->editor));
   free(cfdata);
}

static void
_e_desktop_edit_user_local_desktop_filename_generate(E_Config_Dialog_Data *cfdata, char *path)
{
   char basename[PATH_MAX];
   const char *name;
   unsigned int i;
   int prefix;

   if ((cfdata->name) && (cfdata->name[0]))
     name = cfdata->name;
   else if ((cfdata->desktop) && (cfdata->desktop->name) &&
	    (cfdata->desktop->name[0]))
     name = cfdata->name;
   else
     name = NULL;

   if (name)
     {
	const char *s = name;
	for (i = 0; i < sizeof(basename) && s[i]; i++)
	  {
	     if (isalnum(s[i]))
	       basename[i] = s[i];
	     else
	       basename[i] = '_';
	  }
	basename[i] = '\0';
     }
   else
     eina_strlcpy(basename, "unnamed_desktop", sizeof(basename));

   i = snprintf(path, PATH_MAX, "%s/applications/%s.desktop",
		efreet_data_home_get(), basename);
   if (i >= PATH_MAX)
     {
	path[0] = '\0';
	return;
     }
   prefix = i - (sizeof(".desktop") - 1);

   for (i = 0; ecore_file_exists(path); i++)
     snprintf(path + prefix, PATH_MAX - prefix, "-%u.desktop", i);
}

static void
_e_desktop_edit_update_orig_path(E_Config_Dialog_Data *cfdata)
{
   const char *orig_path;
   char path[PATH_MAX];

   if ((cfdata->desktop->orig_path) && (cfdata->desktop->orig_path[0]))
     orig_path = cfdata->desktop->orig_path;
   else
     {
	_e_desktop_edit_user_local_desktop_filename_generate(cfdata, path);
	orig_path = cfdata->orig_path = strdup(path);
     }

   if (cfdata->orig_path_entry)
     e_widget_entry_text_set(cfdata->orig_path_entry, orig_path);
}

/**
 * Apply the basic config dialog
 */
static int
_e_desktop_edit_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   char *str;

   IFFREE(cfdata->desktop->name);
   IFDUP(cfdata->name, cfdata->desktop->name);
   IFFREE(cfdata->desktop->comment);
   IFDUP(cfdata->comment, cfdata->desktop->comment);

   switch (cfdata->type)
     {
        case 1:
          IFFREE(cfdata->desktop->url);
          IFDUP(cfdata->url, cfdata->desktop->url);
          break;
        default:
          IFFREE(cfdata->desktop->exec);
          IFDUP(cfdata->exec, cfdata->desktop->exec);
          IFFREE(cfdata->desktop->try_exec);
          IFDUP(cfdata->try_exec, cfdata->desktop->try_exec);
          break;
     }

   IFFREE(cfdata->desktop->generic_name);
   IFDUP(cfdata->generic_name, cfdata->desktop->generic_name);
   IFFREE(cfdata->desktop->startup_wm_class);
   IFDUP(cfdata->startup_wm_class, cfdata->desktop->startup_wm_class);

   EINA_LIST_FREE(cfdata->desktop->categories, str)
     eina_stringshare_del(str);
   cfdata->desktop->categories = efreet_desktop_string_list_parse(cfdata->categories);

   EINA_LIST_FREE(cfdata->desktop->mime_types, str)
     eina_stringshare_del(str);
   cfdata->desktop->mime_types = efreet_desktop_string_list_parse(cfdata->mimes);

   IFFREE(cfdata->desktop->icon);
   IFDUP(cfdata->icon, cfdata->desktop->icon);

   cfdata->desktop->startup_notify = cfdata->startup_notify;
   cfdata->desktop->terminal = cfdata->terminal;
   cfdata->desktop->no_display = !cfdata->show_in_menus;

   if (cfdata->desktop->orig_path && cfdata->desktop->orig_path[0])
     cfdata->editor->saved = efreet_desktop_save(cfdata->desktop);
   else
     {
	_e_desktop_edit_update_orig_path(cfdata);
	cfdata->editor->saved = efreet_desktop_save_as
	  (cfdata->desktop, cfdata->orig_path);
     }
   return 1;
}

static int
_e_desktop_edit_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   int ret;

   if ((cfdata->startup_notify != cfdata->desktop->startup_notify) ||
       (cfdata->terminal != cfdata->desktop->terminal) ||
       (cfdata->show_in_menus != !cfdata->desktop->no_display))
     return 1;

#define CHECK(k)						\
   do								\
     {								\
	const char *__k;					\
	if (cfdata->desktop->k)					\
	  __k = cfdata->desktop->k;				\
	else if ((cfdata->editor) && (cfdata->editor->desktop))	\
	  __k = cfdata->editor->desktop->k;			\
	else							\
	  __k = NULL;						\
	if ((cfdata->k && !__k) || (!cfdata->k && __k) ||	\
	    (cfdata->k && __k && strcmp(cfdata->k, __k) != 0))	\
	  return 1;						\
     }								\
   while (0)

   CHECK(name);
   CHECK(exec);
   CHECK(try_exec);
   CHECK(url);
   CHECK(comment);
   CHECK(generic_name);
   CHECK(startup_wm_class);
   CHECK(icon);
#undef CHECK

   ret = 0;

   if (cfdata->edited_categories)
     {
	const char *str;
	Eina_List *lst, *old_lst;

	cfdata->edited_categories = EINA_FALSE;
	cfdata->changed_categories = EINA_FALSE;

	if (cfdata->desktop->categories)
	  old_lst = cfdata->desktop->categories;
	else if ((cfdata->editor) && (cfdata->editor->desktop))
	  old_lst = cfdata->editor->desktop->categories;
	else
	  old_lst = NULL;

	lst = efreet_desktop_string_list_parse(cfdata->categories);
	if (eina_list_count(lst) != eina_list_count(old_lst))
	  cfdata->changed_categories = EINA_TRUE;

	EINA_LIST_FREE(lst, str)
	  {
	     if (!cfdata->changed_categories)
	       {
		  Eina_List *n;
		  const char *old;
		  Eina_Bool found = EINA_FALSE;
		  EINA_LIST_FOREACH(old_lst, n, old)
		    {
		       if (strcmp(str, old) == 0)
			 {
			    found = EINA_TRUE;
			    break;
			 }
		    }
		  if (!found)
		    cfdata->changed_categories = EINA_TRUE;
	       }
	     eina_stringshare_del(str);
	  }
     }
   ret |= cfdata->changed_categories;

   if (!ret)
     {
	if (cfdata->edited_mimes)
	  {
	     const char *str;
	     Eina_List *lst, *old_lst;

	     cfdata->edited_mimes = EINA_FALSE;
	     cfdata->changed_mimes = EINA_FALSE;

	     if (cfdata->desktop->mime_types)
	       old_lst = cfdata->desktop->mime_types;
	     else if ((cfdata->editor) && (cfdata->editor->desktop))
	       old_lst = cfdata->editor->desktop->mime_types;
	     else
	       old_lst = NULL;

	     lst = efreet_desktop_string_list_parse(cfdata->mimes);
	     if (eina_list_count(lst) != eina_list_count(old_lst))
	       cfdata->changed_mimes = EINA_TRUE;

	     EINA_LIST_FREE(lst, str)
	       {
		  if (!cfdata->changed_mimes)
		    {
		       Eina_List *n;
		       const char *old;
		       Eina_Bool found = EINA_FALSE;
		       EINA_LIST_FOREACH(old_lst, n, old)
			 {
			    if (strcmp(str, old) == 0)
			      {
				 found = EINA_TRUE;
				 break;
			      }
			 }
		       if (!found)
			 cfdata->changed_mimes = EINA_TRUE;
		    }
		  eina_stringshare_del(str);
	       }

	  }
	ret |= cfdata->changed_mimes;
     }

   return ret;
}

static void
_e_desktop_editor_name_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   _e_desktop_edit_update_orig_path(cfdata);
}

static void
_e_desktop_editor_categories_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->edited_categories = EINA_TRUE;
}

static void
_e_desktop_editor_icon_entry_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   _e_desktop_editor_icon_update(cfdata);
}

static void
_e_desktop_editor_mimes_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->edited_mimes = EINA_TRUE;
}

static Evas_Object *
_e_desktop_edit_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   E_Desktop_Edit *editor = cfdata->editor;
   E_Radio_Group *rg;
   Evas_Object *otb, *ol, *o, *ot;
   Evas_Coord mw, mh;

   editor->evas = evas;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   ot = e_widget_table_add(evas, 0);

   o = e_widget_label_add(evas, _("Name"));
   e_widget_table_object_append(ot, o, 0, 0, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->name), NULL, NULL, NULL);
   e_widget_size_min_get(o, &mw, &mh);
   if (mw < 220) mw = 220;
   e_widget_size_min_set(o, mw, mh);
   if ((!cfdata->desktop->orig_path) || (!cfdata->desktop->orig_path[0]))
     e_widget_on_change_hook_set(o, _e_desktop_editor_name_changed, cfdata);
   e_widget_table_object_append(ot, o, 1, 0, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Comment"));
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->comment), NULL, NULL, NULL);
   e_widget_table_object_append(ot, o, 1, 1, 1, 1, 1, 1, 1, 0);

   rg = e_widget_radio_group_new(&(cfdata->type));

   // desktop type: application

   o = e_widget_radio_add(evas, _("Application"), 0, rg);
   e_widget_table_object_append(ot, o, 0, 2, 1, 1, 1, 1, 0, 0);

   editor->entry_widget_exec = e_widget_entry_add
     (evas, &(cfdata->exec), NULL, NULL, NULL);
   e_widget_table_object_append(ot, editor->entry_widget_exec, 1, 2, 1, 1, 1, 1, 1, 0);

   o = e_widget_button_add
     (evas, "...", NULL, _e_desktop_editor_cb_exec_select, cfdata, editor);
   e_widget_table_object_append(ot, o, 2, 2, 1, 1, 0, 0, 0, 0);

   // desktop type: url
   o = e_widget_radio_add(evas, _("URL"), 1, rg);
   e_widget_table_object_append(ot, o, 0, 3, 1, 1, 1, 1, 0, 0);

   editor->entry_widget_url = e_widget_entry_add
     (evas, &(cfdata->url), NULL, NULL, NULL);
   e_widget_table_object_append(ot, editor->entry_widget_url, 1, 3, 1, 1, 1, 1, 1, 0);
   // FIXME: add url selection dialog (file:/etc/..)

   e_widget_toolbook_page_append
     (otb, NULL, _("Basic"), ot, 1, 0, 1, 0, 0.5, 0.0);

   /* e_widget_size_min_get(ol, &mw, &mh); */

   ot = e_widget_table_add(evas, 0);
   
   editor->img_widget = e_widget_button_add
     (evas, "", NULL, _e_desktop_editor_cb_icon_select, cfdata, editor);
   _e_desktop_editor_icon_update(cfdata);
   e_widget_size_min_set(editor->img_widget, 192, 192);

   e_widget_table_object_append(ot, editor->img_widget, 0, 0, 2, 1, 1, 1, 0, 0);

   o = e_widget_label_add(evas, _("Icon"));
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->icon), NULL, NULL, NULL);
   cfdata->icon_entry = o;
   e_widget_on_change_hook_set(o, _e_desktop_editor_icon_entry_changed, cfdata);
   e_widget_table_object_append(ot, o, 1, 1, 1, 1, 1, 1, 1, 0);

   e_widget_toolbook_page_append
     (otb, NULL, _("Icon"), ot, 0, 0, 0, 0, 0.5, 0.5);
   
   ot = e_widget_table_add(evas, 0);

   o = e_widget_label_add(evas, _("Generic Name"));
   e_widget_table_object_append(ot, o, 0, 0, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->generic_name), NULL, NULL, NULL);
   e_widget_table_object_append(ot, o, 1, 0, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Window Class"));
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->startup_wm_class), NULL, NULL, NULL);
   e_widget_table_object_append(ot, o, 1, 1, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Categories"));
   e_widget_table_object_append(ot, o, 0, 2, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->categories), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_desktop_editor_categories_changed, cfdata);
   e_widget_table_object_append(ot, o, 1, 2, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Mime Types"));
   e_widget_table_object_append(ot, o, 0, 3, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, &(cfdata->mimes), NULL, NULL, NULL);
   e_widget_on_change_hook_set(o, _e_desktop_editor_mimes_changed, cfdata);
   e_widget_table_object_append(ot, o, 1, 3, 1, 1, 1, 1, 1, 0);

   o = e_widget_label_add(evas, _("Desktop file"));
   e_widget_table_object_append(ot, o, 0, 4, 1, 1, 1, 1, 0, 0);

   o = e_widget_entry_add(evas, NULL, NULL, NULL, NULL);
   e_widget_table_object_append(ot, o, 1, 4, 1, 1, 1, 1, 1, 0);
   e_widget_disabled_set(o, 1);
   cfdata->orig_path_entry = o;
   _e_desktop_edit_update_orig_path(cfdata);

   e_widget_toolbook_page_append
     (otb, NULL, _("General"), ot, 1, 0, 1, 0, 0.5, 0.0);


   ol = e_widget_list_add(evas, 0, 0);
   o = e_widget_check_add(evas, _("Startup Notify"), &(cfdata->startup_notify));
   e_widget_list_object_append(ol, o, 1, 0, 0.0);
   o = e_widget_check_add(evas, _("Run in Terminal"), &(cfdata->terminal));
   e_widget_list_object_append(ol, o, 1, 0, 0.0);
   o = e_widget_check_add(evas, _("Show in Menus"), &(cfdata->show_in_menus));
   e_widget_list_object_append(ol, o, 1, 0, 0.0);
   e_widget_toolbook_page_append
     (otb, NULL, _("Options"), ol, 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   e_util_win_auto_resize_fill(cfd->dia->win);
   e_win_centered_set(cfd->dia->win, 1);

   return otb;
}

static void
_e_desktop_editor_cb_icon_select(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   E_Desktop_Edit *editor;
   char *path = NULL;
   const char *icon_path = NULL;

   editor = data2;
   cfdata = data1;

   if (editor->icon_fsel_dia) return;

   dia = e_dialog_new(cfdata->editor->cfd->con, "E", "_eap_icon_select_dialog");
   if (!dia) return;
   e_object_del_attach_func_set(E_OBJECT(dia), 
				_e_desktop_edit_cb_icon_select_destroy);
   e_dialog_title_set(dia, _("Select an Icon"));
   dia->data = cfdata;

   /* absolute path to icon */
   /* XXX change this to a generic icon selector (that can do either
    * files from a dir, or icons in the current theme */
   if (cfdata->icon) 
     {
	if (ecore_file_exists(cfdata->icon))
	  icon_path = cfdata->icon;
	else
	  icon_path = efreet_icon_path_find(e_config->icon_theme, cfdata->icon, 64);

        if (icon_path)
          path = ecore_file_dir_get(icon_path);
     }

   if (path)
     {
	o = e_widget_fsel_add(dia->win->evas, "/", path, NULL, NULL,
			      _e_desktop_edit_select_cb, cfdata, 
			      NULL, cfdata, 1);
	free(path);
     }
   else
     {
	o = e_widget_fsel_add(dia->win->evas, "~/", "/", NULL, NULL,
			      _e_desktop_edit_select_cb, cfdata,
			      NULL, cfdata, 1);
     }
   
   evas_object_show(o);
   editor->icon_fsel = o;
   e_widget_size_min_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);

   /* buttons at the bottom */
   e_dialog_button_add(dia, _("OK"), NULL, 
		       _e_desktop_edit_cb_icon_select_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, 
		       _e_desktop_edit_cb_icon_select_cancel, cfdata);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   editor->icon_fsel_dia = dia;
}

static void
_e_desktop_editor_cb_exec_select(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dia;
   Evas_Object *o;
   Evas_Coord mw, mh;
   E_Desktop_Edit *editor;
   char *path = NULL;

   editor = data2;
   cfdata = data1;

   if (editor->exec_fsel_dia) return;

   dia = e_dialog_new(cfdata->editor->cfd->con, "E", "_eap_exec_select_dialog");
   if (!dia) return;
   e_object_del_attach_func_set(E_OBJECT(dia), 
				_e_desktop_edit_cb_exec_select_destroy);
   e_dialog_title_set(dia, _("Select an Executable"));
   dia->data = cfdata;

   /* absolute path to exe */
   if (cfdata->exec) 
     {
	path = ecore_file_dir_get(cfdata->exec);
	if (path && !ecore_file_exists(path))
	  {
	     free(path);
	     path = NULL;
	  }
     }

   if (path)
     {
	o = e_widget_fsel_add(dia->win->evas, "/", path, NULL, NULL,
			      _e_desktop_edit_select_cb, cfdata,
			      NULL, cfdata, 1);
	free(path);
	path = NULL;
     }
   else
     {
	o = e_widget_fsel_add(dia->win->evas, "~/", "/", NULL, NULL,
			      _e_desktop_edit_select_cb, cfdata,
			      NULL, cfdata, 1);
     }
   
   evas_object_show(o);
   editor->exec_fsel = o;
   e_widget_size_min_get(o, &mw, &mh);
   e_dialog_content_set(dia, o, mw, mh);

   /* buttons at the bottom */
   e_dialog_button_add(dia, _("OK"), NULL, 
		       _e_desktop_edit_cb_exec_select_ok, cfdata);
   e_dialog_button_add(dia, _("Cancel"), NULL, 
		       _e_desktop_edit_cb_exec_select_cancel, cfdata);
   e_dialog_resizable_set(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   editor->exec_fsel_dia = dia;
}

static void
_e_desktop_edit_select_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
}

static void
_e_desktop_edit_cb_icon_select_destroy(void *obj)
{
   E_Dialog *dia = obj;
   E_Config_Dialog_Data *cfdata = dia->data;

/* extra unref isn't needed - there is no extra ref() anywhere i saw */   
/*   e_object_unref(E_OBJECT(dia));*/
   _e_desktop_edit_cb_icon_select_cancel(cfdata, NULL);
}

static void
_e_desktop_edit_cb_icon_select_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   const char *file;

   cfdata = data;
   file = e_widget_fsel_selection_path_get(cfdata->editor->icon_fsel);

   IFFREE(cfdata->icon);
   IFDUP(file, cfdata->icon);
   e_widget_entry_text_set(cfdata->icon_entry, cfdata->icon);

   _e_desktop_edit_cb_icon_select_cancel(data, dia);
}

static void
_e_desktop_edit_cb_icon_select_cancel(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   cfdata->editor->icon_fsel_dia = NULL;
   _e_desktop_editor_icon_update(cfdata);
}

static void
_e_desktop_editor_icon_update(E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o;

   if (!cfdata->editor->img_widget) return;
   o = e_util_icon_theme_icon_add(cfdata->icon, 128, cfdata->editor->evas);

   /* NB this takes care of freeing any previous icon object */
   e_widget_button_icon_set(cfdata->editor->img_widget, o);
   e_widget_change(cfdata->editor->img_widget);
}

static void
_e_desktop_edit_cb_exec_select_destroy(void *obj)
{
   E_Dialog *dia = obj;
   E_Config_Dialog_Data *cfdata = dia->data;

/* guess it should work like _e_desktop_edit_cb_icon_select_destroy */
/*    e_object_unref(E_OBJECT(dia)); */
   _e_desktop_edit_cb_exec_select_cancel(cfdata, NULL);
}

static void
_e_desktop_edit_cb_exec_select_ok(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   const char *file;

   cfdata = data;
   file = e_widget_fsel_selection_path_get(cfdata->editor->exec_fsel);

   IFFREE(cfdata->exec);
   IFDUP(file, cfdata->exec);

   _e_desktop_edit_cb_exec_select_cancel(data, dia);
}

static void
_e_desktop_edit_cb_exec_select_cancel(void *data, E_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (dia) e_util_defer_object_del(E_OBJECT(dia));
   cfdata->editor->exec_fsel_dia = NULL;
   _e_desktop_editor_exec_update(cfdata);
}

static void
_e_desktop_editor_exec_update(E_Config_Dialog_Data *cfdata)
{
   if (!cfdata->editor->entry_widget_exec) return;
   e_widget_entry_text_set(cfdata->editor->entry_widget_exec, cfdata->exec);
}

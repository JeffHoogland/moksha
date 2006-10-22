/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: fwin - he fm2 filemanager wrapped with a window and scrollframe.
 * primitive BUT enough to test generic dnd and fm stuff more easily. don't
 * play with this unless u want to help with it. NOT COMPLETE! BEWARE!
 */

/* local subsystem functions */
static void _e_fwin_free(E_Fwin *fwin);
static void _e_fwin_cb_delete(E_Win *win);
static void _e_fwin_cb_resize(E_Win *win);
static void _e_fwin_deleted(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_menu_extend(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
static void _e_fwin_parent(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_file_open(E_Fwin *fwin, const char *file, const char *mime);
static void _e_fwin_file_open_app(E_Fwin *fwin, E_App *a, const char *file);

/* local subsystem globals */
static Evas_List *fwins = NULL;

/* externally accessible functions */
EAPI int
e_fwin_init(void)
{
   return 1;
}

EAPI int
e_fwin_shutdown(void)
{
   return 1;
}

EAPI E_Fwin *
e_fwin_new(E_Container *con, const char *dev, const char *path)
{
   E_Fwin *fwin;
   char buf[4096];
   const char *file;
   Evas_Object *o;
   E_Fm2_Config fmc;
   
   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return NULL;
   fwin->win = e_win_new(con);
   if (!fwin->win)
     {
	free(fwin);
	return NULL;
     }
   fwins = evas_list_append(fwins, fwin);
   e_win_delete_callback_set(fwin->win, _e_fwin_cb_delete);
   e_win_resize_callback_set(fwin->win, _e_fwin_cb_resize);
   fwin->win->data = fwin;

   /* fm issues: */
   /* FIXME: "select" of a file opens that file  based on mimetype etc. */
   /* FIXME: if file executable - run it */
   
   o = evas_object_rectangle_add(e_win_evas_get(fwin->win));
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_show(o);
   fwin->bg_obj = o;
   
   o = e_fm2_add(e_win_evas_get(fwin->win));
   fwin->fm_obj = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 0;
   fmc.view.selector = 0;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 1;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 0;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _e_fwin_changed, fwin);
   evas_object_smart_callback_add(o, "dir_deleted",
				  _e_fwin_deleted, fwin);
   evas_object_smart_callback_add(o, "selected",
				  _e_fwin_selected, fwin);
   e_fm2_path_set(o, dev, path);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend, fwin);
   evas_object_show(o);
   
   o = e_scrollframe_add(e_win_evas_get(fwin->win));
   /* FIXME: this theme object will have more versions and options later
    * for things like swallowing widgets/buttons ot providing them - a
    * gadcon for starters for fm widgets. need to register the owning 
    * e_object of the gadcon so gadcon clients can get it and thus do
    * things like find out what dirs/path the fwin is for etc. this will
    * probably be how you add optional gadgets to fwin views like empty/full
    * meters for disk usage, and other dir info/stats or controls. also it
    * might be possible that we can have custom frames per dir later so need
    * a way to set an edje file directly
    */
   e_scrollframe_custom_theme_set(o, "base/theme/fileman",
				  "e/fileman/scrollframe/default");
   e_scrollframe_extern_pan_set(o, fwin->fm_obj,
				e_fm2_pan_set,
				e_fm2_pan_get,
				e_fm2_pan_max_get,
				e_fm2_pan_child_size_get);
   evas_object_propagate_events_set(fwin->fm_obj, 0);
   fwin->scrollframe_obj = o;
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   e_fm2_window_object_set(fwin->fm_obj, E_OBJECT(fwin->win));
   
   evas_object_focus_set(fwin->fm_obj, 1);
   
   snprintf(buf, sizeof(buf), "_fwin::/%s", e_fm2_real_path_get(fwin->fm_obj));
   e_win_name_class_set(fwin->win, "E", buf);
   file = ecore_file_get_file(e_fm2_real_path_get(fwin->fm_obj));
   if (file)
     snprintf(buf, sizeof(buf), "%s", file);
   else
     snprintf(buf, sizeof(buf), "%s", e_fm2_real_path_get(fwin->fm_obj));
   e_win_title_set(fwin->win, buf);
   e_win_size_min_set(fwin->win, 24, 24);
   e_win_resize(fwin->win, 280, 200);
   e_win_show(fwin->win);
   
   return fwin;
}

/* local subsystem functions */
static void
_e_fwin_free(E_Fwin *fwin)
{
   e_object_del(E_OBJECT(fwin->win));
   fwins = evas_list_remove(fwins, fwin);
   free(fwin);
}

static void
_e_fwin_cb_delete(E_Win *win)
{
   E_Fwin *fwin;
   
   fwin = win->data;
   e_object_del(E_OBJECT(fwin));
}

static void
_e_fwin_cb_resize(E_Win *win)
{
   E_Fwin *fwin;
   
   fwin = win->data;
   evas_object_resize(fwin->bg_obj, fwin->win->w, fwin->win->h);
   evas_object_resize(fwin->scrollframe_obj, fwin->win->w, fwin->win->h);
}

static void
_e_fwin_deleted(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   
   fwin = data;
   e_object_del(E_OBJECT(fwin));
}

static void
_e_fwin_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   
   fwin = data;
   if (fwin->scrollframe_obj)
     e_scrollframe_child_pos_set(fwin->scrollframe_obj, 0, 0);
}

static void
_e_fwin_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   Evas_List *selected, *l;
   E_Fm2_Icon_Info *ici;
   char buf[4096];
   
   fwin = data;
   selected = e_fm2_selected_list_get(fwin->fm_obj);
   if (!selected) return;
   for (l = selected; l; l = l->next)
     {
	ici = l->data;
	if ((ici->link) && (ici->mount))
	  e_fwin_new(fwin->win->container, ici->link, "/");
	else if (ici->link)
	  {
	     if (S_ISDIR(ici->statinfo.st_mode))
	       e_fwin_new(fwin->win->container, NULL, ici->link);
	     else
	       _e_fwin_file_open(fwin, ici->link, e_fm_mime_filename_get(ici->link));
	  }
	else
	  {
	     snprintf(buf, sizeof(buf), "%s/%s", 
		      e_fm2_real_path_get(fwin->fm_obj), ici->file);
	     if (S_ISDIR(ici->statinfo.st_mode))
	       e_fwin_new(fwin->win->container, NULL, buf);
	     else
	       _e_fwin_file_open(fwin, buf, ici->mime);
	  }
     }
   evas_list_free(selected);
}

static void
_e_fwin_menu_extend(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info)
{
   E_Fwin *fwin;
   E_Menu_Item *mi;
   
   fwin = data;
   if (e_fm2_has_parent_get(obj))
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Go to Parent Directory"));
	e_menu_item_icon_edje_set(mi,
				  e_theme_edje_file_get("base/theme/fileman",
							"e/fileman/button/parent"),
				  "e/fileman/button/parent");
	e_menu_item_callback_set(mi, _e_fwin_parent, obj);
     }
   /* FIXME: if info != null then check mime type and offer options based
    * on that
    */
}

static void
_e_fwin_parent(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_fm2_parent_go(data);
}

static void
_e_fwin_file_open(E_Fwin *fwin, const char *file, const char *mime)
{
   Evas_List *apps, *l;
   E_App *a;
   char pcwd[4096];

   /* 1. find previously used launcher */
   /*   if found - is there still an app for it? */
   /*     a = e_app_file_find(prev_launcher); */
   /*     if (a) - run it */
   /*       return; */
   /* 2. list possible apps */
   /*   if list == 1 element - run it */
   /*     return; */
   /* 3. create dialog */
   /*   if (list) add apps in list */
   /*     dialog should have optin to show all apps */   
   getcwd(pcwd, sizeof(pcwd));
   chdir(e_fm2_real_path_get(fwin->fm_obj));
   /* 1. */
   /* FIXME: implement the above # 1. */
   /* 2. */
   apps = e_app_mime_list(e_fm_mime_filename_get(file));
   if (apps)
     {
	if (evas_list_count(apps) == 1)
	  {
	     a = apps->data;
	     _e_fwin_file_open_app(fwin, a, ecore_file_get_file(file));
	  }
	else
	  {
	     /* FIXME: hack. do dialog as above in # 3. */
	     a = apps->data;
	     _e_fwin_file_open_app(fwin, a, ecore_file_get_file(file));
	     /* FIXME: register app a as handling mime type if app doesnt */
	     /* say it can already in a separate info blob so in future */
	     /* e will list it as an option */
	     for (l = apps; l; l = l->next)
	       {
		  a = l->data;
	       }
	  }
	evas_list_free(apps);
     }
   chdir(pcwd);
}

static void
_e_fwin_file_open_app(E_Fwin *fwin, E_App *a, const char *file)
{
   char buf[4096], *p, *e, *s;
   
   if (!a->exe) return;
   if (a->exe_params)
     {
	/* cmd is "a->exe a->exe_params" where a->exe_params will */
	/* have replaced %[uU] or %[fF] or %[mM] with the filename */
	/* in question. also replace %[cC] and %[iI] with blanks */
	e = buf + sizeof(buf) - 1;
	if ((strlen(a->exe) + strlen(file)) >= (sizeof(buf) - 10)) return;
        strcpy(buf, a->exe);
	strcat(buf, " ");
	p = buf + strlen(buf);
	s = (char *)a->exe_params;
	while ((*s) && (p < e))
	  {
	     if (*s != '%')
	       {
		  *p = *s;
		  p++;
	       }
	     else
	       {
		  s++;
		  if (*s == '%')
		    {
		       *p = '%';
		       p++;
		    }
		  else
		    {
		       if ((*s == 'u') || (*s == 'U') ||
			   (*s == 'f') || (*s == 'F') ||
			   (*s == 'm') || (*s == 'M'))
			 {
			    if ((e - p) > (strlen(file) + 1))
			      {
				 strcpy(p, file);
				 p += strlen(file);
			      }
			 }
		    }
	       }
	     s++;
	  }
	*p = 0;
     }
   else
     {
	/* cmd is "a->exe filename" */
	snprintf(buf, sizeof(buf), "%s %s", a->exe, file);
     }
   e_zone_exec(fwin->win->border->zone, buf);
}

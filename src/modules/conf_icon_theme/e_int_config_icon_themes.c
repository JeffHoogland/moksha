/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define MOD_UNLOADED 0
#define MOD_ENABLED 1

typedef struct _CFIconTheme CFIconTheme;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
//static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _ilist_cb_change(void *data, Evas_Object *obj);
static int _sort_icon_themes(void *data1, void *data2);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Eina_List *icon_themes;
   int state;
   char *themename;
   struct {
      Evas_Object *comment;
      Evas_Object *list;
      Evas_Object *o_fm;
      Evas_Object *o_frame;
      Evas_Object *o_up_button;
   } gui;
};

EAPI E_Config_Dialog *
e_int_config_icon_themes(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_icon_theme_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.check_changed     = _basic_check_changed;
   /*
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.apply_cfdata   = _basic_apply_data;
   */
   
   cfd = e_config_dialog_new(con,
			     _("Icon Theme Settings"),
			     "E", "_config_icon_theme_dialog",
			     "enlightenment/icon_theme", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   Ecore_List *icon_themes;

   icon_themes = efreet_icon_theme_list_get();
   if (icon_themes)
     {
	Efreet_Icon_Theme *theme;

	ecore_list_first_goto(icon_themes);
	while ((theme = ecore_list_next(icon_themes)))
	  cfdata->icon_themes = eina_list_append(cfdata->icon_themes, theme);
	cfdata->icon_themes = eina_list_sort(cfdata->icon_themes,
					     eina_list_count(cfdata->icon_themes),
					     _sort_icon_themes);
	ecore_list_destroy(icon_themes);
     }
   cfdata->themename = strdup(e_config->icon_theme);

   return;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->icon_themes);
   E_FREE(cfdata->themename);
   E_FREE(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return strcmp(cfdata->themename, e_config->icon_theme);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Event_Config_Icon_Theme *ev;
   
   /* Actually take our cfdata settings and apply them in real life */
   e_config->icon_theme = eina_stringshare_add(cfdata->themename);
   e_config_save_queue();

   ev = E_NEW(E_Event_Config_Icon_Theme, 1);
   if (ev)
     {
	ev->icon_theme = e_config->icon_theme;
	ecore_event_add(E_EVENT_CONFIG_ICON_THEME, ev, NULL, NULL);
     }
   return 1; /* Apply was OK */
}

/*
static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->gui.o_fm)
     e_fm2_parent_go(cfdata->gui.o_fm);
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static void
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   if (!e_fm2_has_parent_get(cfdata->gui.o_fm))
     {
	if (cfdata->gui.o_up_button)
	  e_widget_disabled_set(cfdata->gui.o_up_button, 1);
     }
   else
     {
	if (cfdata->gui.o_up_button)
	  e_widget_disabled_set(cfdata->gui.o_up_button, 0);
     }
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ot, *ilist, *mt;
   Eina_List *l;
   E_Fm2_Config fmc;
   int i;

   o = e_widget_list_add(evas, 1, 0);
   ot = e_widget_table_add(evas, 1);

   of = e_widget_framelist_add(evas, _("Icon Themes"), 1);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->themename));
   cfdata->gui.list = ilist;
   e_widget_on_change_hook_set(ilist, _ilist_cb_change, cfdata);

   evas_event_freeze(evas_object_evas_get(ilist));
   edje_freeze();
   e_widget_ilist_freeze(ilist);
   
   cfdata->state = -1;
   i = 0;
   for (l = cfdata->icon_themes; l; l = l->next)
     {
	Efreet_Icon_Theme *theme;
	Evas_Object *oc = NULL;

	theme = l->data;
	if (theme->example_icon)
	  {
	     char *path;

	     path = efreet_icon_path_find(theme->name.internal, theme->example_icon, 24);
	     if (path)
	       {
		  oc = e_icon_add(evas);
		  e_icon_file_set(oc, path);
		  e_icon_fill_inside_set(oc, 1);
		  free(path);
	       }
	  }
	e_widget_ilist_append(ilist, oc, theme->name.name, NULL, NULL, theme->name.internal);
	if (!strcmp(cfdata->themename, theme->name.internal))
	  e_widget_ilist_selected_set(ilist, i);
	i++;
     }

   e_widget_ilist_go(ilist);
   e_widget_min_size_set(of, 160, 160);
   e_widget_ilist_thaw(ilist);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ilist));
   
   e_widget_framelist_object_append(of, ilist);
   e_widget_table_object_append(ot, of, 0, 0, 2, 4, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Icon Theme"), 0);

   mt = e_widget_textblock_add(evas);
   e_widget_textblock_plain_set(mt, "");
   e_widget_min_size_set(mt, 200, 75);
   cfdata->gui.comment = mt;
   e_widget_framelist_object_append(of, mt);

   mt = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_button_up, cfdata, NULL);
   cfdata->gui.o_up_button = mt;
   e_widget_framelist_object_append(of, mt);

   mt = e_fm2_add(evas);
   cfdata->gui.o_fm = mt;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 1;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(mt, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_SHOW_HIDDEN);
   evas_object_smart_callback_add(mt, "dir_changed",
				  _cb_files_changed, cfdata);

   ob = e_widget_scrollframe_pan_add(evas, mt,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->gui.o_frame = ob;
   e_widget_min_size_set(ob, 200, 120);
   e_widget_framelist_object_append(of, ob);
   e_box_pack_options_set(ob,
			  1, 1,
			  1, 1,
			  1, 1,
			  200, 120,
			  99999, 99999
			  );

   e_widget_table_object_append(ot, of, 2, 0, 2, 4, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   _ilist_cb_change(cfdata, ilist);
   return o;
}
*/

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ilist, *of;
   Eina_List *l;
   int i;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Icon Themes"), 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->themename));
   cfdata->gui.list = ilist;
   e_widget_on_change_hook_set(ilist, _ilist_cb_change, cfdata);

   evas_event_freeze(evas_object_evas_get(ilist));
   edje_freeze();
   e_widget_ilist_freeze(ilist);
   
   cfdata->state = -1;
   i = 0;
   for (l = cfdata->icon_themes; l; l = l->next)
     {
	Efreet_Icon_Theme *theme;
	Evas_Object *oc = NULL;

	theme = l->data;
	if (theme->example_icon)
	  {
	     char *path;

	     path = efreet_icon_path_find(theme->name.internal, theme->example_icon, 24);
	     if (path)
	       {
		  oc = e_icon_add(evas);
		  e_icon_file_set(oc, path);
		  e_icon_fill_inside_set(oc, 1);
		  free(path);
	       }
	  }
	e_widget_ilist_append(ilist, oc, theme->name.name, NULL, NULL, theme->name.internal);
	if (!strcmp(cfdata->themename, theme->name.internal))
	  e_widget_ilist_selected_set(ilist, i);
	i++;
     }

   e_widget_ilist_go(ilist);
   e_widget_min_size_set(ilist, 200, 240);
   e_widget_ilist_thaw(ilist);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(ilist));

   e_widget_framelist_object_append(of, ilist);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   _ilist_cb_change(cfdata, ilist);
   return o;
}

static void
_ilist_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Icon_Theme *theme;
   char *dir = NULL;
   char *text;
   size_t length = 0, size = 4096;

   cfdata = data;
   if (!cfdata->gui.comment) return;
   theme = efreet_icon_theme_find(cfdata->themename);
   if (!theme) return;

   text = malloc(4096);
   text[0] = 0;
   if (theme->comment)
     {
	length += strlen(theme->comment) + 1;
	while (length >= size)
	  {
	     size += 4096;
	     text = realloc(text, size);
	  }
	strcat(text, theme->comment);
	strcat(text, "\n");
     }
   if (theme->paths.count == 1)
     {
	length += strlen(theme->paths.path) + 8;
	while (length >= size)
	  {
	     size += 4096;
	     text = realloc(text, size);
	  }
	dir = theme->paths.path;
	strcat(text, "path = ");
	strcat(text, dir);
	strcat(text, "\n");
     }
   else if (theme->paths.count > 1)
     {
	char *path;
	int first = 1;

	ecore_list_first_goto(theme->paths.path);
	while ((path = ecore_list_next(theme->paths.path)))
	  {
	     length += strlen(theme->paths.path) + 16;
	     while (length >= size)
	       {
		  size += 4096;
		  text = realloc(text, size);
	       }
	     if (first)
	       {
		  dir = path;
		  strcat(text, "paths = ");
		  strcat(text, path);
		  first = 0;
	       }
	     else
	       {
		  strcat(text, ", ");
		  strcat(text, path);
	       }
	  }
	strcat(text, "\n");
     }
   if (theme->inherits)
     {
	const char *inherit;
	int first = 1;

	ecore_list_first_goto(theme->inherits);
	while ((inherit = ecore_list_next(theme->inherits)))
	  {
	     length += strlen(theme->paths.path) + 32;
	     while (length >= size)
	       {
		  size += 4096;
		  text = realloc(text, size);
	       }
	     if (first)
	       {
		  strcat(text, "inherits =  ");
		  strcat(text, inherit);
		  first = 0;
	       }
	     else
	       {
		  strcat(text, ", ");
		  strcat(text, inherit);
	       }
	  }
	strcat(text, "\n");
     }
   e_widget_textblock_plain_set(cfdata->gui.comment, text);
   free(text);
   if (dir)
     {
	dir = ecore_file_dir_get(dir);
	e_fm2_path_set(cfdata->gui.o_fm, dir, "/");
	free(dir);
     }
}

static int
_sort_icon_themes(void *data1, void *data2)
{
   Efreet_Icon_Theme *m1, *m2;

   if (!data2) return -1;

   m1 = data1;
   m2 = data2;

   if (!m1->name.name) return 1;
   if (!m2->name.name) return -1;

   return (strcmp(m1->name.name, m2->name.name));
}

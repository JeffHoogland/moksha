/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_int_config_theme_import.h"
#include "e_int_config_theme_web.h"

static void        *_create_data               (E_Config_Dialog *cfd);
static void         _free_data                 (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _fill_data                 (E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data          (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets      (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data       (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_List   *_get_theme_categories_list (void);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Basic */
   Evas_Object *o_fm;
   Evas_Object *o_up_button;
   Evas_Object *o_preview;
   Evas_Object *o_personal;
   Evas_Object *o_system;
   int fmdir;
   const char *theme;

   /* Advanced */
   Evas_Object *o_categories_ilist;
   Evas_Object *o_files_ilist;
   int         personal_file_count;
   Eina_List   *theme_list;
   Eina_List   *parts_list;

   /* Dialog */
   E_Win    *win_import;
   E_Dialog *dia_web;
};

static const char *parts_list[] = {
  "about:e/widgets/about/main",
  "borders:e/widgets/border/default/border",
  "background:e/desktop/background",
  "configure:e/widgets/configure/main",
  "dialog:e/widgets/dialog/main",
  "dnd:ZZZ",
  "error:e/error/main",
  "exebuf:e/widgets/exebuf/main",
  "fileman:ZZZ",
  "gadman:e/gadman/control",
  "icons:ZZZ",
  "menus:ZZZ",
  "modules:ZZZ",
  "modules/pager:e/widgets/pager/popup",
  "modules/ibar:ZZZ",
  "modules/ibox:ZZZ",
  "modules/clock:e/modules/clock/main",
  "modules/battery:e/modules/battery/main",
  "modules/cpufreq:e/modules/cpufreq/main",
  "modules/start:e/modules/start/main",
  "modules/temperature:e/modules/temperature/main",
  "pointer:e/pointer",
  "shelf:e/shelf/default/base",
  "transitions:ZZZ",
  "widgets:ZZZ",
  "winlist:e/widgets/winlist/main",
  NULL
};

EAPI E_Config_Dialog *
e_int_config_theme(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_theme_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->override_auto_apply = 1;
   cfd = e_config_dialog_new(con,
			     _("Theme Selector"),
			     "E", "_config_theme_dialog",
			     "preferences-desktop-theme", 0, v, NULL);
   return cfd;
}

EAPI void
e_int_config_theme_import_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->win_import = NULL;
}

EAPI void
e_int_config_theme_web_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->dia_web = NULL;
}

EAPI void
e_int_config_theme_update(E_Config_Dialog *dia, char *file)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];

   cfdata = dia->cfdata;

   cfdata->fmdir = 1;
   e_widget_radio_toggle_set(cfdata->o_personal, 1);

   snprintf(path, sizeof(path), "%s/.e/e/themes", e_user_homedir_get());
   eina_stringshare_del(cfdata->theme);
   cfdata->theme = eina_stringshare_add(file);

   if (cfdata->o_fm)
     e_widget_flist_path_set(cfdata->o_fm, path, "/");

   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, cfdata->theme, "e/desktop/background");
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->o_fm) e_widget_flist_parent_go(cfdata->o_fm);
}

static void
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata->o_fm) return;
   if (cfdata->o_up_button)
     e_widget_disabled_set(cfdata->o_up_button, 
                           !e_widget_flist_has_parent_get(cfdata->o_fm));
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];

   cfdata = data;
   if (!cfdata->o_fm) return;

   if (!(selected = e_widget_flist_selected_list_get(cfdata->o_fm))) return;

   ici = selected->data;
   realpath = e_widget_flist_real_path_get(cfdata->o_fm);

   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   eina_list_free(selected);

   if (ecore_file_is_dir(buf)) return;

   eina_stringshare_del(cfdata->theme);
   cfdata->theme = eina_stringshare_add(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "e/desktop/background");
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *p;
   char buf[4096];

   cfdata = data;
   if ((!cfdata->theme) || (!cfdata->o_fm)) return;

   p = e_widget_flist_real_path_get(cfdata->o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->theme, strlen(p))) return;
     }
   if (!p) return;

   snprintf(buf, sizeof(buf), "%s/.e/e/themes", e_user_homedir_get());
   if (!strncmp(cfdata->theme, buf, strlen(buf)))
     p = cfdata->theme + strlen(buf) + 1;
   else
     {
	snprintf(buf, sizeof(buf), "%s/data/themes", e_prefix_data_get());
	if (!strncmp(cfdata->theme, buf, strlen(buf)))
	  p = cfdata->theme + strlen(buf) + 1;
	else
	  p = cfdata->theme;
     }
   e_widget_flist_select_set(cfdata->o_fm, p, 1);
   e_widget_flist_file_show(cfdata->o_fm, p);
}

static void
_cb_dir(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];

   cfdata = data;
   if (cfdata->fmdir == 1)
     snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   else
     snprintf(path, sizeof(path), "%s/.e/e/themes", e_user_homedir_get());
   e_widget_flist_path_set(cfdata->o_fm, path, "/");
}

static void
_cb_files_files_deleted(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   cfdata = data;
   if ((!cfdata->theme) || (!cfdata->o_fm)) return;

   if (!(all = e_widget_flist_all_list_get(cfdata->o_fm))) return;
   if (!(sel = e_widget_flist_selected_list_get(cfdata->o_fm))) return;

   ici = sel->data;

   all = eina_list_data_find_list(all, ici);
   n = eina_list_next(all);
   if (!n)
     {
	n = eina_list_prev(all);
	if (!n) return;
     }

   if (!(ic = n->data)) return;

   e_widget_flist_select_set(cfdata->o_fm, ic->file, 1);
   e_widget_flist_file_show(cfdata->o_fm, ic->file);

   eina_list_free(n);

   evas_object_smart_callback_call(cfdata->o_fm, "selection_change", cfdata);
}

static void
_cb_import(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->win_import)
     e_win_raise(cfdata->win_import);
   else
     cfdata->win_import = e_int_config_theme_import(cfdata->cfd);
}

static void
_cb_web(void *data1, void *data2)
{
#ifdef HAVE_EXCHANGE
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->dia_web)
     e_win_raise(cfdata->dia_web->win);
   else
     cfdata->dia_web = e_int_config_theme_web(cfdata->cfd);
#endif
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme * c;
   char path[4096];

   c = e_theme_config_get("theme");
   if (c)
     cfdata->theme = eina_stringshare_add(c->file);
   else
     {
	snprintf(path, sizeof(path), "%s/data/themes/default.edj", 
                 e_prefix_data_get());
	cfdata->theme = eina_stringshare_add(path);
     }
   if (cfdata->theme[0] != '/')
     {
        snprintf(path, sizeof(path), "%s/.e/e/themes/%s", 
                 e_user_homedir_get(), cfdata->theme);
	if (ecore_file_exists(path))
	  {
	     eina_stringshare_del(cfdata->theme);
	     cfdata->theme = eina_stringshare_add(path);
	  }
	else
	  {
	     snprintf(path, sizeof(path), "%s/data/themes/%s", 
                      e_prefix_data_get(), cfdata->theme);
	     if (ecore_file_exists(path))
	       {
		  eina_stringshare_del(cfdata->theme);
		  cfdata->theme = eina_stringshare_add(path);
	       }
	  }
     }

   cfdata->theme_list = _get_theme_categories_list();

   snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   if (!strncmp(cfdata->theme, path, strlen(path)))
     cfdata->fmdir = 1;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme *t;

   if (cfdata->win_import)
     e_int_config_theme_del(cfdata->win_import);

   EINA_LIST_FREE(cfdata->theme_list, t)
     {
	eina_stringshare_del(t->file);
	eina_stringshare_del(t->category);
	free(t);
     }

   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *il, *ol;
   E_Zone *z;
   E_Radio_Group *rg;
   char path[4096];

   z = e_zone_current_get(cfd->con);

   ot = e_widget_table_add(evas, 0);
   ol = e_widget_table_add(evas, 0);
   il = e_widget_table_add(evas, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   o = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = o;
   evas_object_smart_callback_add(o, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(il, o, 0, 0, 1, 1, 1, 1, 0, 0);
   o = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = o;
   evas_object_smart_callback_add(o, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(il, o, 1, 0, 1, 1, 1, 1, 0, 0);

   e_widget_table_object_append(ol, il, 0, 0, 1, 1, 0, 0, 0, 0);

   o = e_widget_button_add(evas, _("Go up a Directory"), "go-up",
			   _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = o;
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   else
     snprintf(path, sizeof(path), "%s/.e/e/themes", e_user_homedir_get());

   o = e_widget_flist_add(evas);
   cfdata->o_fm = o;
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_files_files_changed, cfdata);
   evas_object_smart_callback_add(o, "files_deleted",
				  _cb_files_files_deleted, cfdata);
   e_widget_flist_path_set(o, path, "/");

   e_widget_min_size_set(o, 160, 160);
   e_widget_table_object_append(ol, o, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_list_add(evas, 0, 0);

   il = e_widget_list_add(evas, 0, 1);
   o = e_widget_button_add(evas, _(" Import..."), "preferences-desktop-theme",
			   _cb_import, cfdata, NULL);
   e_widget_list_object_append(il, o, 1, 0, 0.5);
#ifdef HAVE_EXCHANGE
   o = e_widget_button_add(evas, _(" Online..."), "network-website",
			   _cb_web, cfdata, NULL);
   e_widget_list_object_append(il, o, 1, 0, 0.5);
#endif
   e_widget_list_object_append(of, il, 1, 0, 0.0);

   {
      Evas_Object *oa;
      int mw, mh;

      mw = 320;
      mh = (mw * z->h) / z->w;
      oa = e_widget_aspect_add(evas, mw, mh);
      o = e_widget_preview_add(evas, mw, mh);
      cfdata->o_preview = o;
      if (cfdata->theme)
	e_widget_preview_edje_set(o, cfdata->theme, "e/desktop/background");
      e_widget_aspect_child_set(oa, o);
      e_widget_list_object_append(of, oa, 1, 1, 0);

   }
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Action *a;
   E_Config_Theme *ct;

   /* Actually take our cfdata settings and apply them in real life */
   ct = e_theme_config_get("theme");
   if ((ct) && (!strcmp(ct->file, cfdata->theme))) return 1;

   e_theme_config_set("theme", cfdata->theme);
   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   return 1; /* Apply was OK */
}

/*
 * --------------------------------------
 * --- A D V A N C E D  S U P P O R T ---
 * --------------------------------------
 */

static int
_cb_sort(const void *data1, const void *data2)
{
   const char *d1, *d2;

   d1 = data1;
   d2 = data2;
   if (!d1) return 1;
   if (!d2) return -1;

   return strcmp(d1, d2);
}

static Eina_List *
_get_theme_categories_list(void)
{
   Eina_List *themes, *tcl = NULL;
   Eina_List *cats = NULL, *g = NULL, *cats2 = NULL;
   const char *c;
   char *category;

   /* Setup some default theme categories */
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/about"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/borders"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/background"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/configure"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/dialog"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/dnd"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/error"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/exebuf"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/fileman"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/gadman"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/icons"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/menus"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/pager"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/ibar"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/ibox"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/clock"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/battery"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/cpufreq"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/start"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/modules/temperature"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/pointer"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/shelf"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/transitions"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/widgets"));
   cats = eina_list_append(cats, eina_stringshare_add("base/theme/winlist"));

   cats = eina_list_sort(cats, 0, _cb_sort);

   /*
    * Get a list of registered themes.
    * Add those which are not in the above list
    */
   EINA_LIST_FOREACH(e_theme_category_list(), g, c)
     {
	const char *result;

	if (!c) continue;

	cats2 = eina_list_search_sorted_near_list(cats, _cb_sort, c);
	result = eina_list_data_get(cats2);
	if (result)
	  {
	     int res;

	     res = strcmp(c, result);
	     if (!res) continue;
	     if (res < 0)
	       cats = eina_list_prepend_relative_list(cats, eina_stringshare_ref(c), cats2);
	     else
	       cats = eina_list_append_relative_list(cats, eina_stringshare_ref(c), cats2);
	  }
     }

   EINA_LIST_FREE(cats, category)
     {
	E_Config_Theme *theme, *newtheme = NULL;

	/* Not interested in adding "base" */
	if (strcmp(category, "base"))
	  {
	     newtheme = (E_Config_Theme *)malloc(sizeof(E_Config_Theme));
	     if (!newtheme) break;
	     if (!strcmp(category, "base/theme"))
	       newtheme->category = eina_stringshare_add("base/theme/Base Theme");
	     else
	       newtheme->category = eina_stringshare_ref(category);
	     newtheme->file = NULL;

	     EINA_LIST_FOREACH(e_config->themes, themes, theme)
	       {
		  if (!strcmp(category + 5, theme->category))
		    {
		       newtheme->file = eina_stringshare_add(theme->file);
		    }
	       }
	     tcl = eina_list_append(tcl, newtheme);
	  }
	eina_stringshare_del(category);
     }

   return tcl;
}

static const char *
_files_ilist_nth_label_to_file(void *data, int n)
{
   E_Config_Dialog_Data *cfdata;
   char file[1024];

   if (!(cfdata = data)) return NULL;
   if (!cfdata->o_files_ilist) return NULL;

   if (n > cfdata->personal_file_count)
     snprintf(file, sizeof(file), "%s/data/themes/%s.edj",
              e_prefix_data_get(), 
              e_widget_ilist_nth_label_get(cfdata->o_files_ilist, n));
   else
     snprintf(file, sizeof(file), "%s/.e/e/themes/%s.edj",
              e_user_homedir_get(), 
              e_widget_ilist_nth_label_get(cfdata->o_files_ilist, n));

   return eina_stringshare_add(file);
}

static void
_preview_set(void *data)
{
   E_Config_Dialog_Data *cfdata;
   const char *theme;
   char c_label[128];
   int n;

   if (!(cfdata = data)) return;

   n = e_widget_ilist_selected_get(cfdata->o_files_ilist);
   theme = _files_ilist_nth_label_to_file(cfdata, n);
   snprintf(c_label, sizeof(c_label), "%s:",
            e_widget_ilist_selected_label_get(cfdata->o_categories_ilist));
   if (theme)
     {
	int ret = 0;
	int i;

	for (i = 0; parts_list[i] != NULL; i++)
	  if (strstr(parts_list[i], c_label)) break;

	if (parts_list[i])
	  ret = e_widget_preview_edje_set(cfdata->o_preview, theme,
					  parts_list[i] + strlen(c_label));
        if (!ret)
	  ret = e_widget_preview_edje_set(cfdata->o_preview, theme,
					  "e/desktop/background");
	eina_stringshare_del(theme);
     }
}

static void
_cb_adv_categories_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   const char *label = NULL;
   const char *file = NULL;
   char category[256];
   Eina_List *themes = NULL;
   E_Config_Theme *t;
   Evas_Object *ic = NULL;
   int n;

   if (!(cfdata = data)) return;

   label = e_widget_ilist_selected_label_get(cfdata->o_categories_ilist);
   if (!label) return;

   n = e_widget_ilist_selected_get(cfdata->o_categories_ilist);
   ic = e_widget_ilist_nth_icon_get(cfdata->o_categories_ilist, n);
   if (!ic)
     {
	_preview_set(data);
	return;
     }

   snprintf(category, sizeof(category), "base/theme/%s", label);
   EINA_LIST_FOREACH(cfdata->theme_list, themes, t)
     {
	if (!strcmp(category, t->category) && (t->file))
	  {
	     file = t->file;
	     break;
	  }
     }
   if (!file) return;

   for (n = 0; n < e_widget_ilist_count(cfdata->o_files_ilist); n++)
     {
	const char *tmp;

	tmp = _files_ilist_nth_label_to_file(cfdata, n);
	eina_stringshare_del(tmp);
	if (file == tmp) /* We don't need the value, just the adress. */
	  {
	     e_widget_ilist_selected_set(cfdata->o_files_ilist, n);
	     break;
	  }
     }
}

static void
_cb_adv_theme_change(void *data, Evas_Object *obj)
{
   _preview_set(data);
}

static int
_theme_file_used(Eina_List *tlist, const char *filename)
{
   E_Config_Theme *theme;
   Eina_List *l;

   if (!filename) return 0;

   EINA_LIST_FOREACH(tlist, l, theme)
     if (theme->file == filename) return 1;

   return 0;
}

static int
_ilist_files_add(E_Config_Dialog_Data *cfdata, const char *header, const char *dir)
{
   DIR *d = NULL;
   struct dirent *dentry = NULL;
   Eina_List *themefiles = NULL;
   int count = 0;
   char themename[1024];
   char *tmp;
   Evas_Object *o;
   const char *theme;
   Evas *evas;

   o = cfdata->o_files_ilist;
   e_widget_ilist_header_append(o, NULL, header);
   evas = evas_object_evas_get(o);

   d = opendir(dir);
   if (d)
     {
	while ((dentry = readdir(d)) != NULL)
	  {
	     if (strstr(dentry->d_name,".edj") != NULL)
	       {
		  snprintf(themename, sizeof(themename), "%s/%s",
			   dir, dentry->d_name);
		  themefiles = eina_list_append(themefiles, eina_stringshare_add(themename));
	       }
	  }
	closedir(d);
     }

   if (themefiles)
     {
	themefiles = eina_list_sort(themefiles, -1, _cb_sort);
	count = eina_list_count(themefiles);

	EINA_LIST_FREE(themefiles, theme)
	  {
	     Evas_Object *ic = NULL;

	     if (_theme_file_used(cfdata->theme_list, theme))
	       {
		  ic = e_icon_add(evas);
		  e_util_icon_theme_set(ic, "preferences-desktop-theme");
	       }
	     tmp = strdup(strrchr(theme, '/') + 1);
	     strncpy(themename, tmp, strlen(tmp) - 3);
	     themename[strlen(tmp) - 4] = '\0';
	     e_widget_ilist_append(o, ic, themename, NULL, NULL, NULL);
	     free(tmp);

	     eina_stringshare_del(theme);
	  }
     }

   return count;
}

static void
_fill_files_ilist(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Evas_Object *o;
   char theme_dir[4096];

   if (!(o = cfdata->o_files_ilist)) return;

   evas = evas_object_evas_get(o);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   e_widget_ilist_clear(o);

   /* Grab the "Personal" themes. */
   snprintf(theme_dir, sizeof(theme_dir), "%s/.e/e/themes",
            e_user_homedir_get());
   cfdata->personal_file_count = 
     _ilist_files_add(cfdata, _("Personal"), theme_dir);

   /* Grab the "System" themes. */
   snprintf(theme_dir, sizeof(theme_dir),
            "%s/data/themes", e_prefix_data_get());
   _ilist_files_add(cfdata, _("System"), theme_dir);

   e_widget_ilist_go(o);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_fill_categories_ilist(E_Config_Dialog_Data *cfdata)
{
   Evas *evas;
   Eina_List *themes;
   E_Config_Theme *theme;
   Evas_Object *o;

   if (!(o = cfdata->o_categories_ilist)) return;

   evas = evas_object_evas_get(o);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   e_widget_ilist_clear(o);

   EINA_LIST_FOREACH(cfdata->theme_list, themes, theme)
     {
	Evas_Object *ic = NULL;

	if (theme->file)
	  {
	     ic = e_icon_add(evas);
	     e_util_icon_theme_set(ic, "dialog-ok-apply");
	  }
	e_widget_ilist_append(o, ic, theme->category + 11, NULL, NULL, NULL);
     }

   e_widget_ilist_go(o);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_cb_adv_btn_assign(void *data1, void *data2)
{
   Evas *evas;
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *newtheme, *t;
   Eina_List *themes;
   Evas_Object *ic = NULL, *oc = NULL, *of = NULL;
   char buf[1024];
   const char *label;
   int n;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   evas = evas_object_evas_get(oc);
   n = e_widget_ilist_selected_get(oc);
   ic = e_icon_add(evas);
   e_util_icon_theme_set(ic, "enlightenment");
   e_widget_ilist_nth_icon_set(oc, n, ic);

   newtheme = malloc(sizeof(E_Config_Theme));
   if (!newtheme) return;

   label = e_widget_ilist_selected_label_get(oc);
   snprintf(buf, sizeof(buf), "base/theme/%s", label);
   newtheme->category = eina_stringshare_add(buf);

   n = e_widget_ilist_selected_get(of);
   ic = e_icon_add(evas);
   e_util_icon_theme_set(ic, "preferences-desktop-theme");
   e_widget_ilist_nth_icon_set(of, n, ic);
   newtheme->file = _files_ilist_nth_label_to_file(cfdata, n);

   EINA_LIST_FOREACH(cfdata->theme_list, themes, t)
     {
	const char *filename = NULL;

	if (!strcmp(t->category, newtheme->category))
	  {
	     if ((t->file) && (strcmp(t->file, newtheme->file)))
	       {
		  filename = t->file;
		  t->file = NULL;

		  if (!_theme_file_used(cfdata->theme_list, filename))
		    {
		       for (n = 0; n < e_widget_ilist_count(of); n++)
			 {
			    const char *tmp;

			    tmp = _files_ilist_nth_label_to_file(cfdata, n);
			    eina_stringshare_del(tmp);
			    if (filename == tmp) /* We just need the pointer, not the value. */
			      e_widget_ilist_nth_icon_set(of, n, NULL);
			 }
		    }
	       }
	     t->file = eina_stringshare_add(newtheme->file);
	     if (filename) eina_stringshare_del(filename);
	     break;
	  }
     }
   if (!themes)
     cfdata->theme_list = eina_list_append(cfdata->theme_list, newtheme);
   else
     {
	eina_stringshare_del(newtheme->category);
	eina_stringshare_del(newtheme->file);
	free(newtheme);
     }

   return;
}

static void
_cb_adv_btn_clear(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Eina_List *themes;
   Evas_Object *oc = NULL, *of = NULL;
   char cat[1024];
   const char *label;
   const char *filename = NULL;
   int n;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   n = e_widget_ilist_selected_get(oc);
   e_widget_ilist_nth_icon_set(oc, n, NULL);

   label = e_widget_ilist_selected_label_get(oc);
   snprintf(cat, sizeof(cat), "base/theme/%s", label);

   EINA_LIST_FOREACH(cfdata->theme_list, themes, t)
     {
	if (!strcmp(t->category, cat))
	  {
	     if (t->file)
	       {
		  filename = t->file;
		  t->file = NULL;
	       }
	     break;
	  }
     }

   if ((filename) && (!_theme_file_used(cfdata->theme_list, filename)))
     {
	for (n = 0; n < e_widget_ilist_count(of); n++)
	  {
	     const char *tmp;

	     tmp = _files_ilist_nth_label_to_file(cfdata, n);
	     if (filename == tmp)
	       e_widget_ilist_nth_icon_set(of, n, NULL);
	     eina_stringshare_del(tmp);
	  }
	eina_stringshare_del(filename);
     }

   return;
}

static void
_cb_adv_btn_clearall(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Eina_List *themes;
   Evas_Object *oc = NULL, *of = NULL;
   int n;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   for (n = 0; n < e_widget_ilist_count(oc); n++)
     e_widget_ilist_nth_icon_set(oc, n, NULL);
   for (n = 0; n < e_widget_ilist_count(of); n++)
     e_widget_ilist_nth_icon_set(of, n, NULL);

   EINA_LIST_FOREACH(cfdata->theme_list, themes, t)
     {
	eina_stringshare_del(t->file);
	t->file = NULL;
     }

   return;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ot, *of, *ob, *oa, *ol;
   int mw, mh;
   E_Zone *zone;

   zone = e_zone_current_get(cfd->con);
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Theme Categories"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ob, _cb_adv_categories_change, cfdata);
   cfdata->o_categories_ilist = ob;
   e_widget_ilist_multi_select_set(ob, 0);
   e_widget_min_size_set(ob, 150, 250);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 0, 1);

   of = e_widget_framelist_add(evas, _("Themes"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ob, _cb_adv_theme_change, cfdata);
   cfdata->o_files_ilist = ob;
   e_widget_min_size_set(ob, 150, 250);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   ol = e_widget_list_add(evas, 1, 1);
   ob = e_widget_button_add(evas, _("Assign"), NULL, 
			    _cb_adv_btn_assign, cfdata, NULL);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_button_add(evas, _("Clear"), NULL, 
			    _cb_adv_btn_clear, cfdata, NULL);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   ob = e_widget_button_add(evas, _("Clear All"), NULL, 
			    _cb_adv_btn_clearall, cfdata, NULL);
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);
   e_widget_table_object_append(ot, ol, 0, 1, 1, 1, 1, 0, 0, 0);

   of = e_widget_framelist_add(evas, _("Preview"), 0);
   mw = 320;
   mh = (mw * zone->h) / zone->w;
   oa = e_widget_aspect_add(evas, mw, mh);
   ob = e_widget_preview_add(evas, mw, mh);
   cfdata->o_preview = ob;
   if (cfdata->theme)
     e_widget_preview_edje_set(ob, cfdata->theme, "e/desktop/background");
   e_widget_aspect_child_set(oa, ob);
   e_widget_framelist_object_append(of, oa);
   e_widget_table_object_append(ot, of, 2, 0, 1, 1, 1, 1, 1, 1);

   _fill_files_ilist(cfdata);
   _fill_categories_ilist(cfdata);

   e_widget_ilist_selected_set(cfdata->o_files_ilist, 1);
   e_widget_ilist_selected_set(cfdata->o_categories_ilist, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme *theme;
   Eina_List *themes;
   E_Action *a;

   EINA_LIST_FOREACH(cfdata->theme_list, themes, theme)
     {
	E_Config_Theme *ec_theme;
	Eina_List *ec_themes;

	if (!strcmp(theme->category, "base/theme/Base Theme"))
	  theme->category = eina_stringshare_add("base/theme");

	EINA_LIST_FOREACH(e_config->themes, ec_themes, ec_theme)
	  {
	     if (!strcmp(theme->category + 5, ec_theme->category))
	       {
		  if (theme->file)
		    e_theme_config_set(theme->category + 5, theme->file);
		  else
		    e_theme_config_remove(theme->category + 5);
		  break;
	       }
	  }
	if ((!ec_themes) && (theme->file))
	  e_theme_config_set(theme->category + 5, theme->file);
     }

   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);

   return 1;
}

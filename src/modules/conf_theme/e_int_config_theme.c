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
static Evas_List   *_get_theme_categories_list (void);
static Evas_List   *_get_parts_list            (void);

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
   char *theme;

   /* Advanced */
   Evas_Object *o_categories_ilist;
   Evas_Object *o_files_ilist;
   int         personal_file_count;
   Evas_List   *theme_list;
   Evas_List   *parts_list;

   /* Dialog */
   E_Win    *win_import;
   E_Dialog *dia_web;
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
			     "enlightenment/themes", 0, v, NULL);
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
   E_FREE(cfdata->theme);
   cfdata->theme = strdup(file);

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
   Evas_List *selected;
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
   evas_list_free(selected);

   if (ecore_file_is_dir(buf)) return;

   E_FREE(cfdata->theme);
   cfdata->theme = strdup(buf);
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
   Evas_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   cfdata = data;
   if ((!cfdata->theme) || (!cfdata->o_fm)) return;

   if (!(all = e_widget_flist_all_list_get(cfdata->o_fm))) return;
   if (!(sel = e_widget_flist_selected_list_get(cfdata->o_fm))) return;

   ici = sel->data;

   all = evas_list_find_list(all, ici);
   n = evas_list_next(all);
   if (!n)
     {
	n = evas_list_prev(all);
	if (!n) return;
     }

   if (!(ic = n->data)) return;

   e_widget_flist_select_set(cfdata->o_fm, ic->file, 1);
   e_widget_flist_file_show(cfdata->o_fm, ic->file);

   evas_list_free(n);

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
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->dia_web)
     e_win_raise(cfdata->dia_web->win);
   else
     cfdata->dia_web = e_int_config_theme_web(cfdata->cfd);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme * c;
   char path[4096];

   c = e_theme_config_get("theme");
   if (c)
     cfdata->theme = strdup(c->file);
   else
     {
	snprintf(path, sizeof(path), "%s/data/themes/default.edj", 
                 e_prefix_data_get());
	cfdata->theme = strdup(path);
     }
   if (cfdata->theme[0] != '/')
     {
        snprintf(path, sizeof(path), "%s/.e/e/themes/%s", 
                 e_user_homedir_get(), cfdata->theme);
	if (ecore_file_exists(path))
	  {
	     E_FREE(cfdata->theme);
	     cfdata->theme = strdup(path);
	  }
	else
	  {
	     snprintf(path, sizeof(path), "%s/data/themes/%s", 
                      e_prefix_data_get(), cfdata->theme);
	     if (ecore_file_exists(path))
	       {
		  E_FREE(cfdata->theme);
		  cfdata->theme = strdup(path);
	       }
	  }
     }

   cfdata->theme_list = _get_theme_categories_list();
   cfdata->parts_list = _get_parts_list();

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
   if (cfdata->win_import)
     e_int_config_theme_del(cfdata->win_import);

   evas_list_free(cfdata->theme_list);
   E_FREE(cfdata->theme);
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

   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
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
   o = e_widget_button_add(evas, _(" Import..."), "enlightenment/themes",
			   _cb_import, cfdata, NULL);
   e_widget_list_object_append(il, o, 1, 0, 0.5);
   if (ecore_file_download_protocol_available("http://"))
   {
      o = e_widget_button_add(evas, _(" Online..."), "enlightenment/website",
			      _cb_web, cfdata, NULL);
      e_widget_list_object_append(il, o, 1, 0, 0.5);
   }	   
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
_cb_sort(void *data1, void *data2)
{
   const char *d1, *d2;

   d1 = data1;
   d2 = data2;
   if (!d1) return 1;
   if (!d2) return -1;

   return strcmp(d1, d2);
}

static Evas_List *
_get_parts_list(void)
{
   Evas_List *parts = NULL;

   /*
    * TODO: Those parts with ZZZ, are the ones I could not find a suitable
    * part to use as its preview.
    */
   parts = evas_list_append(parts, strdup("about:e/widgets/about/main"));
   parts = evas_list_append(parts, strdup("borders:e/widgets/border/default/border"));
   parts = evas_list_append(parts, strdup("background:e/desktop/background"));
   parts = evas_list_append(parts, strdup("configure:e/widgets/configure/main"));
   parts = evas_list_append(parts, strdup("dialog:e/widgets/dialog/main"));
   parts = evas_list_append(parts, strdup("dnd:ZZZ"));
   parts = evas_list_append(parts, strdup("error:e/error/main"));
   parts = evas_list_append(parts, strdup("exebuf:e/widgets/exebuf/main"));
   parts = evas_list_append(parts, strdup("fileman:ZZZ"));
   parts = evas_list_append(parts, strdup("gadman:e/gadman/control"));
   parts = evas_list_append(parts, strdup("icons:ZZZ"));
   parts = evas_list_append(parts, strdup("menus:ZZZ"));
   parts = evas_list_append(parts, strdup("modules:ZZZ"));
   parts = evas_list_append(parts, strdup("modules/pager:e/widgets/pager/popup"));
   parts = evas_list_append(parts, strdup("modules/ibar:ZZZ"));
   parts = evas_list_append(parts, strdup("modules/ibox:ZZZ"));
   parts = evas_list_append(parts, strdup("modules/clock:e/modules/clock/main"));
   parts = evas_list_append(parts, strdup("modules/battery:e/modules/battery/main"));
   parts = evas_list_append(parts, strdup("modules/cpufreq:e/modules/cpufreq/main"));
   parts = evas_list_append(parts, strdup("modules/start:e/modules/start/main"));
   parts = evas_list_append(parts, strdup("modules/temperature:e/modules/temperature/main"));
   parts = evas_list_append(parts, strdup("pointer:e/pointer"));
   parts = evas_list_append(parts, strdup("shelf:e/shelf/default/base"));
   parts = evas_list_append(parts, strdup("transitions:ZZZ"));
   parts = evas_list_append(parts, strdup("widgets:ZZZ"));
   parts = evas_list_append(parts, strdup("winlist:e/widgets/winlist/main"));

   return parts;
}

static Evas_List *
_get_theme_categories_list(void)
{
   Evas_List *themes, *tcl = NULL;
   Evas_List *cats = NULL, *g = NULL, *cats2 = NULL;
   const char *category;

   /* Setup some default theme categories */
   cats = evas_list_append(cats, strdup("base/theme/about"));
   cats = evas_list_append(cats, strdup("base/theme/borders"));
   cats = evas_list_append(cats, strdup("base/theme/background"));
   cats = evas_list_append(cats, strdup("base/theme/configure"));
   cats = evas_list_append(cats, strdup("base/theme/dialog"));
   cats = evas_list_append(cats, strdup("base/theme/dnd"));
   cats = evas_list_append(cats, strdup("base/theme/error"));
   cats = evas_list_append(cats, strdup("base/theme/exebuf"));
   cats = evas_list_append(cats, strdup("base/theme/fileman"));
   cats = evas_list_append(cats, strdup("base/theme/gadman"));
   cats = evas_list_append(cats, strdup("base/theme/icons"));
   cats = evas_list_append(cats, strdup("base/theme/menus"));
   cats = evas_list_append(cats, strdup("base/theme/modules"));
   cats = evas_list_append(cats, strdup("base/theme/modules/pager"));
   cats = evas_list_append(cats, strdup("base/theme/modules/ibar"));
   cats = evas_list_append(cats, strdup("base/theme/modules/ibox"));
   cats = evas_list_append(cats, strdup("base/theme/modules/clock"));
   cats = evas_list_append(cats, strdup("base/theme/modules/battery"));
   cats = evas_list_append(cats, strdup("base/theme/modules/cpufreq"));
   cats = evas_list_append(cats, strdup("base/theme/modules/start"));
   cats = evas_list_append(cats, strdup("base/theme/modules/temperature"));
   cats = evas_list_append(cats, strdup("base/theme/pointer"));
   cats = evas_list_append(cats, strdup("base/theme/shelf"));
   cats = evas_list_append(cats, strdup("base/theme/transitions"));
   cats = evas_list_append(cats, strdup("base/theme/widgets"));
   cats = evas_list_append(cats, strdup("base/theme/winlist"));

   /*
    * Get a list of registered themes.
    * Add those which are not in the above list
    */
   for (g = e_theme_category_list(); g; g = g->next)
     {
	const char *c;

	if (!(c = g->data)) continue;

	cats2 = cats;
	while (cats2)
	  {
	     if (!strcmp(c, cats2->data)) break;
	     cats2 = cats2->next;
	  }
	if (!cats2)
	  cats = evas_list_append(cats, strdup(c));
     }
   cats = evas_list_sort(cats, -1, _cb_sort);

   while (cats)
     {
	E_Config_Theme *theme, *newtheme = NULL;

	category = cats->data;
	/* Not interested in adding "base" */
	if (strcmp(category, "base"))
	  {
	     newtheme = (E_Config_Theme *)malloc(sizeof(E_Config_Theme));
	     if (!newtheme) break;
	     if (!strcmp(category, "base/theme"))
	       newtheme->category = strdup("base/theme/Base Theme");
	     else
	       newtheme->category = strdup(category);
	     newtheme->file = NULL;

	     for (themes = e_config->themes; themes; themes = themes->next)
	       {
		  theme = themes->data;
		  if (!strcmp(category + 5, theme->category))
                    newtheme->file = strdup(theme->file);
	       }
	     tcl = evas_list_append(tcl, newtheme);
	  }
	cats = cats->next;
     }
   cats = evas_list_free(cats);

   return tcl;
}

static char *
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

   return strdup(file);
}

static void
_preview_set(void *data)
{
   E_Config_Dialog_Data *cfdata;
   const char *theme;
   char c_label[128];
   int n, ret = 0;
   Evas_List *p;

   if (!(cfdata = data)) return;

   n = e_widget_ilist_selected_get(cfdata->o_files_ilist);
   theme = _files_ilist_nth_label_to_file(cfdata, n);
   snprintf(c_label, sizeof(c_label), "%s:",
            e_widget_ilist_selected_label_get(cfdata->o_categories_ilist));
   if (theme)
     {
	p = cfdata->parts_list;
	while (p)
	  {
	     if (strstr((char *)(p->data), c_label)) break;
	     p = p->next;
	  }
	if (p)
	  ret = e_widget_preview_edje_set(cfdata->o_preview, theme,
					  (char *)p->data + strlen(c_label));
        if (!ret)
	  ret = e_widget_preview_edje_set(cfdata->o_preview, theme,
					  "e/desktop/background");
     }
}

static void
_cb_adv_categories_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   const char *label = NULL;
   const char *file = NULL;
   char category[256];
   Evas_List *themes = NULL;
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
   for (themes = cfdata->theme_list; themes; themes = themes->next)
     {
	E_Config_Theme *t;

	t = themes->data;
	if (!strcmp(category, t->category) && (t->file))
	  {
	     file = strdup(t->file);
	     break;
	  }
     }
   if (!file) return;

   for (n = 0; n < e_widget_ilist_count(cfdata->o_files_ilist); n++)
     {
	if (!strcmp(file, _files_ilist_nth_label_to_file(cfdata, n)))
	  {
	     e_widget_ilist_selected_set(cfdata->o_files_ilist, n);
	     break;
	  }
     }

   free((void *)file);
}

static void
_cb_adv_theme_change(void *data, Evas_Object *obj)
{
   _preview_set(data);
}

static int
_theme_file_used(Evas_List *tlist, const char *filename)
{
   E_Config_Theme *theme;

   if (!filename) return 0;

   while (tlist)
     {
	theme = tlist->data;
	if (theme->file && !strcmp(theme->file, filename)) return 1;
	tlist = tlist->next;
     }

   return 0;
}

static int
_ilist_files_add(E_Config_Dialog_Data *cfdata, const char *header, const char *dir)
{
   DIR *d = NULL;
   struct dirent *dentry = NULL;
   Evas_List *themefiles = NULL;
   int count = 0;
   char themename[1024];
   char *tmp;
   Evas_Object *o;
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
		  themefiles = evas_list_append(themefiles, strdup(themename));
	       }
	  }
	closedir(d);
     }

   if (themefiles)
     {
	themefiles = evas_list_sort(themefiles, -1, _cb_sort);
	count = evas_list_count(themefiles);

	while (themefiles)
	  {
	     Evas_Object *ic = NULL;

	     if (_theme_file_used(cfdata->theme_list, themefiles->data))
	       {
		  ic = edje_object_add(evas);
		  e_util_edje_icon_set(ic, "enlightenment/themes");
	       }
	     tmp = strdup(strrchr(themefiles->data, '/') + 1);
	     strncpy(themename, tmp, strlen(tmp)-3);
	     themename[strlen(tmp)-4] = '\0';
	     e_widget_ilist_append(o, ic, themename, NULL, NULL, NULL);
	     free(tmp);

	     themefiles = themefiles->next;
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
   Evas_List *themes;
   E_Config_Theme *theme;
   Evas_Object *o;

   if (!(o = cfdata->o_categories_ilist)) return;

   evas = evas_object_evas_get(o);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   e_widget_ilist_clear(o);

   themes = cfdata->theme_list;
   while (themes)
     {
	Evas_Object *ic = NULL;

	theme = themes->data;
	if (theme->file)
	  {
	     ic = edje_object_add(evas);
	     e_util_edje_icon_set(ic, "enlightenment/check");
	  }
	e_widget_ilist_append(o, ic, theme->category + 11, NULL, NULL, NULL);
	themes = themes->next;
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
   Evas_List *themes;
   Evas_Object *ic = NULL, *oc = NULL, *of = NULL;
   char buf[1024];
   const char *label;
   int n;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   evas = evas_object_evas_get(oc);
   n = e_widget_ilist_selected_get(oc);
   ic = edje_object_add(evas);
   e_util_edje_icon_set(ic, "enlightenment/e");
   e_widget_ilist_nth_icon_set(oc, n, ic);

   newtheme = malloc(sizeof(E_Config_Theme));
   if (!newtheme) return;

   label = e_widget_ilist_selected_label_get(oc);
   snprintf(buf, sizeof(buf), "base/theme/%s", label);
   newtheme->category = strdup(buf);

   n = e_widget_ilist_selected_get(of);
   ic = edje_object_add(evas);
   e_util_edje_icon_set(ic, "enlightenment/themes");
   e_widget_ilist_nth_icon_set(of, n, ic);
   newtheme->file = _files_ilist_nth_label_to_file(cfdata, n);

   for (themes = cfdata->theme_list; themes; themes = themes->next)
     {
	const char *filename = NULL;

	t = themes->data;
	if (!strcmp(t->category, newtheme->category))
	  {
	     if ((t->file) && (strcmp(t->file, newtheme->file)))
	       {
		  filename = eina_stringshare_add(t->file);
		  free((void *)(t->file));
		  t->file = NULL;
		  if (!_theme_file_used(cfdata->theme_list, filename))
		    {
		       for (n = 0; n < e_widget_ilist_count(of); n++)
			 if (!strcmp(filename, _files_ilist_nth_label_to_file(cfdata, n)))
			   e_widget_ilist_nth_icon_set(of, n, NULL);
		    }
	       }
	     t->file = strdup(newtheme->file);
	     if (filename) eina_stringshare_del(filename);
	     break;
	  }
     }
   if (!themes)
     cfdata->theme_list = evas_list_append(cfdata->theme_list, newtheme);
   else
     free(newtheme);

   return;
}

static void
_cb_adv_btn_clear(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Evas_List *themes;
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

   for (themes = cfdata->theme_list; themes; themes = themes->next)
     {
	t = themes->data;
	if (!strcmp(t->category, cat))
	  {
	     if (t->file)
	       {
		  filename = eina_stringshare_add(t->file);
		  free((void *)(t->file));
		  t->file = NULL;
	       }
	     break;
	  }
     }

   if ((filename) && (!_theme_file_used(cfdata->theme_list, filename)))
     {
	for (n = 0; n < e_widget_ilist_count(of); n++)
	  if (!strcmp(filename, _files_ilist_nth_label_to_file(cfdata, n)))
	    e_widget_ilist_nth_icon_set(of, n, NULL);
	eina_stringshare_del(filename);
     }

   return;
}

static void
_cb_adv_btn_clearall(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Evas_List *themes;
   Evas_Object *oc = NULL, *of = NULL;
   int n;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   for (n = 0; n < e_widget_ilist_count(oc); n++)
     e_widget_ilist_nth_icon_set(oc, n, NULL);
   for (n = 0; n < e_widget_ilist_count(of); n++)
     e_widget_ilist_nth_icon_set(of, n, NULL);

   for (themes = cfdata->theme_list; themes; themes = themes->next)
     {
	t = themes->data;
	if (t->file)
	  {
	     free((void *)(t->file));
	     t->file = NULL;
	  }
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
   Evas_List *themes, *ec_themes;
   E_Action *a;

   themes = cfdata->theme_list;
   while (themes)
     {
	E_Config_Theme *theme, *ec_theme;

	theme = themes->data;
	if (!strcmp(theme->category, "base/theme/Base Theme"))
	      theme->category = strdup("base/theme");
	for (ec_themes = e_config->themes; ec_themes; ec_themes = ec_themes->next)
	  {
	     ec_theme = ec_themes->data;
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

	themes = themes->next;
     }

   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);

   return 1;
}

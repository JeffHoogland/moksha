/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static int _sort_icon_themes(const void *data1, const void *data2);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Eina_List *icon_themes;
   char *themename;
   int overrides;
   struct {
      Evas_Object *list;
      Evas_Object *checkbox;
   } gui;
   Ecore_Idler *fill_icon_themes_delayed;
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

   cfd = e_config_dialog_new(con,
			     _("Icon Theme Settings"),
			     "E", "_config_icon_theme_dialog",
			     "enlightenment/icon_theme", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->icon_themes = efreet_icon_theme_list_get();
   cfdata->icon_themes = eina_list_sort(cfdata->icon_themes,
					eina_list_count(cfdata->icon_themes),
					_sort_icon_themes);

   return;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   if (e_config->icon_theme)
     cfdata->themename = strdup(e_config->icon_theme);
   else
     cfdata->themename = NULL;

   cfdata->overrides = e_config->icon_theme_overrides;

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->fill_icon_themes_delayed)
     free(ecore_idler_del(cfdata->fill_icon_themes_delayed));

   eina_list_free(cfdata->icon_themes);
   E_FREE(cfdata->themename);
   E_FREE(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if ((Eina_Bool)cfdata->overrides != e_config->icon_theme_overrides)
     return 1;

   if ((!cfdata->themename) && (!e_config->icon_theme))
     return 0;

   if ((!cfdata->themename) || (!e_config->icon_theme))
     return 1;

   return strcmp(cfdata->themename, e_config->icon_theme) != 0;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Event_Config_Icon_Theme *ev;
   const char *s;

   if (!_basic_check_changed(cfd, cfdata))
     return 1;

   s = eina_stringshare_add(cfdata->themename);
   eina_stringshare_del(e_config->icon_theme);
   e_config->icon_theme = s;

   e_config->icon_theme_overrides = !!cfdata->overrides;

   e_config_save_queue();

   ev = E_NEW(E_Event_Config_Icon_Theme, 1);
   if (ev)
     {
	ev->icon_theme = e_config->icon_theme;
	ecore_event_add(E_EVENT_CONFIG_ICON_THEME, ev, NULL, NULL);
     }
   return 1;
}

struct _fill_icon_themes_data
{
   Eina_List *l;
   int i;
   Evas *evas;
   E_Config_Dialog_Data *cfdata;
   Eina_Bool themes_loaded;
};

static int
_fill_icon_themes(void *data)
{
   struct _fill_icon_themes_data *d = data;
   Efreet_Icon_Theme *theme;
   Evas_Object *oc = NULL;

   if (!d->themes_loaded)
     {
	d->cfdata->icon_themes = eina_list_free(d->cfdata->icon_themes);
	_fill_data(d->cfdata);
	d->l = d->cfdata->icon_themes;
	d->i = 0;
	d->themes_loaded = 1;
	return 1;
     }

   if (!d->l)
     {
	e_widget_ilist_go(d->cfdata->gui.list);
	d->cfdata->fill_icon_themes_delayed = NULL;
	free(d);
	return 0;
     }

   theme = d->l->data;
   if (theme->example_icon)
     {
	char *path;

	path = efreet_icon_path_find(theme->name.internal, theme->example_icon, 24);
	if (path)
	  {
	     oc = e_icon_add(d->evas);
	     e_icon_file_set(oc, path);
	     e_icon_fill_inside_set(oc, 1);
	     free(path);
	  }
     }

   e_widget_ilist_append(d->cfdata->gui.list, oc, theme->name.name,
			 NULL, NULL, theme->name.internal);
   if ((d->cfdata->themename) && (theme->name.internal) &&
       (strcmp(d->cfdata->themename, theme->name.internal) == 0))
     e_widget_ilist_selected_set(d->cfdata->gui.list, d->i);
   d->i++;
   d->l = d->l->next;
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ilist, *of, *checkbox;
   struct _fill_icon_themes_data *d;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Icon Themes"), 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->themename));
   cfdata->gui.list = ilist;

   e_widget_min_size_set(ilist, 200, 240);

   e_widget_framelist_object_append(of, ilist);

   checkbox = e_widget_check_add(evas, _("Icon theme overrides general theme"), &(cfdata->overrides));
   e_widget_framelist_object_append(of, checkbox);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);

   if (cfdata->fill_icon_themes_delayed)
     free(ecore_idler_del(cfdata->fill_icon_themes_delayed));

   d = malloc(sizeof(*d));
   d->l = NULL;
   d->cfdata = cfdata;
   d->themes_loaded = 0;
   d->evas = evas;
   cfdata->fill_icon_themes_delayed = ecore_idler_add(_fill_icon_themes, d);

   return o;
}

static int
_sort_icon_themes(const void *data1, const void *data2)
{
   const Efreet_Icon_Theme *m1, *m2;

   if (!data2) return -1;

   m1 = data1;
   m2 = data2;

   if (!m1->name.name) return 1;
   if (!m2->name.name) return -1;

   return (strcmp(m1->name.name, m2->name.name));
}

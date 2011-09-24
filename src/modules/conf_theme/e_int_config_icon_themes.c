#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _sort_icon_themes(const void *data1, const void *data2);
static Evas_Object *_icon_new(Evas *evas, const char *theme, const char *icon, unsigned int size);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Eina_List *icon_themes;
   const char *themename;
   int overrides;
   int enable_icon_theme;
   int match_e17_icon_theme;
   int populating;
   struct 
     {
        Evas_Object *list;
        Evas_Object *checkbox;
        Evas_Object *preview[4]; /* same size as _icon_previews */
     } gui;
   Ecore_Idler *fill_icon_themes_delayed;
};

static const char *_icon_previews[4] = 
{
   "system-run",
   "system-file-manager",
   "preferences-desktop-theme",
   "text-x-generic"
};

#define PREVIEW_SIZE (48)

E_Config_Dialog *
e_int_config_icon_themes(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/icon_theme")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Icon Theme Settings"), 
                             "E", "appearance/icon_theme",
			     "preferences-icon-theme", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->icon_themes = efreet_icon_theme_list_get();
   cfdata->icon_themes = eina_list_sort(cfdata->icon_themes, 
                                        eina_list_count(cfdata->icon_themes),
                                        _sort_icon_themes);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   cfdata->themename = eina_stringshare_add(e_config->icon_theme);
   cfdata->overrides = e_config->icon_theme_overrides;
   cfdata->match_e17_icon_theme = e_config->xsettings.match_e17_icon_theme;
   cfdata->enable_icon_theme = !!(e_config->icon_theme);   
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->fill_icon_themes_delayed)
     free(ecore_idler_del(cfdata->fill_icon_themes_delayed));

   eina_list_free(cfdata->icon_themes);
   eina_stringshare_del(cfdata->themename);
   E_FREE(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->overrides != e_config->icon_theme_overrides)
     return 1;

   if (cfdata->match_e17_icon_theme != e_config->xsettings.match_e17_icon_theme)
     return 1;

   if (cfdata->enable_icon_theme != !!(e_config->icon_theme))
     return 1;

   if ((!cfdata->themename) && (!e_config->icon_theme))
     return 0;

   if ((!cfdata->themename) || (!e_config->icon_theme))
     return 1;

   return strcmp(cfdata->themename, e_config->icon_theme) != 0;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Event_Config_Icon_Theme *ev;

   if (!_basic_check_changed(cfd, cfdata)) return 1;

   eina_stringshare_del(e_config->icon_theme);
   if (cfdata->enable_icon_theme)
     e_config->icon_theme = eina_stringshare_ref(cfdata->themename);
   else
     e_config->icon_theme = NULL;
   
   e_config->icon_theme_overrides = !!cfdata->overrides;
   e_config->xsettings.match_e17_icon_theme = cfdata->match_e17_icon_theme;
   e_config_save_queue();

   e_util_env_set("E_ICON_THEME", e_config->icon_theme);

   ev = E_NEW(E_Event_Config_Icon_Theme, 1);
   if (ev)
     {
	ev->icon_theme = e_config->icon_theme;
	ecore_event_add(E_EVENT_CONFIG_ICON_THEME, ev, NULL, NULL);
     }
   return 1;
}

static void
_populate_preview(E_Config_Dialog_Data *cfdata)
{
   const char *t = cfdata->themename;
   unsigned int i;

   for (i = 0; i < sizeof(_icon_previews)/sizeof(_icon_previews[0]); i++)
     {
	const char *path;

        if (!(path = efreet_icon_path_find(t, _icon_previews[i], PREVIEW_SIZE)))
          continue;
	if (e_icon_file_set(cfdata->gui.preview[i], path))
	  e_icon_fill_inside_set(cfdata->gui.preview[i], EINA_TRUE);
     }
}

struct _fill_icon_themes_data
{
   Eina_List *l;
   int i;
   Evas *evas;
   E_Config_Dialog_Data *cfdata;
   Eina_Bool themes_loaded;
};

static Eina_Bool
_fill_icon_themes(void *data)
{
   struct _fill_icon_themes_data *d = data;
   Efreet_Icon_Theme *theme;
   Evas_Object *oc = NULL;
   const char **example_icon, *example_icons[] = 
     {
        NULL,
        "folder",
        "user-home",
        "text-x-generic",
        "system-run",
        "preferences-system",
        NULL,
     };

   if (!d->themes_loaded)
     {
	d->cfdata->icon_themes = eina_list_free(d->cfdata->icon_themes);
	_fill_data(d->cfdata);
	d->l = d->cfdata->icon_themes;
	d->i = 0;
	d->themes_loaded = 1;
	return ECORE_CALLBACK_RENEW;
     }

   if (!d->l)
     {
        int mw, mh;

	e_widget_ilist_go(d->cfdata->gui.list);

        e_widget_size_min_get(d->cfdata->gui.list, &mw, &mh);
        e_widget_size_min_set(d->cfdata->gui.list, mw, 100);

	d->cfdata->fill_icon_themes_delayed = NULL;
	d->cfdata->populating = EINA_FALSE;
	_populate_preview(d->cfdata);
	free(d);
	return ECORE_CALLBACK_CANCEL;
     }

   theme = d->l->data;
   if (theme->example_icon)
     {
	example_icons[0] = theme->example_icon;
	example_icon = example_icons;
     }
   else
     example_icon = example_icons + 1;

   for (; (*example_icon) && (!oc); example_icon++)
     oc = _icon_new(d->evas, theme->name.internal, *example_icon, 24);

   if (oc)
     {
	e_widget_ilist_append(d->cfdata->gui.list, oc, theme->name.name,
			      NULL, NULL, theme->name.internal);
	if ((d->cfdata->themename) && (theme->name.internal) &&
	    (strcmp(d->cfdata->themename, theme->name.internal) == 0))
	  e_widget_ilist_selected_set(d->cfdata->gui.list, d->i);
     }

   d->i++;
   d->l = d->l->next;
   return ECORE_CALLBACK_RENEW;
}

static void
_icon_theme_changed(void *data, Evas_Object *o __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->populating) return;
   _populate_preview(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ilist, *checkbox, *ol;
   struct _fill_icon_themes_data *d;
   unsigned int i;

   o = e_widget_list_add(evas, 0, 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->themename));
   cfdata->gui.list = ilist;
   e_widget_size_min_set(cfdata->gui.list, 100, 100);

   cfdata->populating = EINA_TRUE;
   e_widget_on_change_hook_set(ilist, _icon_theme_changed, cfdata);
   e_widget_list_object_append(o, ilist, 1, 1, 0.5);

   ol = e_widget_framelist_add(evas, _("Preview"), 1);
   for (i = 0; i < sizeof(_icon_previews)/sizeof(_icon_previews[0]); i++)
     {
	cfdata->gui.preview[i] = e_icon_add(evas);
	e_icon_preload_set(cfdata->gui.preview[i], EINA_TRUE);
	e_icon_scale_size_set(cfdata->gui.preview[i], PREVIEW_SIZE);
	e_widget_framelist_object_append_full(ol, cfdata->gui.preview[i], 
                                              0, 0, 0, 0, 0.5, 0.5,
                                              PREVIEW_SIZE, PREVIEW_SIZE, 
                                              PREVIEW_SIZE, PREVIEW_SIZE);
     }
   e_widget_list_object_append(o, ol, 0, 0, 0.5);

   checkbox = e_widget_check_add(evas, _("Enable icon theme"), 
                                 &(cfdata->enable_icon_theme));
   e_widget_on_change_hook_set(checkbox, _icon_theme_changed, cfdata);
   e_widget_list_object_append(o, checkbox, 0, 0, 0.0);

   checkbox = e_widget_check_add(evas, _("Use icon theme for applications"), 
                                 &(cfdata->match_e17_icon_theme));
   e_widget_list_object_append(o, checkbox, 0, 0, 0.0);

   checkbox = e_widget_check_add(evas, _("Icons override general theme"), 
                                 &(cfdata->overrides));
   e_widget_list_object_append(o, checkbox, 0, 0, 0.0);

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

static Evas_Object *
_icon_new(Evas *evas, const char *theme, const char *icon, unsigned int size)
{
   Evas_Object *o;
   const char *path;

   if (!(path = efreet_icon_path_find(theme, icon, size))) return NULL;
   o = e_icon_add(evas);
   if (e_icon_file_set(o, path))
     e_icon_fill_inside_set(o, 1);
   else
     {
	evas_object_del(o);
	o = NULL;
     }

   return o;
}

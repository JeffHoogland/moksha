#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
//static int _sort_widget_themes(const void *data1, const void *data2);
//static Evas_Object *_icon_new(Evas *evas, const char *theme, const char *icon, unsigned int size);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Eina_List       *widget_themes;
   const char      *widget_theme;
   int              enable_xsettings;
   int              match_e17_theme;
   int              match_e17_icon_theme;
   Eina_List       *icon_themes;
   const char      *icon_theme;
   int              icon_overrides;
   //int              enable_icon_theme; // We just need to check whether override or match icon theme is set.
   int              icon_populating;
   struct
   {
      Evas_Object *widget_list;
      Evas_Object *icon_list;
      Evas_Object *icon_preview[4];   /* same size as _icon_previews */
   } gui;
   Ecore_Idler     *fill_icon_themes_delayed;
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
e_int_config_xsettings(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/xsettings")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Application Theme Settings"),
                             "E", "appearance/xsettings",
                             "preferences-desktop-theme", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   cfdata->widget_theme = eina_stringshare_add(e_config->xsettings.net_theme_name);
   cfdata->match_e17_icon_theme = e_config->xsettings.match_e17_icon_theme;
   cfdata->match_e17_theme = e_config->xsettings.match_e17_theme;
   cfdata->enable_xsettings = e_config->xsettings.enabled;
   cfdata->icon_theme = eina_stringshare_add(e_config->icon_theme);
   cfdata->icon_overrides = e_config->icon_theme_overrides;
   //cfdata->enable_icon_theme = !!(e_config->icon_theme);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->fill_icon_themes_delayed)
     free(ecore_idler_del(cfdata->fill_icon_themes_delayed));

   eina_list_free(cfdata->icon_themes);
   eina_stringshare_del(cfdata->icon_theme);
   eina_stringshare_del(cfdata->widget_theme);
   E_FREE(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->match_e17_icon_theme != e_config->xsettings.match_e17_icon_theme)
     return 1;

   if (cfdata->match_e17_theme != e_config->xsettings.match_e17_theme)
     return 1;

   if (cfdata->enable_xsettings != !!(e_config->xsettings.enabled))
     return 1;

   if ((!cfdata->widget_theme) && e_config->xsettings.net_theme_name)
     return 1;

   if (cfdata->widget_theme && (!e_config->xsettings.net_theme_name))
     return 1;

   if (cfdata->icon_overrides != e_config->icon_theme_overrides)
     return 1;

   /* if (cfdata->enable_icon_theme != !!(e_config->icon_theme))
    *   return 1; */

   if ((!cfdata->icon_theme) && (e_config->icon_theme))
     return 1;

   if ((cfdata->icon_theme) && (!e_config->icon_theme))
     return 1;

   if ((cfdata->widget_theme && e_config->xsettings.net_theme_name) &&
       (strcmp(cfdata->widget_theme, e_config->xsettings.net_theme_name) != 0))
     return 1;

   if ((cfdata->icon_theme && e_config->icon_theme) &&
       (strcmp(cfdata->icon_theme, e_config->icon_theme) != 0))
     return 1;

   return 0;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Event_Config_Icon_Theme *ev;

   if (!_basic_check_changed(cfd, cfdata)) return 1;

   e_widget_ilist_selected_label_get(cfdata->gui.widget_list);

   eina_stringshare_del(e_config->xsettings.net_icon_theme_name);
   e_config->xsettings.net_theme_name = eina_stringshare_ref(cfdata->widget_theme);

//   e_config->xsettings.match_e17_icon_theme = cfdata->match_e17_icon_theme;
   e_config->xsettings.match_e17_theme = cfdata->match_e17_theme;
   e_config->xsettings.enabled = cfdata->enable_xsettings;

   eina_stringshare_del(e_config->icon_theme);
   if (cfdata->icon_overrides || cfdata->match_e17_icon_theme)
     e_config->icon_theme = eina_stringshare_ref(cfdata->icon_theme);
   else
     e_config->icon_theme = NULL;

   e_config->icon_theme_overrides = !!cfdata->icon_overrides;
   e_config->xsettings.match_e17_icon_theme = cfdata->match_e17_icon_theme;
   e_config_save_queue();

   e_util_env_set("E_ICON_THEME", e_config->icon_theme);

   ev = E_NEW(E_Event_Config_Icon_Theme, 1);
   if (ev)
     {
        ev->icon_theme = e_config->icon_theme;
        ecore_event_add(E_EVENT_CONFIG_ICON_THEME, ev, NULL, NULL);
     }

   e_xsettings_config_update();

   return 1;
}

static int
_sort_widget_themes(const void *data1, const void *data2)
{
   const char *d1, *d2;

   d1 = data1;
   d2 = data2;
   if (!d1) return 1;
   if (!d2) return -1;

   return strcmp(d1, d2);
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

   return strcmp(m1->name.name, m2->name.name);
}

static void
_ilist_files_add(E_Config_Dialog_Data *cfdata, const char *dir)
{
   Eina_Iterator *it;
   const char *file;

   it = eina_file_ls(dir);
   if (!it) return;

   EINA_ITERATOR_FOREACH(it, file)
     {
        if (ecore_file_is_dir(file))
          {
             if (!eina_list_data_find(cfdata->widget_themes, file))
               {
                  cfdata->widget_themes = eina_list_append(cfdata->widget_themes, file);
                  continue;
               }
          }

        eina_stringshare_del(file);
     }

   eina_iterator_free(it);
}

static Eina_Bool
_fill_files_ilist(void *data)
{
   Evas *evas;
   Evas_Object *o;
   char theme_dir[4096];
   E_Config_Dialog_Data *cfdata = data;
   Eina_List *xdg_dirs, *l;
   const char *dir;

   if (!(o = cfdata->gui.widget_list))
     return ECORE_CALLBACK_CANCEL;

   e_user_homedir_concat_static(theme_dir, ".themes");
   _ilist_files_add(cfdata, theme_dir);

   xdg_dirs = efreet_data_dirs_get();
   EINA_LIST_FOREACH(xdg_dirs, l, dir)
     {
        snprintf(theme_dir, sizeof(theme_dir), "%s/themes", dir);
        _ilist_files_add(cfdata, theme_dir);
     }

   evas = evas_object_evas_get(o);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   e_widget_ilist_clear(o);

   if (cfdata->widget_themes)
     {
        const char *theme;
        int cnt = 0;

        cfdata->widget_themes = eina_list_sort(cfdata->widget_themes, -1, _sort_widget_themes);

        EINA_LIST_FREE(cfdata->widget_themes, theme)
          {
             char *tmp = strdup(strrchr(theme, '/') + 1);
             const char *label;

             if (tmp)
               {
                  /* label pointer will exist as long as ilist item
                     so val remains valid */
                  label = eina_stringshare_add(tmp);
                  e_widget_ilist_append(o, NULL, label, NULL, NULL, label /* val */);
                  if ((e_config->xsettings.net_theme_name_detected == label) || (cfdata->widget_theme == label))
                    e_widget_ilist_selected_set(cfdata->gui.widget_list, cnt);
                  eina_stringshare_del(label);
                  free(tmp);
                  cnt++;
               }

             eina_stringshare_del(theme);
          }
     }

   e_widget_ilist_go(o);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);

   return ECORE_CALLBACK_CANCEL;
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

static void
_populate_icon_preview(E_Config_Dialog_Data *cfdata)
{
   const char *t = cfdata->icon_theme;
   unsigned int i;

   for (i = 0; i < sizeof(_icon_previews) / sizeof(_icon_previews[0]); i++)
     {
        const char *path;

        if (!(path = efreet_icon_path_find(t, _icon_previews[i], PREVIEW_SIZE)))
          continue;
        if (e_icon_file_set(cfdata->gui.icon_preview[i], path))
          e_icon_fill_inside_set(cfdata->gui.icon_preview[i], EINA_TRUE);
     }
}

struct _fill_icon_themes_data
{
   Eina_List            *l;
   int                   i;
   Evas                 *evas;
   E_Config_Dialog_Data *cfdata;
   Eina_Bool             themes_loaded;
};

static void
_fill_icon_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->icon_themes = efreet_icon_theme_list_get();
   cfdata->icon_themes = eina_list_sort(cfdata->icon_themes,
                                        eina_list_count(cfdata->icon_themes),
                                        _sort_icon_themes);
}

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
        _fill_icon_data(d->cfdata);
        d->l = d->cfdata->icon_themes;
        d->i = 0;
        d->themes_loaded = 1;
        return ECORE_CALLBACK_RENEW;
     }

   if (!d->l)
     {
        int mw, mh;

        e_widget_ilist_go(d->cfdata->gui.icon_list);

        e_widget_size_min_get(d->cfdata->gui.icon_list, &mw, &mh);
        e_widget_size_min_set(d->cfdata->gui.icon_list, mw, 100);

        d->cfdata->fill_icon_themes_delayed = NULL;
        d->cfdata->icon_populating = EINA_FALSE;
        _populate_icon_preview(d->cfdata);
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
        e_widget_ilist_append(d->cfdata->gui.icon_list, oc, theme->name.name,
                              NULL, NULL, theme->name.internal);
        if ((d->cfdata->icon_theme) && (theme->name.internal) &&
            (strcmp(d->cfdata->icon_theme, theme->name.internal) == 0))
          e_widget_ilist_selected_set(d->cfdata->gui.icon_list, d->i);
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
   if (cfdata->icon_populating) return;
   _populate_icon_preview(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ilist, *of, *ow;
   struct _fill_icon_themes_data *d;
   unsigned int i;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->widget_theme));
   cfdata->gui.widget_list = ilist;
   e_widget_size_min_set(cfdata->gui.widget_list, 100, 100);

   /* e_widget_on_change_hook_set(ilist, _icon_theme_changed, cfdata); */
   e_widget_list_object_append(ol, ilist, 1, 1, 0.5);

   /* ow = e_widget_check_add(evas, _("Use icon theme for applications"),
    *                        &(cfdata->match_e17_icon_theme));
    * e_widget_list_object_append(ol, ow, 0, 0, 0.0); */

   ow = e_widget_check_add(evas, _("Match Enlightenment theme if possible"),
                           &(cfdata->match_e17_theme));
   e_widget_list_object_append(ol, ow, 0, 0, 0.0);

   /* ow = e_widget_check_add(evas, _("Set application theme"),
    *                               &(cfdata->enable_app_theme));
    * e_widget_list_object_append(o, ow, 0, 0, 0.0); */

   // >> advanced
   ow = e_widget_check_add(evas, _("Enable Settings Daemon"),
                           &(cfdata->enable_xsettings));
   /* e_widget_on_change_hook_set(ow, _settings_changed, cfdata); */
   e_widget_list_object_append(ol, ow, 0, 0, 0.0);
   e_widget_toolbook_page_append(otb, NULL, _("Applications"), ol, 
                                 1, 1, 1, 1, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->icon_theme));
   cfdata->gui.icon_list = ilist;
   e_widget_size_min_set(cfdata->gui.icon_list, 100, 100);

   cfdata->icon_populating = EINA_TRUE;
   e_widget_on_change_hook_set(ilist, _icon_theme_changed, cfdata);
   e_widget_list_object_append(ol, ilist, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Preview"), 1);

   for (i = 0; i < sizeof(_icon_previews) / sizeof(_icon_previews[0]); i++)
     {
        cfdata->gui.icon_preview[i] = e_icon_add(evas);
        e_icon_preload_set(cfdata->gui.icon_preview[i], EINA_TRUE);
        e_icon_scale_size_set(cfdata->gui.icon_preview[i], PREVIEW_SIZE);
        e_widget_framelist_object_append_full(of, cfdata->gui.icon_preview[i],
                                              0, 0, 0, 0, 0.5, 0.5,
                                              PREVIEW_SIZE, PREVIEW_SIZE,
                                              PREVIEW_SIZE, PREVIEW_SIZE);
     }
   e_widget_list_object_append(ol, of, 0, 0, 0.5);

   /* ow = e_widget_check_add(evas, _("Enable icon theme"),
    *                         &(cfdata->enable_icon_theme));
    * e_widget_on_change_hook_set(ow, _icon_theme_changed, cfdata);
    * e_widget_list_object_append(ol, ow, 0, 0, 0.0); */

   ow = e_widget_check_add(evas, _("Enable icon theme for applications"),
                           &(cfdata->match_e17_icon_theme));
   e_widget_list_object_append(ol, ow, 0, 0, 0.0);

   ow = e_widget_check_add(evas, _("Enable icon theme for Enlightenment"),
                           &(cfdata->icon_overrides));
   e_widget_list_object_append(ol, ow, 0, 0, 0.0);

   e_widget_toolbook_page_append(otb, NULL, _("Icons"), ol, 
                                 1, 1, 1, 1, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   _fill_files_ilist(cfdata);

   if (cfdata->fill_icon_themes_delayed)
     free(ecore_idler_del(cfdata->fill_icon_themes_delayed));

   d = malloc(sizeof(*d));
   d->l = NULL;
   d->cfdata = cfdata;
   d->themes_loaded = 0;
   d->evas = evas;
   cfdata->fill_icon_themes_delayed = ecore_idler_add(_fill_icon_themes, d);

   return otb;
}


#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _sort_widget_themes(const void *data1, const void *data2);
static Evas_Object *_icon_new(Evas *evas, const char *theme, const char *icon, unsigned int size);

struct _E_Config_Dialog_Data
{
  E_Config_Dialog *cfd;
  Eina_List *widget_themes;
  const char *widget_theme;
  int enable_xsettings;
  int match_e17_theme;
  int match_e17_icon_theme;
  struct
  {
    Evas_Object *list;
  } gui;

  /* Ecore_Idler *fill_widget_themes_delayed; */
};

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
                             "preferences-dekstop-theme", 0, v, NULL);
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
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   /* if (cfdata->fill_widget_themes_delayed)
    *   free(ecore_idler_del(cfdata->fill_widget_themes_delayed)); */

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

   if ((!cfdata->widget_theme) && (!e_config->xsettings.net_theme_name))
     return 0;

   if ((!cfdata->widget_theme) || (!e_config->xsettings.net_theme_name))
     return 1;

   return strcmp(cfdata->widget_theme, e_config->xsettings.net_theme_name) != 0;

   return 1;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Event_Config_Icon_Theme *ev;

   if (!_basic_check_changed(cfd, cfdata)) return 1;
   printf("is    %s \n", e_config->xsettings.net_icon_theme_name);
   printf("chose %s \n", cfdata->widget_theme);
   printf("...   %s \n", e_widget_ilist_selected_label_get(cfdata->gui.list));

   e_widget_ilist_selected_label_get(cfdata->gui.list);

   eina_stringshare_del(e_config->xsettings.net_icon_theme_name);
   e_config->xsettings.net_theme_name =
     eina_stringshare_ref(e_widget_ilist_selected_label_get(cfdata->gui.list));

   e_config->xsettings.match_e17_icon_theme = cfdata->match_e17_icon_theme;
   e_config->xsettings.match_e17_theme = cfdata->match_e17_theme;
   e_config->xsettings.enabled = cfdata->enable_xsettings;

   e_config_save_queue();

   e_xsettings_config_update();

   return 1;
}

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

   /* cfdata->fill_widget_themes_delayed = NULL; */

   if (!(o = cfdata->gui.list))
     return ECORE_CALLBACK_CANCEL;

   evas = evas_object_evas_get(o);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   e_widget_ilist_clear(o);

   e_user_homedir_concat_static(theme_dir, ".themes");
   _ilist_files_add(cfdata, theme_dir);

   xdg_dirs = efreet_data_dirs_get();
   EINA_LIST_FOREACH(xdg_dirs, l, dir)
     {
        snprintf(theme_dir, sizeof(theme_dir), "%s/themes", dir);
        _ilist_files_add(cfdata, theme_dir);
     }

   if (cfdata->widget_themes)
     {
        const char *theme;
        int cnt = 0;

        cfdata->widget_themes = eina_list_sort(cfdata->widget_themes, -1, _cb_sort);

        EINA_LIST_FREE(cfdata->widget_themes, theme)
          {
             char *tmp = strdup(strrchr(theme, '/') + 1);

             if (tmp)
               {
                  e_widget_ilist_append(o, NULL, tmp, NULL, NULL, NULL);
                  free(tmp);
                  if (cfdata->widget_theme && !strcmp(cfdata->widget_theme, tmp))
                    e_widget_ilist_selected_set(cfdata->gui.list, cnt);

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
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ilist, *ol, *ow;
   unsigned int i;

   o = e_widget_list_add(evas, 0, 0);
   ilist = e_widget_ilist_add(evas, 24, 24, &(cfdata->widget_theme));
   cfdata->gui.list = ilist;
   e_widget_size_min_set(cfdata->gui.list, 100, 100);

   /* e_widget_on_change_hook_set(ilist, _icon_theme_changed, cfdata); */
   e_widget_list_object_append(o, ilist, 1, 1, 0.5);

   ow = e_widget_check_add(evas, _("Use E17 icon theme for applications"),
                                 &(cfdata->match_e17_icon_theme));
   e_widget_list_object_append(o, ow, 0, 0, 0.0);

   ow = e_widget_check_add(evas, _("Match E17 theme if possible"),
                           &(cfdata->match_e17_theme));
   e_widget_list_object_append(o, ow, 0, 0, 0.0);

   /* ow = e_widget_check_add(evas, _("Set application theme"),
    *                               &(cfdata->enable_app_theme));
    * e_widget_list_object_append(o, ow, 0, 0, 0.0); */

   // >> advanced
   ow = e_widget_check_add(evas, _("Enable Settings Daemon"),
                           &(cfdata->enable_xsettings));
   /* e_widget_on_change_hook_set(ow, _settings_changed, cfdata); */
   e_widget_list_object_append(o, ow, 0, 0, 0.0);


   e_dialog_resizable_set(cfd->dia, 1);

   /* if (cfdata->fill_widget_themes_delayed)
    *   free(ecore_idler_del(cfdata->fill_widget_themes_delayed)); */
   /* cfdata->fill_widget_themes_delayed = ecore_idler_add(_fill_files_ilist, cfdata); */

   _fill_files_ilist(cfdata);

   return o;
}

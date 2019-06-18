#include "e.h"
#include "e_mod_main.h"

#define IMPORT_STRETCH          0
#define IMPORT_TILE             1
#define IMPORT_CENTER           2
#define IMPORT_SCALE_ASPECT_IN  3
#define IMPORT_SCALE_ASPECT_OUT 4
#define IMPORT_PAN              5
#define IMPORT_COLOR            6
typedef struct _Import Import;

struct _Import
{
  const char *file;
  int method;
  int external;
  int quality;
  E_Color *color;

  Ecore_Exe *exe;
  Ecore_Event_Handler *exe_handler;
  Ecore_End_Cb ok;
  char *tmpf;
  char *fdest;
  void *data;
};


static void            *_create_data(E_Config_Dialog *cfd);
static void             _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void             _fill_data(E_Config_Dialog_Data *cfdata);
static int              _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object     *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int              _adv_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object     *_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static E_Config_Dialog *_e_int_config_wallpaper_desk(E_Container *con, int con_num, int zone_num, int desk_x, int desk_y);

static void             _cb_button_up(void *data1, void *data2);
static void             _cb_files_changed(void *data, Evas_Object *obj, void *event_info);
static void             _cb_files_selection_change(void *data, Evas_Object *obj, void *event_info);
static void             _cb_files_files_changed(void *data, Evas_Object *obj, void *event_info);
static void             _cb_files_files_deleted(void *data, Evas_Object *obj, void *event_info);
static void             _cb_theme_wallpaper(void *data, Evas_Object *obj, void *event_info);
static void             _cb_dir(void *data, Evas_Object *obj, void *event_info);
static void             _cb_import(void *data1, void *data2);
static void             _cb_color(void *data1, void *data2 __UNUSED__);
static void             _cb_color_ok(void *data, void *dia __UNUSED__);
static void             _cb_color_select(E_Color_Dialog *dia __UNUSED__, E_Color *color, void *data);
static void             _cb_color_del(void *data);
static void             _cb_color_cancel(E_Color_Dialog *dia __UNUSED__, E_Color *color __UNUSED__, void *data);
static const char*      _edj_gen(Import *import);
static Eina_Bool        _cb_edje_cc_exit(void *data, int type __UNUSED__, void *event);
static Eina_Bool         _image_size(const char *path, int *w, int *h);
static void             _import_free(Import *import);
 
#define E_CONFIG_WALLPAPER_ALL    0
#define E_CONFIG_WALLPAPER_DESK   1
#define E_CONFIG_WALLPAPER_SCREEN 2

struct _E_Config_Wallpaper
{
   int specific_config;
   int con_num, zone_num;
   int desk_x, desk_y;
};

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas_Object     *o_fm;
   Evas_Object     *o_up_button;
   Evas_Object     *o_preview;
   Evas_Object     *o_theme_bg;
   Evas_Object     *o_personal;
   Evas_Object     *o_system;
   int              fmdir, use_theme_bg;
   const char      *bg;

   /* advanced */
   int              all_this_desk_screen;

   /* dialogs */
   E_Import_Dialog *win_import;
   E_Import_Config_Dialog *import;
   E_Color_Dialog *win_color;
};



E_Config_Dialog *
e_int_config_wallpaper(E_Container *con, const char *params __UNUSED__)
{
   return _e_int_config_wallpaper_desk(con, -1, -1, -1, -1);
}

E_Config_Dialog *
e_int_config_wallpaper_desk(E_Container *con, const char *params)
{
   int con_num, zone_num, desk_x, desk_y;

   if (!params) return NULL;
   con_num = zone_num = desk_x = desk_y = -1;
   if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &desk_x, &desk_y) != 4)
     return NULL;
   return _e_int_config_wallpaper_desk(con, con_num, zone_num, desk_x, desk_y);
}

static E_Config_Dialog *
_e_int_config_wallpaper_desk(E_Container *con, int con_num, int zone_num, int desk_x, int desk_y)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Config_Wallpaper *cw;

   if (e_config_dialog_find("E", "appearance/wallpaper")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   cw = E_NEW(E_Config_Wallpaper, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;

   if (!(con_num == -1 && zone_num == -1 && desk_x == -1 && desk_y == -1))
     cw->specific_config = 1;
   else
     {
        v->advanced.apply_cfdata = _adv_apply;
        v->advanced.create_widgets = _adv_create;
     }

   v->override_auto_apply = 1;

   cw->con_num = con_num;
   cw->zone_num = zone_num;
   cw->desk_x = desk_x;
   cw->desk_y = desk_y;

   cfd = e_config_dialog_new(con, _("Wallpaper Settings"), "E",
                             "appearance/wallpaper",
                             "preferences-desktop-wallpaper", 0, v, cw);
   return cfd;
}

static void
_bg_set(E_Config_Dialog_Data *cfdata)
{
   if (!cfdata->o_preview) return;
   if (cfdata->bg)
     {
        if (eina_str_has_extension(cfdata->bg, ".edj"))
          e_widget_preview_edje_set(cfdata->o_preview, cfdata->bg,
                                    "e/desktop/background");
        else
          e_widget_preview_file_set(cfdata->o_preview, cfdata->bg,
                                    NULL);
     }
   else
     {
        const char *f;

        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        e_widget_preview_edje_set(cfdata->o_preview, f,
                                  "e/desktop/background");
     }
}

static void
_cb_button_up(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->o_fm) e_widget_flist_parent_go(cfdata->o_fm);
}

static void
_cb_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   if (!cfdata->o_fm) return;
   if (cfdata->o_up_button)
     e_widget_disabled_set(cfdata->o_up_button,
                           !e_widget_flist_has_parent_get(cfdata->o_fm));
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *real_path;
   char buf[PATH_MAX];

   cfdata = data;
   if (!cfdata->o_fm) return;
   if (!(selected = e_widget_flist_selected_list_get(cfdata->o_fm))) return;
   ici = selected->data;
   real_path = e_widget_flist_real_path_get(cfdata->o_fm);
   if (!strcmp(real_path, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", real_path, ici->file);
   eina_list_free(selected);
   if (ecore_file_is_dir(buf)) return;

   eina_stringshare_replace(&cfdata->bg, buf);
   _bg_set(cfdata);
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   e_widget_change(cfdata->o_fm);
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *p = NULL;
   char buf[PATH_MAX];
   size_t len;

   cfdata = data;
   if ((!cfdata->bg) || (!cfdata->o_fm)) return;
   p = e_widget_flist_real_path_get(cfdata->o_fm);
   if (p)
     {
        if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   else return;

   len = e_user_dir_concat_static(buf, "backgrounds");
   if (!strncmp(cfdata->bg, buf, len))
     p = cfdata->bg + len + 1;
   else
     {
        len = e_prefix_data_concat_static(buf, "data/backgrounds");
        if (!strncmp(cfdata->bg, buf, len))
          p = cfdata->bg + len + 1;
        else
          p = cfdata->bg;
     }

   e_widget_flist_select_set(cfdata->o_fm, p, 1);
   e_widget_flist_file_show(cfdata->o_fm, p);
}

static void
_cb_files_files_deleted(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   cfdata = data;
   if ((!cfdata->bg) || (!cfdata->o_fm)) return;

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
_cb_theme_wallpaper(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *f;

   cfdata = data;
   if (cfdata->use_theme_bg)
     {
        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        eina_stringshare_replace(&cfdata->bg, f);
        _bg_set(cfdata);
     }
   else
     {
        evas_object_smart_callback_call(cfdata->o_fm, "selection_change",
                                        cfdata);
        _bg_set(cfdata);
     }
}

static void
_cb_dir(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];

   cfdata = data;
   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");
   e_widget_flist_path_set(cfdata->o_fm, path, "/");
}

static void
_cb_import_ok(const char *path, void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = e_object_data_get(data);
   cfdata->fmdir = 1;
   e_widget_radio_toggle_set(cfdata->o_personal, cfdata->fmdir);
   e_widget_change(cfdata->o_personal);
   eina_stringshare_replace(&cfdata->bg, path);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, cfdata->use_theme_bg);
   _bg_set(cfdata);
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

static void
_cb_import_del(void *data)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = e_object_data_get(data);
   cfdata->win_import = NULL;
}

static void
_cb_import(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->win_import)
     {
        e_win_raise(cfdata->win_import->dia->win);
        return;
     }
   cfdata->win_import = e_import_dialog_show(cfdata->cfd->dia->win->container, NULL, NULL, (Ecore_End_Cb)_cb_import_ok, NULL);
   e_object_data_set(E_OBJECT(cfdata->win_import), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->win_import), _cb_import_del);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Wallpaper *cw;
   const E_Config_Desktop_Background *cfbg;
   char path[PATH_MAX];

   cw = cfdata->cfd->data;
   if (cw->specific_config)
     {
        /* specific config passed in. set for that only */
        cfdata->bg = e_bg_file_get(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y);
     }
   else
     {
        /* get current desk. advanced mode allows selecting all, screen or desk */
        E_Container *c;
        E_Zone *z;
        E_Desk *d;

        c = e_container_current_get(e_manager_current_get());
        z = e_zone_current_get(c);
        d = e_desk_current_get(z);

        cfbg = e_bg_config_get(c->num, z->num, d->x, d->y);
        /* if we have a config for this bg, use it. */
        if (cfbg)
          {
             if (cfbg->container >= 0 && cfbg->zone >= 0)
               {
                  if (cfbg->desk_x >= 0 && cfbg->desk_y >= 0)
                    cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_DESK;
                  else
                    cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_SCREEN;
               }
             eina_stringshare_replace(&cfdata->bg, cfbg->file);
          }
     }

   if ((!cfdata->bg) && (e_config->desktop_default_background))
     cfdata->bg = eina_stringshare_add(e_config->desktop_default_background);

   if (cfdata->bg)
     {
        const char *f;
        size_t len;

        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        if (!strcmp(cfdata->bg, f)) cfdata->use_theme_bg = 1;
        len = e_prefix_data_concat_static(path, "data/backgrounds");
        if (!strncmp(cfdata->bg, path, len)) cfdata->fmdir = 1;
     }
   else
     cfdata->use_theme_bg = 1;
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
   if (cfdata->win_import) e_object_del(E_OBJECT(cfdata->win_import));
   if (cfdata->win_color) e_object_del(E_OBJECT(cfdata->win_color));
   eina_stringshare_del(cfdata->bg);
   E_FREE(cfd->data);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *rt, *ot, *oa;
   Evas_Object *ow;
   E_Zone *zone = NULL;
   E_Radio_Group *rg;
   char path[PATH_MAX];
   int mw, mh;

   e_dialog_resizable_set(cfd->dia, 1);

   zone = e_zone_current_get(cfd->con);
   o = e_widget_list_add(evas, 0, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ot = e_widget_table_add(evas, 0);
   rt = e_widget_table_add(evas, 1);

   /* create dir radios */
   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(ot, rt, 0, 0, 1, 1, 0, 0, 0, 0);

   ow = e_widget_button_add(evas, _("Go up a directory"), "go-up",
                            _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = ow;
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_widget_flist_add(evas);
   {
      E_Fm2_Config *cfg;
      cfg = e_widget_flist_config_get(ow);
      cfg->view.no_click_rename = 1;
   }
   cfdata->o_fm = ow;
   evas_object_smart_callback_add(ow, "dir_changed",
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "selection_change",
                                  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(ow, "changed",
                                  _cb_files_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "files_deleted",
                                  _cb_files_files_deleted, cfdata);
   e_widget_flist_path_set(ow, path, "/");

   e_widget_size_min_set(ow, 160, 160);
   e_widget_table_object_append(ot, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.0);

   ot = e_widget_table_add(evas, 0);
   ow = e_widget_check_add(evas, _("Use Theme Wallpaper"),
                           &cfdata->use_theme_bg);
   cfdata->o_theme_bg = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_theme_wallpaper, cfdata);
   e_widget_table_object_append(ot, ow, 0, 0, 2, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Picture..."), "folder-image",
                            _cb_import, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 0, 0);

   ow = e_widget_button_add(evas, _("Color..."), NULL,
                            _cb_color, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 1, 1, 1, 1, 1, 0, 0, 0);


   mw = 320;
   mh = (320 * zone->h) / zone->w;
   oa = e_widget_aspect_add(evas, mw, mh);
   ow = e_widget_preview_add(evas, mw, mh);
   evas_object_size_hint_min_set(ow, mw, mh);
   cfdata->o_preview = ow;
   _bg_set(cfdata);
   e_widget_aspect_child_set(oa, ow);
   evas_object_show(ow);
   e_widget_table_object_append(ot, oa, 0, 2, 2, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   return o;
}

static void
_apply_import_ok(const char *file, E_Import_Config_Dialog *import)
{
   E_Config_Dialog *cfd;

   cfd = e_object_data_get(E_OBJECT(import));
   eina_stringshare_replace(&cfd->cfdata->bg, file);
   if (cfd->view_type == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     _basic_apply(cfd, cfd->cfdata);
   else
     _adv_apply(cfd, cfd->cfdata);
}

static void
_apply_import_del(void *import)
{
   E_Config_Dialog *cfd;

   cfd = e_object_data_get(import);
   cfd->cfdata->import = NULL;
   e_object_delfn_clear(E_OBJECT(cfd)); // get rid of idler delete function
   e_object_unref(E_OBJECT(cfd));
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Wallpaper *cw;

   if (!cfdata->bg) return 0;
   cw = cfd->data;
   if (!eina_str_has_extension(cfdata->bg, ".edj"))
     {
        cfdata->import = e_import_config_dialog_show(NULL, cfdata->bg, (Ecore_End_Cb)_apply_import_ok, NULL);
        e_dialog_parent_set(cfdata->import->dia, cfd->dia->win);
        e_object_del_attach_func_set(E_OBJECT(cfdata->import), _apply_import_del);
        e_object_data_set(E_OBJECT(cfdata->import), cfd);
        e_object_ref(E_OBJECT(cfd));
        return 1;
     }
   if (cw->specific_config)
     {
        /* update a specific config */
        e_bg_del(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y);
        e_bg_add(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y, cfdata->bg);
     }
   else
     {
        /* set the default and nuke individual configs */
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;

             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        if ((cfdata->use_theme_bg) || (!cfdata->bg))
          e_bg_default_set(NULL);
        else
          e_bg_default_set(cfdata->bg);

        cfdata->all_this_desk_screen = 0;
     }

   e_bg_update();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *rt, *ot, *oa;
   Evas_Object *ow, *of;
   E_Zone *zone = NULL;
   E_Radio_Group *rg;
   char path[PATH_MAX];
   int mw, mh;

   e_dialog_resizable_set(cfd->dia, 1);

   zone = e_zone_current_get(cfd->con);
   o = e_widget_list_add(evas, 0, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ot = e_widget_table_add(evas, 0);
   rt = e_widget_table_add(evas, 1);

   /* create dir radios */
   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(ot, rt, 0, 0, 1, 1, 0, 0, 0, 0);

   ow = e_widget_button_add(evas, _("Go up a directory"), "go-up",
                            _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = ow;
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_widget_flist_add(evas);
   cfdata->o_fm = ow;
   evas_object_smart_callback_add(ow, "dir_changed",
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "selection_change",
                                  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(ow, "changed",
                                  _cb_files_files_changed, cfdata);
   e_widget_flist_path_set(ow, path, "/");

   e_widget_size_min_set(ow, 160, 160);
   e_widget_table_object_append(ot, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.0);

   ot = e_widget_table_add(evas, 0);
   ow = e_widget_check_add(evas, _("Use Theme Wallpaper"),
                           &cfdata->use_theme_bg);
   cfdata->o_theme_bg = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_theme_wallpaper, cfdata);
   e_widget_table_object_append(ot, ow, 0, 0, 2, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Picture..."), "folder-image",
                            _cb_import, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Color..."), NULL,
                            _cb_color, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 1, 1, 1, 1, 1, 0, 0, 0);

   mw = 320;
   mh = (320 * zone->h) / zone->w;
   oa = e_widget_aspect_add(evas, mw, mh);
   ow = e_widget_preview_add(evas, mw, mh);
   evas_object_size_hint_min_set(ow, mw, mh);
   evas_object_size_hint_aspect_set(ow, EVAS_ASPECT_CONTROL_BOTH, zone->w, zone->h);
   cfdata->o_preview = ow;
   _bg_set(cfdata);
   e_widget_aspect_child_set(oa, ow);
   e_widget_table_object_append(ot, oa, 0, 2, 2, 1, 1, 1, 1, 1);

   rg = e_widget_radio_group_new(&(cfdata->all_this_desk_screen));
   of = e_widget_frametable_add(evas, _("Where to place the Wallpaper"), 0);
   ow = e_widget_radio_add(evas, _("All Desktops"), E_CONFIG_WALLPAPER_ALL, rg);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("This Desktop"), E_CONFIG_WALLPAPER_DESK, rg);
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("This Screen"), E_CONFIG_WALLPAPER_SCREEN, rg);
   if (!(e_util_container_zone_number_get(0, 1) ||
         (e_util_container_zone_number_get(1, 0))))
     e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 0, 3, 2, 1, 1, 0, 1, 0);

   e_widget_list_object_append(o, ot, 1, 1, 0.0);

   return o;
}

static int
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *fl = NULL, *l;
   E_Zone *z;
   E_Desk *d;

   if (!(z = e_zone_current_get(cfdata->cfd->con))) return 0;
   if (!(d = e_desk_current_get(z))) return 0;
   if (cfdata->use_theme_bg)
     {
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;

             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        e_bg_default_set(NULL);
     }
   else
     {
        if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_ALL)
          {
             while (e_config->desktop_backgrounds)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = e_config->desktop_backgrounds->data;
                  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
               }
             e_bg_default_set(cfdata->bg);
          }
        else if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_DESK)
          {
             e_bg_del(z->container->num, z->num, d->x, d->y);
             e_bg_del(z->container->num, -1, d->x, d->y);
             e_bg_del(-1, z->num, d->x, d->y);
             e_bg_del(-1, -1, d->x, d->y);
             e_bg_add(z->container->num, z->num, d->x, d->y, cfdata->bg);
          }
        else if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_SCREEN)
          {
             for (l = e_config->desktop_backgrounds; l; l = l->next)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = l->data;
                  if ((cfbg->container == (int)z->container->num) &&
                      (cfbg->zone == (int)z->num))
                    fl = eina_list_append(fl, cfbg);
               }
             while (fl)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = fl->data;
                  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x,
                           cfbg->desk_y);
                  fl = eina_list_remove_list(fl, fl);
               }
             e_bg_add(z->container->num, z->num, -1, -1, cfdata->bg);
          }
     }
   e_bg_update();
   e_config_save_queue();
   return 1;
}

static void
_cb_color(void *data1, void *data2 __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data1);
   E_Config_Dialog_Data *cfdata = data1;

   if (cfdata->win_color)
     {
        e_win_raise(cfdata->win_color->dia->win);
        return;
     }
   cfdata->win_color = e_color_dialog_new(cfdata->cfd->dia->win->container, NULL, EINA_FALSE);
   e_object_data_set(E_OBJECT(cfdata->win_color), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->win_color), _cb_color_del);
   e_color_dialog_select_callback_set(cfdata->win_color, _cb_color_select, cfdata);
   e_color_dialog_cancel_callback_set(cfdata->win_color, _cb_color_cancel, cfdata);
   e_color_dialog_show(cfdata->win_color);
}

static void
_cb_color_ok(void *data, void *dia __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   E_Config_Dialog_Data *cfdata = ((Import*) data)->data;
   _cb_color_del(cfdata);
   cfdata->fmdir = 1;
   e_widget_radio_toggle_set(cfdata->o_personal, cfdata->fmdir);
   e_widget_change(cfdata->o_personal);
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
   _bg_set(cfdata);
}

static void
_cb_color_select(E_Color_Dialog *dia __UNUSED__, E_Color *color, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   E_Config_Dialog_Data *cfdata = data;

   Import *import = E_NEW(Import, 1);
   import->color  = E_NEW(E_Color,1);

   e_color_copy(color, import->color);
   import->method = IMPORT_COLOR;
   import->ok = _cb_color_ok;
   import->data = (void*) cfdata;
   eina_stringshare_replace(&cfdata->bg, _edj_gen(import));
}
static void
_cb_color_cancel(E_Color_Dialog *dia __UNUSED__, E_Color *color __UNUSED__, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data *) data;
   cfdata->win_color = NULL;
}

/* Code duplicated more or less from _import_edj_gen in 
 *   moksha/src/bin/e_import_config_dialog.c
 * */
static const char *
_edj_gen(Import *import)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(import, NULL);

   Eina_Bool anim = EINA_FALSE;
   int fd, num = 1;
   int w = 0, h = 0;
   const char *file, *locale;
   char buf[PATH_MAX], cmd[PATH_MAX], tmpn[PATH_MAX], ipart[PATH_MAX] = "", enc[128];
   char *imgdir = NULL, *fstrip;
   int cr, cg, cb, ca;
   FILE *f;
   size_t len, off;
   const char *ret;
   
   if (!import->file && import->method != IMPORT_COLOR) return NULL;
   if (import->file)
     {
       file = ecore_file_file_get(import->file);
       fstrip = ecore_file_strip_ext(file);
       if (!fstrip)
         {
            _import_free(import);
            return NULL;
         }
     }
   else
     fstrip = strdup("bg_color");
   len = e_user_dir_snprintf(buf, sizeof(buf), "backgrounds/%s.edj", fstrip);
   if (len >= sizeof(buf))
     {
        free(fstrip);
        _import_free(import);
        return NULL;
     }
   off = len - (sizeof(".edj") - 1);
   for (num = 1; ecore_file_exists(buf) && num < 100; num++)
     snprintf(buf + off, sizeof(buf) - off, "-%d.edj", num);
   free(fstrip);

   cr = import->color->r;
   cg = import->color->g;
   cb = import->color->b;
   ca = import->color->a;

   if (num == 100)
     {
        printf("Couldn't come up with another filename for %s\n", buf);
        _import_free(import);
        return NULL;
     }

   strcpy(tmpn, "/tmp/e_bgdlg_new.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0)
     {
        printf("Error Creating tmp file: %s\n", strerror(errno));
        _import_free(import);
        return NULL;
     }

   f = fdopen(fd, "w");
   if (!f)
     {
        printf("Cannot open %s for writing\n", tmpn);
        _import_free(import);
        return NULL;
     }
   if (import->file)
     {
        anim = eina_str_has_extension(import->file, "gif");
        imgdir = ecore_file_dir_get(import->file);
       if (!imgdir) ipart[0] = '\0';
       else
        {
          snprintf(ipart, sizeof(ipart), "-id %s", e_util_filename_escape(imgdir));
          free(imgdir);
        }

      if (!_image_size(import->file, &w, &h))
       {
          _import_free(import);
          return NULL;
       }


     if (import->external)
       {
          fstrip = strdupa(e_util_filename_escape(import->file));
          snprintf(enc, sizeof(enc), "USER");
       }
     else
       {
          fstrip = strdupa(e_util_filename_escape(file));
          if (import->quality == 100)
            snprintf(enc, sizeof(enc), "COMP");
          else
            snprintf(enc, sizeof(enc), "LOSSY %i", import->quality);
       }
     }
   switch (import->method)
     {
      case IMPORT_STRETCH:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "%s"
                "data { item: \"style\" \"0\"; }\n"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "image { normal: \"%s\"; scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, fstrip);
        break;

      case IMPORT_TILE:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"1\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "image { normal: \"%s\"; }\n"
                "fill { size {\n"
                "relative: 0.0 0.0;\n"
                "offset: %i %i;\n"
                "} } } } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, fstrip, w, h);
        break;

      case IMPORT_CENTER:
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"2\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "color: %i %i %i %i;\n"
                "} }\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "min: %i %i; max: %i %i;\n"
                "image { normal: \"%s\"; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n", w, h, cr, cg, cb, ca, w, h, w, h, fstrip);
        break;

      case IMPORT_SCALE_ASPECT_IN:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"3\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "color: %i %i %i %i;\n"
                "} }\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: BOTH;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, cr, cg, cb, ca, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      case IMPORT_SCALE_ASPECT_OUT:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"4\"; }\n"
                "%s"
                "max: %i %i;\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      case IMPORT_PAN:
        locale = e_intl_language_get();
        setlocale(LC_NUMERIC, "C");
        fprintf(f,
                "images { image: \"%s\" %s; }\n"
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "data { item: \"style\" \"4\"; }\n"
                "%s"
                "max: %i %i;\n"
                "script {\n"
                "public cur_anim; public cur_x; public cur_y; public prev_x;\n"
                "public prev_y; public total_x; public total_y; \n"
                "public pan_bg(val, Float:v) {\n"
                "new Float:x, Float:y, Float:px, Float: py;\n"

                "px = get_float(prev_x); py = get_float(prev_y);\n"
                "if (get_int(total_x) > 1) {\n"
                "x = float(get_int(cur_x)) / (get_int(total_x) - 1);\n"
                "x = px - (px - x) * v;\n"
                "} else { x = 0.0; v = 1.0; }\n"
                "if (get_int(total_y) > 1) {\n"
                "y = float(get_int(cur_y)) / (get_int(total_y) - 1);\n"
                "y = py - (py - y) * v;\n"
                "} else { y = 0.0; v = 1.0; }\n"

                "set_state_val(PART:\"bg\", STATE_ALIGNMENT, x, y);\n"

                "if (v >= 1.0) {\n"
                "set_int(cur_anim, 0); set_float(prev_x, x);\n"
                "set_float(prev_y, y); return 0;\n"
                "}\n"
                "return 1;\n"
                "}\n"
                "public message(Msg_Type:type, id, ...) {\n"
                "if ((type == MSG_FLOAT_SET) && (id == 0)) {\n"
                "new ani;\n"

                "get_state_val(PART:\"bg\", STATE_ALIGNMENT, prev_x, prev_y);\n"
                "set_int(cur_x, round(getfarg(3))); set_int(total_x, round(getfarg(4)));\n"
                "set_int(cur_y, round(getfarg(5))); set_int(total_y, round(getfarg(6)));\n"

                "ani = get_int(cur_anim); if (ani > 0) cancel_anim(ani);\n"
                "ani = anim(getfarg(2), \"pan_bg\", 0); set_int(cur_anim, ani);\n"
                "} } }\n"
                "parts {\n"
                "part { name: \"bg\"; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "aspect: %1.9f %1.9f; aspect_preference: NONE;\n"
                "image { normal: \"%s\";  scale_hint: STATIC; }\n"
                "} } }\n"
                "programs { program {\n"
                " name: \"init\";\n"
                " signal: \"load\";\n"
                " source: \"\";\n"
                " script { custom_state(PART:\"bg\", \"default\", 0.0);\n"
                " set_state(PART:\"bg\", \"custom\", 0.0);\n"
                " set_float(prev_x, 0.0); set_float(prev_y, 0.0);\n"
                "} } } } }\n"
                , fstrip, enc, anim ? "" : "data.item: \"noanimation\" \"1\";\n",
                w, h, (double)w / (double)h, (double)w / (double)h, fstrip);
        setlocale(LC_NUMERIC, locale);
        break;

      case IMPORT_COLOR:
        fprintf(f,
                "collections {\n"
                "group { name: \"e/desktop/background\";\n"
                "parts {\n"
                "part { name: \"col\"; type: RECT; mouse_events: 0;\n"
                "description { state: \"default\" 0.0;\n"
                "color: %i %i %i %i;\n"
                "} }\n } } }\n"
                , cr, cg, cb, ca);
        break;
      default:
        /* shouldn't happen */
        break;
     }

   fclose(f);

   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s",
            ipart, tmpn, e_util_filename_escape(buf));
   import->tmpf = strdup(tmpn);
   import->fdest = strdup(buf);
   if (import->exe_handler) ecore_event_handler_del(import->exe_handler);
   import->exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                             _cb_edje_cc_exit, import);
   import->exe = ecore_exe_run(cmd, import);

   ret = eina_stringshare_add(buf);
   return ret;
}

static Eina_Bool
_cb_edje_cc_exit(void *data, int type __UNUSED__, void *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data,  ECORE_CALLBACK_DONE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, ECORE_CALLBACK_DONE);

   Import *import;
   Ecore_Exe_Event_Del *ev;
   int r = 1;

   ev = event;
   import = data;
   if (ecore_exe_data_get(ev->exe) != import) return ECORE_CALLBACK_PASS_ON;

   if (ev->exit_code != 0)
     {
        e_util_dialog_show(_("Picture Import Error"),
                           _("Moksha was unable to import the picture<br>"
                             "due to conversion errors."));
        r = 0;
     }

   if (r && import->ok) import->ok(import, NULL);
   _import_free(import);

   return ECORE_CALLBACK_DONE;
}

Eina_Bool
_image_size(const char *path, int *w, int *h)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL((path||w||h), EINA_FALSE);

   Eina_Bool ret = EINA_TRUE;
   Ecore_Evas *ee = ecore_evas_buffer_new(100, 100);
   Evas *evas = ecore_evas_get(ee);
   Evas_Object *img;
   int err;

   img = evas_object_image_add(evas);
   evas_object_image_file_set(img, path, NULL);
   err = evas_object_image_load_error_get(img);
   if (err == EVAS_LOAD_ERROR_NONE)
      evas_object_image_size_get(img, w, h);
   else
     {
         //DPICL(("Picture load error: $d", err));
         ret = EINA_FALSE;
      }
   evas_object_del(img);
   return ret;
}

static void
_import_free(Import *import)
{
   EINA_SAFETY_ON_NULL_RETURN(import);
   ecore_event_handler_del(import->exe_handler);
   import->exe_handler = NULL;
   E_FREE(import->color);
   E_FREE(import);
}

static void
_cb_color_del(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   E_Config_Dialog_Data *cfdata = (E_Config_Dialog_Data *) data;
   if (cfdata->win_color)
      e_object_del(E_OBJECT(cfdata->win_color));
   cfdata->win_color = NULL;
}

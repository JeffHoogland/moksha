#include "e.h"
#include "e_mod_main.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_List   *_get_theme_categories_list(void);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Basic */
   Evas_Object     *o_fm;
   Evas_Object     *o_up_button;
   Evas_Object     *o_preview;
   Evas_Object     *o_personal;
   Evas_Object     *o_system;
   int              fmdir;
   const char      *theme;
   Eio_File        *eio[2];
   Eio_File        *init[2];
   Eina_List       *theme_init; /* list of eio ops to load themes */
   Eina_List       *themes; /* eet file refs to work around load locking */
   Eina_Bool        free : 1;

   /* Advanced */
   Evas_Object     *o_categories_ilist;
   Evas_Object     *o_files_ilist;
   int              personal_file_count;
   Eina_List       *personal_file_list;
   Eina_List       *system_file_list;
   Eina_List       *theme_list;
   Eina_List       *parts_list;

   /* Dialog */
   E_Win           *win_import;
};

static const char *parts_list[] =
{
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

static void
_e_int_theme_preview_clear(Evas_Object *preview)
{
   Eina_List *objs = evas_object_data_get(preview, "objects");
   Evas_Object *o;

   e_widget_preview_extern_object_set(preview, NULL);
   EINA_LIST_FREE(objs, o) evas_object_del(o);
   evas_object_data_del(preview, "objects");
}

static Eina_Bool
_e_int_theme_preview_group_set(Evas_Object *preview, const char *file, const char *group)
{
   _e_int_theme_preview_clear(preview);
   return e_widget_preview_edje_set(preview, file, group);
}

static void
_e_int_theme_edje_file_set(Evas_Object *o, const char *file, const char *group)
{
   if (!edje_object_file_set(o, file, group))
     {
        file = e_path_find(path_themes, "default.edj");
        if (file)
          {
             edje_object_file_set(o, file, group);
             eina_stringshare_del(file);
          }
     }
}

static Eina_Bool
_e_int_theme_preview_set(Evas_Object *preview, const char *file)
{
   Evas *e;
   Evas_Coord w = 320, h = 240, mw = 0, mh = 0;
   Eina_List *objs = NULL;
   Evas_Object *o, *po, *po2, *po3;
   
   _e_int_theme_preview_clear(preview);
   e = e_widget_preview_evas_get(preview);
   evas_object_size_hint_min_get(preview, &w, &h);
   w *= 2; h *= 2;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/desktop/background");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/comp/popup");
   evas_object_move(o, (w - (400 * e_scale)) / 2, h - (40 * e_scale));
   evas_object_resize(o, 400 * e_scale, (40 * e_scale));
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,shadow,on", "e");
   edje_object_signal_emit(o, "e,state,visible,on", "e");
   objs = eina_list_append(objs, o);
   po = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/shelf/default/base");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,orientation,bottom", "e");
   edje_object_part_swallow(po, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   po = o;
   po2 = po;
   
   o = e_box_add(e);
   e_box_orientation_set(o, 1);
   evas_object_show(o);
   edje_object_part_swallow(po, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   po = o;
   
   mh = 42 * e_scale;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/start/main");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/shelf/default/inset");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, 4 * mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);
   po2 = o;

   o = e_box_add(e);
   e_box_orientation_set(o, 1);
   evas_object_show(o);
   edje_object_part_swallow(po2, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   po3 = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/pager/desk");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,selected", "e");
   e_box_pack_end(po3, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/pager/desk");
   evas_object_show(o);
   e_box_pack_end(po3, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/pager/desk");
   evas_object_show(o);
   e_box_pack_end(po3, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/pager/desk");
   evas_object_show(o);
   e_box_pack_end(po3, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/backlight/main");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/mixer/main");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);

   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/battery/main");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/modules/clock/main");
   evas_object_show(o);
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mh, 0, 9999, 9999);
   objs = eina_list_append(objs, o);
   
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/comp/default");
   evas_object_move(o, w / 2, h / 9);
   evas_object_resize(o, w / 3, h / 3);
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,shadow,on", "e");
   edje_object_signal_emit(o, "e,state,visible,on", "e");
   edje_object_signal_emit(o, "e,state,focus,off", "e");
   objs = eina_list_append(objs, o);
   po = o;
   po2 = po;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/border/default/border");
   edje_object_part_text_set(o, "e.text.title", "Title");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,unfocused", "e");
   edje_object_part_swallow(po, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   po = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/theme/about");
   edje_object_size_min_get(o, &mw, &mh);
   if (mw > 0) evas_object_resize(po2, mw, mh);
   edje_object_part_text_set(o, "e.text.label", "Close");
   edje_object_part_text_set(o, "e.text.theme", "Select Theme");
   evas_object_show(o);
   edje_object_part_swallow(po, "e.swallow.client", o);
   objs = eina_list_append(objs, o);

   
   
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/comp/default");
   evas_object_move(o, w / 10, h / 5);
   evas_object_resize(o, w / 2, h / 3);
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,shadow,on", "e");
   edje_object_signal_emit(o, "e,state,visible,on", "e");
   edje_object_signal_emit(o, "e,state,focus,on", "e");
   objs = eina_list_append(objs, o);
   po = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/border/default/border");
   edje_object_part_text_set(o, "e.text.title", "Title");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,focused", "e");
   edje_object_part_swallow(po, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   po = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/dialog/main");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,icon,enabled", "e");
   edje_object_part_swallow(po, "e.swallow.client", o);
   objs = eina_list_append(objs, o);
   po = o;
   po2 = po;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/dialog/text");
   edje_object_part_text_set(o, "e.textblock.message", 
                             "<hilight>Welcome to enlightenment.</hilight><br>"
                             "<br>"
                             "This is a sample set of content for a<br>"
                             "theme to test to see what it looks like.");
   evas_object_show(o);
   edje_object_part_swallow(po, "e.swallow.content", o);
   objs = eina_list_append(objs, o);
   
   o = e_icon_add(e);
   e_util_icon_theme_set(o, "dialog-warning");
   evas_object_show(o);
   evas_object_size_hint_min_set(o, 64 * e_scale, 64 * e_scale);
   edje_object_part_swallow(po, "e.swallow.icon", o);
   objs = eina_list_append(objs, o);

   o = e_box_add(e);
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   evas_object_show(o);
   edje_object_part_swallow(po, "e.swallow.buttons", o);
   objs = eina_list_append(objs, o);
   po = o;
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/button");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,text", "e");
   edje_object_part_text_set(o, "e.text.label", "OK");
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, 50, 20, 9999, 9999);
   objs = eina_list_append(objs, o);
   
   o = edje_object_add(e);
   _e_int_theme_edje_file_set(o, file, "e/widgets/button");
   evas_object_show(o);
   edje_object_signal_emit(o, "e,state,text", "e");
   edje_object_part_text_set(o, "e.text.label", "Cancel");
   e_box_pack_end(po, o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, 50, 20, 9999, 9999);
   objs = eina_list_append(objs, o);

   e_box_size_min_get(po, &mw, &mh);
   evas_object_size_hint_min_set(po, mw, mh);
   edje_object_part_swallow(po2, "e.swallow.buttons", po);

   evas_object_data_set(preview, "objects", objs);
   
//   e_widget_preview_edje_set(preview, file, "e/desktop/background");
   return EINA_TRUE;
}

E_Config_Dialog *
e_int_config_theme(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/theme")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->override_auto_apply = 1;
   cfd = e_config_dialog_new(con,
                             _("Theme Selector"),
                             "E", "appearance/theme",
                             "preferences-desktop-theme", 0, v, NULL);
   return cfd;
}

void
e_int_config_theme_import_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->win_import = NULL;
}

void
e_int_config_theme_update(E_Config_Dialog *dia, char *file)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];

   cfdata = dia->cfdata;

   cfdata->fmdir = 1;
   e_widget_radio_toggle_set(cfdata->o_personal, 1);

   e_user_dir_concat_static(path, "themes");
   eina_stringshare_del(cfdata->theme);
   cfdata->theme = eina_stringshare_add(file);

   if (cfdata->o_fm)
     e_widget_flist_path_set(cfdata->o_fm, path, "/");

   if (cfdata->o_preview)
     _e_int_theme_preview_set(cfdata->o_preview, cfdata->theme);
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

static Eina_Bool
_eio_filter_cb(void *data __UNUSED__, Eio_File *handler __UNUSED__, const char *file)
{
   return eina_str_has_extension(file, ".edj");
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

   cfdata = data;
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
   char buf[4096];

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

   eina_stringshare_del(cfdata->theme);
   cfdata->theme = eina_stringshare_add(buf);
   if (cfdata->o_preview)
     _e_int_theme_preview_set(cfdata->o_preview, buf);
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

#if 0
/* FIXME unused */
static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
}

#endif

static void
_cb_files_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *p;
   char buf[4096];
   size_t len;

   cfdata = data;
   if ((!cfdata->theme) || (!cfdata->o_fm)) return;

   p = e_widget_flist_real_path_get(cfdata->o_fm);
   if (p)
     {
        if (strncmp(p, cfdata->theme, strlen(p))) return;
     }
   if (!p) return;

   len = e_user_dir_concat_static(buf, "themes");
   if (!strncmp(cfdata->theme, buf, len))
     p = cfdata->theme + len + 1;
   else
     {
        len = e_prefix_data_concat_static(buf, "data/themes");
        if (!strncmp(cfdata->theme, buf, len))
          p = cfdata->theme + len + 1;
        else
          p = cfdata->theme;
     }
   e_widget_flist_select_set(cfdata->o_fm, p, 1);
   e_widget_flist_file_show(cfdata->o_fm, p);
}

static void
_cb_dir(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];

   cfdata = data;
   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/themes");
   else
     e_user_dir_concat_static(path, "themes");
   e_widget_flist_path_set(cfdata->o_fm, path, "/");
}

static void
_cb_files_files_deleted(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
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
_cb_import(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->win_import)
     e_win_raise(cfdata->win_import);
   else
     cfdata->win_import = e_int_config_theme_import(cfdata->cfd);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme *c;
   char path[4096];
   size_t len;

   c = e_theme_config_get("theme");
   if (c)
     cfdata->theme = eina_stringshare_ref(c->file);
   else
     {
        e_prefix_data_concat_static(path, "data/themes/default.edj");
        cfdata->theme = eina_stringshare_add(path);
     }
   if (cfdata->theme[0] != '/')
     {
        e_user_dir_snprintf(path, sizeof(path), "themes/%s", cfdata->theme);
        if (ecore_file_exists(path))
          eina_stringshare_replace(&cfdata->theme, path);
        else
          {
             e_prefix_data_snprintf(path, sizeof(path), "data/themes/%s",
                                    cfdata->theme);
             if (ecore_file_exists(path))
               eina_stringshare_replace(&cfdata->theme, path);
          }
     }

   cfdata->theme_list = _get_theme_categories_list();

   len = e_prefix_data_concat_static(path, "data/themes");
   if (!strncmp(cfdata->theme, path, len))
     cfdata->fmdir = 1;
}

static void
_open_test_cb(void *file)
{
   if (!edje_file_group_exists(eet_file_get(file), "e/desktop/background"))
     e_util_dialog_show(_("Theme File Error"),
                        _("%s is probably not an E17 theme!"),
                        eet_file_get(file));
}

static void
_open_done_cb(void *data, Eio_File *handler, Eet_File *file)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->themes = eina_list_append(cfdata->themes, file);
   cfdata->theme_init = eina_list_remove(cfdata->theme_init, handler);
   ecore_job_add(_open_test_cb, file);
}

static void
_open_error_cb(void *data, Eio_File *handler, int error __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->theme_init = eina_list_remove(cfdata->theme_init, handler);
   if (cfdata->free) _free_data(NULL, cfdata);
}

static void
_init_main_cb(void *data, Eio_File *handler __UNUSED__, const char *file)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->theme_init = eina_list_append(cfdata->theme_init, eio_eet_open(file, EET_FILE_MODE_READ, _open_done_cb, _open_error_cb, cfdata));
}

static void
_init_done_cb(void *data, Eio_File *handler)
{
   E_Config_Dialog_Data *cfdata = data;
   if (cfdata->init[0] == handler)
     cfdata->init[0] = NULL;
   else
     cfdata->init[1] = NULL;
   if (cfdata->free) _free_data(NULL, cfdata);
}

static void
_init_error_cb(void *data, Eio_File *handler, int error __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   if (cfdata->init[0] == handler)
     cfdata->init[0] = NULL;
   else
     cfdata->init[1] = NULL;
   if (cfdata->free) _free_data(NULL, cfdata);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;
   char theme_dir[PATH_MAX];

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   /* Grab the "Personal" themes. */
   e_user_dir_concat_static(theme_dir, "themes");
   cfdata->init[0] = eio_file_ls(theme_dir, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, cfdata);

   /* Grab the "System" themes. */
   e_prefix_data_concat_static(theme_dir, "data/themes");
   cfdata->init[1] = eio_file_ls(theme_dir, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme *t;
   Eina_List *l;
   Eio_File *ls;
   Eet_File *ef;

   if (cfdata->win_import)
     e_int_config_theme_del(cfdata->win_import);
   cfdata->win_import = NULL;
   E_FREE_LIST(cfdata->personal_file_list, eina_stringshare_del);
   E_FREE_LIST(cfdata->system_file_list, eina_stringshare_del);
   EINA_LIST_FREE(cfdata->theme_list, t)
     {
        eina_stringshare_del(t->file);
        eina_stringshare_del(t->category);
        free(t);
     }
   if (cfdata->eio[0]) eio_file_cancel(cfdata->eio[0]);
   if (cfdata->eio[1]) eio_file_cancel(cfdata->eio[1]);
   EINA_LIST_FOREACH(cfdata->theme_init, l, ls)
     eio_file_cancel(ls);
   EINA_LIST_FREE(cfdata->themes, ef)
     eet_close(ef);
   if (cfdata->eio[0] || cfdata->eio[1] || cfdata->themes || cfdata->theme_init)
     cfdata->free = EINA_TRUE;
   else
     E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *il, *ol;
   E_Zone *z;
   E_Radio_Group *rg;
   char path[4096];

   e_dialog_resizable_set(cfd->dia, 1);
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
     e_prefix_data_concat_static(path, "data/themes");
   else
     e_user_dir_concat_static(path, "themes");

   o = e_widget_flist_add(evas);
   cfdata->o_fm = o;
   {
      E_Fm2_Config *cfg;
      cfg = e_widget_flist_config_get(o);
      cfg->view.no_click_rename = 1;
   }
   evas_object_smart_callback_add(o, "dir_changed",
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
                                  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "changed",
                                  _cb_files_files_changed, cfdata);
   evas_object_smart_callback_add(o, "files_deleted",
                                  _cb_files_files_deleted, cfdata);
   e_widget_flist_path_set(o, path, "/");

   e_widget_size_min_set(o, 160, 160);
   e_widget_table_object_append(ol, o, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_list_add(evas, 0, 0);

   il = e_widget_list_add(evas, 0, 1);
   o = e_widget_button_add(evas, _(" Import..."), "preferences-desktop-theme",
                           _cb_import, cfdata, NULL);
   e_widget_list_object_append(il, o, 1, 0, 0.5);
   e_widget_list_object_append(of, il, 1, 0, 0.0);

   {
      Evas_Object *oa;
      int mw, mh;

      mw = 320;
      mh = (mw * z->h) / z->w;
      oa = e_widget_aspect_add(evas, mw, mh);
      o = e_widget_preview_add(evas, mw, mh);
      evas_object_size_hint_min_set(o, mw, mh);
      cfdata->o_preview = o;
      if (cfdata->theme)
        _e_int_theme_preview_set(o, cfdata->theme);
      e_widget_aspect_child_set(oa, o);
      e_widget_list_object_append(of, oa, 1, 1, 0);
      evas_object_show(o);
      evas_object_show(oa);
   }
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   return ot;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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
        int res;

        if (!c) continue;

        cats2 = eina_list_search_sorted_near_list(cats, _cb_sort, c, &res);
        if (!res) continue;
        if (res < 0)
          cats = eina_list_prepend_relative_list(cats, eina_stringshare_ref(c), cats2);
        else
          cats = eina_list_append_relative_list(cats, eina_stringshare_ref(c), cats2);
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
     e_prefix_data_snprintf(file, sizeof(file), "data/themes/%s.edj",
                            e_widget_ilist_nth_label_get(cfdata->o_files_ilist, n));
   else
     e_user_dir_snprintf(file, sizeof(file), "themes/%s.edj",
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

        for (i = 0; parts_list[i]; i++)
          if (strstr(parts_list[i], c_label)) break;

        if (parts_list[i])
          ret = _e_int_theme_preview_group_set(cfdata->o_preview, theme,
                                               parts_list[i] + strlen(c_label));
        if (!ret)
          _e_int_theme_preview_set(cfdata->o_preview, theme);
        eina_stringshare_del(theme);
     }
}

static void
_cb_adv_categories_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *label = NULL;
   const char *file = NULL;
   char category[256];
   Eina_List *themes = NULL;
   E_Config_Theme *t;
   Evas_Object *ic = NULL;
   int n, cnt;

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

   cnt = e_widget_ilist_count(cfdata->o_files_ilist);
   for (n = 0; n < cnt; n++)
     {
        const char *tmp;

        tmp = _files_ilist_nth_label_to_file(cfdata, n);
        eina_stringshare_del(tmp);
        if (file == tmp) /* We don't need the value, just the address. */
          {
             e_widget_ilist_selected_set(cfdata->o_files_ilist, n);
             break;
          }
     }
}

static void
_cb_adv_theme_change(void *data, Evas_Object *obj __UNUSED__)
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

static void
_ilist_item_new(E_Config_Dialog_Data *cfdata, const char *file, Eina_Bool append)
{
   char *themename;
   Evas_Object *ic = NULL;
   Eina_Bool used = EINA_FALSE;
   int sel;

   if (_theme_file_used(cfdata->theme_list, file))
     {
        ic = e_icon_add(evas_object_evas_get(cfdata->o_files_ilist));
        e_util_icon_theme_set(ic, "preferences-desktop-theme");
        used = EINA_TRUE;
     }
   themename = strdupa(ecore_file_file_get(file));
   themename[strlen(themename) - 4] = '\0';
   if (append)
     e_widget_ilist_append(cfdata->o_files_ilist, ic, themename, NULL, NULL, NULL);
   else
     e_widget_ilist_prepend(cfdata->o_files_ilist, ic, themename, NULL, NULL, NULL);
   if (!used) return;
   sel = append ? (e_widget_ilist_count(cfdata->o_files_ilist) - 1) : 0;
   e_widget_ilist_selected_set(cfdata->o_files_ilist, sel);
}

static void
_ilist_files_main_cb(void *data, Eio_File *handler, const char *file)
{
   E_Config_Dialog_Data *cfdata = data;

   if (handler == cfdata->eio[0])
     cfdata->personal_file_list = eina_list_append(cfdata->personal_file_list, eina_stringshare_add(file));
   else
     cfdata->system_file_list = eina_list_append(cfdata->system_file_list, eina_stringshare_add(file));
}

static int
_ilist_cmp_cb(const char *a, const char *b)
{
   const char *y, *z;

   y = ecore_file_file_get(a);
   z = ecore_file_file_get(b);
   return strcmp(y, z);
}

static void
_ilist_files_done_cb(void *data, Eio_File *handler)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_List *l;
   const char *file;
   if (handler == cfdata->eio[0])
     {
        cfdata->eio[0] = NULL;
        cfdata->personal_file_list = eina_list_sort(cfdata->personal_file_list, 0, (Eina_Compare_Cb)_ilist_cmp_cb);
        cfdata->personal_file_count = eina_list_count(cfdata->personal_file_list);
        if (cfdata->eio[1])
          {
             e_widget_ilist_header_prepend(cfdata->o_files_ilist, NULL, _("Personal"));
             EINA_LIST_FOREACH(cfdata->personal_file_list, l, file)
               _ilist_item_new(cfdata, file, EINA_TRUE);
             e_widget_ilist_header_append(cfdata->o_files_ilist, NULL, _("System"));
          }
        else
          {
             EINA_LIST_REVERSE_FOREACH(cfdata->personal_file_list, l, file)
               _ilist_item_new(cfdata, file, EINA_FALSE);
             e_widget_ilist_header_prepend(cfdata->o_files_ilist, NULL, _("Personal"));
          }
     }
   else
     {
        cfdata->system_file_list = eina_list_sort(cfdata->system_file_list, 0, (Eina_Compare_Cb)_ilist_cmp_cb);
        cfdata->eio[1] = NULL;
        if (cfdata->eio[0])
          e_widget_ilist_header_append(cfdata->o_files_ilist, NULL, _("System"));
        EINA_LIST_FOREACH(cfdata->system_file_list, l, file)
          _ilist_item_new(cfdata, file, EINA_TRUE);
     }
   if (cfdata->free) _free_data(NULL, cfdata);
}

static void
_ilist_files_error_cb(void *data, Eio_File *handler, int error __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   //oh well
   if (handler == cfdata->eio[0])
     cfdata->eio[0] = NULL;
   else
     cfdata->eio[1] = NULL;
   if (cfdata->free) _free_data(NULL, cfdata);
}

static Eio_File *
_ilist_files_add(E_Config_Dialog_Data *cfdata, const char *dir)
{
   return eio_file_ls(dir, _eio_filter_cb, _ilist_files_main_cb, _ilist_files_done_cb, _ilist_files_error_cb, cfdata);
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
   E_FREE_LIST(cfdata->personal_file_list, eina_stringshare_del);
   E_FREE_LIST(cfdata->system_file_list, eina_stringshare_del);
   cfdata->personal_file_count = 0;

   /* Grab the "Personal" themes. */
   e_user_dir_concat_static(theme_dir, "themes");
   cfdata->eio[0] = _ilist_files_add(cfdata, theme_dir);

   /* Grab the "System" themes. */
   e_prefix_data_concat_static(theme_dir, "data/themes");
   cfdata->eio[1] = _ilist_files_add(cfdata, theme_dir);

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
_cb_adv_btn_assign(void *data1, void *data2 __UNUSED__)
{
   Evas *evas;
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *newtheme, *t;
   Eina_List *themes;
   Evas_Object *ic = NULL, *oc = NULL, *of = NULL;
   char buf[1024];
   const char *label;
   int n, cnt;

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
                       cnt = e_widget_ilist_count(of);
                       for (n = 0; n < cnt; n++)
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
_cb_adv_btn_clear(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Eina_List *themes;
   Evas_Object *oc = NULL, *of = NULL;
   char cat[1024];
   const char *label;
   const char *filename = NULL;
   int n, cnt;

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
        cnt = e_widget_ilist_count(of);
        for (n = 0; n < cnt; n++)
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
_cb_adv_btn_clearall(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   E_Config_Theme *t;
   Eina_List *themes;
   Evas_Object *oc = NULL, *of = NULL;
   int n, cnt;

   if (!(cfdata = data1)) return;

   if (!(oc = cfdata->o_categories_ilist)) return;
   if (!(of = cfdata->o_files_ilist)) return;

   cnt = e_widget_ilist_count(oc);
   for (n = 0; n < cnt; n++)
     e_widget_ilist_nth_icon_set(oc, n, NULL);

   cnt = e_widget_ilist_count(of);
   for (n = 0; n < cnt; n++)
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

   e_dialog_resizable_set(cfd->dia, 1);
   zone = e_zone_current_get(cfd->con);
   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Theme Categories"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ob, _cb_adv_categories_change, cfdata);
   cfdata->o_categories_ilist = ob;
   e_widget_ilist_multi_select_set(ob, 0);
   e_widget_size_min_set(ob, 150, 250);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 0, 1);

   of = e_widget_framelist_add(evas, _("Themes"), 0);
   ob = e_widget_ilist_add(evas, 16, 16, NULL);
   e_widget_on_change_hook_set(ob, _cb_adv_theme_change, cfdata);
   cfdata->o_files_ilist = ob;
   e_widget_size_min_set(ob, 150, 250);
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
     _e_int_theme_preview_set(ob, cfdata->theme);
   e_widget_aspect_child_set(oa, ob);
   e_widget_framelist_object_append(of, oa);
   e_widget_table_object_append(ot, of, 2, 0, 1, 1, 1, 1, 1, 1);

   _fill_files_ilist(cfdata);
   _fill_categories_ilist(cfdata);

   e_widget_ilist_selected_set(cfdata->o_files_ilist, 1);
   e_widget_ilist_selected_set(cfdata->o_categories_ilist, 0);

   /* FIXME this makes the preview disappear at the beginning and
      when resizing (Issue is caused by e_widget_aspect i guess) */
   return ot;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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


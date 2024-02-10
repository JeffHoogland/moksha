#include "e_mod_gadman.h"

struct _E_Config_Dialog_Data
{
   Evas_Object *o_avail;
   Evas_Object *o_config;
   Evas_Object *o_fm; //Filemanager Object
   Evas_Object *o_sf; //Filemanager Scrollframe
   Evas_Object *o_btn; //Filemanager updir button
   E_Color     *color; //Custom Color
   int          bg_type; //Type of background
   int          anim_bg; //Anim the background
   int          anim_gad; //Anim the gadgets
   int          fmdir; //Filemanager dir (personal or system)
   Eina_List *waiting;
   E_Config_Dialog *cfd;
};

static const char *gadman_layer_names[] =
{
   N_("Background"),
   N_("Hover (Key Toggle)"),
   NULL
};

/* Local protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _avail_list_cb_change(void *data);
static void         _cb_fm_radio_change(void *data, Evas_Object *obj);
static void         _cb_color_changed(void *data, Evas_Object *o);
static void         _cb_fm_change(void *data, Evas_Object *obj, void *event_info);
static void         _cb_fm_sel_change(void *data, Evas_Object *obj, void *event_info);
static void         _cb_button_up(void *data1, void *data2);

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *sel;

#define CHECK(X, Y) \
   if (Man->conf->X != cfdata->Y) return 1
   CHECK(bg_type, bg_type);
   CHECK(color_r, color->r);
   CHECK(color_g, color->g);
   CHECK(color_b, color->b);
   CHECK(anim_bg, anim_bg);
   CHECK(anim_gad, anim_gad);
#undef CHECK

   sel = e_fm2_selected_list_get(cfdata->o_fm);
   if ((!sel) && (!Man->conf->custom_bg)) return 0;
   eina_list_free(sel);
   return 1;
}

E_Config_Dialog *
_config_gadman_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   //~ char buf[4096];

   /* check if config dialog exists ... */
   if (e_config_dialog_find("E", "extensions/gadman"))
     return NULL;

   /* ... else create it */
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.check_changed = _basic_check_changed;

   //~ snprintf(buf, sizeof(buf), "%s/e-module-gadman.edj", Man->module->dir);
   cfd = e_config_dialog_new(con, _("Desktop Gadgets"),
                             "E", "extensions/gadman",
                             "preferences-extensions" , 0, v, Man);

   Man->config_dialog = cfd;
   return Man->config_dialog;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   cfdata->bg_type = Man->conf->bg_type;
   if (Man->conf->custom_bg)
     {
        if (!strstr(Man->conf->custom_bg, e_user_homedir_get()))
          cfdata->fmdir = 1;
     }

   cfdata->color = E_NEW(E_Color, 1);
   cfdata->color->r = Man->conf->color_r;
   cfdata->color->g = Man->conf->color_g;
   cfdata->color->b = Man->conf->color_b;
   cfdata->color->a = Man->conf->color_a;
   cfdata->anim_bg = Man->conf->anim_bg;
   cfdata->anim_gad = Man->conf->anim_gad;

   e_color_update_rgb(cfdata->color);

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Man->config_dialog = NULL;
   E_FREE(cfdata->color);
   E_FREE(cfdata);
}

static void
_cb_config_del(void *data)
{
   int layer;
   Eina_List *l;
   E_Gadcon *gc;
   Eina_Bool del = EINA_TRUE;

   for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
     EINA_LIST_FOREACH(Man->gadcons[layer], l, gc)
       if (gc->config_dialog)
         {
            del = EINA_FALSE;
            break;
         }
   Man->waiting = eina_list_remove(Man->waiting, data);
   if (del && Man->add) ecore_event_handler_del(Man->add);
}

static void
_cb_config(void *data, void *data2 __UNUSED__)
{
   int x;
   E_Config_Dialog_Data *cfdata = data;
   Eina_List *l;
   E_Gadcon *gc;

   x = e_widget_ilist_selected_get(cfdata->o_avail);
   if (x < 0) return;

   EINA_LIST_FOREACH(Man->gadcons[x], l, gc)
     {
        if (gc->zone != cfdata->cfd->dia->win->border->zone) continue;
        if (gc->config_dialog) return;
        e_int_gadcon_config_hook(gc, _("Desktop Gadgets"), E_GADCON_SITE_DESKTOP);
        if (!Man->add)
          Man->add = ecore_event_handler_add(E_EVENT_GADCON_CLIENT_ADD, (Ecore_Event_Handler_Cb)gadman_gadget_add_handler, NULL);
        Man->waiting = eina_list_append(Man->waiting, gc);
        e_object_data_set(E_OBJECT(gc->config_dialog), cfdata);
        e_object_del_attach_func_set(E_OBJECT(gc->config_dialog), _cb_config_del);
        break;
     }
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ol, *ob, *ow, *ft, *of, *otb;
   E_Radio_Group *rg;
   Evas_Coord mw, mh;
   E_Fm2_Config fmc;
   char path[PATH_MAX];
   int layer;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Available Layers"), 0);

   //o_avail  List of available layers
   ol = e_widget_ilist_add(evas, 24, 24, NULL);
   cfdata->o_avail = ol;
   for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
     e_widget_ilist_append(ol, NULL, _(gadman_layer_names[layer]), _avail_list_cb_change, cfdata, NULL);
   e_widget_framelist_object_append(of, ol);

   //o_config  Button to configure a layer
   ob = e_widget_button_add(evas, _("Configure Layer"), NULL, _cb_config, cfdata, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->o_config = ob;
   e_widget_size_min_get(ob, &mw, &mh);
   e_widget_framelist_object_append_full(of, ob,
                                         1, 1, /* fill */
                                         1, 0, /* expand */
                                         0.5, 0.5, /* align */
                                         mw, mh, /* min */
                                         99999, 99999 /* max */
                                         );

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Layers"), o, 1, 1, 1, 1, 0.5, 0.0);
   /////////////////////////////////////////////////////////////////////
   ft = e_widget_table_add(evas, 0);

   //Background mode
   of = e_widget_frametable_add(evas, _("Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->bg_type));
   ow = e_widget_radio_add(evas, _("Theme Defined"), BG_STD, rg);
   //~ evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Custom Image"), BG_CUSTOM, rg);
   //~ evas_object_smart_callback_add(cfdata->o_custom, "changed",
   //~ _cb_method_change, cfdata);
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Custom Color"), BG_COLOR, rg);
   //~ evas_object_smart_callback_add(cfdata->o_custom, "changed",
   //~ _cb_method_change, cfdata);
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("Transparent"), BG_TRANS, rg);
   //~ evas_object_smart_callback_add(cfdata->o_custom, "changed",
   //~ _cb_method_change, cfdata);
   e_widget_frametable_object_append(of, ow, 0, 3, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ft, of, 0, 0, 1, 1, 1, 1, 1, 1);

   //Animations
   of = e_widget_frametable_add(evas, _("Animations"), 0);
   ow = e_widget_check_add(evas, _("Background"), &(cfdata->anim_bg));
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);

   ow = e_widget_check_add(evas, _("Gadgets"), &(cfdata->anim_gad));
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(ft, of, 0, 1, 1, 1, 1, 1, 1, 1);

   //Custom Color
   of = e_widget_framelist_add(evas, _("Custom Color"), 0);
   ow = e_widget_color_well_add(evas, cfdata->color, 1);
   e_widget_framelist_object_append(of, ow);
   e_widget_on_change_hook_set(ow, _cb_color_changed, cfdata);
   e_widget_table_object_append(ft, of, 0, 2, 1, 1, 1, 1, 1, 1);

   //Background filemanager chooser
   of = e_widget_frametable_add(evas, _("Custom Image"), 0);
   rg = e_widget_radio_group_new(&(cfdata->fmdir));

   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   e_widget_on_change_hook_set(ow, _cb_fm_radio_change, cfdata);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 1, 0, 0);

   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   e_widget_on_change_hook_set(ow, _cb_fm_radio_change, cfdata);
   e_widget_frametable_object_append(of, ow, 1, 0, 1, 1, 1, 1, 0, 0);

   cfdata->o_btn = e_widget_button_add(evas, _("Go up a Directory"),
                                       "widgets/up_dir", _cb_button_up,
                                       cfdata, NULL);
   e_widget_frametable_object_append(of, cfdata->o_btn, 0, 1, 2, 1, 1, 1, 1, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_fm2_add(evas);
   cfdata->o_fm = ow;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 36;
   fmc.icon.list.h = 36;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(ow, &fmc);
   e_fm2_icon_menu_flags_set(ow, E_FM2_MENU_NO_SHOW_HIDDEN);
   e_fm2_path_set(ow, path, "/");

   evas_object_smart_callback_add(ow, "selection_change",
                                  _cb_fm_sel_change, cfdata);
   evas_object_smart_callback_add(ow, "changed", _cb_fm_change, cfdata);

   cfdata->o_sf = e_widget_scrollframe_pan_add(evas, ow, e_fm2_pan_set,
                                               e_fm2_pan_get,
                                               e_fm2_pan_max_get,
                                               e_fm2_pan_child_size_get);
   e_widget_size_min_set(cfdata->o_sf, 150, 250);
   e_widget_frametable_object_append(of, cfdata->o_sf, 0, 2, 2, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ft, of, 2, 0, 1, 3, 1, 1, 1, 1);


   e_widget_toolbook_page_append(otb, NULL, _("Background Options"), ft, 0, 0, 0, 0, 0.5, 0.0);
   e_widget_toolbook_page_show(otb, 0);

   return otb;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *sel;
   const char *p = NULL;
   E_Fm2_Icon_Info *ic;
   char path[PATH_MAX];

   Man->conf->bg_type = cfdata->bg_type;
   Man->conf->color_r = cfdata->color->r;
   Man->conf->color_g = cfdata->color->g;
   Man->conf->color_b = cfdata->color->b;
   Man->conf->color_a = 255;
   Man->conf->anim_bg = cfdata->anim_bg;
   Man->conf->anim_gad = cfdata->anim_gad;

   p = e_fm2_real_path_get(cfdata->o_fm);
   sel = e_fm2_selected_list_get(cfdata->o_fm);
   if (sel && p)
     {
        ic = sel->data;
        if (ic->file)
          {
             snprintf(path, sizeof(path), "%s/%s", p, ic->file);
             eina_stringshare_replace(&Man->conf->custom_bg, path);
          }
        eina_list_free(sel);
     }

   gadman_gadget_edit_end(NULL, NULL, NULL, NULL);
   e_config_save_queue();
   gadman_update_bg();

   return 1;
}

//Basic Callbacks
static void
_avail_list_cb_change(void *data)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_config, 0);
}

//Advanced Callbacks
static void
_cb_color_changed(void *data, Evas_Object *o __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;
   if (!cfdata) return;
   e_color_update_rgb(cfdata->color);
}

static void
_cb_fm_radio_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];

   cfdata = data;
   if (!cfdata->o_fm) return;
   if (cfdata->fmdir == 0)
     e_user_dir_concat_static(path, "backgrounds");
   else
     e_prefix_data_concat_static(path, "data/backgrounds");
   e_fm2_path_set(cfdata->o_fm, path, "/");
}

static void
_cb_fm_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *p;
   char path[PATH_MAX];
   size_t len;

   cfdata = data;
   if (!Man->conf->custom_bg) return;
   if (!cfdata->o_fm) return;

   p = e_fm2_real_path_get(cfdata->o_fm);
   if (!p) return;

   if (strncmp(p, Man->conf->custom_bg, strlen(p))) return;

   len = e_user_dir_concat_static(path, "backgrounds");
   if (!strncmp(Man->conf->custom_bg, path, len))
     p = Man->conf->custom_bg + len + 1;
   else
     {
        len = e_prefix_data_concat_static(path, "data/backgrounds");
        if (!strncmp(Man->conf->custom_bg, path, len))
          p = Man->conf->custom_bg + len + 1;
        else
          p = Man->conf->custom_bg;
     }

   e_fm2_select_set(cfdata->o_fm, p, 1);
   e_fm2_file_show(cfdata->o_fm, p);
}

static void
_cb_fm_sel_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_widget_change(cfdata->o_sf);
}

static void
_cb_button_up(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (!cfdata->o_fm) return;

   e_fm2_parent_go(cfdata->o_fm);
   e_widget_scrollframe_child_pos_set(cfdata->o_sf, 0, 0);
}

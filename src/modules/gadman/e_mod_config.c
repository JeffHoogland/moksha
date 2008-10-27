#include <e.h>
#include "e_mod_main.h"
#include "e_mod_gadman.h"
#include "e_mod_config.h"
#include "config.h"

struct _E_Config_Dialog_Data
{
   Evas_Object *o_avail;        //List of available gadgets
   Evas_Object *o_add;          //Add button
   Evas_Object *o_fm;           //Filemanager Object
   Evas_Object *o_sf;           //Filemanager Scrollframe
   Evas_Object *o_btn;          //Filemanager updir button
   E_Color *color;              //Custom Color
   int bg_method;               //Type of background
   int anim_bg;                 //Anim the background
   int anim_gad;                //Anim the gadgets
   int fmdir;                   //Filemanager dir (personal or system)
};

/* Local protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_gadgets_list(Evas_Object *ilist);
static void _cb_add(void *data, void *data2);
static void _avail_list_cb_change(void *data, Evas_Object *obj);
static void _cb_fm_radio_change(void *data, Evas_Object *obj);
static void _cb_color_changed(void *data, Evas_Object *o);
static void _cb_fm_change(void *data, Evas_Object *obj, void *event_info);
static void _cb_fm_sel_change(void *data, Evas_Object *obj, void *event_info);
static void _cb_button_up(void *data1, void *data2);

EAPI E_Config_Dialog *
_config_gadman_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   /* check if config dialog exists ... */
   if (e_config_dialog_find("E", "_e_modules_gadman_config_dialog"))
     return NULL;

   /* ... else create it */
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.apply_cfdata = _basic_apply_data;
   v->advanced.create_widgets = _adv_create_widgets;
   v->advanced.apply_cfdata = _adv_apply_data;

   snprintf(buf, sizeof(buf), "%s/e-module-gadman.edj", Man->module->dir);
   cfd = e_config_dialog_new(con, _("Gadgets Manager"),
                             "E", "_e_modules_gadman_config_dialog",
                             buf, 0, v, Man);

   Man->config_dialog = cfd;
   return Man->config_dialog;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->bg_method = Man->conf->bg_type;
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
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Man->config_dialog = NULL;
   E_FREE(cfdata->color);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ol;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Available Gadgets"), 0);

   //o_avail  List of available gadgets
   ol = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ol, 0);
   e_widget_on_change_hook_set(ol, _avail_list_cb_change, cfdata);
   cfdata->o_avail = ol;
   _fill_gadgets_list(ol);
   e_widget_framelist_object_append(of, ol);

   //o_add  Button to add a gadget
   ob = e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->o_add = ob;
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);
   
   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   gadman_gadget_edit_end();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_adv_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ow, *ft, *of;
   E_Radio_Group *rg;
   E_Fm2_Config fmc;
   char path[PATH_MAX];

   ft = e_widget_table_add(evas, 0);

   //Background mode
   of = e_widget_frametable_add(evas, _("Background Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->bg_method));
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
   ow = e_widget_check_add(evas, "Background", &(cfdata->anim_bg));
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   
   ow = e_widget_check_add(evas, "Gadgets", &(cfdata->anim_gad));
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
   e_widget_frametable_object_append(of, cfdata->o_btn, 0, 1, 2, 1, 1, 1, 0, 0);


   if (cfdata->fmdir == 1)
     snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
   else
     snprintf(path, sizeof(path), "%s/.e/e/backgrounds", e_user_homedir_get());

   ow = e_fm2_add(evas);
   cfdata->o_fm = ow;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 48;
   fmc.icon.list.h = 48;
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
   e_widget_min_size_set(cfdata->o_sf, 200, 250);
   e_widget_frametable_object_append(of, cfdata->o_sf, 0, 2, 2, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ft, of, 1, 0, 1, 3, 1, 1, 1, 1);

   e_dialog_resizable_set(cfd->dia, 1);

   return ft;
}

static int
_adv_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_List *sel;
   const char *p = NULL;
   E_Fm2_Icon_Info *ic;
   char path[PATH_MAX];

   Man->conf->bg_type = cfdata->bg_method;
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
		if (Man->conf->custom_bg)
		  eina_stringshare_del(Man->conf->custom_bg);
		Man->conf->custom_bg = eina_stringshare_add(path);
	  }
	eina_list_free(sel);
     }

   e_config_save_queue();
   gadman_update_bg();
   
   return 1;
}
//Basic Callbacks
static void 
_fill_gadgets_list(Evas_Object *ilist)
{
   Eina_List *l = NULL;
   int w;

   e_widget_ilist_freeze(ilist);
   e_widget_ilist_clear(ilist);

   for (l = e_gadcon_provider_list(); l; l = l->next) 
     {
        E_Gadcon_Client_Class *cc;
        Evas_Object *icon = NULL;
        const char *lbl = NULL;

        if (!(cc = l->data)) continue;
        if (cc->func.label) lbl = cc->func.label();
        if (!lbl) lbl = cc->name;
        if (cc->func.icon) icon = cc->func.icon(Man->gc->evas);
        e_widget_ilist_append(ilist, icon, lbl, NULL, (void *)cc, NULL);
     }

   e_widget_ilist_go(ilist);
   e_widget_min_size_get(ilist, &w, NULL);
   if (w < 200) w = 200;
   e_widget_min_size_set(ilist, w, 250);
   e_widget_ilist_thaw(ilist);
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *l = NULL;
   int i;

   if (!(cfdata = data)) return;

   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_avail); l; l = l->next, i++) 
     {
        E_Ilist_Item *item = NULL;
        E_Gadcon_Client_Class *cc;
        E_Gadcon_Client *gcc;

        if (!(item = l->data)) continue;
        if (!item->selected) continue;

        cc = e_widget_ilist_nth_data_get(cfdata->o_avail, i);
        if (!cc) continue;

        gcc = gadman_gadget_add(cc, 0);
        gadman_gadget_edit_start(gcc);
     }

   if (l) eina_list_free(l);
}

static void 
_avail_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_add, 0);
}

//Advanced Callbacks
static void
_cb_color_changed(void *data, Evas_Object *o)
{
   E_Config_Dialog_Data *cfdata;
   cfdata = data;
   if (!cfdata) return;
   e_color_update_rgb(cfdata->color);
}

static void
_cb_fm_radio_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];
   
   cfdata = data;
   if (!cfdata->o_fm) return;
   if (cfdata->fmdir == 0) 
     snprintf(path, sizeof(path), "%s/.e/e/backgrounds", e_user_homedir_get());
   else 
     snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
   e_fm2_path_set(cfdata->o_fm, path, "/");
}

static void 
_cb_fm_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *p;
   char path[PATH_MAX];

   cfdata = data;
   if (!Man->conf->custom_bg) return;
   if (!cfdata->o_fm) return;

   p = e_fm2_real_path_get(cfdata->o_fm);
   if (!p) return; 

   if (strncmp(p, Man->conf->custom_bg, strlen(p))) return;

   snprintf(path, sizeof(path), "%s/.e/e/backgrounds", e_user_homedir_get());
   if (!strncmp(Man->conf->custom_bg, path, strlen(path))) 
     p = Man->conf->custom_bg + strlen(path) + 1;
   else 
     {
	snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
	if (!strncmp(Man->conf->custom_bg, path, strlen(path)))
	  p = Man->conf->custom_bg + strlen(path) + 1;
	else
	  p = Man->conf->custom_bg;
     }

   e_fm2_select_set(cfdata->o_fm, p, 1);
   e_fm2_file_show(cfdata->o_fm, p);
}

static void 
_cb_fm_sel_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   e_widget_change(cfdata->o_sf);
}
static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (!cfdata->o_fm) return;

   e_fm2_parent_go(cfdata->o_fm);
   e_widget_scrollframe_child_pos_set(cfdata->o_sf, 0, 0);
}


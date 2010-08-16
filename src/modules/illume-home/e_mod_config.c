#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* local function prototypes */
static void *_il_home_config_create(E_Config_Dialog *cfd);
static void _il_home_config_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_home_config_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_home_config_changed(void *data, Evas_Object *obj, void *event);
static void _il_home_config_slider_changed(void *data, Evas_Object *obj);
static void _il_home_config_click_changed(void *data, Evas_Object *obj, void *event);
static Eina_Bool _il_home_config_change_timeout(void *data);

/* local variables */
EAPI Il_Home_Config *il_home_cfg = NULL;
static E_Config_DD *conf_edd = NULL;
Ecore_Timer *_il_home_config_change_timer = NULL;
Evas_Object *delay_label, *delay_slider;

/* public functions */
int 
il_home_config_init(E_Module *m) 
{
   char buff[PATH_MAX];

   conf_edd = E_CONFIG_DD_NEW("Illume-Home_Cfg", Il_Home_Config);
   #undef T
   #undef D
   #define T Il_Home_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, icon_size, INT);
   E_CONFIG_VAL(D, T, single_click, INT);
   E_CONFIG_VAL(D, T, single_click_delay, INT);

   il_home_cfg = e_config_domain_load("module.illume-home", conf_edd);
   if ((il_home_cfg) && 
       ((il_home_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     {
        E_FREE(il_home_cfg);
        il_home_cfg = NULL;
     }
   if (!il_home_cfg) 
     {
        il_home_cfg = E_NEW(Il_Home_Config, 1);
        il_home_cfg->version = 0;
        il_home_cfg->icon_size = 120;
        il_home_cfg->single_click = 1;
        il_home_cfg->single_click_delay = 50;
     }
   if (il_home_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_home_cfg->version & 0xffff) < 1) */
        il_home_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }

   il_home_cfg->mod_dir = eina_stringshare_add(m->dir);

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);

   e_configure_registry_category_add("illume", 0, _("Illume"), NULL, 
                                     "enlightenment/display");
   e_configure_registry_generic_item_add("illume/home", 0, _("Home"), 
                                         buff, "icon", il_home_config_show);
   return 1;
}

int 
il_home_config_shutdown(void) 
{
   il_home_cfg->cfd = NULL;

   e_configure_registry_item_del("illume/home");
   e_configure_registry_category_del("illume");

   if (il_home_cfg->mod_dir) eina_stringshare_del(il_home_cfg->mod_dir);

   E_FREE(il_home_cfg);
   il_home_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

int 
il_home_config_save(void) 
{
   e_config_domain_save("module.illume-home", conf_edd, il_home_cfg);
   return 1;
}

void 
il_home_config_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v = NULL;

   if (e_config_dialog_find("E", "_config_illume_home_settings")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_home_config_create;
   v->free_cfdata = _il_home_config_free;
   v->basic.create_widgets = _il_home_config_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;

   cfd = e_config_dialog_new(con, _("Home Settings"), "E", 
                             "_config_illume_home_settings", 
                             "enlightenment/launcher_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   il_home_cfg->cfd = cfd;
}

/* local functions */
static void *
_il_home_config_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_home_config_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   il_home_cfg->cfd = NULL;
   il_home_win_cfg_update();
}

static Evas_Object *
_il_home_config_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *of, *o;
   E_Radio_Group *rg;

   list = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Icon Size"), 0);
   rg = e_widget_radio_group_new(&(il_home_cfg->icon_size));
   o = e_widget_radio_add(evas, _("Small"), 60, rg);
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", _il_home_config_changed, NULL);
   o = e_widget_radio_add(evas, _("Medium"), 80, rg);
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", _il_home_config_changed, NULL);
   o = e_widget_radio_add(evas, _("Large"), 120, rg);
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", _il_home_config_changed, NULL);
   o = e_widget_radio_add(evas, _("Very Large"), 160, rg);
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", _il_home_config_changed, NULL);
   o = e_widget_radio_add(evas, _("Massive"), 240, rg);
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", _il_home_config_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Launch Action"), 0);
   o = e_widget_check_add(evas, _("Single press"), 
                          &(il_home_cfg->single_click));
   e_widget_framelist_object_append(of, o);
   evas_object_smart_callback_add(o, "changed", 
                                  _il_home_config_click_changed, NULL);
   o = e_widget_label_add(evas, _("Press Delay"));
   delay_label = o;
   e_widget_disabled_set(o, !(il_home_cfg->single_click));
   e_widget_framelist_object_append(of, o);
   o = e_widget_slider_add(evas, 1, 0, "%1.0f ms", 0, 350, 1, 0, NULL, 
                           &(il_home_cfg->single_click_delay), 150);
   delay_slider = o;
   e_widget_on_change_hook_set(o, _il_home_config_slider_changed, NULL);
   e_widget_disabled_set(o, !(il_home_cfg->single_click));
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   return list;
}

static void 
_il_home_config_changed(void *data, Evas_Object *obj, void *event) 
{
   if (_il_home_config_change_timer) 
     ecore_timer_del(_il_home_config_change_timer);
   _il_home_config_change_timer = 
     ecore_timer_add(0.5, _il_home_config_change_timeout, data);
}

static void 
_il_home_config_slider_changed(void *data, Evas_Object *obj) 
{
   if (_il_home_config_change_timer) 
     ecore_timer_del(_il_home_config_change_timer);
   _il_home_config_change_timer = 
     ecore_timer_add(0.5, _il_home_config_change_timeout, data);
}

static void 
_il_home_config_click_changed(void *data, Evas_Object *obj, void *event) 
{
   e_widget_disabled_set(delay_label, !il_home_cfg->single_click);
   e_widget_disabled_set(delay_slider, !il_home_cfg->single_click);
   _il_home_config_changed(data, obj, event);
}

static Eina_Bool
_il_home_config_change_timeout(void *data) 
{
   il_home_win_cfg_update();
   e_config_save_queue();
   _il_home_config_change_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

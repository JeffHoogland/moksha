#include "e.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_mod_policy_settings.h"

/* local function prototypes */
static void *_il_config_policy_settings_create(E_Config_Dialog *cfd);
static void _il_config_policy_settings_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_policy_settings_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_policy_settings_changed(void *data, Evas_Object *obj, void *event);
static int _il_config_policy_settings_change_timeout(void *data);

/* local variables */
Ecore_Timer *_ps_change_timer = NULL;
Evas_Object *o_top, *o_left;

void 
il_config_policy_settings_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_policy_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_config_policy_settings_create;
   v->free_cfdata = _il_config_policy_settings_free;
   v->basic.create_widgets = _il_config_policy_settings_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Policy Settings"), "E", 
                             "_config_illume_policy_settings", 
                             "enlightenment/policy_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local function prototypes */
static void *
_il_config_policy_settings_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_policy_settings_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_ps_change_timer) ecore_timer_del(_ps_change_timer);
}

static Evas_Object *
_il_config_policy_settings_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *of, *ow;
   E_Radio_Group *rg;

   list = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Mode"), 0);
   rg = e_widget_radio_group_new(&(il_cfg->policy.mode.dual));
   ow = e_widget_radio_add(evas, _("Single App Mode"), 0, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_policy_settings_changed, NULL);
   ow = e_widget_radio_add(evas, _("Dual App Mode"), 1, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_policy_settings_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Window Layout"), 0);
   rg = e_widget_radio_group_new(&(il_cfg->policy.mode.side));
   o_top = e_widget_radio_add(evas, _("Top/Bottom"), 0, rg);
   e_widget_framelist_object_append(of, o_top);
   evas_object_smart_callback_add(o_top, "changed", 
                                  _il_config_policy_settings_changed, NULL);
   o_left = e_widget_radio_add(evas, _("Left/Right"), 1, rg);
   e_widget_framelist_object_append(of, o_left);
   evas_object_smart_callback_add(o_left, "changed", 
                                  _il_config_policy_settings_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   e_widget_disabled_set(o_top, !il_cfg->policy.mode.dual);
   e_widget_disabled_set(o_left, !il_cfg->policy.mode.dual);

   return list;
}

static void 
_il_config_policy_settings_changed(void *data, Evas_Object *obj, void *event) 
{
   e_widget_disabled_set(o_top, !il_cfg->policy.mode.dual);
   e_widget_disabled_set(o_left, !il_cfg->policy.mode.dual);

   if (_ps_change_timer) ecore_timer_del(_ps_change_timer);
   _ps_change_timer = 
     ecore_timer_add(0.5, _il_config_policy_settings_change_timeout, data);
}

static int 
_il_config_policy_settings_change_timeout(void *data) 
{
   Ecore_X_Window xwin;
   Ecore_X_Illume_Mode mode;

   e_config_save_queue();
   _ps_change_timer = NULL;

   if (il_cfg->policy.mode.dual)
     mode = ECORE_X_ILLUME_MODE_DUAL;
   else 
     mode = ECORE_X_ILLUME_MODE_SINGLE;

   xwin = ecore_x_window_root_first_get();
   ecore_x_e_illume_mode_set(xwin, mode);
   ecore_x_e_illume_mode_send(xwin, mode);

   return 0;
}

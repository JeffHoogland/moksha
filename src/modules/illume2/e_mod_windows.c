#include "E_Illume.h"
#include "e_mod_windows.h"
#include "e_mod_select_window.h"

/* local function prototypes */
static void *_il_config_windows_create(E_Config_Dialog *cfd);
static void _il_config_windows_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_windows_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_windows_check_changed(void *data, Evas_Object *obj, void *event);
static void _il_config_windows_change(void *data, Evas_Object *obj, void *event);
static int _il_config_windows_change_timeout(void *data);
static void _il_config_windows_select_home(void *data, void *data2);
static void _il_config_windows_select_vkbd(void *data, void *data2);
static void _il_config_windows_select_softkey(void *data, void *data2);
static void _il_config_windows_select_indicator(void *data, void *data2);

/* local variables */
Ecore_Timer *_windows_change_timer = NULL;

/* public functions */
void 
il_config_windows_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_windows_settings")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return;

   v->create_cfdata = _il_config_windows_create;
   v->free_cfdata = _il_config_windows_free;
   v->basic.create_widgets = _il_config_windows_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Window Settings"), "E", 
                             "_config_illume_windows_settings", 
                             "preferences-system-windows", 0, v, NULL);
   if (!cfd) return;
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local function prototypes */
static void *
_il_config_windows_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_windows_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_windows_change_timer) ecore_timer_del(_windows_change_timer);
   _windows_change_timer = NULL;
}

static Evas_Object *
_il_config_windows_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *of, *ow;

   list = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Home"), 0);
   ow = e_widget_button_add(evas, _("Select Window"), NULL, 
                            _il_config_windows_select_home, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Match Window Class"), 
                           &il_cfg->policy.home.match.class);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Name"), 
                           &il_cfg->policy.home.match.name);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Title"), 
                           &il_cfg->policy.home.match.title);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Type"), 
                           &il_cfg->policy.home.match.win_type);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Indicator"), 0);
   ow = e_widget_button_add(evas, _("Select Window"), NULL, 
                            _il_config_windows_select_indicator, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Match Window Class"), 
                           &il_cfg->policy.indicator.match.class);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Name"), 
                           &il_cfg->policy.indicator.match.name);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Title"), 
                           &il_cfg->policy.indicator.match.title);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Type"), 
                           &il_cfg->policy.indicator.match.win_type);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Keyboard"), 0);
   ow = e_widget_button_add(evas, _("Select Window"), NULL, 
                            _il_config_windows_select_vkbd, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Match Window Class"), 
                           &il_cfg->policy.vkbd.match.class);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Name"), 
                           &il_cfg->policy.vkbd.match.name);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Title"), 
                           &il_cfg->policy.vkbd.match.title);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Type"), 
                           &il_cfg->policy.vkbd.match.win_type);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Softkey"), 0);
   ow = e_widget_button_add(evas, _("Select Window"), NULL, 
                            _il_config_windows_select_softkey, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Match Window Class"), 
                           &il_cfg->policy.softkey.match.class);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Name"), 
                           &il_cfg->policy.softkey.match.name);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Title"), 
                           &il_cfg->policy.softkey.match.title);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   ow = e_widget_check_add(evas, _("Match Window Type"), 
                           &il_cfg->policy.softkey.match.win_type);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_windows_check_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   return list;
}

static void 
_il_config_windows_check_changed(void *data, Evas_Object *obj, void *event) 
{
   _il_config_windows_change(data, obj, event);
}

static void 
_il_config_windows_change(void *data, Evas_Object *obj, void *event) 
{
   if (_windows_change_timer) ecore_timer_del(_windows_change_timer);
   _windows_change_timer = 
     ecore_timer_add(0.5, _il_config_windows_change_timeout, data);
}

static int 
_il_config_windows_change_timeout(void *data) 
{
   e_config_save_queue();
   _windows_change_timer = NULL;
   return 0;
}

static void 
_il_config_windows_select_home(void *data, void *data2) 
{
   if (e_config_dialog_find("E", "_config_illume_select_window")) return;
   il_config_select_window(IL_SELECT_WINDOW_TYPE_HOME);
}

static void 
_il_config_windows_select_vkbd(void *data, void *data2) 
{
   if (e_config_dialog_find("E", "_config_illume_select_window")) return;
   il_config_select_window(IL_SELECT_WINDOW_TYPE_VKBD);
}

static void 
_il_config_windows_select_softkey(void *data, void *data2) 
{
   if (e_config_dialog_find("E", "_config_illume_select_window")) return;
   il_config_select_window(IL_SELECT_WINDOW_TYPE_SOFTKEY);
}

static void 
_il_config_windows_select_indicator(void *data, void *data2) 
{
   if (e_config_dialog_find("E", "_config_illume_select_window")) return;
   il_config_select_window(IL_SELECT_WINDOW_TYPE_INDICATOR);
}

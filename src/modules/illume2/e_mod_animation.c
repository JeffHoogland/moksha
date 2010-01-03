#include "E_Illume.h"
#include "e_mod_animation.h"

/* local function prototypes */
static void *_il_config_animation_create(E_Config_Dialog *cfd);
static void _il_config_animation_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_config_animation_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_config_animation_change(void *data, Evas_Object *obj, void *event);
static int _il_config_animation_change_timeout(void *data);

/* local variables */
Ecore_Timer *_anim_change_timer = NULL;

/* public functions */
void 
il_config_animation_show(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_animation_settings")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_config_animation_create;
   v->free_cfdata = _il_config_animation_free;
   v->basic.create_widgets = _il_config_animation_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;
   cfd = e_config_dialog_new(con, _("Animation Settings"), "E", 
                             "_config_illume_animation_settings", 
                             "enlightenment/animation_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
}

/* local function prototypes */
static void *
_il_config_animation_create(E_Config_Dialog *cfd) 
{
   return NULL;
}

static void 
_il_config_animation_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_anim_change_timer) ecore_timer_del(_anim_change_timer);
}

static Evas_Object *
_il_config_animation_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *list, *of, *ow;
   E_Radio_Group *rg;

   list = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Keyboard"), 0);
   rg = e_widget_radio_group_new(&(il_cfg->sliding.kbd.duration));
   ow = e_widget_radio_add(evas, _("Slow"), 2000, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Medium"), 1000, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Fast"), 500, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Very Fast"), 250, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Off"), 0, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Quickpanel"), 0);
   rg = e_widget_radio_group_new(&(il_cfg->sliding.quickpanel.duration));
   ow = e_widget_radio_add(evas, _("Slow"), 2000, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Medium"), 1000, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Fast"), 500, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Very Fast"), 250, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   ow = e_widget_radio_add(evas, _("Off"), 0, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", 
                                  _il_config_animation_change, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);

   return list;
}

static void 
_il_config_animation_change(void *data, Evas_Object *obj, void *event) 
{
   if (_anim_change_timer) ecore_timer_del(_anim_change_timer);
   _anim_change_timer = 
     ecore_timer_add(0.5, _il_config_animation_change_timeout, data);
}

static int 
_il_config_animation_change_timeout(void *data) 
{
   e_config_save_queue();
   _anim_change_timer = NULL;
   return 0;
}

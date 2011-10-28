#include "e.h"

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _basic_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void _basic_use_dpi_changed(void *data, Evas_Object *obj __UNUSED__);
static Evas_Object *_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _adv_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void _adv_policy_changed(void *data, Evas_Object *obj __UNUSED__);

struct _E_Config_Dialog_Data 
{
   int use_dpi;
   double min, max, factor;
   int use_mode, base_dpi, use_custom;
   struct 
     {
        struct 
          {
             Evas_Object *o_lbl, *o_slider;
          } basic;
        struct 
          {
             Evas_Object *dpi_lbl, *dpi_slider;
             Evas_Object *custom_slider;
             Evas_Object *min_lbl, *min_slider;
             Evas_Object *max_lbl, *max_slider;
          } adv;
     } gui;
};

E_Config_Dialog *
e_int_config_scale(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "appearance/scale")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_changed;
   v->advanced.create_widgets = _adv_create;
   v->advanced.apply_cfdata = _adv_apply;
   v->advanced.check_changed = _adv_changed;

   cfd = e_config_dialog_new(con, _("Scale Settings"), "E", "appearance/scale", 
                             "preferences-scale", 0, v, NULL);
   return cfd;
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->use_dpi = e_config->scale.use_dpi;
   cfdata->use_custom = e_config->scale.use_custom;
   cfdata->use_mode = 0;
   if (cfdata->use_dpi) cfdata->use_mode = 1;
   else if (cfdata->use_custom) cfdata->use_mode = 2;
   cfdata->min = e_config->scale.min;
   cfdata->max = e_config->scale.max;
   cfdata->factor = e_config->scale.factor;
   cfdata->base_dpi = e_config->scale.base_dpi;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ow;
   char buff[256];

   o = e_widget_list_add(evas, 0, 0);

   ow = e_widget_check_add(evas, _("Scale with DPI"), &(cfdata->use_dpi));
   e_widget_on_change_hook_set(ow, _basic_use_dpi_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Relative"), 0);
   ow = e_widget_label_add(evas, _("Base DPI to scale relative to"));
   cfdata->gui.basic.o_lbl = ow;
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f DPI"), 30, 600, 1, 0, 
                            NULL, &(cfdata->base_dpi), 100);
   cfdata->gui.basic.o_slider = ow;
   e_widget_framelist_object_append(of, ow);
   snprintf(buff, sizeof(buff), _("Currently %i DPI"), ecore_x_dpi_get());
   ow = e_widget_label_add(evas, buff);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   _basic_use_dpi_changed(cfdata, NULL);
   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   cfdata->use_custom = 0;
   if (cfdata->use_dpi) cfdata->use_mode = 1;
   else cfdata->use_mode = 0;

   e_config->scale.use_dpi = cfdata->use_dpi;
   e_config->scale.use_custom = cfdata->use_custom;
   e_config->scale.min = cfdata->min;
   e_config->scale.max = cfdata->max;
   e_config->scale.factor = cfdata->factor;
   e_config->scale.base_dpi = cfdata->base_dpi;

   e_scale_update();
   e_canvas_recache();
   e_config_save_queue();

   return 1;
}

static int 
_basic_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   return ((cfdata->use_dpi != e_config->scale.use_dpi) || 
           (cfdata->base_dpi != e_config->scale.base_dpi));
}

static void 
_basic_use_dpi_changed(void *data, Evas_Object *obj __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.basic.o_lbl, !cfdata->use_dpi);
   e_widget_disabled_set(cfdata->gui.basic.o_slider, !cfdata->use_dpi);
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *otb, *ow;
   E_Radio_Group *rg;
   char buff[256];

   otb = e_widget_toolbook_add(evas, 24, 24);

   /* Policy */
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->use_mode));
   ow = e_widget_radio_add(evas, _("Don't Scale"), 0, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Scale relative to screen DPI"), 1, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);

   snprintf(buff, sizeof(buff), 
            _("Base DPI (Currently %i DPI)"), ecore_x_dpi_get());
   ow = e_widget_label_add(evas, buff);
   cfdata->gui.adv.dpi_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f DPI"), 30, 600, 1, 0, 
                            NULL, &(cfdata->base_dpi), 100);
   cfdata->gui.adv.dpi_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Custom scaling factor"), 2, rg);
   e_widget_on_change_hook_set(ow, _adv_policy_changed, cfdata);
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f x"), 0.25, 8.0, 0.05, 
                            0, &(cfdata->factor), NULL, 100);
   cfdata->gui.adv.custom_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Policy"), o, 
                                 1, 0, 1, 0, 0.5, 0.0);


   /* Constraints */
   o = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Minimum"));
   cfdata->gui.adv.min_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05, 
                            0, &(cfdata->min), NULL, 150);
   cfdata->gui.adv.min_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_label_add(evas, _("Maximum"));
   cfdata->gui.adv.max_lbl = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05, 
                            0, &(cfdata->max), NULL, 150);
   cfdata->gui.adv.max_slider = ow;
   e_widget_list_object_append(o, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Constraints"), o, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   _adv_policy_changed(cfdata, NULL);
   return otb;
}

static int 
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   cfdata->use_custom = 0;
   cfdata->use_dpi = 0;
   if (cfdata->use_mode == 1) 
     cfdata->use_dpi = 1;
   else if (cfdata->use_mode == 2) 
     cfdata->use_custom = 1;

   e_config->scale.use_dpi = cfdata->use_dpi;
   e_config->scale.use_custom = cfdata->use_custom;
   e_config->scale.min = cfdata->min;
   e_config->scale.max = cfdata->max;
   e_config->scale.factor = cfdata->factor;
   e_config->scale.base_dpi = cfdata->base_dpi;

   e_scale_update();
   e_canvas_recache();
   e_config_save_queue();

   return 1;
}

static int 
_adv_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   int use_dpi = 0, use_custom = 0;

   if (cfdata->use_mode == 1)
     use_dpi = 1;
   else if (cfdata->use_mode == 2)
     use_custom = 1;

   return ((use_dpi != e_config->scale.use_dpi) || 
           (use_custom != e_config->scale.use_custom) || 
           (cfdata->min != e_config->scale.min) || 
           (cfdata->max != e_config->scale.max) || 
           (cfdata->factor != e_config->scale.factor) || 
           (cfdata->base_dpi != e_config->scale.base_dpi));
   return 1;
}

static void 
_adv_policy_changed(void *data, Evas_Object *obj __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->gui.adv.dpi_lbl, (cfdata->use_mode != 1));
   e_widget_disabled_set(cfdata->gui.adv.dpi_slider, (cfdata->use_mode != 1));
   e_widget_disabled_set(cfdata->gui.adv.custom_slider, (cfdata->use_mode != 2));
}

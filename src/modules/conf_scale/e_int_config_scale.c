#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   int use_dpi;

   /* Advanced */
   double min;
   double max;
   double factor;
   int use_mode;
   
   int base_dpi;
   int use_custom;

   struct {
      struct {
	 Evas_Object *label;
	 Evas_Object *slider;
      } basic;
      struct {
	 Evas_Object *dpi_label;
	 Evas_Object *dpi_slider;
	 Evas_Object *custom_slider;
	 Evas_Object *min_label;
	 Evas_Object *min_slider;
	 Evas_Object *max_label;
	 Evas_Object *max_slider;
      } adv;
   } gui;
};

EAPI E_Config_Dialog *
e_int_config_scale(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_scale_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.check_changed = _advanced_check_changed;

   cfd = e_config_dialog_new(con,
			     _("Scaling Settings"),
			     "E", "_config_scale_dialog",
			     "preferences-scale", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->use_dpi = e_config->scale.use_dpi;
   cfdata->use_custom = e_config->scale.use_custom;
   cfdata->use_mode = 0;
   if (cfdata->use_dpi) cfdata->use_mode = 1;
   if (cfdata->use_custom) cfdata->use_mode = 2;
   cfdata->min = e_config->scale.min;
   cfdata->max = e_config->scale.max;
   cfdata->factor = e_config->scale.factor;
   cfdata->base_dpi = e_config->scale.base_dpi;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
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
_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return ((cfdata->use_dpi != e_config->scale.use_dpi) ||
	   (cfdata->base_dpi != e_config->scale.base_dpi));
}

static void
_scale_basic_use_dpi_cb_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;
   e_widget_disabled_set(cfdata->gui.basic.label, !cfdata->use_dpi);
   e_widget_disabled_set(cfdata->gui.basic.slider, !cfdata->use_dpi);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   char buf[256];
   
   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Scale with DPI"), &(cfdata->use_dpi));
   e_widget_on_change_hook_set(ob, _scale_basic_use_dpi_cb_change, cfdata);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);   

   of = e_widget_framelist_add(evas, _("Relative"), 0);
   ob = e_widget_label_add(evas, _("Base DPI to scale relative to"));
   cfdata->gui.basic.label = ob;
   e_widget_framelist_object_append(of, ob);   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f DPI"), 30, 1200, 1, 0, NULL, &(cfdata->base_dpi), 150);
   cfdata->gui.basic.slider = ob;
   e_widget_framelist_object_append(of, ob);
   snprintf(buf, sizeof(buf), _("Currently %i DPI"), ecore_x_dpi_get());
   ob = e_widget_label_add(evas, buf);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   _scale_basic_use_dpi_cb_change(cfdata, NULL);

   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->use_mode == 0)
     {
	cfdata->use_dpi = 0;
	cfdata->use_custom = 0;
     }
   else if (cfdata->use_mode == 1)
     {
	cfdata->use_dpi = 1;
	cfdata->use_custom = 0;
     }
   else if (cfdata->use_mode == 2)
     {
	cfdata->use_dpi = 0;
	cfdata->use_custom = 1;
     }
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
_advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   int use_dpi, use_custom;

   if (cfdata->use_mode == 1)
     {
	use_dpi = 1;
	use_custom = 0;
     }
   else if (cfdata->use_mode == 2)
     {
	use_dpi = 0;
	use_custom = 1;
     }
   else
     {
	use_dpi = 0;
	use_custom = 0;
     }

   return ((use_dpi != e_config->scale.use_dpi) ||
	   (use_custom != e_config->scale.use_custom) ||
	   (cfdata->min != e_config->scale.min) ||
	   (cfdata->max != e_config->scale.max) ||
	   (cfdata->factor != e_config->scale.factor) ||
	   (cfdata->base_dpi != e_config->scale.base_dpi));
}

static void
_scale_adv_policy_cb_changed(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;

   e_widget_disabled_set(cfdata->gui.adv.dpi_label, cfdata->use_mode != 1);
   e_widget_disabled_set(cfdata->gui.adv.dpi_slider, cfdata->use_mode != 1);
   e_widget_disabled_set(cfdata->gui.adv.custom_slider, cfdata->use_mode != 2);

   e_widget_disabled_set(cfdata->gui.adv.min_label, cfdata->use_mode == 0);
   e_widget_disabled_set(cfdata->gui.adv.min_slider, cfdata->use_mode == 0);
   e_widget_disabled_set(cfdata->gui.adv.max_label, cfdata->use_mode == 0);
   e_widget_disabled_set(cfdata->gui.adv.max_slider, cfdata->use_mode == 0);
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
   char buf[256];

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Policy"), 0);

   rg = e_widget_radio_group_new(&(cfdata->use_mode));
   ob = e_widget_radio_add(evas, _("Don't Scale"), 0, rg);
   e_widget_on_change_hook_set(ob, _scale_adv_policy_cb_changed, cfdata);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_radio_add(evas, _("Scale relative to screen DPI"), 1, rg);
   e_widget_on_change_hook_set(ob, _scale_adv_policy_cb_changed, cfdata);
   e_widget_framelist_object_append(of, ob);
   snprintf(buf, sizeof(buf), _("Base DPI (Currently %i DPI)"), ecore_x_dpi_get());
   ob = e_widget_label_add(evas, buf);
   cfdata->gui.adv.dpi_label = ob;
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f DPI"), 30, 1200, 1, 0, NULL, &(cfdata->base_dpi), 150);
   cfdata->gui.adv.dpi_slider = ob;
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_radio_add(evas, _("Custom scaling factor"), 2, rg);
   e_widget_on_change_hook_set(ob, _scale_adv_policy_cb_changed, cfdata);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05, 0, &(cfdata->factor), NULL, 150);
   cfdata->gui.adv.custom_slider = ob;
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Constraints"), 0);
   ob = e_widget_label_add(evas, _("Minimum"));
   cfdata->gui.adv.min_label = ob;
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05, 0, &(cfdata->min), NULL, 150);
   cfdata->gui.adv.min_slider = ob;
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Maximum"));
   cfdata->gui.adv.max_label = ob;
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f times"), 0.25, 8.0, 0.05, 0, &(cfdata->max), NULL, 150);
   cfdata->gui.adv.max_slider = ob;
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   _scale_adv_policy_cb_changed(cfdata, NULL);

   return o;
}

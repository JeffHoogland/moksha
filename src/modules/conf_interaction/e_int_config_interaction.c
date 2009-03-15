#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _enabled_cb(void *data, Evas_Object *obj, void *event_info);

struct _E_Config_Dialog_Data 
{
   Evas_Object *l1, *l2, *l3, *sl1, *sl2, *sl3;
   int thumbscroll_enable;
   int thumbscroll_threshhold;
   double thumbscroll_momentum_threshhold;
   double thumbscroll_friction;
};

EAPI E_Config_Dialog *
e_int_config_interaction(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_config_interaction_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con,
			     _("Interaction Settings"),
			     "E", "_config_config_interaction_dialog",
			     "preferences-interaction", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->thumbscroll_enable = e_config->thumbscroll_enable;
   cfdata->thumbscroll_threshhold = e_config->thumbscroll_threshhold;
   cfdata->thumbscroll_momentum_threshhold = e_config->thumbscroll_momentum_threshhold;
   cfdata->thumbscroll_friction = e_config->thumbscroll_friction;
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
_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   return ((cfdata->thumbscroll_enable != e_config->thumbscroll_enable) ||
	   (cfdata->thumbscroll_threshhold != e_config->thumbscroll_threshhold) ||
	   (cfdata->thumbscroll_momentum_threshhold != e_config->thumbscroll_momentum_threshhold) ||
	   (cfdata->thumbscroll_friction != e_config->thumbscroll_friction));
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->thumbscroll_enable = cfdata->thumbscroll_enable;
   e_config->thumbscroll_threshhold = cfdata->thumbscroll_threshhold;
   e_config->thumbscroll_momentum_threshhold = cfdata->thumbscroll_momentum_threshhold;
   e_config->thumbscroll_friction = cfdata->thumbscroll_friction;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
 
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Thumbscroll"), 0);

   ob = e_widget_check_add(evas, _("Enable Thumbscroll"), &(cfdata->thumbscroll_enable));
   e_widget_framelist_object_append(of, ob);
   evas_object_smart_callback_add(ob, "changed", _enabled_cb, cfdata);
 
   ob = e_widget_label_add(evas, _("Threshold for a thumb drag"));
   e_widget_framelist_object_append(of, ob);
   cfdata->l1 = ob;
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels"), 0, 64, 4, 0, NULL, &(cfdata->thumbscroll_threshhold), 200);
   e_widget_framelist_object_append(of, ob);
   cfdata->sl1 = ob;
   
   ob = e_widget_label_add(evas, _("Threshold for for applying drag momentum"));
   e_widget_framelist_object_append(of, ob);
   cfdata->l2 = ob;
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f pixels/sec"), 0, 2000, 20, 0, &(cfdata->thumbscroll_momentum_threshhold), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   cfdata->sl2 = ob;
   
   ob = e_widget_label_add(evas, _("Friction slowdown"));
   e_widget_framelist_object_append(of, ob);
   cfdata->l3 = ob;
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f sec"), 0.00, 5.0, 0.1, 0, &(cfdata->thumbscroll_friction), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   cfdata->sl3 = ob;

   if (!e_config->thumbscroll_enable)
     {
	e_widget_disabled_set(cfdata->l1, 1);
	e_widget_disabled_set(cfdata->sl1, 1);
	e_widget_disabled_set(cfdata->l2, 1);
	e_widget_disabled_set(cfdata->sl2, 1);
	e_widget_disabled_set(cfdata->l3, 1);
	e_widget_disabled_set(cfdata->sl3, 1);
     }

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static void
_enabled_cb(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;

   e_widget_disabled_set(cfdata->l1, !cfdata->thumbscroll_enable);
   e_widget_disabled_set(cfdata->sl1, !cfdata->thumbscroll_enable);
   e_widget_disabled_set(cfdata->l2, !cfdata->thumbscroll_enable);
   e_widget_disabled_set(cfdata->sl2, !cfdata->thumbscroll_enable);
   e_widget_disabled_set(cfdata->l3, !cfdata->thumbscroll_enable);
   e_widget_disabled_set(cfdata->sl3, !cfdata->thumbscroll_enable);
}

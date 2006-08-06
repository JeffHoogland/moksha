#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int show_popup;
   double popup_speed;
   int drag_resist;
};

/* Protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void 
_config_pager_module(Config_Item *ci)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;

   snprintf(buf, sizeof(buf), "%s/module.eap", e_module_dir_get(pager_config->module));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     _("Pager Configuration"), buf, 0, v, ci);
   pager_config->config_dialog = cfd;
}

static void 
_fill_data(Config_Item *ci, E_Config_Dialog_Data *cfdata) 
{
   /* FIXME: configure zone config item */
   cfdata->show_popup = pager_config->popup;
   cfdata->popup_speed = pager_config->popup_speed;
   cfdata->drag_resist = pager_config->drag_resist;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   Config_Item *ci;
   
   ci = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(ci, cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   pager_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("General Settings"), 0);
   ob = e_widget_check_add(evas, _("Show Popup"), &(cfdata->show_popup));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   pager_config->popup = cfdata->show_popup;
   _pager_cb_config_updated();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Resistance to Dragging Windows:"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%.0f px"), 0.0, 10.0, 1.0, 0, NULL, &(cfdata->drag_resist), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Popup Settings"), 0);   
   ob = e_widget_check_add(evas, _("Show Popup"), &(cfdata->show_popup));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Popup Speed"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.1, 10.0, 0.1, 0, &(cfdata->popup_speed), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);   

   return o;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   pager_config->popup = cfdata->show_popup;
   pager_config->popup_speed = cfdata->popup_speed;
   pager_config->drag_resist = cfdata->drag_resist;
   _pager_cb_config_updated();
   e_config_save_queue();
   return 1;
}

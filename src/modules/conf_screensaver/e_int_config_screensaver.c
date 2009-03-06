#include "e.h"

#define E_CONFIG_BLANKING_DEFAULT 2
#define E_CONFIG_BLANKING_PREFERRED 1
#define E_CONFIG_BLANKING_NOT_PREFERRED 0

#define E_CONFIG_EXPOSURES_DEFAULT 2
#define E_CONFIG_EXPOSURES_ALLOWED 1
#define E_CONFIG_EXPOSURES_NOT_ALLOWED 0

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);
static int  _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					      E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   
   int enable_screensaver;
   double timeout;
   double interval;
   int blanking;
   int exposures;
};

EAPI E_Config_Dialog *
e_int_config_screensaver(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_screensaver_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con,_("Screen Saver Settings"),
			     "E", "_config_screensaver_dialog",
			     "preferences-desktop-screensaver", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->enable_screensaver = e_config->screensaver_enable;
   cfdata->timeout = e_config->screensaver_timeout / 60;
   cfdata->interval = e_config->screensaver_interval;
   cfdata->blanking = e_config->screensaver_blanking;
   cfdata->exposures = e_config->screensaver_expose;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return;
   E_FREE(cfdata);
}

static int
_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{   
   e_config->screensaver_enable = cfdata->enable_screensaver;
   e_config->screensaver_timeout = cfdata->timeout * 60;
   e_config->screensaver_interval = cfdata->interval;
   e_config->screensaver_blanking = cfdata->blanking;
   e_config->screensaver_expose = cfdata->exposures;

   /* Apply settings */
   e_screensaver_init();
   return 1;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  _apply_data(cfd, cfdata);
   
  e_config_save_queue();
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Enable X screensaver"), &(cfdata->enable_screensaver));
   e_widget_list_object_append(o, ob, 1, 1, 0);   
   
   of = e_widget_framelist_add(evas, _("Screensaver Timer(s)"), 0);

   ob = e_widget_label_add(evas, _("Time until X screensaver starts"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0, 
			    &(cfdata->timeout), NULL, 100);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

/* advanced window */
static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return 0;
   _apply_data(cfd, cfdata);
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Enable X screensaver"), &(cfdata->enable_screensaver));
   e_widget_list_object_append(o, ob, 1, 1, 0);   
   
   of = e_widget_framelist_add(evas, _("Screensaver Timer(s)"), 0);

   ob = e_widget_label_add(evas, _("Time until X screensaver starts"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"),
			    1.0, 90.0, 1.0, 0, &(cfdata->timeout), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_label_add(evas, _("Time until X screensaver alternates"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 300.0, 1.0, 0, &(cfdata->interval), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Blanking"), 0);
   rg = e_widget_radio_group_new(&(cfdata->blanking));
   ob = e_widget_radio_add(evas, _("Default"), E_CONFIG_BLANKING_DEFAULT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Preferred"), E_CONFIG_BLANKING_PREFERRED, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Not Preferred"), E_CONFIG_BLANKING_NOT_PREFERRED, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Exposure Events"), 0);
   rg = e_widget_radio_group_new(&(cfdata->exposures));
   ob = e_widget_radio_add(evas, _("Default"), E_CONFIG_EXPOSURES_DEFAULT, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Allow"), E_CONFIG_EXPOSURES_ALLOWED, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Don't Allow"), E_CONFIG_EXPOSURES_NOT_ALLOWED, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;   
}

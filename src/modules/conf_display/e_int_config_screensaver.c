#include "e.h"

#define E_CONFIG_BLANKING_DEFAULT 2
#define E_CONFIG_BLANKING_PREFERRED 1
#define E_CONFIG_BLANKING_NOT_PREFERRED 0

#define E_CONFIG_EXPOSURES_DEFAULT 2
#define E_CONFIG_EXPOSURES_ALLOWED 1
#define E_CONFIG_EXPOSURES_NOT_ALLOWED 0

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_disable(void *data, Evas_Object *obj);
static void _cb_ask_presentation_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   int enable_screensaver;
   double timeout;
   double interval;
   int blanking;
   int exposures;
   int ask_presentation;
   double ask_presentation_timeout;
   
   int screensaver_suspend;
   int screensaver_suspend_on_ac;
   double screensaver_suspend_delay;

   Eina_List *disable_list;

   struct 
     {
        Evas_Object *ask_presentation_slider;
     } gui;
};

E_Config_Dialog *
e_int_config_screensaver(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "screen/screen_saver")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con,_("Screen Saver Settings"),
			     "E", "screen/screen_saver",
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
   cfdata->ask_presentation = e_config->screensaver_ask_presentation;
   cfdata->ask_presentation_timeout = e_config->screensaver_ask_presentation_timeout;
   cfdata->screensaver_suspend = e_config->screensaver_suspend;
   cfdata->screensaver_suspend_on_ac = e_config->screensaver_suspend_on_ac;
   cfdata->screensaver_suspend_delay = e_config->screensaver_suspend_delay;
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
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->disable_list);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->screensaver_enable = cfdata->enable_screensaver;
   e_config->screensaver_timeout = (cfdata->timeout * 60);
   e_config->screensaver_interval = cfdata->interval;
   e_config->screensaver_blanking = cfdata->blanking;
   e_config->screensaver_expose = cfdata->exposures;
   e_config->screensaver_ask_presentation = cfdata->ask_presentation;
   e_config->screensaver_ask_presentation_timeout = cfdata->ask_presentation_timeout;
   e_config->screensaver_suspend = cfdata->screensaver_suspend;
   e_config->screensaver_suspend_on_ac = cfdata->screensaver_suspend_on_ac;
   e_config->screensaver_suspend_delay = cfdata->screensaver_suspend_delay;
   
   /* Apply settings */
   e_screensaver_update();

   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->screensaver_enable != cfdata->enable_screensaver) ||
	   (e_config->screensaver_timeout != (int)(cfdata->timeout * 60)) ||
	   (e_config->screensaver_interval != cfdata->interval) ||
	   (e_config->screensaver_blanking != cfdata->blanking) ||
	   (e_config->screensaver_expose != cfdata->exposures) ||
	   (e_config->screensaver_ask_presentation != cfdata->ask_presentation) ||
	   (e_config->screensaver_ask_presentation_timeout != cfdata->ask_presentation_timeout) ||
	   (e_config->screensaver_suspend != cfdata->screensaver_suspend) ||
	   (e_config->screensaver_suspend_on_ac != cfdata->screensaver_suspend_on_ac) ||
	   (e_config->screensaver_suspend_delay != cfdata->screensaver_suspend_delay));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ow, *of;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Screensaver */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Enable X Screensaver"), 
                           &(cfdata->enable_screensaver));
   e_widget_on_change_hook_set(ow, _cb_disable, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_check_add(evas, _("Suspend on blank"), 
                           &(cfdata->screensaver_suspend));
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_check_add(evas, _("Even if on power"), 
                           &(cfdata->screensaver_suspend_on_ac));
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_label_add(evas, _("Delay until suspend"));
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 20.0, 1.0, 0, &(cfdata->screensaver_suspend_delay), NULL, 100);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Screensaver"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Timers */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Initial timeout"));
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"),
			    1.0, 90.0, 1.0, 0, &(cfdata->timeout), NULL, 100);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_label_add(evas, _("Alternation timeout"));
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 300.0, 1.0, 0, &(cfdata->interval), NULL, 100);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Timers"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Presentation */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Suggest if deactivated before"), 
                           &(cfdata->ask_presentation));
   e_widget_on_change_hook_set(ow, _cb_ask_presentation_changed, cfdata);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 300.0, 10.0, 0,
			    &(cfdata->ask_presentation_timeout), NULL, 100);
   cfdata->gui.ask_presentation_slider = ow;
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Presentation Mode"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Misc */
   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Blanking"), 0);
   rg = e_widget_radio_group_new(&(cfdata->blanking));
   ow = e_widget_radio_add(evas, _("Default"), E_CONFIG_BLANKING_DEFAULT, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Preferred"), E_CONFIG_BLANKING_PREFERRED, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Not Preferred"), E_CONFIG_BLANKING_NOT_PREFERRED, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Exposure Events"), 0);
   rg = e_widget_radio_group_new(&(cfdata->exposures));
   ow = e_widget_radio_add(evas, _("Default"), E_CONFIG_EXPOSURES_DEFAULT, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Allow"), E_CONFIG_EXPOSURES_ALLOWED, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Don't Allow"), 
                           E_CONFIG_EXPOSURES_NOT_ALLOWED, rg);
   cfdata->disable_list = eina_list_append(cfdata->disable_list, ow);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Miscellaneous"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);

   // handler for enable/disable widget array
   _cb_disable(cfdata, NULL);

   return otb;
}

static void
_cb_disable(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const Eina_List *l;
   Evas_Object *o;

   if (!(cfdata = data)) return;
   EINA_LIST_FOREACH(cfdata->disable_list, l, o)
     e_widget_disabled_set(o, !cfdata->enable_screensaver);

   _cb_ask_presentation_changed(cfdata, NULL);
}

static void
_cb_ask_presentation_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_Bool disable;

   if (!(cfdata = data)) return;
   disable = ((!cfdata->enable_screensaver) || (!cfdata->ask_presentation));
   e_widget_disabled_set(cfdata->gui.ask_presentation_slider, disable);
}

#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int show_alert;
   int poll_interval;
#ifdef HAVE_EEZE
   int fuzzy;
#endif
   int alert_time;
   int alert_percent;
   int dismiss_alert;
   int alert_timeout;
   int suspend_below;
   int suspend_method;
   int force_mode; // 0 == auto, 1 == batget, 2 == subsystem
#ifdef HAVE_ENOTIFY
   int desktop_notifications;
#endif
   struct
   {
      Evas_Object *show_alert_label;
      Evas_Object *show_alert_time;
      Evas_Object *show_alert_percent;
      Evas_Object *dismiss_alert_label;
      Evas_Object *alert_timeout;
#ifdef HAVE_EEZE
      Evas_Object *fuzzy;
#endif
   } ui;
};

/* Protos */
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _cb_radio_changed(void *data, Evas_Object *obj __UNUSED__);

E_Config_Dialog *
e_int_config_battery_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   //~ char buf[PATH_MAX];

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->advanced.check_changed = _advanced_check_changed;

   //~ snprintf(buf, sizeof(buf), "%s/e-module-battery.edj",
            //~ e_module_dir_get(battery_config->module));
   cfd = e_config_dialog_new(con, _("Battery Monitor Settings"),
                             "E", "_e_mod_battery_config_dialog",
                             "battery", 0, v, NULL);
   battery_config->config_dialog = cfd;
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   if (!battery_config) return;
   cfdata->alert_time = battery_config->alert;
   cfdata->alert_percent = battery_config->alert_p;
   cfdata->poll_interval = battery_config->poll_interval;
   cfdata->alert_timeout = battery_config->alert_timeout;
   cfdata->suspend_below = battery_config->suspend_below;
   cfdata->suspend_method = battery_config->suspend_method;
   cfdata->force_mode = battery_config->force_mode;
#ifdef HAVE_EEZE
   cfdata->fuzzy = battery_config->fuzzy;
#endif
#ifdef HAVE_ENOTIFY
   cfdata->desktop_notifications = battery_config->desktop_notifications;
#endif

   if ((cfdata->alert_time > 0) || (cfdata->alert_percent > 0))
     cfdata->show_alert = 1;
   else
     cfdata->show_alert = 0;

   if (cfdata->alert_timeout > 0)
     cfdata->dismiss_alert = 1;
   else
     cfdata->dismiss_alert = 0;
}

static void
_ensure_alert_time(E_Config_Dialog_Data *cfdata)
{
   if ((cfdata->alert_time > 0) || (cfdata->alert_percent > 0))
     return;

   // must handle the case where user toggled the checkbox but set no threshold
   cfdata->alert_time = 5;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!battery_config) return;
   battery_config->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob;

   o = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Show alert when battery is low"),
                           &(cfdata->show_alert));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
#ifdef HAVE_ENOTIFY
   ob = e_widget_check_add(evas, _("Use desktop notifications for alert"),
                           &(cfdata->desktop_notifications));
   e_widget_list_object_append(o, ob, 1, 0, 0.5);
#endif
   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!battery_config) return 0;

   if (cfdata->show_alert)
     {
        _ensure_alert_time(cfdata);
        battery_config->alert = cfdata->alert_time;
        battery_config->alert_p = cfdata->alert_percent;
#ifdef HAVE_ENOTIFY
        battery_config->desktop_notifications = cfdata->desktop_notifications;
#endif
     }
   else
     {
        battery_config->alert = 0;
        battery_config->alert_p = 0;
#ifdef HAVE_ENOTIFY
        battery_config->desktop_notifications = EINA_FALSE;
#endif
     }

   _battery_config_updated();
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_Bool ret;
   int old_show_alert = ((battery_config->alert > 0) ||
                         (battery_config->alert_p > 0));

   ret = (cfdata->show_alert != old_show_alert);
#ifdef HAVE_ENOTIFY
   ret |= (cfdata->desktop_notifications != battery_config->desktop_notifications);
#endif
   return ret;
}

static void
_cb_show_alert_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool show_alert = cfdata->show_alert;
   Eina_Bool dismiss_alert = cfdata->show_alert && cfdata->dismiss_alert;

   e_widget_disabled_set(cfdata->ui.show_alert_label, !show_alert);
   e_widget_disabled_set(cfdata->ui.show_alert_time, !show_alert);
   e_widget_disabled_set(cfdata->ui.show_alert_percent, !show_alert);
   e_widget_disabled_set(cfdata->ui.dismiss_alert_label, !show_alert);

   e_widget_disabled_set(cfdata->ui.alert_timeout, !dismiss_alert);
}

static void
_cb_dismiss_alert_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool dismiss_alert = cfdata->show_alert && cfdata->dismiss_alert;

   e_widget_disabled_set(cfdata->ui.alert_timeout, !dismiss_alert);
}

static void
_cb_radio_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_Bool fuzzy;

   if (!(cfdata = data)) return;
   fuzzy = (cfdata->force_mode == SUBSYSTEM);
#ifdef HAVE_EEZE
   e_widget_disabled_set(cfdata->ui.fuzzy, !fuzzy);
#endif
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *otb;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   /* Use Sliders for both cfg options */
   o = e_widget_table_add(evas, 0);

   ob = e_widget_label_add(evas, _("Check every:"));
   e_widget_table_object_append(o, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 1, 256, 4, 0,
                            NULL, &(cfdata->poll_interval), 100);
   e_widget_table_object_append(o, ob, 0, 1, 1, 1, 1, 0, 1, 0);

   rg = e_widget_radio_group_new(&(cfdata->suspend_method));
   ob = e_widget_radio_add(evas, _("Suspend when below:"), 0, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_table_object_append(o, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_radio_add(evas, _("Hibernate when below:"), 1, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_table_object_append(o, ob, 0, 3, 1, 1, 1, 0, 1, 0);
   ob = e_widget_radio_add(evas, _("Shutdown when below:"), 2, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_table_object_append(o, ob, 0, 4, 1, 1, 1, 0, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f %%"), 0, 50, 1, 0,
                            NULL, &(cfdata->suspend_below), 100);
   e_widget_table_object_append(o, ob, 0, 5, 1, 1, 1, 0, 1, 0);

   e_widget_toolbook_page_append(otb, NULL, _("Polling"), o, 1, 0, 1, 0,
                                 0.5, 0.0);

   o = e_widget_table_add(evas, 0);
   ob = e_widget_check_add(evas, _("Show low battery alert"),
                           &(cfdata->show_alert));
   e_widget_on_change_hook_set(ob, _cb_show_alert_changed, cfdata);
   e_widget_table_object_append(o, ob, 0, 0, 1, 1, 1, 1, 1, 0);
   ob = e_widget_label_add(evas, _("Alert when at:"));
   cfdata->ui.show_alert_label = ob;
   e_widget_table_object_append(o, ob, 0, 1, 1, 1, 1, 0, 1, 1);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f min"), 0, 60, 1, 0, NULL,
                            &(cfdata->alert_time), 100);
   cfdata->ui.show_alert_time = ob;
   e_widget_table_object_append(o, ob, 0, 2, 1, 1, 1, 0, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f %%"), 0, 100, 1, 0, NULL,
                            &(cfdata->alert_percent), 100);
   cfdata->ui.show_alert_percent = ob;
   e_widget_table_object_append(o, ob, 0, 3, 1, 1, 1, 0, 1, 0);
   ob = e_widget_check_add(evas, _("Auto dismiss in..."),
                           &(cfdata->dismiss_alert));
   cfdata->ui.dismiss_alert_label = ob;
   e_widget_on_change_hook_set(ob, _cb_dismiss_alert_changed, cfdata);
   e_widget_table_object_append(o, ob, 0, 4, 1, 1, 1, 1, 1, 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f s"), 1, 300, 1, 0, NULL,
                            &(cfdata->alert_timeout), 100);
   cfdata->ui.alert_timeout = ob;
   e_widget_table_object_append(o, ob, 0, 5, 1, 1, 1, 0, 1, 0);

   _cb_show_alert_changed(cfdata, NULL);

   e_widget_toolbook_page_append(otb, NULL, _("Alert"), o, 1, 0, 1, 0,
                                 0.5, 0.0);

   o = e_widget_list_add(evas, 0, 0);

   rg = e_widget_radio_group_new(&(cfdata->force_mode));
   ob = e_widget_radio_add(evas, _("Auto Detect"), UNKNOWN, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_list_object_append(o, ob, 1, 0, 0.0);
   ob = e_widget_radio_add(evas, _("Internal"), NOSUBSYSTEM, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_list_object_append(o, ob, 1, 0, 0.0);
#ifdef HAVE_EEZE
   ob = e_widget_radio_add(evas, _("udev"), SUBSYSTEM, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
   e_widget_list_object_append(o, ob, 1, 0, 0.0);
   ob = e_widget_check_add(evas, _("Fuzzy Mode"),
                           &(cfdata->fuzzy));
   cfdata->ui.fuzzy = ob;
#else
   ob = e_widget_radio_add(evas, _("HAL"), 2, rg);
   e_widget_on_change_hook_set(ob, _cb_radio_changed, cfdata);
#endif
   e_widget_list_object_append(o, ob, 1, 0, 0.0);

   e_widget_toolbook_page_append(otb, NULL, _("Hardware"), o, 1, 0, 1, 0,
                                 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!battery_config) return 0;

   battery_config->poll_interval = cfdata->poll_interval;
#ifdef HAVE_EEZE
   battery_config->fuzzy = cfdata->fuzzy;
#endif

   if (cfdata->show_alert)
     {
        _ensure_alert_time(cfdata);
        battery_config->alert = cfdata->alert_time;
        battery_config->alert_p = cfdata->alert_percent;
     }
   else
     {
        battery_config->alert = 0;
        battery_config->alert_p = 0;
     }

   if ((cfdata->dismiss_alert) && (cfdata->alert_timeout > 0))
     battery_config->alert_timeout = cfdata->alert_timeout;
   else
     battery_config->alert_timeout = 0;

   battery_config->force_mode = cfdata->force_mode;
   battery_config->suspend_below = cfdata->suspend_below;
   battery_config->suspend_method = cfdata->suspend_method;

   _battery_config_updated();
   e_config_save_queue();
   return 1;
}

static int
_advanced_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   int old_show_alert = ((battery_config->alert > 0) ||
                         (battery_config->alert_p > 0));
   int old_dismiss_alert = (battery_config->alert_timeout > 0);

   return (cfdata->alert_time != battery_config->alert) ||
          (cfdata->alert_percent != battery_config->alert_p) ||
          (cfdata->poll_interval != battery_config->poll_interval) ||
          (cfdata->alert_timeout != battery_config->alert_timeout) ||
          (cfdata->suspend_below != battery_config->suspend_below) ||
          (cfdata->suspend_method != battery_config->suspend_method) ||
#ifdef HAVE_EEZE
          (cfdata->fuzzy != battery_config->fuzzy) ||
#endif
          (cfdata->force_mode != battery_config->force_mode) ||
          (cfdata->show_alert != old_show_alert) ||
          (cfdata->dismiss_alert != old_dismiss_alert);
}


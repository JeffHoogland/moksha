#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					      E_Config_Dialog_Data *cfdata);
static void _cb_standby_slider_change(void *data, Evas_Object *obj);
static void _cb_suspend_slider_change(void *data, Evas_Object *obj);
static void _cb_off_slider_change(void *data, Evas_Object *obj);

static int _e_int_config_dpms_available(void);
static int _e_int_config_dpms_capable(void);

static void _cb_disable_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   Evas_Object *standby_slider;
   Evas_Object *suspend_slider;
   Evas_Object *off_slider;

   int enable_dpms;
   int enable_standby;
   int enable_suspend;
   int enable_off;

   /*
    * The following timeouts are represented as minutes
    * while the underlying e_config variables are in seconds
    */
   double standby_timeout;
   double suspend_timeout;
   double off_timeout;
   Eina_List *dpms_list;
   
   double backlight_normal;
   double backlight_dim;
   double backlight_transition;
};

/* static E_Dialog *dpms_dialog = NULL; */

/* always allow as dmps now has backlight too
static void
_cb_dpms_dialog_ok(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(dpms_dialog));
   dpms_dialog = NULL;
}
*/

static int
_e_int_config_dpms_capable(void)
{
   return 1;
/* always allow as dmps now has backlight too 
 * this all needs to merge with screensaver too into a simple screen blank+
 * dim+brightness etc. config
   if (ecore_x_dpms_capable_get()) return 1;

   if (dpms_dialog) e_object_del(E_OBJECT(dpms_dialog));
   dpms_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()),
			      "E", "_dpms_capable_dialog");
   if (!dpms_dialog) return 0;

   e_dialog_title_set(dpms_dialog, _("Display Power Management Signaling"));
   e_dialog_text_set(dpms_dialog, _("The current display server is not <br>"
				    "DPMS capable."));
   e_dialog_icon_set(dpms_dialog, "preferences-system-power-management", 64);
   e_dialog_button_add(dpms_dialog, _("OK"), NULL, _cb_dpms_dialog_ok, NULL);
   e_dialog_button_focus_num(dpms_dialog, 1);
   e_win_centered_set(dpms_dialog->win, 1);
   e_dialog_show(dpms_dialog);
 */
   return 0;
}

static int
_e_int_config_dpms_available(void)
{
   return 1;
/* always allow as dmps now has backlight too
   if (ecore_x_dpms_query()) return 1;

   if (dpms_dialog) e_object_del(E_OBJECT(dpms_dialog));
   dpms_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()),
			      "E", "_dpms_available_dialog");
   if (!dpms_dialog) return 0;

   e_dialog_title_set(dpms_dialog, _("Display Power Management Signaling"));
   e_dialog_text_set(dpms_dialog, _("The current display server does not <br>"
				    "have the DPMS extension."));
   e_dialog_icon_set(dpms_dialog, "preferences-system-power-management", 64);
   e_dialog_button_add(dpms_dialog, _("OK"), NULL, _cb_dpms_dialog_ok, NULL);
   e_dialog_button_focus_num(dpms_dialog, 1);
   e_win_centered_set(dpms_dialog->win, 1);
   e_dialog_show(dpms_dialog);
   return 0;
 */
}

E_Config_Dialog *
e_int_config_dpms(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if ((e_config_dialog_find("E", "screen/power_management")) ||
       (!_e_int_config_dpms_available()) ||
       (!_e_int_config_dpms_capable()))
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _advanced_apply_data;
   v->basic.create_widgets = _advanced_create_widgets;
   v->basic.check_changed = _advanced_check_changed;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Display Power Management Settings"), "E",
			     "screen/power_management", "preferences-system-power-management",
			     0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->enable_dpms = e_config->dpms_enable;
   cfdata->enable_standby = e_config->dpms_standby_enable;
   cfdata->standby_timeout = e_config->dpms_standby_timeout / 60;
   cfdata->enable_suspend = e_config->dpms_suspend_enable;
   cfdata->suspend_timeout = e_config->dpms_suspend_timeout / 60;
   cfdata->enable_off = e_config->dpms_off_enable;
   cfdata->off_timeout = e_config->dpms_off_timeout / 60;
   cfdata->backlight_normal = e_config->backlight.normal * 100.0;
   cfdata->backlight_dim = e_config->backlight.dim * 100.0;
   cfdata->backlight_transition = e_config->backlight.transition;
}

static void
_list_disabled_state_apply(Eina_List *list, Eina_Bool disabled)
{
   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(list, l, o)
      e_widget_disabled_set(o, disabled);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;

   cfdata->standby_slider = NULL;
   cfdata->suspend_slider = NULL;
   cfdata->off_slider = NULL;

   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->dpms_list);
   E_FREE(cfdata);
}

static int
_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->dpms_enable = cfdata->enable_dpms;
   e_config->dpms_standby_enable = cfdata->enable_standby;
   e_config->dpms_suspend_enable = cfdata->enable_suspend;
   e_config->dpms_off_enable = cfdata->enable_off;

   e_config->dpms_standby_timeout = cfdata->standby_timeout * 60;
   e_config->dpms_suspend_timeout = cfdata->suspend_timeout * 60;
   e_config->dpms_off_timeout = cfdata->off_timeout * 60;
   
   e_config->backlight.normal = cfdata->backlight_normal / 100.0;
   e_config->backlight.dim = cfdata->backlight_dim / 100.0;
   e_config->backlight.transition = cfdata->backlight_transition;

   e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_NORMAL);
   e_backlight_level_set(NULL, e_config->backlight.normal, -1.0);
   
   e_config_save_queue();
   e_dpms_update();
   return 1;
}

/* advanced window */
static int
_advanced_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return (e_config->dpms_enable != cfdata->enable_dpms) ||
	  (e_config->dpms_standby_enable != cfdata->enable_standby) ||
	  (e_config->dpms_suspend_enable != cfdata->enable_suspend) ||
	  (e_config->dpms_off_enable != cfdata->enable_off) ||
	  (e_config->dpms_standby_timeout / 60 != cfdata->standby_timeout) ||
	  (e_config->dpms_suspend_timeout / 60 != cfdata->suspend_timeout) ||
	  (e_config->dpms_off_timeout / 60 != cfdata->off_timeout) ||
          (e_config->backlight.normal * 100.0 != cfdata->backlight_normal) ||
          (e_config->backlight.dim * 100.0 != cfdata->backlight_dim) ||
          (e_config->backlight.transition != cfdata->backlight_transition);
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   _apply_data(cfd, cfdata);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *otb;
   Evas_Object *dpms_check;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));
   
   /* dpms */
   o = e_widget_list_add(evas, 0, 0);

   dpms_check = e_widget_check_add(evas, _("Enable Display Power Management"),
			   &(cfdata->enable_dpms));
   e_widget_list_object_append(o, dpms_check, 1, 1, 0);

   of = e_widget_framelist_add(evas, _("Timers"), 0);

   ob = e_widget_check_add(evas, _("Standby time"), &(cfdata->enable_standby));
   e_widget_framelist_object_append(of, ob);
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0,
			    &(cfdata->standby_timeout), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_standby_slider_change, cfdata);
   cfdata->standby_slider = ob;
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Suspend time"), &(cfdata->enable_suspend));
   e_widget_framelist_object_append(of, ob);
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0,
			    &(cfdata->suspend_timeout), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_suspend_slider_change, cfdata);
   cfdata->suspend_slider = ob;
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Off time"), &(cfdata->enable_off));
   e_widget_framelist_object_append(of, ob);
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0,
			    &(cfdata->off_timeout), NULL, 100);
   e_widget_on_change_hook_set(ob, _cb_off_slider_change, cfdata);
   cfdata->off_slider = ob;
   cfdata->dpms_list = eina_list_append(cfdata->dpms_list, ob);
   e_widget_disabled_set(ob, !cfdata->enable_standby); // set state from saved config
   e_widget_framelist_object_append(of, ob);

   // handler for enable/disable widget array
   e_widget_on_change_hook_set(dpms_check, _cb_disable_changed, cfdata->dpms_list);
   // setting initial state
   _list_disabled_state_apply(cfdata->dpms_list, !cfdata->enable_dpms);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("DPMS"), o,
                                 1, 0, 1, 0, 0.5, 0.0);

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_label_add(evas, _("Normal Backlight"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%3.0f"), 0.0, 100.0, 1.0, 0,
			    &(cfdata->backlight_normal), NULL, 100);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   ob = e_widget_label_add(evas, _("Dim Backlight"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%3.0f"), 0.0, 100.0, 1.0, 0,
			    &(cfdata->backlight_dim), NULL, 100);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   ob = e_widget_label_add(evas, _("Fade Time"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f sec"), 0.0, 5.0, 0.1, 0,
			    &(cfdata->backlight_transition), NULL, 100);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Backlight"), o,
                                 1, 0, 1, 0, 0.5, 0.0);
   
   e_widget_toolbook_page_show(otb, 0);
   
   return otb;
}

/* general functionality/callbacks */
static void
_cb_standby_slider_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   /* standby-slider */
   if (cfdata->standby_timeout > cfdata->suspend_timeout)
     {
	cfdata->suspend_timeout = cfdata->standby_timeout;
	if (cfdata->suspend_slider)
	  e_widget_slider_value_double_set(cfdata->suspend_slider,
					   cfdata->suspend_timeout);

	if (cfdata->suspend_timeout > cfdata->off_timeout)
	  {
	     cfdata->off_timeout = cfdata->suspend_timeout;
	     if (cfdata->off_slider)
	       e_widget_slider_value_double_set(cfdata->off_slider,
						cfdata->off_timeout);
	  }
     }
}

static void
_cb_suspend_slider_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   /* suspend-slider */
   if (cfdata->suspend_timeout > cfdata->off_timeout)
     {
	cfdata->off_timeout = cfdata->suspend_timeout;
	if (cfdata->off_slider)
	  e_widget_slider_value_double_set(cfdata->off_slider,
					   cfdata->off_timeout);
     }
   if (cfdata->suspend_timeout < cfdata->standby_timeout)
     {
	cfdata->standby_timeout = cfdata->suspend_timeout;
	if (cfdata->standby_slider)
	  e_widget_slider_value_double_set(cfdata->standby_slider,
					   cfdata->standby_timeout);
     }
}

static void
_cb_off_slider_change(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   /* off-slider */
   if (cfdata->off_timeout < cfdata->suspend_timeout)
     {
	cfdata->suspend_timeout = cfdata->off_timeout;
	if (cfdata->suspend_slider)
	  e_widget_slider_value_double_set(cfdata->suspend_slider,
					   cfdata->suspend_timeout);

	if (cfdata->suspend_timeout < cfdata->standby_timeout)
	  {
	     cfdata->standby_timeout = cfdata->suspend_timeout;
	     if (cfdata->standby_slider)
	       e_widget_slider_value_double_set(cfdata->standby_slider,
						cfdata->standby_timeout);
	  }
     }
}

/*!
 * @param data A Eina_List of Evas_Object to chain widgets together with the checkbox
 * @param obj A Evas_Object checkbox created with e_widget_check_add()
 */
static void
_cb_disable_changed(void *data, Evas_Object *obj)
{
   _list_disabled_state_apply(data, !e_widget_check_checked_get(obj));
}

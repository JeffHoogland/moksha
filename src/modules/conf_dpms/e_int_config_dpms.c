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
};

static E_Dialog *dpms_dialog = NULL;

static void
_cb_dpms_dialog_ok(void *data, E_Dialog *dia)
{
   e_object_del(E_OBJECT(dpms_dialog));
   dpms_dialog = NULL;
}

static int
_e_int_config_dpms_capable(void)
{
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
   return 0;
}

static int
_e_int_config_dpms_available(void)
{
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
}

EAPI E_Config_Dialog *
e_int_config_dpms(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if ((e_config_dialog_find("E", "_config_dpms_dialog")) ||
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
			     "_config_dpms_dialog", "preferences-system-power-management", 
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
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return;

   cfdata->standby_slider=NULL;
   cfdata->suspend_slider=NULL;
   cfdata->off_slider=NULL;

   E_FREE(cfdata);
}

static int
_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   e_config->dpms_enable = cfdata->enable_dpms;
   e_config->dpms_standby_enable = cfdata->enable_standby;
   e_config->dpms_suspend_enable = cfdata->enable_suspend;
   e_config->dpms_off_enable = cfdata->enable_off;   

   e_config->dpms_standby_timeout = cfdata->standby_timeout * 60;
   e_config->dpms_suspend_timeout = cfdata->suspend_timeout * 60;
   e_config->dpms_off_timeout = cfdata->off_timeout * 60;

   e_config_save_queue();
   e_dpms_init();
   return 1;
}

/* advanced window */
static int
_advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return (e_config->dpms_enable != cfdata->enable_dpms) ||
	  (e_config->dpms_standby_enable != cfdata->enable_standby) ||
	  (e_config->dpms_suspend_enable != cfdata->enable_suspend) ||
	  (e_config->dpms_off_enable != cfdata->enable_off) ||
	  (e_config->dpms_standby_timeout / 60 != cfdata->standby_timeout) ||
	  (e_config->dpms_suspend_timeout / 60 != cfdata->suspend_timeout) ||
	  (e_config->dpms_off_timeout / 60 != cfdata->off_timeout);
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   _apply_data(cfd, cfdata);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;

   o = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Enable Display Power Management"), 
			   &(cfdata->enable_dpms));
   e_widget_list_object_append(o, ob, 1, 1, 0);   
   
   of = e_widget_framelist_add(evas, _("Timers"), 0);

   ob = e_widget_check_add(evas, _("Standby time"), &(cfdata->enable_standby));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0, 
			    &(cfdata->standby_timeout), NULL, 200);
   e_widget_on_change_hook_set(ob, _cb_standby_slider_change, cfdata);   
   cfdata->standby_slider = ob;
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Suspend time"), &(cfdata->enable_suspend));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0, 
			    &(cfdata->suspend_timeout), NULL, 200);
   e_widget_on_change_hook_set(ob, _cb_suspend_slider_change, cfdata);
   cfdata->suspend_slider = ob;
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Off time"), &(cfdata->enable_off));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0, 
			    &(cfdata->off_timeout), NULL, 200);
   e_widget_on_change_hook_set(ob, _cb_off_slider_change, cfdata);
   cfdata->off_slider = ob;
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;   
}

/* general functionality/callbacks */
static void
_cb_standby_slider_change(void *data, Evas_Object *obj)
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
_cb_suspend_slider_change(void *data, Evas_Object *obj)
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
_cb_off_slider_change(void *data, Evas_Object *obj)
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

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   /*- BASIC -*/
   int mode;
   /*- ADVANCED -*/
   int focus_policy;
   int focus_setting;
   int pass_click_on;
   int always_click_to_raise;
   int always_click_to_focus;
   int focus_last_focused_per_desktop;
   int focus_revert_on_hide_or_close;
   int pointer_slide;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_focus(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_focus_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* methods */
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con,
			     _("Focus Settings"),
			     "E", "_config_focus_dialog",
			     "enlightenment/focus", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->focus_policy = e_config->focus_policy;
   cfdata->focus_setting = e_config->focus_setting;
   cfdata->pass_click_on = e_config->pass_click_on;
   cfdata->always_click_to_raise = e_config->always_click_to_raise;
   cfdata->always_click_to_focus = e_config->always_click_to_focus;
   cfdata->focus_last_focused_per_desktop = e_config->focus_last_focused_per_desktop;
   cfdata->focus_revert_on_hide_or_close = e_config->focus_revert_on_hide_or_close;
   cfdata->pointer_slide = e_config->pointer_slide;

   cfdata->mode = cfdata->focus_policy;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   E_FREE(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_border_button_bindings_ungrab_all();
   if (cfdata->mode == E_FOCUS_CLICK)
     {
	e_config->focus_policy = E_FOCUS_CLICK;
	e_config->focus_setting = E_FOCUS_NEW_WINDOW;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
	e_config->focus_last_focused_per_desktop = 1;
	e_config->focus_revert_on_hide_or_close = 1;
	e_config->pointer_slide = 0;
     }
   else if (cfdata->mode == E_FOCUS_MOUSE)
     {
	e_config->focus_policy = E_FOCUS_MOUSE;
	e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
	e_config->focus_last_focused_per_desktop = 0;
	e_config->focus_revert_on_hide_or_close = 0;
	e_config->pointer_slide = 1;
     }
   else
     {
	e_config->focus_policy = E_FOCUS_SLOPPY;
	e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
	e_config->focus_last_focused_per_desktop = 1;
	e_config->focus_revert_on_hide_or_close = 1;
	e_config->pointer_slide = 1;
     }
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_border_button_bindings_ungrab_all();
   e_config->focus_policy = cfdata->focus_policy;
   e_config->focus_setting = cfdata->focus_setting;
   e_config->pass_click_on = cfdata->pass_click_on;
   e_config->always_click_to_raise = cfdata->always_click_to_raise;
   e_config->always_click_to_focus = cfdata->always_click_to_focus;
   e_config->focus_last_focused_per_desktop = cfdata->focus_last_focused_per_desktop;
   e_config->focus_revert_on_hide_or_close = cfdata->focus_revert_on_hide_or_close;
   e_config->pointer_slide = cfdata->pointer_slide;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ob;
   E_Radio_Group *rg;
      
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->mode));
   ob = e_widget_radio_add(evas, _("Click Window to Focus"), E_FOCUS_CLICK, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Window under the Mouse"), E_FOCUS_MOUSE, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Most recent Window under the Mouse"), E_FOCUS_SLOPPY, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
      
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Focus"), 0);
   rg = e_widget_radio_group_new(&(cfdata->focus_policy));
   ob = e_widget_radio_add(evas, _("Click to focus"), E_FOCUS_CLICK, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Pointer focus"), E_FOCUS_MOUSE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Sloppy focus"), E_FOCUS_SLOPPY, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("New Window Focus"), 0);
   rg = e_widget_radio_group_new(&(cfdata->focus_setting));
   ob = e_widget_radio_add(evas, _("No new windows get focus"), E_FOCUS_NONE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("All new windows get focus"), E_FOCUS_NEW_WINDOW, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Only new dialogs get focus"), E_FOCUS_NEW_DIALOG, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Only new dialogs get focus if the parent has focus"), E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Other Settings"), 0);
   ob = e_widget_check_add(evas, _("Always pass on caught click events to programs"), &(cfdata->pass_click_on));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("A click on a window always raises it"), &(cfdata->always_click_to_raise));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("A click in a window always focuses it"), &(cfdata->always_click_to_focus));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Refocus last focused window on desktop switch"), &(cfdata->focus_last_focused_per_desktop));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Revert focus when hiding or closing a window"), &(cfdata->focus_revert_on_hide_or_close));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Slide pointer to a new focused window"), &(cfdata->pointer_slide));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

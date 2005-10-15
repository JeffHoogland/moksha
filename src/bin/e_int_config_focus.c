/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
typedef struct _CFData CFData;

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _CFData
{
   /*- BASIC -*/
   int mode;
   /*- ADVANCED -*/
   int focus_policy;
   int focus_setting;
   int pass_click_on;
   int always_click_to_raise;
   int always_click_to_focus;
};

/* a nice easy setup function that does the dirty work */
void
e_int_config_focus(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   /* methods */
   v.create_cfdata           = _create_data;
   v.free_cfdata             = _free_data;
   v.basic.apply_cfdata      = _basic_apply_data;
   v.basic.create_widgets    = _basic_create_widgets;
   v.advanced.apply_cfdata   = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Focus Settings"), NULL, 0, &v, NULL);
}

/**--CREATE--**/
static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   cfdata->focus_policy = e_config->focus_policy;
   cfdata->focus_setting = e_config->focus_setting;
   cfdata->pass_click_on = e_config->pass_click_on;
   cfdata->always_click_to_raise = e_config->always_click_to_raise;
   cfdata->always_click_to_focus = e_config->always_click_to_focus;

   cfdata->mode = cfdata->focus_policy;
   
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   /* Free the cfdata */
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
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
     }
   else if (cfdata->mode == E_FOCUS_MOUSE)
     {
	e_config->focus_policy = E_FOCUS_MOUSE;
	e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
     }
   else
     {
	e_config->focus_policy = E_FOCUS_SLOPPY;
	e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
     }
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_border_button_bindings_ungrab_all();
   e_config->focus_policy = cfdata->focus_policy;
   e_config->focus_setting = cfdata->focus_setting;
   e_config->pass_click_on = cfdata->pass_click_on;
   e_config->always_click_to_raise = cfdata->always_click_to_raise;
   e_config->always_click_to_focus = cfdata->always_click_to_focus;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
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
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata)
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
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define MD_CLICK 0
#define MD_MOUSE 1
#define MD_SLOPPY 2
typedef struct _A_CFData {
   int focus_policy;
   int focus_setting;
   int pass_click_on;
   int always_click_to_raise;
   int always_click_to_focus;
} A_CFData;
typedef struct _B_CFData {
   int mode;
   A_CFData advanced;
} B_CFData;

/*** BEGIN template ***/
static void *_b_create_data(void *cfdata_other, E_Config_Dialog_CFData_Type type_other);
static void _b_free_data(B_CFData *cfdata);
static void _b_apply_data(B_CFData *cfdata);
static Evas_Object *_b_create_widgets(Evas *evas, B_CFData *cfdata);
static void *_a_create_data(void *cfdata_other, E_Config_Dialog_CFData_Type type_other);
static void _a_free_data(A_CFData *cfdata);
static void _a_apply_data(A_CFData *cfdata);
static Evas_Object *_a_create_widgets(Evas *evas, A_CFData *cfdata);

void e_int_config_focus(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View a, b;
   
   b.create_cfdata  = _b_create_data;
   b.free_cfdata    = _b_free_data;
   b.apply_cfdata   = _b_apply_data;
   b.create_widgets = _b_create_widgets;
   a.create_cfdata  = _a_create_data;
   a.free_cfdata    = _a_free_data;
   a.apply_cfdata   = _a_apply_data;
   a.create_widgets = _a_create_widgets;
   cfd = e_config_dialog_new(con, _("Focus Settings"), &b, &a);
}
/*** END template ***/

static void *_b_create_data(void *cfdata_other, E_Config_Dialog_CFData_Type type_other) {
   B_CFData *cfdata;
   cfdata = E_NEW(B_CFData, 1);
   
   cfdata->advanced.focus_policy = e_config->focus_policy;
   cfdata->advanced.focus_setting = e_config->focus_setting;
   cfdata->advanced.pass_click_on = e_config->pass_click_on;
   cfdata->advanced.always_click_to_raise = e_config->always_click_to_raise;
   cfdata->advanced.always_click_to_focus = e_config->always_click_to_focus;
   
   if (cfdata->advanced.focus_policy == E_FOCUS_CLICK)
     cfdata->mode = MD_CLICK;
   else if (cfdata->advanced.focus_policy == E_FOCUS_MOUSE)
     cfdata->mode = MD_MOUSE;
   else
     cfdata->mode = MD_SLOPPY;
   return cfdata;
}
static void _b_free_data(B_CFData *cfdata) {
   free(cfdata);
}
static void _b_apply_data(B_CFData *cfdata) {
   e_border_button_bindings_ungrab_all();
   if (cfdata->mode == MD_CLICK)
     {
	e_config->focus_policy = E_FOCUS_CLICK;
	e_config->focus_setting = E_FOCUS_NEW_WINDOW;
	e_config->pass_click_on = 1;
	e_config->always_click_to_raise = 0;
	e_config->always_click_to_focus = 0;
     }
   else if (cfdata->mode == MD_MOUSE)
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
}
static Evas_Object *_b_create_widgets(Evas *evas, B_CFData *cfdata) {
   Evas_Object *o, *ob;
   E_Radio_Group *rg;
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->mode));
   ob = e_widget_radio_add(evas, _("Click Window to Focus"), MD_CLICK, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Window under the Mouse"), MD_MOUSE, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Most recent Window under the Mouse"), MD_SLOPPY, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   return o;
}

static void *_a_create_data(void *cfdata_other, E_Config_Dialog_CFData_Type type_other) {
   A_CFData *cfdata;
   cfdata = E_NEW(A_CFData, 1);
   
   cfdata->focus_policy = e_config->focus_policy;
   cfdata->focus_setting = e_config->focus_setting;
   cfdata->pass_click_on = e_config->pass_click_on;
   cfdata->always_click_to_raise = e_config->always_click_to_raise;
   cfdata->always_click_to_focus = e_config->always_click_to_focus;
   
   return cfdata;
}
static void _a_free_data(A_CFData *cfdata) {
   free(cfdata);
}
static void _a_apply_data(A_CFData *cfdata) {
   e_border_button_bindings_ungrab_all();
   e_config->focus_policy = cfdata->focus_policy;
   e_config->focus_setting = cfdata->focus_setting;
   e_config->pass_click_on = cfdata->pass_click_on;
   e_config->always_click_to_raise = cfdata->always_click_to_raise;
   e_config->always_click_to_focus = cfdata->always_click_to_focus;
   e_border_button_bindings_grab_all();
   e_config_save_queue();
}
static Evas_Object *_a_create_widgets(Evas *evas, A_CFData *cfdata) {
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

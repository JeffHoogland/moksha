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
   int move_resize_info;
   int animate_shading;
   int window_placement_policy;
   int desk_auto_switch;

   /*- ADVANCED -*/
   int move_info_visible;
   int move_info_follows;
   int resize_info_visible;
   int resize_info_follows;
   int border_shade_animate;
   int border_shade_transition;
   double border_shade_speed;
   int use_app_icon;
   int remember_internal_windows;
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_window_display(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_window_display_dialog")) return NULL;
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
			     _("Window Display"),
			     "E", "_config_window_display_dialog",
			     "enlightenment/windows", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->window_placement_policy = e_config->window_placement_policy;
   cfdata->move_info_visible = e_config->move_info_visible;
   cfdata->move_info_follows = e_config->move_info_follows;
   cfdata->resize_info_visible = e_config->resize_info_visible;
   cfdata->resize_info_follows = e_config->resize_info_follows;
   cfdata->border_shade_animate = e_config->border_shade_animate;
   cfdata->border_shade_transition = e_config->border_shade_transition;
   cfdata->border_shade_speed = e_config->border_shade_speed;
   if (cfdata->move_info_visible &&
       cfdata->resize_info_visible) cfdata->move_resize_info = 1;
   if (cfdata->border_shade_animate) cfdata->animate_shading = 1;
   cfdata->use_app_icon = e_config->use_app_icon;
   cfdata->remember_internal_windows = e_config->remember_internal_windows;
   cfdata->desk_auto_switch = e_config->desk_auto_switch;
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
   if (cfdata->move_resize_info)
     {
	e_config->move_info_visible = 1;
	e_config->resize_info_visible = 1;
	cfdata->move_info_visible = 1;
	cfdata->resize_info_visible = 1;
     }
   else
     {
	e_config->move_info_visible = 0;
	e_config->resize_info_visible = 0;
	cfdata->move_info_visible = 0;
	cfdata->resize_info_visible = 0;
     }
   e_config->window_placement_policy = cfdata->window_placement_policy;
   e_config->border_shade_animate = cfdata->animate_shading;
   e_config->desk_auto_switch = cfdata->desk_auto_switch;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_config->window_placement_policy = cfdata->window_placement_policy;
   e_config->move_info_visible = cfdata->move_info_visible;
   e_config->move_info_follows = cfdata->move_info_follows;
   e_config->resize_info_visible = cfdata->resize_info_visible;
   e_config->resize_info_follows = cfdata->resize_info_follows;
   e_config->border_shade_animate = cfdata->border_shade_animate;
   e_config->border_shade_transition = cfdata->border_shade_transition;
   e_config->border_shade_speed = cfdata->border_shade_speed;
   e_config->use_app_icon = cfdata->use_app_icon;
   e_config->remember_internal_windows = cfdata->remember_internal_windows;
   e_config->desk_auto_switch = cfdata->desk_auto_switch;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;

   if (cfdata->move_info_visible && cfdata->resize_info_visible)
     cfdata->move_resize_info = 1;
   else
     cfdata->move_resize_info = 0;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Display"), 0);
   ob = e_widget_check_add(evas, _("Show window geometry information when moving or resizing"), &(cfdata->move_resize_info));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Animate the shading and unshading of windows"), &(cfdata->animate_shading));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Automatic New Window Placement"), 0);
   rg = e_widget_radio_group_new(&(cfdata->window_placement_policy));
   ob = e_widget_radio_add(evas, _("Smart Placement"), E_WINDOW_PLACEMENT_SMART, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Don't hide Gadgets"), E_WINDOW_PLACEMENT_ANTIGADGET, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Place at mouse pointer"), E_WINDOW_PLACEMENT_CURSOR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Place manually with the mouse"), E_WINDOW_PLACEMENT_MANUAL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Automatically switch to desktop of new window"), &(cfdata->desk_auto_switch));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *ob, *of, *ot;
   E_Radio_Group *rg;

   ot = e_widget_table_add(evas, 0);

   of = e_widget_framelist_add(evas, _("Window Move Geometry"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_check_add(evas, _("Display information"), &(cfdata->move_info_visible));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Follow the window as it moves"), &(cfdata->move_info_follows));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Window Resize Geometry"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_check_add(evas, _("Display information"), &(cfdata->resize_info_visible));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Follow the window as it resizes"), &(cfdata->resize_info_follows));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Window Shading"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_check_add(evas, _("Animate the shading and unshading of windows"), &(cfdata->border_shade_animate));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%4.0f pixels/sec"), 100, 9900, 100, 0, &(cfdata->border_shade_speed), NULL, 150);
   e_widget_framelist_object_append(of, ob);
   rg = e_widget_radio_group_new(&(cfdata->border_shade_transition));
   ob = e_widget_radio_add(evas, _("Linear"), E_TRANSITION_LINEAR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Smooth accelerate and decelerate"), E_TRANSITION_SINUSOIDAL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Accelerate"), E_TRANSITION_ACCELERATE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Decelerate"), E_TRANSITION_DECELERATE, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Automatic New Window Placement"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->window_placement_policy));
   ob = e_widget_radio_add(evas, _("Smart Placement"), E_WINDOW_PLACEMENT_SMART, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Don't hide Gadgets"), E_WINDOW_PLACEMENT_ANTIGADGET, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Place at mouse pointer"), E_WINDOW_PLACEMENT_CURSOR, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Place manually with the mouse"), E_WINDOW_PLACEMENT_MANUAL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Automatically switch to desktop of new window"), &(cfdata->desk_auto_switch));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Window Border"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->use_app_icon));
   ob = e_widget_radio_add(evas, _("Prefer user defined icon"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Prefer application provided icon"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Internal Windows"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_check_add(evas, _("Always remember internal windows"), &(cfdata->remember_internal_windows));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 2, 1, 1, 1, 1, 1, 1);

   return ot;
}

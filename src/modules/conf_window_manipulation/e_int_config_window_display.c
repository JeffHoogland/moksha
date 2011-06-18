#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_disable_check(void *data, Evas_Object *obj);
static void _cb_disable_check_list(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   int move_info_visible;
   int move_info_follows;
   int resize_info_visible;
   int resize_info_follows;
   int border_shade_animate;
   int border_shade_transition;
   double border_shade_speed;
   int use_app_icon;
   int window_placement_policy;
   int desk_auto_switch;

   Eina_List *shading_list;
};

/* a nice easy setup function that does the dirty work */
E_Config_Dialog *
e_int_config_window_display(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/window_display")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Window Display"),
			     "E", "windows/window_display",
			     "preferences-system-windows", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->move_info_visible = e_config->move_info_visible;
   cfdata->move_info_follows = e_config->move_info_follows;
   cfdata->resize_info_visible = e_config->resize_info_visible;
   cfdata->resize_info_follows = e_config->resize_info_follows;
   cfdata->use_app_icon = e_config->use_app_icon;

   cfdata->window_placement_policy = 
     e_config->window_placement_policy;
   cfdata->desk_auto_switch = e_config->desk_auto_switch;

   cfdata->border_shade_animate = e_config->border_shade_animate;
   cfdata->border_shade_transition = e_config->border_shade_transition;
   cfdata->border_shade_speed = e_config->border_shade_speed;

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->shading_list);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->window_placement_policy = cfdata->window_placement_policy;
   e_config->move_info_visible = cfdata->move_info_visible;
   e_config->move_info_follows = cfdata->move_info_follows;
   e_config->resize_info_visible = cfdata->resize_info_visible;
   e_config->resize_info_follows = cfdata->resize_info_follows;
   e_config->border_shade_animate = cfdata->border_shade_animate;
   e_config->border_shade_transition = cfdata->border_shade_transition;
   e_config->border_shade_speed = cfdata->border_shade_speed;
   e_config->use_app_icon = cfdata->use_app_icon;

   e_config->desk_auto_switch = cfdata->desk_auto_switch;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->window_placement_policy != cfdata->window_placement_policy) ||
	   (e_config->move_info_visible != cfdata->move_info_visible) ||
	   (e_config->move_info_follows != cfdata->move_info_follows) ||
	   (e_config->resize_info_visible != cfdata->resize_info_visible) ||
	   (e_config->resize_info_follows != cfdata->resize_info_follows) ||
	   (e_config->border_shade_animate != cfdata->border_shade_animate) ||
	   (e_config->border_shade_transition != cfdata->border_shade_transition) ||
	   (e_config->border_shade_speed != cfdata->border_shade_speed) ||
	   (e_config->use_app_icon != cfdata->use_app_icon) ||
	   (e_config->desk_auto_switch != cfdata->desk_auto_switch));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *of, *ow, *oc;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Display */
   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Move Geometry"), 0);
   oc = e_widget_check_add(evas, _("Display information"), 
                           &(cfdata->move_info_visible));
   e_widget_framelist_object_append(of, oc);
   ow = e_widget_check_add(evas, _("Follows the window"), 
                           &(cfdata->move_info_follows));
   e_widget_disabled_set(ow, !cfdata->move_info_visible);
   e_widget_on_change_hook_set(oc, _cb_disable_check, ow);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Resize Geometry"), 0);
   oc = e_widget_check_add(evas, _("Display information"), 
                           &(cfdata->resize_info_visible));
   e_widget_framelist_object_append(of, oc);
   ow = e_widget_check_add(evas, _("Follows the window"), 
                           &(cfdata->resize_info_follows));
   e_widget_disabled_set(ow, !cfdata->resize_info_visible);
   e_widget_on_change_hook_set(oc, _cb_disable_check, ow);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Display"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   /* Border Icon */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->use_app_icon));
   ow = e_widget_radio_add(evas, _("User defined"), 0, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Application provided"), 1, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Border Icon"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   /* New Windows */
   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Placement"), 0);
   rg = e_widget_radio_group_new(&(cfdata->window_placement_policy));
   ow = e_widget_radio_add(evas, _("Smart Placement"), 
                           E_WINDOW_PLACEMENT_SMART, rg);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Don't hide Gadgets"), 
                           E_WINDOW_PLACEMENT_ANTIGADGET, rg);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Place at mouse pointer"), 
                           E_WINDOW_PLACEMENT_CURSOR, rg);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Place manually with the mouse"), 
                           E_WINDOW_PLACEMENT_MANUAL, rg);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   ow = e_widget_check_add(evas, _("Switch to desktop of new window"), 
                           &(cfdata->desk_auto_switch));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("New Windows"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   /* Shading */
   ol = e_widget_list_add(evas, 0, 0);
   oc = e_widget_check_add(evas, _("Animate"), 
                           &(cfdata->border_shade_animate));
   e_widget_list_object_append(ol, oc, 1, 1, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%4.0f pixels/sec"), 
                            100, 9900, 100, 0, 
                            &(cfdata->border_shade_speed), NULL, 100);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);

   rg = e_widget_radio_group_new(&(cfdata->border_shade_transition));
   
   ow = e_widget_radio_add(evas, _("Linear"), E_TRANSITION_LINEAR, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Accelerate, then decelerate"), E_TRANSITION_SINUSOIDAL, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Accelerate"), E_TRANSITION_ACCELERATE, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Decelerate"), E_TRANSITION_DECELERATE, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Pronounced Accelerate"), E_TRANSITION_ACCELERATE_LOTS, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Pronounced Decelerate"), E_TRANSITION_DECELERATE_LOTS, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Pronounced Acceleratem then decelerate"), E_TRANSITION_SINUSOIDAL_LOTS, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Bounce"), E_TRANSITION_BOUNCE, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   ow = e_widget_radio_add(evas, _("Bounce more"), E_TRANSITION_BOUNCE_LOTS, rg);
   e_widget_disabled_set(ow, !cfdata->border_shade_animate);
   cfdata->shading_list = eina_list_append(cfdata->shading_list, ow);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Shading"), ol, 
                                 0, 0, 1, 0, 0.5, 0.0);

   e_widget_on_change_hook_set(oc, _cb_disable_check_list, 
                               cfdata->shading_list);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static void
_cb_disable_check(void *data, Evas_Object *obj)
{
   e_widget_disabled_set(data, !e_widget_check_checked_get(obj));
}

static void
_cb_disable_check_list(void *data, Evas_Object *obj)
{
   Eina_List *list = data;
   const Eina_List *l;
   Evas_Object *o;
   Eina_Bool disable = !e_widget_check_checked_get(obj);

   EINA_LIST_FOREACH(list, l, o)
      e_widget_disabled_set(o, disable);
}

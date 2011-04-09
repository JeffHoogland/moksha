#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   int mouse_hand;
   double numerator;
   double denominator;
   double threshold;
};

E_Config_Dialog *
e_int_config_mouse(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "keyboard_and_mouse/mouse_settings"))
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Mouse Settings"), "E",
			     "keyboard_and_mouse/mouse_settings",
			     "preferences-desktop-mouse", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->mouse_hand = e_config->mouse_hand;
   cfdata->numerator = e_config->mouse_accel_numerator;
   cfdata->denominator = e_config->mouse_accel_denominator;
   cfdata->threshold = e_config->mouse_accel_threshold;
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

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return !((cfdata->mouse_hand == e_config->mouse_hand) &&
	    (cfdata->numerator == e_config->mouse_accel_numerator) &&
	    (cfdata->denominator == e_config->mouse_accel_denominator) &&
	    (cfdata->threshold == e_config->mouse_accel_threshold));
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

/* advanced window */
static int
_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->mouse_hand = cfdata->mouse_hand;
   e_config->mouse_accel_numerator = cfdata->numerator;
   e_config->mouse_accel_denominator = cfdata->denominator;
   e_config->mouse_accel_threshold = cfdata->threshold;

   /* Apply the above settings */
   e_mouse_update();

   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_frametable_add(evas, _("Mouse Hand"), 0);
   rg = e_widget_radio_group_new(&(cfdata->mouse_hand));
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-desktop-mouse-right", 48, 48, E_MOUSE_HAND_LEFT, rg);
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   ob = e_widget_radio_icon_add(evas, NULL, "preferences-desktop-mouse-left", 48, 48, E_MOUSE_HAND_RIGHT, rg);
   e_widget_frametable_object_append(of, ob, 1, 0, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Mouse Acceleration"), 0);

   ob = e_widget_label_add(evas, _("Acceleration"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 1.0, 10.0, 1.0, 0,
			    &(cfdata->numerator), NULL, 100);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Threshold"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 1.0, 10.0, 1.0, 0,
			    &(cfdata->threshold), NULL, 100);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

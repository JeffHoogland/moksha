#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_min_changed(void *data, Evas_Object *obj);
static void _cb_max_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   struct {
        Eina_List *rmin;
        Eina_List *rmax;
   } gui;
   double powersave_none;
   double powersave_low;
   double powersave_medium;
   double powersave_high;
   double powersave_extreme;
   E_Powersave_Mode powersave_min;
   E_Powersave_Mode powersave_max;
};

E_Config_Dialog *
e_int_config_powermanagement(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "advanced/powermanagement")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Power Management Settings"),
			     "E", "advanced/powermanagement",
			     "preferences-system-power-management", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->powersave_none = e_config->powersave.none;
   cfdata->powersave_low = e_config->powersave.low;
   cfdata->powersave_medium = e_config->powersave.medium;
   cfdata->powersave_high = e_config->powersave.high;
   cfdata->powersave_extreme = e_config->powersave.extreme;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->gui.rmin);
   eina_list_free(cfdata->gui.rmax);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->powersave.none = cfdata->powersave_none;
   e_config->powersave.low = cfdata->powersave_low;
   e_config->powersave.medium = cfdata->powersave_medium;
   e_config->powersave.high = cfdata->powersave_high;
   e_config->powersave.extreme = cfdata->powersave_extreme;
   if (e_config->powersave.min != cfdata->powersave_min
       || e_config->powersave.max != cfdata->powersave_max)
     {
        e_config->powersave.min = cfdata->powersave_min;
        e_config->powersave.max = cfdata->powersave_max;
        ecore_event_add(E_EVENT_POWERSAVE_CONFIG_UPDATE,
                        NULL, NULL, NULL);
     }

   e_powersave_mode_set(e_powersave_mode_get());
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->powersave.min != cfdata->powersave_min) ||
           (e_config->powersave.max != cfdata->powersave_max) ||
           (e_config->powersave.none != cfdata->powersave_none) ||
           (e_config->powersave.low != cfdata->powersave_low) ||
           (e_config->powersave.medium != cfdata->powersave_medium) ||
           (e_config->powersave.high != cfdata->powersave_high) ||
           (e_config->powersave.extreme != cfdata->powersave_extreme));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *ol;
   E_Radio_Group *rmin, *rmax;
   int y;
   
   cfdata->powersave_min = e_config->powersave.min;
   cfdata->powersave_max = e_config->powersave.max;
   rmin = e_widget_radio_group_new((int*) &(cfdata->powersave_min));
   rmax = e_widget_radio_group_new((int*) &(cfdata->powersave_max));

   ol = e_widget_table_add(evas, 0);
   
   y = 0;
   ob = e_widget_label_add(evas,
                           _("Levels Allowed"));
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      2, 1,    //span
                                      1, 1,    //fill
                                      1, 1,    //expand
                                      0.5, 0.5 //align
                                      );
   ob = e_widget_label_add(evas,
                           _("Time to defer power-hungry tasks"));
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      1, 1,    //expand
                                      0.5, 0.5 //align
                                      );

   
   y++;
   ob = e_widget_label_add(evas, _("Min"));
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.5, 0.5 //align
                                      );
   ob = e_widget_label_add(evas, _("Max"));
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.5, 0.5 //align
                                      );
   ob = e_widget_label_add(evas,
                           _("Level"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      1, 1,    //expand
                                      0.5, 0.5 //align
                                      );
   ob = e_widget_label_add(evas,
                           _("e.g. Saving to disk"));
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      1, 1,    //expand
                                      0.5, 0.5 //align
                                      );
   y++;
   
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_NONE, rmin);
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmin = eina_list_append(cfdata->gui.rmin, ob);
   e_widget_on_change_hook_set(ob, _cb_min_changed, cfdata);
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_NONE, rmax);
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmax = eina_list_append(cfdata->gui.rmax, ob);
   e_widget_on_change_hook_set(ob, _cb_max_changed, cfdata);
   ob = e_widget_label_add(evas, _("None"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      0, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f s"), 0.1, 1.0, 0, 0,
                            &(cfdata->powersave_none), NULL, 100);
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      1, 0,    //expand
                                      0.5, 0.5 //align
                                      );
   y++;
   
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_LOW, rmin);
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmin = eina_list_append(cfdata->gui.rmin, ob);
   e_widget_on_change_hook_set(ob, _cb_min_changed, cfdata);
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_LOW, rmax);
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   cfdata->gui.rmax = eina_list_append(cfdata->gui.rmax, ob);
   e_widget_on_change_hook_set(ob, _cb_max_changed, cfdata);
   ob = e_widget_label_add(evas, _("Low"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   ob = e_widget_slider_add(evas, 1, 0, _("%1.2f s"), 1.0, 10.0, 1, 0,
                            &(cfdata->powersave_low), NULL, 100);
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   y++;
   
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_MEDIUM, rmin);
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmin = eina_list_append(cfdata->gui.rmin, ob);
   e_widget_on_change_hook_set(ob, _cb_min_changed, cfdata);
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_MEDIUM, rmax);
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   cfdata->gui.rmax = eina_list_append(cfdata->gui.rmax, ob);
   e_widget_on_change_hook_set(ob, _cb_max_changed, cfdata);
   ob = e_widget_label_add(evas, _("Medium"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   ob = e_widget_slider_add(evas, 1, 0, _("%.0f s"), 10.0, 120.0, 1, 0,
                            &(cfdata->powersave_medium), NULL, 100);
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   y++;
   
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_HIGH, rmin);
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmin = eina_list_append(cfdata->gui.rmin, ob);
   e_widget_on_change_hook_set(ob, _cb_min_changed, cfdata);
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_HIGH, rmax);
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   cfdata->gui.rmax = eina_list_append(cfdata->gui.rmax, ob);
   e_widget_on_change_hook_set(ob, _cb_max_changed, cfdata);
   ob = e_widget_label_add(evas, _("High"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   ob = e_widget_slider_add(evas, 1, 0, _("%.0f s"), 120.0, 1200.0, 1, 0,
                            &(cfdata->powersave_high), NULL, 100);
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   y++;
   
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_EXTREME, rmin);
   e_widget_table_object_align_append(ol, ob,
                                      0, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );
   cfdata->gui.rmin = eina_list_append(cfdata->gui.rmin, ob);
   e_widget_on_change_hook_set(ob, _cb_min_changed, cfdata);
   ob = e_widget_radio_add(evas, "", E_POWERSAVE_MODE_EXTREME, rmax);
   e_widget_table_object_align_append(ol, ob,
                                      1, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   cfdata->gui.rmax = eina_list_append(cfdata->gui.rmax, ob);
   e_widget_on_change_hook_set(ob, _cb_max_changed, cfdata);
   ob = e_widget_label_add(evas, _("Extreme"));
   e_widget_table_object_align_append(ol, ob,
                                      2, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                     );
   ob = e_widget_slider_add(evas, 1, 0, _("%.0f s"), 120.0, 2400.0, 1, 0,
                            &(cfdata->powersave_extreme), NULL, 100);
   e_widget_table_object_align_append(ol, ob,
                                      3, y,    //place
                                      1, 1,    //span
                                      1, 1,    //fill
                                      0, 0,    //expand
                                      0.0, 0.5 //align
                                      );

   return ol;
}


static void
_cb_min_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *o;
   cfdata = data;
   if (!cfdata) return;
   if (cfdata->powersave_max < cfdata->powersave_min)
     {
        o = eina_list_nth(cfdata->gui.rmax, cfdata->powersave_min);
        e_widget_radio_toggle_set(o, EINA_TRUE);
        e_widget_change(o);
     }

}

static void
_cb_max_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Evas_Object *o;

   cfdata = data;
   if (!cfdata) return;
   if (cfdata->powersave_min > cfdata->powersave_max)
     {
        o = eina_list_nth(cfdata->gui.rmin, cfdata->powersave_max);
        e_widget_radio_toggle_set(o, EINA_TRUE);
        e_widget_change(o);
     }
}

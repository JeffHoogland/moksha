#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_RANDR_H
#define E_INT_CONFIG_RANDR_H

#include "e.h"

typedef struct _E_Config_Randr_Dialog_Output_Dialog_Data  E_Config_Randr_Dialog_Output_Dialog_Data;
typedef struct _E_Config_Randr_Dialog_Confirmation_Dialog_Data E_Config_Randr_Dialog_Confirmation_Dialog_Data;
typedef struct _Config       Config;

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   //list of E_Config_Randr_Dialog_Output_Dialog_Data
   Eina_List *output_dialog_data_list;
   E_Manager *manager;
   struct {
        Evas *canvas;
        Evas_Object *dialog, *widget_list;
        E_Config_Randr_Dialog_Output_Dialog_Data *selected_output_dd;
        E_Config_Randr_Dialog_Confirmation_Dialog_Data *confirmation_dialog;
        struct {
             struct {
                  Evas_Object *widget, *scrollframe, *area, *widget_list, *swallowing_edje, *suggestion, *check_display_disconnected_outputs;
                  int suggestion_dist_max, check_val_display_disconnected_outputs;
                  Evas_Coord_Point sel_rep_previous_pos;
                  Eina_Rectangle dummy_geo;
             } arrangement;
             struct {
                  Evas_Object *widget;
                  //Evas_Object *swallowing_edje;
                  Evas_Object *radio_above, *radio_right, *radio_below, *radio_left, *radio_clone, *radio_none;
                  int radio_val;
                  //Evas_Object *current_displays_setup, *current_displays_setup_background, *new_display, *new_display_background;
             } policy;
             struct {
                  Evas_Object *widget;
             } resolution;
             struct {
                  Evas_Object *widget;
                  //Evas_Object *swallowing_edje;
                  Evas_Object *radio_normal, *radio_rot90, *radio_rot180, *radio_rot270, *radio_reflect_horizontal, *radio_reflect_vertical;
                  int radio_val;
             } orientation;
        } widgets;
   } gui;
};

struct _E_Config_Randr_Dialog_Output_Dialog_Data
{
   E_Randr_Crtc_Info *crtc;
   E_Randr_Output_Info *output;
   Ecore_X_Randr_Mode_Info *previous_mode, *new_mode, *preferred_mode;
   Ecore_X_Randr_Orientation previous_orientation, new_orientation;
   Ecore_X_Randr_Output_Policy previous_policy, new_policy;
   Evas_Coord_Point previous_pos, new_pos;
   Evas_Object *bg, *rep;
};

struct _E_Config_Randr_Dialog_Confirmation_Dialog_Data
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dialog;
   Ecore_Timer *timer;
   int countdown;
};

struct _Config
{
   Eina_Bool display_disconnected_outputs;
};

E_Config_Dialog *e_int_config_randr(E_Container *con, const char *params __UNUSED__);

// Functions for the arrangement widget interaction
Eina_Bool    arrangement_widget_create_data(E_Config_Dialog_Data *cfdata);
Evas_Object *arrangement_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    arrangement_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    arrangement_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         arrangement_widget_free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         arrangement_widget_keep_changes(E_Config_Dialog_Data *cfdata);
void         arrangement_widget_discard_changes(E_Config_Dialog_Data *cfdata);
void         arrangement_widget_rep_update(E_Config_Randr_Dialog_Output_Dialog_Data *odd);

// Functions for the policies widget interaction
Eina_Bool    policy_widget_create_data(E_Config_Dialog_Data *cfdata);
Evas_Object *policy_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    policy_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    policy_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         policy_widget_free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         policy_widget_keep_changes(E_Config_Dialog_Data *cfdata);
void         policy_widget_discard_changes(E_Config_Dialog_Data *cfdata);
void         policy_widget_update_radio_buttons(E_Config_Randr_Dialog_Output_Dialog_Data *odd);

// Functions for the resolutions widget interaction
Eina_Bool    resolution_widget_create_data(E_Config_Dialog_Data *cfdata);
Evas_Object *resolution_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    resolution_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    resolution_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         resolution_widget_free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         resolution_widget_keep_changes(E_Config_Dialog_Data *cfdata);
void         resolution_widget_discard_changes(E_Config_Dialog_Data *cfdata);
void         resolution_widget_update_list(E_Config_Randr_Dialog_Output_Dialog_Data *odd);

// Functions for the orientation widget interaction
Eina_Bool    orientation_widget_create_data(E_Config_Dialog_Data *cfdata);
Evas_Object *orientation_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    orientation_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    orientation_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         orientation_widget_free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         orientation_widget_update_radio_buttons(E_Config_Randr_Dialog_Output_Dialog_Data *odd);
void         orientation_widget_update_edje(E_Config_Randr_Dialog_Output_Dialog_Data *odd);
void         orientation_widget_keep_changes(E_Config_Dialog_Data *cfdata);
void         orientation_widget_discard_changes(E_Config_Dialog_Data *cfdata);


#endif
#endif

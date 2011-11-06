#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_RANDR_H
#define E_INT_CONFIG_RANDR_H

#include "e.h"

typedef struct _E_Config_Randr_Dialog_Output_Dialog_Data  E_Config_Randr_Dialog_Output_Dialog_Data;
typedef struct _E_Config_Randr_Dialog_Confirmation_Dialog_Data E_Config_Randr_Dialog_Confirmation_Dialog_Data;

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   //list of E_Config_Randr_Dialog_Output_Dialog_Data
   Eina_List *output_dialog_data_list;
   E_Manager *manager;
   struct {
        Evas_Object *dialog, *widget_list, *selected_eo;
        E_Config_Randr_Dialog_Output_Dialog_Data *selected_output_dd;
        E_Config_Randr_Dialog_Confirmation_Dialog_Data *confirmation_dialog;
        struct {
             struct {
                  Evas_Object *dialog, *swallowing_edje, *smart_parent, *suggestion, *clipper;
                  Evas_Coord_Point previous_pos, relative_zero;
                  int suggestion_dist_min;
             } arrangement;
             struct {
                  Evas_Object *dialog;
                  //Evas_Object *swallowing_edje;
                  Evas_Object *radio_above, *radio_right, *radio_below, *radio_left, *radio_clone, *radio_none;
                  int radio_val;
                  //Evas_Object *current_displays_setup, *current_displays_setup_background, *new_display, *new_display_background;
             } policies;
             struct {
                  Evas_Object *dialog;
             } resolutions;
             struct {
                  Evas_Object *dialog;
                  //Evas_Object *swallowing_edje;
                  Evas_Object *radio_normal, *radio_rot90, *radio_rot180, *radio_rot270, *radio_reflect_horizontal, *radio_reflect_vertical;
                  int radio_val;
             } orientation;
        } subdialogs;
   } gui;
   Ecore_X_Randr_Screen_Size screen_size;

};

struct _E_Config_Randr_Dialog_Output_Dialog_Data
{
   E_Randr_Crtc_Info *crtc;
   E_Randr_Output_Info *output;
   Evas_Coord_Point previous_pos, new_pos;
   Ecore_X_Randr_Mode_Info *previous_mode, *new_mode, *preferred_mode;
   Ecore_X_Randr_Orientation previous_orientation, new_orientation;
   Ecore_X_Randr_Output_Policy previous_policy, new_policy;
   Evas_Object *bg;
};

struct _E_Config_Randr_Dialog_Confirmation_Dialog_Data
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_Data *cfdata;
   E_Dialog *dialog;
   Ecore_Timer *timer;
   int countdown;
};

E_Config_Dialog *e_int_config_randr(E_Container *con, const char *params __UNUSED__);

#endif
#endif

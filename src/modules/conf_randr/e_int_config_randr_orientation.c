#include "e_int_config_randr.h"
#include "e_randr.h"

#ifndef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset          -1
#endif

#define RANDR_DIALOG_ORIENTATION_ALL (ECORE_X_RANDR_ORIENTATION_ROT_0 | ECORE_X_RANDR_ORIENTATION_ROT_90 | ECORE_X_RANDR_ORIENTATION_ROT_180 | ECORE_X_RANDR_ORIENTATION_ROT_270 | ECORE_X_RANDR_ORIENTATION_ROT_270 | ECORE_X_RANDR_ORIENTATION_FLIP_X | ECORE_X_RANDR_ORIENTATION_FLIP_Y)

Eina_Bool    dialog_subdialog_orientation_create_data(E_Config_Dialog_Data *cfdata);
Evas_Object *dialog_subdialog_orientation_basic_create_widgets(Evas *canvas);
Eina_Bool    dialog_subdialog_orientation_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    dialog_subdialog_orientation_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    dialog_subdialog_orientation_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         dialog_subdialog_orientation_update_radio_buttons(Evas_Object *crtc);
void         dialog_subdialog_orientation_update_edje(Evas_Object *crtc);

//static void _dialog_subdialog_orientation_policy_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

/*
   static void
   _dialog_subdialog_orientation_radio_add_callbacks(void)
   {
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_rot270, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_rot180, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_rot90, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add (e_config_runtime_info->gui.subdialogs.orientation.radio_normal, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_orientation_policy_mouse_up_cb, NULL);
   }
 */

Eina_Bool
dialog_subdialog_orientation_create_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   E_Randr_Crtc_Info *ci;
   Eina_List *iter;

   if (!cfdata || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        ci = odd->crtc;
        if (!ci || !ci->current_mode) continue;
        odd->new_orientation = Ecore_X_Randr_Unset;
        odd->previous_orientation = ci->current_orientation;
     }

   return EINA_TRUE;
}

Evas_Object *
dialog_subdialog_orientation_basic_create_widgets(Evas *canvas)
{
   Evas_Object *subdialog;
   E_Radio_Group *rg;
   //char signal[29];

   if (!canvas || !e_config_runtime_info) return NULL;
   if (e_config_runtime_info->gui.subdialogs.orientation.dialog) return e_config_runtime_info->gui.subdialogs.orientation.dialog;

   if (!(subdialog = e_widget_framelist_add(canvas, _("Display Orientation"), EINA_FALSE))) return NULL;

   // Add radio buttons
   if (!(rg = e_widget_radio_group_new(&e_config_runtime_info->gui.subdialogs.orientation.radio_val))) goto _dialog_subdialog_orientation_radio_add_fail;

   //IMPROVABLE: use enum to determine objects via 'switch'-statement
   e_config_runtime_info->gui.subdialogs.orientation.radio_normal = e_widget_radio_add(canvas, _("Normal"), ECORE_X_RANDR_OUTPUT_POLICY_ABOVE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_normal);

   e_config_runtime_info->gui.subdialogs.orientation.radio_rot90 = e_widget_radio_add(canvas, _("Rotated, 90°"), ECORE_X_RANDR_OUTPUT_POLICY_RIGHT, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_rot90);

   e_config_runtime_info->gui.subdialogs.orientation.radio_rot180 = e_widget_radio_add(canvas, _("Rotated, 180°"), ECORE_X_RANDR_OUTPUT_POLICY_BELOW, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_rot180);

   e_config_runtime_info->gui.subdialogs.orientation.radio_rot270 = e_widget_radio_add(canvas, _("Rotated, 270°"), ECORE_X_RANDR_OUTPUT_POLICY_LEFT, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_rot270);

   e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal = e_widget_radio_add(canvas, _("Flipped, horizontally"), ECORE_X_RANDR_OUTPUT_POLICY_CLONE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal);

   e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical = e_widget_radio_add(canvas, _("Flipped, vertically"), ECORE_X_RANDR_OUTPUT_POLICY_NONE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical);

   //_dialog_subdialog_orientation_radio_add_callbacks();

   /*
      // Add orientation demonstration edje
      if (!(e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje = edje_object_add(canvas)))
      goto _dialog_subdialog_orientation_edje_add_fail;
      if (!edje_object_file_set(e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje, _theme_file_path, "e/conf/randr/dialog/subdialog/orientation"))
      goto _dialog_subdialog_orientation_edje_set_fail;

      e_widget_table_object_align_append(subdialog, e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje, 1, 0, 1, 1, 1, 1, 1, 1, 1.0, 1.0);
    */

   //disable widgets, if no CRTC is selected
   dialog_subdialog_orientation_update_radio_buttons(e_config_runtime_info->gui.selected_eo);

   //evas_object_show(e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje);

   return subdialog;

   /*
      _dialog_subdialog_orientation_edje_set_fail:
      evas_object_del(ol);
      evas_object_del(e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje);
      _dialog_subdialog_orientation_edje_add_fail:
      fprintf(stderr, "CONF_RANDR: Couldn't set edj for orientation subdialog!\n");
      evas_object_del(subdialog);
      return NULL;
    */
_dialog_subdialog_orientation_radio_add_fail:
   evas_object_del(subdialog);
   fprintf(stderr, "CONF_RANDR: Could not add radio group!\n");
   return NULL;
}

#if 0
static void
_dialog_subdialog_orientation_policy_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   char signal[40];
   int orientation = ECORE_X_RANDR_ORIENTATION_ROT_0;

   /*
    * IMPROVABLE:
    * "sadly" the evas callbacks are called before radio_val is set to its new
    * value. If that is ever changed, remove the used code below and just use the
    * 1-liner below.
    * snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,%d", e_config_runtime_info->gui.subdialogs.orientation.radio_val);
    */
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_normal) orientation = ECORE_X_RANDR_ORIENTATION_ROT_0;
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_rot90) orientation = ECORE_X_RANDR_ORIENTATION_ROT_90;
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_rot180) orientation = ECORE_X_RANDR_ORIENTATION_ROT_180;
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_rot270) orientation = ECORE_X_RANDR_ORIENTATION_ROT_270;
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal) orientation = ECORE_X_RANDR_ORIENTATION_FLIP_X;
   if (obj == e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical) orientation = ECORE_X_RANDR_ORIENTATION_FLIP_Y;

   snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,%d", orientation);

   edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.orientation.swallowing_edje, signal, "e");

   fprintf(stderr, "CONF_RANDR: mouse button released. Emitted signal to orientation: %s\n", signal);
}

#endif

void
dialog_subdialog_orientation_update_radio_buttons(Evas_Object *crtc)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Orientation supported_oris, ori;

   //disable widgets, if no crtc is selected
   if (!crtc)
     {
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_normal, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot90, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot180, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot270, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical, EINA_TRUE);
        return;
     }

   if (!(output_dialog_data = evas_object_data_get(crtc, "output_info"))) return;

   if (output_dialog_data->crtc)
     {
        //enabled monitor
        supported_oris = output_dialog_data->crtc->orientations;
        ori = output_dialog_data->crtc->current_orientation;
     }
   else
     {
        //disabled monitor
        //assume all orientations are supported
        supported_oris = RANDR_DIALOG_ORIENTATION_ALL;
        ori = ECORE_X_RANDR_ORIENTATION_ROT_0;
     }

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_0)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_normal, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_normal, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_90)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot90, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot90, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_180)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot180, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot180, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_270)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot270, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot270, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_FLIP_X)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_FLIP_Y)
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical, EINA_TRUE);

   //toggle the switch of the currently used orientation
   switch (ori)
     {
      case ECORE_X_RANDR_ORIENTATION_ROT_90:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot90, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_ROT_180:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot180, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_ROT_270:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_rot270, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_FLIP_X:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_horizontal, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_FLIP_Y:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_reflect_vertical, EINA_TRUE);
        break;

      default: //== ECORE_X_RANDR_ORIENTATION_ROT_0:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.orientation.radio_normal, EINA_TRUE);
     }
}

void
dialog_subdialog_orientation_update_edje(Evas_Object *crtc)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Orientation supported_oris, ori;
   char signal[40];

   if (!e_config_runtime_info->gui.selected_eo || !(output_dialog_data = evas_object_data_get(crtc, "output_info"))) return;

   if (output_dialog_data->crtc)
     {
        //enabled monitor
        supported_oris = output_dialog_data->crtc->orientations;
        ori = output_dialog_data->crtc->current_orientation;
     }
   else
     {
        //disabled monitor
        //assume all orientations are supported
        supported_oris = RANDR_DIALOG_ORIENTATION_ALL;
        ori = ECORE_X_RANDR_ORIENTATION_ROT_0;
     }
   //Send signal to the edje, to represent the supported and current set orientation
   snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,supported,%d", supported_oris);
   edje_object_signal_emit(crtc, signal, "e");
   snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,current,%d", ori);
   edje_object_signal_emit(crtc, signal, "e");
}

Eina_Bool
dialog_subdialog_orientation_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   Ecore_X_Randr_Orientation orientation;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   if (!e_config_runtime_info->gui.subdialogs.orientation.dialog || !e_config_runtime_info->gui.selected_eo || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "output_info")) || !output_dialog_data->crtc) return EINA_FALSE;

   orientation = e_config_runtime_info->gui.subdialogs.orientation.radio_val;

   fprintf(stderr, "CONF_RANDR: Change orientation of crtc %x to %d.\n", output_dialog_data->crtc->xid, orientation);

   if (ecore_x_randr_crtc_orientation_set(cfd->con->manager->root, output_dialog_data->crtc->xid, orientation))
     {
        ecore_x_randr_screen_reset(cfd->con->manager->root);
        output_dialog_data->previous_orientation = output_dialog_data->new_orientation;
        output_dialog_data->new_orientation = orientation;
        return EINA_TRUE;
     }
   else
     return EINA_FALSE;
}

Eina_Bool
dialog_subdialog_orientation_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   if (!e_config_runtime_info->gui.subdialogs.orientation.dialog || !e_config_runtime_info->gui.selected_eo || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "output_info"))) return EINA_FALSE;

   return (int)output_dialog_data->previous_orientation != (int)e_config_runtime_info->gui.subdialogs.orientation.radio_val;
}

void
dialog_subdialog_orientation_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || ((int)odd->previous_orientation == Ecore_X_Randr_Unset)) continue;
        odd->previous_orientation = odd->new_orientation;
        odd->new_orientation = Ecore_X_Randr_Unset;
     }
}

void
dialog_subdialog_orientation_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((int)odd->previous_orientation == Ecore_X_Randr_Unset)) continue;
        if (ecore_x_randr_crtc_orientation_set(cfdata->manager->root, odd->crtc->xid, odd->previous_orientation))
          {
             odd->new_orientation = odd->previous_orientation;
             odd->previous_orientation = Ecore_X_Randr_Unset;
             ecore_x_randr_screen_reset(cfdata->manager->root);
          }
     }
}


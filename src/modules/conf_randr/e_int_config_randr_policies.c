#include "e_int_config_randr.h"
#include "e_randr.h"

#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)
#endif

#ifndef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#endif

Evas_Object *dialog_subdialog_policies_basic_create_widgets(Evas *canvas);
Eina_Bool    dialog_subdialog_policies_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    dialog_subdialog_policies_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    dialog_subdialog_policies_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         dialog_subdialog_policies_update_radio_buttons(Evas_Object *crtc);

static void  _dialog_subdialog_policies_policy_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

/*
   static void
   _dialog_subdialog_policies_radio_add_callbacks(void)
   {
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_none, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_clone, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_left, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_below, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_above, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.subdialogs.policies.radio_right, EVAS_CALLBACK_MOUSE_UP, _dialog_subdialog_policies_policy_mouse_up_cb, NULL);
   }
 */

Eina_Bool
dialog_subdialog_policies_create_data(E_Config_Dialog_Data *e_config_runtime_info)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!e_config_runtime_info || !e_config_runtime_info->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH (e_config_runtime_info->output_dialog_data_list, iter, odd)
     {
        E_Randr_Output_Info *oi;
        if (odd->crtc)
          oi = eina_list_data_get(eina_list_last(odd->crtc->outputs));
        else if (odd->output)
          oi = odd->output;
        else continue;
        odd->previous_policy = oi->policy;
        odd->new_policy = Ecore_X_Randr_Unset;
     }

   return EINA_TRUE;
}

Evas_Object *
dialog_subdialog_policies_basic_create_widgets(Evas *canvas)
{
   Evas_Object *subdialog;
   E_Radio_Group *rg;
   //char signal[29];

   if (!canvas || !e_config_runtime_info) return NULL;

   if (e_config_runtime_info->gui.subdialogs.policies.dialog) return e_config_runtime_info->gui.subdialogs.policies.dialog;

   if (!(subdialog = e_widget_framelist_add(canvas, _("Screen attachement policy"), EINA_FALSE))) return NULL;

   // Add radio buttons
   if (!(rg = e_widget_radio_group_new(&e_config_runtime_info->gui.subdialogs.policies.radio_val))) goto _dialog_subdialog_policies_radio_add_fail;

   //IMPROVABLE: use enum to determine objects via 'switch'-statement
   e_config_runtime_info->gui.subdialogs.policies.radio_above = e_widget_radio_add(canvas, _("Above"), ECORE_X_RANDR_OUTPUT_POLICY_ABOVE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_above);

   e_config_runtime_info->gui.subdialogs.policies.radio_right = e_widget_radio_add(canvas, _("Right"), ECORE_X_RANDR_OUTPUT_POLICY_RIGHT, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_right);

   e_config_runtime_info->gui.subdialogs.policies.radio_below = e_widget_radio_add(canvas, _("Below"), ECORE_X_RANDR_OUTPUT_POLICY_BELOW, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_below);

   e_config_runtime_info->gui.subdialogs.policies.radio_left = e_widget_radio_add(canvas, _("Left"), ECORE_X_RANDR_OUTPUT_POLICY_LEFT, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_left);

   e_config_runtime_info->gui.subdialogs.policies.radio_clone = e_widget_radio_add(canvas, _("Clone display content"), ECORE_X_RANDR_OUTPUT_POLICY_CLONE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_clone);

   e_config_runtime_info->gui.subdialogs.policies.radio_none = e_widget_radio_add(canvas, _("No reaction"), ECORE_X_RANDR_OUTPUT_POLICY_NONE, rg);
   e_widget_framelist_object_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.radio_none);

   //_dialog_subdialog_policies_radio_add_callbacks();

   /*
      // Add policies demonstration edje
      if (!(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje = edje_object_add(canvas)))
      {
        goto _dialog_subdialog_policies_edje_add_fail;

      }
      if (!edje_object_file_set(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, _theme_file_path, "e/conf/randr/dialog/subdialog/policies"))
      {
        goto _dialog_subdialog_policies_edje_set_fail;
      }

      e_widget_table_object_align_append(subdialog, e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, 1, 0, 1, 1, 1, 1, 1, 1, 1.0, 1.0);
    */

   /*
      evas_object_show(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje);

      //emit signal to edje so a demonstration can be shown
      snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_randr_screen_info->rrvd_info.randr_info_12->output_policy);
      edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, signal, "e");
      fprintf(stderr, "CONF_RANDR: Initial signal emitted to policy dialog: %s\n", signal);

      //Use theme's background as screen representation
      e_config_runtime_info->gui.subdialogs.policies.new_display = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.subdialogs.policies.new_display, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.subdialogs.policies.new_display_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.subdialogs.policies.new_display_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.subdialogs.policies.new_display, "e.swallow.content", e_config_runtime_info->gui.subdialogs.policies.new_display_background);
      edje_object_part_text_set(e_config_runtime_info->gui.subdialogs.policies.new_display, "e.text.label", _("New display"));
      edje_object_part_swallow(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, "new_display.swallow.content", e_config_runtime_info->gui.subdialogs.policies.new_display);
      //add theme's frame
      //for now use the theme's background for the new display as well
      e_config_runtime_info->gui.subdialogs.policies.current_displays_setup = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.subdialogs.policies.current_displays_setup, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.subdialogs.policies.current_displays_setup_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.subdialogs.policies.current_displays_setup_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.subdialogs.policies.current_displays_setup, "e.swallow.content", e_config_runtime_info->gui.subdialogs.policies.current_displays_setup_background);
      edje_object_part_text_set(e_config_runtime_info->gui.subdialogs.policies.current_displays_setup, "e.text.label", _("Used display"));
      edje_object_part_swallow(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, "current_displays_setup.swallow.content", e_config_runtime_info->gui.subdialogs.policies.current_displays_setup);
    */

   evas_object_show(subdialog);

   return subdialog;

   /*
      _dialog_subdialog_policies_edje_set_fail:
      evas_object_del(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje);
      _dialog_subdialog_policies_edje_add_fail:
      fprintf(stderr, "CONF_RANDR: Couldn't set edj for policies subdialog!\n");
      evas_object_del(subdialog);
      return NULL;
    */
_dialog_subdialog_policies_radio_add_fail:
   evas_object_del(subdialog);
   return NULL;
}

static void
_dialog_subdialog_policies_policy_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   char signal[29];
   int policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

   /*
    * IMPROVABLE:
    * "sadly" the evas callbacks are called before radio_val is set to its new
    * value. If that is ever changed, remove the used code below and just use the
    * 1-liner below.
    * snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_config_runtime_info->gui.subdialogs.policies.radio_val);
    */
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_above) policy = ECORE_X_RANDR_OUTPUT_POLICY_ABOVE;
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_right) policy = ECORE_X_RANDR_OUTPUT_POLICY_RIGHT;
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_below) policy = ECORE_X_RANDR_OUTPUT_POLICY_BELOW;
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_left) policy = ECORE_X_RANDR_OUTPUT_POLICY_LEFT;
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_clone) policy = ECORE_X_RANDR_OUTPUT_POLICY_CLONE;
   if (obj == e_config_runtime_info->gui.subdialogs.policies.radio_none) policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

   snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", policy);

   //edje_object_signal_emit(e_config_runtime_info->gui.subdialogs.policies.swallowing_edje, signal, "e");

   fprintf(stderr, "CONF_RANDR: mouse button released. Emitted signal to policy: %s\n", signal);
}

Eina_Bool
dialog_subdialog_policies_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   if (!e_randr_screen_info || !e_config_runtime_info->gui.selected_output_dd) return EINA_FALSE;

   //policy update
   e_config_runtime_info->gui.selected_output_dd->previous_policy = e_config_runtime_info->gui.selected_output_dd->new_policy;
   e_config_runtime_info->gui.selected_output_dd->new_policy = e_config_runtime_info->gui.subdialogs.policies.radio_val;
   fprintf(stderr, "CONF_RANDR: 'New display attached'-policy set to %d\n", e_config_runtime_info->gui.selected_output_dd->new_policy);

   return EINA_TRUE;
}

Eina_Bool
dialog_subdialog_policies_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!e_randr_screen_info || !cfdata || !cfdata->gui.selected_output_dd) return EINA_FALSE;

   return (int)cfdata->gui.selected_output_dd->previous_policy != (int)cfdata->gui.subdialogs.policies.radio_val;
}

void
dialog_subdialog_policies_update_radio_buttons(Evas_Object *crtc)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   E_Randr_Output_Info *output = NULL;
   Ecore_X_Randr_Output_Policy policy;

   //disable widgets, if no crtc is selected
   if (!crtc || !(output_dialog_data = evas_object_data_get(crtc, "output_info")))
     {
        //Evas_Object *radio_above, *radio_right, *radio_below, *radio_left, *radio_clone, *radio_none;
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_above, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_right, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_below, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_left, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_clone, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_none, EINA_TRUE);
        return;
     }
   else
     {
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_above, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_right, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_below, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_left, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_clone, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.policies.radio_none, EINA_FALSE);
     }

   if (output_dialog_data->crtc && output_dialog_data->crtc->outputs)
     {
        output = (E_Randr_Output_Info *)eina_list_data_get(eina_list_last(output_dialog_data->crtc->outputs));
     }
   else if (output_dialog_data->output)
     {
        output = output_dialog_data->output;
     }

   if (!output) return;
   policy = output->policy;
   e_config_runtime_info->gui.selected_output_dd = output_dialog_data;
   //toggle the switch of the currently used policies
   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_right, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_below, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_left, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_clone, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_none, EINA_TRUE);
        break;

      default: //== ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.subdialogs.policies.radio_above, EINA_TRUE);
     }
}

void
dialog_subdialog_policies_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || ((int)odd->previous_policy == Ecore_X_Randr_Unset)) continue;
        odd->previous_policy = odd->new_policy;
        odd->new_policy = Ecore_X_Randr_Unset;
     }
}

void
dialog_subdialog_policies_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH (cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((int)odd->previous_policy == Ecore_X_Randr_Unset)) continue;
        odd->new_policy = odd->previous_policy;
        odd->previous_policy = Ecore_X_Randr_Unset;
     }
}


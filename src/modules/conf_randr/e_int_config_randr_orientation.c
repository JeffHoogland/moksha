#include "e_int_config_randr.h"
#include "e_randr.h"

#ifndef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset          -1
#endif

static void _orientation_widget_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _orientation_widget_radio_add_callbacks(void);
extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

/*
static const char *_ORIENTATION_STRINGS[] = {
    "Normal",
    "Rotated, 90°",
    "Rotated, 180°",
    "Rotated, 270°",
    "Flipped, Horizontally",
    "Flipped, Vertically" };
    */

Eina_Bool
orientation_widget_create_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        odd->new_orientation = Ecore_X_Randr_Unset;
        odd->previous_orientation = odd->crtc ? odd->crtc->current_orientation : (Ecore_X_Randr_Orientation) Ecore_X_Randr_Unset;
     }

   return EINA_TRUE;
}

void
orientation_widget_free_cfdata(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_reflect_vertical, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_reflect_horizontal, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_rot270, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_rot180, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_rot90, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
   evas_object_event_callback_del(cfdata->gui.widgets.orientation.radio_normal, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb);
}

Evas_Object *
orientation_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget;
   E_Radio_Group *rg;
   //char signal[29];

   if (!canvas || !e_config_runtime_info) return NULL;
   if (e_config_runtime_info->gui.widgets.orientation.widget) return e_config_runtime_info->gui.widgets.orientation.widget;

   if (!(widget = e_widget_framelist_add(canvas, _("Display Orientation"), EINA_FALSE))) return NULL;

   // Add radio buttons
   if (!(rg = e_widget_radio_group_new(&e_config_runtime_info->gui.widgets.orientation.radio_val))) goto _orientation_widget_radio_add_fail;

   //IMPROVABLE: use enum to determine objects via 'switch'-statement
   e_config_runtime_info->gui.widgets.orientation.radio_normal = e_widget_radio_add(canvas, _("Normal"), ECORE_X_RANDR_OUTPUT_POLICY_ABOVE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_normal);

   e_config_runtime_info->gui.widgets.orientation.radio_rot90 = e_widget_radio_add(canvas, _("Rotated, 90°"), ECORE_X_RANDR_OUTPUT_POLICY_RIGHT, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_rot90);

   e_config_runtime_info->gui.widgets.orientation.radio_rot180 = e_widget_radio_add(canvas, _("Rotated, 180°"), ECORE_X_RANDR_OUTPUT_POLICY_BELOW, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_rot180);

   e_config_runtime_info->gui.widgets.orientation.radio_rot270 = e_widget_radio_add(canvas, _("Rotated, 270°"), ECORE_X_RANDR_OUTPUT_POLICY_LEFT, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_rot270);

   e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal = e_widget_radio_add(canvas, _("Flipped, horizontally"), ECORE_X_RANDR_OUTPUT_POLICY_CLONE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal);

   e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical = e_widget_radio_add(canvas, _("Flipped, vertically"), ECORE_X_RANDR_OUTPUT_POLICY_NONE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical);

   _orientation_widget_radio_add_callbacks();

   /*
      // Add orientation demonstration edje
      if (!(e_config_runtime_info->gui.widgets.orientation.swallowing_edje = edje_object_add(canvas)))
      goto _orientation_widget_edje_add_fail;
      if (!edje_object_file_set(e_config_runtime_info->gui.widgets.orientation.swallowing_edje, _theme_file_path, "e/conf/randr/dialog/widget/orientation"))
      goto _orientation_widget_edje_set_fail;

      e_widget_table_object_align_append(widget, e_config_runtime_info->gui.widgets.orientation.swallowing_edje, 1, 0, 1, 1, 1, 1, 1, 1, 1.0, 1.0);
    */

   //disable widgets, if no CRTC is selected
   orientation_widget_update_radio_buttons(e_config_runtime_info->gui.selected_output_dd);

   //evas_object_show(e_config_runtime_info->gui.widgets.orientation.swallowing_edje);

   return widget;

   /*
      _orientation_widget_edje_set_fail:
      evas_object_del(ol);
      evas_object_del(e_config_runtime_info->gui.widgets.orientation.swallowing_edje);
      _orientation_widget_edje_add_fail:
      fprintf(stderr, "CONF_RANDR: Couldn't set edj for orientation widget!\n");
      evas_object_del(widget);
      return NULL;
    */
_orientation_widget_radio_add_fail:
   evas_object_del(widget);
   fprintf(stderr, "CONF_RANDR: Could not add radio group!\n");
   return NULL;
}

static void
_orientation_widget_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   //char signal[40];
   int orientation = Ecore_X_Randr_Unset;

   if (!e_config_runtime_info->gui.selected_output_dd)
     return;

   /*
    * IMPROVABLE:
    * "sadly" the evas callbacks are called before radio_val is set to its new
    * value. If that is ever changed, remove the used code below and just use the
    * 1-liner below.
    * snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,%d", e_config_runtime_info->gui.widgets.orientation.radio_val);
    */
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_normal) orientation = ECORE_X_RANDR_ORIENTATION_ROT_0;
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_rot90) orientation = ECORE_X_RANDR_ORIENTATION_ROT_90;
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_rot180) orientation = ECORE_X_RANDR_ORIENTATION_ROT_180;
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_rot270) orientation = ECORE_X_RANDR_ORIENTATION_ROT_270;
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal) orientation = ECORE_X_RANDR_ORIENTATION_FLIP_X;
   if (obj == e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical) orientation = ECORE_X_RANDR_ORIENTATION_FLIP_Y;

   e_config_runtime_info->gui.selected_output_dd->new_orientation = orientation;

   arrangement_widget_rep_update(e_config_runtime_info->gui.selected_output_dd);

   //edje_object_signal_emit(e_config_runtime_info->gui.widgets.orientation.swallowing_edje, signal, "e");

   //fprintf(stderr, "CONF_RANDR: mouse button released. Emitted signal to orientation: %s\n", signal);
}

void
orientation_widget_update_radio_buttons(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   Ecore_X_Randr_Orientation supported_oris, ori;

   //disable widgets, if no crtc is selected
   if (!odd)
     {
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_normal, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot90, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot180, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot270, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical, EINA_TRUE);
        return;
     }

   if (odd->crtc)
     {
        //enabled monitor
        supported_oris = odd->crtc->orientations;
        ori = (odd->new_orientation != (Ecore_X_Randr_Orientation) Ecore_X_Randr_Unset) ? odd->new_orientation : odd->crtc->current_orientation;
     }
   else
     {
        //disabled monitor
        //assume all orientations are supported
        supported_oris = Ecore_X_Randr_Unset;
        ori = Ecore_X_Randr_Unset;
     }

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_0)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_normal, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_normal, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_90)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot90, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot90, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_180)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot180, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot180, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_ROT_270)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot270, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_rot270, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_FLIP_X)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal, EINA_TRUE);

   if (supported_oris & ECORE_X_RANDR_ORIENTATION_FLIP_Y)
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical, EINA_FALSE);
   else
     e_widget_disabled_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical, EINA_TRUE);

   //toggle the switch of the currently used orientation
   switch (ori)
     {
      case ECORE_X_RANDR_ORIENTATION_ROT_0:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_normal, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_ROT_90:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_rot90, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_ROT_180:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_rot180, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_ROT_270:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_rot270, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_FLIP_X:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal, EINA_TRUE);
        break;

      case ECORE_X_RANDR_ORIENTATION_FLIP_Y:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical, EINA_TRUE);
        break;

      default:
        break;
     }
}

/*
void
orientation_widget_update_edje(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   Ecore_X_Randr_Orientation supported_oris, ori;
   char signal[40];

   if (!odd)
     return;

   if (odd->crtc)
     {
        //enabled monitor
        supported_oris = odd->crtc->orientations;
        ori = odd->crtc->current_orientation;
     }
   else
     {
        //disabled monitor
        //assume all orientations are supported
        //#define RANDR_DIALOG_ORIENTATION_ALL (ECORE_X_RANDR_ORIENTATION_ROT_0 | ECORE_X_RANDR_ORIENTATION_ROT_90 | ECORE_X_RANDR_ORIENTATION_ROT_180 | ECORE_X_RANDR_ORIENTATION_ROT_270 | ECORE_X_RANDR_ORIENTATION_ROT_270 | ECORE_X_RANDR_ORIENTATION_FLIP_X | ECORE_X_RANDR_ORIENTATION_FLIP_Y)
        supported_oris = RANDR_DIALOG_ORIENTATION_ALL;
        ori = ECORE_X_RANDR_ORIENTATION_ROT_0;
     }
   //Send signal to the edje, to represent the supported and current set orientation
   snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,supported,%d", supported_oris);
   edje_object_signal_emit(crtc, signal, "e");
   snprintf(signal, sizeof(signal), "conf,randr,dialog,orientation,current,%d", ori);
   edje_object_signal_emit(crtc, signal, "e");
}
*/

Eina_Bool
orientation_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;
   Eina_Bool success = EINA_TRUE;

   if (!cfdata) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || !odd->crtc || ((int)odd->new_orientation == Ecore_X_Randr_Unset))
          continue;
        fprintf(stderr, "CONF_RANDR: Change orientation of crtc %d to %d.\n", odd->crtc->xid, odd->new_orientation);
        if (!ecore_x_randr_crtc_orientation_set(cfd->con->manager->root, odd->crtc->xid, odd->new_orientation))
          success = EINA_FALSE;
     }

   return success;
}

Eina_Bool
orientation_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || !odd->crtc || ((int)odd->previous_orientation == Ecore_X_Randr_Unset) || ((int)odd->new_orientation == Ecore_X_Randr_Unset))
          continue;
        if (odd->previous_orientation != odd->new_orientation)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
orientation_widget_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || ((int)odd->previous_orientation == Ecore_X_Randr_Unset))
          continue;
        odd->previous_orientation = odd->new_orientation;
        odd->new_orientation = Ecore_X_Randr_Unset;
     }
}

void
orientation_widget_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd->crtc || ((int)odd->previous_orientation == Ecore_X_Randr_Unset))
          continue;
        ecore_x_randr_crtc_orientation_set(cfdata->manager->root, odd->crtc->xid, odd->previous_orientation);
        odd->new_orientation =  Ecore_X_Randr_Unset;
     }
}

   static void
_orientation_widget_radio_add_callbacks(void)
{
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_reflect_vertical, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_reflect_horizontal, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_rot270, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_rot180, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_rot90, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.orientation.radio_normal, EVAS_CALLBACK_MOUSE_UP, _orientation_widget_mouse_up_cb, NULL);
}

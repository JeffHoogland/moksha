#include "e_int_config_randr.h"
#include "e_randr.h"
#include "e_widget_ilist.h"

#ifdef  Ecore_X_Randr_Unset
#undef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset             -1
#else
#define Ecore_X_Randr_Unset             -1
#endif

#ifdef  Ecore_X_Randr_None
#undef  Ecore_X_Randr_None
#endif
#define Ecore_X_Randr_None   0

#define ICON_WIDTH 10
#define ICON_HEIGHT 10
#define RESOLUTION_TXT_MAX_LENGTH 50

Evas_Object *dialog_subdialog_resolutions_basic_create_widgets(Evas *canvas);
Eina_Bool dialog_subdialog_resolutions_basic_apply_data (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool dialog_subdialog_resolutions_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void dialog_subdialog_resolutions_update_list(Evas_Object *crtc);
void dialog_subdialog_resolutions_keep_changes(E_Config_Dialog_Data *cfdata);
void dialog_subdialog_resolutions_discard_changes(E_Config_Dialog_Data *cfdata);

extern E_Config_Dialog_Data *e_config_runtime_info;

   Eina_Bool
dialog_subdialog_resolutions_create_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Ecore_X_Randr_Mode_Info *mi;
   Eina_List *iter;

   if (!cfdata || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (odd->previous_mode || odd->preferred_mode)
          {
             //this means, that mode info is already filled
             //(by the display arrangement code)
             break;
          }
        if (odd->crtc)
          {
             if(!(mi = odd->crtc->current_mode))
               mi = (Ecore_X_Randr_Mode_Info*)eina_list_data_get(eina_list_last(odd->crtc->outputs_common_modes));
             odd->previous_mode = mi;
          }
        else if (odd->output)
          {
             odd->preferred_mode = (Ecore_X_Randr_Mode_Info*)eina_list_data_get(eina_list_last(odd->output->preferred_modes));
          }
     }

   return EINA_TRUE;
}

   Evas_Object *
dialog_subdialog_resolutions_basic_create_widgets(Evas *canvas)
{
   Evas_Object *subdialog;

   if (!canvas || !e_config_runtime_info || e_config_runtime_info->gui.subdialogs.resolutions.dialog || !(subdialog = e_widget_ilist_add(canvas, ICON_WIDTH * e_scale, ICON_HEIGHT * e_scale, NULL))) return NULL;

   e_widget_ilist_multi_select_set(subdialog, EINA_FALSE);
   e_widget_disabled_set(subdialog, EINA_TRUE);

   evas_object_show(subdialog);

   return subdialog;
}

   Eina_Bool
dialog_subdialog_resolutions_basic_apply_data (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   //Apply new mode
   Ecore_X_Randr_Mode_Info* selected_mode;
   Ecore_X_ID selected_mode_xid;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Output *output = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL, *crtc_iter;
   Eina_List *iter;
   int noutputs = Ecore_X_Randr_Unset;

   if (!e_config_runtime_info->gui.selected_eo || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "output_info")))
     {
        fprintf(stderr, "CONF_RADNR: no crtc was selected or no output info could be retrieved for the selected crtc element (%p).\n", e_config_runtime_info->gui.selected_eo);
        return EINA_FALSE;
     }

   if (output_dialog_data->crtc)
     {
        //CRTC is already asssigned, easy one!
        crtc_info = output_dialog_data->crtc;
     }
   else if (output_dialog_data->output)
     {
        //CRTC not assigned yet. Let's try to find a non occupied one.
        fprintf(stderr, "CONF_RANDR: Trying to find a CRTC for output %x, %d crtcs are possible.\n", output_dialog_data->output->xid, eina_list_count(output_dialog_data->output->possible_crtcs));
        output = &output_dialog_data->output->xid;
        noutputs = 1;
        EINA_LIST_FOREACH(output_dialog_data->output->possible_crtcs, iter, crtc_iter)
          {
             if (!crtc_iter->outputs)
               {
                  //CRTC is not occupied yet
                  crtc_info = crtc_iter;
                  break;
               }
          }
     }
   if (!crtc_info)
     {
        fprintf(stderr, "CONF_RANDR: Changing mode failed, no unoccupied CRTC found!\n");
        return EINA_FALSE;
     }
   //get selected mode
   if ((selected_mode = (Ecore_X_Randr_Mode_Info*)e_widget_ilist_selected_data_get(e_config_runtime_info->gui.subdialogs.resolutions.dialog)))
     {
        selected_mode_xid = selected_mode->xid;
     }
   else
     {
        selected_mode_xid = Ecore_X_Randr_None;
     }

   fprintf(stderr, "CONF_RANDR: Change mode of crtc %x to %x.\n", crtc_info->xid, selected_mode_xid);

   if (ecore_x_randr_crtc_mode_set(cfd->con->manager->root, crtc_info->xid, output, noutputs, selected_mode_xid))
     {
        //remove unused space
        ecore_x_randr_screen_reset(cfd->con->manager->root);
        //update information
        if (!output_dialog_data->crtc)
          output_dialog_data->crtc = crtc_info;
        output_dialog_data->new_mode = selected_mode;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

   Eina_Bool
dialog_subdialog_resolutions_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   Ecore_X_Randr_Mode_Info* selected_mode;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   if (!e_config_runtime_info->gui.selected_eo || !(selected_mode = (Ecore_X_Randr_Mode_Info*)e_widget_ilist_selected_data_get(e_config_runtime_info->gui.subdialogs.resolutions.dialog)) || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "output_info"))) return EINA_FALSE;

   return (selected_mode != output_dialog_data->previous_mode);
}

   void
dialog_subdialog_resolutions_update_list(Evas_Object *crtc)
{
   Eina_List *iter, *modelist = NULL;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Mode_Info *mode_info, *current_mode;
   char resolution_text[RESOLUTION_TXT_MAX_LENGTH];
   float rate;
   int str_ret, i = 0;

   e_widget_ilist_freeze(e_config_runtime_info->gui.subdialogs.resolutions.dialog);
   e_widget_ilist_clear(e_config_runtime_info->gui.subdialogs.resolutions.dialog);
   if (!crtc)
     {
        e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.resolutions.dialog, EINA_TRUE);
        return;
     }
   if (!(output_dialog_data = evas_object_data_get(crtc, "output_info")))
     return;

   //select correct mode list
   if (output_dialog_data->crtc)
     {
        current_mode = output_dialog_data->crtc->current_mode;
        modelist = output_dialog_data->crtc->outputs_common_modes;
     }
   else if (output_dialog_data->output)
     {
        current_mode = NULL;
        if (output_dialog_data->output->modes)
          modelist = output_dialog_data->output->modes;
        else
          modelist = output_dialog_data->output->modes;
     }
   EINA_LIST_FOREACH(modelist, iter, mode_info)
     {
        //calculate refresh rate
        if (!mode_info) continue;
        if (mode_info->hTotal && mode_info->vTotal)
          rate = ((float) mode_info->dotClock /
                ((float) mode_info->hTotal * (float) mode_info->vTotal));
        else
          rate = 0.0;

        str_ret = snprintf(resolution_text, RESOLUTION_TXT_MAX_LENGTH, "%dx%d@%.1fHz", mode_info->width, mode_info->height, rate);
        if (str_ret < 0 || str_ret > (RESOLUTION_TXT_MAX_LENGTH - 1))
          {
             fprintf(stderr, "CONF_RANDR: Resolution text could not be created.");
             continue;
          }
        e_widget_ilist_append(e_config_runtime_info->gui.subdialogs.resolutions.dialog, NULL, resolution_text, NULL, mode_info, NULL);

        //select currently enabled mode
        if (mode_info == current_mode)
            e_widget_ilist_selected_set(e_config_runtime_info->gui.subdialogs.resolutions.dialog, i);
        i++;
     }

   //append 'disabled' mode
   e_widget_ilist_append(e_config_runtime_info->gui.subdialogs.resolutions.dialog, NULL, _("Disabled"), NULL, NULL, NULL);

   //reenable widget
   e_widget_disabled_set(e_config_runtime_info->gui.subdialogs.resolutions.dialog, EINA_FALSE);
   e_widget_ilist_go(e_config_runtime_info->gui.subdialogs.resolutions.dialog);
   e_widget_ilist_thaw(e_config_runtime_info->gui.subdialogs.resolutions.dialog);
}


void
dialog_subdialog_resolutions_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (odd && odd->new_mode && (odd->new_mode != odd->previous_mode))
          {
             odd->previous_mode = odd->new_mode;
             odd->new_mode = NULL;
          }
     }
}

   void
dialog_subdialog_resolutions_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        //for now, there is no way to redisable an output during discartion
        if (!odd->crtc || !odd->previous_mode) continue;
        //use currently used outputs (noutputs == Ecore_X_Randr_Unset)
        if (ecore_x_randr_crtc_mode_set(cfdata->manager->root, odd->crtc->xid, NULL, Ecore_X_Randr_Unset, odd->previous_mode->xid))
          {
             odd->new_mode = odd->previous_mode;
             odd->previous_mode = NULL;
             ecore_x_randr_screen_reset(cfdata->manager->root);
          }
     }
}

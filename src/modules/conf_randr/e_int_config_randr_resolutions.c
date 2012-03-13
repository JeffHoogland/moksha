#include "e_int_config_randr.h"
#include "e_randr.h"
#include "e_widget_ilist.h"

#ifdef  Ecore_X_Randr_Unset
#undef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#else
#define Ecore_X_Randr_Unset -1
#endif

#ifdef  Ecore_X_Randr_None
#undef  Ecore_X_Randr_None
#endif
#define Ecore_X_Randr_None        0

#define ICON_WIDTH                10
#define ICON_HEIGHT               10
#define RESOLUTION_TXT_MAX_LENGTH 50

Evas_Object *resolution_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    resolution_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    resolution_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         resolution_widget_update_list(Evas_Object *crtc);
void         resolution_widget_keep_changes(E_Config_Dialog_Data *cfdata);
void         resolution_widget_discard_changes(E_Config_Dialog_Data *cfdata);

extern E_Config_Dialog_Data *e_config_runtime_info;
static Ecore_X_Randr_Mode_Info disabled_mode = {.xid = Ecore_X_Randr_None};

Eina_Bool
resolution_widget_create_data(E_Config_Dialog_Data *cfdata)
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
             if (!(mi = odd->crtc->current_mode))
               mi = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->crtc->outputs_common_modes));
             odd->previous_mode = mi;
          }
        else if (odd->output && odd->output->monitor)
          {
             odd->previous_mode = NULL;
             odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(eina_list_last(odd->output->monitor->preferred_modes));
          }
     }

   return EINA_TRUE;
}

Evas_Object *
resolution_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget;

   if (!canvas || !e_config_runtime_info || e_config_runtime_info->gui.widgets.resolutions.widget || !(widget = e_widget_ilist_add(canvas, ICON_WIDTH * e_scale, ICON_HEIGHT * e_scale, NULL))) return NULL;

   e_widget_ilist_multi_select_set(widget, EINA_FALSE);
   e_widget_disabled_set(widget, EINA_TRUE);

   evas_object_show(widget);

   return widget;
}

Eina_Bool
resolution_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   //Apply new mode
   Ecore_X_Randr_Mode_Info *selected_mode;
   Ecore_X_Randr_Mode mode;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Output *outputs = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL, *crtc_iter;
   Eina_List *iter;
   int noutputs = Ecore_X_Randr_Unset;

   if (!e_config_runtime_info->gui.selected_eo || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "rep_info")))
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
        fprintf(stderr, "CONF_RANDR: Trying to find a CRTC for output %d, %d crtcs are possible.\n", output_dialog_data->output->xid, eina_list_count(output_dialog_data->output->possible_crtcs));
        outputs = &output_dialog_data->output->xid;
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
   if ((selected_mode = (Ecore_X_Randr_Mode_Info *)e_widget_ilist_selected_data_get(e_config_runtime_info->gui.widgets.resolutions.widget)))
     {
        mode = selected_mode->xid;
     }
   if (selected_mode == crtc_info->current_mode)
     {
        if (output_dialog_data->output && (eina_list_data_find(crtc_info->outputs, output_dialog_data->output)))
          {
             fprintf(stderr, "CONF_RANDR: Nothing to be done for output %s.\n", output_dialog_data->output->name);
             return EINA_TRUE;
          }
        fprintf(stderr, "CONF_RANDR: Resolution remains unchanged for CRTC %d.\n", crtc_info->xid);
        return EINA_TRUE;
     }
   fprintf(stderr, "CONF_RANDR: Changing mode of crtc %d to %d.\n", crtc_info->xid, mode);

   if (ecore_x_randr_crtc_mode_set(cfd->con->manager->root, crtc_info->xid, outputs, noutputs, mode))
     {
        //update information
        output_dialog_data->crtc = crtc_info;
        output_dialog_data->new_mode = selected_mode;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

Eina_Bool
resolution_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   Ecore_X_Randr_Mode_Info *selected_mode;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;

   if (!e_config_runtime_info->gui.selected_eo || !(selected_mode = (Ecore_X_Randr_Mode_Info *)e_widget_ilist_selected_data_get(e_config_runtime_info->gui.widgets.resolutions.widget)) || !(output_dialog_data = evas_object_data_get(e_config_runtime_info->gui.selected_eo, "rep_info"))) return EINA_FALSE;

   return selected_mode != output_dialog_data->previous_mode;
}

void
resolution_widget_update_list(Evas_Object *rep)
{
   Eina_List *iter, *modelist = NULL;
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Mode_Info *mode_info, *current_mode;
   char resolution_text[RESOLUTION_TXT_MAX_LENGTH];
   float rate;
   Eina_Bool enable = EINA_FALSE;
   int i = 0;

   e_widget_ilist_freeze(e_config_runtime_info->gui.widgets.resolutions.widget);
   e_widget_ilist_clear(e_config_runtime_info->gui.widgets.resolutions.widget);

   if (!rep || !(output_dialog_data = evas_object_data_get(rep, "rep_info")))
     goto _go_and_return;

   //select correct mode list
   if (output_dialog_data->crtc)
     {
        current_mode = output_dialog_data->crtc->current_mode;
        modelist = output_dialog_data->crtc->outputs_common_modes;
     }
   else if (output_dialog_data->output && output_dialog_data->output->monitor)
     {
        current_mode = NULL;
        if (output_dialog_data->output->monitor->modes)
          modelist = output_dialog_data->output->monitor->modes;
     }
   if (!modelist)
     goto _go_and_return;

   EINA_LIST_FOREACH(modelist, iter, mode_info)
     {
        //calculate refresh rate
        if (!mode_info) continue;
        if (mode_info->hTotal && mode_info->vTotal)
          rate = ((float)mode_info->dotClock /
                  ((float)mode_info->hTotal * (float)mode_info->vTotal));
        else
          rate = 0.0;

        if (mode_info->name)
          snprintf(resolution_text, (RESOLUTION_TXT_MAX_LENGTH - 1), "%s@%.1fHz", mode_info->name, rate);
        else
          snprintf(resolution_text, (RESOLUTION_TXT_MAX_LENGTH - 1), "%dx%d@%.1fHz", mode_info->width, mode_info->height, rate);

        e_widget_ilist_append(e_config_runtime_info->gui.widgets.resolutions.widget, NULL, resolution_text, NULL, mode_info, NULL);

        //select currently enabled mode
        if (mode_info == current_mode)
          e_widget_ilist_selected_set(e_config_runtime_info->gui.widgets.resolutions.widget, i);
        i++;
     }

   //append 'disabled' mode
   e_widget_ilist_append(e_config_runtime_info->gui.widgets.resolutions.widget, NULL, _("Disabled"), NULL, &disabled_mode, NULL);

   //reenable widget
   enable = EINA_TRUE;

   _go_and_return:
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.resolutions.widget, enable);
   e_widget_ilist_go(e_config_runtime_info->gui.widgets.resolutions.widget);
   e_widget_ilist_thaw(e_config_runtime_info->gui.widgets.resolutions.widget);
}

void
resolution_widget_keep_changes(E_Config_Dialog_Data *cfdata)
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
resolution_widget_discard_changes(E_Config_Dialog_Data *cfdata)
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


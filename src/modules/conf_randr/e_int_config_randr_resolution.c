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

extern E_Config_Dialog_Data *e_config_runtime_info;
static Ecore_X_Randr_Mode_Info disabled_mode = {.xid = Ecore_X_Randr_None, .name = "Disabled"};
static void _resolution_widget_selected_cb(void *data);

Eina_Bool
resolution_widget_create_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (odd->crtc)
          {
             odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(odd->crtc->outputs_common_modes);
             odd->previous_mode = odd->crtc->current_mode ? odd->crtc->current_mode : &disabled_mode;
          }
        else if (odd->output && odd->output->monitor)
          {
             odd->previous_mode = NULL;
             odd->preferred_mode = (Ecore_X_Randr_Mode_Info *)eina_list_data_get(odd->output->monitor->preferred_modes);
          }
        odd->new_mode = NULL;
     }

   return EINA_TRUE;
}

void
resolution_widget_free_cfdata(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
}

Evas_Object *
resolution_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget;

   if (e_config_runtime_info->gui.widgets.resolution.widget)
     return e_config_runtime_info->gui.widgets.resolution.widget;
   else if (!canvas || !e_config_runtime_info || !(widget = e_widget_ilist_add(canvas, ICON_WIDTH * e_scale, ICON_HEIGHT * e_scale, NULL)))
     return NULL;

   e_widget_ilist_multi_select_set(widget, EINA_FALSE);
   e_widget_disabled_set(widget, EINA_TRUE);
   evas_object_show(widget);

   return widget;
}

Eina_Bool
resolution_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Ecore_X_Randr_Output *outputs = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL;
   Eina_List *it, *it2;
   int noutputs = Ecore_X_Randr_Unset;
   Eina_Bool success = EINA_TRUE, another_one_enabled = EINA_FALSE;

   //make sure at least one other output is not disabled
   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, it, odd)
     {
        if ((!odd->new_mode && odd->previous_mode && (odd->previous_mode != &disabled_mode)) || (odd->new_mode && (odd->new_mode != &disabled_mode)))
          {
             another_one_enabled = EINA_TRUE;
             break;
          }
     }

   if (!another_one_enabled)
     {
        e_util_dialog_show("Invalid configuration!", "At least one monitor has to remain enabled!");
        fprintf(stderr, "CONF_RANDR: No other monitor is enabled, aborting reconfiguration.\n");
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, it, odd)
     {
        if (!odd || !odd->new_mode || (odd->new_mode == odd->previous_mode))
          continue;

        if (odd->crtc)
          crtc_info = odd->crtc; //CRTC is already asssigned, easy one!
        else if (odd->output)
          {
             //CRTC not assigned yet. Let's try to find a non occupied one.
             fprintf(stderr, "CONF_RANDR: Trying to find a CRTC for output %d, %d crtcs are possible.\n", odd->output->xid, eina_list_count(odd->output->possible_crtcs));
             outputs = &odd->output->xid;
             noutputs = 1;
             EINA_LIST_FOREACH(odd->output->possible_crtcs, it2, crtc_info)
               {
                  if (crtc_info->outputs)
                    crtc_info = NULL; //CRTC is not occupied yet
                  else
                    break;
               }
          }
        if (!crtc_info)
          {
             fprintf(stderr, "CONF_RANDR: Changing mode failed, no unoccupied CRTC found!\n");
             success = EINA_FALSE;
             continue;
          }
        if (odd->new_mode == crtc_info->current_mode)
          {
             if (odd->output && (eina_list_data_find(crtc_info->outputs, odd->output)))
                  fprintf(stderr, "CONF_RANDR: Nothing to be done for output %s.\n", odd->output->name);
             fprintf(stderr, "CONF_RANDR: Resolution remains unchanged for CRTC %d.\n", crtc_info->xid);
             continue;
          }
        fprintf(stderr, "CONF_RANDR: Changing mode of crtc %d to %d.\n", crtc_info->xid, odd->new_mode->xid);

        if (ecore_x_randr_crtc_mode_set(cfd->con->manager->root, crtc_info->xid, outputs, noutputs, odd->new_mode->xid))
          {
             //update information
             odd->crtc = crtc_info;
          }
     }

   return success;
}

Eina_Bool
resolution_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *it;

   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, it, odd)
     {
         if (!odd || !odd->new_mode)
             continue;
         if (odd->new_mode != odd->previous_mode)
             return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
resolution_widget_update_list(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   Eina_List *iter, *modelist = NULL;
   Ecore_X_Randr_Mode_Info *mode_info, *current_mode = NULL;
   char resolution_text[RESOLUTION_TXT_MAX_LENGTH];
   float rate;
   Eina_Bool enable = EINA_FALSE;
   int i = 0;

   e_widget_ilist_freeze(e_config_runtime_info->gui.widgets.resolution.widget);
   e_widget_ilist_clear(e_config_runtime_info->gui.widgets.resolution.widget);

   if (!odd)
     goto _go_and_return;

   //select correct mode list
   if (odd->new_mode)
     current_mode = odd->new_mode;
   else if (odd->crtc)
     {
        if (!odd->crtc->current_mode)
          current_mode = &disabled_mode;
        else
          current_mode = odd->crtc->current_mode;
     }

   if (odd->crtc)
     modelist = odd->crtc->outputs_common_modes;
   else if (odd->output && odd->output->monitor)
     modelist = odd->output->monitor->modes;

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
          snprintf(resolution_text, (RESOLUTION_TXT_MAX_LENGTH - 1), _("%s@%.1fHz"), mode_info->name, rate);
        else
          snprintf(resolution_text, (RESOLUTION_TXT_MAX_LENGTH - 1), _("%d×%d@%.1fHz"), mode_info->width, mode_info->height, rate);

        e_widget_ilist_append(e_config_runtime_info->gui.widgets.resolution.widget, NULL, resolution_text, _resolution_widget_selected_cb, mode_info, NULL);

        if (mode_info != current_mode)
          i++;
        else
          e_widget_ilist_selected_set(e_config_runtime_info->gui.widgets.resolution.widget, i);
     }

   //append 'disabled' mode
   e_widget_ilist_append(e_config_runtime_info->gui.widgets.resolution.widget, NULL, _("Disabled"), NULL, &disabled_mode, NULL);
   if (current_mode == &disabled_mode) //select currently enabled mode
     {
        i = eina_list_count(modelist);
        e_widget_ilist_selected_set(e_config_runtime_info->gui.widgets.resolution.widget, i);
     }


   //reenable widget
   enable = EINA_TRUE;

   _go_and_return:
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.resolution.widget, enable);
   e_widget_ilist_go(e_config_runtime_info->gui.widgets.resolution.widget);
   e_widget_ilist_thaw(e_config_runtime_info->gui.widgets.resolution.widget);
}

void
resolution_widget_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *iter;

   if (!cfdata) return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, iter, odd)
     {
        if (!odd || !odd->new_mode || (odd->new_mode == odd->previous_mode))
          continue;
        odd->previous_mode = (odd->new_mode == &disabled_mode) ? NULL : odd->new_mode;
        odd->new_mode = NULL;
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
        ecore_x_randr_crtc_mode_set(cfdata->manager->root, odd->crtc->xid, NULL, Ecore_X_Randr_Unset, odd->previous_mode->xid);
     }
   ecore_x_randr_screen_reset(cfdata->manager->root);
}

static void
_resolution_widget_selected_cb(void *data)
{
   Ecore_X_Randr_Mode_Info *selected_mode = (Ecore_X_Randr_Mode_Info*)data;

   if (!selected_mode)
     return;

   if (!e_config_runtime_info->gui.selected_output_dd)
     {
        fprintf(stderr, "CONF_RANDR: Can't set newly selected resolution, because no odd was set \"selected\" yet!\n");
        return;
     }

   e_config_runtime_info->gui.selected_output_dd->new_mode = selected_mode;

   fprintf(stderr, "CONF_RANDR: Mode %s was selected for crtc/output %d!\n", (selected_mode ? selected_mode->name : "None"), (e_config_runtime_info->gui.selected_output_dd->crtc ? e_config_runtime_info->gui.selected_output_dd->crtc->xid :  e_config_runtime_info->gui.selected_output_dd->output->xid));
   arrangement_widget_rep_update(e_config_runtime_info->gui.selected_output_dd);
}

#include "e_randr_private.h"
#include "e_randr.h"

E_Randr_Monitor_Info           *_monitor_info_new(E_Randr_Output_Info *output_info);
static int                      _modes_size_sort_cb(const void *d1, const void *d2);

void
_monitor_modes_refs_set(E_Randr_Monitor_Info *mi, Ecore_X_Randr_Output o)
{
   Ecore_X_Randr_Mode *modes = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   int nmodes = 0, npreferred = 0;

   EINA_SAFETY_ON_NULL_RETURN(mi);
   EINA_SAFETY_ON_TRUE_RETURN(o == Ecore_X_Randr_None);

   // Add (preferred) modes
   modes = ecore_x_randr_output_modes_get(e_randr_screen_info.root, o, &nmodes, &npreferred);
   while (--nmodes >= 0)
     {
        if (!modes[nmodes]) continue;
        if (!(mode_info = _12_screen_info_mode_info_get(modes[nmodes])))
          {
             //Mode unknown to the global structure, so add it
             mode_info = ecore_x_randr_mode_info_get(e_randr_screen_info.root, modes[nmodes]);
             e_randr_screen_info.rrvd_info.randr_info_12->modes = eina_list_append(e_randr_screen_info.rrvd_info.randr_info_12->modes, mode_info);
          }
        mi->modes = eina_list_prepend(mi->modes, mode_info);
        if (nmodes <= npreferred)
          mi->preferred_modes = eina_list_prepend(mi->preferred_modes, mode_info);
     }
   free(modes);
}

/**
 * @brief Allocates a new E_Randr_Monitor_Info struct and initializes it with
 * default values.
 * @return E_Randr_Monitor_Info element, or if it could not be
 * created, NULL
 */
E_Randr_Monitor_Info *
_monitor_info_new(E_Randr_Output_Info *oi)
{
   E_Randr_Monitor_Info *mi = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(oi, NULL);

   mi = E_NEW(E_Randr_Monitor_Info, 1);

   // Set some default values
   mi->modes = NULL;
   mi->preferred_modes = NULL;
   mi->size_mm.width = Ecore_X_Randr_Unset;
   mi->size_mm.height = Ecore_X_Randr_Unset;
   mi->edid = NULL;
   mi->edid_length = 0;
   mi->edid_hash.hash = 0;
   mi->max_backlight = Ecore_X_Randr_Unset;
   mi->backlight_level = 0.0;

   _monitor_modes_refs_set(mi, oi->xid);

   ecore_x_randr_output_size_mm_get(e_randr_screen_info.root, oi->xid, &mi->size_mm.width, &mi->size_mm.height);
   mi->edid = ecore_x_randr_output_edid_get(e_randr_screen_info.root, oi->xid, &mi->edid_length);
   if (mi->edid_length > 0)
     mi->edid_hash.hash = eina_hash_superfast((char *)mi->edid, mi->edid_length);

   return mi;
}

/**
 * @brief Frees E_Randr_Monitor_Info structure
 */
void
_monitor_info_free(E_Randr_Monitor_Info *monitor_info)
{
   if (!monitor_info)
     return;

   eina_list_free(monitor_info->modes);
   eina_list_free(monitor_info->preferred_modes);
   free(monitor_info->edid);
   free(monitor_info);
}

/**
 * @brief allocates a struct and fills it with default values.
 * @param output the output the display is queried for. If Ecore_X_Randr_None is
 * given, a struct with only the xid will be set
 * @return E_Randr_Output_Info element
 */
E_Randr_Output_Info *
_output_info_new(Ecore_X_Randr_Output output)
{
   E_Randr_Output_Info *output_info = NULL;
   char *str;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, NULL);

   output_info = E_NEW(E_Randr_Output_Info, 1);

   output_info->xid = output;

   //Use default values
   output_info->crtc = NULL;
   output_info->wired_clones = NULL;
   output_info->possible_crtcs = NULL;
   output_info->signalformats = Ecore_X_Randr_Unset;
   output_info->signalformat = Ecore_X_Randr_Unset;
   output_info->connector_number = 0;
   output_info->monitor = NULL;
   output_info->connector_type = Ecore_X_Randr_Unset;
   output_info->policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;
   output_info->compatibility_list = NULL;
   output_info->subpixel_order = Ecore_X_Randr_Unset;

   str = ecore_x_randr_output_name_get(e_randr_screen_info.root, output_info->xid, &output_info->name_length);
   output_info->name = eina_stringshare_add(str);
   free(str);
   output_info->connection_status = ecore_x_randr_output_connection_status_get(e_randr_screen_info.root, output_info->xid);

   return output_info;
}

void
_output_info_free(E_Randr_Output_Info *output_info)
{
   EINA_SAFETY_ON_NULL_RETURN(output_info);

    eina_list_free(output_info->wired_clones);
    eina_list_free(output_info->possible_crtcs);
    eina_list_free(output_info->compatibility_list);
    eina_stringshare_del(output_info->name);
    _monitor_info_free(output_info->monitor);
    output_info->monitor = NULL;
    free(output_info);
}

void
_output_refs_set(E_Randr_Output_Info *output_info)
{
   Ecore_X_Randr_Crtc crtc, *crtcs = NULL;
   E_Randr_Crtc_Info *crtc_info;
   int ncrtcs = 0;

   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);
   EINA_SAFETY_ON_NULL_RETURN(output_info);

   eina_list_free(output_info->possible_crtcs);

   //Add possible crtcs
   crtcs = ecore_x_randr_output_possible_crtcs_get(e_randr_screen_info.root, output_info->xid, &ncrtcs);
   while (--ncrtcs >= 0)
     {
        if (!(crtc_info = _12_screen_info_crtc_info_get(crtcs[ncrtcs])))
          continue;
        output_info->possible_crtcs = eina_list_append(output_info->possible_crtcs, crtc_info);
     }
   free(crtcs);

   crtc = ecore_x_randr_output_crtc_get(e_randr_screen_info.root, output_info->xid);
   output_info->crtc = _12_screen_info_crtc_info_get(crtc);

   if (output_info->connection_status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
     output_info->monitor = _monitor_info_new(output_info);
   else
     output_info->monitor = NULL;
}

Ecore_X_Randr_Output *
_outputs_to_array(Eina_List *outputs_info)
{
   Ecore_X_Randr_Output *ret = NULL;
   E_Randr_Output_Info *output_info;
   Eina_List *output_iter;
   int i = 0;

   if (!outputs_info || !(ret = malloc(sizeof(Ecore_X_Randr_Output) * eina_list_count(outputs_info)))) return NULL;
   EINA_LIST_FOREACH(outputs_info, output_iter, output_info)
      /* output_info == NULL should _not_ be possible! */
      ret[i++] = output_info ? output_info->xid : Ecore_X_Randr_None;
   return ret;
}

/*
 * returns a list of modes common ammongst the given outputs,
 * optionally limited by max_size_mode. If none are found, NULL is returned.
 */
Eina_List
*_outputs_common_modes_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode)
{
   Eina_List *common_modes = NULL, *mode_iter, *output_iter, *mode_next, *output_next;
   E_Randr_Output_Info *output_info;
   Ecore_X_Randr_Mode_Info *mode_info;

   EINA_SAFETY_ON_NULL_RETURN_VAL(outputs, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_randr_screen_info.rrvd_info.randr_info_12->modes, NULL);

   //create a list of all common modes
   common_modes = eina_list_clone(e_randr_screen_info.rrvd_info.randr_info_12->modes);
   common_modes = eina_list_sort(common_modes, 0, _modes_size_sort_cb);

   EINA_LIST_FOREACH_SAFE(common_modes, mode_iter, mode_next, mode_info)
     {
        EINA_LIST_FOREACH_SAFE(outputs, output_iter, output_next, output_info)
          {
             if (!output_info || !output_info->monitor)
               continue;
             if (!eina_list_data_find(output_info->monitor->modes, mode_info))
               common_modes = eina_list_remove(common_modes, mode_info);
          }
     }

   if (max_size_mode)
     {
        //remove all modes that are larger than max_size_mode
        EINA_LIST_FOREACH_SAFE(common_modes, mode_iter, mode_next, mode_info)
          {
             if (_modes_size_sort_cb((void *)max_size_mode, (void *)mode_info) < 0)
               common_modes = eina_list_remove(common_modes, mode_info);
          }
     }

   //sort modes desc. by their sizes
   common_modes = eina_list_reverse(common_modes);

   return common_modes;
}

static int
_modes_size_sort_cb(const void *d1, const void *d2)
{
   Ecore_X_Randr_Mode_Info *mode1 = ((Ecore_X_Randr_Mode_Info *)d1), *mode2 = ((Ecore_X_Randr_Mode_Info *)d2);

   if (!d1) return 1;
   if (!d2) return -1;

   return (mode1->width * mode1->height) - (mode2->width * mode2->height);
}

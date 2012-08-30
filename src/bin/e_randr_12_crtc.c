#include "e_randr_private.h"
#include "e_randr.h"

// E_Randr_Crtc_Info helper functions
/* static Eina_Bool _crtc_mode_intersects_crtcs(E_Randr_Crtc_Info *crtc_info, Ecore_X_Randr_Mode_Info *mode); */
/* static Eina_Bool _crtc_outputs_mode_max_set(E_Randr_Crtc_Info *crtc_info); */

void
_crtc_outputs_refs_set(E_Randr_Crtc_Info *crtc_info)
{
   E_Randr_Output_Info *output_info = NULL;
   Ecore_X_Randr_Output *outputs = NULL;
   int noutputs = 0;

   EINA_SAFETY_ON_NULL_RETURN(crtc_info);

   outputs = ecore_x_randr_crtc_outputs_get(e_randr_screen_info.root, crtc_info->xid, &noutputs);

   while (--noutputs >= 0)
     {
        output_info = _12_screen_info_output_info_get(outputs[noutputs]);
        if (!output_info)
          {
             ERR("E_RANDR: Could not find output struct for output %d.", outputs[noutputs]);
             continue;
          }
        crtc_info->outputs = eina_list_append(crtc_info->outputs, output_info);
     }
   free(outputs);
   E_FREE_LIST(crtc_info->outputs_common_modes, ecore_x_randr_mode_info_free);
   crtc_info->outputs_common_modes = _outputs_common_modes_get(crtc_info->outputs, NULL);
}

void
_crtc_refs_set(E_Randr_Crtc_Info *crtc_info)
{
   Ecore_X_Randr_Mode mode = Ecore_X_Randr_None;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   Ecore_X_Randr_Output *poutputs = NULL;
   E_Randr_Output_Info *output_info = NULL;
   int npoutputs = 0;

   EINA_SAFETY_ON_NULL_RETURN(crtc_info);

   mode = ecore_x_randr_crtc_mode_get(e_randr_screen_info.root, crtc_info->xid);
   if (!(mode_info = _12_screen_info_mode_info_get(mode)) && (mode != Ecore_X_Randr_None))
     {
        //Mode does not equal "disabled" and is unknown to the global structure, so add it
        mode_info = ecore_x_randr_mode_info_get(e_randr_screen_info.root, mode);
        e_randr_screen_info.rrvd_info.randr_info_12->modes = eina_list_append(e_randr_screen_info.rrvd_info.randr_info_12->modes, mode_info);
     }
   crtc_info->current_mode = mode_info;

   poutputs = ecore_x_randr_crtc_possible_outputs_get(e_randr_screen_info.root, crtc_info->xid, &npoutputs);

   while (--npoutputs >= 0)
     {
        output_info = _12_screen_info_output_info_get(poutputs[npoutputs]);
        if (!output_info)
          {
             ERR("E_RANDR: Could not find output struct for output %d.", poutputs[npoutputs]);
             continue;
          }
        crtc_info->possible_outputs = eina_list_append(crtc_info->possible_outputs, output_info);
     }
   free(poutputs);

   _crtc_outputs_refs_set(crtc_info);
}

/**
 * @brief Allocate and init with values.
 * @param crtc the crtc the display is queried for. If Ecore_X_Randr_None is
 * given, a struct with only the xid will be set
 * @return E_Randr_Crtc_Info element
 */
E_Randr_Crtc_Info *
_crtc_info_new(Ecore_X_Randr_Crtc crtc)
{
   E_Randr_Crtc_Info *crtc_info = NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, NULL);

   crtc_info = E_NEW(E_Randr_Crtc_Info, 1);

   crtc_info->xid = crtc;
   crtc_info->panning.x = Ecore_X_Randr_Unset;
   crtc_info->panning.y = Ecore_X_Randr_Unset;
   crtc_info->panning.w = Ecore_X_Randr_Unset;
   crtc_info->panning.h = Ecore_X_Randr_Unset;
   crtc_info->tracking.x = Ecore_X_Randr_Unset;
   crtc_info->tracking.y = Ecore_X_Randr_Unset;
   crtc_info->tracking.w = Ecore_X_Randr_Unset;
   crtc_info->tracking.h = Ecore_X_Randr_Unset;
   crtc_info->border.x = Ecore_X_Randr_Unset;
   crtc_info->border.y = Ecore_X_Randr_Unset;
   crtc_info->border.w = Ecore_X_Randr_Unset;
   crtc_info->border.h = Ecore_X_Randr_Unset;

   crtc_info->gamma_ramps = NULL;
   crtc_info->gamma_ramp_size = Ecore_X_Randr_Unset;
   crtc_info->outputs = NULL;
   crtc_info->possible_outputs = NULL;
   crtc_info->outputs_common_modes = NULL;
   crtc_info->current_mode = NULL;

   ecore_x_randr_crtc_geometry_get(e_randr_screen_info.root, crtc_info->xid, &crtc_info->geometry.x, &crtc_info->geometry.y, &crtc_info->geometry.w, &crtc_info->geometry.h);
   crtc_info->current_orientation = ecore_x_randr_crtc_orientation_get(e_randr_screen_info.root, crtc_info->xid);
   crtc_info->orientations = ecore_x_randr_crtc_orientations_get(e_randr_screen_info.root, crtc_info->xid);

   return crtc_info;
}

/**
 * @param crtc_info the crtc info to be freed.
 */
void
_crtc_info_free(E_Randr_Crtc_Info *crtc_info)
{
   if (!crtc_info) return;

   free(crtc_info->gamma_ramps);
   crtc_info->outputs = eina_list_free(crtc_info->outputs);
   crtc_info->possible_outputs = eina_list_free(crtc_info->possible_outputs);
   free(crtc_info);
}

/*
 * returns EINA_TRUE if given CRTC would intersect with other CRTCs if set to
 * given mode
 */
/* static Eina_Bool */
/* _crtc_mode_intersects_crtcs(E_Randr_Crtc_Info *crtc_info, Ecore_X_Randr_Mode_Info *mode) */
/* { */
/*    Eina_List *iter; */
/*    E_Randr_Crtc_Info *tmp; */
/*    int width, height; */

/*    EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, tmp) */
/*      { */
/*         if ((tmp == crtc_info) || */
/*             ((tmp->geometry.w <= 0) || (tmp->geometry.h <= 0))) */
/*           continue; */
/*         width = (mode->width > INT_MAX) ? INT_MAX : mode->width; */
/*         height = (mode->height > INT_MAX) ? INT_MAX : mode->height; */
/*         if (E_INTERSECTS(crtc_info->geometry.x, crtc_info->geometry.y, */
/*                          width, height, tmp->geometry.x, */
/*                          tmp->geometry.y, tmp->geometry.w, tmp->geometry.h) */
/*             && ((crtc_info->geometry.x != tmp->geometry.x) && */
/*                 (crtc_info->geometry.y != tmp->geometry.y))) */
/*           return EINA_TRUE; */
/*      } */
/*    return EINA_FALSE; */
/* } */

/*
 * reconfigures a CRTC enabling the highest resolution amongst its outputs,
 * without touching any other CRTC currently activated
 */
/* static Eina_Bool */
/* _crtc_outputs_mode_max_set(E_Randr_Crtc_Info *crtc_info) */
/* { */
/*    Ecore_X_Randr_Mode_Info *mode_info; */
/*    Eina_List *iter; */
/*    Eina_Bool ret = EINA_TRUE; */
/*    Ecore_X_Randr_Output *outputs; */

/*    if (!crtc_info || !crtc_info->outputs || !crtc_info->outputs_common_modes) return EINA_FALSE; */

/*    EINA_LIST_REVERSE_FOREACH(crtc_info->outputs_common_modes, iter, mode_info) */
/*      { */
/*         if (!_crtc_mode_intersects_crtcs(crtc_info, mode_info)) */
/*           break; */
/*      } */
/*    if (!mode_info) */
/*      { */
/*         //eina_list_free(crtc_info->outputs_common_modes); */
/*         return EINA_FALSE; */
/*      } */
/*    if ((outputs = _outputs_to_array(crtc_info->outputs))) */
/*      { */
/*         ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, crtc_info->xid, outputs, eina_list_count(crtc_info->outputs), mode_info->xid); */
/*         free(outputs); */
/*      } */
/*    //eina_list_free(crtc_info->outputs_common_modes); */
/*    //crtc_info->outputs_common_modes = NULL; */

/*    ecore_x_randr_screen_reset(e_randr_screen_info.root); */

/*    return ret; */
/* } */

/*
 * this retrieves a CRTC depending on a policy.
 * Note that this is enlightenment specific! Enlightenment doesn't 'allow' zones
 * to overlap. Thus we always use the output with the most extreme position
 * instead of trying to fill gaps like tetris. Though this could be done by
 * simply implementing another policy.
 *
 * Simply put: get the
 *                      -right
 *                      -left
 *                      -top
 *                      -bottom
 *                      most CRTC and return it.
 */
const E_Randr_Crtc_Info *
_crtc_according_to_policy_get(E_Randr_Crtc_Info *but, Ecore_X_Randr_Output_Policy policy)
{
   Eina_List *iter, *possible_crtcs = NULL;
   E_Randr_Crtc_Info *crtc_info, *ret = NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO_CRTCS, NULL);

   //get any crtc that besides 'but' to start with
   possible_crtcs = eina_list_clone(e_randr_screen_info.rrvd_info.randr_info_12->crtcs);
   possible_crtcs = eina_list_remove(possible_crtcs, but);

   if ((eina_list_count(possible_crtcs) == 0) && (policy != ECORE_X_RANDR_OUTPUT_POLICY_CLONE))
     {
        eina_list_free(possible_crtcs);
        return NULL;
     }

   // get an initial value for ret
   ret = (E_Randr_Crtc_Info*)eina_list_data_get(eina_list_last(possible_crtcs));

   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info->geometry.y < ret->geometry.y)
               ret = crtc_info;
          }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if ((crtc_info->geometry.x + crtc_info->geometry.w) >
                  (ret->geometry.x + ret->geometry.w))
               ret = crtc_info;
          }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if ((crtc_info->geometry.y + crtc_info->geometry.h) >
                  (ret->geometry.y + ret->geometry.h))
               ret = crtc_info;
          }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info->geometry.x < ret->geometry.x)
               ret = crtc_info;
          }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        ret = (e_randr_screen_info.rrvd_info.randr_info_12->primary_output) ? e_randr_screen_info.rrvd_info.randr_info_12->primary_output->crtc : NULL;
        break;

      default:
        break;
     }

   eina_list_free(possible_crtcs);
   return ret;
}

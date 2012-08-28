#include "e_randr_private.h"
#include "e_randr.h"

//New helper functions
/**
 * @brief allocate and initialize a new E_Randr_Screen_Info_11 element
 * @return E_Randr_Screen_Info_11 elements or in case it could
 * not be created and properly initialized, NULL
 */
E_Randr_Screen_Info_11 *
_11_screen_info_new(void)
{
   E_Randr_Screen_Info_11 *randr_info_11 = NULL;
   Ecore_X_Randr_Screen_Size_MM *sizes = NULL;
   Ecore_X_Randr_Refresh_Rate *rates = NULL;
   int i, nsizes;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_11_NO, NULL);

   randr_info_11 = malloc(sizeof(E_Randr_Screen_Info_11));

   randr_info_11->sizes = NULL;
   randr_info_11->csize_index = Ecore_X_Randr_Unset;
   randr_info_11->corientation = Ecore_X_Randr_Unset;
   randr_info_11->orientations = Ecore_X_Randr_Unset;
   randr_info_11->rates = NULL;
   randr_info_11->current_rate = Ecore_X_Randr_Unset;

   if (!(sizes = ecore_x_randr_screen_primary_output_sizes_get(e_randr_screen_info.root, &nsizes)))
     goto _info_11_new_fail;
   randr_info_11->sizes = sizes, randr_info_11->nsizes = nsizes;
   ecore_x_randr_screen_primary_output_current_size_get(e_randr_screen_info.root, NULL, NULL, NULL, NULL, &(randr_info_11->csize_index));
   randr_info_11->corientation = ecore_x_randr_screen_primary_output_orientation_get(e_randr_screen_info.root);
   randr_info_11->orientations = ecore_x_randr_screen_primary_output_orientations_get(e_randr_screen_info.root);
   randr_info_11->rates = malloc(sizeof(Ecore_X_Randr_Refresh_Rate*) * nsizes);
   randr_info_11->nrates = malloc(sizeof(int) * nsizes);
   for (i = 0; i < nsizes; i++)
     {
        if (!(rates = ecore_x_randr_screen_primary_output_refresh_rates_get(e_randr_screen_info.root, i, &randr_info_11->nrates[i])))
          return EINA_FALSE;
        randr_info_11->rates[i] = rates;
     }
   randr_info_11->current_rate = ecore_x_randr_screen_primary_output_current_refresh_rate_get(e_randr_screen_info.root);

   return randr_info_11;

_info_11_new_fail:
   free(randr_info_11);
   return NULL;
}

//Free helper functions
/**
 * @param screen_info the screen info to be freed.
 */
void
_11_screen_info_free(E_Randr_Screen_Info_11 *screen_info)
{
   int x;

   EINA_SAFETY_ON_NULL_RETURN(screen_info);

   for (x = 0; x < screen_info->nsizes; x++)
     free(screen_info->rates[x]);
   free(screen_info->sizes);
   free(screen_info);
}

/*****************************************************************
 *
 * Init. and Shutdown code
 *
 *****************************************************************
 */
Eina_Bool
_11_screen_info_refresh(void)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_11_NO, EINA_FALSE);

   _11_screen_info_free(e_randr_screen_info.rrvd_info.randr_info_11);
   e_randr_screen_info.rrvd_info.randr_info_11 = _11_screen_info_new();

   return (e_randr_screen_info.rrvd_info.randr_info_11 != NULL);
}

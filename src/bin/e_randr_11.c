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
   Eina_List *rates_list;
   int i, j, nsizes, nrates;

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
   for (i = 0; i < nsizes; i++)
     if (!(randr_info_11->sizes = eina_list_append(randr_info_11->sizes, &sizes[i])))
       goto _info_11_new_fail_sizes;
   ecore_x_randr_screen_primary_output_current_size_get(e_randr_screen_info.root, NULL, NULL, NULL, NULL, &(randr_info_11->csize_index));
   randr_info_11->corientation = ecore_x_randr_screen_primary_output_orientation_get(e_randr_screen_info.root);
   randr_info_11->orientations = ecore_x_randr_screen_primary_output_orientations_get(e_randr_screen_info.root);
   for (i = 0; i < nsizes; i++)
     {
        rates_list = NULL;
        if (!(rates = ecore_x_randr_screen_primary_output_refresh_rates_get(e_randr_screen_info.root, i, &nrates)))
          return EINA_FALSE;
        for (j = 0; j < nrates; j++)
          if (!(rates_list = eina_list_append(rates_list, &rates[j])))
            goto _info_11_new_fail_rates_list;
        if (!(randr_info_11->rates = eina_list_append(randr_info_11->rates, rates_list)))
          goto _info_11_new_fail_rates;
     }
   randr_info_11->current_rate = ecore_x_randr_screen_primary_output_current_refresh_rate_get(e_randr_screen_info.root);

   return randr_info_11;

_info_11_new_fail_rates_list:
   eina_list_free(rates_list);
_info_11_new_fail_rates:
   free(rates);
_info_11_new_fail_sizes:
   free(sizes);
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
   EINA_SAFETY_ON_NULL_RETURN(screen_info);

   if (screen_info->sizes)
     {
        free(eina_list_nth(screen_info->sizes, 0));
        eina_list_free(screen_info->sizes);
        screen_info->sizes = NULL;
     }
   if (screen_info->rates)
     {
        /* this may be leaking, but at least it will be valid */
        eina_list_free(eina_list_nth(screen_info->rates, 0));
        eina_list_free(screen_info->rates);
        screen_info->rates = NULL;
     }
   free(screen_info);
   screen_info = NULL;
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
   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_11_NO);

   _11_screen_info_free(e_randr_screen_info.rrvd_info.randr_info_11);
   return ((e_randr_screen_info.rrvd_info.randr_info_11 = _11_screen_info_new()));
}

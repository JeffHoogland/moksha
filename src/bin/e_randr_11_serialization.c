#include "e_randr_private.h"
#include "e_randr.h"

/**********************************************************************
 *
 * Storage/Restorage of setups
 *
 **********************************************************************
 */

//New helper functions

E_Randr_Serialized_Setup_11 *
_serialized_setup_11_new(void)
{
   E_Randr_Serialized_Setup_11 *ss;
   Ecore_X_Randr_Screen_Size_MM *size;

   ss = malloc(sizeof(*ss));

   if (!(size = (Ecore_X_Randr_Screen_Size_MM *)eina_list_data_get(eina_list_nth(e_randr_screen_info.rrvd_info.randr_info_11->sizes, e_randr_screen_info.rrvd_info.randr_info_11->csize_index)))) goto _serialized_setup_11_new_failed_free_ss;
   ss->size.width = size->width;
   ss->size.height = size->height;
   ss->refresh_rate = e_randr_screen_info.rrvd_info.randr_info_11->current_rate;
   ss->orientation = e_randr_screen_info.rrvd_info.randr_info_11->corientation;

   return ss;

_serialized_setup_11_new_failed_free_ss:
   free(ss);
   return NULL;
}

//Update/value set helper functions
E_Randr_Serialized_Setup_11 *
_serialized_setup_11_update(E_Randr_Serialized_Setup_11 *ss_11)
{
   Ecore_X_Randr_Screen_Size_MM *size;

   if (ss_11)
     {
        if (!(size = (Ecore_X_Randr_Screen_Size_MM *)eina_list_data_get(eina_list_nth(e_randr_screen_info.rrvd_info.randr_info_11->sizes, e_randr_screen_info.rrvd_info.randr_info_11->csize_index)))) return NULL;
        if (!memcpy(&ss_11->size, size, sizeof(Ecore_X_Randr_Screen_Size_MM)))
          goto _update_serialized_setup_11_failed_free_ss;
        ss_11->refresh_rate = e_randr_screen_info.rrvd_info.randr_info_11->current_rate;
        ss_11->orientation = e_randr_screen_info.rrvd_info.randr_info_11->corientation;
     }
   else
     ss_11 = _serialized_setup_11_new();

   return ss_11;

_update_serialized_setup_11_failed_free_ss:
   free(ss_11);
   return NULL;
}

//Free helper functions
void
_e_randr_serialized_setup_11_free(E_Randr_Serialized_Setup_11 *ss11)
{
   free(ss11);
}

EINTERN void
e_randr_11_serialized_setup_free(E_Randr_Serialized_Setup_11 *ss_11)
{
   _e_randr_serialized_setup_11_free(ss_11);
}

Eina_Bool
_11_try_restore_configuration(void)
{
   Ecore_X_Randr_Screen_Size_MM *stored_size, *size;
   Eina_List *iter;
   int i = 0;

   if (!e_config->randr_serialized_setup->serialized_setup_11) return EINA_FALSE;
   stored_size = &e_config->randr_serialized_setup->serialized_setup_11->size;
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_11->sizes, iter, size)
     {
        if ((stored_size->width == size->width)
              && (stored_size->height == size->height)
              && (stored_size->width_mm == size->width_mm)
              && (stored_size->height_mm == size->height_mm))
          {
             return ecore_x_randr_screen_primary_output_size_set(e_randr_screen_info.root, i);
          }
        i++;
     }

   return EINA_FALSE;
}

void
_11_store_configuration(E_Randr_Configuration_Store_Modifier modifier)
{
   if (e_config->randr_serialized_setup->serialized_setup_11)
     e_config->randr_serialized_setup->serialized_setup_11 = _serialized_setup_11_update(e_config->randr_serialized_setup->serialized_setup_11);
   else
     e_config->randr_serialized_setup->serialized_setup_11 = _serialized_setup_11_new();
}

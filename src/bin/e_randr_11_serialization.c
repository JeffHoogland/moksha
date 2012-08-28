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
   Ecore_X_Randr_Orientation ori = ECORE_X_RANDR_ORIENTATION_ROT_0;
   Ecore_X_Randr_Refresh_Rate rate = 0;

   ss = malloc(sizeof(*ss));

   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     {
        if (e_randr_screen_info.rrvd_info.randr_info_11->csize_index >= e_randr_screen_info.rrvd_info.randr_info_11->nsizes) goto _serialized_setup_11_new_failed_free_ss;
        size = e_randr_screen_info.rrvd_info.randr_info_11->sizes + e_randr_screen_info.rrvd_info.randr_info_11->csize_index;
        if (!size) goto _serialized_setup_11_new_failed_free_ss;;
        rate = e_randr_screen_info.rrvd_info.randr_info_11->current_rate;
        ori = e_randr_screen_info.rrvd_info.randr_info_11->corientation;
        ss->size.width = size->width;
        ss->size.width_mm = size->width_mm;
        ss->size.height = size->height;
        ss->size.height_mm = size->height_mm;
     }
   else if (e_randr_screen_info.randr_version > ECORE_X_RANDR_1_1)
     {
        ecore_x_randr_screen_primary_output_current_size_get(e_randr_screen_info.root, &ss->size.width, &ss->size.height, &ss->size.width_mm, &ss->size.height_mm, NULL);
        rate = ecore_x_randr_screen_primary_output_current_refresh_rate_get(e_randr_screen_info.root);
        ori = ecore_x_randr_screen_primary_output_orientation_get(e_randr_screen_info.root);
     }

   ss->refresh_rate = rate;
   ss->orientation = ori;

   return ss;

_serialized_setup_11_new_failed_free_ss:
   free(ss);
   return NULL;
}

//Update/value set helper functions
E_Randr_Serialized_Setup_11 *
_serialized_setup_11_update(E_Randr_Serialized_Setup_11 *ss_11)
{
   if (ss_11)
     e_randr_11_serialized_setup_free(ss_11);

   ss_11 = _serialized_setup_11_new();

   return ss_11;
}

void
_11_store_configuration(E_Randr_Configuration_Store_Modifier modifier __UNUSED__)
{
   if (!e_config->randr_serialized_setup)
     e_config->randr_serialized_setup = e_randr_serialized_setup_new();

   if (e_config->randr_serialized_setup->serialized_setup_11)
     e_config->randr_serialized_setup->serialized_setup_11 = _serialized_setup_11_update(e_config->randr_serialized_setup->serialized_setup_11);
   else
     e_config->randr_serialized_setup->serialized_setup_11 = _serialized_setup_11_new();
}


EAPI void e_randr_11_store_configuration(E_Randr_Configuration_Store_Modifier modifier __UNUSED__)
{
   _11_store_configuration(modifier);
   e_config_save_queue();
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
   Ecore_X_Randr_Screen_Size_MM *stored_size, *sizes;
   int i = 0, nsizes;

#define SIZE_EQUAL(size) \
   ((stored_size->width == (size).width) \
    && (stored_size->height == (size).height) \
    && (stored_size->width_mm == (size).width_mm) \
    && (stored_size->height_mm == (size).height_mm))

   if (!e_config->randr_serialized_setup->serialized_setup_11) return EINA_FALSE;
   stored_size = &e_config->randr_serialized_setup->serialized_setup_11->size;
   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     {
        int x;
        for (x = 0; x < e_randr_screen_info.rrvd_info.randr_info_11->nsizes; x++)
          {
             if (SIZE_EQUAL(e_randr_screen_info.rrvd_info.randr_info_11->sizes[x]))
               {
                  return ecore_x_randr_screen_primary_output_size_set(e_randr_screen_info.root, i);
               }
             i++;
          }
     }
   else if (e_randr_screen_info.randr_version > ECORE_X_RANDR_1_1)
     {
        sizes = ecore_x_randr_screen_primary_output_sizes_get(e_randr_screen_info.root, &nsizes);
        for (i = 0; i < nsizes; i++)
          {
             if (SIZE_EQUAL(sizes[i]))
               {
                  free(sizes);
                  return ecore_x_randr_screen_primary_output_size_set(e_randr_screen_info.root, i);
               }
          }
     }
#undef SIZE_EQUAL

   return EINA_FALSE;
}

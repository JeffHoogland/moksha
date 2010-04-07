/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI E_Config_DD *
e_config_descriptor_new(const char *name, int size)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, name, size))
     return NULL;

   /* FIXME: We can directly map string inside an Eet_File and reuse it.
      But this need a break in all user of config every where in E.
   */

   return (E_Config_DD *) eet_data_descriptor_stream_new(&eddc);
}


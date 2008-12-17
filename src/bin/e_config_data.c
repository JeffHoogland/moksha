/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI E_Config_DD *
e_config_descriptor_new(const char *name, int size)
{
   Eet_Data_Descriptor_Class eddc;
   
   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.mem_alloc = NULL;
   eddc.func.mem_free = NULL;
   eddc.func.str_alloc = (char *(*)(const char *)) eina_stringshare_add;
   eddc.func.str_free = (void (*)(const char *)) eina_stringshare_del;
   eddc.func.list_next = (void *(*)(void *)) eina_list_next;
   eddc.func.list_append = (void *(*)(void *l, void *d)) eina_list_append;
   eddc.func.list_data = (void *(*)(void *)) eina_list_data_get;
   eddc.func.list_free = (void *(*)(void *)) eina_list_free;
   eddc.func.hash_foreach =
      (void  (*) (const Eina_Hash *, Eina_Bool (*) (const Eina_Hash *, const void *, void *, void *), const void *))
      eina_hash_foreach;
   eddc.func.hash_add = (Eina_Hash* (*) (Eina_Hash *, const void *, void *)) eet_eina_hash_add_alloc;
   eddc.func.hash_free = (void  (*) (Eina_Hash *)) eina_hash_free;
   eddc.name = name;
   eddc.size = size;
   return (E_Config_DD *)eet_data_descriptor2_new(&eddc);
}


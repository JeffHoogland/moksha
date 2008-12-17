/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#define E_CONFIG_DD_NEW(str, typ) \
   e_config_descriptor_new(str, sizeof(typ))
#define E_CONFIG_DD_FREE(eed) if (eed) { eet_data_descriptor_free((eed)); (eed) = NULL; }
#define E_CONFIG_VAL(edd, type, member, dtype) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define E_CONFIG_SUB(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)
#define E_CONFIG_LIST(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)
#define E_CONFIG_HASH(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_HASH(edd, type, #member, member, eddtype)

#define CHAR   EET_T_CHAR
#define SHORT  EET_T_SHORT
#define INT    EET_T_INT
#define LL     EET_T_LONG_LONG
#define FLOAT  EET_T_FLOAT
#define DOUBLE EET_T_DOUBLE
#define UCHAR  EET_T_UCHAR
#define USHORT EET_T_USHORT
#define UINT   EET_T_UINT
#define ULL    EET_T_ULONG_LONG
#define STR    EET_T_STRING

typedef Eet_Data_Descriptor                 E_Config_DD;

#else
#ifndef E_CONFIG_DATA_H
#define E_CONFIG_DATA_H

static inline Eina_Hash *
eet_eina_hash_add_alloc(Eina_Hash *hash, const void *key, void *data)
{
   if (!hash) hash = eina_hash_string_superfast_new(NULL);
   if (!hash) return NULL;
   eina_hash_add(hash, key, data);
   return hash;
}

EAPI E_Config_DD *e_config_descriptor_new(const char *name, int size);

#endif
#endif

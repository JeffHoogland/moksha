#ifdef E_TYPEDEFS

/** \def E_CONFIG_DD_NEW(str, typ)
 * is used to create definition of a struct  
 * \str str name to give to struct
 * \typ typ the actual struct type
 */
#define E_CONFIG_DD_NEW(str, typ) \
   e_config_descriptor_new(str, sizeof(typ))

/** \def E_CONFIG_DD_FREE(eed)
 * is used to free definition of a struct  
 * \eed eed the pointer created by \link #E_CONFIG_DD_NEW
 */
#define E_CONFIG_DD_FREE(eed) if (eed) { eet_data_descriptor_free((eed)); (eed) = NULL; }
#define E_CONFIG_VAL(edd, type, member, dtype) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define E_CONFIG_SUB(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)

/** \def E_CONFIG_LIST(edd, type, member, eddtype)
 * declares a struct member to be included definition
 * list type must be Evas_List and not Ecore_List  
 * \edd edd the pointer created by \link #E_CONFIG_DD_NEW
 * \type type struct type
 * \member member member of struct
 * \eddtype struct definition to use for each entry in the list
 */
#define E_CONFIG_LIST(edd, type, member, eddtype) EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)

/** \def E_CONFIG_HASH(edd, type, member, eddtype)
 * declares a struct member to be included definition
 * list type must be Evas_Hash and not Ecore_Hash  
 * \edd edd the pointer created by \link #E_CONFIG_DD_NEW
 * \type type struct type
 * \member member member of struct
 * \eddtype struct definition to use for each entry in the hash
 */
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

EAPI E_Config_DD *e_config_descriptor_new(const char *name, int size);

#endif
#endif

#ifndef _DB_H
#define _DB_H

/*
 * The API described in this file is used to map data from a database into a
 * struct. TODO: Arbitrary cleanup function for handling fields not described
 * by the base type, ie. not read in from the database.
 */

typedef struct _e_data_base_type E_Data_Base_Type;
typedef union _e_data_value E_Data_Value;
typedef struct _e_data_node E_Data_Node;
typedef enum _e_data_datatype E_Data_Datatype;

enum _e_data_datatype
{
   E_DATA_TYPE_INT,
   E_DATA_TYPE_STR,
   E_DATA_TYPE_PTR,
   E_DATA_TYPE_FLOAT,
   E_DATA_TYPE_LIST,
   E_DATA_TYPE_KEY
};

struct _e_data_base_type
{
   int                 size;
   Evas_List          *nodes;
};

union _e_data_value
{
   int                 int_val;
   float               float_val;
   char               *str_val;
};

struct _e_data_node
{
   char               *prefix;
   E_Data_Datatype     type;
   int                 offset;
   E_Data_Base_Type   *sub_type;
   E_Data_Value        def_val;
};

#define E_DATA_NODE(var, prefix, type, sub, struct_type, struct_member, def_val) \
{ \
  struct_type _cfg_dummy; \
  char *_cfg_p1, *_cfg_p2; \
  int _cfg_offset; \
  \
  _cfg_p1 = (char *)(&(_cfg_dummy)); \
  _cfg_p2 = (char *)(&(_cfg_dummy.struct_member)); \
  _cfg_offset = (int)(_cfg_p2 - _cfg_p1); \
  \
  e_data_type_add_node(var, prefix, type, sub, _cfg_offset, def_val); \
  var->size = sizeof(struct_type); \
}

/**
 * e_data_type_new - create the basis for a new datatype description
 *
 * Returns a pointer to base type for tracking the elements of a data type.
 */
E_Data_Base_Type   *e_data_type_new(void);

/**
 * e_data_type_add_node - add a type node to the base data type
 *
 * Add the necessary information for setting a data field in the base struct.
 */
void                e_data_type_add_node(E_Data_Base_Type * base,
					 char *prefix,
					 E_Data_Datatype type,
					 E_Data_Base_Type * list_type,
					 int offset, E_Data_Value def_val);

/**
 * e_data_load - allocate and assign the data of the base type
 *
 * Returns a newly allocated struct of the base type, with data filled from
 * the database file with keys prefixed by prefix.
 */
void               *e_data_load(char *file, char *prefix,
				E_Data_Base_Type * type);

/**
 * e_data_free - free the data allocated by e_data_load
 *
 * The data assigned by e_data_load is freed, but the struct itself is not,
 * since the programmer may have other fields in the struct that were not
 * allocated by e_data_load.
 */
void                e_data_free(E_Data_Base_Type * type, char *data);

#endif

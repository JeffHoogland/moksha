#ifndef E_CONFIG_H
#define E_CONFIG_H

#include "e.h"

typedef struct _E_Config_File         E_Config_File;
typedef struct _E_Config_Element      E_Config_Element;

/* something to check validity of config files where we get data from */
/* for now its just a 5 second timout so it will only invalidate */
/* if we havent looked for 5 seconds... BUT later when efsd is more solid */
/* we should use that to tell us when its invalid */
struct _E_Config_File
{
   char   *src;
   double  last_fetch;
};

struct _E_Config_Element 
{
   char   *src;
   char   *key;
   double  last_fetch;
   int     type;
   int     def_int_val;
   float   def_float_val;
   char   *def_str_val;
   void   *def_data_val;
   int     def_data_val_size;
   int     cur_int_val;
   float   cur_float_val;
   char   *cur_str_val;
   void   *cur_data_val;
   int     cur_data_val_size;
};

#define E_CFG_FILE(_var, _src) \
static E_Config_File _var = {_src, 0.0}
#define E_CONFIG_CHECK_VALIDITY(_var, _src) \
{ \
double __time; \
__time = ecore_get_time(); \
if (_var.last_fetch < (__time - 5.0)) { \
_var.last_fetch = __time;
#define E_CONFIG_CHECK_VALIDITY_END \
} \
}

typedef enum e_config_type
  {
    E_CFG_INT_T,
    E_CFG_FLOAT_T,
    E_CFG_STR_T,
    E_CFG_DATA_T,
  }
E_Config_Type;

#define E_CFG_INT(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_INT_T, \
_default, 0.0, NULL, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}

#define E_CFG_FLOAT(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_FLOAT_T, \
0, _default, NULL, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}

#define E_CFG_STR(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_STR_T, \
0, 0.0, _default, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}

#define E_CFG_DATA(_var, _src, _key, _default, _default_size) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_DATAT_T, \
0, 0.0, NULL, _default, _default_size, \
0, 0.0, NULL, NULL, 0, \
}

/* yes for now it only fetches them every 5 seconds - in the end i need a */
/* validity flag for the database file to know if it changed and only then */
/* get the value again. this is waiting for efsd to become more solid */
#define E_CFG_VALIDITY_CHECK(_var) \
{ \
double __time; \
__time = ecore_get_time(); \
if (_var.last_fetch < (__time - 5.0)) { \
int __cfg_ok = 0; \
_var.last_fetch = __time;

#define E_CFG_END_VALIDITY_CHECK \
} \
}

#define E_CONFIG_INT_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
E_DB_INT_GET(e_config_get(_var.src), _var.key, _var.cur_int_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_int_val = _var.def_int_val; \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_int_val;}

#define E_CONFIG_FLOAT_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
E_DB_FLOAT_GET(e_config_get(_var.src), _var.key, _var.cur_float_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_float_val = _var.def_float_val; \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_float_val;}

#define E_CONFIG_STR_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
if (_var.cur_str_val) free(_var.cur_str_val); \
_var.cur_str_val = NULL; \
E_DB_STR_GET(e_config_get(_var.src), _var.key, _var.cur_str_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_str_val = _var.def_str_val \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_str_val;}

#define E_CONFIG_DATA_GET(_var, _val, _size) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
if (_var.cur_data_val) free(_var.cur_data_val); \
_var.cur_data_val = NULL; \
_var.cur_data_size = 0; \
{ E_DB_File *__db; \
__db = e_db_open_read(e_config_get(_var.src)); \
if (__db) { \
_var.cur_data_val = e_db_data_get(__db, _var.key, &(_var.cur_data_size)); \
if (_var.cur_data_val) __cfg_ok = 1; \
e_db_close(__db); \
} \
} \
if (!__cfg_ok) { \
_var.cur_data_val = e_memdup(_var.def_data_val, _var.def_data_size); \
_var.cur_data_size = _var.def_data_size; \
} \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_data_val; \
_size = _var.cur_data_size;}


char *e_config_get(char *type);
void  e_config_init(void);
void  e_config_set_user_dir(char *dir);
char *e_config_user_dir(void);

typedef struct _e_config_base_type E_Config_Base_Type;
typedef struct _e_config_node      E_Config_Node;
typedef struct _e_config_value     E_Config_Value;
typedef enum   _e_config_datatype  E_Config_Datatype;

enum _e_config_datatype
{
   E_CFG_TYPE_INT,
   E_CFG_TYPE_STR,
   E_CFG_TYPE_FLOAT,
   E_CFG_TYPE_LIST,
   E_CFG_TYPE_KEY
};

struct _e_config_base_type
{
   int size;
   Evas_List nodes;
};

struct _e_config_node
{
   char               *prefix;
   E_Config_Datatype   type;
   int                 offset;
   E_Config_Base_Type *sub_type;
   int                 def_int;
   float               def_float;
   char               *def_str;
};

#define E_CONFIG_NODE(var, prefix, type, sub, struct_type, struct_member, def_int, def_float, def_str) \
{ \
  struct_type _cfg_dummy; \
  char *_cfg_p1, *_cfg_p2; \
  int _cfg_offset; \
  \
  _cfg_p1 = (char *)(&(_cfg_dummy)); \
  _cfg_p2 = (char *)(&(_cfg_dummy.struct_member)); \
  _cfg_offset = (int)(_cfg_p2 - _cfg_p1); \
  \
  e_config_type_add_node(var, prefix, type, sub, _cfg_offset, def_int, def_float, def_str); \
  var->size = sizeof(struct_type); \
}

E_Config_Value *e_config_value_get_int(E_Config_Value *handle, char *file,
				       char *prefix, char *key,
				       int *val_ret, int default_val);
E_Config_Value *e_config_value_get_str(E_Config_Value *handle, char *file,
				       char *prefix, char *key,
				       char **val_ret, char *default_val);
E_Config_Value *e_config_value_get_float(E_Config_Value *handle, char *file,
					 char *prefix, char *key,
					 float *val_ret, float default_val);
E_Config_Base_Type *e_config_type_new(void);
void                e_config_type_add_node(E_Config_Base_Type *base, 
					   char *prefix,
					   E_Config_Datatype type, 
					   E_Config_Base_Type *list_type,
					   int offset,
					   int def_int,
					   float def_float,
					   char *def_str);
void               *e_config_load(char *file, 
				  char *prefix,
				  E_Config_Base_Type *type);

#endif

#include "e.h"

/* local subsystem functions */

/* encode functions, Should these be global? */

/* local subsystem globals */
static Eet_Data_Descriptor *_e_ipc_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_double_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_int_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_int_list_edd = NULL;

/* externally accessible functions */
int
e_ipc_codec_init(void)
{
   _e_ipc_int_edd = E_CONFIG_DD_NEW("int", E_Ipc_Int);
   E_CONFIG_VAL(_e_ipc_int_edd, E_Ipc_Int, val, INT);

   _e_ipc_double_edd = E_CONFIG_DD_NEW("double", E_Ipc_Double);
   E_CONFIG_VAL(_e_ipc_double_edd, E_Ipc_Double, val, DOUBLE);
   
   _e_ipc_2int_edd = E_CONFIG_DD_NEW("2int", E_Ipc_2Int);
   E_CONFIG_VAL(_e_ipc_2int_edd, E_Ipc_2Int, val1, INT);
   E_CONFIG_VAL(_e_ipc_2int_edd, E_Ipc_2Int, val2, INT);

   _e_ipc_str_edd = E_CONFIG_DD_NEW("str", E_Ipc_Str);
   E_CONFIG_VAL(_e_ipc_str_edd, E_Ipc_Str, str, STR);

   _e_ipc_2str_edd = E_CONFIG_DD_NEW("2str", E_Ipc_2Str);
   E_CONFIG_VAL(_e_ipc_2str_edd, E_Ipc_2Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_2str_edd, E_Ipc_2Str, str2, STR);
   
   _e_ipc_str_list_edd = E_CONFIG_DD_NEW("str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_str_list_edd, E_Ipc_List, list, _e_ipc_str_edd);

   _e_ipc_str_int_edd = E_CONFIG_DD_NEW("str_int", E_Ipc_Str_Int);
   E_CONFIG_VAL(_e_ipc_str_int_edd, E_Ipc_Str_Int, str, STR);
   E_CONFIG_VAL(_e_ipc_str_int_edd, E_Ipc_Str_Int, val, INT);
   
   _e_ipc_str_int_list_edd = E_CONFIG_DD_NEW("str_int_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_str_int_list_edd, E_Ipc_List, list, _e_ipc_str_int_edd);

   _e_ipc_2str_int_edd = E_CONFIG_DD_NEW("2str_int", E_Ipc_2Str_Int);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, str1, STR);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, str2, STR);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, val, INT);
   
   _e_ipc_2str_int_list_edd = E_CONFIG_DD_NEW("2str_int_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_2str_int_list_edd, E_Ipc_List, list, _e_ipc_2str_int_edd);
   return 1;
}

void
e_ipc_codec_shutdown(void)
{
   E_CONFIG_DD_FREE(_e_ipc_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_double_edd);
   E_CONFIG_DD_FREE(_e_ipc_2int_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_int_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_int_list_edd);
}

int
e_ipc_codec_int_dec(char *data, int bytes, int *dest)
{
   E_Ipc_Int *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->val;
   free(dat);
   return 1;
}

void *
e_ipc_codec_int_enc(int val, int *size_ret)
{
   E_Ipc_Int dat;
   
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_int_edd, &dat, size_ret);
}

int
e_ipc_codec_double_dec(char *data, int bytes, double *dest)
{
   E_Ipc_Double *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_double_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->val;
   free(dat);
   return 1;
}

void *
e_ipc_codec_double_enc(double val, int *size_ret)
{
   E_Ipc_Double dat;
   
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_double_edd, &dat, size_ret);
}

int
e_ipc_codec_2int_dec(char *data, int bytes, int *dest, int *dest2)
{
   E_Ipc_2Int *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->val1;
   if (dest2) *dest2 = dat->val2;
   free(dat);
   return 1;
}

void *
e_ipc_codec_2int_enc(int val1, int val2, int *size_ret)
{
   E_Ipc_2Int dat;
   
   dat.val1 = val1;
   dat.val2 = val2;
   return eet_data_descriptor_encode(_e_ipc_2int_edd, &dat, size_ret);
}

int
e_ipc_codec_str_dec(char *data, int bytes, char **dest)
{
   E_Ipc_Str *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->str;
   free(dat);
   return 1;
}

void *
e_ipc_codec_str_enc(char *str, int *size_ret)
{
   E_Ipc_Str dat;
   
   dat.str = str;
   return eet_data_descriptor_encode(_e_ipc_str_edd, &dat, size_ret);
}

int
e_ipc_codec_2str_dec(char *data, int bytes, E_Ipc_2Str **dest)
{
   E_Ipc_2Str *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

void *
e_ipc_codec_2str_enc(char *str1, char *str2, int *size_ret)
{
   E_Ipc_2Str dat;
   
   dat.str1 = str1;
   dat.str2 = str2;
   return eet_data_descriptor_encode(_e_ipc_2str_edd, &dat, size_ret);
}

int
e_ipc_codec_str_list_dec(char *data, int bytes, Evas_List **dest)
{
   E_Ipc_List *dat;
   Evas_List *list = NULL, *l;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_list_edd, data, bytes);
   if (!dat) return 0;
   for (l = dat->list; l; l = l->next)
     {
        E_Ipc_Str *str_node;
	
        str_node = l->data;
	list = evas_list_append(list, str_node->str);
     }
   if (dest) *dest = list;
   while (dat->list)
     {
	free(dat->list->data);
	dat->list = evas_list_remove_list(dat->list, dat->list);
     }
   free(dat);
   return 1;
}

void *
e_ipc_codec_str_list_enc(Evas_List *list, int *size_ret)
{
   E_Ipc_List dat;
   Evas_List *l;
   void *data;
   
   dat.list = NULL;
   for (l = list; l; l = l->next)
     {
	E_Ipc_Str *str_node;
	
	str_node = malloc(sizeof(E_Ipc_Str));
	str_node->str = l->data;
	dat.list = evas_list_append(dat.list, str_node);
     }
   data = eet_data_descriptor_encode(_e_ipc_str_list_edd, &dat, size_ret);
   while (dat.list)
     {
	free(dat.list->data);
	dat.list = evas_list_remove_list(dat.list, dat.list);
     }
   return data;
}

int
e_ipc_codec_str_int_dec(char *data, int bytes, E_Ipc_Str_Int **dest)
{
   E_Ipc_Str_Int *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

void *
e_ipc_codec_str_int_enc(char *str, int val, int *size_ret)
{
   E_Ipc_Str_Int dat;
   
   dat.str = str;
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_str_int_edd, &dat, size_ret);
}

int
e_ipc_codec_str_int_list_dec(char *data, int bytes, Evas_List **dest)
{
   E_Ipc_List *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_int_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

void *
e_ipc_codec_str_int_list_enc(Evas_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_str_int_list_edd, &dat, size_ret);
}

int
e_ipc_codec_2str_int_dec(char *data, int bytes, E_Ipc_2Str_Int **dest)
{
   E_Ipc_2Str_Int *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

void *
e_ipc_codec_2str_int_enc(char *str1, char *str2, int val, int *size_ret)
{
   E_Ipc_2Str_Int dat;
   
   dat.str1 = str1;
   dat.str2 = str2;
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_2str_int_edd, &dat, size_ret);
}

int
e_ipc_codec_2str_int_list_dec(char *data, int bytes, Evas_List **dest)
{
   E_Ipc_List *dat;
   
   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_int_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

void *
e_ipc_codec_2str_int_list_enc(Evas_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_2str_int_list_edd, &dat, size_ret);
}


/* local subsystem globals */


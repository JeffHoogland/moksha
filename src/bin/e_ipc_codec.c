#include "e.h"

#ifdef USE_IPC
/* local subsystem functions */

/* encode functions, Should these be global? */

/* local subsystem globals */
static Eet_Data_Descriptor *_e_ipc_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_double_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_int_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_2str_int_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_4int_2str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_4int_2str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_5int_2str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_5int_2str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_3int_4str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_3int_4str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_3int_3str_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_3int_3str_list_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_4int_edd = NULL;
static Eet_Data_Descriptor *_e_ipc_str_4int_list_edd = NULL;

#define E_IPC_DD(Edd, Eddc, Name, Type)		\
  Eddc.name = Name;				\
  Eddc.size = sizeof (Type);			\
  Edd = eet_data_descriptor_stream_new(&Eddc);

/* externally accessible functions */
EINTERN int
e_ipc_codec_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "int", sizeof (E_Ipc_Int)))
     return 0;
   _e_ipc_int_edd = eet_data_descriptor_stream_new(&eddc);
   E_CONFIG_VAL(_e_ipc_int_edd, E_Ipc_Int, val, INT);

   E_IPC_DD(_e_ipc_double_edd, eddc, "double", E_Ipc_Double);
   E_CONFIG_VAL(_e_ipc_double_edd, E_Ipc_Double, val, DOUBLE);

   E_IPC_DD(_e_ipc_2int_edd, eddc, "2int", E_Ipc_2Int);
   E_CONFIG_VAL(_e_ipc_2int_edd, E_Ipc_2Int, val1, INT);
   E_CONFIG_VAL(_e_ipc_2int_edd, E_Ipc_2Int, val2, INT);

   E_IPC_DD(_e_ipc_str_edd, eddc, "str", E_Ipc_Str);
   E_CONFIG_VAL(_e_ipc_str_edd, E_Ipc_Str, str, STR);

   E_IPC_DD(_e_ipc_str_list_edd, eddc, "str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_str_list_edd, E_Ipc_List, list, _e_ipc_str_edd);

   E_IPC_DD(_e_ipc_2str_edd, eddc, "2str", E_Ipc_2Str);
   E_CONFIG_VAL(_e_ipc_2str_edd, E_Ipc_2Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_2str_edd, E_Ipc_2Str, str2, STR);

   E_IPC_DD(_e_ipc_2str_list_edd, eddc, "2str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_2str_list_edd, E_Ipc_List, list, _e_ipc_2str_edd);

   E_IPC_DD(_e_ipc_str_int_edd, eddc, "str_int", E_Ipc_Str_Int);
   E_CONFIG_VAL(_e_ipc_str_int_edd, E_Ipc_Str_Int, str, STR);
   E_CONFIG_VAL(_e_ipc_str_int_edd, E_Ipc_Str_Int, val, INT);

   E_IPC_DD(_e_ipc_str_int_list_edd, eddc, "str_int_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_str_int_list_edd, E_Ipc_List, list, _e_ipc_str_int_edd);

   E_IPC_DD(_e_ipc_2str_int_edd, eddc, "2str_int", E_Ipc_2Str_Int);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, str1, STR);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, str2, STR);
   E_CONFIG_VAL(_e_ipc_2str_int_edd, E_Ipc_2Str_Int, val, INT);

   E_IPC_DD(_e_ipc_2str_int_list_edd, eddc, "2str_int_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_2str_int_list_edd, E_Ipc_List, list, _e_ipc_2str_int_edd);

   E_IPC_DD(_e_ipc_4int_2str_edd, eddc, "4int_2str", E_Ipc_4Int_2Str);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, val1, INT);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, val2, INT);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, val3, INT);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, val4, INT);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_4int_2str_edd, E_Ipc_4Int_2Str, str2, STR);

   E_IPC_DD(_e_ipc_4int_2str_list_edd, eddc, "4int_2str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_4int_2str_list_edd, E_Ipc_List, list, _e_ipc_4int_2str_edd);

   E_IPC_DD(_e_ipc_5int_2str_edd, eddc, "5int_2str", E_Ipc_5Int_2Str);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, val1, INT);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, val2, INT);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, val3, INT);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, val4, INT);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, val5, INT);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_5int_2str_edd, E_Ipc_5Int_2Str, str2, STR);

   E_IPC_DD(_e_ipc_5int_2str_list_edd, eddc, "5int_2str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_5int_2str_list_edd, E_Ipc_List, list, _e_ipc_5int_2str_edd);

   E_IPC_DD(_e_ipc_3int_4str_edd, eddc, "3int_4str", E_Ipc_3Int_4Str);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, val1, INT);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, val2, INT);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, val3, INT);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, str2, STR);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, str3, STR);
   E_CONFIG_VAL(_e_ipc_3int_4str_edd, E_Ipc_3Int_4Str, str4, STR);

   E_IPC_DD(_e_ipc_3int_4str_list_edd, eddc, "3int_4str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_3int_4str_list_edd, E_Ipc_List, list, _e_ipc_3int_4str_edd);

   E_IPC_DD(_e_ipc_3int_3str_edd, eddc, "3int_3str", E_Ipc_3Int_3Str);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, val1, INT);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, val2, INT);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, val3, INT);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, str1, STR);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, str2, STR);
   E_CONFIG_VAL(_e_ipc_3int_3str_edd, E_Ipc_3Int_3Str, str3, STR);

   E_IPC_DD(_e_ipc_3int_3str_list_edd, eddc, "3int_3str_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_3int_3str_list_edd, E_Ipc_List, list, _e_ipc_3int_3str_edd);

   E_IPC_DD(_e_ipc_str_4int_edd, eddc, "str_4int", E_Ipc_Str_4Int);
   E_CONFIG_VAL(_e_ipc_str_4int_edd, E_Ipc_Str_4Int, str, STR);
   E_CONFIG_VAL(_e_ipc_str_4int_edd, E_Ipc_Str_4Int, val1, INT);
   E_CONFIG_VAL(_e_ipc_str_4int_edd, E_Ipc_Str_4Int, val2, INT);
   E_CONFIG_VAL(_e_ipc_str_4int_edd, E_Ipc_Str_4Int, val3, INT);
   E_CONFIG_VAL(_e_ipc_str_4int_edd, E_Ipc_Str_4Int, val4, INT);

   E_IPC_DD(_e_ipc_str_4int_list_edd, eddc, "str_4int_list", E_Ipc_List);
   E_CONFIG_LIST(_e_ipc_str_4int_list_edd, E_Ipc_List, list, _e_ipc_str_4int_edd);

   return 1;
}

EINTERN void
e_ipc_codec_shutdown(void)
{
   E_CONFIG_DD_FREE(_e_ipc_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_double_edd);
   E_CONFIG_DD_FREE(_e_ipc_2int_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_int_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_int_edd);
   E_CONFIG_DD_FREE(_e_ipc_2str_int_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_4int_2str_edd);
   E_CONFIG_DD_FREE(_e_ipc_4int_2str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_5int_2str_edd);
   E_CONFIG_DD_FREE(_e_ipc_5int_2str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_3int_4str_edd);
   E_CONFIG_DD_FREE(_e_ipc_3int_4str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_3int_3str_edd);
   E_CONFIG_DD_FREE(_e_ipc_3int_3str_list_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_4int_edd);
   E_CONFIG_DD_FREE(_e_ipc_str_4int_list_edd);
}

EAPI int
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

EAPI void *
e_ipc_codec_int_enc(int val, int *size_ret)
{
   E_Ipc_Int dat;

   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_int_edd, &dat, size_ret);
}

EAPI int
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

EAPI void *
e_ipc_codec_double_enc(double val, int *size_ret)
{
   E_Ipc_Double dat;

   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_double_edd, &dat, size_ret);
}

EAPI int
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

EAPI void *
e_ipc_codec_2int_enc(int val1, int val2, int *size_ret)
{
   E_Ipc_2Int dat;

   dat.val1 = val1;
   dat.val2 = val2;
   return eet_data_descriptor_encode(_e_ipc_2int_edd, &dat, size_ret);
}

EAPI int
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

EAPI void *
e_ipc_codec_str_enc(const char *str, int *size_ret)
{
   E_Ipc_Str dat;

   dat.str = (char *) str;
   return eet_data_descriptor_encode(_e_ipc_str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_2str_dec(char *data, int bytes, E_Ipc_2Str **dest)
{
   E_Ipc_2Str *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_2str_enc(const char *str1, const char *str2, int *size_ret)
{
   E_Ipc_2Str dat;

   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   return eet_data_descriptor_encode(_e_ipc_2str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_2str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_2str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_2str_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;
   Eina_List *list = NULL, *l;
   E_Ipc_Str *str_node;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_list_edd, data, bytes);
   if (!dat) return 0;
   EINA_LIST_FOREACH(dat->list, l, str_node)
     {
	list = eina_list_append(list, str_node->str);
     }
   if (dest) *dest = list;
   E_FREE_LIST(dat->list, free);
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   Eina_List *l;
   E_Ipc_Str *str_node;
   char *str;
   void *data;

   dat.list = NULL;
   EINA_LIST_FOREACH(list, l, str)
     {
	str_node = malloc(sizeof(E_Ipc_Str));
	str_node->str = str;
	dat.list = eina_list_append(dat.list, str_node);
     }
   data = eet_data_descriptor_encode(_e_ipc_str_list_edd, &dat, size_ret);
   E_FREE_LIST(dat.list, free);
   return data;
}

EAPI int
e_ipc_codec_str_int_dec(char *data, int bytes, E_Ipc_Str_Int **dest)
{
   E_Ipc_Str_Int *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_str_int_enc(const char *str, int val, int *size_ret)
{
   E_Ipc_Str_Int dat;

   dat.str = (char *) str;
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_str_int_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_str_int_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_int_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_str_int_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_str_int_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_2str_int_dec(char *data, int bytes, E_Ipc_2Str_Int **dest)
{
   E_Ipc_2Str_Int *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_int_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_2str_int_enc(const char *str1, const char *str2, int val, int *size_ret)
{
   E_Ipc_2Str_Int dat;

   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   dat.val = val;
   return eet_data_descriptor_encode(_e_ipc_2str_int_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_2str_int_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_2str_int_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_2str_int_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_2str_int_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_4int_2str_dec(char *data, int bytes, E_Ipc_4Int_2Str **dest)
{
   E_Ipc_4Int_2Str *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_4int_2str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_4int_2str_enc(int val1, int val2, int val3, int val4, const char *str1, const char *str2, int *size_ret)
{
   E_Ipc_4Int_2Str dat;

   dat.val1 = val1;
   dat.val2 = val2;
   dat.val3 = val3;
   dat.val4 = val4;
   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   return eet_data_descriptor_encode(_e_ipc_4int_2str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_4int_2str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_4int_2str_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_4int_2str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_4int_2str_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_5int_2str_dec(char *data, int bytes, E_Ipc_5Int_2Str **dest)
{
   E_Ipc_5Int_2Str *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_5int_2str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_5int_2str_enc(int val1, int val2, int val3, int val4, int val5, const char *str1, const char *str2, int *size_ret)
{
   E_Ipc_5Int_2Str dat;

   dat.val1 = val1;
   dat.val2 = val2;
   dat.val3 = val3;
   dat.val4 = val4;
   dat.val5 = val5;
   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   return eet_data_descriptor_encode(_e_ipc_5int_2str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_5int_2str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_5int_2str_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_5int_2str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_5int_2str_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_3int_4str_dec(char *data, int bytes, E_Ipc_3Int_4Str **dest)
{
   E_Ipc_3Int_4Str *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_3int_4str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_3int_4str_enc(int val1, int val2, int val3, const char *str1, const char *str2, const char *str3, const char *str4, int *size_ret)
{
   E_Ipc_3Int_4Str dat;

   dat.val1 = val1;
   dat.val2 = val2;
   dat.val3 = val3;
   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   dat.str3 = (char *) str3;
   dat.str4 = (char *) str4;
   return eet_data_descriptor_encode(_e_ipc_3int_4str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_3int_4str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_3int_4str_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_3int_4str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_3int_4str_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_3int_3str_dec(char *data, int bytes, E_Ipc_3Int_3Str **dest)
{
   E_Ipc_3Int_3Str *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_3int_3str_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat;
   return 1;
}

EAPI void *
e_ipc_codec_3int_3str_enc(int val1, int val2, int val3, const char *str1, const char *str2, const char *str3, int *size_ret)
{
   E_Ipc_3Int_3Str dat;

   dat.val1 = val1;
   dat.val2 = val2;
   dat.val3 = val3;
   dat.str1 = (char *) str1;
   dat.str2 = (char *) str2;
   dat.str3 = (char *) str3;
   return eet_data_descriptor_encode(_e_ipc_3int_3str_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_3int_3str_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_3int_3str_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_3int_3str_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_3int_3str_list_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_str_4int_dec(char *data, int bytes, E_Ipc_Str_4Int **dest)
{
  E_Ipc_Str_4Int *dat;

  if (!data) return 0;
  dat = eet_data_descriptor_decode(_e_ipc_str_4int_edd, data, bytes);
  if (!dat) return 0;
  if (dest) *dest = dat;
  return 1;
}

EAPI void *
e_ipc_codec_str_4int_enc(const char *str1, int val1, int val2, int val3, int val4, int *size_ret)
{
  E_Ipc_Str_4Int dat;

  dat.str = (char *) str1;
  dat.val1 = val1;
  dat.val2 = val2;
  dat.val3 = val3;
  dat.val4 = val4;

  return eet_data_descriptor_encode(_e_ipc_str_4int_edd, &dat, size_ret);
}

EAPI int
e_ipc_codec_str_4int_list_dec(char *data, int bytes, Eina_List **dest)
{
   E_Ipc_List *dat;

   if (!data) return 0;
   dat = eet_data_descriptor_decode(_e_ipc_str_4int_list_edd, data, bytes);
   if (!dat) return 0;
   if (dest) *dest = dat->list;
   free(dat);
   return 1;
}

EAPI void *
e_ipc_codec_str_4int_list_enc(Eina_List *list, int *size_ret)
{
   E_Ipc_List dat;
   dat.list = list;
   return eet_data_descriptor_encode(_e_ipc_str_4int_list_edd, &dat, size_ret);
}

/* local subsystem globals */

#endif

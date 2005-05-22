/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Ipc_Int         E_Ipc_Int;
typedef struct _E_Ipc_Double      E_Ipc_Double;
typedef struct _E_Ipc_2Int        E_Ipc_2Int;
typedef struct _E_Ipc_Str         E_Ipc_Str;
typedef struct _E_Ipc_Str_List    E_Ipc_Str_List;

#else
#ifndef E_IPC_CODEC_H
#define E_IPC_CODEC_H

struct _E_Ipc_Int
{
   int val;
};

struct _E_Ipc_Double
{
   double val;
};

struct _E_Ipc_2Int
{
   int val1, val2;
};

struct _E_Ipc_Str
{
   char *str;
};

struct _E_Ipc_Str_List
{
   Evas_List *list;
};

EAPI int      e_ipc_codec_init(void);
EAPI void     e_ipc_codec_shutdown(void);

EAPI int      e_ipc_codec_int_dec(char *data, int bytes, int *dest);
EAPI void    *e_ipc_codec_int_enc(int val, int *size_ret);
EAPI int      e_ipc_codec_double_dec(char *data, int bytes, double *dest);
EAPI void    *e_ipc_codec_double_enc(double val, int *size_ret);
EAPI int      e_ipc_codec_2int_dec(char *data, int bytes, int *dest, int *dest2x);
EAPI void    *e_ipc_codec_2int_enc(int val1, int val2, int *size_ret);
EAPI int      e_ipc_codec_str_dec(char *data, int bytes, char **dest);
EAPI void    *e_ipc_codec_str_enc(char *str, int *size_ret);
EAPI int      e_ipc_codec_str_list_dec(char *data, int bytes, Evas_List **dest);
EAPI void    *e_ipc_codec_str_list_enc(Evas_List *list, int *size_ret);
    
#endif
#endif

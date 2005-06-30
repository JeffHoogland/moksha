/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* NOTE:
 * 
 * This is a very SPECIAL file. This servers as a kind of "auto code generator"
 * to handle the encoder, decoder etc. aspects of ipc requests. the aim here
 * is to make writing support for a new opcode simple and compact. It intends
 * to centralize the handling into 1 spot, instead of having ti spread around
 * the code in N different places, as well as providing common construct macros
 * to help make the code more compact and easier to create.
 */

/* This is a bunch of convenience stuff for this to work */
#ifndef E_IPC_HANDLERS_H
# define E_IPC_HANDLERS_H

/* 
 * add a new ooption for enlightenment_remote
 * OP(opt, num_params, description, num_expected_replies, HDL) 
 */
# define OP(__a, __b, __c, __d, __e) \
     {__a, __b, __c, __d, __e},


# define STRING(__str, HDL) \
case HDL: \
if (e->data) { \
   char *__str = NULL; \
   if (e_ipc_codec_str_dec(e->data, e->size, &__str)) {
# define END_STRING(__str) \
      free(__str); \
   } \
} \
break;

# define STRING2(__str1, __str2, __2str, HDL) \
case HDL: \
if (e->data) { \
   char *__str1 = NULL, *__str2 = NULL; \
   E_Ipc_2Str *__2str = NULL; \
   __2str = calloc(1, sizeof(E_Ipc_2Str)); \
   if (e_ipc_codec_2str_dec(e->data, e->size, &(__2str))) { \
      __str1 = __2str->str1; \
      __str2 = __2str->str2;
# define END_STRING2(__2str) \
      free(__2str->str1); \
      free(__2str->str2); \
      free(__2str); \
   } \
} \
break;

/**
 * INT3_STRING3:
 * Decode event data of type E_Ipc_3Int_3Str
 */
# define INT3_STRING3(__3int_3str, HDL) \
case HDL: \
if (e->data) { \
   E_Ipc_3Int_3Str *__3int_3str = NULL; \
   __3int_3str = calloc(1, sizeof(E_Ipc_3Int_3Str)); \
   if (e_ipc_codec_3int_3str_dec(e->data, e->size, &(__3int_3str))) {
# define END_INT3_STRING3(__3int_3str) \
      free(__3int_3str->str1); \
      free(__3int_3str->str2); \
      free(__3int_3str->str3); \
      free(__3int_3str); \
   } \
} \
break;

/**
 * INT4_STRING2:
 * Decode event data of type E_Ipc_4Int_2Str
 */
# define INT4_STRING2(__4int_2str, HDL) \
case HDL: \
if (e->data) { \
   E_Ipc_4Int_2Str *__4int_2str = NULL; \
   __4int_2str = calloc(1, sizeof(E_Ipc_4Int_2Str)); \
   if (e_ipc_codec_4int_2str_dec(e->data, e->size, &(__4int_2str))) {
# define END_INT4_STRING2(__4int_2str) \
      free(__4int_2str->str1); \
      free(__4int_2str->str2); \
      free(__4int_2str); \
   } \
} \
break;

# define STRING2_INT(__str1, __str2, __int, __2str_int, HDL) \
case HDL: \
if (e->data) { \
   char *__str1 = NULL, *__str2 = NULL; \
   int __int; \
   E_Ipc_2Str_Int *__2str_int = NULL; \
   __2str_int = calloc(1, sizeof(E_Ipc_2Str_Int)); \
   if (e_ipc_codec_2str_int_dec(e->data, e->size, &(__2str_int))) { \
      __str1 = __2str_int->str1; \
      __str2 = __2str_int->str2; \
      __int  = __2str_int->val;
# define END_STRING2_INT(__2str_int) \
      free(__2str_int->str1); \
      free(__2str_int->str2); \
      free(__2str_int); \
   } \
} \
break;

# define START_DOUBLE(__dbl, HDL) \
case HDL: \
if (e->data) { \
   double __dbl = 0.0; \
   if (e_ipc_codec_double_dec(e->data, e->size, &(__dbl))) {
# define END_DOUBLE \
   } \
} \
break;

# define START_INT(__int, HDL) \
case HDL: \
if (e->data) { \
   int __int = 0; \
   if (e_ipc_codec_int_dec(e->data, e->size, &(__int))) {
# define END_INT \
   } \
} \
break;

# define START_2INT(__int1, __int2, HDL) \
case HDL: \
if (e->data) { \
   int __int1 = 0; \
   int __int2 = 0; \
   if (e_ipc_codec_2int_dec(e->data, e->size, &(__int1), &(__int2))) {
# define END_2INT \
   } \
} \
break;

# define RESPONSE(__res, __store, HDL) \
   __store *__res = calloc(1, sizeof(__store)); \
   if (e->data) {
#define END_RESPONSE(__res, __type) \
   } \
   ecore_event_add(__type, __res, NULL, NULL);

# define SAVE e_config_save_queue()

# define REQ_STRING(__str, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_str_enc(__str, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_2STRING(__str1, __str2, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_2str_enc(__str1, __str2, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

#define REQ_3INT_3STRING_START(HDL) \
case HDL: { void *data; int bytes; \

#define REQ_3INT_3STRING_END(__val1, __val2, __val3, __str1, __str2, __str3, HDL) \
   data = e_ipc_codec_3int_3str_enc(__val1, __val2, __val3, __str1, __str2, __str3, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;


#define REQ_4INT_2STRING_START(HDL) \
case HDL: { void *data; int bytes; \

#define REQ_4INT_2STRING_END(__val1, __val2, __val3, __val4, __str1, __str2, HDL) \
   data = e_ipc_codec_4int_2str_enc(__val1, __val2, __val3, __val4, __str1, __str2, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_2STRING_INT(__str1, __str2, __int, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_2str_int_enc(__str1, __str2, __int, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_DOUBLE(__dbl, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_double_enc(__dbl, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_INT_START(HDL) \
case HDL: { void *data; int bytes;

# define REQ_INT_END(__int, HDL) \
   data = e_ipc_codec_int_enc(__int, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_INT(__int, HDL) \
   REQ_INT_START(HDL) \
   REQ_INT_END(__int, HDL)

# define REQ_2INT(__int1, __int2, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_2int_enc(__int1, __int2, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define REQ_NULL(HDL) \
case HDL: \
   ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, NULL, 0); \
break;

# define FREE_LIST(__list) \
while (__list) { \
   free(__list->data); \
   __list = evas_list_remove_list(__list, __list); \
}

# define SEND_DATA(__opcode) \
ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __opcode, 0, 0, 0, data, bytes); \
free(data);

# define STRING_INT_LIST(__v, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    if (e_ipc_codec_str_int_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_Str_Int *__v; \
	  __v = l->data;
#define END_STRING_INT_LIST(__v) \
	  free(__v->str); \
	  free(__v); \
       } \
       evas_list_free(dat); \
    } \
    reply_count++; \
 } \
   break;

#define SEND_STRING_INT_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_Str_Int *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_Str_Int));
#define END_SEND_STRING_INT_LIST(__v1, __op) \
       dat = evas_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_str_int_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * INT3_STRING3:
 * Decode event data is a list of E_Ipc_3Int_3Str objects and iterate
 * the list. For each iteration the object __v will contain a decoded list
 * element.
 *
 * Use END_INT3_STRING3_LIST to terminate the loop and free all data. 
 */
#define INT3_STRING3_LIST(__v, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    if (e_ipc_codec_3int_3str_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_3Int_3Str *__v; \
	  __v = l->data;
#define END_INT3_STRING3_LIST(__v) \
          free(__v->str1); \
          free(__v->str2); \
          free(__v->str3); \
          free(__v); \
       } \
       evas_list_free(dat); \
    } else { \
       printf("Decode FAILURE!!!\n"); \
    } \
    reply_count++; \
 } \
  break;

/** 
 * SEND_INT3_STRING3_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_3Int_3Str. 
 *
 * Use END_SEND_INT3_STRING3_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_INT3_STRING3_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_3Int_3Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_3Int_3Str));
#define END_SEND_INT3_STRING3_LIST(__v1, __op) \
       dat = evas_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_3int_3str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * INT4_STRING2:
 * Decode event data is a list of E_Ipc_4Int_2Str objects and iterate
 * the list. For each iteration the object __v will contain a decoded list
 * element.
 *
 * Use END_INT4_STRING2_LIST to terminate the loop and free all data. 
 */
#define INT4_STRING2_LIST(__v, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    if (e_ipc_codec_4int_2str_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_4Int_2Str *__v; \
	  __v = l->data;
#define END_INT4_STRING2_LIST(__v) \
          free(__v->str1); \
          free(__v->str2); \
          free(__v); \
       } \
       evas_list_free(dat); \
    } else { \
       printf("Decode FAILURE!!!\n"); \
    } \
    reply_count++; \
 } \
  break;

/** 
 * SEND_INT4_STRING2_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_4Int_2Str. 
 *
 * Use END_SEND_INT4_STRING2_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_INT4_STRING2_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_4Int_2Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_4Int_2Str));
#define END_SEND_INT4_STRING2_LIST(__v1, __op) \
       dat = evas_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_4int_2str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * STRING2_INT_LIST:
 * Decode event data which is a list of E_Ipc_2Str_Int objects and iterate 
 * the list. For each iteration the object __v will contain a decoded list
 * element. 
 *
 * Use END_STRING2_INT_LIST to terminate the loop and free all data.
 */
#define STRING2_INT_LIST(__v, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    if (e_ipc_codec_2str_int_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_2Str_Int *__v; \
	  __v = l->data;
#define END_STRING2_INT_LIST(__v) \
	  free(__v->str1); \
	  free(__v->str2); \
	  free(__v); \
       } \
       evas_list_free(dat); \
    } \
    reply_count++; \
 } \
   break;

/** 
 * SEND_STRING2_INT_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_2Str_Int. 
 *
 * Use END_SEND_STRING2_INT_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_STRING2_INT_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Evas_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_2Str_Int *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_2Str_Int));
#define END_SEND_STRING2_INT_LIST(__v1, __op) \
       dat = evas_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_2str_int_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

#define SEND_STRING(__str, __op, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_str_enc(__str, &bytes); \
   if (data) { \
      ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __op, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

#define SEND_DOUBLE(__dbl, __op, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_double_enc(__dbl, &bytes); \
   if (data) { \
      ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __op, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define SEND_INT(__int, __op, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_int_enc(__int, &bytes); \
   if (data) { \
      ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __op, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

# define SEND_2INT(__int1, __int2,__op, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_2int_enc(__int1, __int2, &bytes); \
   if (data) { \
      ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __op, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

#define LIST_DATA() \
   Evas_List *dat = NULL, *l; \
   void *data; int bytes;

#define ENCODE(__dat, __enc) \
   data = __enc(__dat, &bytes);

#define FOR(__start) \
   for (l = __start; l; l = l->next)
#define GENERIC(HDL) \
 case HDL: {

#define END_GENERIC() \
   } \
break;

#define LIST() \
   Evas_List *dat = NULL, *l;

#define DECODE(__dec) \
   if (__dec(e->data, e->size, &dat))

# define E_PATH_GET(__path, __str) \
   E_Path *__path = NULL; \
   if (!strcmp(__str, "data")) \
     __path = path_data; \
   else if (!strcmp(__str, "images")) \
     __path = path_images; \
   else if (!strcmp(__str, "fonts")) \
     __path = path_fonts; \
   else if (!strcmp(__str, "themes")) \
     __path = path_themes; \
   else if (!strcmp(__str, "init")) \
     __path = path_init; \
   else if (!strcmp(__str, "icons")) \
     __path = path_icons; \
   else if (!strcmp(__str, "modules")) \
     __path = path_modules; \
   else if (!strcmp(__str, "backgrounds")) \
     __path = path_backgrounds; 


#endif










/*
 * ****************
 * IPC handlers
 * ****************
 */

/* what a handler looks like
 * 
 * E_REMOTE_OPTIONS
 *   OP(opt, num_params, description, num_expected_replies, HDL)
 * E_REMOTE_OUT
 *   ...
 * E_WM_IN
 *   ...
 * E_REMOTE_IN
 *   ...
 * E_LIB_IN
 *   ...
 */

#if 0
{
#endif
   
   
   
   
   
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_LOAD
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-module-load", 1, "Loads the module named 'OPT1' into memory", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (!e_module_find(s)) {
      e_module_new(s);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_UNLOAD
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-module-unload", 1, "Unloads the module named 'OPT1' from memory", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_disable(m);
      e_object_del(E_OBJECT(m));
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
   
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_ENABLE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-module-enable", 1, "Enable the module named 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_enable(m);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_DISABLE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-module-disable", 1, "Disable the module named 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_disable(m);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-module-list", 0, "List all loaded modules", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING_INT_LIST(e_module_list(), E_Module, mod, v, HDL);
   v->str = mod->name;
   v->val = mod->enabled;
   END_SEND_STRING_INT_LIST(v, E_IPC_OP_MODULE_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_MODULE_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING_INT_LIST(v, HDL);
   printf("REPLY: \"%s\" ENABLED %i\n", v->str, v->val);
   END_STRING_INT_LIST(v);
#elif (TYPE == E_LIB_IN)
   GENERIC(HDL);
   Evas_List *dat = NULL;
   DECODE(e_ipc_codec_str_int_list_dec) {
      LIST();
      int count;
      RESPONSE(r, E_Response_Module_List, HDL);

      /* FIXME - this is a mess, needs to be merged into macros... */
      count = evas_list_count(dat);
      r->modules = malloc(sizeof(E_Response_Module_Data *) * count);
      r->count = count;

      count = 0;
      FOR(dat) {
	 E_Response_Module_Data *md;
	 E_Ipc_Str_Int *v;
	 
	 v = l->data;
	 md = malloc(sizeof(E_Response_Module_Data));
	 
	 md->name = v->str;
	 md->enabled = v->val;
	 r->modules[count] = md;
	 count++;
      }
      END_RESPONSE(r, E_RESPONSE_MODULE_LIST); /* FIXME - need a custom free */
   }
   END_GENERIC();
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_BG_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-bg-set", 1, "Set the default background edje to the desktop background in the file 'OPT1' (must be a full path)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   E_FREE(e_config->desktop_default_background);
   e_config->desktop_default_background = strdup(s);
   e_bg_update();
   SAVE;
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_BG_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-bg-get", 0, "Get the default background edje file path", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->desktop_default_background, E_IPC_OP_BG_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_BG_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
   STRING(s, HDL);
   RESPONSE(r, E_Response_Background_Get, HDL);
   r->file = strdup(s);
   END_RESPONSE(r, E_RESPONSE_BACKGROUND_GET);
   END_STRING(s);
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_FONT_AVAILABLE_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-available-list", 0, "List all available fonts", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   LIST_DATA();
   E_Font_Available *fa;
   Evas_List *fa_list;
   fa_list = e_font_available_list();
   FOR(fa_list) { fa = l->data;
      dat = evas_list_append(dat, fa->name);
   }
   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_FONT_AVAILABLE_LIST_REPLY);
   evas_list_free(dat);
   e_font_available_list_free(fa_list);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_FONT_AVAILABLE_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   GENERIC(HDL);
   LIST();
   DECODE(e_ipc_codec_str_list_dec) {
      FOR(dat) {
	 printf("REPLY: \"%s\"\n", (char *)(l->data));
      }
      FREE_LIST(dat);
   }
   END_GENERIC();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_APPLY
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-apply", 0, "Apply font settings changes", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   e_font_apply();
   SAVE;
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
 
/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_APPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP(	"-font-fallback-append", 
	1 /*num_params*/, 
	"Append OPT1 to the fontset", 
	0 /*num_expected_replies*/, 
	HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   e_font_fallback_append(s);
   SAVE;   
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_PREPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP(	"-font-fallback-prepend", 
	1 /*num_params*/, 
	"Prepend OPT1 to the fontset", 
	0 /*num_expected_replies*/, 
	HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   e_font_fallback_prepend(s);
   SAVE;   
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-fallback-list", 0, "List the fallback fonts in order", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);

   LIST_DATA();
   E_Font_Fallback *ff;
   FOR(e_config->font_fallbacks) { ff = l->data;
      dat = evas_list_append(dat, ff->name);
   }
   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_FONT_FALLBACK_LIST_REPLY);
   evas_list_free(dat);

   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   GENERIC(HDL);
   LIST();
   DECODE(e_ipc_codec_str_list_dec) {
      FOR(dat) {
	 printf("REPLY: \"%s\"\n", (char *)(l->data));
      }
      FREE_LIST(dat);
   }
   END_GENERIC();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_REMOVE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-fallback-remove", 1, "Remove OPT1 from the fontset", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   e_font_fallback_remove(s);
   SAVE;   
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-default-set", 3, "Set textclass (OPT1) font (OPT2) and size (OPT3)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING_INT(params[0], params[1], atoi(params[2]), HDL) 
#elif (TYPE == E_WM_IN)
   STRING2_INT(text_class, font_name, font_size, e_2str_int, HDL);
   e_font_default_set(text_class, font_name, font_size);
   SAVE;   
   END_STRING2_INT(e_2str_int);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-default-get", 1, "List the default font associated with OPT1", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(text_class, HDL);
   E_Font_Default *efd;
   void *data;
   int bytes;
   
   efd = e_font_default_get(text_class);
   data = e_ipc_codec_2str_int_enc(efd->text_class, efd->font, efd->size, &bytes);   
   SEND_DATA(E_IPC_OP_FONT_DEFAULT_GET_REPLY);

   END_STRING(text_class);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING2_INT(text_class, font_name, font_size, e_2str_int, HDL);
   printf("REPLY: DEFAULT TEXT_CLASS=\"%s\" NAME=\"%s\" SIZE=%d\n",
                    text_class, font_name, font_size); 
   END_STRING2_INT(e_2str_int);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_REMOVE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-default-remove", 1, "Remove the default text class OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(text_class, HDL);
   e_font_default_remove(text_class);
   SAVE;
   END_STRING(text_class);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-default-list", 0, "List all configured text classes", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING2_INT_LIST(e_font_default_list(), E_Font_Default, efd, v, HDL);
   v->str1 = efd->text_class;
   v->str2 = efd->font;
   v->val  = efd->size;
   END_SEND_STRING2_INT_LIST(v, E_IPC_OP_FONT_DEFAULT_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_DEFAULT_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING2_INT_LIST(v, HDL);
   printf("REPLY: DEFAULT TEXT_CLASS=\"%s\" NAME=\"%s\" SIZE=%d\n", v->str1, v->str2, v->val);
   END_STRING2_INT_LIST(v);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_FALLBACK_CLEAR
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-fallback-clear", 0, "Clear list of fallback fonts", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   e_font_fallback_clear();
   SAVE;
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
 





/****************************************************************************/
#define HDL E_IPC_OP_RESTART
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-restart", 0, "Restart Enlightenment", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   restart = 1;
   ecore_main_loop_quit();
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
/****************************************************************************/
#define HDL E_IPC_OP_SHUTDOWN
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-shutdown", 0, "Shutdown (exit) Enlightenment", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ecore_main_loop_quit();
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_LANG_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-lang-list", 0, "List all available languages", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   LIST_DATA();
   ENCODE((Evas_List *)e_intl_language_list(), e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_LANG_LIST_REPLY);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_LANG_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   GENERIC(HDL);
   LIST();
   DECODE(e_ipc_codec_str_list_dec) {
      FOR(dat) {
	 printf("REPLY: \"%s\"\n", (char *)(l->data));
      }
      FREE_LIST(dat);
   }
   END_GENERIC();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_LANG_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-lang-set", 1, "Set the current language to 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   IF_FREE(e_config->language);
   e_config->language = strdup(s);
   if ((e_config->language) && (strlen(e_config->language) > 0))
     e_intl_language_set(e_config->language);
   else
     e_intl_language_set(NULL);
   SAVE;
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_LANG_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-lang-get", 0, "Get the current language", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->language, E_IPC_OP_LANG_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_LANG_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
   STRING(s, HDL);
   RESPONSE(r, E_Response_Language_Get, HDL);
   r->lang = strdup(s);
   END_RESPONSE(r, E_RESPONSE_LANGUAGE_GET);
   END_STRING(s);
#endif
#undef HDL




/****************************************************************************/
#define HDL E_IPC_OP_DIRS_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-dirs-list", 1, "List the directory of type specified by 'OPT1', try 'themes'", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   LIST_DATA()
   Evas_List *dir_list = NULL;
   if (!strcmp(s, "data"))
     dir_list = e_path_dir_list_get(path_data);
   else if (!strcmp(s, "images"))
     dir_list = e_path_dir_list_get(path_images);
   else if (!strcmp(s, "fonts"))
     dir_list = e_path_dir_list_get(path_fonts);
   else if (!strcmp(s, "themes"))
     dir_list = e_path_dir_list_get(path_themes);
   else if (!strcmp(s, "init"))
     dir_list = e_path_dir_list_get(path_init);
   else if (!strcmp(s, "icons"))
     dir_list = e_path_dir_list_get(path_icons);
   else if (!strcmp(s, "modules"))
     dir_list = e_path_dir_list_get(path_modules);
   else if (!strcmp(s, "backgrounds"))
     dir_list = e_path_dir_list_get(path_backgrounds);
   E_Path_Dir *p;
   dat = evas_list_append(dat, strdup(s));
   FOR(dir_list) { p = l->data;
     dat = evas_list_append(dat, p->dir);
   }

   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_DIRS_LIST_REPLY);
   evas_list_free(dat);
   e_path_dir_list_free(dir_list);
   END_STRING(s)
#elif (TYPE == E_REMOTE_IN)
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DIRS_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   GENERIC(HDL);
   LIST();
   DECODE(e_ipc_codec_str_list_dec) {
     FOR(dat) {
       if (dat == l)
	 printf("REPLY: Listing for \"%s\"\n", (char *)(l->data));
       else
	 printf("REPLY: \"%s\"\n", (char *)(l->data));
     }
     FREE_LIST(dat);
   }
   END_GENERIC();
#elif (TYPE == E_LIB_IN)
   GENERIC(HDL);
   LIST();
   DECODE(e_ipc_codec_str_list_dec) {
      int count;
      char *type;
      int res;
      RESPONSE(r, E_Response_Dirs_List, HDL);

      /* FIXME - this is a mess, needs to be merged into macros... */
      count = evas_list_count(dat);
      r->dirs = malloc(sizeof(char *) * count);
      r->count = count - 1; /* leave off the "type" */

      count = 0;
      FOR(dat) {
	 if (dat == l)
	   type = l->data;
	 else {
	   r->dirs[count] = l->data;
	   count++;
	 }
      }

      if (!strcmp(type, "data"))
	res = E_RESPONSE_DATA_DIRS_LIST;
      else if (!strcmp(type, "images"))
	res = E_RESPONSE_IMAGE_DIRS_LIST;
      else if (!strcmp(type, "fonts"))
	res = E_RESPONSE_FONT_DIRS_LIST;
      else if (!strcmp(type, "themes"))
	res = E_RESPONSE_THEME_DIRS_LIST;
      else if (!strcmp(type, "init"))
	res = E_RESPONSE_INIT_DIRS_LIST;
      else if (!strcmp(type, "icons"))
	res = E_RESPONSE_ICON_DIRS_LIST;
      else if (!strcmp(type, "modules"))
	res = E_RESPONSE_MODULE_DIRS_LIST;
      else if (!strcmp(type, "backgrounds"))
	res = E_RESPONSE_BACKGROUND_DIRS_LIST;
      END_RESPONSE(r, res); /* FIXME - need a custom free */
   }
   END_GENERIC();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DIRS_APPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-dirs-list-append", 1, "Append the directory of type specified by 'OPT2 to the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   { E_PATH_GET(path, s1)
     e_path_user_path_append(path, s2);
   }
   SAVE;
   END_STRING2(e_2str)
#elif (TYPE == E_REMOTE_IN)
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DIRS_PREPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-dirs-list-prepend", 1, "Prepend the directory of type specified by 'OPT2 to the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   { E_PATH_GET(path, s1)
     e_path_user_path_prepend(path, s2);
   }
   SAVE;
   END_STRING2(e_2str)
#elif (TYPE == E_REMOTE_IN)
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DIRS_REMOVE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-dirs-list-remove", 1, "Remove the directory of type specified by 'OPT2 to the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   { E_PATH_GET(path, s1)
     e_path_user_path_remove(path, s2);
   }
   SAVE;
   END_STRING2(e_2str)
#elif (TYPE == E_REMOTE_IN)
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FRAMERATE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-framerate-set", 1, "Set the animation framerate (fps)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(dbl, HDL);
   e_config->framerate = dbl;
   E_CONFIG_LIMIT(e_config->framerate, 1.0, 200.0);
   edje_frametime_set(1.0 / e_config->framerate);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FRAMERATE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-framerate-get", 0, "Get the animation framerate (fps)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->framerate, E_IPC_OP_FRAMERATE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FRAMERATE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(fps, HDL);
   printf("REPLY: %3.3f\n", fps);
   END_DOUBLE;
#endif
#undef HDL


/****************************************************************************/
#define HDL E_IPC_OP_MENUS_SCROLL_SPEED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-scroll-speed-set", 1, "Set the scroll speed of menus (pixels/sec)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(dbl, HDL);
   e_config->menus_scroll_speed = dbl;
   E_CONFIG_LIMIT(e_config->menus_scroll_speed, 1.0, 20000.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_SCROLL_SPEED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-scroll-speed-get", 0, "Get the scroll speed of menus (pixels/sec)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->menus_scroll_speed, E_IPC_OP_MENUS_SCROLL_SPEED_GET_REPLY, HDL)
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_SCROLL_SPEED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(speed, HDL);
   printf("REPLY: %3.3f\n", speed);
   END_DOUBLE;
#endif
#undef HDL

   
/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_POLICY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-policy-set", 1, "Set the focus policy. OPT1 = CLICK, MOUSE or SLOPPY", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "MOUSE")) value = E_FOCUS_MOUSE;
   else if (!strcmp(params[0], "CLICK")) value = E_FOCUS_CLICK;
   else if (!strcmp(params[0], "SLOPPY")) value = E_FOCUS_SLOPPY;
   else
     {
	 printf("focus must be MOUSE, CLICK or SLOPPY\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_border_button_bindings_ungrab_all();
   e_config->focus_policy = value;
   E_CONFIG_LIMIT(e_config->focus_policy, 0, 2);
   e_border_button_bindings_grab_all();
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_POLICY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-policy-get", 0, "Get focus policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->focus_policy, E_IPC_OP_FOCUS_POLICY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_POLICY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == E_FOCUS_MOUSE)
     printf("REPLY: MOUSE\n");
   else if (policy == E_FOCUS_CLICK)
     printf("REPLY: CLICK\n");
   else if (policy == E_FOCUS_SLOPPY)
     printf("REPLY: SLOPPY\n");
   END_INT
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_EDGE_FLIP_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-set", 1, "Set the edge flip flag (0/1)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->use_edge_flip = value;
   E_CONFIG_LIMIT(e_config->use_edge_flip, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_EDGE_FLIP_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-get", 0, "Get the edge flip flag", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->use_edge_flip, E_IPC_OP_USE_EDGE_FLIP_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_EDGE_FLIP_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL)
   printf("REPLY: %i\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDGE_FLIP_TIMEOUT_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-timeout-set", 1, "Set the edge flip timeout (sec)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL)
#elif (TYPE == E_WM_IN)
   START_DOUBLE(dbl, HDL);
   e_config->edge_flip_timeout = dbl;
   E_CONFIG_LIMIT(e_config->edge_flip_timeout, 0.0, 2.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDGE_FLIP_TIMEOUT_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-timeout-get", 0, "Get the edge flip timeout", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->edge_flip_timeout, E_IPC_OP_EDGE_FLIP_TIMEOUT_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDGE_FLIP_TIMEOUT_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL)
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_CACHE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-cache-set", 1, "Set the font cache size (Kb)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL)
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->font_cache = val;
   E_CONFIG_LIMIT(e_config->font_cache, 0, 32 * 1024);
   e_canvas_recache();
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_CACHE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-cache-get", 0, "Get the speculative font cache size (Kb)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->font_cache, E_IPC_OP_FONT_CACHE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_CACHE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL)
   printf("REPLY: %i\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_IMAGE_CACHE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-image-cache-set", 1, "Set the image cache size (Kb)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL)
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->image_cache = val;
   E_CONFIG_LIMIT(e_config->image_cache, 0, 256 * 1024);
   e_canvas_recache();
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_IMAGE_CACHE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-image-cache-get", 0, "Get the speculative image cache size (Kb)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->image_cache, E_IPC_OP_IMAGE_CACHE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_IMAGE_CACHE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL)
   printf("REPLY: %i\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_FAST_MOVE_THRESHOLD_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-fast-move-threshold-set", 1, "Set the mouse speed (pixels/second) that is considered a 'fast move'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->menus_fast_mouse_move_threshhold = val;
   E_CONFIG_LIMIT(e_config->menus_fast_mouse_move_threshhold, 1.0, 2000.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_FAST_MOVE_THRESHOLD_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-fast-move-threshold-get", 0, "Get the mouse speed (pixels/second) that is considered a 'fast move'", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->menus_fast_mouse_move_threshhold, E_IPC_OP_MENUS_FAST_MOVE_THRESHOLD_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_FAST_MOVE_THRESHOLD_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL)
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-click-drag-timeout-set", 1, "Set the time (in sec) between a mouse press and release that will keep the menu up anyway", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->menus_click_drag_timeout = val;
   E_CONFIG_LIMIT(e_config->menus_click_drag_timeout, 0.0, 10.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menus-click-drag-timeout-get", 0, "Get the time (in sec) between a mouse press and release that will keep the menu up anyway", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->menus_click_drag_timeout, E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL)
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_ANIMATE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-animate-set", 1, "Set the shading animation flag (0/1)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->border_shade_animate = val;
   E_CONFIG_LIMIT(e_config->border_shade_animate, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_ANIMATE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-animate-get", 0, "Get the shading animation flag (0/1)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->border_shade_animate, E_IPC_OP_BORDER_SHADE_ANIMATE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_ANIMATE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL)
   printf("REPLY: %i\n", val);
   END_INT;
#endif
#undef HDL

   /****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_TRANSITION_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-transition-set", 1, "Set the shading animation algorithm (0, 1, 2 or 3)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
      REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->border_shade_transition = val;
   E_CONFIG_LIMIT(e_config->border_shade_transition, 0, 3);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_TRANSITION_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-transition-get", 0, "Get the shading animation algorithm (0, 1, 2 or 3)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->border_shade_transition, E_IPC_OP_BORDER_SHADE_TRANSITION_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_TRANSITION_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL)
   printf("REPLY: %i\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_SPEED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-speed-set", 1, "Set the shading speed (pixels/sec)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
      REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->border_shade_speed = val;
   E_CONFIG_LIMIT(e_config->border_shade_speed, 1.0, 20000.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_SPEED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-border-shade-speed-get", 0, "Get the shading speed (pixels/sec)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->border_shade_speed, E_IPC_OP_BORDER_SHADE_SPEED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BORDER_SHADE_SPEED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL)
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desks-set", 1, "Set the number of virtual desktops (X x Y. OPT1 = X, OPT2 = Y)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2INT(atoi(params[0]), atoi(params[1]), HDL);
#elif (TYPE == E_WM_IN)
   START_2INT(val1, val2, HDL);
   e_config->zone_desks_x_count = val1;
   e_config->zone_desks_y_count = val2;
   E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64)
   E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64)
   {
      Evas_List *l;
      for (l = e_manager_list(); l; l = l->next)
	{
	   E_Manager *man;
	   Evas_List *l2;
	   man = l->data;
	   for (l2 = man->containers; l2; l2 = l2->next)
	     {
		E_Container *con;
		Evas_List *l3;
		con = l2->data;
		for (l3 = con->zones; l3; l3 = l3->next)
		  {
		     E_Zone *zone;
		     zone = l3->data;
		     e_zone_desk_count_set(zone, 
					   e_config->zone_desks_x_count, 
					   e_config->zone_desks_y_count);
		  }
	     }
	}
   }
   SAVE;
   END_2INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desks-get", 0, "Get the number of virtual desktops", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_2INT(e_config->zone_desks_x_count, e_config->zone_desks_y_count, E_IPC_OP_DESKS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_2INT(val1, val2, HDL)
   printf("REPLY: %i %i\n", val1, val2);
   END_2INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_POLICY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-maximize-policy-set", 1, "Set the maximize policy. OPT1 = FULLSCREEN, SMART, EXPAND or FILL", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "FULLSCREEN")) value = E_MAXIMIZE_FULLSCREEN;
   else if (!strcmp(params[0], "SMART")) value = E_MAXIMIZE_SMART;
   else if (!strcmp(params[0], "EXPAND")) value = E_MAXIMIZE_EXPAND;
   else if (!strcmp(params[0], "FILL")) value = E_MAXIMIZE_FILL;
   else
     {
	 printf("maximize must be FULLSCREEN, SMART, EXPAND or FILL\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->maximize_policy = value;
   E_CONFIG_LIMIT(e_config->maximize_policy, E_MAXIMIZE_FULLSCREEN, E_MAXIMIZE_FILL);
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_POLICY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-maximize-policy-get", 0, "Get maximize policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->maximize_policy, E_IPC_OP_MAXIMIZE_POLICY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_POLICY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == E_MAXIMIZE_FULLSCREEN)
     printf("REPLY: FULLSCREEN\n");
   else if (policy == E_MAXIMIZE_SMART)
     printf("REPLY: SMART\n");
   else if (policy == E_MAXIMIZE_EXPAND)
     printf("REPLY: EXPAND\n");
   else if (policy == E_MAXIMIZE_FILL)
     printf("REPLY: FILL\n");
   END_INT
#endif
#undef HDL

/***************/
#define HDL E_IPC_OP_BINDING_MOUSE_LIST 
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-mouse-list", 0, "List all mouse bindings", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action or send reply */
   SEND_INT4_STRING2_LIST(e_config->mouse_bindings, E_Config_Binding_Mouse, emb, v, HDL);
   v->val1 = emb->context;
   v->val2 = emb->modifiers;
   v->str1 = emb->action;
   v->str2 = emb->params;
   v->val3 = emb->button;
   v->val4 = emb->any_mod;
   END_SEND_INT4_STRING2_LIST(v, E_IPC_OP_BINDING_MOUSE_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_BINDING_MOUSE_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   /* e_remote decode the response from e_ipc and print it to the console */
   INT4_STRING2_LIST(v, HDL);
   {
      char *context;
      char modifier[256];
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_ANY) context = "ANY";
      else context = "";
 
      modifier[0] = 0;
      if (v->val2 & E_BINDING_MODIFIER_SHIFT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "SHIFT");
      }
      if (v->val2 & E_BINDING_MODIFIER_CTRL)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "CTRL");
      }
      if (v->val2 & E_BINDING_MODIFIER_ALT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "ALT");
      }
      if (v->val2 & E_BINDING_MODIFIER_WIN)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "WIN");
      }
      if (v->val2 == E_BINDING_MODIFIER_NONE)
         strcpy(modifier, "NONE");

      printf("REPLY: BINDING CONTEXT=%s BUTTON=%i MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
                v->val3,
		modifier,
                v->val4,
                v->str1,
                v->str2
        );
   }
   END_INT4_STRING2_LIST(v);
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_BINDING_MOUSE_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-mouse-add", 6, "Add an existing mouse binding. OPT1 = Context, OPT2 = button, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_4INT_2STRING_START(HDL);
   E_Config_Binding_Mouse eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
        exit(-1);
     }
   eb.button = atoi(params[1]);
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[2];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (strlen(pp) > 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  pp = p + 1;
               }
             else
               {
                  if (!strcmp(pp, "SHIFT")) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strcmp(pp, "CTRL")) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strcmp(pp, "ALT")) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strcmp(pp, "WIN")) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (!strcmp(pp, "NONE")) eb.modifiers = E_BINDING_MODIFIER_NONE;
                  else if (strlen(pp) > 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[3]);
   eb.action = params[4];
   eb.params = params[5];
   REQ_4INT_2STRING_END(eb.context, eb.modifiers, eb.button, eb.any_mod, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT4_STRING2(v, HDL)
   E_Config_Binding_Mouse bind, *eb;

   bind.context = v->val1; 
   bind.modifiers = v->val2;
   bind.button = v->val3;
   bind.any_mod = v->val4;
   bind.action = v->str1; 
   bind.params = v->str2;

   eb = e_config_binding_mouse_match(&bind);
   if (!eb)
     {
        eb = E_NEW(E_Config_Binding_Key, 1);
        e_config->mouse_bindings = evas_list_append(e_config->mouse_bindings, eb);
        eb->context = bind.context;
        eb->button = bind.button;
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        eb->action = strdup(bind.action);
        eb->params = strdup(bind.params);
        e_border_button_bindings_ungrab_all();
        e_bindings_mouse_add(bind.context, bind.button, bind.modifiers,
                        bind.any_mod, bind.action, bind.params);
	e_border_button_bindings_grab_all();
        e_config_save_queue();  
     }
   END_INT4_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
/*****/



/***************/
#define HDL E_IPC_OP_BINDING_MOUSE_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-mouse-del", 6, "Delete an existing mouse binding. OPT1 = Context, OPT2 = button, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_4INT_2STRING_START(HDL);
   E_Config_Binding_Mouse eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
        exit(-1);
     }
   eb.button = atoi(params[1]);
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[2];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (strlen(pp) > 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  pp = p + 1;
               }
             else
               {
                  if (!strcmp(pp, "SHIFT")) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strcmp(pp, "CTRL")) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strcmp(pp, "ALT")) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strcmp(pp, "WIN")) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (!strcmp(pp, "NONE")) eb.modifiers = E_BINDING_MODIFIER_NONE;
                  else if (strlen(pp) > 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[3]);
   eb.action = params[4];
   eb.params = params[5];
 
   REQ_4INT_2STRING_END(eb.context, eb.modifiers, eb.button, eb.any_mod, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT4_STRING2(v, HDL)
   E_Config_Binding_Mouse bind, *eb;

   bind.context = v->val1;
   bind.modifiers = v->val2;
   bind.button = v->val3;
   bind.any_mod = v->val4;
   bind.action = v->str1;
   bind.params = v->str2;
   
   eb = e_config_binding_mouse_match(&bind);
   if (eb)
     {
	e_config->mouse_bindings = evas_list_remove(e_config->mouse_bindings, eb);
        IF_FREE(eb->action);
        IF_FREE(eb->params);
        IF_FREE(eb);
        e_border_button_bindings_ungrab_all();
        e_bindings_mouse_del(bind.context, bind.button, bind.modifiers,
                bind.any_mod, bind.action, bind.params);
        e_border_button_bindings_grab_all();
        e_config_save_queue();
     }
   END_INT4_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_BINDING_KEY_LIST 
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-key-list", 0, "List all key bindings", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action or send reply */
   SEND_INT3_STRING3_LIST(e_config->key_bindings, E_Config_Binding_Key, ekb, v, HDL);
   v->val1 = ekb->context;
   v->val2 = ekb->modifiers;
   v->val3 = ekb->any_mod;
   v->str1 = ekb->key;
   v->str2 = ekb->action;
   v->str3 = ekb->params;
   END_SEND_INT3_STRING3_LIST(v, E_IPC_OP_BINDING_KEY_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_BINDING_KEY_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   /* e_remote decode the response from e_ipc and print it to the console */
   INT3_STRING3_LIST(v, HDL);
   {
      char *context;
      char modifier[256];
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_ANY) context = "ANY";
      else context = "";
 
      modifier[0] = 0;
      if (v->val2 & E_BINDING_MODIFIER_SHIFT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "SHIFT");
      }
      if (v->val2 & E_BINDING_MODIFIER_CTRL)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "CTRL");
      }
      if (v->val2 & E_BINDING_MODIFIER_ALT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "ALT");
      }
      if (v->val2 & E_BINDING_MODIFIER_WIN)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "WIN");
      }
      if (v->val2 == E_BINDING_MODIFIER_NONE)
         strcpy(modifier, "NONE");

      printf("REPLY: BINDING CONTEXT=%s KEY=\"%s\" MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
                v->str1,
		modifier,
                v->val3,
                v->str2,
                v->str3
        );
   }
   END_INT3_STRING3_LIST(v);
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_BINDING_KEY_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-key-add", 6, "Add an existing key binding. OPT1 = Context, OPT2 = key, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_3INT_3STRING_START(HDL);
   E_Config_Binding_Key eb;
   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
	printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
	       "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
	exit(-1);
     }
   eb.key = params[1];
   /* M1[|M2...] */
     {
	char *p, *pp;
	
	eb.modifiers = 0;
	pp = params[2];
	for (;;)
	  {
	     p = strchr(pp, '|');
	     if (p)
	       {
		  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (strlen(pp) > 0)
		    {
		       printf("OPT3 moidifier unknown. Must be or mask of:\n"
			      "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
		       exit(-1);
		    }
		  pp = p + 1;
	       }
	     else
	       {
		  if (!strcmp(pp, "SHIFT")) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strcmp(pp, "CTRL")) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strcmp(pp, "ALT")) eb.modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strcmp(pp, "WIN")) eb.modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (!strcmp(pp, "NONE")) eb.modifiers = E_BINDING_MODIFIER_NONE;
		  else if (strlen(pp) > 0)
		    {
		       printf("OPT3 moidifier unknown. Must be or mask of:\n"
			      "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
		       exit(-1);
		    }
		  break;
	       }
	  }
     }
   eb.any_mod = atoi(params[3]);
   eb.action = params[4];
   eb.params = params[5];
   REQ_3INT_3STRING_END(eb.context, eb.modifiers, eb.any_mod, eb.key, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT3_STRING3(v, HDL)
   E_Config_Binding_Key bind, *eb;

   bind.context = v->val1; 
   bind.modifiers = v->val2;
   bind.any_mod = v->val3;
   bind.key = v->str1; 
   bind.action = v->str2;
   bind.params = v->str3;

   eb = e_config_binding_key_match(&bind);
   if (!eb)
     {
        eb = E_NEW(E_Config_Binding_Key, 1);
        e_config->key_bindings = evas_list_append(e_config->key_bindings, eb);
        eb->context = bind.context;
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        eb->key = strdup(bind.key);
        eb->action = strdup(bind.action);
        eb->params = strdup(bind.params);
        e_managers_keys_ungrab();
        e_bindings_key_add(bind.context, bind.key, bind.modifiers,
                bind.any_mod, bind.action, bind.params);
        e_managers_keys_grab();
        e_config_save_queue();
     }
   END_INT3_STRING3(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
/*****/



/***************/
#define HDL E_IPC_OP_BINDING_KEY_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-key-del", 6, "Delete an existing key binding. OPT1 = Context, OPT2 = key, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_3INT_3STRING_START(HDL);
   E_Config_Binding_Key eb;
   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
	printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
	       "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
	exit(-1);
     }
   eb.key = params[1];
   /* M1[|M2...] */
     {
	char *p, *pp;
	
	eb.modifiers = 0;
	pp = params[2];
	for (;;)
	  {
	     p = strchr(pp, '|');
	     if (p)
	       {
		  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (strlen(pp) > 0)
		    {
		       printf("OPT3 moidifier unknown. Must be or mask of:\n"
			      "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
		       exit(-1);
		    }
		  pp = p + 1;
	       }
	     else
	       {
		  if (!strcmp(pp, "SHIFT")) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strcmp(pp, "CTRL")) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strcmp(pp, "ALT")) eb.modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strcmp(pp, "WIN")) eb.modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (!strcmp(pp, "NONE")) eb.modifiers = E_BINDING_MODIFIER_NONE;
		  else if (strlen(pp) > 0)
		    {
		       printf("OPT3 moidifier unknown. Must be or mask of:\n"
			      "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
		       exit(-1);
		    }
		  break;
	       }
	  }
     }
   eb.any_mod = atoi(params[3]);
   eb.action = params[4];
   eb.params = params[5];
   REQ_3INT_3STRING_END(eb.context, eb.modifiers, eb.any_mod, eb.key, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT3_STRING3(v, HDL)
   E_Config_Binding_Key bind, *eb;

   bind.context = v->val1; 
   bind.modifiers = v->val2;
   bind.any_mod = v->val3;
   bind.key = v->str1; 
   bind.action = v->str2;
   bind.params = v->str3;

   eb = e_config_binding_key_match(&bind);
   if (eb)
     {
       e_config->key_bindings = evas_list_remove(e_config->key_bindings, eb);
       IF_FREE(eb->key);
       IF_FREE(eb->action);
       IF_FREE(eb->params);
       IF_FREE(eb);
       e_managers_keys_ungrab();
       e_bindings_key_del(bind.context, bind.key, bind.modifiers,
			  bind.any_mod, bind.action, bind.params);
       e_managers_keys_grab();
       e_config_save_queue();
    }

   END_INT3_STRING3(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_RAISE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-always-click-to-raise-set", 1, "Set the always click to raise policy, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->always_click_to_raise = policy;
   E_CONFIG_LIMIT(e_config->always_click_to_raise, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_RAISE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-always-click-to-raise-get", 0, "Get the always click to raise policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->always_click_to_raise, E_IPC_OP_ALWAYS_CLICK_TO_RAISE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_RAISE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   printf("REPLY: POLICY=%d\n", policy);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_AUTO_RAISE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-auto-raise-set", 1, "Set use auto raise policy, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->use_auto_raise = policy;
   E_CONFIG_LIMIT(e_config->use_auto_raise, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_AUTO_RAISE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-auto-raise-get", 0, "Get use auto raise policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->use_auto_raise, E_IPC_OP_USE_AUTO_RAISE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_AUTO_RAISE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   printf("REPLY: POLICY=%d\n", policy);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PASS_CLICK_ON_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-pass-click-on-set", 1, "Set pass click on policy, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->pass_click_on = policy;
   E_CONFIG_LIMIT(e_config->pass_click_on, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PASS_CLICK_ON_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-pass-click-on-get", 0, "Get pass click on policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->pass_click_on, E_IPC_OP_PASS_CLICK_ON_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PASS_CLICK_ON_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   printf("REPLY: POLICY=%d\n", policy);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_AUTO_RAISE_DELAY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-auto-raise-delay-set", 1, "Set the auto raise delay (Seconds)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(dbl, HDL);
   e_config->auto_raise_delay = dbl;
   E_CONFIG_LIMIT(e_config->auto_raise_delay, 0.0, 5.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_AUTO_RAISE_DELAY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-auto-raise-delay-get", 0, "Get the auto raise delay  (Seconds)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->auto_raise_delay, E_IPC_OP_AUTO_RAISE_DELAY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_AUTO_RAISE_DELAY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(dbl, HDL);
   printf("REPLY: DELAY=%3.3f\n", dbl);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_USE_RESIST_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-resist-set", 1, "Set resist policy, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->use_resist = policy;
   E_CONFIG_LIMIT(e_config->use_resist, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_RESIST_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-resist-get", 0, "Get use resist policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->use_resist, E_IPC_OP_USE_RESIST_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_RESIST_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   printf("REPLY: POLICY=%d\n", policy);
   END_INT;
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_DRAG_RESIST_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-drag-resist-set", 1, "Set drag resist threshold (0-100)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->drag_resist = val;
   E_CONFIG_LIMIT(e_config->drag_resist, 0, 100);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DRAG_RESIST_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-drag-resist-get", 0, "Get drag resist threshold", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->drag_resist, E_IPC_OP_DRAG_RESIST_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DRAG_RESIST_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: THRESHOLD=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_DESK_RESIST_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desk-resist-set", 1, "Set desktop resist threshold (0-100)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->desk_resist = val;
   E_CONFIG_LIMIT(e_config->desk_resist, 0, 100);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESK_RESIST_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desk-resist-get", 0, "Get desktop resist threshold", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->desk_resist, E_IPC_OP_DESK_RESIST_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESK_RESIST_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: THRESHOLD=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_WINDOW_RESIST_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-window-resist-set", 1, "Set window resist threshold (0-100)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->window_resist = val;
   E_CONFIG_LIMIT(e_config->window_resist, 0, 100);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINDOW_RESIST_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-window-resist-get", 0, "Get window resist threshold", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->window_resist, E_IPC_OP_WINDOW_RESIST_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINDOW_RESIST_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: THRESHOLD=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/

#define HDL E_IPC_OP_GADGET_RESIST_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-gadget-resist-set", 1, "Set gadget resist threshold (0-100)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->gadget_resist = val;
   E_CONFIG_LIMIT(e_config->gadget_resist, 0, 100);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_GADGET_RESIST_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-gadget-resist-get", 0, "Get gadget resist threshold", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->gadget_resist, E_IPC_OP_GADGET_RESIST_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_GADGET_RESIST_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: THRESHOLD=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-bg-add", 4, "Add a desktop bg definition. OPT1 = container no. OPT2 = zone no. OPT3 = desktop name OPT4 = bg file path", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_3INT_3STRING_START(HDL);
   REQ_3INT_3STRING_END(atoi(params[0]), atoi(params[1]), 0, params[2], params[3], "", HDL);
#elif (TYPE == E_WM_IN)
   INT3_STRING3(v, HDL);
   e_bg_add(v->val1, v->val2, v->str1, v->str2);
   e_bg_update();
   SAVE;
   END_INT3_STRING3(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-bg-del", 3, "Delete a desktop bg definition. OPT1 = container no. OPT2 = zone no. OPT3 = desktop name", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_3INT_3STRING_START(HDL);
   REQ_3INT_3STRING_END(atoi(params[0]), atoi(params[1]), 0, params[2], "", "", HDL);
#elif (TYPE == E_WM_IN)
   INT3_STRING3(v, HDL);
   e_bg_del(v->val1, v->val2, v->str1);
   e_bg_update();
   SAVE;
   END_INT3_STRING3(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-bg-list", 0, "List all current desktop bg definitions", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT3_STRING3_LIST(e_config->desktop_backgrounds, E_Config_Desktop_Background, cfbg, v, HDL);
   v->val1 = cfbg->container;
   v->val2 = cfbg->zone;
   v->val3 = 0;
   v->str1 = cfbg->desk;
   v->str2 = cfbg->file;
   v->str3 = "";
   END_SEND_INT3_STRING3_LIST(v, E_IPC_OP_DESKTOP_BG_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   INT3_STRING3_LIST(v, HDL);
   printf("REPLY: BG CONTAINER=%i ZONE=%i DESK=\"%s\" FILE=\"%s\"\n",
	  v->val1, v->val2, v->str1, v->str2);
   END_INT3_STRING3_LIST(v);
#endif
#undef HDL

/****************************************************************************/

#if 0
}
#endif

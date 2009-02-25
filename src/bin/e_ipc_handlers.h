/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* NOTE:
 * 
 * This is a very SPECIAL file. This serves as a kind of "auto code generator"
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
      E_FREE(__str); \
   } \
} \
break;

/**
 * STRING2:
 * decode event data of type E_Ipc_2Str
 */
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
      E_FREE(__2str->str1); \
      E_FREE(__2str->str2); \
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
      E_FREE(__3int_3str->str1); \
      E_FREE(__3int_3str->str2); \
      E_FREE(__3int_3str->str3); \
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
      E_FREE(__4int_2str->str1); \
      E_FREE(__4int_2str->str2); \
      free(__4int_2str); \
   } \
} \
break;

/**
 * INT5_STRING2:
 * Decode event data of type E_Ipc_5Int_2Str
 */
# define INT5_STRING2(__5int_2str, HDL) \
case HDL: \
if (e->data) { \
   E_Ipc_5Int_2Str *__5int_2str = NULL; \
   __5int_2str = calloc(1, sizeof(E_Ipc_5Int_2Str)); \
   if (e_ipc_codec_5int_2str_dec(e->data, e->size, &(__5int_2str))) {
# define END_INT5_STRING2(__5int_2str) \
      E_FREE(__5int_2str->str1); \
      E_FREE(__5int_2str->str2); \
      free(__5int_2str); \
   } \
} \
break;

/**
 * INT3_STRING4:
 * Decode event data of type E_Ipc_4Int_2Str
 */
# define INT3_STRING4(__3int_4str, HDL) \
case HDL: \
if (e->data) { \
   E_Ipc_3Int_4Str *__3int_4str = NULL; \
   __3int_4str = calloc(1, sizeof(E_Ipc_3Int_4Str)); \
   if (e_ipc_codec_3int_4str_dec(e->data, e->size, &(__3int_4str))) {
# define END_INT3_STRING4(__3int_4str) \
      E_FREE(__3int_4str->str1); \
      E_FREE(__3int_4str->str2); \
      E_FREE(__3int_4str->str3); \
      E_FREE(__3int_4str->str4); \
      free(__3int_4str); \
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
      E_FREE(__2str_int->str1); \
      E_FREE(__2str_int->str2); \
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

# define STRING_4INT(__str, __int1, __int2, __int3, __int4, __str_4int, HDL) \
case HDL: \
if (e->data) { \
   char *__str = NULL; \
   int __int1, __int2, __int3, __int4; \
   E_Ipc_Str_4Int *__str_4int = NULL; \
   __str_4int = calloc(1, sizeof(E_Ipc_Str_4Int)); \
   if (e_ipc_codec_str_4int_dec(e->data, e->size, &(__str_4int))) { \
      __str  = __str_4int->str; \
      __int1 = __str_4int->val1; \
      __int2 = __str_4int->val2; \
      __int3 = __str_4int->val3; \
      __int4 = __str_4int->val4; 
# define END_STRING_4INT(__str_4int) \
      E_FREE(__str_4int->str); \
      free(__str_4int); \
   } \
} \
break;


/**
 * Get event data for libe processing
 */
# define RESPONSE(__res, __store) \
   __store *__res = calloc(1, sizeof(__store)); \
   if (e->data) {
#define END_RESPONSE(__res, __type) \
   } \
   ecore_event_add(__type, __res, NULL, NULL);
#define END_RESPONSE_CALLBACK(__res, __type, __callback) \
   } \
   ecore_event_add(__type, __res, __callback, NULL);

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

#define REQ_2INT_START(HDL) \
case HDL: { void *data; int bytes; \

#define REQ_2INT_END(__val1, __val2, HDL) \
   data = e_ipc_codec_2int_enc(__val1, __val2, &bytes); \
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

#define REQ_5INT_2STRING_START(HDL) \
case HDL: { void *data; int bytes; \

#define REQ_5INT_2STRING_END(__val1, __val2, __val3, __val4, __val5, __str1, __str2, HDL) \
   data = e_ipc_codec_5int_2str_enc(__val1, __val2, __val3, __val4, __val5, __str1, __str2, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HDL, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;

#define REQ_3INT_4STRING_START(HDL) \
case HDL: { void *data; int bytes; \

#define REQ_3INT_4STRING_END(__val1, __val2, __val3, __str1, __str2, __str3, __str4, HDL) \
   data = e_ipc_codec_3int_4str_enc(__val1, __val2, __val3, __str1, __str2, __str3, __str4, &bytes); \
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

# define REQ_STRING_4INT(__str, __int1, __int2, __int3, __int4, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_str_4int_enc(__str, __int1, __int2, __int3, __int4, &bytes); \
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
   __list = eina_list_remove_list(__list, __list); \
}

# define SEND_DATA(__opcode) \
ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __opcode, 0, 0, 0, data, bytes); \
free(data);

# define STRING_INT_LIST(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_str_int_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_Str_Int *__v; \
	  __v = l->data;
#define END_STRING_INT_LIST(__v) \
	  E_FREE(__v->str); \
	  free(__v); \
       } \
       eina_list_free(dat); \
    } \
 } \
   break;

#define SEND_STRING_INT_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_Str_Int *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_Str_Int));
#define END_SEND_STRING_INT_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
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
   INT3_STRING3_LIST_START(__v, HDL) \
   INT3_STRING3_LIST_ITERATE(__v)

#define INT3_STRING3_LIST_START(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_3int_3str_list_dec(e->data, e->size, &dat)) {
#define INT3_STRING3_LIST_ITERATE(__v) \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_3Int_3Str *__v; \
	  __v = l->data;
#define END_INT3_STRING3_LIST(__v) \
   END_INT3_STRING3_LIST_ITERATE(__v) \
   END_INT3_STRING3_LIST_START()

#define END_INT3_STRING3_LIST_ITERATE(__v) \
          E_FREE(__v->str1); \
          E_FREE(__v->str2); \
          E_FREE(__v->str3); \
          free(__v); \
       } 
#define END_INT3_STRING3_LIST_START() \
       eina_list_free(dat); \
    } \
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
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_3Int_3Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_3Int_3Str));
#define END_SEND_INT3_STRING3_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
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
   INT4_STRING2_LIST_START(__v, HDL) \
   INT4_STRING2_LIST_ITERATE(__v)

#define INT4_STRING2_LIST_START(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_4int_2str_list_dec(e->data, e->size, &dat)) { 
#define INT4_STRING2_LIST_ITERATE(__v) \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_4Int_2Str *__v; \
	  __v = l->data;
#define END_INT4_STRING2_LIST(__v) \
   END_INT4_STRING2_LIST_ITERATE(__v) \
   END_INT4_STRING2_LIST_START()

#define END_INT4_STRING2_LIST_ITERATE(__v) \
          E_FREE(__v->str1); \
          E_FREE(__v->str2); \
          free(__v); \
       } \
       eina_list_free(dat);
#define END_INT4_STRING2_LIST_START() \
    } \
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
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_4Int_2Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_4Int_2Str));
#define END_SEND_INT4_STRING2_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_4int_2str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * INT5_STRING2:
 * Decode event data is a list of E_Ipc_5Int_2Str objects and iterate
 * the list. For each iteration the object __v will contain a decoded list
 * element.
 *
 * Use END_INT5_STRING2_LIST to terminate the loop and free all data. 
 */
#define INT5_STRING2_LIST(__v, HDL) \
   INT5_STRING2_LIST_START(__v, HDL) \
   INT5_STRING2_LIST_ITERATE(__v)

#define INT5_STRING2_LIST_START(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_5int_2str_list_dec(e->data, e->size, &dat)) { 
#define INT5_STRING2_LIST_ITERATE(__v) \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_5Int_2Str *__v; \
	  __v = l->data;
#define END_INT5_STRING2_LIST(__v) \
   END_INT5_STRING2_LIST_ITERATE(__v) \
   END_INT5_STRING2_LIST_START()

#define END_INT5_STRING2_LIST_ITERATE(__v) \
          E_FREE(__v->str1); \
          E_FREE(__v->str2); \
          free(__v); \
       } \
       eina_list_free(dat);
#define END_INT5_STRING2_LIST_START() \
    } \
 } \
  break;

/** 
 * SEND_INT5_STRING2_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_5Int_2Str. 
 *
 * Use END_SEND_INT5_STRING2_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_INT5_STRING2_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_5Int_2Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_5Int_2Str));
#define END_SEND_INT5_STRING2_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_5int_2str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * INT3_STRING4:
 * Decode event data is a list of E_Ipc_3Int_4Str objects and iterate
 * the list. For each iteration the object __v will contain a decoded list
 * element.
 *
 * Use END_INT3_STRING4_LIST to terminate the loop and free all data. 
 */
#define INT3_STRING4_LIST(__v, HDL) \
   INT3_STRING4_LIST_START(__v, HDL) \
   INT3_STRING4_LIST_ITERATE(__v)

#define INT3_STRING4_LIST_START(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_3int_4str_list_dec(e->data, e->size, &dat)) { 
#define INT3_STRING4_LIST_ITERATE(__v) \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_3Int_4Str *__v; \
	  __v = l->data;
#define END_INT3_STRING4_LIST(__v) \
   END_INT3_STRING4_LIST_ITERATE(__v) \
   END_INT3_STRING4_LIST_START()

#define END_INT3_STRING4_LIST_ITERATE(__v) \
          E_FREE(__v->str1); \
          E_FREE(__v->str2); \
          E_FREE(__v->str3); \
          E_FREE(__v->str4); \
          free(__v); \
       } \
       eina_list_free(dat);
#define END_INT3_STRING4_LIST_START() \
    } \
 } \
  break;

/** 
 * SEND_INT3_STRING4_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_3Int_4Str. 
 *
 * Use END_SEND_INT3_STRING4_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_INT3_STRING4_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_3Int_4Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_3Int_4Str));
#define END_SEND_INT3_STRING4_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_3int_4str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

# define STRING_INT4_LIST(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_str_4int_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_Str_4Int *__v; \
	  __v = l->data;
#define END_STRING_INT4_LIST(__v) \
	  E_FREE(__v->str); \
	  free(__v); \
       } \
       eina_list_free(dat); \
    } \
 } \
   break;

/** 
 * SEND_STRING_INT4_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_Str_4Int. 
 *
 * Use END_SEND_STRING_INT4_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_STRING_INT4_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_Str_4Int *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_Str_4Int));
#define END_SEND_STRING_INT4_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_str_4int_list_enc(dat, &bytes); \
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
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_2str_int_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_2Str_Int *__v; \
	  __v = l->data;
#define END_STRING2_INT_LIST(__v) \
	  E_FREE(__v->str1); \
	  E_FREE(__v->str2); \
	  free(__v); \
       } \
       eina_list_free(dat); \
    } \
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
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_2Str_Int *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_2Str_Int));
#define END_SEND_STRING2_INT_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_2str_int_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;

/**
 * STRING2_LIST:
 * Decode event data which is a list of E_Ipc_2Str objects and iterate 
 * the list. For each iteration the object __v will contain a decoded list
 * element. 
 *
 * Use END_STRING2_LIST to terminate the loop and free all data.
 */
#define STRING2_LIST(__v, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    if (e_ipc_codec_2str_list_dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  E_Ipc_2Str *__v; \
	  __v = l->data;
#define END_STRING2_LIST(__v) \
	  E_FREE(__v->str1); \
	  E_FREE(__v->str2); \
	  free(__v); \
       } \
       eina_list_free(dat); \
    } \
 } \
   break;

/** 
 * SEND_STRING2_LIST:
 * Start to encode a list of objects to prepare them for sending via
 * ipc. The object __v1 will be of type __typ1 and __v2 will be of type
 * E_Ipc_2Str. 
 *
 * Use END_SEND_STRING2_LIST to terminate the encode iteration and 
 * send that data. The list will be freed.
 */
#define SEND_STRING2_LIST(__list, __typ1, __v1, __v2, HDL) \
 case HDL: { \
    Eina_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = __list; l; l = l->next) { \
       __typ1 *__v1; \
       E_Ipc_2Str *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(E_Ipc_2Str));
#define END_SEND_STRING2_LIST(__v1, __op) \
       dat = eina_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_2str_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    FREE_LIST(dat); \
 } \
   break;


#define SEND_STRING(__str, __op, HDL) \
case HDL: { void *data; int bytes = 0; \
   data = e_ipc_codec_str_enc(__str, &bytes); \
   ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __op, 0, 0, 0, data, bytes); \
   E_FREE(data); \
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

/* 
   Declare variables used when encoding that needs to first be 
   constructed into a list using the FOR macro. The list to create
   will be called 'dat' and the list to iterate will be l;
 */
#define LIST_ENCODE_START() \
   Eina_List *dat = NULL, *l; \
   void *data; int bytes;

/* 
   Declare variables used by the encode macro. Is separate to allow
   operations to be done between declairation and encoding. 
 */
#define ENCODE_START() \
   void *data; int bytes;

#define ENCODE(__dat, __enc) \
   data = __enc(__dat, &bytes);

/*
   Iterate an eina_list starting with the pointer to __start. l 
   is the pointer to the current position in the list. 
 */
#define FOR(__start) \
   for (l = __start; l; l = l->next)
#define GENERIC(HDL) \
 case HDL: {

#define END_GENERIC() \
   } \
break;

#define LIST() \
   Eina_List *dat = NULL, *l;

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
   else if (!strcmp(__str, "icons")) \
     __path = path_icons; \
   else if (!strcmp(__str, "modules")) \
     __path = path_modules; \
   else if (!strcmp(__str, "backgrounds")) \
     __path = path_backgrounds; \
   else if (!strcmp(__str, "messages")) \
     __path = path_messages; 

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
   v->str = (char *) mod->name;
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
   if (v->str) printf("REPLY: \"%s\" ENABLED %i\n", v->str, v->val);
   else printf("REPLY: \"\" ENABLED %i\n", v->val);
   END_STRING_INT_LIST(v);
#elif (TYPE == E_LIB_IN)
   GENERIC(HDL);
   Eina_List *dat = NULL;
   DECODE(e_ipc_codec_str_int_list_dec) {
      LIST();
      int count;
      RESPONSE(r, E_Response_Module_List);

      /* FIXME - this is a mess, needs to be merged into macros... */
      count = eina_list_count(dat);
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
      END_RESPONSE_CALLBACK(r, E_RESPONSE_MODULE_LIST, _e_cb_module_list_free);
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
   /* TODO: Check if file exists */
   if (e_config->desktop_default_background) eina_stringshare_del(e_config->desktop_default_background);
   e_config->desktop_default_background = NULL;
   if (s) e_config->desktop_default_background = eina_stringshare_add(s);
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
   if (s) printf("REPLY: \"%s\"\n", s);
   else printf("REPLY: \"\"\n");
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
   STRING(s, HDL);
   RESPONSE(r, E_Response_Background_Get);
   if (s) r->file = strdup(s);
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
   LIST_ENCODE_START();
   E_Font_Available *fa;
   Eina_List *fa_list;
   fa_list = e_font_available_list();
   FOR(fa_list) { fa = l->data;
      dat = eina_list_append(dat, fa->name);
   }
   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_FONT_AVAILABLE_LIST_REPLY);
   eina_list_free(dat);
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
	 if (l->data) printf("REPLY: \"%s\"\n", (char *)(l->data));
	 else printf("REPLY: \"\"\n");
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

   LIST_ENCODE_START();
   E_Font_Fallback *ff;
   FOR(e_config->font_fallbacks) { ff = l->data;
      dat = eina_list_append(dat, ff->name);
   }
   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_FONT_FALLBACK_LIST_REPLY);
   eina_list_free(dat);

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
	 if (l->data) printf("REPLY: \"%s\"\n", (char *)(l->data));
	 else printf("REPLY: \"\"\n");
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
   /* TODO: Check if font_name (and text_class?) exists */
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
   if (efd == NULL)
     data = NULL;
   else
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
   v->str1 = (char *) efd->text_class;
   v->str2 = (char *) efd->font;
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
   REQ_NULL(HDL);
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
   OP("-exit", 0, "Exit Enlightenment", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   if (!e_util_immortal_check()) ecore_main_loop_quit();
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
   ENCODE_START();
   Eina_List *languages;
   languages = e_intl_language_list();
   ENCODE(languages, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_LANG_LIST_REPLY);
   FREE_LIST(languages); 
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
	 if (l->data) printf("REPLY: \"%s\"\n", (char *)(l->data));
         else printf("REPLY: \"\"\n");
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
   /* TODO: Check if language exists */
   if (e_config->language) eina_stringshare_del(e_config->language);
   e_config->language = NULL;
   if (s) e_config->language = eina_stringshare_add(s);
   if ((e_config->language) && (e_config->language[0] != 0))
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
   if (s) printf("REPLY: \"%s\"\n", s);
   else printf("REPLY: \"\"\n");
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
   STRING(s, HDL);
   RESPONSE(r, E_Response_Language_Get);
   if (s) r->lang = strdup(s);
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
   LIST_ENCODE_START()
   Eina_List *dir_list = NULL;
   E_PATH_GET(path, s);
   if (path)
      dir_list = e_path_dir_list_get(path);
     
   E_Path_Dir *p;
   if (s) {
      dat = eina_list_append(dat, eina_stringshare_add(s));
      FOR(dir_list) { p = l->data;
	 dat = eina_list_append(dat, eina_stringshare_add(p->dir));
      }
   }

   ENCODE(dat, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_DIRS_LIST_REPLY);
   while (dat)
     {
	const char *dir;
	
	dir = dat->data;
	eina_stringshare_del(dir);
	dat = eina_list_remove_list(dat, dat);	
     }
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
	 printf("REPLY Listing for \"%s\"\n", (char *)(l->data));
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
      char * type;
      int res;

      res = 0;
      RESPONSE(r, E_Response_Dirs_List);

      /* FIXME - this is a mess, needs to be merged into macros... */
      count = eina_list_count(dat);
      r->dirs = malloc(sizeof(char *) * count);
      r->count = count - 1; /* leave off the "type" */

      type = NULL;
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
      END_RESPONSE_CALLBACK(r, res, _e_cb_dir_list_free);
   }
   END_GENERIC();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DIRS_APPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-dirs-list-append", 2, "Append the directory of type specified by 'OPT2' to the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   {
      E_PATH_GET(path, s1)
      if (path) e_path_user_path_append(path, s2);
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
   OP("-dirs-list-prepend", 2, "Prepend the directory of type specified by 'OPT2' to the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   {
      E_PATH_GET(path, s1)
      if (path) e_path_user_path_prepend(path, s2);
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
   OP("-dirs-list-remove", 2, "Remove the directory of type specified by 'OPT2' from the list in 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
   STRING2(s1, s2, e_2str, HDL);
   {
      E_PATH_GET(path, s1)
      if (path) e_path_user_path_remove(path, s2);
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
#define HDL E_IPC_OP_EDGE_FLIP_DRAGGING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-dragging-set", 1, "Set the edge flip while dragging policy flag (0/1)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->edge_flip_dragging = value;
   E_CONFIG_LIMIT(e_config->edge_flip_dragging, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDGE_FLIP_DRAGGING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edge-flip-dragging-get", 0, "Get the edge flip while dragging policy flag", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->edge_flip_dragging, E_IPC_OP_EDGE_FLIP_DRAGGING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDGE_FLIP_DRAGGING_GET_REPLY
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
#define HDL E_IPC_OP_EDJE_CACHE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edje-cache-set", 1, "Set the edje cache size (items)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL)
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->edje_cache = val;
   E_CONFIG_LIMIT(e_config->edje_cache, 0, 256);
   e_canvas_recache();
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDJE_CACHE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edje-cache-get", 0, "Get the speculative edje cache size (items)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->edje_cache, E_IPC_OP_EDJE_CACHE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDJE_CACHE_GET_REPLY
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
#define HDL E_IPC_OP_EDJE_COLLECTION_CACHE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edje-collection-cache-set", 1, "Set the edje collection cache size (items)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL)
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->edje_collection_cache = val;
   E_CONFIG_LIMIT(e_config->edje_collection_cache, 0, 512);
   e_canvas_recache();
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDJE_COLLECTION_CACHE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-edje-collection-cache-get", 0, "Get the speculative edje collection cache size (items)", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->edje_collection_cache, E_IPC_OP_EDJE_COLLECTION_CACHE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EDJE_COLLECTION_CACHE_GET_REPLY
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
   OP("-desks-set", 2, "Set the number of virtual desktops (X x Y desks OPT1 = X, OPT2 = Y)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2INT(atoi(params[0]), atoi(params[1]), HDL);
#elif (TYPE == E_WM_IN)
   START_2INT(val1, val2, HDL);
   e_config->zone_desks_x_count = val1;
   e_config->zone_desks_y_count = val2;
   E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64)
   E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64)
   {
      Eina_List *l;
      for (l = e_manager_list(); l; l = l->next)
	{
	   E_Manager *man;
	   Eina_List *l2;
	   man = l->data;
	   for (l2 = man->containers; l2; l2 = l2->next)
	     {
		E_Container *con;
		Eina_List *l3;
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

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_MANIPULATION_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-maximize-manipulation-set", 1, "Allow manipulation, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->allow_manip = policy;
   E_CONFIG_LIMIT(e_config->allow_manip, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_MANIPULATION_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-maximize-manipulation-get", 0, "Get manipulation, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->allow_manip, E_IPC_OP_MAXIMIZE_MANIPULATION_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MAXIMIZE_MANIPULATION_GET_REPLY
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
   v->str1 = (char *) emb->action;
   v->str2 = (char *) emb->params;
   v->val3 = emb->button;
   v->val4 = emb->any_mod;
   END_SEND_INT4_STRING2_LIST(v, E_IPC_OP_BINDING_MOUSE_LIST_REPLY);
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
      char *s1, *s2;
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_CONTAINER) context = "CONTAINER";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_MENU) context = "MENU";
      else if (v->val1 == E_BINDING_CONTEXT_WINLIST) context = "WINLIST";
      else if (v->val1 == E_BINDING_CONTEXT_POPUP) context = "POPUP";
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

      s1 = v->str1;
      s2 = v->str2;
      if (!s1) s1 = "";
      if (!s2) s2 = "";
      printf("REPLY: BINDING CONTEXT=%s BUTTON=%i MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
                v->val3,
		modifier,
                v->val4,
                s1,
                s2
        );
   }
   END_INT4_STRING2_LIST(v);
#elif E_LIB_IN
   INT4_STRING2_LIST_START(v, HDL);
   {
      int count;
      RESPONSE(r, E_Response_Binding_Mouse_List);
      count = eina_list_count(dat);
      r->bindings = malloc(sizeof(E_Response_Binding_Mouse_Data *) * count);
      r->count = count;

      count = 0;
      INT4_STRING2_LIST_ITERATE(v);
      {
	 E_Response_Binding_Mouse_Data *d;

	 d = malloc(sizeof(E_Response_Binding_Mouse_Data));
	 d->ctx = v->val1;
	 d->button = v->val3;
	 d->mod = v->val2;
	 d->any_mod = v->val4;
	 d->action = ((v->str1) ? eina_stringshare_add(v->str1) : NULL);
	 d->params = ((v->str2) ? eina_stringshare_add(v->str2) : NULL);

	 r->bindings[count] = d;
	 count++;
      }
      END_INT4_STRING2_LIST_ITERATE(v);
      /* this will leak, need to free the event data */
      END_RESPONSE(r, E_RESPONSE_BINDING_MOUSE_LIST);
   }
   END_INT4_STRING2_LIST_START();
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
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
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
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
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
        eb = E_NEW(E_Config_Binding_Mouse, 1);
        e_config->mouse_bindings = eina_list_append(e_config->mouse_bindings, eb);
        eb->context = bind.context;
        eb->button = bind.button;
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        if (bind.action) eb->action = eina_stringshare_add(bind.action);
        if (bind.params) eb->params = eina_stringshare_add(bind.params);
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

/****************************************************************************/
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
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
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
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
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
	e_config->mouse_bindings = eina_list_remove(e_config->mouse_bindings, eb);
        if (eb->action) eina_stringshare_del(eb->action);
        if (eb->params) eina_stringshare_del(eb->params);
        E_FREE(eb);
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
   v->str1 = (char *) ekb->key;
   v->str2 = (char *) ekb->action;
   v->str3 = (char *) ekb->params;
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
      char *s1, *s2, *s3;
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_CONTAINER) context = "CONTAINER";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_MENU) context = "MENU";
      else if (v->val1 == E_BINDING_CONTEXT_WINLIST) context = "WINLIST";
      else if (v->val1 == E_BINDING_CONTEXT_POPUP) context = "POPUP";
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

      s1 = v->str1;
      s2 = v->str2;
      s3 = v->str3;
      if (!s1) s1 = "";
      if (!s2) s2 = "";
      if (!s3) s3 = "";
      printf("REPLY: BINDING CONTEXT=%s KEY=\"%s\" MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
                s1,
		modifier,
                v->val3,
                s2,
                s3
        );
   }
   END_INT3_STRING3_LIST(v);
#elif E_LIB_IN
   INT3_STRING3_LIST_START(v, HDL);
   {
      int count = 0;
      RESPONSE(r, E_Response_Binding_Key_List);
      count = eina_list_count(dat);
      r->bindings = malloc(sizeof(E_Response_Binding_Key_Data *) * count);
      r->count = count;

      count = 0;
      INT3_STRING3_LIST_ITERATE(v);
      {
	 E_Response_Binding_Key_Data *d;

	 d = malloc(sizeof(E_Response_Binding_Key_Data));
	 d->ctx = v->val1;
	 d->key = ((v->str1) ? eina_stringshare_add(v->str1) : NULL);
	 d->mod = v->val2;
	 d->any_mod = v->val3;
	 d->action = ((v->str2) ? eina_stringshare_add(v->str2) : NULL);
	 d->params = ((v->str3) ? eina_stringshare_add(v->str3) : NULL);

	 r->bindings[count] = d;
	 count++;
      }
      END_INT3_STRING3_LIST_ITERATE(v);
      /* this will leak, need to free the event data */
      END_RESPONSE(r, E_RESPONSE_BINDING_KEY_LIST);
   }
   END_INT3_STRING3_LIST_START();
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
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
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
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
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
        e_config->key_bindings = eina_list_append(e_config->key_bindings, eb);
        eb->context = bind.context;
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        if (bind.key) eb->key = eina_stringshare_add(bind.key);
        if (bind.action) eb->action = eina_stringshare_add(bind.action);
        if (bind.params) eb->params = eina_stringshare_add(bind.params);
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

/****************************************************************************/
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
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST ANY POPUP\n");
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
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
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
       e_config->key_bindings = eina_list_remove(e_config->key_bindings, eb);
       if (eb->key) eina_stringshare_del(eb->key);
       if (eb->action) eina_stringshare_del(eb->action);
       if (eb->params) eina_stringshare_del(eb->params);
       E_FREE(eb);
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
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_FOCUS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-always-click-to-focus-set", 1, "Set the always click to focus policy, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->always_click_to_focus = policy;
   E_CONFIG_LIMIT(e_config->always_click_to_focus, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_FOCUS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-always-click-to-focus-get", 0, "Get the always click to focus policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->always_click_to_focus, E_IPC_OP_ALWAYS_CLICK_TO_FOCUS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ALWAYS_CLICK_TO_FOCUS_GET_REPLY
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
   OP("-desktop-bg-add", 5, "Add a desktop bg definition. OPT1 = container no. OPT2 = zone no. OPT3 = desk_x. OPT4 = desk_y. OPT5 = bg file path", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_4INT_2STRING_START(HDL);
   REQ_4INT_2STRING_END(atoi(params[0]), atoi(params[1]), atoi(params[2]), atoi(params[3]), params[4], "", HDL);
#elif (TYPE == E_WM_IN)
   INT4_STRING2(v, HDL);
   e_bg_add(v->val1, v->val2, v->val3, v->val4, v->str1);
   e_bg_update();
   SAVE;
   END_INT4_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-bg-del", 4, "Delete a desktop bg definition. OPT1 = container no. OPT2 = zone no. OPT3 = desk_x. OPT4 = desk_y.", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_4INT_2STRING_START(HDL);
   REQ_4INT_2STRING_END(atoi(params[0]), atoi(params[1]), atoi(params[2]), atoi(params[3]), "", "", HDL);
#elif (TYPE == E_WM_IN)
   INT4_STRING2(v, HDL);
   e_bg_del(v->val1, v->val2, v->val3, v->val4);
   e_bg_update();
   SAVE;
   END_INT4_STRING2(v);
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
   SEND_INT4_STRING2_LIST(e_config->desktop_backgrounds, E_Config_Desktop_Background, cfbg, v, HDL);
   v->val1 = cfbg->container;
   v->val2 = cfbg->zone;
   v->val3 = cfbg->desk_x;
   v->val4 = cfbg->desk_y;
   v->str1 = (char *) cfbg->file;
   v->str2 = "";
   END_SEND_INT4_STRING2_LIST(v, E_IPC_OP_DESKTOP_BG_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_BG_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   INT4_STRING2_LIST(v, HDL);
   printf("REPLY: BG CONTAINER=%i ZONE=%i DESK_X=%i DESK_Y=%i FILE=\"%s\"\n",
	  v->val1, v->val2, v->val3, v->val4, v->str1);
   END_INT4_STRING2_LIST(v);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_WHILE_SELECTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-while-selecting-set", 1, "Set winlist (alt+tab) warp while selecting policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_warp_while_selecting = policy;
   E_CONFIG_LIMIT(e_config->winlist_warp_while_selecting, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_WHILE_SELECTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-while-selecting-get", 0, "Get winlist (alt+tab) warp while selecting policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_warp_while_selecting, E_IPC_OP_WINLIST_WARP_WHILE_SELECTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_WHILE_SELECTING_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_AT_END_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-at-end-set", 1, "Set winlist (alt+tab) warp at end policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_warp_at_end = policy;
   E_CONFIG_LIMIT(e_config->winlist_warp_at_end, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_AT_END_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-at-end-get", 0, "Get winlist (alt+tab) warp at end policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_warp_at_end, E_IPC_OP_WINLIST_WARP_AT_END_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_AT_END_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_SPEED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-speed-set", 1, "Set winlist warp speed (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_warp_speed = val;
   E_CONFIG_LIMIT(e_config->winlist_warp_speed, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_SPEED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-warp-speed-get", 0, "Get winlist warp speed", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_warp_speed, E_IPC_OP_WINLIST_WARP_SPEED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_WARP_SPEED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: SPEED=%3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_ANIMATE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-scroll-animate-set", 1, "Set winlist (alt+tab) scroll animate policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_scroll_animate = policy;
   E_CONFIG_LIMIT(e_config->winlist_scroll_animate, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_ANIMATE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-scroll-animate-get", 0, "Get winlist (alt+tab) scroll animate policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_scroll_animate, E_IPC_OP_WINLIST_SCROLL_ANIMATE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_ANIMATE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_SPEED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-scroll-speed-set", 1, "Set winlist scroll speed (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_scroll_speed = val;
   E_CONFIG_LIMIT(e_config->winlist_scroll_speed, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_SPEED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-scroll-speed-get", 0, "Get winlist scroll speed", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_scroll_speed, E_IPC_OP_WINLIST_SCROLL_SPEED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_SCROLL_SPEED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: SPEED=%3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_ICONIFIED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-iconified-set", 1, "Set whether winlist (alt+tab) will show iconfied windows", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_show_iconified = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_show_iconified, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_ICONIFIED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-iconified-get", 0, "Get whether winlist (alt+tab) will show iconfied windows", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_show_iconified, E_IPC_OP_WINLIST_LIST_SHOW_ICONIFIED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_ICONIFIED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_ICONIFIED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-desk-iconified-set", 1, "Set whether winlist (alt+tab) will show iconfied windows from other desks", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_show_other_desk_iconified = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_iconified, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_ICONIFIED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-desk-iconified-get", 0, "Get whether winlist (alt+tab) will show iconfied windows from other desks", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_show_other_desk_iconified, E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_ICONIFIED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_ICONIFIED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_ICONIFIED_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-screen-iconified-set", 1, "Set whether winlist (alt+tab) will show iconfied windows from other screens", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_show_other_screen_iconified = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_iconified, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_ICONIFIED_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-screen-iconified-get", 0, "Get whether winlist (alt+tab) will show iconfied windows from other screens", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_show_other_screen_iconified, E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_ICONIFIED_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_ICONIFIED_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_WINDOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-desk-windows-set", 1, "Set whether winlist (alt+tab) will show other desk windows", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_show_other_desk_windows = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_windows, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_WINDOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-desk-windows-get", 0, "Get winlist (alt+tab) show other desk windows", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_show_other_desk_windows, E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_WINDOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_DESK_WINDOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_WINDOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-screen-windows-set", 1, "Set winlist (alt+tab) show other screen windows policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_show_other_screen_windows = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_windows, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_WINDOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-show-other-screen-windows-get", 0, "Get winlist (alt+tab) show other screen windows policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_show_other_screen_windows, E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_WINDOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_SHOW_OTHER_SCREEN_WINDOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_UNCOVER_WHILE_SELECTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-uncover-while-selecting-set", 1, "Set whether winlist (alt+tab) will show iconified windows while selecting", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_uncover_while_selecting = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_uncover_while_selecting, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_UNCOVER_WHILE_SELECTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-uncover-while-selecting-get", 0, "Get whether winlist (alt+tab) will show iconified windows while selecting", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_uncover_while_selecting, E_IPC_OP_WINLIST_LIST_UNCOVER_WHILE_SELECTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_UNCOVER_WHILE_SELECTING_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_JUMP_DESK_WHILE_SELECTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-jump-desk-while-selecting-set", 1, "Set winlist (alt+tab) jump desk while selecting policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_jump_desk_while_selecting = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_jump_desk_while_selecting, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_JUMP_DESK_WHILE_SELECTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-jump-desk-while-selecting-get", 0, "Get winlist (alt+tab) jump desk while selecting policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_jump_desk_while_selecting, E_IPC_OP_WINLIST_LIST_JUMP_DESK_WHILE_SELECTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_JUMP_DESK_WHILE_SELECTING_GET_REPLY
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
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_X_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-align-x-set", 1, "Set winlist position align for x axis (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_pos_align_x = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_align_x, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_X_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-align-x-get", 0, "Get winlist position align for x axis", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_pos_align_x, E_IPC_OP_WINLIST_POS_ALIGN_X_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_X_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_Y_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-align-y-set", 1, "Set winlist position align for y axis (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_pos_align_y = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_align_y, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_Y_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-align-y-get", 0, "Get winlist position align for y axis", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_pos_align_y, E_IPC_OP_WINLIST_POS_ALIGN_Y_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_ALIGN_Y_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_W_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-size-w-set", 1, "Set winlist position size width (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_pos_size_w = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_size_w, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_W_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-size-w-get", 0, "Get winlist position size width", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_pos_size_w, E_IPC_OP_WINLIST_POS_SIZE_W_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_W_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_H_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-size-h-set", 1, "Set winlist position size height (0.0-1.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->winlist_pos_size_h = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_size_h, 0.0, 1.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_H_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-size-h-get", 0, "Get winlist position size height", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->winlist_pos_size_h, E_IPC_OP_WINLIST_POS_SIZE_H_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_SIZE_H_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_W_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-min-w-set", 1, "Set winlist (alt+tab) minimum width", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->winlist_pos_min_w = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_min_w, 0, 4000);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_W_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-min-w-get", 0, "Get winlist (alt+tab) minimum width", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_pos_min_w, E_IPC_OP_WINLIST_POS_MIN_W_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_W_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_H_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-min-h-set", 1, "Set winlist (alt+tab) minimum height", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->winlist_pos_min_h = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_min_h, 0, 4000);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_H_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-min-h-get", 0, "Get winlist (alt+tab) minimum height", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_pos_min_h, E_IPC_OP_WINLIST_POS_MIN_H_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MIN_H_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_W_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-max-w-set", 1, "Set winlist (alt+tab) maximum width", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->winlist_pos_max_w = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_max_w, 8, 4000);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_W_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-max-w-get", 0, "Get winlist (alt+tab) maximum width", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_pos_max_w, E_IPC_OP_WINLIST_POS_MAX_W_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_W_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_H_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-max-h-set", 1, "Set winlist (alt+tab) maximum height", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->winlist_pos_max_h = val;
   E_CONFIG_LIMIT(e_config->winlist_pos_max_h, 8, 4000);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_H_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-pos-max-h-get", 0, "Get winlist (alt+tab) maximum height", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_pos_max_h, E_IPC_OP_WINLIST_POS_MAX_H_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_POS_MAX_H_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_IF_CLOSE_NOT_POSSIBLE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-if-close-not-possible-set", 1, "Set whether E should kill an application if it can not close", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->kill_if_close_not_possible = val;
   E_CONFIG_LIMIT(e_config->kill_if_close_not_possible, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_IF_CLOSE_NOT_POSSIBLE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-if-close-not-possible-get", 0, "Get whether E should kill an application if it can not close", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->kill_if_close_not_possible, E_IPC_OP_KILL_IF_CLOSE_NOT_POSSIBLE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_IF_CLOSE_NOT_POSSIBLE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: KILL=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_PROCESS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-process-set", 1, "Set whether E should kill the process directly or through x", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->kill_process = val;
   E_CONFIG_LIMIT(e_config->kill_process, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_PROCESS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-process-get", 0, "Get whether E should kill the process directly or through x", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->kill_process, E_IPC_OP_KILL_PROCESS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_PROCESS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: KILL=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_TIMER_WAIT_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-timer-wait-set", 1, "Set interval to wait before killing client (0.0-120.0)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_DOUBLE(atof(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_DOUBLE(val, HDL);
   e_config->kill_timer_wait = val;
   E_CONFIG_LIMIT(e_config->kill_timer_wait, 0.0, 120.0);
   SAVE;
   END_DOUBLE;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_TIMER_WAIT_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-kill-timer-wait-get", 0, "Get interval to wait before killing client", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_DOUBLE(e_config->kill_timer_wait, E_IPC_OP_KILL_TIMER_WAIT_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_KILL_TIMER_WAIT_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_DOUBLE(val, HDL);
   printf("REPLY: %3.3f\n", val);
   END_DOUBLE;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PING_CLIENTS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-ping-clients-set", 1, "Set whether E should ping clients", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->ping_clients = val;
   E_CONFIG_LIMIT(e_config->ping_clients, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PING_CLIENTS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-ping-clients-get", 0, "Get whether E should ping clients", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->ping_clients, E_IPC_OP_PING_CLIENTS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PING_CLIENTS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_START_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-start-set", 1, "Get the background transition used when E starts", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (!s)
     {
	if (e_config->transition_start) eina_stringshare_del(e_config->transition_start);
	e_config->transition_start = NULL;
     }
   if ((s) && (e_theme_transition_find(s)))
     {
	if (e_config->transition_start) eina_stringshare_del(e_config->transition_start);
	e_config->transition_start = NULL;
	if (s) e_config->transition_start = eina_stringshare_add(s);
	SAVE;
     }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_START_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-start-get", 0, "Get the background transition used when E starts", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->transition_start, E_IPC_OP_TRANSITION_START_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_START_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL
 
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_DESK_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-desk-set", 1, "Set the transition used when switching desktops", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (!s)
     {
	if (e_config->transition_desk) eina_stringshare_del(e_config->transition_desk);
	e_config->transition_desk = NULL;
     }
   if ((s) && (e_theme_transition_find(s)))
     {
	if (e_config->transition_desk) eina_stringshare_del(e_config->transition_desk);
	e_config->transition_desk = NULL;
	if (s) e_config->transition_desk = eina_stringshare_add(s);
     }
   SAVE;
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_DESK_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-desk-get", 0, "Get the transition used when switching desktops", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->transition_desk, E_IPC_OP_TRANSITION_DESK_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_DESK_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL
 
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_CHANGE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-change-set", 1, "Set the transition used when changing backgrounds", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (!s)
     {
	if (e_config->transition_change) eina_stringshare_del(e_config->transition_change);
	e_config->transition_change = NULL;
     }
   if ((s) && (e_theme_transition_find(s)))
     {
	if (e_config->transition_change) eina_stringshare_del(e_config->transition_change);
	e_config->transition_change = NULL;
	if (s) e_config->transition_change = eina_stringshare_add(s);
       	SAVE;
     }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_CHANGE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-change-get", 0, "Get the transition used when changing backgrounds", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->transition_change, E_IPC_OP_TRANSITION_CHANGE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_CHANGE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_SETTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-setting-set", 1, "Set the focus setting policy (\"NONE\", \"NEW_WINDOW\", \"NEW_DIALOG\", \"NEW_DIALOG_IF_OWNER_FOCUSED\")", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "NONE")) value = E_FOCUS_NONE;
   else if (!strcmp(params[0], "NEW_WINDOW")) value = E_FOCUS_NEW_WINDOW;
   else if (!strcmp(params[0], "NEW_DIALOG")) value = E_FOCUS_NEW_DIALOG;
   else if (!strcmp(params[0], "NEW_DIALOG_IF_OWNER_FOCUSED")) value = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
   else
     {
	 printf("focus setting must be \"NONE\", \"NEW_WINDOW\", \"NEW_DIALOG\", \"NEW_DIALOG_IF_OWNER_FOCUSED\"\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_border_button_bindings_ungrab_all();
   e_config->focus_setting = value;
   E_CONFIG_LIMIT(e_config->focus_setting, 0, 3);
   e_border_button_bindings_grab_all();
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_SETTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-setting-get", 0, "Get the focus setting policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->focus_setting, E_IPC_OP_FOCUS_SETTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_SETTING_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(setting, HDL);
   if (setting == E_FOCUS_NONE)
     printf("REPLY: NONE\n");
   else if (setting == E_FOCUS_NEW_WINDOW)
     printf("REPLY: NEW_WINDOW\n");
   else if (setting == E_FOCUS_NEW_DIALOG)
     printf("REPLY: NEW_DIALOG\n");
   else if (setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)
     printf("REPLY: NEW_DIALOG_IF_OWNER_FOCUSED\n");
   END_INT
#endif
#undef HDL
 
/****************************************************************************/
#define HDL E_IPC_OP_EXEC_ACTION
#if (TYPE == E_REMOTE_OPTIONS)
	OP("-exec-action", 2, "Executes an action given the name (OPT1) and a string of parameters (OPT2).", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
	REQ_2STRING(params[0], params[1], HDL);
#elif (TYPE == E_WM_IN)
	STRING2(actionName, paramList, e_2str, HDL);
	{
		Eina_List *m = e_manager_list();
		int ok = 0;
		if (m) {
			E_Manager *man = m->data;

			if (man) {
				E_Action *act = e_action_find(actionName);

				if (act && act->func.go) {
					act->func.go(E_OBJECT(man), paramList);
					ok = 1;
				}
			}
		}

		void *d;
		int len;
		d = e_ipc_codec_int_enc(ok, &len);
		if (d) {
		   ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, E_IPC_OP_EXEC_ACTION_REPLY, 0, 0, 0, d, len);
		   free(d);
		}
	}
	END_STRING2(e_2str)
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_EXEC_ACTION_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_THEME_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-theme-list", 0, "List themes and associated categories", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING2_LIST(e_config->themes, E_Config_Theme, theme, v, HDL);
   v->str1 = (char *) theme->category;
   v->str2 = (char *) theme->file;
   END_SEND_STRING2_LIST(v, E_IPC_OP_THEME_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
    
/****************************************************************************/
#define HDL E_IPC_OP_THEME_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING2_LIST(v, HDL);
   printf("REPLY: CATEGORY=\"%s\" FILE=\"%s\"\n", v->str1, v->str2);
   END_STRING2_LIST(v);
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_THEME_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-theme-set", 2, "Set theme category (OPT1) and edje file (OPT2)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2STRING(params[0], params[1], HDL) 
#elif (TYPE == E_WM_IN)
   STRING2(category, file, e_2str, HDL);
   /* TODO: Check if category is sane and file exists */
   e_theme_config_set(category, file);
   SAVE;   
   END_STRING2(e_2str);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_THEME_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-theme-get", 1, "List the theme associated with the category OPT1", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(category, HDL);
   E_Config_Theme *ect;
   void *data;
   int bytes;
   
   ect = e_theme_config_get(category);
   if (ect == NULL)
     data = NULL;
   else
     data = e_ipc_codec_2str_enc(ect->category, ect->file, &bytes);
  	
   SEND_DATA(E_IPC_OP_THEME_GET_REPLY);

   END_STRING(category);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_THEME_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING2(category, file, e_2str, HDL);
   printf("REPLY: DEFAULT CATEGORY=\"%s\" FILE=\"%s\"\n",
                    category, file); 
   END_STRING2(e_2str);
#elif (TYPE == E_LIB_IN)
   STRING2(category, file, e_2str, HDL);
   RESPONSE(r, E_Response_Theme_Get);
   if (file) r->file = strdup(file);
   r->category = strdup(category);
   END_RESPONSE(r, E_RESPONSE_THEME_GET);
   END_STRING2(e_2str);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_THEME_REMOVE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-theme-remove", 1, "Remove the theme category OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(category, HDL);
   e_theme_config_remove(category);
   SAVE;
   END_STRING(category);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_FOLLOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-move-info-follows-set", 1, "Set whether the move dialog should follow the client window", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->move_info_follows = val;
   E_CONFIG_LIMIT(e_config->move_info_follows, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_FOLLOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-move-info-follows-get", 0, "Get whether the move dialog should follow the client window", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->move_info_follows, E_IPC_OP_MOVE_INFO_FOLLOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_FOLLOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_FOLLOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-resize-info-follows-set", 1, "Set whether the resize dialog should follow the client window", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->resize_info_follows = val;
   E_CONFIG_LIMIT(e_config->resize_info_follows, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_FOLLOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-resize-info-follows-get", 0, "Set whether the resize dialog should follow the client window", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->resize_info_follows, E_IPC_OP_RESIZE_INFO_FOLLOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_FOLLOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_VISIBLE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-move-info-visible-set", 1, "Set whether the move dialog should be visible", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->move_info_visible = val;
   E_CONFIG_LIMIT(e_config->move_info_visible, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_VISIBLE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-move-info-visible-get", 0, "Get whether the move dialog should be visible", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->move_info_visible, E_IPC_OP_MOVE_INFO_VISIBLE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MOVE_INFO_VISIBLE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_VISIBLE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-resize-info-visible-set", 1, "Set whether the resize dialog should be visible", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->resize_info_visible = val;
   E_CONFIG_LIMIT(e_config->resize_info_visible, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_VISIBLE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-resize-info-visible-get", 0, "Set whether the resize dialog should be visible", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->resize_info_visible, E_IPC_OP_RESIZE_INFO_VISIBLE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_RESIZE_INFO_VISIBLE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/   
#define HDL E_IPC_OP_FOCUS_LAST_FOCUSED_PER_DESKTOP_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-last-focused-per-desktop-set", 1, "Set whether E should remember focused windows when switching desks", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->focus_last_focused_per_desktop = val;
   E_CONFIG_LIMIT(e_config->focus_last_focused_per_desktop, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_LAST_FOCUSED_PER_DESKTOP_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-last-focused-per-desktop-get", 0, "Get whether E should remember focused windows when switching desks", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->focus_last_focused_per_desktop, E_IPC_OP_FOCUS_LAST_FOCUSED_PER_DESKTOP_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_LAST_FOCUSED_PER_DESKTOP_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_REVERT_ON_HIDE_OR_CLOSE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-revert-on-hide-or-close-set", 1, "Set whether E will focus the last focused window when you hide or close a focused window", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->focus_revert_on_hide_or_close = val;
   E_CONFIG_LIMIT(e_config->focus_revert_on_hide_or_close, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_REVERT_ON_HIDE_OR_CLOSE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-focus-revert-on-hide-or-close-get", 0, "Get whether E will focus the last focused window when you hide or close a focused window", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->focus_revert_on_hide_or_close, E_IPC_OP_FOCUS_REVERT_ON_HIDE_OR_CLOSE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FOCUS_REVERT_ON_HIDE_OR_CLOSE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-profile-set", 1, "Set the default configuration profile to OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   /* TODO: Check if profile exists (or create profile?) */
   e_config_save_flush();
   e_config_profile_set(s);
   e_config_profile_save();
   e_config_save_block_set(1);
   restart = 1;
   ecore_main_loop_quit();
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-profile-get", 0, "Get the default configuration profile", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config_profile_get(), E_IPC_OP_PROFILE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-profile-list", 0, "List all existing profiles", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   Eina_List *profiles;
   profiles = e_config_profile_list();
   ENCODE(profiles, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_PROFILE_LIST_REPLY);
   FREE_LIST(profiles);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_LIST_REPLY
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
#define HDL E_IPC_OP_DESKTOP_NAME_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-name-add", 5, "Add a desktop name definition. OPT1 = container no. OPT2 = zone no. OPT3 = desk_x. OPT4 = desk_y. OPT5 = desktop name", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_4INT_2STRING_START(HDL);
   REQ_4INT_2STRING_END(atoi(params[0]), atoi(params[1]), atoi(params[2]), atoi(params[3]), params[4], "", HDL);
#elif (TYPE == E_WM_IN)
   INT4_STRING2(v, HDL);
   e_desk_name_add(v->val1, v->val2, v->val3, v->val4, v->str1);
   e_desk_name_update();
   SAVE;
   END_INT4_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_NAME_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-name-del", 4, "Delete a desktop name definition. OPT1 = container no. OPT2 = zone no. OPT3 = desk_x. OPT4 = desk_y.", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_4INT_2STRING_START(HDL);
   REQ_4INT_2STRING_END(atoi(params[0]), atoi(params[1]), atoi(params[2]), atoi(params[3]), "", "", HDL);
#elif (TYPE == E_WM_IN)
   INT4_STRING2(v, HDL);
   e_desk_name_del(v->val1, v->val2, v->val3, v->val4);
   e_desk_name_update();
   SAVE;
   END_INT4_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_NAME_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desktop-name-list", 0, "List all current desktop name definitions", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT4_STRING2_LIST(e_config->desktop_names, E_Config_Desktop_Name, cfname, v, HDL);
   v->val1 = cfname->container;
   v->val2 = cfname->zone;
   v->val3 = cfname->desk_x;
   v->val4 = cfname->desk_y;
   v->str1 = (char *) cfname->name;
   v->str2 = "";
   END_SEND_INT4_STRING2_LIST(v, E_IPC_OP_DESKTOP_NAME_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_NAME_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   INT4_STRING2_LIST(v, HDL);
   printf("REPLY: BG CONTAINER=%i ZONE=%i DESK_X=%i DESK_Y=%i NAME=\"%s\"\n",
	  v->val1, v->val2, v->val3, v->val4, v->str1);
   END_INT4_STRING2_LIST(v);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CURSOR_SIZE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cursor-size-set", 1, "Set the E cursor size", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->cursor_size = val;
   E_CONFIG_LIMIT(e_config->cursor_size, 0, 1024);
   e_pointers_size_set(e_config->cursor_size);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CURSOR_SIZE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cursor-size-get", 0, "Get the E cursor size", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->cursor_size, E_IPC_OP_CURSOR_SIZE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CURSOR_SIZE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_E_CURSOR_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-e-cursor-set", 1, "Set whether E's cursor should be used", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   Eina_List *ml;
   e_config->use_e_cursor = val;
   E_CONFIG_LIMIT(e_config->use_e_cursor, 0, 1);
   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	E_Manager *man;
	man = ml->data;
	if (man->pointer) e_object_del(E_OBJECT(man->pointer));
	man->pointer = e_pointer_window_new(man->root, 1);
     }
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_E_CURSOR_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-e-cursor-get", 0, "Get whether E's cursor should be used", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->use_e_cursor, E_IPC_OP_USE_E_CURSOR_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_E_CURSOR_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_AUTOSCROLL_MARGIN_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-autoscroll-margin-set", 1, "Set the distance from the edge of the screen the menu will autoscroll to", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->menu_autoscroll_margin = value;
   E_CONFIG_LIMIT(e_config->menu_autoscroll_margin, 0, 50);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_AUTOSCROLL_MARGIN_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-autoscroll-margin-get", 0, "Get the distance from the edge of the screen the menu will autoscroll to", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->menu_autoscroll_margin, E_IPC_OP_MENU_AUTOSCROLL_MARGIN_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_AUTOSCROLL_MARGIN_GET_REPLY
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
#define HDL E_IPC_OP_MENU_AUTOSCROLL_CURSOR_MARGIN_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-autoscroll-cursor-margin-set", 1, "Set the distance from the edge of the screen the cursor needs to be to start menu autoscrolling", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->menu_autoscroll_cursor_margin = value;
   E_CONFIG_LIMIT(e_config->menu_autoscroll_cursor_margin, 0, 50);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_AUTOSCROLL_CURSOR_MARGIN_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-autoscroll-cursor-margin-get", 0, "Get the distance from the edge of the screen the cursor needs to be to start menu autoscrolling", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL)
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->menu_autoscroll_cursor_margin, E_IPC_OP_MENU_AUTOSCROLL_CURSOR_MARGIN_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_AUTOSCROLL_CURSOR_MARGIN_GET_REPLY
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
#define HDL E_IPC_OP_TRANSIENT_MOVE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-move-set", 1, "Set if transients should move with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.move = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_MOVE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-move-get", 0, "Get if transients should move with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.move, E_IPC_OP_TRANSIENT_MOVE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_MOVE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RESIZE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-resize-set", 1, "Set if transients should move when it's parent resizes", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.resize = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RESIZE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-resize-get", 0, "Get if transients should move when it's parent resizes", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.resize, E_IPC_OP_TRANSIENT_RESIZE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RESIZE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RAISE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-raise-set", 1, "Set if transients should raise with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.raise = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RAISE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-raise-get", 0, "Get if transients should raise with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.raise, E_IPC_OP_TRANSIENT_RAISE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_RAISE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LOWER_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-lower-set", 1, "Set if transients should lower with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.lower = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LOWER_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-lower-get", 0, "Get if transients should lower with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.lower, E_IPC_OP_TRANSIENT_LOWER_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LOWER_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LAYER_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-layer-set", 1, "Set if transients should change layer with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.layer = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LAYER_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-layer-get", 0, "Get if transients should change layer with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.layer, E_IPC_OP_TRANSIENT_LAYER_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_LAYER_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_DESKTOP_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-desktop-set", 1, "Set if transients should change desktop with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.desktop = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_DESKTOP_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-desktop-get", 0, "Get if transients should change desktop with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.desktop, E_IPC_OP_TRANSIENT_DESKTOP_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_DESKTOP_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_ICONIFY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-iconify-set", 1, "Set if transients should iconify with it's parent", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->transient.iconify = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_ICONIFY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transient-iconify-get", 0, "Get if transients should iconify with it's parent", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->transient.iconify, E_IPC_OP_TRANSIENT_ICONIFY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_TRANSIENT_ICONIFY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MODAL_WINDOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-modal-windows-set", 1, "Set if enlightenment should honour modal windows", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->modal_windows = val;
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MODAL_WINDOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-modal-windows-get", 0, "Get if enlightenment should honour modal windows", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->modal_windows, E_IPC_OP_MODAL_WINDOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MODAL_WINDOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL
   
/****************************************************************************/
#define HDL E_IPC_OP_IM_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-input-method-list", 0, "List all available input methods", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   Eina_List *iml;
   iml = e_intl_input_method_list();
   ENCODE(iml, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_IM_LIST_REPLY);
   FREE_LIST(iml);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_IM_LIST_REPLY
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
#define HDL E_IPC_OP_IM_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-input-method-set", 1, "Set the current input method to 'OPT1'", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (e_config->input_method) eina_stringshare_del(e_config->input_method);
   e_config->input_method = NULL;
   if (s) e_config->input_method = eina_stringshare_add(s);
   if ((e_config->input_method) && (e_config->input_method[0] != 0))
     e_intl_input_method_set(e_config->input_method);
   else
     e_intl_input_method_set(NULL);
   SAVE;
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_IM_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-input-method-get", 0, "Get the current input method", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->input_method, E_IPC_OP_IM_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_IM_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINDOW_PLACEMENT_POLICY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-window-placement-policy-set", 1, "Set the window placement policy. OPT1 = SMART, ANTIGADGET, CURSOR or MANUAL", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "SMART")) value = E_WINDOW_PLACEMENT_SMART;
   else if (!strcmp(params[0], "CURSOR")) value = E_WINDOW_PLACEMENT_CURSOR;
   else if (!strcmp(params[0], "MANUAL")) value = E_WINDOW_PLACEMENT_MANUAL;
   else if (!strcmp(params[0], "ANTIGADGET")) value = E_WINDOW_PLACEMENT_ANTIGADGET;
   else
     {
	 printf("window placement policy must be SMART, ANTIGADGET, CURSOR or MANUAL\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->window_placement_policy = value;
   E_CONFIG_LIMIT(e_config->window_placement_policy, E_WINDOW_PLACEMENT_SMART, E_WINDOW_PLACEMENT_MANUAL);
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINDOW_PLACEMENT_POLICY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-window-placement-policy-get", 0, "Get window placement policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->window_placement_policy, E_IPC_OP_WINDOW_PLACEMENT_POLICY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINDOW_PLACEMENT_POLICY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == E_WINDOW_PLACEMENT_SMART)
     printf("REPLY: SMART\n");
   else if (policy == E_WINDOW_PLACEMENT_CURSOR)
     printf("REPLY: CURSOR\n");
   else if (policy == E_WINDOW_PLACEMENT_MANUAL)
     printf("REPLY: MANUAL\n");
   END_INT
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_SIGNAL_LIST 
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-signal-list", 0, "List all signal bindings", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action or send reply */
   SEND_INT3_STRING4_LIST(e_config->signal_bindings, E_Config_Binding_Signal, emb, v, HDL);
   v->val1 = emb->context;
   v->str1 = (char *) emb->signal;
   v->str2 = (char *) emb->source;
   v->val2 = emb->modifiers;
   v->val3 = emb->any_mod;
   v->str3 = (char *) emb->action;
   v->str4 = (char *) emb->params;
   END_SEND_INT3_STRING4_LIST(v, E_IPC_OP_BINDING_SIGNAL_LIST_REPLY);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_SIGNAL_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   /* e_remote decode the response from e_ipc and print it to the console */
   INT3_STRING4_LIST(v, HDL);
   {
      char *context;
      char modifier[256];
      char *s1, *s2, *s3, *s4;
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_CONTAINER) context = "CONTAINER";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_MENU) context = "MENU";
      else if (v->val1 == E_BINDING_CONTEXT_WINLIST) context = "WINLIST";
      else if (v->val1 == E_BINDING_CONTEXT_POPUP) context = "POPUP";
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

      s1 = v->str1;
      s2 = v->str2;
      s3 = v->str3;
      s4 = v->str4;
      if (!s1) s1 = "";
      if (!s2) s2 = "";
      if (!s3) s3 = "";
      if (!s4) s4 = "";
      printf("REPLY: BINDING CONTEXT=%s SIGNAL=%s SOURCE=%s MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
		s1,
		s2,
		modifier,
                v->val3,
                s3,
                s4
        );
   }
   END_INT3_STRING4_LIST(v);
#elif E_LIB_IN
   INT3_STRING4_LIST_START(v, HDL);
   {
      int count;
      RESPONSE(r, E_Response_Binding_Signal_List);
      count = eina_list_count(dat);
      r->bindings = malloc(sizeof(E_Response_Binding_Signal_Data *) * count);
      r->count = count;

      count = 0;
      INT3_STRING4_LIST_ITERATE(v);
      {
	 E_Response_Binding_Signal_Data *d;

	 d = malloc(sizeof(E_Response_Binding_Signal_Data));
	 d->ctx = v->val1;
	 d->signal = ((v->str1) ? eina_stringshare_add(v->str1) : NULL);
	 d->source = ((v->str2) ? eina_stringshare_add(v->str2) : NULL);
	 d->mod = v->val2;
	 d->any_mod = v->val3;
	 d->action = ((v->str3) ? eina_stringshare_add(v->str3) : NULL);
	 d->params = ((v->str4) ? eina_stringshare_add(v->str4) : NULL);

	 r->bindings[count] = d;
	 count++;
      }
      END_INT3_STRING4_LIST_ITERATE(v);
      /* this will leak, need to free the event data */
      END_RESPONSE(r, E_RESPONSE_BINDING_SIGNAL_LIST);
   }
   END_INT3_STRING4_LIST_START();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_SIGNAL_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-signal-add", 7, "Add an existing signal binding. OPT1 = Context, OPT2 = signal, OPT3 = source, OPT4 = modifiers, OPT5 = any modifier ok, OPT6 = action, OPT7 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_3INT_4STRING_START(HDL);
   E_Config_Binding_Signal eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
        exit(-1);
     }
   eb.signal = params[1];
   eb.source = params[2];
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[3];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (pp[0] != 0)
                    {
                       printf("OPT4 moidifier unknown. Must be or mask of:\n"
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
                  else if (pp[0] != 0)
                    {
                       printf("OPT4 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[4]);
   eb.action = params[5];
   eb.params = params[6];
   REQ_3INT_4STRING_END(eb.context, eb.modifiers, eb.any_mod, eb.signal, eb.source, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT3_STRING4(v, HDL)
   E_Config_Binding_Signal bind, *eb;

   bind.context = v->val1; 
   bind.signal = v->str1; 
   bind.source = v->str2; 
   bind.modifiers = v->val2;
   bind.any_mod = v->val3;
   bind.action = v->str3; 
   bind.params = v->str4;

   eb = e_config_binding_signal_match(&bind);
   if (!eb)
     {
        eb = E_NEW(E_Config_Binding_Signal, 1);
        e_config->signal_bindings = eina_list_append(e_config->signal_bindings, eb);
        eb->context = bind.context;
        if (bind.signal) eb->signal = eina_stringshare_add(bind.signal);
        if (bind.source) eb->source = eina_stringshare_add(bind.source);
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        if (bind.action) eb->action = eina_stringshare_add(bind.action);
        if (bind.params) eb->params = eina_stringshare_add(bind.params);
        e_bindings_signal_add(bind.context, bind.signal, bind.source, bind.modifiers,
			      bind.any_mod, bind.action, bind.params);
        e_config_save_queue();  
     }
   END_INT3_STRING4(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_SIGNAL_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-signal-del", 7, "Delete an existing signal binding. OPT1 = Context, OPT2 = signal, OPT3 = source, OPT4 = modifiers, OPT5 = any modifier ok, OPT6 = action, OPT7 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_3INT_4STRING_START(HDL);
   E_Config_Binding_Signal eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
        exit(-1);
     }
   eb.signal = params[1];
   eb.source = params[2];
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[3];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[4]);
   eb.action = params[5];
   eb.params = params[6];
 
   REQ_3INT_4STRING_END(eb.context, eb.modifiers, eb.any_mod, eb.signal, eb.source, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT3_STRING4(v, HDL)
   E_Config_Binding_Signal bind, *eb;

   bind.context = v->val1;
   bind.signal = v->str1;
   bind.source = v->str2;
   bind.modifiers = v->val2;
   bind.any_mod = v->val3;
   bind.action = v->str3;
   bind.params = v->str4;
   
   eb = e_config_binding_signal_match(&bind);
   if (eb)
     {
	e_config->signal_bindings = eina_list_remove(e_config->signal_bindings, eb);
        if (eb->signal) eina_stringshare_del(eb->signal);
        if (eb->source) eina_stringshare_del(eb->source);
        if (eb->action) eina_stringshare_del(eb->action);
        if (eb->params) eina_stringshare_del(eb->params);
        E_FREE(eb);
        e_bindings_signal_del(bind.context, bind.signal, bind.source, bind.modifiers,
			      bind.any_mod, bind.action, bind.params);
        e_config_save_queue();
     }
   END_INT3_STRING4(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_WHEEL_LIST 
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-wheel-list", 0, "List all wheel bindings", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action or send reply */
   SEND_INT5_STRING2_LIST(e_config->wheel_bindings, E_Config_Binding_Wheel, emb, v, HDL);
   v->val1 = emb->context;
   v->val2 = emb->direction;
   v->val3 = emb->z;
   v->val4 = emb->modifiers;
   v->val5 = emb->any_mod;
   v->str1 = (char *) emb->action;
   v->str2 = (char *) emb->params;
   END_SEND_INT5_STRING2_LIST(v, E_IPC_OP_BINDING_WHEEL_LIST_REPLY);
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_WHEEL_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   /* e_remote decode the response from e_ipc and print it to the console */
   INT5_STRING2_LIST(v, HDL);
   {
      char *context;
      char modifier[256];
      char *s1, *s2;
      
      if (v->val1 == E_BINDING_CONTEXT_NONE) context = "NONE";
      else if (v->val1 == E_BINDING_CONTEXT_UNKNOWN) context = "UNKNOWN";
      else if (v->val1 == E_BINDING_CONTEXT_BORDER) context = "BORDER";
      else if (v->val1 == E_BINDING_CONTEXT_ZONE) context = "ZONE";
      else if (v->val1 == E_BINDING_CONTEXT_CONTAINER) context = "CONTAINER";
      else if (v->val1 == E_BINDING_CONTEXT_MANAGER) context = "MANAGER";
      else if (v->val1 == E_BINDING_CONTEXT_MENU) context = "MENU";
      else if (v->val1 == E_BINDING_CONTEXT_WINLIST) context = "WINLIST";
      else if (v->val1 == E_BINDING_CONTEXT_POPUP) context = "POPUP";
      else if (v->val1 == E_BINDING_CONTEXT_ANY) context = "ANY";
      else context = "";
 
      modifier[0] = 0;
      if (v->val4 & E_BINDING_MODIFIER_SHIFT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "SHIFT");
      }
      if (v->val4 & E_BINDING_MODIFIER_CTRL)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "CTRL");
      }
      if (v->val4 & E_BINDING_MODIFIER_ALT)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "ALT");
      }
      if (v->val4 & E_BINDING_MODIFIER_WIN)
      {
        if (modifier[0] != 0) strcat(modifier, "|");
        strcat(modifier, "WIN");
      }
      if (v->val4 == E_BINDING_MODIFIER_NONE)
         strcpy(modifier, "NONE");

      s1 = v->str1;
      s2 = v->str2;
      if (!s1) s1 = "";
      if (!s2) s2 = "";
      printf("REPLY: BINDING CONTEXT=%s DIRECTION=%i Z=%i MODIFIERS=%s ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n",
		context,
                v->val2,
                v->val3,
		modifier,
                v->val5,
                s1,
                s2
        );
   }
   END_INT5_STRING2_LIST(v);
#elif E_LIB_IN
   INT5_STRING2_LIST_START(v, HDL);
   {
      int count;
      RESPONSE(r, E_Response_Binding_Wheel_List);
      count = eina_list_count(dat);
      r->bindings = malloc(sizeof(E_Response_Binding_Wheel_Data *) * count);
      r->count = count;

      count = 0;
      INT5_STRING2_LIST_ITERATE(v);
      {
	 E_Response_Binding_Wheel_Data *d;

	 d = malloc(sizeof(E_Response_Binding_Wheel_Data));
	 d->ctx = v->val1;
	 d->direction = v->val2;
	 d->z = v->val3;
	 d->mod = v->val4;
	 d->any_mod = v->val5;
	 d->action = ((v->str1) ? eina_stringshare_add(v->str1) : NULL);
	 d->params = ((v->str2) ? eina_stringshare_add(v->str2) : NULL);

	 r->bindings[count] = d;
	 count++;
      }
      END_INT5_STRING2_LIST_ITERATE(v);
      /* this will leak, need to free the event data */
      END_RESPONSE(r, E_RESPONSE_BINDING_WHEEL_LIST);
   }
   END_INT5_STRING2_LIST_START();
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_WHEEL_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-wheel-add", 7, "Add an existing wheel binding. OPT1 = Context, OPT2 = direction, OPT3 = z, OPT4 = modifiers, OPT5 = any modifier ok, OPT6 = action, OPT7 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_5INT_2STRING_START(HDL);
   E_Config_Binding_Wheel eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
        exit(-1);
     }
   eb.direction = atoi(params[1]);
   eb.z = atoi(params[2]);
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[3];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (pp[0] != 0)
                    {
                       printf("OPT4 moidifier unknown. Must be or mask of:\n"
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
                  else if (pp[0] != 0)
                    {
                       printf("OPT4 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[4]);
   eb.action = params[5];
   eb.params = params[6];
   REQ_5INT_2STRING_END(eb.context, eb.direction, eb.z, eb.modifiers, eb.any_mod, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT5_STRING2(v, HDL)
   E_Config_Binding_Wheel bind, *eb;

   bind.context = v->val1; 
   bind.direction = v->val2;
   bind.z = v->val3;
   bind.modifiers = v->val4;
   bind.any_mod = v->val5;
   bind.action = v->str1; 
   bind.params = v->str2;

   eb = e_config_binding_wheel_match(&bind);
   if (!eb)
     {
        eb = E_NEW(E_Config_Binding_Wheel, 1);
        e_config->wheel_bindings = eina_list_append(e_config->wheel_bindings, eb);
        eb->context = bind.context;
        eb->direction = bind.direction;
        eb->z = bind.z;
        eb->modifiers = bind.modifiers;
        eb->any_mod = bind.any_mod;
        if (bind.action) eb->action = eina_stringshare_add(bind.action);
        if (bind.params) eb->params = eina_stringshare_add(bind.params);
        e_bindings_wheel_add(bind.context, bind.direction, bind.z, bind.modifiers,
			     bind.any_mod, bind.action, bind.params);
        e_config_save_queue();  
     }
   END_INT5_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_BINDING_WHEEL_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   /* e_remote define command line args */
   OP("-binding-wheel-del", 7, "Delete an existing wheel binding. OPT1 = Context, OPT2 = direction, OPT3 = z, OPT4 = modifiers, OPT5 = any modifier ok, OPT6 = action, OPT7 = action parameters", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   /* e_remote parse command line args encode and send request */
   REQ_5INT_2STRING_START(HDL);
   E_Config_Binding_Wheel eb;

   if (!strcmp(params[0], "NONE")) eb.context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb.context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb.context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb.context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "CONTAINER")) eb.context = E_BINDING_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "MANAGER")) eb.context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "MENU")) eb.context = E_BINDING_CONTEXT_MENU;
   else if (!strcmp(params[0], "WINLIST")) eb.context = E_BINDING_CONTEXT_WINLIST;
   else if (!strcmp(params[0], "POPUP")) eb.context = E_BINDING_CONTEXT_POPUP;
   else if (!strcmp(params[0], "ANY")) eb.context = E_BINDING_CONTEXT_ANY;
   else
     {
        printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
               "  NONE UNKNOWN BORDER ZONE CONTAINER MANAGER MENU WINLIST POPUP ANY\n");
        exit(-1);
     }
   eb.direction = atoi(params[1]);
   eb.z = atoi(params[2]);
   /* M1[|M2...] */
     {
        char *p, *pp;

        eb.modifiers = 0;
        pp = params[3];
        for (;;)
          {
             p = strchr(pp, '|');
             if (p)
               {
                  if (!strncmp(pp, "SHIFT|", 6)) eb.modifiers |= E_BINDING_MODIFIER_SHIFT;
                  else if (!strncmp(pp, "CTRL|", 5)) eb.modifiers |= E_BINDING_MODIFIER_CTRL;
                  else if (!strncmp(pp, "ALT|", 4)) eb.modifiers |= E_BINDING_MODIFIER_ALT;
                  else if (!strncmp(pp, "WIN|", 4)) eb.modifiers |= E_BINDING_MODIFIER_WIN;
                  else if (pp[0] != 0)
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
                  else if (pp[0] != 0)
                    {
                       printf("OPT3 moidifier unknown. Must be or mask of:\n"
                              "  SHIFT CTRL ALT WIN (eg SHIFT|CTRL or ALT|SHIFT|CTRL or ALT or just NONE)\n");
                       exit(-1);
                    }
                  break;
               }
          }
     }
   eb.any_mod = atoi(params[4]);
   eb.action = params[5];
   eb.params = params[6];
 
   REQ_5INT_2STRING_END(eb.context, eb.direction, eb.z, eb.modifiers, eb.any_mod, eb.action, eb.params, HDL);
#elif (TYPE == E_WM_IN)
   /* e_ipc decode request and do action */
   INT5_STRING2(v, HDL)
   E_Config_Binding_Wheel bind, *eb;

   bind.context = v->val1;
   bind.direction = v->val2;
   bind.z = v->val3;
   bind.modifiers = v->val4;
   bind.any_mod = v->val5;
   bind.action = v->str1;
   bind.params = v->str2;
   
   eb = e_config_binding_wheel_match(&bind);
   if (eb)
     {
	e_config->wheel_bindings = eina_list_remove(e_config->wheel_bindings, eb);
        if (eb->action) eina_stringshare_del(eb->action);
        if (eb->params) eina_stringshare_del(eb->params);
        E_FREE(eb);
        e_bindings_wheel_del(bind.context, bind.direction, bind.z, bind.modifiers,
			     bind.any_mod, bind.action, bind.params);
        e_config_save_queue();
     }
   END_INT5_STRING2(v);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_FOCUS_WHILE_SELECTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-focus-while-selecting-set", 1, "Set winlist (alt+tab) focus while selecting policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_focus_while_selecting = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_focus_while_selecting, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_FOCUS_WHILE_SELECTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-focus-while-selecting-get", 0, "Get winlist (alt+tab) focus while selecting policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_focus_while_selecting, E_IPC_OP_WINLIST_LIST_FOCUS_WHILE_SELECTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_FOCUS_WHILE_SELECTING_GET_REPLY
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
#define HDL E_IPC_OP_WINLIST_LIST_RAISE_WHILE_SELECTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-raise-while-selecting-set", 1, "Set winlist (alt+tab) raise while selecting policy", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->winlist_list_raise_while_selecting = policy;
   E_CONFIG_LIMIT(e_config->winlist_list_raise_while_selecting, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_RAISE_WHILE_SELECTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-winlist-list-raise-while-selecting-get", 0, "Get winlist (alt+tab) raise while selecting policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->winlist_list_raise_while_selecting, E_IPC_OP_WINLIST_LIST_RAISE_WHILE_SELECTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_WINLIST_LIST_RAISE_WHILE_SELECTING_GET_REPLY
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
#define HDL E_IPC_OP_THEME_CATEGORY_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-theme-category-list", 0, "List all available theme categories", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   ENCODE(e_theme_category_list(), e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_THEME_CATEGORY_LIST_REPLY);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_THEME_CATEGORY_LIST_REPLY
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
#define HDL E_IPC_OP_TRANSITION_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-transition-list", 0, "List all available transitions", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   ENCODE(e_theme_transition_list(), e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_TRANSITION_LIST_REPLY);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_TRANSITION_LIST_REPLY
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
#define HDL E_IPC_OP_ACTION_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-action-list", 0, "List all available actions", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   ENCODE(e_action_name_list(), e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_ACTION_LIST_REPLY);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_ACTION_LIST_REPLY
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
#define HDL E_IPC_OP_PROFILE_ADD
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-profile-add", 1, "Add profile named OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   e_config_profile_add(s);
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_PROFILE_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-profile-del", 1, "Delete profile named OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (!strcmp(e_config_profile_get(), s))
     {
	printf("Can't delete active profile\n");
	exit(-1);
     }
   e_config_profile_del(s);
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DEFAULT_ENGINE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-engine-set", 1, "Set the default rendering engine to OPT1 (SOFTWARE, SOFTWARE_16 or XRENDER)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "SOFTWARE")) value = E_EVAS_ENGINE_SOFTWARE_X11;
   else if (!strcmp(params[0], "GL"))
     {
	value = E_EVAS_ENGINE_GL_X11;
	printf("GL engine is disabled as default engine.\n");
	exit(-1);
     }
   else if (!strcmp(params[0], "XRENDER")) value = E_EVAS_ENGINE_XRENDER_X11;
   else if (!strcmp(params[0], "SOFTWARE_16")) value = E_EVAS_ENGINE_SOFTWARE_X11_16;
   else
     {
	printf("engine must be SOFTWARE or XRENDER\n");
	exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->evas_engine_default = value;
   E_CONFIG_LIMIT(e_config->evas_engine_default, E_EVAS_ENGINE_SOFTWARE_X11, E_EVAS_ENGINE_XRENDER_X11);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DEFAULT_ENGINE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-default-engine-get", 0, "Get the default rendering engine", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->evas_engine_default, E_IPC_OP_DEFAULT_ENGINE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_DEFAULT_ENGINE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(engine, HDL);
   if (engine == E_EVAS_ENGINE_SOFTWARE_X11)
     printf("REPLY: SOFTWARE\n");
   else if (engine == E_EVAS_ENGINE_GL_X11)
     printf("REPLY: GL\n");
   else if (engine == E_EVAS_ENGINE_XRENDER_X11)
     printf("REPLY: XRENDER\n");
   else if (engine == E_EVAS_ENGINE_SOFTWARE_X11_16)
     printf("REPLY: SOFTWARE_16\n");
   else
     printf("REPLY: UNKNOWN ENGINE: %d\n", engine);
   END_INT
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ENGINE_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-engine-list", 0, "List all existing rendering engines", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   Eina_List *engines;
   engines = e_config_engine_list();
   ENCODE(engines, e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_ENGINE_LIST_REPLY);
   FREE_LIST(engines);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ENGINE_LIST_REPLY
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
#define HDL E_IPC_OP_ENGINE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-engine-set", 2, "Set the rendering engine for OPT1 to OPT2 (SOFTWARE or XRENDER)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_2INT_START(HDL)
   int context = 0, engine = 0;
   if (!strcmp(params[0], "INIT")) context = E_ENGINE_CONTEXT_INIT;
   else if (!strcmp(params[0], "CONTAINER")) context = E_ENGINE_CONTEXT_CONTAINER;
   else if (!strcmp(params[0], "ZONE")) context = E_ENGINE_CONTEXT_ZONE;
   else if (!strcmp(params[0], "BORDER")) context = E_ENGINE_CONTEXT_BORDER;
   else if (!strcmp(params[0], "MENU")) context = E_ENGINE_CONTEXT_MENU;
   else if (!strcmp(params[0], "ERROR")) context = E_ENGINE_CONTEXT_ERROR;
   else if (!strcmp(params[0], "WIN")) context = E_ENGINE_CONTEXT_WIN;
   else if (!strcmp(params[0], "POPUP")) context = E_ENGINE_CONTEXT_POPUP;
   else if (!strcmp(params[0], "DRAG")) context = E_ENGINE_CONTEXT_DRAG;
   else
     {
	 printf("context must be INIT, CONTAINER, ZONE, BORDER, MENU, ERROR, WIN, POPUP or DRAG\n");
	 exit(-1);
     }
   if (!strcmp(params[1], "DEFAULT")) engine = E_EVAS_ENGINE_DEFAULT;
   else if (!strcmp(params[1], "SOFTWARE")) engine = E_EVAS_ENGINE_SOFTWARE_X11;
   else if (!strcmp(params[1], "GL"))
     {
	engine = E_EVAS_ENGINE_GL_X11;
	printf("GL engine is disabled as default engine.\n");
	exit(-1);
     }
   else if (!strcmp(params[1], "XRENDER")) engine = E_EVAS_ENGINE_XRENDER_X11;
   else if (!strcmp(params[1], "SOFTWARE_16")) engine = E_EVAS_ENGINE_SOFTWARE_X11_16;
   else
     {
	 printf("engine must be DEFAULT, SOFTWARE or XRENDER\n");
	 exit(-1);
     }
   REQ_2INT_END(context, engine, HDL);
#elif (TYPE == E_WM_IN)
   START_2INT(context, engine, HDL);
   switch (context)
     {
      case E_ENGINE_CONTEXT_INIT:
	 e_config->evas_engine_init = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_init, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_CONTAINER:
	 e_config->evas_engine_container = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_container, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_ZONE:
	 e_config->evas_engine_zone = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_zone, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_BORDER:
	 e_config->evas_engine_borders = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_borders, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_MENU:
	 e_config->evas_engine_menus = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_menus, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_ERROR:
	 e_config->evas_engine_errors = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_errors, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_WIN:
	 e_config->evas_engine_win = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_win, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_POPUP:
	 e_config->evas_engine_popups = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_popups, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      case E_ENGINE_CONTEXT_DRAG:
	 e_config->evas_engine_drag = engine;
	 E_CONFIG_LIMIT(e_config->evas_engine_drag, E_EVAS_ENGINE_DEFAULT, E_EVAS_ENGINE_XRENDER_X11);
	 break;
      default:
	 printf("Unknown context: %d\n", context);
	 break;
     }
   SAVE;
   END_2INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_ENGINE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-engine-get", 1, "Get the rendering engine for OPT1", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(context, HDL);
   int engine = 0;
   /* TODO, this should come from a define */
   int bytes = 0;
   switch (context)
     {
      case E_ENGINE_CONTEXT_INIT:
	 engine = e_config->evas_engine_init;
	 break;
      case E_ENGINE_CONTEXT_CONTAINER:
	 engine = e_config->evas_engine_container;
	 break;
      case E_ENGINE_CONTEXT_ZONE:
	 engine = e_config->evas_engine_zone;
	 break;
      case E_ENGINE_CONTEXT_BORDER:
	 engine = e_config->evas_engine_borders;
	 break;
      case E_ENGINE_CONTEXT_MENU:
	 engine = e_config->evas_engine_menus;
	 break;
      case E_ENGINE_CONTEXT_ERROR:
	 engine = e_config->evas_engine_errors;
	 break;
      case E_ENGINE_CONTEXT_WIN:
	 engine = e_config->evas_engine_win;
	 break;
      case E_ENGINE_CONTEXT_POPUP:
	 engine = e_config->evas_engine_popups;
	 break;
      case E_ENGINE_CONTEXT_DRAG:
	 engine = e_config->evas_engine_drag;
	 break;
      default:
	 printf("Unknown context: %d\n", context);
	 break;
     }
   ENCODE(engine, e_ipc_codec_int_enc);
   SEND_DATA(E_IPC_OP_ENGINE_GET_REPLY);

   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_ENGINE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(engine, HDL);
   if (engine == E_EVAS_ENGINE_DEFAULT)
     printf("REPLY: DEFAULT\n");
   else if (engine == E_EVAS_ENGINE_SOFTWARE_X11)
     printf("REPLY: SOFTWARE\n");
   else if (engine == E_EVAS_ENGINE_GL_X11)
     printf("REPLY: GL\n");
   else if (engine == E_EVAS_ENGINE_XRENDER_X11)
     printf("REPLY: XRENDER\n");
   else if (engine == E_EVAS_ENGINE_SOFTWARE_X11_16)
     printf("REPLY: SOFTWARE_16\n");
   else
     printf("REPLY: UNKNOWN ENGINE: %d\n", engine);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_NAME_SHOW_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-name-show-set", 1, "Set whether to show eapps' name field in menus", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->menu_eap_name_show = val;
   E_CONFIG_LIMIT(e_config->menu_eap_name_show, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_NAME_SHOW_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-name-show-get", 0, "Get whether eapps' name field is shown in menus", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->menu_eap_name_show, E_IPC_OP_MENU_EAP_NAME_SHOW_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_NAME_SHOW_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_GENERIC_SHOW_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-generic-show-set", 1, "Set whether to show eapps' generic field in menus", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->menu_eap_generic_show = val;
   E_CONFIG_LIMIT(e_config->menu_eap_generic_show, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_GENERIC_SHOW_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-generic-show-get", 0, "Get whether eapps' generic field is shown in menus", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->menu_eap_generic_show, E_IPC_OP_MENU_EAP_GENERIC_SHOW_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_GENERIC_SHOW_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_COMMENT_SHOW_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-comment-show-set", 1, "Set whether to show eapps' comment field in menus", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->menu_eap_comment_show = val;
   E_CONFIG_LIMIT(e_config->menu_eap_comment_show, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_COMMENT_SHOW_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-menu-eap-comment-show-get", 0, "Get whether eapps' comment field is shown in menus", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->menu_eap_comment_show, E_IPC_OP_MENU_EAP_COMMENT_SHOW_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_MENU_EAP_COMMENT_SHOW_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FULLSCREEN_POLICY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-fullscreen-policy-set", 1, "Set the fullscreen policy. OPT1 = RESIZE or ZOOM", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "RESIZE")) value = E_FULLSCREEN_RESIZE;
   else if (!strcmp(params[0], "ZOOM")) value = E_FULLSCREEN_ZOOM;
   else
     {
	 printf("fullscreen must be RESIZE or ZOOM\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->fullscreen_policy = value;
   E_CONFIG_LIMIT(e_config->fullscreen_policy, E_FULLSCREEN_RESIZE, E_FULLSCREEN_ZOOM);
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FULLSCREEN_POLICY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-fullscreen-policy-get", 0, "Get fullscreen policy", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->fullscreen_policy, E_IPC_OP_FULLSCREEN_POLICY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FULLSCREEN_POLICY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == E_FULLSCREEN_RESIZE)
     printf("REPLY: RESIZE\n");
   else if (policy == E_FULLSCREEN_ZOOM)
     printf("REPLY: ZOOM\n");
   END_INT
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color-set", 5, "Set color_class (OPT1) r, g, b, a (OPT2-5)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING_4INT(params[0], atoi(params[1]), atoi(params[2]), atoi(params[3]), atoi(params[4]), HDL) 
#elif (TYPE == E_WM_IN)
   STRING_4INT(color_class, r, g, b, a, e_str_4int, HDL);
   e_color_class_set(color_class, r, g, b, a, -1, -1, -1, -1, -1, -1, -1, -1);
   SAVE;   
   END_STRING_4INT(e_str_4int);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR2_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color2-set", 5, "Set color_class (OPT1) color2 r, g, b, a (OPT2-5)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING_4INT(params[0], atoi(params[1]), atoi(params[2]), atoi(params[3]), atoi(params[4]), HDL) 
#elif (TYPE == E_WM_IN)
   STRING_4INT(color_class, r, g, b, a, e_str_4int, HDL);
   e_color_class_set(color_class, -1, -1, -1, -1, r, g, b, a, -1, -1, -1, -1);
   SAVE;   
   END_STRING_4INT(e_str_4int);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR3_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color3-set", 5, "Set color_class (OPT1) color3 r, g, b, a (OPT2-5)", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING_4INT(params[0], atoi(params[1]), atoi(params[2]), atoi(params[3]), atoi(params[4]), HDL) 
#elif (TYPE == E_WM_IN)
   STRING_4INT(color_class, r, g, b, a, e_str_4int, HDL);
   e_color_class_set(color_class, -1, -1, -1, -1, -1, -1, -1, -1, r, g, b, a);
   SAVE;   
   END_STRING_4INT(e_str_4int);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color-list", 0, "List color values for all set color classes", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING_INT4_LIST(e_color_class_list(), E_Color_Class, cc, v, HDL);
   v->str = (char *) cc->name;
   v->val1 = cc->r;
   v->val2 = cc->g;
   v->val3 = cc->b;
   v->val4 = cc->a;
   END_SEND_STRING_INT4_LIST(v, E_IPC_OP_COLOR_CLASS_COLOR_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR2_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color2-list", 0, "List color2 values for all set color classes", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING_INT4_LIST(e_color_class_list(), E_Color_Class, cc, v, HDL);
   v->str = (char *) cc->name;
   v->val1 = cc->r2;
   v->val2 = cc->g2;
   v->val3 = cc->b2;
   v->val4 = cc->a2;
   END_SEND_STRING_INT4_LIST(v, E_IPC_OP_COLOR_CLASS_COLOR_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR3_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-color3-list", 0, "List color3 values for all set color classes", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING_INT4_LIST(e_color_class_list(), E_Color_Class, cc, v, HDL);
   v->str = (char *) cc->name;
   v->val1 = cc->r3;
   v->val2 = cc->g3;
   v->val3 = cc->b3;
   v->val4 = cc->a3;
   END_SEND_STRING_INT4_LIST(v, E_IPC_OP_COLOR_CLASS_COLOR_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_DEL
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-del", 1, "Delete color class named OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   e_color_class_del(s);
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_COLOR_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING_INT4_LIST(v, HDL);
   if (v->str) printf("REPLY: \"%s\" (RGBA) %i %i %i %i\n", v->str, v->val1, v->val2, v->val3, v->val4);
   else printf("REPLY: \"\" (RGBA) %i %i %i %i\n", v->val1, v->val2, v->val3, v->val4);
   END_STRING_INT4_LIST(v);
#elif (TYPE == E_LIB_IN)
   /* FIXME implement */
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-color-class-list", 0, "List all color classes used by currently loaded edje objects", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   ENCODE_START();
   ENCODE(edje_color_class_list(), e_ipc_codec_str_list_enc);
   SEND_DATA(E_IPC_OP_COLOR_CLASS_LIST_REPLY);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_COLOR_CLASS_LIST_REPLY
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
#define HDL E_IPC_OP_CFGDLG_AUTO_APPLY_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cfgdlg-auto-apply-set", 1, "Set config dialogs to use auto apply, 1 for enabled 0 for disabled", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->cfgdlg_auto_apply = policy;
   E_CONFIG_LIMIT(e_config->cfgdlg_auto_apply, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CFGDLG_AUTO_APPLY_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cfgdlg-auto-apply-get", 0, "Get config dialogs use auto apply policy, 1 for enabled 0 for disabled", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->cfgdlg_auto_apply, E_IPC_OP_CFGDLG_AUTO_APPLY_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CFGDLG_AUTO_APPLY_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == 0) 
     printf("REPLY: Disabled\n");
   if (policy == 1) 
     printf("REPLY: Enabled\n");
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CFGDLG_DEFAULT_MODE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cfgdlg-default-mode-set", 1, "Set default mode for config dialogs. OPT1 = BASIC or ADVANCED", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT_START(HDL)
   int value = 0;
   if (!strcmp(params[0], "BASIC")) value = E_CONFIG_DIALOG_CFDATA_TYPE_BASIC;
   else if (!strcmp(params[0], "ADVANCED")) value = E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED;
   else
     {
	 printf("default mode must be BASIC or ADVANCED\n");
	 exit(-1);
     }
   REQ_INT_END(value, HDL);
#elif (TYPE == E_WM_IN)
   START_INT(value, HDL);
   e_config->cfgdlg_default_mode = value;
   E_CONFIG_LIMIT(e_config->cfgdlg_default_mode, E_CONFIG_DIALOG_CFDATA_TYPE_BASIC, E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED);
   SAVE;
   END_INT
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CFGDLG_DEFAULT_MODE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-cfgdlg-default-mode-get", 0, "Get default mode for config dialogs", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->cfgdlg_default_mode, E_IPC_OP_CFGDLG_DEFAULT_MODE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_CFGDLG_DEFAULT_MODE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == E_CONFIG_DIALOG_CFDATA_TYPE_BASIC)
     printf("REPLY: BASIC\n");
   else if (policy == E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED)
     printf("REPLY: ADVANCED\n");
   END_INT
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKTOP_LOCK
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-lock-desktop", 0, "Locks the desktop", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   e_desklock_show();
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_INIT_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-init-set", 1, "Set edje file (OPT1) to use for init screen", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (e_config->init_default_theme) eina_stringshare_del(e_config->init_default_theme);
   e_config->init_default_theme = NULL;
   if (s) 
     {
	const char *i;
	const char *f;
	f = ecore_file_file_get(s);
	i = e_path_find(path_themes, f);
	if (!e_util_edje_collection_exists(i, "init/splash")) 
	  {
	     printf("The edje file you selected does not contain any init information.\n");
	     exit(-1);
	  }
	if (f) e_config->init_default_theme = eina_stringshare_add(f);
     }
  SAVE;
  END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_INIT_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-init-get", 0, "Get the current edje file for init screen", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->init_default_theme, E_IPC_OP_INIT_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_INIT_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   if (s) printf("REPLY: \"%s\"\n", s);
   else printf("REPLY: \"\"\n");
   END_STRING(s);
#elif (TYPE == E_LIB_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_HINTING_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-hinting-set", 1, "Set font hinting method to use, 0 = Bytecode, 1 = Auto, 2 = None", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->font_hinting = policy;
   E_CONFIG_LIMIT(e_config->font_hinting, 0, 2);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_HINTING_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-font-hinting-get", 0, "Get font hinting method", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->font_hinting, E_IPC_OP_FONT_HINTING_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_FONT_HINTING_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(policy, HDL);
   if (policy == 0) 
     printf("REPLY: Bytecode\n");
   else if (policy == 1) 
     printf("REPLY: Auto\n");
   else if (policy == 2) 
     printf("REPLY: None\n"); 
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_COMPOSITE_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-composite-set", 1, "Set whether composite should be used", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->use_composite = val;
   E_CONFIG_LIMIT(e_config->use_composite, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_COMPOSITE_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-use-composite-get", 0, "Get whether composite should be used", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->use_composite, E_IPC_OP_USE_COMPOSITE_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_USE_COMPOSITE_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_REMEMBER_INTERNAL_WINDOWS_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-remember-internal-windows-set", 1, "Set whether internal windows should be remembered", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(val, HDL);
   e_config->remember_internal_windows = val;
   E_CONFIG_LIMIT(e_config->remember_internal_windows, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_REMEMBER_INTERNAL_WINDOWS_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-remember-internal-windows-get", 0, "Get whether internal windows are remembered", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->remember_internal_windows, E_IPC_OP_REMEMBER_INTERNAL_WINDOWS_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_REMEMBER_INTERNAL_WINDOWS_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: %d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_LOGOUT
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-logout", 0, "Logout your user", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   E_Action  *act;
   act = e_action_find("logout");
   if (act && act->func.go)
      act->func.go(NULL, NULL);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_HIBERNATE
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-hibernate", 0, "Hibernate the computer", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   E_Action  *act;
   act = e_action_find("hibernate");
   if (act && act->func.go)
      act->func.go(NULL, NULL);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_REBOOT
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-reboot", 0, "Reboot the computer", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   E_Action  *act;
   act = e_action_find("reboot");
   if (act && act->func.go)
      act->func.go(NULL, NULL);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_SUSPEND
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-suspend", 0, "Suspend the computer", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   E_Action  *act;
   act = e_action_find("suspend");
   if (act && act->func.go)
      act->func.go(NULL, NULL);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_HALT
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-shutdown", 0, "Halt (shutdown) the computer", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   E_Action  *act;
   act = e_action_find("halt");
   if (act && act->func.go)
      act->func.go(NULL, NULL);
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_USE_CUSTOM_DESKLOCK_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desklock-use-custom-desklock-set", 1, "Set whether a custom desklock will be utilized", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_INT(atoi(params[0]), HDL);
#elif (TYPE == E_WM_IN)
   START_INT(policy, HDL);
   e_config->desklock_use_custom_desklock = policy;
   E_CONFIG_LIMIT(e_config->desklock_use_custom_desklock, 0, 1);
   SAVE;
   END_INT;
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_USE_CUSTOM_DESKLOCK_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desklock-use-custom-desklock-get", 0, "Get whether a custom desklock is being used", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_INT(e_config->desklock_use_custom_desklock, E_IPC_OP_DESKLOCK_USE_CUSTOM_DESKLOCK_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_USE_CUSTOM_DESKLOCK_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   START_INT(val, HDL);
   printf("REPLY: POLICY=%d\n", val);
   END_INT;
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_CUSTOM_DESKLOCK_CMD_SET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desklock-custom-desklock-cmd-set", 1, "Set the current custom desklock command to OPT1", 0, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HDL);
#elif (TYPE == E_WM_IN)
   STRING(s, HDL);
   if (e_config->desklock_custom_desklock_cmd)
	   eina_stringshare_del(e_config->desklock_custom_desklock_cmd);
   e_config->desklock_custom_desklock_cmd = eina_stringshare_add(s);   
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL

/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_CUSTOM_DESKLOCK_CMD_GET
#if (TYPE == E_REMOTE_OPTIONS)
   OP("-desklock-custom-desklock-cmd-get", 0, "Get the current custom desklock command", 1, HDL)
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HDL);
#elif (TYPE == E_WM_IN)
   SEND_STRING(e_config->desklock_custom_desklock_cmd, E_IPC_OP_DESKLOCK_CUSTOM_DESKLOCK_CMD_GET_REPLY, HDL);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HDL
     
/****************************************************************************/
#define HDL E_IPC_OP_DESKLOCK_CUSTOM_DESKLOCK_CMD_GET_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING(s, HDL);
   printf("REPLY: \"%s\"\n", s);
   END_STRING(s);
#endif
#undef HDL

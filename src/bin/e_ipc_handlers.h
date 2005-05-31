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
    for (l = e_module_list(); l; l = l->next) { \
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

#define SEND_STRING(__str, __op, HDL) \
case HDL: { void *data; int bytes; \
   data = e_ipc_codec_str_enc(__str, &bytes); \
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
   Evas_List *l, *ll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   E_FREE(e_config->desktop_default_background);
   e_config->desktop_default_background = strdup(s);
   for (l = e_manager_list(); l; l = l->next) {
      man = l->data;
      for (ll = man->containers; ll; ll = ll->next) {	
	 con = ll->data;
	 zone = e_zone_current_get(con);
	 e_zone_bg_reconfigure(zone);
      }
   }
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
   SEND_STRING(e_config->desktop_default_background, E_IPC_OP_MODULE_LIST_REPLY, HDL);
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
#elif (TYPE == E_WM_IN)
   GENERIC(HDL);
   e_font_apply();
   SAVE;
   END_GENERIC();
#elif (TYPE == E_REMOTE_IN)
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
 

   
   
   
   
#if 0
}
#endif

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

# define STRING(__str, HANDLER) \
case HANDLER: \
if (e->data) { \
   char *__str = NULL; \
   if (e_ipc_codec_str_dec(e->data, e->size, &__str)) {
# define END_STRING(__str) \
      free(__str); \
   } \
} \
break;
# define SAVE e_config_save_queue()
# define REQ_STRING(__str, HANDLER) \
case HANDLER: { void *data; int bytes; \
   data = e_ipc_codec_str_enc(__str, &bytes); \
   if (data) { \
      ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HANDLER, 0, 0, 0, data, bytes); \
      free(data); \
   } \
} \
break;
# define REQ_NULL(HANDLER) \
case HANDLER: \
   ecore_ipc_server_send(e->server, E_IPC_DOMAIN_REQUEST, HANDLER, 0, 0, 0, NULL, 0); \
break;
# define FREE_LIST(__list) \
while (__list) { \
   free(__list->data); \
   __list = evas_list_remove_list(__list, __list); \
}
# define SEND_DATA(__opcode) \
ecore_ipc_client_send(e->client, E_IPC_DOMAIN_REPLY, __opcode, 0, 0, 0, data, bytes)
# define STRING_INT_LIST(__v, __dec, __typ, HANDLER) \
 case HANDLER: { \
    Evas_List *dat = NULL, *l; \
    if (__dec(e->data, e->size, &dat)) { \
       for (l = dat; l; l = l->next) { \
	  __typ *__v; \
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

#define SEND_STRING_INT_LIST(__list, __typ1, __v1, __typ2, __v2, HANDLER) \
 case HANDLER: { \
    Evas_List *dat = NULL, *l; \
    void *data; int bytes; \
    for (l = e_module_list(); l; l = l->next) { \
       __typ1 *__v1; \
       __typ2 *__v2; \
       __v1 = l->data; \
       __v2 = calloc(1, sizeof(__typ2));
#define END_SEND_STRING_INT_LIST(__v1, __op) \
       dat = evas_list_append(dat, __v1); \
    } \
    data = e_ipc_codec_str_int_list_enc(dat, &bytes); \
    SEND_DATA(__op); \
    free(data); \
    FREE_LIST(dat); \
 } \
   break;

#endif











/*
 * ****************
 * IPC handlers
 * ****************
 */

/* what a handler looks like
 * 
 * E_REMOTE_OPTIONS
 *   (opt, num_params, description, num_expected_replies, HANDLER),
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
#define HANDLER E_IPC_OP_MODULE_LOAD
#if (TYPE == E_REMOTE_OPTIONS)
     {"-module-load", 1, "Loads the module named 'OPT1' into memory", 0, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HANDLER);
#elif (TYPE == E_WM_IN)
   STRING(s, HANDLER);
   if (!e_module_find(s)) {
      e_module_new(s);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HANDLER
     
/****************************************************************************/
#define HANDLER E_IPC_OP_MODULE_UNLOAD
#if (TYPE == E_REMOTE_OPTIONS)
   {"-module-unload", 1, "Unloads the module named 'OPT1' from memory", 0, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HANDLER);
#elif (TYPE == E_WM_IN)
   STRING(s, HANDLER);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_disable(m);
      e_object_del(E_OBJECT(m));
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HANDLER
   
/****************************************************************************/
#define HANDLER E_IPC_OP_MODULE_ENABLE
#if (TYPE == E_REMOTE_OPTIONS)
   {"-module-enable", 1, "Enable the module named 'OPT1'", 0, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HANDLER);
#elif (TYPE == E_WM_IN)
   STRING(s, HANDLER);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_enable(m);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HANDLER
     
/****************************************************************************/
#define HANDLER E_IPC_OP_MODULE_DISABLE
#if (TYPE == E_REMOTE_OPTIONS)
   {"-module-disable", 1, "Disable the module named 'OPT1'", 0, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HANDLER);
#elif (TYPE == E_WM_IN)
   STRING(s, HANDLER);
   E_Module *m;
   if ((m = e_module_find(s))) {
      e_module_disable(m);
      SAVE;
   }
   END_STRING(s);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HANDLER
     
/****************************************************************************/
#define HANDLER E_IPC_OP_MODULE_LIST
#if (TYPE == E_REMOTE_OPTIONS)
   {"-module-list", 0, "List all loaded modules", 1, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_NULL(HANDLER);
#elif (TYPE == E_WM_IN)
   SEND_STRING_INT_LIST(e_module_list(), E_Module, mod, E_Ipc_Str_Int, v, HANDLER);
   v->str = mod->name;
   v->val = mod->enabled;
   END_SEND_STRING_INT_LIST(v, E_IPC_OP_MODULE_LIST_REPLY);
#elif (TYPE == E_REMOTE_IN)
#endif
#undef HANDLER
     
/****************************************************************************/
#define HANDLER E_IPC_OP_MODULE_LIST_REPLY
#if (TYPE == E_REMOTE_OPTIONS)
#elif (TYPE == E_REMOTE_OUT)
#elif (TYPE == E_WM_IN)
#elif (TYPE == E_REMOTE_IN)
   STRING_INT_LIST(v, e_ipc_codec_str_int_list_dec, E_Ipc_Str_Int, HANDLER);
   printf("REPLY: \"%s\" ENABLED %i\n", v->str, v->val);
   END_STRING_INT_LIST(v);
#endif
#undef HANDLER
     
/****************************************************************************/
#define HANDLER E_IPC_OP_BG_SET
#if (TYPE == E_REMOTE_OPTIONS)
   {"-default-bg-set", 1, "Set the default background edje to the desktop background in the file 'OPT1' (must be a full path)", 0, HANDLER},
#elif (TYPE == E_REMOTE_OUT)
   REQ_STRING(params[0], HANDLER);
#elif (TYPE == E_WM_IN)
   STRING(s, HANDLER);
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
#undef HANDLER
     
   
   
   
   
#if 0
}
#endif

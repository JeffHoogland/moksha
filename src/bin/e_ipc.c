#include "e.h"

/* local subsystem functions */
static int _e_ipc_cb_client_add(void *data, int type, void *event);
static int _e_ipc_cb_client_del(void *data, int type, void *event);
static int _e_ipc_cb_client_data(void *data, int type, void *event);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;

/* externally accessible functions */
int
e_ipc_init(void)
{
   char buf[1024];
   char *disp;
   
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   if (!_e_ipc_server) return 0;
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _e_ipc_cb_client_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _e_ipc_cb_client_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _e_ipc_cb_client_data, NULL);

   e_ipc_codec_init();
   return 1;
}

void
e_ipc_shutdown(void)
{
   e_ipc_codec_shutdown();
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

/* local subsystem globals */
static int
_e_ipc_cb_client_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p connected to server!\n", e->client);
   return 1;
}

static int
_e_ipc_cb_client_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Del *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p disconnected from server!\n", e->client);
   /* delete client sruct */
   ecore_ipc_client_del(e->client);
   return 1;
}

static int
_e_ipc_cb_client_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   switch (e->minor)
     {
#define TYPE  E_WM_IN
#include      "e_ipc_handlers.h"
#undef TYPE	
/* here to steal from to port over to the new e_ipc_handlers.h */	
#if 0
      case E_IPC_OP_BINDING_MOUSE_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->mouse_bindings;
	     data = _e_ipc_mouse_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_MOUSE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_ADD:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
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
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_DEL:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
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
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->key_bindings;
	     data = _e_ipc_key_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_KEY_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_KEY_ADD:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
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
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_DEL:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
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
	       }
 	  }
	break;
#endif
      default:
	break;
     }
   printf("E-IPC: client sent: [%i] [%i] (%i) \"%p\"\n", e->major, e->minor, e->size, e->data);
   /* ecore_ipc_client_send(e->client, 1, 2, 7, 77, 0, "ABC", 4); */
   /* we can disconnect a client like this: */
   /* ecore_ipc_client_del(e->client); */
   /* or we can end a server by: */
   /* ecore_ipc_server_del(ecore_ipc_client_server_get(e->client)); */
   return 1;
}  

#if 0
static void
_e_ipc_reply_double_send(Ecore_Ipc_Client *client, double val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_double_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_int_send(Ecore_Ipc_Client *client, int val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_int_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_2int_send(Ecore_Ipc_Client *client, int val1, int val2, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_2int_enc(val1, val2, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}
#endif

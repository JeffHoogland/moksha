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
   return 1;
}

void
e_ipc_shutdown(void)
{
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

/* local subsystem globals */
static int
_e_ipc_cb_client_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p connected to server!\n", e->client);
   return 1;
}

static int
_e_ipc_cb_client_del(void *data, int type, void *event)
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
_e_ipc_cb_client_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   switch (e->minor)
     {
      case E_IPC_OP_MODULE_LOAD:
	if (e->data)
	  {
	     char *name;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if (!e_module_find(name))
	       {
		  e_module_new(name);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_UNLOAD:
	if (e->data)
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
		  e_object_del(E_OBJECT(m));
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_ENABLE:
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (!e_module_enabled_get(m))
		    e_module_enable(m);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_DISABLE:
	  {
	     char *name;
	     E_Module *m;
	     
	     name = malloc(e->size + 1);
	     name[e->size] = 0;
	     memcpy(name, e->data, e->size);
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
	       }
	     free(name);
	  }
	break;
      case E_IPC_OP_MODULE_LIST:
	  {
	     Evas_List *modules, *l;
	     int bytes;
	     E_Module *m;
	     char *data, *p;
		  
	     bytes = 0;
	     modules = e_module_list();
	     for (l = modules; l; l = l->next)
	       {
		  m = l->data;
		  bytes += strlen(m->name) + 1 + 1;
	       }
	     data = malloc(bytes);
	     p = data;
	     for (l = modules; l; l = l->next)
	       {
		  m = l->data;
		  strcpy(p, m->name);
		  p += strlen(m->name);
		  *p = 0;
		  p++;
		  *p = e_module_enabled_get(m);
		  p++;
	       }
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
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

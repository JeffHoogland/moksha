/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */


/*
 * TODO:
 * add ecore events for callbacks to all/some ipc calls, e.g. module_list
 *
 * add module_list, module_enabled_get and module_enabled_set
 *
 * augment IPC calls and add wrappers for them - i.e.:
 *   e restart/shutdown
 *   desktops add/remove/list etc
 *   windows shade[get/set]/maximise[get/set]/iconify[get/set]/list
 *
 * add ability to e to set theme, so we can have a theme_set call :)
 */

#include "E.h"
#include "e_private.h"
#include <Ecore.h>
#include <Ecore_Ipc.h>

static int  _e_ipc_init(const char *display);
static void _e_ipc_shutdown(void);
static int _e_cb_server_data(void *data, int type, void *event);

static Ecore_Ipc_Server *_e_ipc_server  = NULL;

int E_RESPONSE_MODULE_LIST = 0;
int E_RESPONSE_BACKGROUND_GET = 0;

int
e_init(const char* display)
{
   if (_e_ipc_server)
     return 0;

   /* basic ecore init */
   if (!ecore_init())
     {
	printf("ERROR: Enlightenment cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	return 0;
     }

   /* init ipc */
   if (!ecore_ipc_init())
     {
	printf("ERROR: Enlightenment cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	return 0;
     }

   /* setup e ipc service */
   if (!_e_ipc_init(display))
     {
	printf("ERROR: Enlightenment cannot set up the IPC socket.\n"
	       "Did you specify the right display?\n");
	return 0;
     }
   
   if (!E_RESPONSE_MODULE_LIST)
     {
	E_RESPONSE_MODULE_LIST = ecore_event_type_new();
	E_RESPONSE_BACKGROUND_GET = ecore_event_type_new();
     }
   
   return 1;
}

int
e_shutdown(void)
{
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();

   return 1;
}

void
e_module_enabled_set(const char *module, int enable)
{
   E_Ipc_Op type;

   if (!module)
     return;

   if (enable)
     type = E_IPC_OP_MODULE_ENABLE;
   else
     type = E_IPC_OP_MODULE_DISABLE;

   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST, type, 0/*ref*/,
			 0/*ref_to*/, 0/*response*/, (void *)module,
			 strlen(module));
}

void
e_module_load_set(const char *module, int load)
{
   E_Ipc_Op type;

   if (!module)
     return;

   if (load)
     type = E_IPC_OP_MODULE_LOAD;
   else
     type = E_IPC_OP_MODULE_UNLOAD;

   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST, type, 0/*ref*/,
			 0/*ref_to*/, 0/*response*/, (void *)module,
			 strlen(module));
}

void
e_module_list(void)
{
   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_MODULE_LIST, 0/*ref*/, 0/*ref_to*/,
			 0/*response*/, NULL, 0);
}

void
e_background_set(const char *bgfile)
{
   if (!bgfile)
     return;

   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST, E_IPC_OP_BG_SET,
			 0/*ref*/, 0/*ref_to*/, 0/*response*/, (void *)bgfile,
			 strlen(bgfile));
}

void
e_background_get(void)
{
   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_BG_GET, 0/*ref*/, 0/*ref_to*/,
			 0/*response*/, NULL, 0);
}

static int
_e_ipc_init(const char *display)
{
   char buf[1024];
   char *disp;
   
   disp = (char *)display;
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   /* FIXME: we shoudl also try the generic ":0" if the display is ":0.0" */
   /* similar... */
   if (!_e_ipc_server) return 0;
   
//   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_cb_server_add, NULL);
//   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_cb_server_data, NULL);
   
   return 1;
}

static void
_e_ipc_shutdown(void)
{
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

static int
_e_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;

   type = E_IPC_OP_MODULE_LIST;
   switch (e->minor)
     {  
	case E_IPC_OP_MODULE_LIST_REPLY:
	  if (e->data)
	    {
	       char *p;
             
	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
		    E_Response_Module_List *res;

		    res = calloc(1, sizeof(E_Response_Module_List));
		    res->name = p;
		    p += strlen(res->name);
		    if (p < (char *)(e->data + e->size))
		      {
			 p++;
			 if (p < (char *)(e->data + e->size))
			   {
			      res->enabled = *p;
			      p++;
			      ecore_event_add(E_RESPONSE_MODULE_LIST, res,
					      NULL, NULL);
			   }
		      }
		 }
	    }
          break;
	case E_IPC_OP_BG_GET_REPLY:
	    {
	       E_Response_Background_Get *res;

	       res = calloc(1, sizeof(E_Response_Background_Get));
	       res->data = e->data;
	       ecore_event_add(E_RESPONSE_BACKGROUND_GET, res, NULL, NULL);
	       break;
	    }
	default:
          break;
     }
   return 1;
}

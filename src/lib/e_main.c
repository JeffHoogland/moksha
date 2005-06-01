/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */


/*
 * TODO:
 * add ecore events for callbacks to all/some ipc calls, e.g. module_list
 *
 * add module_enabled_get
 *
 * augment IPC calls and add wrappers for them - i.e.:
 *   desktops add/remove/list/currentset etc
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

static void _e_cb_module_list_free(void *data, void *ev);
static void _e_cb_module_dir_list_free(void *data, void *ev);
static void _e_cb_bg_dir_list_free(void *data, void *ev);
static void _e_cb_theme_dir_list_free(void *data __UNUSED__, void *ev);

static Ecore_Ipc_Server *_e_ipc_server  = NULL;

int E_RESPONSE_MODULE_LIST = 0;
//int E_RESPONSE_MODULE_DIRS_LIST = 0;
int E_RESPONSE_BACKGROUND_GET = 0;
//int E_RESPONSE_BACKGROUND_DIRS_LIST = 0;
//int E_RESPONSE_THEME_DIRS_LIST = 0;

/*
 * initialise connection to the current E running on "display".
 * If parameter is null try to use DISPLAY env var.
 */
int
e_init(const char* display)
{
   char *disp, *pos;
   int free_disp;

   if (_e_ipc_server)
     return 0;

   free_disp = 0;
   if (display)
     disp = (char *) display;
   else
     disp = getenv("DISPLAY");
   
   if (!disp)
     fprintf(stderr, "ERROR: No display parameter passed to e_init, and no DISPLAY variable\n");

   pos = strrchr(disp, ':');
   if (!pos)
     {
	char *tmp;
	tmp = malloc(strlen(disp) + 5);
	snprintf(tmp, sizeof(tmp), "%s:0.0", disp);
	disp = tmp;
	free_disp = 1;
     }
   else
     {
	pos = strrchr(pos, '.');
	if (!pos)
	  {
	     char *tmp;
	     tmp = malloc(strlen(disp) + 3);
	     snprintf(tmp, strlen(tmp), "%s.0", disp);
	     disp = tmp;
	     free_disp = 1;
	  }
     }

   /* basic ecore init */
   if (!ecore_init())
     {
	fprintf(stderr, "ERROR: Enlightenment cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	return 0;
     }

   /* init ipc */
   if (!ecore_ipc_init())
     {
	fprintf(stderr, "ERROR: Enlightenment cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	return 0;
     }

   /* setup e ipc codecs */
   if (!e_ipc_codec_init())
     {       
	fprintf(stderr, "ERROR: Could not start enlightenment IPC codecs.\n");
	return 0;
     }

   /* setup e ipc service */
   if (!_e_ipc_init(disp))
     {
	fprintf(stderr, "ERROR: Enlightenment cannot set up the IPC socket.\n"
	       "Did you specify the right display?\n");
	return 0;
     }
   
   if (!E_RESPONSE_MODULE_LIST)
     {
	E_RESPONSE_MODULE_LIST = ecore_event_type_new();
//	E_RESPONSE_MODULE_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_BACKGROUND_GET = ecore_event_type_new();
//	E_RESPONSE_BACKGROUND_DIRS_LIST = ecore_event_type_new();
//	E_RESPONSE_THEME_DIRS_LIST = ecore_event_type_new();
     }
   
   if (free_disp)
     free(disp);
   return 1;
}

/* 
 * close our connection to E
 */
int
e_shutdown(void)
{
   e_ipc_codec_shutdown();
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();

   return 1;
}

/* actual IPC calls */
void
e_restart(void)
{
   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
                         E_IPC_OP_RESTART, 0/*ref*/, 0/*ref_to*/, 
			 0/*response*/, NULL, 0);
}
    
void
e_quit(void)
{
   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
                         E_IPC_OP_SHUTDOWN, 0/*ref*/, 0/*ref_to*/, 
			 0/*response*/, NULL, 0);
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
e_module_dirs_list(void)
{
//   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
//			 E_IPC_OP_MODULE_DIRS_LIST, 0/*ref*/, 0/*ref_to*/,
//			 0/*response*/, NULL, 0);
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

void
e_background_dirs_list(void)
{
//   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
//			 E_IPC_OP_BG_DIRS_LIST, 0/*ref*/, 
//			 0/*ref_to*/, 0/*response*/, NULL, 0);
}

void
e_theme_dirs_list(void)
{
//   ecore_ipc_server_send(_e_ipc_server, E_IPC_DOMAIN_REQUEST,
//			 E_IPC_OP_THEME_DIRS_LIST, 0/*ref*/, 
//			 0/*ref_to*/, 0/*response*/, NULL, 0);
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
   
/*   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_cb_server_add, NULL);*/
/*   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_cb_server_del, NULL);*/
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
_e_cb_server_data(void *data __UNUSED__, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;

   type = E_IPC_OP_MODULE_LIST;
   switch (e->minor)
     {  
	case E_IPC_OP_MODULE_LIST_REPLY:
	  if (e->data)
	    {
	       E_Response_Module_List *res;
	       int count = 0;
	       char *p;
             
	       res = calloc(1, sizeof(E_Response_Module_List));

	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
		    p += strlen(p) + 1 + 1;
		    count ++;
		 }
	       res->modules = malloc(sizeof(E_Response_Module_Data *) * count);
	       res->count = count;

	       count = 0;
	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
		    E_Response_Module_Data *md;
		    md = malloc(sizeof(E_Response_Module_Data));
		    md->name = p;
		    p += strlen(md->name);
		    if (p < (char *)(e->data + e->size))
		      {
			 p++;
			 if (p < (char *)(e->data + e->size))
			   {
			      md->enabled = *p;
			      p++;
			   }
		      }
		    res->modules[count] = md;
		    count ++;
		 }
			      ecore_event_add(E_RESPONSE_MODULE_LIST, res,
				_e_cb_module_list_free, NULL);
			   }
          break;
/*	
	case E_IPC_OP_MODULE_DIRS_LIST_REPLY:
	  if (e->data)
	    {
	       E_Response_Module_Dirs_List *res;
	       int count = 0;
	       char *p;

	       res = calloc(1, sizeof(E_Response_Module_Dirs_List));

	       p = e->data;
	       while (p < (char *)(e->data + e->size))
	         {
		    p += strlen(p) + 1;
		    count ++;
		      }

	       res->dirs = malloc(sizeof(char *) * count);
	       res->count = count;

	       count = 0;
	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
	            res->dirs[count] = p;
		    p += strlen(res->dirs[count]) + 1;
		    count++;
		 }
	       ecore_event_add(E_RESPONSE_MODULE_DIRS_LIST, res,
				_e_cb_module_dir_list_free, NULL);
	    }
          break;
 */
      case E_IPC_OP_BG_GET_REPLY:
	  {
	     E_Response_Background_Get *res;
	     char *str = NULL;
	     
	     res = calloc(1, sizeof(E_Response_Background_Get));
	     if (e->data)
	       {
		  e_ipc_codec_str_dec(e->data, e->size, &str);
		  res->file = str;
	       }
	     ecore_event_add(E_RESPONSE_BACKGROUND_GET, res, NULL, NULL);
	  }
	break;
/*	case E_IPC_OP_BG_DIRS_LIST_REPLY:
	  if (e->data)
	    {
	       E_Response_Background_Dirs_List *res;
	       char *p;
	       int count = 0;

	       res = calloc(1, sizeof(E_Response_Background_Dirs_List));

	       p = e->data;
	       while (p < (char *)(e->data + e->size))
	         {
		    p += strlen(p) + 1;
		    count ++;
		 }

	       res->dirs = malloc(sizeof(char *) * count);
	       res->count = count;

	       count = 0;
	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
	            res->dirs[count] = p;
		    p += strlen(res->dirs[count]) + 1;
		    count++;
		 }
	       ecore_event_add(E_RESPONSE_BACKGROUND_DIRS_LIST, res,
				_e_cb_bg_dir_list_free, NULL);
	    }
          break;
 */
/*	case E_IPC_OP_THEME_DIRS_LIST_REPLY:
	  if (e->data)
	    {
	       E_Response_Theme_Dirs_List *res;
	       char *p;
	       int count = 0;

	       res = calloc(1, sizeof(E_Response_Theme_Dirs_List));

	       p = e->data;
	       while (p < (char *)(e->data + e->size))
	         {
		    p += strlen(p) + 1;
		    count ++;
		 }

	       res->dirs = malloc(sizeof(char *) * count);
	       res->count = count;

	       count = 0;
	       p = e->data;
	       while (p < (char *)(e->data + e->size))
		 {
	            res->dirs[count] = p;
		    p += strlen(res->dirs[count]) + 1;
		    count++;
		 }
	       ecore_event_add(E_RESPONSE_THEME_DIRS_LIST, res,
				_e_cb_theme_dir_list_free, NULL);
	    }
          break;
 */
	default:
          break;
     }
   return 1;
}

static void _e_cb_module_list_free(void *data __UNUSED__, void *ev)
{
    E_Response_Module_List *e;
    int i;

    e = ev;
    for (i = 0; i < e->count; i++)
      {
         free(e->modules[i]);
      }
    free(e->modules);
    free(e);
}

static void
_e_cb_module_dir_list_free(void *data __UNUSED__, void *ev)
{
    E_Response_Module_Dirs_List *e;
    
    e = ev;
    free(e->dirs);
    free(e);
}

static void
_e_cb_bg_dir_list_free(void *data __UNUSED__, void *ev)
{
    E_Response_Background_Dirs_List *e;

    e = ev;
    free(e->dirs);
    free(e);
}

static void
_e_cb_theme_dir_list_free(void *data __UNUSED__, void *ev)
{
    E_Response_Theme_Dirs_List *e;

    e = ev;
    free(e->dirs);
    free(e);
}



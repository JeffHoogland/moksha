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

#include <Evas.h>
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>
#include "E_Lib.h"
#include "e.h"

#include "e_ipc_codec.c"

typedef struct _Opt Opt;

struct _Opt 
{
   char     *opt;
   int       num_param; 
   char     *desc;
   int       num_reply;
   E_Ipc_Op  opcode;
};

Opt opts[] = { 
#define TYPE  E_REMOTE_OPTIONS
#include      "e_ipc_handlers.h"
#undef TYPE 
};

static int  _e_ipc_init(const char *display);
static void _e_ipc_shutdown(void);
static E_Ipc_Op _e_ipc_call_find(const char *name);
static void _e_ipc_call(E_Ipc_Op opcode, char **params);
static int _e_cb_server_data(void *data, int type, void *event);

static void _e_cb_module_list_free(void *data, void *ev);
static void _e_cb_module_dir_list_free(void *data, void *ev);
static void _e_cb_bg_dir_list_free(void *data, void *ev);
static void _e_cb_theme_dir_list_free(void *data __UNUSED__, void *ev);

static Ecore_Ipc_Server *_e_ipc_server  = NULL;

int E_RESPONSE_MODULE_LIST = 0;
int E_RESPONSE_BACKGROUND_GET = 0;

int E_RESPONSE_DATA_DIRS_LIST = 0;
int E_RESPONSE_IMAGE_DIRS_LIST = 0;
int E_RESPONSE_FONT_DIRS_LIST = 0;
int E_RESPONSE_THEME_DIRS_LIST = 0;
int E_RESPONSE_INIT_DIRS_LIST = 0;
int E_RESPONSE_ICON_DIRS_LIST = 0;
int E_RESPONSE_MODULE_DIRS_LIST = 0;
int E_RESPONSE_BACKGROUND_DIRS_LIST = 0;


/*
 * initialise connection to the current E running on "display".
 * If parameter is null try to use DISPLAY env var.
 */
int
e_lib_init(const char* display)
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
     fprintf(stderr, "ERROR: No display parameter passed to e_lib_init, and no DISPLAY variable\n");

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

   /* setup e ipc service */
   if (!_e_ipc_init(disp))
     {
	fprintf(stderr, "ERROR: Enlightenment cannot set up the IPC socket.\n"
	       "Did you specify the right display?\n");
	return 0;
     }
   e_ipc_codec_init();
   
   if (!E_RESPONSE_MODULE_LIST)
     {
	E_RESPONSE_MODULE_LIST = ecore_event_type_new();
	E_RESPONSE_BACKGROUND_GET = ecore_event_type_new();

	E_RESPONSE_DATA_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_IMAGE_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_FONT_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_THEME_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_INIT_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_ICON_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_MODULE_DIRS_LIST = ecore_event_type_new();
	E_RESPONSE_BACKGROUND_DIRS_LIST = ecore_event_type_new();

     }
   
   if (free_disp)
     free(disp);
   return 1;
}

/* 
 * close our connection to E
 */
int
e_lib_shutdown(void)
{
   e_ipc_codec_shutdown();
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();

   return 1;
}

/* actual IPC calls */
void
e_lib_restart(void)
{
   _e_ipc_call(E_IPC_OP_RESTART, NULL);
}
    
void
e_lib_quit(void)
{
   _e_ipc_call(E_IPC_OP_SHUTDOWN, NULL);
}

void
e_lib_module_enabled_set(const char *module, int enable)
{
   char *tmp;

   if (!module)
     return;

   tmp = strdup(module);
   if (enable)
     _e_ipc_call(E_IPC_OP_MODULE_ENABLE, &tmp);
   else
     _e_ipc_call(E_IPC_OP_MODULE_DISABLE, &tmp);
   free(tmp);
}

void
e_lib_module_load_set(const char *module, int load)
{
   char *tmp;
   if (!module)
     return;

   tmp = strdup(module);
   if (load)
     _e_ipc_call(E_IPC_OP_MODULE_LOAD, &tmp);
   else
     _e_ipc_call(E_IPC_OP_MODULE_UNLOAD, &tmp);
   free(tmp);
}

void
e_lib_module_list(void)
{
   _e_ipc_call(E_IPC_OP_MODULE_LIST, NULL);
}

void
e_lib_background_set(const char *bgfile)
{
   char *tmp;
   if (!bgfile)
     return;

   tmp = strdup(bgfile);
   _e_ipc_call(E_IPC_OP_BG_SET, &tmp);
   free(tmp);
}

void
e_lib_background_get(void)
{
   _e_ipc_call(E_IPC_OP_BG_GET, NULL);
}

void
e_lib_data_dirs_list(void)
{
   char *type = "data";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_image_dirs_list(void)
{
   char *type = "images";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_font_dirs_list(void)
{
   char *type = "fonts";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_theme_dirs_list(void)
{
   char *type = "themes";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_init_dirs_list(void)
{
   char *type = "inits";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_icon_dirs_list(void)
{
   char *type = "icons";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_module_dirs_list(void)
{
   char *type = "modules";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
}

void
e_lib_background_dirs_list(void)
{
   char *type = "backgrounds";
   _e_ipc_call(E_IPC_OP_DIRS_LIST, &type);
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

static E_Ipc_Op
_e_ipc_call_find(const char *name)
{
   int i, j;

   for (j = 0; j < (int)(sizeof(opts) / sizeof(Opt)); j++)
     { 
        Opt *opt;
        
        opt = &(opts[j]);
        if (!strcmp(opt->opt, name))
	  return opt->opcode;
     }  
   return 0;
}

static void
_e_ipc_call(E_Ipc_Op opcode, char **params)
{
   Ecore_Ipc_Event_Server_Data *e = malloc(sizeof(Ecore_Ipc_Event_Server_Data));
   e->server = _e_ipc_server;
   
   switch(opcode)
     {

#define TYPE  E_REMOTE_OUT
#include      "e_ipc_handlers.h"
#undef TYPE

	default:
	  break;
     }
   free(e);
}

static int
_e_cb_server_data(void *data __UNUSED__, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;

   e = event;

   switch (e->minor)
     {

#define TYPE  E_LIB_IN
#include      "e_ipc_handlers.h"
#undef TYPE

	default:
	  break;
     }
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
_e_cb_dir_list_free(void *data __UNUSED__, void *ev)
{
    E_Response_Dirs_List *e;

    e = ev;
    free(e->dirs);
    free(e);
}



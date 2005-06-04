/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

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

static int _e_cb_signal_exit(void *data, int ev_type, void *ev);
static int _e_ipc_init(void);
static void _e_ipc_shutdown(void);
static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);
static void _e_help(void);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;
static const char *display_name = NULL;
static int reply_count = 0;
static int reply_expect = 0;

int
main(int argc, char **argv)
{
   int i;
   char *s, buf[1024];
   
   /* fix up DISPLAY to be :N.0 if no .screen is in it */
   s = getenv("DISPLAY");
   if (s)
     {
	char *p;
	
	p = strrchr(s, ':');
	if (!p)
	  {
	     snprintf(buf, sizeof(buf), "DISPLAY=%s:0.0", s);
	     putenv(strdup(buf));
	  }
	else
	  {
	     p = strrchr(p, '.');
	     if (!p)
	       {
		  snprintf(buf, sizeof(buf), "DISPLAY=%s.0", s);
		  putenv(strdup(buf));
	       }
	  }
     }
   
   /* handle some command-line parameters */
   display_name = (const char *)getenv("DISPLAY");
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-display")) && (i < (argc - 1)))
	  {
	     i++;
	     display_name = argv[i];
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--h")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     _e_help();
	     exit(0);
	  }
     }
  
   /* basic ecore init */
   if (!ecore_init())
     {
	printf("ERROR: Enlightenment_remote cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   ecore_app_args_set((int)argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_cb_signal_exit, NULL))
     {
	printf("ERROR: Enlightenment_remote cannot set up an exit signal handler.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   /* init ipc */
   if (!ecore_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   
   /* setup e ipc service */
   if (!_e_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot set up the IPC socket.\n"
	       "Maybe try the '-display :0.0' option?\n");
	exit(-1);
     }
   e_ipc_codec_init();

   /* start our main loop */
   ecore_main_loop_begin();

   e_ipc_codec_shutdown();
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();
   
   /* just return 0 to keep the compiler quiet */
   return 0;
}

/* local subsystem functions */
static int
_e_cb_signal_exit(void *data, int ev_type, void *ev)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_init(void)
{
   char buf[1024];
   char *disp;
   
   disp = (char *)display_name;
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   /* FIXME: we shoudl also try the generic ":0" if the display is ":0.0" */
   /* similar... */
   if (!_e_ipc_server) return 0;
   
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);
   
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
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;
   char **argv, **params;
   int i, j, argc, process_count = 0;
   
   e = event;
   ecore_app_args_get(&argc, &argv);
   for (i = 1; i < argc; i++)
     {
	for (j = 0; j < (int)(sizeof(opts) / sizeof(Opt)); j++)
	  {
	     Opt *opt;
	     
	     opt = &(opts[j]);
	     if (!strcmp(opt->opt, argv[i]))
	       {
		  if (i >= (argc - opt->num_param))
		    {
		       printf("ERROR: option %s expects %i parameters\n",
			      opt->opt, opt->num_param);
		       exit(-1);
		    }
		  else
		    {
		       params = &(argv[i + 1]);
		       switch (opt->opcode)
			 {
#define TYPE  E_REMOTE_OUT
#include      "e_ipc_handlers.h"
#undef TYPE
			  default:
			    break;
			 }
		       process_count++;
		       reply_expect += opt->num_reply;
		       i += opt->num_param;
		    }
	       }
	  }
     }
   if (process_count <= 0) _e_help();
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   return 1; 
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;
   printf("REPLY <- BEGIN\n");
   switch (e->minor)
     {
#define TYPE  E_REMOTE_IN
#include      "e_ipc_handlers.h"
#undef TYPE
      default:
	break;
     }
   printf("REPLY <- END\n");
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static void
_e_help(void)
{
   int i, j;
   
   printf("OPTIONS:\n");
   printf("  -h This help\n");
   printf("  -help This help\n");
   printf("  --help This help\n");
   printf("  --h This help\n");
   printf("  -display OPT1 Connect to E running on display 'OPT1'\n");
   for (j = 0; j < (int)(sizeof(opts) / sizeof(Opt)); j++)
     {
	Opt *opt;
	
	opt = &(opts[j]);
	printf("  %s ", opt->opt);
	for (i = 0; i < opt->num_param; i++)
	  printf("OPT%i ", i + 1);
	printf("%s\n", opt->desc);
     }
}


































#if 0
typedef struct _E_IPC_Opt_Handler E_IPC_Opt_Handler;

struct _E_IPC_Opt_Handler
{
   char *option;
   char *desc;
   int   num_params;
   int   replies;
   int   type;
   int   simple_request_id;
   void (*func) (char **params);
};

/* local subsystem functions */
static int  _e_cb_signal_exit(void *data, int ev_type, void *ev);
static int  _e_ipc_init(void);
static void _e_ipc_shutdown(void);

static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);

static void _e_help(void);

static const char *_e_ipc_context_str(int context);
static const char *_e_ipc_modifier_str(int mod);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;
static const char *display_name = NULL;
static int reply_count = 0;
static int reply_expect = 0;

static void
_e_opt_binding_mouse_parse(E_Config_Binding_Mouse *eb, char **params)
{
   if (!strcmp(params[0], "NONE")) eb->context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb->context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb->context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb->context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb->context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb->context = E_BINDING_CONTEXT_ANY;
   else
     {
	printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
	       "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
	exit(-1);
     }
   eb->button = atoi(params[1]);
   /* M1[|M2...] */
     {
	char *p, *pp;
	
	eb->modifiers = 0;
	pp = params[2];
	for (;;)
	  {
	     p = strchr(pp, '|');
	     if (p)
	       {
		  if (!strncmp(pp, "SHIFT|", 6)) eb->modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strncmp(pp, "CTRL|", 5)) eb->modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strncmp(pp, "ALT|", 4)) eb->modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strncmp(pp, "WIN|", 4)) eb->modifiers |= E_BINDING_MODIFIER_WIN;
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
		  if (!strcmp(pp, "SHIFT")) eb->modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strcmp(pp, "CTRL")) eb->modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strcmp(pp, "ALT")) eb->modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strcmp(pp, "WIN")) eb->modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (!strcmp(pp, "NONE")) eb->modifiers = E_BINDING_MODIFIER_NONE;
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
   eb->any_mod = atoi(params[3]);
   eb->action = params[4];
   eb->params = params[5];
}

static void
_e_opt_binding_mouse_add(char **params)
{
   E_Config_Binding_Mouse bind;
   int bytes;
   char *data;
   
   _e_opt_binding_mouse_parse(&bind, params);
   data = _e_ipc_mouse_binding_enc(&bind, &bytes);
   ecore_ipc_server_send(_e_ipc_server,
			 E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_BINDING_MOUSE_ADD,
			 0/*ref*/, 0/*ref_to*/, 0/*response*/,
			 data, bytes);
   free(data);
}

static void
_e_opt_binding_mouse_del(char **params)
{
   E_Config_Binding_Mouse bind;
   int bytes;
   char *data;
   
   _e_opt_binding_mouse_parse(&bind, params);
   data = _e_ipc_mouse_binding_enc(&bind, &bytes);
   ecore_ipc_server_send(_e_ipc_server,
			 E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_BINDING_MOUSE_DEL,
			 0/*ref*/, 0/*ref_to*/, 0/*response*/,
			 data, bytes);
   free(data);
}

static void
_e_opt_binding_key_parse(E_Config_Binding_Key *eb, char **params)
{
   if (!strcmp(params[0], "NONE")) eb->context = E_BINDING_CONTEXT_NONE;
   else if (!strcmp(params[0], "UNKNOWN")) eb->context = E_BINDING_CONTEXT_UNKNOWN;
   else if (!strcmp(params[0], "BORDER")) eb->context = E_BINDING_CONTEXT_BORDER;
   else if (!strcmp(params[0], "ZONE")) eb->context = E_BINDING_CONTEXT_ZONE;
   else if (!strcmp(params[0], "MANAGER")) eb->context = E_BINDING_CONTEXT_MANAGER;
   else if (!strcmp(params[0], "ANY")) eb->context = E_BINDING_CONTEXT_ANY;
   else
     {
	printf("OPT1 (CONTEXT) is not a valid context. Must be:\n"
	       "  NONE UNKNOWN BORDER ZONE MANAGER ANY\n");
	exit(-1);
     }
   eb->key = params[1];
   /* M1[|M2...] */
     {
	char *p, *pp;
	
	eb->modifiers = 0;
	pp = params[2];
	for (;;)
	  {
	     p = strchr(pp, '|');
	     if (p)
	       {
		  if (!strncmp(pp, "SHIFT|", 6)) eb->modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strncmp(pp, "CTRL|", 5)) eb->modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strncmp(pp, "ALT|", 4)) eb->modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strncmp(pp, "WIN|", 4)) eb->modifiers |= E_BINDING_MODIFIER_WIN;
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
		  if (!strcmp(pp, "SHIFT")) eb->modifiers |= E_BINDING_MODIFIER_SHIFT;
		  else if (!strcmp(pp, "CTRL")) eb->modifiers |= E_BINDING_MODIFIER_CTRL;
		  else if (!strcmp(pp, "ALT")) eb->modifiers |= E_BINDING_MODIFIER_ALT;
		  else if (!strcmp(pp, "WIN")) eb->modifiers |= E_BINDING_MODIFIER_WIN;
		  else if (!strcmp(pp, "NONE")) eb->modifiers = E_BINDING_MODIFIER_NONE;
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
   eb->any_mod = atoi(params[3]);
   eb->action = params[4];
   eb->params = params[5];
}

static void
_e_opt_binding_key_add(char **params)
{
   E_Config_Binding_Key bind;
   int bytes;
   char *data;
   
   _e_opt_binding_key_parse(&bind, params);
   data = _e_ipc_key_binding_enc(&bind, &bytes);
   ecore_ipc_server_send(_e_ipc_server,
			 E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_BINDING_KEY_ADD,
			 0/*ref*/, 0/*ref_to*/, 0/*response*/,
			 data, bytes);
   free(data);
}

static void
_e_opt_binding_key_del(char **params)
{
   E_Config_Binding_Key bind;
   int bytes;
   char *data;
   
   _e_opt_binding_key_parse(&bind, params);
   data = _e_ipc_key_binding_enc(&bind, &bytes);
   ecore_ipc_server_send(_e_ipc_server,
			 E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_BINDING_KEY_DEL,
			 0/*ref*/, 0/*ref_to*/, 0/*response*/,
			 data, bytes);
   free(data);
}

static void
_e_opt_focus_policy_set(char **params)
{
   int bytes;
   char *data;
   int value;
   
   value = 0;
   if (!strcmp(params[0], "MOUSE")) value = E_FOCUS_MOUSE;
   else if (!strcmp(params[0], "CLICK")) value = E_FOCUS_CLICK;
   else if (!strcmp(params[0], "SLOPPY")) value = E_FOCUS_SLOPPY;
   else
     {
	printf("focus must be MOUSE, CLICK or SLOPPY\n");
	exit(-1);
     }
   data = e_ipc_codec_int_enc(value, &bytes);
   ecore_ipc_server_send(_e_ipc_server,
			 E_IPC_DOMAIN_REQUEST,
			 E_IPC_OP_FOCUS_POLICY_SET,
			 0, 0, 0,
			 data, bytes);
   free(data);
}

#define SIMPLE_REQ      0
#define SIMPLE_STR_REQ  1
#define FULL_FUNC       2
#define MULTI_STR_REQ   3
#define SIMPLE_INT_REQ  4
#define SIMPLE_DBL_REQ  5
#define SIMPLE_2INT_REQ 6

#define OREQ(opt, desc, ipc, rep)       {opt, desc, 0, rep, SIMPLE_REQ, ipc, NULL}
#define OSTR(opt, desc, ipc, rep)       {opt, desc, 1, rep, SIMPLE_STR_REQ, ipc, NULL}
#define OFNC(opt, desc, param, fn, rep) {opt, desc, param, rep, FULL_FUNC, 0, fn}
#define OMUL(opt, desc, ipc, rep, argc) {opt, desc, argc, rep, MULTI_STR_REQ, ipc, NULL}
#define OINT(opt, desc, ipc, rep)       {opt, desc, 1, rep, SIMPLE_INT_REQ, ipc, NULL}
#define ODBL(opt, desc, ipc, rep)       {opt, desc, 1, rep, SIMPLE_DBL_REQ, ipc, NULL}
#define O2INT(opt, desc, ipc, rep)      {opt, desc, 2, rep, SIMPLE_2INT_REQ, ipc, NULL}

E_IPC_Opt_Handler handlers[] =
{
   OSTR("-font-fallback-remove", "Remove OPT1 from the fontset", E_IPC_OP_FONT_FALLBACK_REMOVE, 0),
   OSTR("-font-fallback-prepend", "Prepend OPT1 to the fontset", E_IPC_OP_FONT_FALLBACK_PREPEND, 0),
   OSTR("-font-fallback-append", "Append OPT1 to the fontset", E_IPC_OP_FONT_FALLBACK_APPEND, 0),
   OREQ("-font-fallback-list", "List the fallback fonts in order", E_IPC_OP_FONT_FALLBACK_LIST, 1),
   OSTR("-font-default-get", "List the default font associated with OPT1", E_IPC_OP_FONT_DEFAULT_GET, 1),
   OSTR("-font-default-remove", "Remove the default text class OPT1", E_IPC_OP_FONT_DEFAULT_REMOVE, 0),
   OREQ("-font-default-list", "List all configured text classes", E_IPC_OP_FONT_DEFAULT_LIST, 1),
   OMUL("-font-default-set", "Set textclass (OPT1) font (OPT2) and size (OPT3)", E_IPC_OP_FONT_DEFAULT_SET, 0, 3),
   OREQ("-lang-get", "Get the current language", E_IPC_OP_LANG_GET, 1),
   OREQ("-lang-list", "List all available languages", E_IPC_OP_LANG_LIST, 1),
   OSTR("-lang-set", "Set the current language", E_IPC_OP_LANG_SET, 0),
   OREQ("-binding-mouse-list", "List all mouse bindings", E_IPC_OP_BINDING_MOUSE_LIST, 1),
   OFNC("-binding-mouse-add", "Add an existing mouse binding. OPT1 = Context, OPT2 = button, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 6, _e_opt_binding_mouse_add, 0),
   OFNC("-binding-mouse-del", "Delete an existing mouse binding. OPT1 = Context, OPT2 = button, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 6, _e_opt_binding_mouse_del, 0),
   OREQ("-binding-key-list", "List all key bindings", E_IPC_OP_BINDING_KEY_LIST, 1),
   OFNC("-binding-key-add", "Add an existing key binding. OPT1 = Context, OPT2 = key, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 6, _e_opt_binding_key_add, 0),
   OFNC("-binding-key-del", "Delete an existing key binding. OPT1 = Context, OPT2 = key, OPT3 = modifiers, OPT4 = any modifier ok, OPT5 = action, OPT6 = action parameters", 6, _e_opt_binding_key_del, 0),
   OSTR("-module-dirs-append", "Append OPT1 to the user module path", E_IPC_OP_MODULE_DIRS_APPEND, 0),
   OSTR("-module-dirs-prepend", "Prepend OPT1 to the user module path", E_IPC_OP_MODULE_DIRS_PREPEND, 0),
   OSTR("-module-dirs-remove", "Remove OPT1 from the user module path", E_IPC_OP_MODULE_DIRS_REMOVE, 0),
   OSTR("-data-dirs-append", "Append OPT1 to the user data path", E_IPC_OP_DATA_DIRS_APPEND, 0),
   OSTR("-data-dirs-prepend", "Prepend OPT1 to the user data path", E_IPC_OP_DATA_DIRS_PREPEND, 0),
   OSTR("-data-dirs-remove", "Remove OPT1 from the user data path", E_IPC_OP_DATA_DIRS_REMOVE, 0),
   OSTR("-font-dirs-append", "Append OPT1 to the user font path", E_IPC_OP_FONT_DIRS_APPEND, 0),
   OSTR("-font-dirs-prepend", "Prepend OPT1 to the user font path", E_IPC_OP_FONT_DIRS_PREPEND, 0),
   OSTR("-font-dirs-remove", "Remove OPT1 from the user font path", E_IPC_OP_FONT_DIRS_REMOVE, 0),
   OSTR("-theme-dirs-append", "Append OPT1 to the user theme path", E_IPC_OP_THEME_DIRS_APPEND, 0),
   OSTR("-theme-dirs-prepend", "Prepend OPT1 to the user theme path", E_IPC_OP_THEME_DIRS_PREPEND, 0),
   OSTR("-theme-dirs-remove", "Remove OPT1 from the user theme path", E_IPC_OP_THEME_DIRS_REMOVE, 0),
   OSTR("-init-dirs-append", "Append OPT1 to the user init path", E_IPC_OP_INIT_DIRS_APPEND, 0),
   OSTR("-init-dirs-prepend", "Prepend OPT1 to the user init path", E_IPC_OP_INIT_DIRS_PREPEND, 0),
   OSTR("-init-dirs-remove", "Remove OPT1 from the user init path", E_IPC_OP_INIT_DIRS_REMOVE, 0),
   OSTR("-icon-dirs-append", "Append OPT1 to the user icon path", E_IPC_OP_ICON_DIRS_APPEND, 0),
   OSTR("-icon-dirs-prepend", "Prepend OPT1 to the user icon path", E_IPC_OP_ICON_DIRS_PREPEND, 0),
   OSTR("-icon-dirs-remove", "Remove OPT1 from the user icon path", E_IPC_OP_ICON_DIRS_REMOVE, 0),
   OSTR("-image-dirs-append", "Append OPT1 to the user image path", E_IPC_OP_IMAGE_DIRS_APPEND, 0),
   OSTR("-image-dirs-prepend", "Prepend OPT1 to the user image path", E_IPC_OP_IMAGE_DIRS_PREPEND, 0),
   OSTR("-image-dirs-remove", "Remove OPT1 from the user image path", E_IPC_OP_IMAGE_DIRS_REMOVE, 0),
   OSTR("-bg-dirs-append", "Append OPT1 to the user background path", E_IPC_OP_BG_DIRS_APPEND, 0),
   OSTR("-bg-dirs-prepend", "Prepend OPT1 to the user background path", E_IPC_OP_BG_DIRS_PREPEND, 0),
   OSTR("-bg-dirs-remove", "Remove OPT1 from the user background path", E_IPC_OP_BG_DIRS_REMOVE, 0),
   ODBL("-menus-fast-move-threshhold-set", "Set the mouse speed in pixels per second that is considered a 'fast move'", E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_SET, 0),
   OREQ("-menus-fast-move-threshhold-get", "Get the mouse speed (pixels/sec) that is considered a fast move", E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET, 1),
   ODBL("-menus-click-drag-timeout-set", "Set the time (in seconds) between a mouse press and release that will keep the menu up anyway", E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_SET, 0),
   OREQ("-menus-click-drag-timeout-get", "Get the above value", E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET, 1),
   OINT("-border-shade-animate-set", "Set the shading animation flag (0/1)", E_IPC_OP_BORDER_SHADE_ANIMATE_SET, 0),
   OREQ("-border-shade-animate-get", "Get the shading animation flag", E_IPC_OP_BORDER_SHADE_ANIMATE_GET, 1),
   OINT("-border-shade-transition-set", "Set the shading animation algorithm (0, 1, 2 or 3)", E_IPC_OP_BORDER_SHADE_TRANSITION_SET, 0),
   OREQ("-border-shade-transition-get", "Get the above value", E_IPC_OP_BORDER_SHADE_TRANSITION_GET, 1),
   ODBL("-border-shade-speed-set", "Set the shading speed (pixels/sec)", E_IPC_OP_BORDER_SHADE_SPEED_SET, 0),
   OREQ("-border-shade-speed-get", "Get the shading speed", E_IPC_OP_BORDER_SHADE_SPEED_GET, 1),
   OINT("-image-cache-set", "Set the speculative image cache size (Kb)", E_IPC_OP_IMAGE_CACHE_SET, 0),
   OREQ("-image-cache-get", "Get the image cache size", E_IPC_OP_IMAGE_CACHE_GET, 1),
   OINT("-font-cache-set", "Get the speculative font cache size (Kb)", E_IPC_OP_FONT_CACHE_SET, 0),
   OREQ("-font-cache-get", "Set the font cache size", E_IPC_OP_FONT_CACHE_GET, 1),
   OINT("-edge-flip-set", "Set the edge flip flag (0/1)", E_IPC_OP_USE_EDGE_FLIP_SET, 0),
   OREQ("-edge-flip-get", "Get the edge flip flag", E_IPC_OP_USE_EDGE_FLIP_GET, 1),
   ODBL("-edge-flip_timeout-set", "Set the edge flip timeout (sec)", E_IPC_OP_EDGE_FLIP_TIMEOUT_SET, 0),
   OREQ("-edge-flip_timeout-get", "Get the edge flip timeout", E_IPC_OP_EDGE_FLIP_TIMEOUT_GET, 1),
   O2INT("-desks-set", "Set the number of virtual desktops (X x Y. OPT1 = X, OPT2 = Y)", E_IPC_OP_DESKS_SET, 0),
   OREQ("-desks-get", "Get the number of virtual desktops", E_IPC_OP_DESKS_GET, 1),
};

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;
   char *s, buf[1024];
   
   /* fix up DISPLAY to be :N.0 if no .screen is in it */
   s = getenv("DISPLAY");
   if (s)
     {
	char *p;
	
	p = strrchr(s, ':');
	if (!p)
	  {
	     snprintf(buf, sizeof(buf), "DISPLAY=%s:0.0", s);
	     putenv(strdup(buf));
	  }
	else
	  {
	     p = strrchr(p, '.');
	     if (!p)
	       {
		  snprintf(buf, sizeof(buf), "DISPLAY=%s.0", s);
		  putenv(strdup(buf));
	       }
	  }
     }
   
   /* handle some command-line parameters */
   display_name = (const char *)getenv("DISPLAY");
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-display")) && (i < (argc - 1)))
	  {
	     i++;
	     display_name = argv[i];
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--h")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     _e_help();
	     exit(0);
	  }
     }
  
   /* basic ecore init */
   if (!ecore_init())
     {
	printf("ERROR: Enlightenment_remote cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   ecore_app_args_set((int)argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_cb_signal_exit, NULL))
     {
	printf("ERROR: Enlightenment_remote cannot set up an exit signal handler.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   /* init ipc */
   if (!ecore_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   
   /* setup e ipc service */
   if (!_e_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot set up the IPC socket.\n"
	       "Maybe try the '-display :0.0' option?\n");
	exit(-1);
     }
   e_ipc_codec_init();

   /* start our main loop */
   ecore_main_loop_begin();

   e_ipc_codec_shutdown();
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();
   
   /* just return 0 to keep the compiler quiet */
   return 0;
}

/* local subsystem functions */
static int
_e_cb_signal_exit(void *data, int ev_type, void *ev)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_init(void)
{
   char buf[1024];
   char *disp;
   
   disp = (char *)display_name;
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   /* FIXME: we shoudl also try the generic ":0" if the display is ":0.0" */
   /* similar... */
   if (!_e_ipc_server) return 0;
   
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);
   
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
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;
   int argc;
   char **argv;
   int i;
   int process_count = 0;
   
   e = event;
   ecore_app_args_get(&argc, &argv);
   for (i = 1; i < argc; i++)
     {
	char *v, *p;
	int j;
	int k;
	int data_size;
	
	for (j = 0; j < (int)(sizeof(handlers) / sizeof(E_IPC_Opt_Handler)); j++)
	  {
	     E_IPC_Opt_Handler *handler;
	     
	     handler = &handlers[j];
	     if (!strcmp(handler->option, argv[i]))
	       {
		  if (i >= (argc - handler->num_params))
		    {
		       printf("ERROR: option %s expects %i parameters\n",
			      handler->option, handler->num_params);
		       exit(-1);
		    }
		  else
		    {
		       switch (handler->type)
			 {
			  case SIMPLE_REQ:
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  NULL, 0);
			    break;
			  case SIMPLE_STR_REQ:
			    v = e_ipc_codec_str_enc(argv[i + 1], &data_size);
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  v, data_size);
			    free(v);
			    break;
			  case MULTI_STR_REQ:
			    /* pack up the data "<str>0<str>0" */
			    data_size = 0;
			    for (k = 0; k < handler->num_params; k++)
			      {
				 data_size += strlen(argv[ i + 1 + k ]);
				 data_size++; /* NULL Pad */
			      }			     
			    v = malloc(data_size);
			    p = v;	    			
			    for (k = 0; k < handler->num_params; k++)
			      {
				 strcpy(p, argv[ i + 1 + k]);
				 p += strlen(argv[ i + 1 + k]);
				 *p = 0;
				 p++;
			      }	
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  v, data_size);
			    free(v);
 
			    break;
			  case FULL_FUNC:
			    handler->func(argv + i + 1);
			    break;
			  case SIMPLE_INT_REQ:
			    v = e_ipc_codec_int_enc(atoi(argv[i + 1]),
						    &data_size);
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  v, data_size);
			    free(v);
			    break;
			  case SIMPLE_DBL_REQ:
			    v = e_ipc_codec_double_enc(atof(argv[i + 1]),
						       &data_size);
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  v, data_size);
			    free(v);
			    break;
			  case SIMPLE_2INT_REQ:
			    v = e_ipc_codec_2int_enc(atoi(argv[i + 1]),
						     atoi(argv[i + 2]),
						     &data_size);
			    ecore_ipc_server_send(_e_ipc_server,
						  E_IPC_DOMAIN_REQUEST,
						  handler->simple_request_id,
						  0/*ref*/, 0/*ref_to*/, 0/*response*/,
						  v, data_size);
			    free(v);
			    break;
			  default:
			    break;
			 }
		       process_count++;
		       reply_expect += handler->replies;
		       i += handler->num_params;
		       break;
		    }
	       }
	  }
     }
   if (process_count <= 0)
     _e_help();
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   return 1; 
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;
   /* FIXME: should make this function/callback based in a table like the */
   /* option handlers... */
   printf("REPLY: BEGIN\n");
   switch (e->minor)
     {  
      case E_IPC_OP_FONT_FALLBACK_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *fallbacks;
	     E_Font_Fallback *eff;
	     
	     fallbacks = _e_ipc_font_fallback_list_dec(e->data, e->size);
	     while (fallbacks)
	       {
		  eff = fallbacks->data;
		  printf("REPLY: FALLBACK NAME=\"%s\"\n", eff->name);
		  fallbacks = evas_list_remove_list(fallbacks, fallbacks);
		  E_FREE(eff);
	       }
	  }
	else
	  printf("REPLY: FALLBACK NONE\n");
	break;
      case E_IPC_OP_FONT_DEFAULT_GET_REPLY:
        if (e->data)
          {
             E_Font_Default efd;
             _e_ipc_font_default_dec(e->data, e->size, &efd);
             printf("REPLY: DEFAULT TEXT_CLASS=\"%s\" NAME=\"%s\" SIZE=%d\n",
		    efd.text_class, efd.font, efd.size);
          }
        else
          printf("REPLY: DEFAULT NONE\n"); 
        break;
      case E_IPC_OP_FONT_DEFAULT_LIST_REPLY:
        if (e->data)
          {
             Evas_List *defaults;
             E_Font_Default *efd;
	     
             defaults = _e_ipc_font_default_list_dec(e->data, e->size);
             while (defaults)
               {
		  efd = defaults->data;
		  printf("REPLY: DEFAULT TEXT_CLASS=\"%s\" NAME=\"%s\" SIZE=%d\n",
			 efd->text_class, efd->font, efd->size);  
                  defaults = evas_list_remove_list(defaults, defaults);
		  E_FREE(efd);
	       }
          }
        else
          printf("REPLY: DEFAULT NONE\n"); 
        break;	
      case E_IPC_OP_LANG_LIST_REPLY:
        if (e->data)
          {
	     Evas_List *langs;
	     
	     langs = _e_ipc_str_list_dec(e->data, e->size);
	     if (langs)
	       {
		  Evas_List *l;
		  
		  for (l = langs; l; l = l->next)
		    printf("REPLY: LANG=\"%s\"\n", (char *)l->data);
		  evas_list_free(langs);
	       }
          }
        else
          printf("REPLY: AVAILABLE NONE\n"); 
        break;   
      case E_IPC_OP_LANG_GET_REPLY:
	if (e->data)
	  {
	     printf("REPLY: %s\n", (char *)e->data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_LIST_REPLY:
        if (e->data)
          {
	     Evas_List *bindings;
	     E_Config_Binding_Mouse *eb;
	     
	     bindings = _e_ipc_mouse_binding_list_dec(e->data, e->size);
	     while (bindings)
	       {
		  eb = bindings->data;
		  printf("REPLY: BINDING CONTEXT=%s MODIFIERS=%s BUTTON=%i ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n", 
			 _e_ipc_context_str(eb->context),
			 _e_ipc_modifier_str(eb->modifiers),
			 eb->button,
			 eb->any_mod,
			 eb->action,
			 eb->params
			 );
                  bindings = evas_list_remove_list(bindings, bindings);
		  E_FREE(eb);
	       }
          }
        else
          printf("REPLY: AVAILABLE NONE\n"); 
        break;   
      case E_IPC_OP_BINDING_KEY_LIST_REPLY:
        if (e->data)
          {
	     Evas_List *bindings;
	     E_Config_Binding_Key *eb;
	     
	     bindings = _e_ipc_key_binding_list_dec(e->data, e->size);
	     while (bindings)
	       {
		  eb = bindings->data;
		  printf("REPLY: BINDING CONTEXT=%s MODIFIERS=%s KEY=\"%s\" ANY_MOD=%i ACTION=\"%s\" PARAMS=\"%s\"\n", 
			 _e_ipc_context_str(eb->context),
			 _e_ipc_modifier_str(eb->modifiers),
			 eb->key,
			 eb->any_mod,
			 eb->action,
			 eb->params
			 );
                  bindings = evas_list_remove_list(bindings, bindings);
		  E_FREE(eb);
	       }
          }
        else
          printf("REPLY: AVAILABLE NONE\n"); 
        break;   
      case E_IPC_OP_DATA_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: DATA DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_IMAGE_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: IMAGE DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_FONT_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: FONT DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_THEME_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: THEME DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_INIT_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: INIT DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_ICON_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: ICON DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_MODULE_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: MODULE DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_BG_DIRS_LIST_REPLY:
	if (e->data)
	  {
	     Evas_List *dirs;
	     E_Path_Dir *dir;

	     dirs = _e_ipc_path_list_dec(e->data, e->size);
	     while (dirs)
	       {
		  dir = dirs->data;
		  printf("REPLY: BACKGROUND DIR=\"%s\"\n", dir->dir);
		  dirs = evas_list_remove_list(dirs, dirs);
		  E_FREE(dir);
	       }
	  }
	break;
      case E_IPC_OP_MENUS_SCROLL_SPEED_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_ANIMATE_GET_REPLY:
	if (e->data)
	  {
	     int val;
	     
	     if (e_ipc_codec_int_dec(e->data, e->size, &val))
	       printf("REPLY: %i\n", val);
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_TRANSITION_GET_REPLY:
	if (e->data)
	  {
	     int val;
	     
	     if (e_ipc_codec_int_dec(e->data, e->size, &val))
	       printf("REPLY: %i\n", val);
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_SPEED_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_FRAMERATE_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_IMAGE_CACHE_GET_REPLY:
	if (e->data)
	  {
	     int val;
	     
	     if (e_ipc_codec_int_dec(e->data, e->size, &val))
	       printf("REPLY: %i\n", val);
	  }
	break;
      case E_IPC_OP_FONT_CACHE_GET_REPLY:
	if (e->data)
	  {
	     int val;
	     
	     if (e_ipc_codec_int_dec(e->data, e->size, &val))
	       printf("REPLY: %i\n", val);
	  }
	break;
      case E_IPC_OP_USE_EDGE_FLIP_GET_REPLY:
	if (e->data)
	  {
	     int val;
	     
	     if (e_ipc_codec_int_dec(e->data, e->size, &val))
	       printf("REPLY: %i\n", val);
	  }
	break;
      case E_IPC_OP_EDGE_FLIP_TIMEOUT_GET_REPLY:
	if (e->data)
	  {
	     double val;
	     
	     if (e_ipc_codec_double_dec(e->data, e->size, &val))
	       printf("REPLY: %3.3f\n", val);
	  }
	break;
      case E_IPC_OP_DESKS_GET_REPLY:
	if (e->data)
	  {
	     int val1;
	     int val2;
	     
	     if (e_ipc_codec_2int_dec(e->data, e->size, &val1, &val2))
	       printf("REPLY: %i %i\n", val1, val2);
	  }
	break;
      default:
	break;
     }
   printf("REPLY: END\n");
   reply_count++;
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static void
_e_help(void)
{
   int j, k, l;
   E_IPC_Opt_Handler *handler;
   char buf[128];
   int parsize = 0, opsize = 0;
   
   printf("OPTIONS:\n");
   for (j = 0; j < (int)(sizeof(handlers) / sizeof(E_IPC_Opt_Handler)); j++)
     {
	handler = &handlers[j];
	if ((int)strlen(handler->option) > parsize) parsize = strlen(handler->option);
	l = 0;
	for (k = 0; k < handler->num_params; k++)
	  {
	     snprintf(buf, sizeof(buf), " OPT%i", k + 1);
	     l += strlen(buf);
	  }
	if (l > opsize) opsize = l;
     }
   for (j = 0; j < (int)(sizeof(handlers) / sizeof(E_IPC_Opt_Handler)); j++)
     {
	handler = &handlers[j];
	printf("  %s", handler->option);
	l = parsize - strlen(handler->option);
	for (k = 0; k < l; k++) printf(" ");
	l = 0;
	for (k = 0; k < handler->num_params; k++)
	  {
	     snprintf(buf, sizeof(buf), " OPT%i", k + 1);
	     printf("%s", buf);
	     l += strlen(buf);
	  }
	while (l < opsize)
	  {
	     printf(" ");
	     l++;
	  }
	printf("  - %s\n", handler->desc);
     }
}

/* generic encoding functions */
static const char *
_e_ipc_context_str(int context)
{
   if (context == E_BINDING_CONTEXT_NONE) return "NONE";
   if (context == E_BINDING_CONTEXT_UNKNOWN) return "UNKNOWN";
   if (context == E_BINDING_CONTEXT_BORDER) return "BORDER";
   if (context == E_BINDING_CONTEXT_ZONE) return "ZONE";
   if (context == E_BINDING_CONTEXT_MANAGER) return "MANAGER";
   if (context == E_BINDING_CONTEXT_ANY) return "ANY";
   return "";
}

static char _mod_buf[256];
static const char *
_e_ipc_modifier_str(int mod)
{
   _mod_buf[0] = 0;
   if (mod & E_BINDING_MODIFIER_SHIFT)
     {
	if (_mod_buf[0] != 0) strcat(_mod_buf, "|");
	strcat(_mod_buf, "SHIFT");
     }
   if (mod & E_BINDING_MODIFIER_CTRL)
     {
	if (_mod_buf[0] != 0) strcat(_mod_buf, "|");
	strcat(_mod_buf, "CTRL");
     }
   if (mod & E_BINDING_MODIFIER_ALT)
     {
	if (_mod_buf[0] != 0) strcat(_mod_buf, "|");
	strcat(_mod_buf, "ALT");
     }
   if (mod & E_BINDING_MODIFIER_WIN)
     {
	if (_mod_buf[0] != 0) strcat(_mod_buf, "|");
	strcat(_mod_buf, "WIN");
     }
   if (mod == E_BINDING_MODIFIER_NONE)
     strcpy(_mod_buf, "NONE");
   return _mod_buf;
}
#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Ecore_Ipc.h>
#include <Ecore_File.h>
#include <Evas.h>
#include <Eet.h>
#include <Edje.h>

#define E_TYPEDEFS 1
#include "e_xinerama.h"
#undef E_TYPEDEFS
#include "e_xinerama.h"

EAPI int            e_init_init(void);
EAPI int            e_init_shutdown(void);
EAPI void           e_init_show(void);
EAPI void           e_init_hide(void);
EAPI void           e_init_title_set(const char *str);
EAPI void           e_init_version_set(const char *str);
EAPI void           e_init_status_set(const char *str);

EAPI void           e_init_done(void);

/* local subsystem functions */
static int _e_ipc_init(void);
static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server = NULL;
static const char *theme = NULL;
static int font_hinting = -1;
static int engine = -1;
static const char *title = NULL;
static const char *verstr = NULL;
static Evas_List *fpath = NULL;
static Ecore_X_Window *initwins = NULL;
static int initwins_num = 0;
static Ecore_Ipc_Server *server = NULL;

static int
delayed_ok(void *data)
{
   kill(getppid(), SIGUSR2);
   return 0;
}

int
main(int argc, char **argv)
{
   int i;
   char *s;
   double scale;
   
   for (i = 1; i < argc; i++)
     {
	if ((i == 1) && 
	    ((!strcmp(argv[i], "-h")) ||
	     (!strcmp(argv[i], "-help")) ||
	     (!strcmp(argv[i], "--help"))))
	  {
	     printf(
		    "This is an internal tool for Enlightenment.\n"
		    "do not use it.\n"
		    );
	     exit(0);
	  }
	else if (!theme) theme = argv[i];
	else if (engine < 0) engine = atoi(argv[i]);
	else if (font_hinting < 0) font_hinting = atoi(argv[i]);
	else if (!title) title = argv[i];
	else if (!verstr) verstr = argv[i];
	else fpath = evas_list_append(fpath, argv[i]);
     }
   
   ecore_init();
   ecore_x_init(NULL);
   ecore_app_args_set(argc, (const char **)argv);
   eet_init();
   evas_init();
   ecore_evas_init();
   edje_init();
   edje_frametime_set(1.0 / 30.0);
   s = getenv("E_SCALE");
   scale = 1.0;
   if (s) scale = atof(s);
   edje_scale_set(scale);
   ecore_file_init();
   ecore_ipc_init();

   if (_e_ipc_init())
     {
	e_init_init();
	e_init_show();
	e_init_title_set(title);
	e_init_version_set(verstr);
	e_init_status_set("");
	ecore_timer_add(0.2, delayed_ok, NULL);
	ecore_main_loop_begin();
     }
   
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
   
   ecore_ipc_shutdown();
   ecore_file_shutdown();
   ecore_evas_shutdown();
   edje_shutdown();
   evas_shutdown();
   eet_shutdown();
   ecore_x_shutdown();
   ecore_shutdown();
   
   return 0;
}

/* local subsystem functions */
static int
_e_ipc_init(void)
{
   char *sdir;
   
   sdir = getenv("E_IPC_SOCKET");
   if (!sdir)
     {
	printf("The E_IPC_SOCKET environment variable is not set. This is\n"
	       "exported by Enlightenment to all processes it launches.\n"
	       "This environment variable must be set and must point to\n"
	       "Enlightenment's IPC socket file (minus port number).\n");
	return 0;
     }
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, sdir, 0, NULL);
   if (!_e_ipc_server) return 0;
   
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);
   
   return 1;
}

static int
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;

   e = event;
   server = e->server;
   ecore_ipc_server_send(server,
			 7/*E_IPC_DOMAIN_INIT*/,
			 1/*hello*/,
			 0, 0, 0,
			 initwins, initwins_num * sizeof(Ecore_X_Window));
   ecore_ipc_server_flush(server);
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   /* quit now */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;
   if (e->major != 7/*E_IPC_DOMAIN_INIT*/) return 1;
   switch (e->minor)
     {
      case 1:
	if (e->data) e_init_status_set(e->data);
	break;
      case 2:
	/* quit now */
	e_init_done();
	break;
      default:
	break;
     }
   return 1;
}

static void _e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_init_cb_signal_enable(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source);
static int _e_init_cb_window_configure(void *data, int ev_type, void *ev);
static int _e_init_cb_timeout(void *data);
static Ecore_Evas *_e_init_evas_new(Ecore_X_Window root, int w, int h, Ecore_X_Window *winret);

/* local subsystem globals */
static Ecore_X_Window  _e_init_root_win = 0;
static Ecore_X_Window  _e_init_win = 0;
static Ecore_Evas     *_e_init_ecore_evas = NULL;
static Evas           *_e_init_evas = NULL;
static Evas_Object    *_e_init_object = NULL;
static Ecore_Event_Handler *_e_init_configure_handler = NULL;
static Ecore_Timer         *_e_init_timeout_timer = NULL;
  
/* externally accessible functions */
EAPI int
e_init_init(void)
{
   int w, h;
   Ecore_X_Window root;
   Ecore_X_Window *roots;
   int i, num;
   Evas_Object *o;
   Evas_List *l, *screens;
   const char *s;

   e_xinerama_init();
   
   _e_init_configure_handler = 
     ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, 
			     _e_init_cb_window_configure, NULL);
   
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
	return 0;
     }
   root = roots[0];
   _e_init_root_win = root;

   s = theme;
   initwins = malloc(num * 2 * sizeof(Ecore_X_Window));
   initwins_num = num * 2;
   
   /* extra root windows/screens */
   for (i = 1; i < num; i++)
     {
	ecore_x_window_size_get(roots[i], &w, &h);
	_e_init_ecore_evas = _e_init_evas_new(roots[i], w, h, 
					      &_e_init_win);
	_e_init_evas = ecore_evas_get(_e_init_ecore_evas);
	initwins[(i * 2) + 0] = roots[i];
	initwins[(i * 2) + 1] = _e_init_win;
	
	o = edje_object_add(_e_init_evas);
	edje_object_file_set(o, s, "e/init/extra_screen");
	evas_object_move(o, 0, 0);
	evas_object_resize(o, w, h);
	evas_object_show(o);
     }
   
   /* primary screen/root */
   ecore_x_window_size_get(root, &w, &h);
   _e_init_ecore_evas = _e_init_evas_new(root, w, h, &_e_init_win);
   _e_init_evas = ecore_evas_get(_e_init_ecore_evas);
   initwins[0] = root;
   initwins[1] = _e_init_win;
   
   /* look at xinerama asto how to slice this up */
   screens = (Evas_List *)e_xinerama_screens_get();
   if (screens)
     {
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;
	     
	     scr = l->data;
	     o = edje_object_add(_e_init_evas);
	     if (l == screens)
	       {
		  edje_object_file_set(o, s, "e/init/splash");
		  _e_init_object = o;
	       }
	     else
	       edje_object_file_set(o, s, "e/init/extra_screen");
	     evas_object_move(o, scr->x, scr->y);
	     evas_object_resize(o, scr->w, scr->h);
	     evas_object_show(o);
	  }
     }
   else
     {
	o = edje_object_add(_e_init_evas);
	edje_object_file_set(o, s, "e/init/splash");
	_e_init_object = o;
	evas_object_move(o, 0, 0);
	evas_object_resize(o, w, h);
	evas_object_show(o);
     }
   
   edje_object_part_text_set(_e_init_object, "e.text.disable_text", 
			     "Disable splash screen");
   edje_object_signal_callback_add(_e_init_object, "e,action,init,disable", "e",
				   _e_init_cb_signal_disable, NULL);
   edje_object_signal_callback_add(_e_init_object, "e,action,init,enable", "e",
				   _e_init_cb_signal_enable, NULL);
   edje_object_signal_callback_add(_e_init_object, "e,state,done_ok", "e",
				   _e_init_cb_signal_done_ok, NULL);
   free(roots);

   _e_init_timeout_timer = ecore_timer_add(240.0, _e_init_cb_timeout, NULL);
   return 1;
}

EAPI int
e_init_shutdown(void)
{
   ecore_event_handler_del(_e_init_configure_handler);
   _e_init_configure_handler = NULL;
   e_init_hide();
   return 1;
}

EAPI void
e_init_show(void)
{
   if (!_e_init_ecore_evas) return;
   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);
}

EAPI void
e_init_hide(void)
{
   if (!_e_init_ecore_evas) return;
   ecore_evas_hide(_e_init_ecore_evas);
   evas_object_del(_e_init_object);
   ecore_evas_free(_e_init_ecore_evas);
   _e_init_ecore_evas = NULL;
   _e_init_evas = NULL;
   _e_init_win = 0;
   _e_init_object = NULL;
}

EAPI void
e_init_title_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.title", str);
}

EAPI void
e_init_version_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.version", str);
}

EAPI void
e_init_status_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.status", str);
}

EAPI void
e_init_done(void)
{
   if (!_e_init_object) return;
   edje_object_signal_emit(_e_init_object, "e,state,done", "e");
   if (_e_init_timeout_timer) ecore_timer_del(_e_init_timeout_timer);
   _e_init_timeout_timer = ecore_timer_add(60.0, _e_init_cb_timeout, NULL);
}


static void
_e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   if (!server) return;
   ecore_ipc_server_send(server,
			 7/*E_IPC_DOMAIN_INIT*/,
			 2/*set splash*/,
			 0, 0, 0,
			 NULL, 0);
   ecore_ipc_server_flush(server);
}

static void
_e_init_cb_signal_enable(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   if (!server) return;
   ecore_ipc_server_send(server,
			 7/*E_IPC_DOMAIN_INIT*/,
			 2/*set splash*/,
			 1, 0, 0,
			 NULL, 0);
   ecore_ipc_server_flush(server);
}

static void
_e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_init_hide();
   if (_e_init_timeout_timer)
     {
	ecore_timer_del(_e_init_timeout_timer);
	_e_init_timeout_timer = NULL;
     }
   ecore_main_loop_quit();
}

static int
_e_init_cb_window_configure(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Configure *e;
   
   e = ev;
   /* really simple - don't handle xinerama - because this event will only
    * happen in single head */
   if (e->win != _e_init_root_win) return 1;
   ecore_evas_resize(_e_init_ecore_evas, e->w, e->h);
   evas_object_resize(_e_init_object, e->w, e->h);
   return 1;
}

static int
_e_init_cb_timeout(void *data)
{
   e_init_hide();
   _e_init_timeout_timer = NULL;
   ecore_main_loop_quit();
   return 0;
}

static Ecore_Evas *
_e_init_evas_new(Ecore_X_Window root, int w, int h, Ecore_X_Window *winret)
{
   Ecore_Evas *ee;
   Evas *e;
   Evas_List *l;
   
   if ((engine == 0) || (engine == 1))
     {
	ee = ecore_evas_software_x11_new(NULL, root, 0, 0, w, h);
	ecore_evas_override_set(ee, 1);
	ecore_evas_software_x11_direct_resize_set(ee, 1);
	*winret = ecore_evas_software_x11_window_get(ee);
     }
   else if (engine == 2)
     {
	ee = ecore_evas_gl_x11_new(NULL, root, 0, 0, w, h);
	ecore_evas_override_set(ee, 1);
	ecore_evas_gl_x11_direct_resize_set(ee, 1);
	*winret = ecore_evas_gl_x11_window_get(ee);
     }
   else if (engine == 3)
     {
	ee = ecore_evas_xrender_x11_new(NULL, root, 0, 0, w, h);
	ecore_evas_override_set(ee, 1);
	ecore_evas_xrender_x11_direct_resize_set(ee, 1);
	*winret = ecore_evas_xrender_x11_window_get(ee);
     }
   else if (engine == 4)
     {
        ee = ecore_evas_software_x11_16_new(NULL, root, 0, 0, w, h);
	ecore_evas_override_set(ee, 1);
	ecore_evas_software_x11_16_direct_resize_set(ee, 1);
	*winret = ecore_evas_software_x11_16_window_get(ee);
     }
   
   e = ecore_evas_get(ee);
   
   evas_image_cache_set(e, 4096 * 1024);
   evas_font_cache_set(e, 512 * 1024);
   
   for (l = fpath; l; l = l->next) evas_font_path_append(e, l->data);
   
   if (font_hinting == 0)
     {
	if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_BYTECODE))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_BYTECODE);
	else if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_AUTO))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_AUTO);
	else
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
     }
   else if (font_hinting == 1)
     {
	if (evas_font_hinting_can_hint(e, EVAS_FONT_HINTING_AUTO))
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_AUTO);
	else
	  evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
     }
   else if (font_hinting == 2)
     evas_font_hinting_set(e, EVAS_FONT_HINTING_NONE);
   
   ecore_evas_name_class_set(ee, "E", "Init_Window");
   ecore_evas_title_set(ee, "Enlightenment Init");

   ecore_evas_raise(ee);
   ecore_evas_show(ee);
   
   return ee;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_startup(void);
static int  _e_startup_timeout_cb(void *data);
static int  _e_startup_next_cb(void *data);
static void _e_startup_app_exit_cb(void *data, E_App *a, E_App_Change ch);

/* local subsystem globals */
static E_App       *startup_apps = NULL;
static int          start_app_pos = -1;
static Ecore_Timer *next_timer = NULL;
static Ecore_Timer *timeout_timer = NULL;
static E_App       *waiting_app = NULL;

/* externally accessible functions */
void
e_startup(E_Startup_Mode mode)
{
   char *homedir;
   char buf[PATH_MAX];
   
   homedir = e_user_homedir_get();
   if (mode == E_STARTUP_START)
     snprintf(buf, sizeof(buf), "%s/.e/e/applications/startup", homedir);
   else if (mode == E_STARTUP_RESTART)
     snprintf(buf, sizeof(buf), "%s/.e/e/applications/restart", homedir);
   free(homedir);
   startup_apps = e_app_new(buf, 1);
   if (!startup_apps)
     {
//	e_init_hide();
	return;
     }
   e_app_change_callback_add(_e_startup_app_exit_cb, NULL);
   start_app_pos = 0;
   _e_startup();
}

/* local subsystem functions */
static void
_e_startup(void)
{
   E_App *a;
   char buf[4096];
   
   if (!startup_apps)
     {
	e_init_done();
	return;
     }
   a = evas_list_nth(startup_apps->subapps, start_app_pos);
   start_app_pos++;
   if (!a)
     {
	e_object_unref(E_OBJECT(startup_apps));
	startup_apps = NULL;
	start_app_pos = -1;
	waiting_app = NULL;
	e_app_change_callback_del(_e_startup_app_exit_cb, NULL);
	e_init_done();
	return;
     }
   e_app_exec(a);
   snprintf(buf, sizeof(buf), "Starting %s", a->name);
   e_init_status_set((const char *)buf);   
   e_init_icons_app_add(a);
   if (a->wait_exit)
     {
	timeout_timer = ecore_timer_add(10.0, _e_startup_timeout_cb, NULL);
	waiting_app = a;
     }
   else
     {
	timeout_timer = ecore_timer_add(0.0, _e_startup_next_cb, NULL);
	waiting_app = NULL;
     }
}

static int
_e_startup_timeout_cb(void *data)
{
   timeout_timer = NULL;
   waiting_app = NULL;
   /* FIXME: error dialog or log etc..... */
   _e_startup();
   return 0;
}

static int
_e_startup_next_cb(void *data)
{
   next_timer = NULL;
   _e_startup();
   return 0;
}

static void
_e_startup_app_exit_cb(void *data, E_App *a, E_App_Change ch)
{
   if (ch == E_APP_EXIT)
     {
	if (a == waiting_app)
	  {
	     waiting_app = NULL;
	     if (timeout_timer)
	       {
		  ecore_timer_del(timeout_timer);
		  timeout_timer = NULL;
	       }
	     _e_startup();
	  }
     }
}

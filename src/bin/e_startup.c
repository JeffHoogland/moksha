/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO:
 * - Need some kind of "wait for exit" system, maybe register with
 *   e_config? startup and restart apps could also be in e_config
 */

/* local subsystem functions */
static void _e_startup(void);
static void _e_startup_next_cb(void *data);

/* local subsystem globals */
static E_Order        *startup_apps = NULL;
static int             start_app_pos = -1;

/* externally accessible functions */
EAPI void
e_startup(E_Startup_Mode mode)
{
   const char *homedir;
   char buf[PATH_MAX];
   
   homedir = e_user_homedir_get();
   if (mode == E_STARTUP_START)
     snprintf(buf, sizeof(buf), "%s/.e/e/applications/startup/.order", homedir);
   else if (mode == E_STARTUP_RESTART)
     snprintf(buf, sizeof(buf), "%s/.e/e/applications/restart/.order", homedir);
   startup_apps = e_order_new(buf);
   if (!startup_apps) return;
   start_app_pos = 0;
   e_init_undone();
   _e_startup();
}

/* local subsystem functions */
static void
_e_startup(void)
{
   Efreet_Desktop *desktop;
   char buf[4096];
   
   if (!startup_apps)
     {
	e_init_done();
	return;
     }
   desktop = evas_list_nth(startup_apps->desktops, start_app_pos);
   start_app_pos++;
   if (!desktop)
     {
	e_object_del(E_OBJECT(startup_apps));
	startup_apps = NULL;
	start_app_pos = -1;
	e_init_done();
	return;
     }
   e_exec(NULL, desktop, NULL, NULL, NULL);
   snprintf(buf, sizeof(buf), "%s %s", _("Starting"), desktop->name);
   e_init_status_set(buf);   
   ecore_job_add(_e_startup_next_cb, NULL);
}

static void
_e_startup_next_cb(void *data)
{
   _e_startup();
}

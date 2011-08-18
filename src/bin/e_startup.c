#include "e.h"

/* TODO:
 * - Need some kind of "wait for exit" system, maybe register with
 *   e_config? startup and restart apps could also be in e_config
 */

/* local subsystem functions */
static void _e_startup(void);
static void _e_startup_next_cb(void *data);
static Eina_Bool _e_startup_event_cb(void *data, int ev_type, void *ev);

/* local subsystem globals */
static E_Order *startup_apps = NULL;
static int start_app_pos = -1;
static Ecore_Event_Handler *desktop_cache_update_handler = NULL;

/* externally accessible functions */
EAPI void
e_startup(E_Startup_Mode mode)
{
   char buf[PATH_MAX];

   if (mode == E_STARTUP_START)
     {
	e_user_dir_concat_static(buf, "applications/startup/.order");
	if (!ecore_file_exists(buf))
	  e_prefix_data_concat_static(buf, "data/applications/startup/.order");
     }
   else if (mode == E_STARTUP_RESTART)
     {
	e_user_dir_concat_static(buf, "applications/restart/.order");
	if (!ecore_file_exists(buf))
	  e_prefix_data_concat_static(buf, "data/applications/restart/.order");
     }
   desktop_cache_update_handler =
      ecore_event_handler_add(EFREET_EVENT_DESKTOP_CACHE_BUILD,
                              _e_startup_event_cb,
                              strdup(buf));
   e_init_undone();
}

/* local subsystem functions */
static void
_e_startup(void)
{
   Efreet_Desktop *desktop;
   char buf[PATH_MAX];

   if (!startup_apps)
     {
	e_init_done();
	return;
     }
   desktop = eina_list_nth(startup_apps->desktops, start_app_pos);
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
_e_startup_next_cb(void *data __UNUSED__)
{
   _e_startup();
}

static Eina_Bool
_e_startup_event_cb(void *data, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   char *buf;

   ecore_event_handler_del(desktop_cache_update_handler);
   buf = data;
   startup_apps = e_order_new(buf);
   if (startup_apps)
      start_app_pos = 0;
   free(buf);
   _e_startup();
   return ECORE_CALLBACK_PASS_ON;
}

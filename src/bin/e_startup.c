#include "e.h"

/* TODO:
 * - Need some kind of "wait for exit" system, maybe register with
 *   e_config? startup and restart apps could also be in e_config
 */

/* local subsystem functions */
static void      _e_startup(void);
static void      _e_startup_next_cb(void *data);
static Eina_Bool _e_startup_event_cb(void *data, int ev_type, void *ev);
static Eina_Bool _e_startup_time_exceeded(void *data);

/* local subsystem globals */
static E_Order *startup_apps = NULL;
static int start_app_pos = -1;
static Ecore_Event_Handler *desktop_cache_update_handler = NULL;
static Ecore_Timer *timer = NULL;
static Eina_Bool desktop_cache_update = EINA_FALSE;
static E_Startup_Mode start_mode = E_STARTUP_START;
static Eina_Bool started = EINA_FALSE;

/* externally accessible functions */
EAPI void
e_startup_mode_set(E_Startup_Mode mode)
{
   char buf[PATH_MAX];

   start_mode = mode;
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
   if (timer) ecore_timer_del(timer);
   timer = ecore_timer_add(5.0, _e_startup_time_exceeded, NULL);
   e_init_undone();
}

EAPI void
e_startup(void)
{
   if (!desktop_cache_update)
     started = EINA_TRUE;
   else
     _e_startup();
}


/* local subsystem functions */
static Eina_Bool
_e_startup_delay(void *data)
{
   Efreet_Desktop *desktop = data;
   e_exec(NULL, desktop, NULL, NULL, NULL);
   efreet_desktop_unref(desktop);
   return EINA_FALSE;
}

// custom float parser for N.nnnn, or N,nnnn or N to avoid locale issues
static double
_atof(const char *s)
{
   const char *p;
   double v = 0, dec;

   for (p = s; isdigit(*p); p++)
     {
        v *= 10.0;
        v += (double)(*p - '0');
     }
   if ((*p == '.') || (*p == ','))
     {
        dec = 0.1;
        for (p++; isdigit(*p); p++)
          {
             v += ((double)(*p - '0')) * dec;
             dec /= 10.0;
          }
     }
   return v;
}

static void
_e_startup(void)
{
   Efreet_Desktop *desktop;
   char buf[1024];
   const char *s;
   double delay = 0.0;

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
   if (desktop->x)
     {
        s = eina_hash_find(desktop->x, "X-GNOME-Autostart-Delay");
        if (s)
          {
             const char *prev = setlocale(LC_NUMERIC, "C");
             delay = _atof(s);
             setlocale(LC_NUMERIC, prev);
          }
     }
   if (delay > 0.0)
     {
        efreet_desktop_ref(desktop);
        ecore_timer_add(delay, _e_startup_delay, desktop);
     }
   else e_exec(NULL, desktop, NULL, NULL, NULL);
   snprintf(buf, sizeof(buf), _("Starting %s"), desktop->name);
   e_init_status_set(buf);
   ecore_job_add(_e_startup_next_cb, NULL);
}

static void
_e_startup_next_cb(void *data __UNUSED__)
{
   _e_startup();
}

static void
_e_startup_error_dialog(const char *msg)
{
   E_Dialog *dia;

   dia = e_dialog_new(NULL, "E", "_startup_error_dialog");
   EINA_SAFETY_ON_NULL_RETURN(dia);

   e_dialog_title_set(dia, "ERROR!");
   e_dialog_icon_set(dia, "enlightenment", 64);
   e_dialog_text_set(dia, msg);
   e_dialog_button_add(dia, _("Close"), NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_win_no_remember_set(dia->win, 1);
   e_dialog_show(dia);
}

static Eina_Bool
_e_startup_event_cb(void *data, int ev_type __UNUSED__, void *ev)
{
   char *buf;

   if (timer) ecore_timer_del(timer);
   timer = NULL;
   Efreet_Event_Cache_Update *e;

   e = ev;
   if ((e) && (e->error))
   {
      fprintf(stderr, "Moksha: efreet didn't notify about cache update\n");
      _e_startup_error_dialog("Moksha: Efreet did not update cache. "
                           "Please check your Efreet setup");
   }

   desktop_cache_update = EINA_TRUE;
   ecore_event_handler_del(desktop_cache_update_handler);
   buf = data;
   startup_apps = e_order_new(buf);
   if (startup_apps)
     start_app_pos = 0;
   free(buf);
   _e_startup();
   if (start_mode == E_STARTUP_START)
   {
     char shfile[PATH_MAX];
     e_user_dir_concat_static(shfile, "applications/startup/startupcommands");
     if (!ecore_file_exists(shfile))
     {
         FILE *f;
         f = fopen(shfile, "w");
         if (f) fclose(f);
     }
     if (chmod(shfile, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
        fprintf(stderr, "Moksha: chmod failure\n");
     // Let this fail when chmod fails so user is aware of problem
     e_exec(NULL, NULL, shfile, NULL, NULL);
   }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_startup_time_exceeded(void *data EINA_UNUSED)
{
   fprintf(stderr, "Moksha: efreet didn't notify about cache update\n");
   _e_startup_error_dialog("Moksha: Efreet did not update cache. "
                           "Please check your Efreet setup");
   timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

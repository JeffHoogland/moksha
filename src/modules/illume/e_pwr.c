#include "e.h"
#include "e_pwr.h"
#include "e_cfg.h"

/* internal calls */
static void _system_req_state(const char *state);
static void _system_unreq_state(void);
static int _cb_saver(void *data, int ev_type, void *ev);
    
/* state */
static Ecore_Event_Handler *save_handler = NULL;
static Ecore_Timer *suspend_timer = NULL;
static Ecore_X_Window coverwin = 0;
static int suspended = 0;
static int init_going = 0;

static E_DBus_Connection *conn = NULL;

/* called from the module core */
EAPI int
e_pwr_init(void)
{
   save_handler = ecore_event_handler_add(ECORE_X_EVENT_SCREENSAVER_NOTIFY,
					  _cb_saver, NULL);
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   _system_req_state("on");
   e_pwr_cfg_update();
   init_going = 1;
   return 1;
}

EAPI int
e_pwr_shutdown(void)
{
   ecore_event_handler_del(save_handler);
   save_handler = NULL;
   return 1;
}

EAPI void
e_pwr_cfg_update(void)
{
   if (suspend_timer)
     {
	ecore_timer_del(suspend_timer);
	suspend_timer = NULL;
     }
   e_screensaver_init();
}

EAPI void
e_pwr_init_done(void)
{
   if (!init_going) return;
   
   ecore_x_test_fake_key_down("Shift_L"); // fake a key to keep screensaver up
   ecore_x_test_fake_key_up("Shift_L"); // fake a key to keep screensaver up
   _system_unreq_state();
   init_going = 0;
}

///////////////////////////////////////////////////////////////////////////////

static void
_system_req_state(const char *state)
{
   DBusMessage *msg;
   
   if (!conn) printf("@@ NO SYSTEM DBUS FOR OMPOWER\n");
   else
     {
	msg = dbus_message_new_method_call("org.openmoko.Power",
					   "/",
					   "org.openmoko.Power.Core",
					   "RequestResourceState");
	if (msg)
	  {
	     DBusMessageIter iter;
	     const char *str;
	     
	     dbus_message_iter_init_append(msg, &iter);
	     str = "cpu";
	     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);
	     str = "illume";
	     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);
	     str = state;
	     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);
	     e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
	     dbus_message_unref(msg);
	  }
     }
}

static void
_system_unreq_state(void)
{
   DBusMessage *msg;
   
   if (!conn) printf("@@ NO SYSTEM DBUS FOR OMPOWER\n");
   else
     {
	msg = dbus_message_new_method_call("org.openmoko.Power",
					   "/",
					   "org.openmoko.Power.Core",
					   "RemoveRequestedResourceState");
	if (msg)
	  {
	     DBusMessageIter iter;
	     const char *str;
	     
	     dbus_message_iter_init_append(msg, &iter);
	     str = "cpu";
	     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);
	     str = "illume";
	     dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);
	     e_dbus_method_call_send(conn, msg, NULL, NULL, NULL, -1, NULL);
	     dbus_message_unref(msg);
	  }
     }
}

/* internal calls */
static int
_cb_suspend(void *data)
{
   suspended = 1;
   
   /* FIXME: should be config */
   if (illume_cfg->power.auto_suspend)
     {
	printf("@@ SUSPEND NOW!\n");
	_system_req_state("off");
//	ecore_exe_run("apm -s", NULL);
     }
   suspend_timer = NULL;
   return 0;
}

static int
_cb_saver(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Screensaver_Notify *event;
   
   event = ev;
   if (event->on)
     {
	if (init_going)
	  {
	     ecore_x_test_fake_key_down("Shift_L"); // fake a key to keep screensaver up
	     ecore_x_test_fake_key_up("Shift_L"); // fake a key to keep screensaver up
	     return 1;
	  }
	if (!coverwin)
	  {
	     E_Zone *zone;
	     
	     zone = e_util_container_zone_number_get(0, 0);
	     if (zone)
	       {
		  coverwin = ecore_x_window_input_new(zone->container->win,
						      zone->x, zone->y,
						      zone->w, zone->h);
		  ecore_x_window_show(coverwin);
	       }
	  }
	if (suspend_timer)
	  {
	     ecore_timer_del(suspend_timer);
	     suspend_timer = NULL;
	  }
	if (illume_cfg->power.auto_suspend)
	  {
	     suspend_timer = ecore_timer_add(illume_cfg->power.auto_suspend_delay, _cb_suspend, NULL);
	  }
     }
   else
     {
	_system_unreq_state();
	if (coverwin)
	  {
	     ecore_x_window_free(coverwin);
	     coverwin = 0;
	  }
	if (suspend_timer)
	  {
	     ecore_timer_del(suspend_timer);
	     suspend_timer = NULL;
	  }
 	if (suspended)
	  {
	     printf("@@ UNSUSPEND\n");
	     suspended = 0;
	  }
     }
   return 1;
}

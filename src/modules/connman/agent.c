#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "agent.h"
#include "E_Connman.h"

static unsigned int init_count;
static E_DBus_Connection *conn;
static E_DBus_Object *agent_obj;

#define AGENT_PATH "/org/enlightenment/connman/agent"
#define AGENT_IFACE "net.connman.Agent"

static DBusMessage *_agent_release(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static DBusMessage *_agent_report_error(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static DBusMessage *_agent_request_browser(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static DBusMessage *_agent_request_input(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static DBusMessage *_agent_cancel(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static void _econnman_agent_object_create(void)
{
   E_DBus_Interface *iface = e_dbus_interface_new(AGENT_IFACE);

   e_dbus_interface_method_add(iface, "Release", "", "", _agent_release);
   e_dbus_interface_method_add(iface, "ReportError", "os", "",
                               _agent_report_error);
   e_dbus_interface_method_add(iface, "RequestBrowser", "os", "",
                               _agent_request_browser);
   e_dbus_interface_method_add(iface, "RequestInput", "oa{sv}", "a{sv}",
                               _agent_request_input);
   e_dbus_interface_method_add(iface, "Cancel", "", "", _agent_cancel);

   agent_obj = e_dbus_object_add(conn, AGENT_PATH, NULL);
   e_dbus_object_interface_attach(agent_obj, iface);

   e_dbus_interface_unref(iface);
}

unsigned int econnman_agent_init(E_DBus_Connection *edbus_conn)
{
   init_count++;

   if (init_count > 1)
      return init_count;

   conn = edbus_conn;
   _econnman_agent_object_create();

   return init_count;
}

unsigned int econnman_agent_shutdown(void)
{
   if (init_count == 0)
     {
        ERR("connman agent already shut down.");
        return 0;
     }

   init_count--;
   if (init_count > 0)
      return init_count;

   e_dbus_object_free(agent_obj);
   conn = NULL;

   return 0;
}

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_dbus.h"

static const char E_FILEMAN_BUS_NAME[] = "org.enlightenment.FileManager";
static const char E_FILEMAN_INTERFACE[] = "org.enlightenment.FileManager";
static const char E_FILEMAN_ERROR[] = "org.enlightenment.FileManager.Error";
static const char E_FILEMAN_PATH[] = "/org/enlightenment/FileManager";

typedef struct _E_Fileman_DBus_Daemon E_Fileman_DBus_Daemon;
struct _E_Fileman_DBus_Daemon
{
   E_DBus_Connection *conn;
   E_DBus_Interface *iface;
   E_DBus_Object *obj;

   struct {
      DBusPendingCall *request_name;
   } pending;
};

static DBusMessage *
_e_fileman_dbus_daemon_error(DBusMessage *message, const char *msg)
{
   return dbus_message_new_error(message, E_FILEMAN_ERROR, msg);
}

static void
_e_fileman_dbus_daemon_object_init(E_Fileman_DBus_Daemon *d)
{
   if (d->obj)
     return;

   d->obj = e_dbus_object_add(d->conn, E_FILEMAN_PATH, d);
   if (!d->obj)
     {
	fprintf(stderr, "ERROR: cannot add object to %s\n", E_FILEMAN_PATH);
	return;
     }

   e_dbus_object_interface_attach(d->obj, d->iface);
}

static void
_e_fileman_dbus_daemon_free(E_Fileman_DBus_Daemon *d)
{
   if (d->pending.request_name)
     dbus_pending_call_cancel(d->pending.request_name);

   if (d->obj)
     {
	e_dbus_object_interface_detach(d->obj, d->iface);
	e_dbus_object_free(d->obj);
     }

   if (d->iface)
     e_dbus_interface_unref(d->iface);

   if (d->conn)
     e_dbus_connection_close(d->conn);

   free(d);
}

DBusMessage *
_e_fileman_dbus_daemon_open_directory_cb(E_DBus_Object *obj, DBusMessage *message)
{
   DBusMessageIter itr;
   const char *directory = NULL, *p;
   char *dev;
   E_Zone *zone;

   dbus_message_iter_init(message, &itr);
   dbus_message_iter_get_basic(&itr, &directory);

   if ((!directory) || (directory[0] == '\0'))
     return _e_fileman_dbus_daemon_error(message, "no directory provided.");

   if (strncmp(directory, "file://", sizeof("file://") - 1) == 0)
     directory += sizeof("file://") - 1;

   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone)
     return _e_fileman_dbus_daemon_error(message, "could not find a zone.");


   p = strchr(directory, '/');
   if (p)
     {
	int len = p - directory + 1;

	dev = malloc(len + 1);
	if (!dev)
	  return _e_fileman_dbus_daemon_error
	    (message, "could not allocate memory.");

	memcpy(dev, directory, len);
	dev[len] = '\0';

	if ((dev[0] != '/') && (dev[0] != '~'))
	  dev[len - 1] = '\0'; /* remove trailing '/' */

	directory += p - directory;
     }
   else
     {
	dev = strdup(directory);
	directory = "/";
     }

   e_fwin_new(zone->container, dev, directory);
   free(dev);
   return dbus_message_new_method_return(message);
}

static void
_e_fileman_dbus_daemon_request_name_cb(void *data, DBusMessage *msg, DBusError *err)
{
   E_Fileman_DBus_Daemon *d = data;
   dbus_uint32_t ret;
   DBusError new_err;

   d->pending.request_name = NULL;

   if (dbus_error_is_set(err))
     {
	fprintf(stderr, "ERROR: FILEMAN: RequestName failed: %s\n",
		err->message);
	dbus_error_free(err);
	return;
     }

   dbus_error_init(&new_err);
   dbus_message_get_args(msg, &new_err, DBUS_TYPE_UINT32, &ret,
			 DBUS_TYPE_INVALID);

   if (dbus_error_is_set(&new_err))
     {
	fprintf(stderr,
		"ERROR: FILEMAN: could not get arguments of RequestName: %s\n",
		new_err.message);
	dbus_error_free(&new_err);
	return;
     }

   switch (ret)
     {
      case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
      case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
	 _e_fileman_dbus_daemon_object_init(d);
	 break;
      case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
	 //XXX mark daemon as queued?
	 break;
      case DBUS_REQUEST_NAME_REPLY_EXISTS:
	 //XXX exit?
	 break;
     }
}

static E_Fileman_DBus_Daemon *
_e_fileman_dbus_daemon_new(void)
{
   E_Fileman_DBus_Daemon *d;

   d = calloc(1, sizeof(*d));
   if (!d)
     {
	perror("ERROR: FILEMAN: cannot allocate fileman dbus daemon memory.");
	return NULL;
     }

   d->conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if (!d->conn)
     goto error;

   d->iface = e_dbus_interface_new(E_FILEMAN_INTERFACE);
   if (!d->iface)
     goto error;

   d->pending.request_name = e_dbus_request_name
     (d->conn, E_FILEMAN_BUS_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING,
      _e_fileman_dbus_daemon_request_name_cb, d);
   if (!d->pending.request_name)
     goto error;

   const struct {
      const char *method;
      const char *signature;
      const char *ret_signature;
      E_DBus_Method_Cb func;
   } *itr, desc[] = {
     {"OpenDirectory", "s", "", _e_fileman_dbus_daemon_open_directory_cb},
     {NULL}
   };

   for (itr = desc; itr->method != NULL; itr++)
     e_dbus_interface_method_add
       (d->iface, itr->method, itr->signature, itr->ret_signature, itr->func);

   return d;

 error:
   fprintf(stderr, "ERROR: FILEMAN: failed to create daemon at %p\n", d);
   _e_fileman_dbus_daemon_free(d);
   return NULL;
}


static E_Fileman_DBus_Daemon *_daemon = NULL;

void
e_fileman_dbus_init(void)
{
   if (_daemon)
     return;

   e_dbus_init();
   _daemon = _e_fileman_dbus_daemon_new();
}

void
e_fileman_dbus_shutdown(void)
{
   if (!_daemon)
     return;

   _e_fileman_dbus_daemon_free(_daemon);
   _daemon = NULL;
   e_dbus_shutdown();
}

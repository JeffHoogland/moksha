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
   E_DBus_Interface  *iface;
   E_DBus_Object     *obj;

   struct
   {
      DBusPendingCall *request_name;
   } pending;
};

static DBusMessage *
_e_fileman_dbus_daemon_error(DBusMessage *message,
                             const char  *msg)
{
   return dbus_message_new_error(message, E_FILEMAN_ERROR, msg);
}

static void
_e_fileman_dbus_daemon_object_init(E_Fileman_DBus_Daemon *d)
{
   if (d->obj) return;

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

static DBusMessage *
_e_fileman_dbus_daemon_open_directory_cb(E_DBus_Object *obj __UNUSED__,
                                         DBusMessage       *message)
{
   DBusMessageIter itr;
   const char *directory = NULL, *p;
   char *dev, *to_free = NULL;
   E_Zone *zone;

   dbus_message_iter_init(message, &itr);
   dbus_message_iter_get_basic(&itr, &directory);

   if ((!directory) || (directory[0] == '\0'))
     return _e_fileman_dbus_daemon_error(message, "no directory provided.");

   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone)
     return _e_fileman_dbus_daemon_error(message, "could not find a zone.");

   if (strstr(directory, "://"))
     {
        Efreet_Uri *uri = efreet_uri_decode(directory);

        directory = NULL;
        if (uri)
          {
             if ((uri->protocol) && (strcmp(uri->protocol, "file") == 0))
               directory = to_free = strdup(uri->path);
             efreet_uri_free(uri);
          }

        if (!directory)
          return _e_fileman_dbus_daemon_error(message, "unsupported protocol");
     }

   p = strchr(directory, '/');
   if (p)
     {
        int len = p - directory + 1;

        dev = malloc(len + 1);
        if (!dev)
          {
             free(to_free);
             return _e_fileman_dbus_daemon_error(message,
                                                 "could not allocate memory.");
          }

        memcpy(dev, directory, len);
        dev[len] = '\0';

        if ((dev[0] != '/') && (dev[0] != '~'))
          dev[len - 1] = '\0';  /* remove trailing '/' */

        directory += p - directory;
     }
   else
     {
        dev = strdup(directory);
        directory = "/";
     }

   e_fwin_new(zone->container, dev, directory);
   free(dev);
   free(to_free);
   return dbus_message_new_method_return(message);
}

static Eina_Bool
_mime_shell_script_check(const char *mime)
{
   static const struct sh_script_map {
      const char *str;
      size_t len;
   } options[] = {
#define O(x) {x, sizeof(x) - 1}
     O("application/x-sh"),
     O("application/x-shellscript"),
     O("text/x-sh"),
#undef O
     {NULL, 0}
   };
   const struct sh_script_map *itr;
   size_t mimelen = strlen(mime);

   for (itr = options; itr->str != NULL; itr++)
     if ((mimelen == itr->len) && (memcmp(mime, itr->str, mimelen) == 0))
       return EINA_TRUE;

   return EINA_FALSE;
}

static DBusMessage *
_e_fileman_dbus_daemon_open_file_cb(E_DBus_Object *obj __UNUSED__,
                                    DBusMessage       *message)
{
   DBusMessageIter itr;
   Eina_List *handlers;
   const char *param_file = NULL, *mime, *errmsg = "unknow error";
   char *real_file, *to_free = NULL;
   E_Zone *zone;

   dbus_message_iter_init(message, &itr);
   dbus_message_iter_get_basic(&itr, &param_file);

   if ((!param_file) || (param_file[0] == '\0'))
     return _e_fileman_dbus_daemon_error(message, "no file provided.");

   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone)
     return _e_fileman_dbus_daemon_error(message, "could not find a zone.");

   if (!strstr(param_file, "://"))
     {
        real_file = ecore_file_realpath(param_file);
        if (!real_file)
          {
             errmsg = "couldn't get realpath for file.";
             goto error;
          }
     }
   else
     {
        Efreet_Uri *uri = efreet_uri_decode(param_file);

        real_file = NULL;
        if (uri)
          {
             if ((uri->protocol) && (strcmp(uri->protocol, "file") == 0))
               {
                  real_file = ecore_file_realpath(uri->path);
                  param_file = to_free = strdup(uri->path);
               }
             efreet_uri_free(uri);
          }

        if (!real_file)
          {
             errmsg = "unsupported protocol";
             goto error;
          }
     }

   mime = efreet_mime_type_get(real_file);
   if (!mime)
     {
        errmsg = "couldn't find mime-type";
        goto error;
     }

   if (strcmp(mime, "application/x-desktop") == 0)
     {
        Efreet_Desktop *desktop = efreet_desktop_new(real_file);
        if (!desktop)
          {
             errmsg = "couldn't open desktop file";
             goto error;
          }

        e_exec(zone, desktop, NULL, NULL, NULL);
        efreet_desktop_free(desktop);
        goto end;
     }
   else if ((strcmp(mime, "application/x-executable") == 0) ||
            ecore_file_can_exec(param_file))
     {
        e_exec(zone, NULL, param_file, NULL, NULL);
        goto end;
     }
   else if (_mime_shell_script_check(mime))
     {
        Eina_Strbuf *b = eina_strbuf_new();
        const char *shell = getenv("SHELL");
        if (!shell)
          {
             uid_t uid = getuid();
             struct passwd *pw = getpwuid(uid);
             if (pw) shell = pw->pw_shell;
          }
        if (!shell) shell = "/bin/sh";
        eina_strbuf_append_printf(b, "%s %s %s",
                                  e_config->exebuf_term_cmd,
                                  shell,
                                  param_file);
        e_exec(zone, NULL, eina_strbuf_string_get(b), NULL, NULL);
        eina_strbuf_free(b);
        goto end;
     }

   handlers = efreet_util_desktop_mime_list(mime);
   if (!handlers)
     {
        errmsg = "no handlers for given file";
        goto end;
     }
   else
     {
        Efreet_Desktop *desktop = handlers->data;
        Eina_List *files = eina_list_append(NULL, param_file);

        e_exec(zone, desktop, NULL, files, NULL);
        eina_list_free(files);

        EINA_LIST_FREE(handlers, desktop)
          efreet_desktop_free(desktop);
     }

 end:
   free(real_file);
   free(to_free);
   return dbus_message_new_method_return(message);

 error:
   free(real_file);
   free(to_free);
   return _e_fileman_dbus_daemon_error(message, errmsg);
}

static void
_e_fileman_dbus_daemon_request_name_cb(void        *data,
                                       DBusMessage *msg,
                                       DBusError   *err)
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
   const struct
   {
      const char      *method;
      const char      *signature;
      const char      *ret_signature;
      E_DBus_Method_Cb func;
   } *itr, desc[] = {
      {"OpenDirectory", "s", "", _e_fileman_dbus_daemon_open_directory_cb},
      {"OpenFile", "s", "", _e_fileman_dbus_daemon_open_file_cb},
      {NULL, NULL, NULL, NULL}
   };
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

   for (itr = desc; itr->method; itr++)
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


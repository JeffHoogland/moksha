#include "e_mod_main.h"

static const char E_FILEMAN_BUS_NAME[] = "org.enlightenment.FileManager";
static const char E_FILEMAN_INTERFACE[] = "org.enlightenment.FileManager";
static const char E_FILEMAN_ERROR[] = "org.enlightenment.FileManager.Error";
static const char E_FILEMAN_PATH[] = "/org/enlightenment/FileManager";

typedef struct _E_Fileman_DBus_Daemon E_Fileman_DBus_Daemon;
struct _E_Fileman_DBus_Daemon
{
   Eldbus_Connection *conn;
   Eldbus_Service_Interface *iface;
};

static Eldbus_Message * _e_fileman_dbus_daemon_open_directory_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message *_e_fileman_dbus_daemon_open_file_cb(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

static Eldbus_Message *
_e_fileman_dbus_daemon_error(const Eldbus_Message *msg,
                             const char  *error_msg)
{
   return eldbus_message_error_new(msg, E_FILEMAN_ERROR, error_msg);
}

static const Eldbus_Method methods[] = {
   { "OpenDirectory", ELDBUS_ARGS({"s", "directory"}), NULL,
      _e_fileman_dbus_daemon_open_directory_cb, 0 },
   { "OpenFile", ELDBUS_ARGS({"s", "file"}), NULL,
      _e_fileman_dbus_daemon_open_file_cb, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const Eldbus_Service_Interface_Desc desc = {
   E_FILEMAN_INTERFACE, methods, NULL, NULL, NULL, NULL
};

static void
_e_fileman_dbus_daemon_object_init(E_Fileman_DBus_Daemon *d)
{
   d->iface = eldbus_service_interface_register(d->conn, E_FILEMAN_PATH, &desc);
   if (!d->iface)
     {
        fprintf(stderr, "ERROR: cannot add object to %s\n", E_FILEMAN_PATH);
        return;
     }
}

static void
_e_fileman_dbus_daemon_free(E_Fileman_DBus_Daemon *d)
{
   if (d->iface)
     eldbus_service_object_unregister(d->iface);
   if (d->conn)
     eldbus_connection_unref(d->conn);

   free(d);
}

static Eldbus_Message *
_e_fileman_dbus_daemon_open_directory_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                                         const Eldbus_Message *msg)
{
   const char *directory = NULL, *p;
   char *dev, *to_free = NULL;
   E_Zone *zone;

   if (!eldbus_message_arguments_get(msg, "s", &directory))
     {
        fprintf(stderr, "Error: getting arguments of OpenDirectory call.\n");
        return eldbus_message_method_return_new(msg);
     }

   if ((!directory) || (directory[0] == '\0'))
     return _e_fileman_dbus_daemon_error(msg, "no directory provided.");

   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone)
     return _e_fileman_dbus_daemon_error(msg, "could not find a zone.");

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
          return _e_fileman_dbus_daemon_error(msg, "unsupported protocol");
     }

   p = strchr(directory, '/');
   if (p)
     {
        int len = p - directory + 1;

        dev = malloc(len + 1);
        if (!dev)
          {
             free(to_free);
             return _e_fileman_dbus_daemon_error(msg,
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
   return eldbus_message_method_return_new(msg);
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

static Eldbus_Message*
_e_fileman_dbus_daemon_open_file_cb(const Eldbus_Service_Interface *iface __UNUSED__,
                                    const Eldbus_Message *msg)
{
   Eina_List *handlers;
   const char *param_file = NULL, *mime, *errmsg = "unknow error";
   char *real_file, *to_free = NULL;
   E_Zone *zone;

   if (!eldbus_message_arguments_get(msg, "s", &param_file))
     {
        fprintf(stderr, "ERROR: getting arguments of OpenFile call.\n");
        return eldbus_message_method_return_new(msg);
     }

   if ((!param_file) || (param_file[0] == '\0'))
     return _e_fileman_dbus_daemon_error(msg, "no file provided.");

   zone = e_util_zone_current_get(e_manager_current_get());
   if (!zone)
     return _e_fileman_dbus_daemon_error(msg, "could not find a zone.");

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
   return eldbus_message_method_return_new(msg);

 error:
   free(real_file);
   free(to_free);
   return _e_fileman_dbus_daemon_error(msg, errmsg);
}

static E_Fileman_DBus_Daemon *
_e_fileman_dbus_daemon_new(void)
{
   E_Fileman_DBus_Daemon *d;

   d = calloc(1, sizeof(E_Fileman_DBus_Daemon));
   if (!d)
     {
        perror("ERROR: FILEMAN: cannot allocate fileman dbus daemon memory.");
        return NULL;
     }

   d->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!d->conn)
     goto error;

   _e_fileman_dbus_daemon_object_init(d);
   eldbus_name_request(d->conn, E_FILEMAN_BUS_NAME,
                      ELDBUS_NAME_REQUEST_FLAG_REPLACE_EXISTING, NULL, NULL);
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

   eldbus_init();
   _daemon = _e_fileman_dbus_daemon_new();
}

void
e_fileman_dbus_shutdown(void)
{
   if (!_daemon)
     return;

   _e_fileman_dbus_daemon_free(_daemon);
   _daemon = NULL;
   eldbus_shutdown();
}

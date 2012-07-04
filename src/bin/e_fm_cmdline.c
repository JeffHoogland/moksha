# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Getopt.h>
#include <E_DBus.h>
#include <unistd.h>

static E_DBus_Connection *conn = NULL;
static int retval = EXIT_SUCCESS;
static int pending = 0;

static void
fm_open_reply(void *data __UNUSED__, DBusMessage *msg __UNUSED__, DBusError *err)
{
   if (dbus_error_is_set(err))
     {
        retval = EXIT_FAILURE;
        fprintf(stderr, "ERROR: %s: %s", err->name, err->message);
     }

   pending--;
   if (!pending) ecore_main_loop_quit();
}

static Eina_Bool
fm_error_quit_last(void *data __UNUSED__)
{
   if (!pending) ecore_main_loop_quit();
   return EINA_FALSE;
}

static void
fm_open(const char *path)
{
   DBusMessage *msg;
   Eina_Bool sent;
   const char *method;
   char *p;

   if (path[0] == '/')
     p = strdup(path);
   else
     {
        char buf[PATH_MAX];
        if (!getcwd(buf, sizeof(buf)))
          {
             fprintf(stderr,
                     "ERROR: Could not get current working directory: %s\n",
                     strerror(errno));
             ecore_idler_add(fm_error_quit_last, NULL);
             return;
          }
        if (strcmp(path, ".") == 0)
          p = strdup(buf);
        else
          {
             char tmp[PATH_MAX];
             snprintf(tmp, sizeof(tmp), "%s/%s", buf, path);
             p = strdup(tmp);
          }
     }

   EINA_LOG_DBG("'%s' -> '%s'", path, p);
   if ((!p) || (p[0] == '\0'))
     {
        fprintf(stderr, "ERROR: Could not get path '%s'\n", path);
        ecore_idler_add(fm_error_quit_last, NULL);
        free(p);
        return;
     }

   if (ecore_file_is_dir(p))
     method = "OpenDirectory";
   else
     method = "OpenFile";

   msg = dbus_message_new_method_call
     ("org.enlightenment.FileManager",
      "/org/enlightenment/FileManager",
      "org.enlightenment.FileManager", method);
   if (!msg)
     {
        fputs("ERROR: Could not create DBus Message\n", stderr);
        ecore_idler_add(fm_error_quit_last, NULL);
        free(p);
        return;
     }

   dbus_message_append_args(msg, DBUS_TYPE_STRING, &p, DBUS_TYPE_INVALID);
   free(p);

   sent = !!e_dbus_message_send(conn, msg, fm_open_reply, -1, NULL);
   dbus_message_unref(msg);

   if (!sent)
     {
        fputs("ERROR: Could not send DBus Message\n", stderr);
        ecore_idler_add(fm_error_quit_last, NULL);
        return;
     }

   pending++;
}

static const Ecore_Getopt options = {
   "enlightenment_filemanager",
   "%prog [options] [file-or-folder1] ... [file-or-folderN]",
   PACKAGE_VERSION,
   "(C) 2012 Gustavo Sverzut Barbieri and others",
   "BSD 2-Clause",
   "Opens the Enlightenment File Manager at a given folders.",
   EINA_FALSE,
   {
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('C', "copyright"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

EAPI int
main(int argc, char *argv[])
{
   Eina_Bool quit_option = EINA_FALSE;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };
   int args;

   args = ecore_getopt_parse(&options, values, argc, argv);
   if (args < 0)
     {
        fputs("ERROR: Could not parse command line options.\n", stderr);
        return EXIT_FAILURE;
     }

   if (quit_option) return EXIT_SUCCESS;

   ecore_init();
   ecore_file_init();
   e_dbus_init();

   conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if (!conn)
     {
        fputs("ERROR: Could not DBus SESSION bus.\n", stderr);
        retval = EXIT_FAILURE;
        goto end;
     }

   retval = EXIT_SUCCESS;

   if (args == argc) fm_open(".");
   else
     {
        for (; args < argc; args++)
          fm_open(argv[args]);
     }

   ecore_main_loop_begin();

 end:
   e_dbus_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();
   return retval;
}

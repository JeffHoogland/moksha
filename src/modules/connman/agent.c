#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "e.h"
#include "agent.h"
#include "E_Connman.h"

static unsigned int init_count;
static E_DBus_Connection *conn;
static E_DBus_Object *agent_obj;

#define AGENT_IFACE "net.connman.Agent"

struct dialog
{
   E_Dialog *dia;
   Evas_Object *list;
};

static struct dialog *dialog;

struct Connman_Field
{
   const char *name;

   const char *type;
   const char *requirement;
   const char *value;
   Eina_Array *alternates;
};

static struct dialog *_dialog_create(void)
{
   struct dialog *dia;

   dia = E_NEW(struct dialog, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dia, NULL);

   dia->dia = e_dialog_new(NULL, "E", "connman_request_input");
   e_dialog_title_set(dia->dia, _("Input requested"));
   e_dialog_icon_set(dia->dia, "dialog-ask", 64);
   e_dialog_border_icon_set(dia->dia, "dialog-ask");

   e_dialog_button_add(dia->dia, _("Ok"), NULL, NULL, NULL);
   e_dialog_button_add(dia->dia, _("Cancel"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia->dia, 0);
   e_win_centered_set(dia->dia->win, 1);

   dia->list = e_widget_list_add(dia->dia->win->evas, 0, 0);

   return dia;
}

void _dialog_release(void)
{
   EINA_SAFETY_ON_NULL_RETURN(dialog);

   e_object_del(E_OBJECT(dialog->dia));
   free(dialog);
   dialog = NULL;
}

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

static bool _parse_field_value(struct Connman_Field *field, const char *key,
                               DBusMessageIter *value)
{
   if (!strcmp(key, "Type"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING, false);
        dbus_message_iter_get_basic(value, &field->type);
        return true;
     }
   else if (!strcmp(key, "Requirement"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING, false);
        dbus_message_iter_get_basic(value, &field->requirement);
        return true;
     }
   else if (!strcmp(key, "Alternates"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_ARRAY, false);
         /* ignore alternates */
        return true;
     }
   else if (!strcmp(key, "Value"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING, false);
        dbus_message_iter_get_basic(value, &field->value);
        return true;
     }
   else
     DBG("Ignored unknown argument: %s", key);

   return false;
}

static bool _parse_field(struct Connman_Field *field, DBusMessageIter *value)
{
   DBusMessageIter dict;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(
      dbus_message_iter_get_arg_type(value) == DBUS_TYPE_ARRAY, false);
   dbus_message_iter_recurse(value, &dict);

   for (; dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&dict))
     {
        DBusMessageIter entry, var;
        const char *key;

        dbus_message_iter_recurse(&dict, &entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING, false);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT, false);
        dbus_message_iter_recurse(&entry, &var);

        if (!_parse_field_value(field, key, &var))
          return false;
     }

   return true;
}

static DBusMessage *_agent_request_input(E_DBus_Object *obj, DBusMessage *msg)
{
   DBusMessage *reply;
   DBusMessageIter iter, dict;
   const char *path;

   if (dialog != NULL)
     _dialog_release();

   reply = dbus_message_new_method_return(msg);

   dialog = _dialog_create();
   EINA_SAFETY_ON_NULL_RETURN_VAL(dialog, reply);

   dbus_message_iter_init(msg, &iter);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH, reply);
   dbus_message_iter_get_basic(&iter, &path);
   //cs = econnman_manager_find_service(cm, path);

   dbus_message_iter_next(&iter);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(
      dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY, reply);
   dbus_message_iter_recurse(&iter, &dict);

   for (; dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&dict))
     {
        struct Connman_Field field = { NULL };
        DBusMessageIter entry, var;

        dbus_message_iter_recurse(&dict, &entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING, reply);
        dbus_message_iter_get_basic(&entry, &field.name);

        dbus_message_iter_next(&entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT, reply);
        dbus_message_iter_recurse(&entry, &var);

        if (!_parse_field(&field, &var))
          return NULL;

        DBG("AGENT Got field:\n"
            "\tname: %s\n"
            "\tType: %s\n"
            "\tRequirement: %s\n"
            "\tAlternates: (omit array)\n"
            "\tValue: %s",
            field.name, field.type, field.requirement, field.value);
     }

   return NULL;
}

static DBusMessage *_agent_cancel(E_DBus_Object *obj, DBusMessage *msg)
{
   if (dialog != NULL)
     _dialog_release();

   return dbus_message_new_method_return(msg);
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

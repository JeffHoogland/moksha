#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "e.h"
#include "E_Connman.h"
#include "e_mod_main.h"
#define AGENT_IFACE "net.connman.Agent"

typedef struct _E_Connman_Agent_Input E_Connman_Agent_Input;

struct Connman_Field
{
   const char *name;

   const char *type;
   const char *requirement;
   const char *value;
   Eina_Array *alternates;
};

struct _E_Connman_Agent_Input
{
   char *key;
   char *value;
   int show_password;
};

struct _E_Connman_Agent
{
   E_Dialog *dialog;
   E_DBus_Object *obj;
   DBusMessage *msg;
   E_DBus_Connection *conn;
   Eina_Bool canceled:1;
};

static void
_dict_append_basic(DBusMessageIter *dict, const char *key, void *val)
{
   DBusMessageIter entry, value;

   dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
                                    NULL, &entry);

   dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
   dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                    DBUS_TYPE_STRING_AS_STRING, &value);
   dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &val);
   dbus_message_iter_close_container(&entry, &value);

   dbus_message_iter_close_container(dict, &entry);
}

static void
_dialog_ok_cb(void *data, E_Dialog *dialog)
{
   E_Connman_Agent *agent = data;
   E_Connman_Agent_Input *input;
   Evas_Object *toolbook, *list;
   DBusMessageIter iter, dict;
   Eina_List *input_list, *l;
   DBusMessage *reply;

   toolbook = agent->dialog->content_object;

   /* fugly - no toolbook page get */
   list = evas_object_data_get(toolbook, "mandatory");
   if ((!list) || (!evas_object_visible_get(list)))
     {
        list = evas_object_data_get(toolbook, "alternate");
        if ((!list) || (!evas_object_visible_get(list)))
          {
             ERR("Couldn't get user input.");
             e_object_del(E_OBJECT(dialog));
             return;
          }
     }

   agent->canceled = EINA_FALSE;
   input_list = evas_object_data_get(list, "input_list");

   reply = dbus_message_new_method_return(agent->msg);
   dbus_message_iter_init_append(reply, &iter);

   dbus_message_iter_open_container(
      &iter, DBUS_TYPE_ARRAY,
      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
      DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
      DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

   EINA_LIST_FOREACH(input_list, l, input)
      _dict_append_basic(&dict, input->key, input->value);

   dbus_message_iter_close_container(&iter, &dict);

   dbus_message_set_no_reply(reply, EINA_TRUE);
   e_dbus_message_send(agent->conn, reply, NULL, -1, NULL);

   e_object_del(E_OBJECT(dialog));
}

static void
_dialog_cancel_cb(void *data, E_Dialog *dialog)
{
   E_Connman_Agent *agent = data;
   agent->canceled = EINA_TRUE;
   e_object_del(E_OBJECT(dialog));
}

static void
_dialog_key_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__,
                    void *event)
{
   Evas_Event_Key_Down *ev = event;
   E_Connman_Agent *agent = data;

   if (!strcmp(ev->key, "Return"))
     _dialog_ok_cb(agent, agent->dialog);
   else if (strcmp(ev->key, "Escape") == 0)
     _dialog_cancel_cb(agent, agent->dialog);
}

static void
_dialog_cancel(E_Connman_Agent *agent)
{
   DBusMessage *reply;

   reply = dbus_message_new_error(agent->msg,
                                  "net.connman.Agent.Error.Canceled",
                                  "User canceled dialog");
   dbus_message_set_no_reply(reply, EINA_TRUE);
   e_dbus_message_send(agent->conn, reply, NULL, -1, NULL);
}

static void
_dialog_del_cb(void *data)
{
   E_Dialog *dialog = data;
   E_Connman_Agent *agent = e_object_data_get(E_OBJECT(dialog));

   if (agent->canceled)
     _dialog_cancel(agent);

   // FIXME need to mark cs->pending_connect = NULL;
   dbus_message_unref(agent->msg);
   agent->dialog = NULL;
}

static void
_page_del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Connman_Agent_Input *input;
   Eina_List *input_list;

   input_list = evas_object_data_get(obj, "input_list");
   EINA_LIST_FREE(input_list, input)
     {
        free(input->key);
        free(input);
     }
}

static void
_show_password_cb(void *data, Evas_Object *obj, void *event  __UNUSED__)
{
   Evas_Object *entry = data;
   int hidden;

   hidden = !e_widget_check_checked_get(obj);
   e_widget_entry_password_set(entry, hidden);
}

static void
_dialog_field_add(E_Connman_Agent *agent, struct Connman_Field *field)
{
   Evas_Object *toolbook, *list, *framelist, *entry, *check;
   E_Connman_Agent_Input *input;
   Eina_List *input_list;
   Eina_Bool mandatory;
   char header[64];
   Evas *evas;

   evas = agent->dialog->win->evas;
   toolbook = agent->dialog->content_object;
   mandatory = !strcmp(field->requirement, "mandatory");

   if ((!mandatory) && (strcmp(field->requirement, "alternate")))
     {
        WRN("Field not handled: %s %s", field->name, field->type);
        return;
     }

   input = E_NEW(E_Connman_Agent_Input, 1);
   input->key = strdup(field->name);
   entry = e_widget_entry_add(evas, &(input->value), NULL, NULL, NULL);
   evas_object_show(entry);

   list = evas_object_data_get(toolbook, field->requirement);
   if (!list)
     {
        list = e_widget_list_add(evas, 0, 0);
        e_widget_toolbook_page_append(toolbook, NULL, field->name,
                                      list, 1, 1, 1, 1, 0.5, 0.0);
        evas_object_data_set(toolbook, field->requirement, list);

        e_widget_toolbook_page_show(toolbook, 0);
        evas_object_event_callback_add(list, EVAS_CALLBACK_DEL,
                                       _page_del, NULL);

        if (mandatory)
          e_widget_focus_set(entry, 1);
     }

   input_list = evas_object_data_get(list, "input_list");
   input_list = eina_list_append(input_list, input);
   evas_object_data_set(list, "input_list", input_list);

   snprintf(header, sizeof(header), "%s required to access network:",
            field->name);
   framelist = e_widget_framelist_add(evas, header, 0);
   evas_object_show(framelist);
   e_widget_list_object_append(list, framelist, 1, 1, 0.5);

   e_widget_framelist_object_append(framelist, entry);

   if ((!strcmp(field->name, "Passphrase")) ||
       (!strcmp(field->name, "Password")))
     {
        e_widget_entry_password_set(entry, 1);

        check = e_widget_check_add(evas, _("Show password"),
                                   &(input->show_password));
        evas_object_show(check);
        e_widget_framelist_object_append(framelist, check);

        evas_object_smart_callback_add(check, "changed", _show_password_cb,
                                       entry);
     }
}

static E_Dialog *
_dialog_new(E_Connman_Agent *agent)
{
   Evas_Object *toolbook;
   E_Dialog *dialog;
   Evas *evas;
   int mw, mh;

   dialog = e_dialog_new(NULL, "E", "connman_request_input");
   if (!dialog)
     return NULL;

   e_dialog_title_set(dialog, _("Input requested"));
   e_dialog_border_icon_set(dialog, "dialog-ask");

   e_dialog_button_add(dialog, _("Ok"), NULL, _dialog_ok_cb, agent);
   e_dialog_button_add(dialog, _("Cancel"), NULL, _dialog_cancel_cb, agent);
   agent->canceled = EINA_TRUE; /* if win is closed it works like cancel */

   evas = dialog->win->evas;

   toolbook = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);
   evas_object_show(toolbook);

   e_widget_size_min_get(toolbook, &mw, &mh);
   /* is it a hack ? */
   if (mw < 260)
     mw = 260;
   if (mh < 130)
     mh = 130;
   e_dialog_content_set(dialog, toolbook, mw, mh);
   e_dialog_show(dialog);

   evas_object_event_callback_add(dialog->bg_object, EVAS_CALLBACK_KEY_DOWN,
                                  _dialog_key_down_cb, agent);

   e_object_del_attach_func_set(E_OBJECT(dialog), _dialog_del_cb);
   e_object_data_set(E_OBJECT(dialog), agent);

   e_dialog_button_focus_num(dialog, 0);
   e_win_centered_set(dialog->win, 1);

   return dialog;
}

static DBusMessage *
_agent_release(E_DBus_Object *obj, DBusMessage *msg)
{
   E_Connman_Agent *agent;
   DBusMessage *reply;

   DBG("Agent released");

   reply = dbus_message_new_method_return(msg);

   agent = e_dbus_object_data_get(obj);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(agent, reply);

   if (agent->dialog)
     e_object_del(E_OBJECT(agent->dialog));

   return reply;
}

static DBusMessage *
_agent_report_error(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static DBusMessage *
_agent_request_browser(E_DBus_Object *obj, DBusMessage *msg)
{
   return NULL;
}

static Eina_Bool
_parse_field_value(struct Connman_Field *field, const char *key,
                   DBusMessageIter *value)
{
   if (!strcmp(key, "Type"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING,
           EINA_FALSE);
        dbus_message_iter_get_basic(value, &field->type);
        return EINA_TRUE;
     }

   if (!strcmp(key, "Requirement"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING,
           EINA_FALSE);
        dbus_message_iter_get_basic(value, &field->requirement);
        return EINA_TRUE;
     }

   if (!strcmp(key, "Alternates"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_ARRAY,
           EINA_FALSE);
         /* ignore alternates */
        return EINA_TRUE;
     }

   if (!strcmp(key, "Value"))
     {
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(value) == DBUS_TYPE_STRING,
           EINA_FALSE);
        dbus_message_iter_get_basic(value, &field->value);
        return EINA_TRUE;
     }

   DBG("Ignored unknown argument: %s", key);
   return EINA_FALSE;
}

static Eina_Bool
_parse_field(struct Connman_Field *field, DBusMessageIter *value)
{
   DBusMessageIter dict;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(
      dbus_message_iter_get_arg_type(value) == DBUS_TYPE_ARRAY, EINA_FALSE);
   dbus_message_iter_recurse(value, &dict);

   for (; dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&dict))
     {
        DBusMessageIter entry, var;
        const char *key;

        dbus_message_iter_recurse(&dict, &entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING,
           EINA_FALSE);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        EINA_SAFETY_ON_FALSE_RETURN_VAL(
           dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT,
           EINA_FALSE);
        dbus_message_iter_recurse(&entry, &var);

        if (!_parse_field_value(field, key, &var))
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

static DBusMessage *
_agent_request_input(E_DBus_Object *obj, DBusMessage *msg)
{
   E_Connman_Module_Context *ctxt = connman_mod->data;
   const Eina_List *l;
   E_Connman_Instance *inst;
   DBusMessageIter iter, dict;
   E_Connman_Agent *agent;
   DBusMessage *reply;
   const char *path;

   agent = e_dbus_object_data_get(obj);

   /* Discard previous requests */
   // if msg is the current agent msg? eek.
   if (agent->msg == msg) return NULL;

   if (agent->msg)
     dbus_message_unref(agent->msg);
   agent->msg = dbus_message_ref(msg);

   EINA_LIST_FOREACH(ctxt->instances, l, inst)
     econnman_popup_del(inst);

   if (agent->dialog)
     e_object_del(E_OBJECT(agent->dialog));
   agent->dialog = _dialog_new(agent);
   EINA_SAFETY_ON_NULL_GOTO(agent->dialog, err);

   dbus_message_iter_init(msg, &iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_OBJECT_PATH)
     goto err;
   dbus_message_iter_get_basic(&iter, &path);

   dbus_message_iter_next(&iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
     goto err;
   dbus_message_iter_recurse(&iter, &dict);

   for (; dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&dict))
     {
        struct Connman_Field field = { NULL, NULL, NULL, NULL, NULL };
        DBusMessageIter entry, var;

        dbus_message_iter_recurse(&dict, &entry);
        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
          goto err;
        dbus_message_iter_get_basic(&entry, &field.name);

        dbus_message_iter_next(&entry);
        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
          goto err;
        dbus_message_iter_recurse(&entry, &var);

        if (!_parse_field(&field, &var))
          return NULL;

        DBG("AGENT Got field:\n"
            "\tName: %s\n"
            "\tType: %s\n"
            "\tRequirement: %s\n"
            "\tAlternates: (omit array)\n"
            "\tValue: %s",
            field.name, field.type, field.requirement, field.value);

        _dialog_field_add(agent, &field);
     }

   return NULL;

err:
   dbus_message_unref(msg);
   agent->msg = NULL;
   WRN("Failed to parse msg");
   reply = dbus_message_new_method_return(msg);
   return reply;
}

static DBusMessage *
_agent_cancel(E_DBus_Object *obj, DBusMessage *msg)
{
   E_Connman_Agent *agent;
   DBusMessage *reply;

   DBG("Agent canceled");

   reply = dbus_message_new_method_return(msg);

   agent = e_dbus_object_data_get(obj);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(agent, reply);

   if (agent->dialog)
     e_object_del(E_OBJECT(agent->dialog));

   return reply;
}

E_Connman_Agent *
econnman_agent_new(E_DBus_Connection *edbus_conn)
{
   E_DBus_Object *agent_obj;
   E_DBus_Interface *iface;
   E_Connman_Agent *agent;

   agent = E_NEW(E_Connman_Agent, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(agent, NULL);

   iface = e_dbus_interface_new(AGENT_IFACE);
   if (!iface)
     {
        ERR("Failed to create e_dbus interface");
        free(agent);
        return NULL;
     }

   e_dbus_interface_method_add(iface, "Release", "", "", _agent_release);
   e_dbus_interface_method_add(iface, "ReportError", "os", "",
                               _agent_report_error);
   e_dbus_interface_method_add(iface, "RequestBrowser", "os", "",
                               _agent_request_browser);
   e_dbus_interface_method_add(iface, "RequestInput", "oa{sv}", "a{sv}",
                               _agent_request_input);
   e_dbus_interface_method_add(iface, "Cancel", "", "", _agent_cancel);

   agent_obj = e_dbus_object_add(edbus_conn, AGENT_PATH, agent);
   if (!agent_obj)
     {
        ERR("Failed to create e_dbus object");
        e_dbus_interface_unref(iface);
        free(agent);
        return NULL;
     }

   agent->obj = agent_obj;
   agent->conn = edbus_conn;
   e_dbus_object_interface_attach(agent_obj, iface);

   e_dbus_interface_unref(iface);

   return agent;
}

void
econnman_agent_del(E_Connman_Agent *agent)
{
   EINA_SAFETY_ON_NULL_RETURN(agent);
   e_dbus_object_free(agent->obj);
   free(agent);
}

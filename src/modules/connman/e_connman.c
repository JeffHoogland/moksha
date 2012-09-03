#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "E_Connman.h"
#include "agent.h"

#define CONNMAN_BUS_NAME "net.connman"
#define CONNMAN_MANAGER_IFACE CONNMAN_BUS_NAME ".Manager"
#define CONNMAN_SERVICE_IFACE CONNMAN_BUS_NAME ".Service"

static unsigned int init_count;
static E_DBus_Connection *conn;
static char *bus_owner;
static struct Connman_Manager *connman_manager;

static DBusPendingCall *pending_get_name_owner;
static E_DBus_Signal_Handler *handler_name_owner;

EAPI int E_CONNMAN_EVENT_MANAGER_IN;
EAPI int E_CONNMAN_EVENT_MANAGER_OUT;

/* utility functions */

static void _eina_str_array_clean(Eina_Array *array)
{
   const char *item;
   Eina_Array_Iterator itr;
   unsigned int i;

   EINA_ARRAY_ITER_NEXT(array, i, item, itr)
     eina_stringshare_del(item);

   eina_array_clean(array);
}

static bool _dbus_bool_get(DBusMessageIter *itr)
{
   dbus_bool_t val;
   dbus_message_iter_get_basic(itr, &val);
   return val;
}

static void _dbus_str_array_to_eina(DBusMessageIter *value, Eina_Array **old,
                                    unsigned nelem)
{
   DBusMessageIter itr;
   Eina_Array *array;
   EINA_SAFETY_ON_NULL_RETURN(value);
   EINA_SAFETY_ON_NULL_RETURN(old);

   EINA_SAFETY_ON_FALSE_RETURN(
      dbus_message_iter_get_arg_type(value) == DBUS_TYPE_ARRAY);

   dbus_message_iter_recurse(value, &itr);

   array = *old;
   if (array == NULL)
     {
        array = eina_array_new(nelem);
        *old = array;
     }
   else
     _eina_str_array_clean(array);

   for (; dbus_message_iter_get_arg_type(&itr) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&itr))
     {
        const char *s;
        if (dbus_message_iter_get_arg_type(&itr) != DBUS_TYPE_STRING)
          {
             ERR("Unexpected D-Bus type %d",
                 dbus_message_iter_get_arg_type(&itr));
             continue;
          }

        dbus_message_iter_get_basic(&itr, &s);
        eina_array_push(array, eina_stringshare_add(s));
        DBG("Push %s", s);
     }

   return;
}

static enum Connman_State str_to_state(const char *s)
{
   if (strcmp(s, "offline") == 0)
     return CONNMAN_STATE_OFFLINE;
   if (strcmp(s, "idle") == 0)
     return CONNMAN_STATE_IDLE;
   if (strcmp(s, "ready") == 0)
     return CONNMAN_STATE_READY;
   if (strcmp(s, "online") == 0)
     return CONNMAN_STATE_ONLINE;

   ERR("Unknown state %s", s);
   return CONNMAN_STATE_NONE;
}

const char *econnman_state_to_str(enum Connman_State state)
{
   switch (state)
     {
      case CONNMAN_STATE_OFFLINE:
         return "offline";
      case CONNMAN_STATE_IDLE:
         return "idle";
      case CONNMAN_STATE_READY:
         return "ready";
      case CONNMAN_STATE_ONLINE:
         return "online";
      case CONNMAN_STATE_NONE:
         break;
     }

   return NULL;
}

static enum Connman_Service_Type str_to_type(const char *s)
{
   if (strcmp(s, "ethernet") == 0)
     return CONNMAN_SERVICE_TYPE_ETHERNET;
   else if (strcmp(s, "wifi") == 0)
     return CONNMAN_SERVICE_TYPE_WIFI;

   DBG("Unknown type %s", s);
   return CONNMAN_SERVICE_TYPE_NONE;
}

const char *econnman_service_type_to_str(enum Connman_Service_Type type)
{
   switch (type)
     {
      case CONNMAN_SERVICE_TYPE_ETHERNET:
         return "ethernet";
      case CONNMAN_SERVICE_TYPE_WIFI:
         return "wifi";
      case CONNMAN_SERVICE_TYPE_NONE:
         break;
     }

   return "other";
}

/* ---- */

static void _connman_object_init(struct Connman_Object *obj, const char *path)
{
   obj->path = eina_stringshare_add(path);
}

static void _connman_object_clear(struct Connman_Object *obj)
{
   E_DBus_Signal_Handler *h;

   EINA_LIST_FREE(obj->handlers, h)
     e_dbus_signal_handler_del(conn, h);

   eina_stringshare_del(obj->path);
}

static void _service_parse_prop_changed(struct Connman_Service *cs,
                                        const char *prop_name,
                                        DBusMessageIter *value)
{
   DBG("service %p %s prop %s", cs, cs->obj.path, prop_name);

   if (strcmp(prop_name, "State") == 0)
     {
        const char *state;
        dbus_message_iter_get_basic(value, &state);
        cs->state = str_to_state(state);
        DBG("New state: %s %d", state, cs->state);
     }
   else if (strcmp(prop_name, "Name") == 0)
     {
        const char *name;
        dbus_message_iter_get_basic(value, &name);
        free(cs->name);
        cs->name = strdup(name);
        DBG("New name: %s", cs->name);
     }
   else if (strcmp(prop_name, "Type") == 0)
     {
        const char *type;
        dbus_message_iter_get_basic(value, &type);
        cs->type = str_to_type(type);
        DBG("New type: %s %d", type, cs->type);
     }
   else if (strcmp(prop_name, "Strength") == 0)
     {
        uint8_t strength;
        dbus_message_iter_get_basic(value, &strength);
        cs->strength = strength;
        DBG("New strength: %d", strength);;
     }
   else if (strcmp(prop_name, "Security") == 0)
     {
        DBG("Old security count: %d",
            cs->security ? eina_array_count(cs->security) : 0);
        _dbus_str_array_to_eina(value, &cs->security, 2);
        DBG("New security count: %d", eina_array_count(cs->security));
     }
}

static void _service_prop_dict_changed(struct Connman_Service *cs,
                                       DBusMessageIter *dict)
{
   EINA_SAFETY_ON_FALSE_RETURN(dbus_message_iter_get_arg_type(dict) !=
                               DBUS_TYPE_ARRAY);

   for (; dbus_message_iter_get_arg_type(dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(dict))
     {
        DBusMessageIter entry, var;
        const char *name;

        dbus_message_iter_recurse(dict, &entry);

        EINA_SAFETY_ON_FALSE_RETURN(dbus_message_iter_get_arg_type(dict) !=
                                    DBUS_TYPE_STRING);
        dbus_message_iter_get_basic(&entry, &name);
        dbus_message_iter_next(&entry);

        EINA_SAFETY_ON_FALSE_RETURN(dbus_message_iter_get_arg_type(dict) !=
                                    DBUS_TYPE_VARIANT);
        dbus_message_iter_recurse(&entry, &var);

        _service_parse_prop_changed(cs, name, &var);
     }
}

static void _service_prop_changed(void *data, DBusMessage *msg)
{
   struct Connman_Service *cs = data;
   DBusMessageIter iter, var;
   const char *name;

   if (!msg || !dbus_message_iter_init(msg, &iter))
     {
        ERR("Could not parse message %p", msg);
        return;
     }

   dbus_message_iter_get_basic(&iter, &name);
   dbus_message_iter_next(&iter);
   dbus_message_iter_recurse(&iter, &var);

   _service_parse_prop_changed(cs, name, &var);
}

static void _service_free(struct Connman_Service *cs)
{
   if (!cs)
     return;

   free(cs->name);
   _eina_str_array_clean(cs->security);
   eina_array_free(cs->security);
   _connman_object_clear(&cs->obj);

   free(cs);
}

static struct Connman_Service *_service_new(const char *path, DBusMessageIter *props)
{
   struct Connman_Service *cs;
   E_DBus_Signal_Handler *h;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   cs = calloc(1, sizeof(*cs));
   EINA_SAFETY_ON_NULL_RETURN_VAL(cs, NULL);

   _connman_object_init(&cs->obj, path);

   h = e_dbus_signal_handler_add(conn, bus_owner,
                                 path, CONNMAN_SERVICE_IFACE, "PropertyChanged",
                                 _service_prop_changed, cs);
   cs->obj.handlers = eina_list_append(cs->obj.handlers, h);

   _service_prop_dict_changed(cs, props);
   return cs;
}

static struct Connman_Service *_manager_find_service_by_path(struct Connman_Manager *cm,
                                                             const char *path)
{
   struct Connman_Service *cs, *found = NULL;

   path = eina_stringshare_add(path);
   EINA_INLIST_FOREACH(cm->services, cs)
     {
        if (cs->obj.path == path)
          {
             found = cs;
             break;
          }
        }

   eina_stringshare_del(path);
   return found;
}

static void _manager_services_remove(struct Connman_Manager *cm,
                                     DBusMessageIter *array)
{
   for (; dbus_message_iter_get_arg_type(array) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(array))
     {
        struct Connman_Service *cs;
        const char *path;

        if (dbus_message_iter_get_arg_type(array) != DBUS_TYPE_OBJECT_PATH)
          {
             ERR("Unexpected D-Bus type %d",
                 dbus_message_iter_get_arg_type(array));
             continue;
          }
        dbus_message_iter_get_basic(array, &path);
        cs = _manager_find_service_by_path(cm, path);
        if (cs == NULL)
          {
             ERR("Received object path '%s' to remove, but it's not in list",
                 path);
             continue;
          }

        cm->services = eina_inlist_remove(cm->services, EINA_INLIST_GET(cs));
        DBG("Removed service: %p %s", cs, path);
        _service_free(cs);
     }
}

static void _manager_services_changed(void *data, DBusMessage *msg)
{
   struct Connman_Manager *cm = data;
   DBusMessageIter iter, changed, removed;
   Eina_Inlist *tmp = NULL;

   if (cm->pending.get_services)
     return;

   dbus_message_iter_init(msg, &iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
     {
        ERR("type=%d", dbus_message_iter_get_arg_type(&iter));
        return;
     }

   dbus_message_iter_recurse(&iter, &changed);
   dbus_message_iter_next(&iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
     {
        ERR("type=%d", dbus_message_iter_get_arg_type(&iter));
        return;
     }
   dbus_message_iter_recurse(&iter, &removed);

   _manager_services_remove(cm, &removed);

   for (; dbus_message_iter_get_arg_type(&changed) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&changed))
     {
        struct Connman_Service *cs;
        DBusMessageIter entry, dict;
        const char *path;

        dbus_message_iter_recurse(&changed, &entry);
        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_OBJECT_PATH)
          {
             ERR("Unexpected D-Bus type %d",
                 dbus_message_iter_get_arg_type(&entry));
             continue;
          }
        dbus_message_iter_get_basic(&entry, &path);

        cs = _manager_find_service_by_path(cm, path);

        dbus_message_iter_next(&entry);
        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_ARRAY)
          {
             ERR("Unexpected D-Bus type %d",
                 dbus_message_iter_get_arg_type(&entry));
             continue;
          }
        dbus_message_iter_recurse(&entry, &dict);

        if (cs == NULL)
          {
             cs = _service_new(path, &dict);
             DBG("Added service: %p %s", cs, path);
          }
        else
          {
             _service_prop_dict_changed(cs, &dict);
             cm->services = eina_inlist_remove(cm->services,
                                               EINA_INLIST_GET(cs));
             DBG("Changed service: %p %s", cs, path);
          }

        tmp = eina_inlist_append(tmp, EINA_INLIST_GET(cs));
     }

   cm->services = tmp;
   econnman_mod_services_changed(cm);
}

static void _manager_get_services_cb(void *data, DBusMessage *reply,
                                     DBusError *err)
{
   struct Connman_Manager *cm = data;
   DBusMessageIter iter, array;

   cm->pending.get_services = NULL;

   if (dbus_error_is_set(err))
     {
        DBG("Could not get services. %s: %s", err->name, err->message);
        return;
     }

   DBG("cm->services=%p", cm->services);

   dbus_message_iter_init(reply, &iter);
   if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
     {
        ERR("type=%d", dbus_message_iter_get_arg_type(&iter));
        return;
     }

   dbus_message_iter_recurse(&iter, &array);

   for (; dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&array))
     {
        struct Connman_Service *cs;
        const char *path;
        DBusMessageIter entry, dict;

        dbus_message_iter_recurse(&array, &entry);
        dbus_message_iter_get_basic(&entry, &path);
        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &dict);

        cs = _service_new(path, &dict);

        if (cs == NULL)
          continue;

        cm->services = eina_inlist_append(cm->services, EINA_INLIST_GET(cs));
        DBG("Added service: %p %s", cs, path);
     }

   econnman_mod_services_changed(cm);
}

static bool _manager_parse_prop_changed(struct Connman_Manager *cm,
                                        const char *name,
                                        DBusMessageIter *value)
{
   if (strcmp(name, "State") == 0)
     {
        const char *state;
        dbus_message_iter_get_basic(value, &state);
        DBG("New state: %s", state);
        cm->state = str_to_state(state);
     }
   else if (strcmp(name, "OfflineMode") == 0)
     cm->offline_mode = _dbus_bool_get(value);
   else
     return false;

   econnman_mod_manager_update(cm);
   return true;
}

static void _manager_prop_changed(void *data, DBusMessage *msg)
{
   struct Connman_Manager *cm = data;
   DBusMessageIter iter, var;
   const char *name;

   if (!msg || !dbus_message_iter_init(msg, &iter))
     {
        ERR("Could not parse message %p", msg);
        return;
     }

   dbus_message_iter_get_basic(&iter, &name);
   dbus_message_iter_next(&iter);
   dbus_message_iter_recurse(&iter, &var);

   _manager_parse_prop_changed(cm, name, &var);
}

static void _manager_get_prop_cb(void *data, DBusMessage *reply,
                                 DBusError *err)
{
   struct Connman_Manager *cm = data;
   DBusMessageIter iter, dict;

   cm->pending.get_properties = NULL;

   if (dbus_error_is_set(err))
     {
        DBG("Could not get properties. %s: %s", err->name, err->message);
        return;
     }

   dbus_message_iter_init(reply, &iter);
   dbus_message_iter_recurse(&iter, &dict);

   for (; dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_INVALID;
        dbus_message_iter_next(&dict))
     {
        DBusMessageIter entry, var;
        const char *name;

        dbus_message_iter_recurse(&dict, &entry);
        dbus_message_iter_get_basic(&entry, &name);
        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &var);

        _manager_parse_prop_changed(cm, name, &var);
     }
}

static void _manager_free(struct Connman_Manager *cm)
{
   if (!cm)
     return;

   while (cm->services)
     {
        struct Connman_Service *cs = EINA_INLIST_CONTAINER_GET(cm->services,
                                                      struct Connman_Service);
        cm->services = eina_inlist_remove(cm->services, cm->services);
        _service_free(cs);
     }

   if (cm->pending.get_services)
     {
        dbus_pending_call_cancel(cm->pending.get_services);
        cm->pending.get_services = NULL;
     }

   if (cm->pending.get_properties)
     {
        dbus_pending_call_cancel(cm->pending.get_properties);
        cm->pending.get_properties = NULL;
     }

   _connman_object_clear(&cm->obj);
   free(cm);
}

static struct Connman_Manager *_manager_new(void)
{
   DBusMessage *msg_props, *msg_services;
   const char *path = "/";
   struct E_DBus_Signal_Handler *h;
   struct Connman_Manager *cm;

   msg_services = dbus_message_new_method_call(CONNMAN_BUS_NAME, "/",
                                      CONNMAN_MANAGER_IFACE, "GetServices");
   msg_props = dbus_message_new_method_call(CONNMAN_BUS_NAME, "/",
                                      CONNMAN_MANAGER_IFACE, "GetProperties");

   if (!msg_services || !msg_services)
     {
        ERR("Could not create D-Bus messages");
        return NULL;
     }

   cm = calloc(1, sizeof(*cm));
   EINA_SAFETY_ON_NULL_RETURN_VAL(cm, NULL);

   _connman_object_init(&cm->obj, path);

   h = e_dbus_signal_handler_add(conn, bus_owner,
                                 path, CONNMAN_MANAGER_IFACE, "PropertyChanged",
                                 _manager_prop_changed, cm);
   cm->obj.handlers = eina_list_append(cm->obj.handlers, h);

   h = e_dbus_signal_handler_add(conn, bus_owner,
                                 path, CONNMAN_MANAGER_IFACE, "ServicesChanged",
                                 _manager_services_changed, cm);
   cm->obj.handlers = eina_list_append(cm->obj.handlers, h);

   /*
    * PropertyChanged signal in service's path is guaranteed to arrive only
    * after ServicesChanged above. So we only add the handler later, in a per
    * service manner.
    */

   cm->pending.get_services = e_dbus_message_send(conn, msg_services,
                                             _manager_get_services_cb, -1, cm);
   cm->pending.get_properties = e_dbus_message_send(conn, msg_props,
                                             _manager_get_prop_cb, -1, cm);

   return cm;
}

static inline void _e_connman_system_name_owner_exit(void)
{
   econnman_mod_manager_inout(NULL);
   _manager_free(connman_manager);
   connman_manager = NULL;

   free(bus_owner);
   bus_owner = NULL;

   ecore_event_add(E_CONNMAN_EVENT_MANAGER_OUT, NULL, NULL, NULL);
}

static inline void _e_connman_system_name_owner_enter(const char *owner)
{
   bus_owner = strdup(owner);
   connman_manager = _manager_new();
   ecore_event_add(E_CONNMAN_EVENT_MANAGER_IN, NULL, NULL, NULL);
   econnman_mod_manager_inout(connman_manager);
}

static void _e_connman_system_name_owner_changed(void *data __UNUSED__,
                                                 DBusMessage *msg)
{
   const char *name, *from, *to;
   DBusError err;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &name,
                              DBUS_TYPE_STRING, &from,
                              DBUS_TYPE_STRING, &to,
                              DBUS_TYPE_INVALID))
     {
        ERR("could not get NameOwnerChanged arguments: %s: %s",
            err.name, err.message);
        dbus_error_free(&err);
        return;
     }

   if (strcmp(name, CONNMAN_BUS_NAME) != 0)
     return;

   DBG("NameOwnerChanged %s from=[%s] to=[%s]", name, from, to);

   if (from[0] == '\0' && to[0] != '\0')
     _e_connman_system_name_owner_enter(to);
   else if (from[0] != '\0' && to[0] == '\0')
     _e_connman_system_name_owner_exit();
   else
     ERR("unknow change from %s to %s", from, to);
}

static void
_e_connman_get_name_owner(void *data __UNUSED__, DBusMessage *msg, DBusError *err)
{
   const char *owner;

   pending_get_name_owner = NULL;

   /* Do nothing if already received a signal */
   if (bus_owner)
     return;

   DBG("get_name_owner msg=%p", msg);

   if (dbus_error_is_set(err))
     {
        if (strcmp(err->name, DBUS_ERROR_NAME_HAS_NO_OWNER) != 0)
          ERR("could not get bus name owner: %s %s", err->name, err->message);
        return;
     }

   if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &owner,
                              DBUS_TYPE_INVALID))
     {
        ERR("Could not get name owner");
        return;
     }

   _e_connman_system_name_owner_enter(owner);
}

/**
 * Initialize E Connection Manager (E_Connman) system.
 *
 * This will connect to ConnMan through DBus and watch for it going in and out.
 *
 * Interesting events are:
 *   - E_CONNMAN_EVENT_MANAGER_IN: issued when connman is avaiable.
 *   - E_CONNMAN_EVENT_MANAGER_OUT: issued when connman connection is lost.
 */
unsigned int
e_connman_system_init(E_DBus_Connection *edbus_conn)
{
   init_count++;

   if (init_count > 1)
      return init_count;

   E_CONNMAN_EVENT_MANAGER_IN = ecore_event_type_new();
   E_CONNMAN_EVENT_MANAGER_OUT = ecore_event_type_new();

   conn = edbus_conn;
   handler_name_owner = e_dbus_signal_handler_add(conn,
                        E_DBUS_FDO_BUS, E_DBUS_FDO_PATH, E_DBUS_FDO_INTERFACE,
                        "NameOwnerChanged", _e_connman_system_name_owner_changed,
                        NULL);
   pending_get_name_owner = e_dbus_get_name_owner(conn,
                                 CONNMAN_BUS_NAME, _e_connman_get_name_owner,
                                 NULL);

   econnman_agent_init(edbus_conn);

   return init_count;
}

/**
 * Shutdown ConnMan system
 *
 * When count drops to 0 resources will be released and no calls should be
 * made anymore.
 */
unsigned int
e_connman_system_shutdown(void)
{
   if (init_count == 0)
     {
        ERR("connman system already shut down.");
        return 0;
     }

   init_count--;
   if (init_count > 0)
      return init_count;

   e_dbus_signal_handler_del(conn, handler_name_owner);
   if (pending_get_name_owner)
     dbus_pending_call_cancel(pending_get_name_owner);

   econnman_agent_shutdown();
   conn = NULL;

   E_CONNMAN_EVENT_MANAGER_OUT = 0;
   E_CONNMAN_EVENT_MANAGER_IN = 0;

   return init_count;
}

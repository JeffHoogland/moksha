#include "e.h"
#include "e_mod_main.h"

/* TODO check if trackerd is running */

typedef struct _Inst Inst;

struct _Inst
{
  E_DBus_Connection *conn;
};

static int  _fetch(const char *input);
static int  _action(Evry_Item *it, const char *input);
static void _cleanup(void);
static void _item_add(char *file, char *service, char *mime, int prio);
static void _item_icon_get(Evry_Item *it, Evas *e);
static void _dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error);

static Evry_Plugin *p;
static Inst *inst;

EAPI int
evry_plug_tracker_init(void)
{
   E_DBus_Connection *conn = e_dbus_bus_get(DBUS_BUS_SESSION);

   if (!conn) return 0;

   p = E_NEW(Evry_Plugin, 1);
   p->name = "Search Files";
   p->type_in = "NONE";
   p->type_out = "FILE";
   p->need_query = 1;
   p->prio = 3;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   
   inst = E_NEW(Inst, 1);
   inst->conn = conn;   
   
   evry_plugin_register(p);

   return 1;
}

EAPI int
evry_plug_tracker_shutdown(void)
{
   evry_plugin_unregister(p);

   if (inst)
     {
	if (inst->conn)
	  e_dbus_connection_close(inst->conn);
	E_FREE(inst);
     }
   
   if (p) E_FREE(p);
   
   return 1;
}

static int
_action(Evry_Item *it, const char *input)
{
   return 0;
}

static void
_cleanup(void)
{
   Evry_Item *it;
   
   EINA_LIST_FREE(p->items, it)
     {
	if (it->mime) eina_stringshare_del(it->mime);
	if (it->uri) eina_stringshare_del(it->uri);
	if (it->label) eina_stringshare_del(it->label);
	free(it);
     }
}

static int
_fetch(const char *input)
{
   Eina_List *list;
   DBusMessage *msg;
   DBusMessageIter iter;
   int live_query_id = 0;
   int offset = 0;
   int max_hits = 50;
   char *service = "Files";
   char *match;
   
   _cleanup(); 

   match = malloc(sizeof(char) * strlen(input) + 2);
   sprintf(match, "%s*", input);

   msg = dbus_message_new_method_call("org.freedesktop.Tracker",
				      "/org/freedesktop/Tracker/Search",
				      "org.freedesktop.Tracker.Search",
				      "TextDetailed");

   dbus_message_iter_init_append(msg, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,  &live_query_id);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &service);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &match);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,  &offset);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32,  &max_hits);
   e_dbus_message_send(inst->conn, msg, _dbus_cb_reply, -1, NULL);
   dbus_message_unref(msg);

   free(match);

   return 0;
}

static void
_item_icon_get(Evry_Item *it, Evas *e)
{
   char *item_path;

   if (!strcmp(it->mime, "Folder"))
     {
	it->o_icon = edje_object_add(e);
	/* e_util_icon_theme_set(it->o_icon, "folder"); */
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/folder");
     }
   else
     {
	item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 32);

	if (item_path)
	  it->o_icon = e_util_icon_add(item_path, e);
	else
	  {
	     it->o_icon = edje_object_add(e);
	     /* e_util_icon_theme_set(it->o_icon, "file"); */
	     e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/fileman/file");
	  }
     }
}

static void
_item_add(char *file, char *service, char *mime, int prio)
{
   Evry_Item *it;   
   
   it = E_NEW(Evry_Item, 1);
   it->priority = prio;
   it->label = eina_stringshare_add(ecore_file_file_get(file));
   it->uri = eina_stringshare_add(file);
   it->mime = eina_stringshare_add(mime);
   it->o_icon = NULL;

   p->items = eina_list_append(p->items, it);
}

static void
_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter array, iter, item;
   char *val;
   
   if (dbus_error_is_set(error))
     {
	printf("Error: %s - %s\n", error->name, error->message);
	return;
     }

   dbus_message_iter_init(msg, &array);
   if(dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
     {
	dbus_message_iter_recurse(&array, &item);
	while(dbus_message_iter_get_arg_type(&item) == DBUS_TYPE_ARRAY)
	  {
	     char *uri;
	     char *service;
	     char *mime;
	     
	     dbus_message_iter_recurse(&item, &iter);
	     
	     if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING)
	       {
		  dbus_message_iter_get_basic(&iter, &uri);
		  dbus_message_iter_next(&iter);
		  dbus_message_iter_get_basic(&iter, &service);
		  dbus_message_iter_next(&iter);
		  dbus_message_iter_get_basic(&iter, &mime);

		  if (uri && service && mime)
		    {
		       _item_add(uri, service, mime, 1); 
		    }
	       }
	     
	     dbus_message_iter_next(&item);
	  }
     }
   
   if (p->items) evry_plugin_async_update(p, 1); 
}


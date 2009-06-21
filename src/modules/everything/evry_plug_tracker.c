#include "e.h"
#include "e_mod_main.h"

/* TODO check if trackerd is running */

typedef struct _Inst Inst;

struct _Inst
{
  E_DBus_Connection *conn;
};

static Evry_Plugin *_plug_tracker_new();
static void _plug_tracker_free(Evry_Plugin *p);
static int  _plug_tracker_fetch(Evry_Plugin *p, char *string);
static int  _plug_tracker_action(Evry_Plugin *p, Evry_Item *item);
static void _plug_tracker_cleanup(Evry_Plugin *p);
static void _plug_tracker_item_add(Evry_Plugin *p, char *file, char *service, char *mime, int prio);
static void _plug_tracker_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static void _plug_tracker_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error);

static Evry_Plugin_Class class;

EAPI int
evry_plug_tracker_init(void)
{
   class.name = "Search Files";
   class.type_in = "NONE";
   class.type_out = "FILE";
   class.need_query = 1;
   class.new = &_plug_tracker_new;
   class.free = &_plug_tracker_free;
   evry_plugin_register(&class);
   
   return 1;
}

EAPI int
evry_plug_tracker_shutdown(void)
{
   evry_plugin_unregister(&class);
   
   return 1;
}

static Evry_Plugin *
_plug_tracker_new()
{
   Evry_Plugin *p;
   Inst *inst;
   E_DBus_Connection *conn = e_dbus_bus_get(DBUS_BUS_SESSION);

   if (!conn) return NULL;
   
   p = E_NEW(Evry_Plugin, 1);
   p->class = &class;
   p->fetch = &_plug_tracker_fetch;
   p->action = &_plug_tracker_action;
   p->cleanup = &_plug_tracker_cleanup;
   p->icon_get = &_plug_tracker_item_icon_get;
   p->items = NULL;   

   inst = E_NEW(Inst, 1);
   inst->conn = conn;
   p->priv = inst;

   return p;
}

static void
_plug_tracker_free(Evry_Plugin *p)
{
   Inst *inst = p->priv;
   
   _plug_tracker_cleanup(p);
   e_dbus_connection_close(inst->conn);

   E_FREE(inst);
   E_FREE(p);
}

static int
_plug_tracker_action(Evry_Plugin *p, Evry_Item *it)
{
   return 0;
}

static void
_plug_tracker_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   
   EINA_LIST_FREE(p->items, it)
     {
	if (it->mime) eina_stringshare_del(it->mime);
	if (it->uri) eina_stringshare_del(it->uri);
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	free(it);
     }
}

static int
_plug_tracker_fetch(Evry_Plugin *p, char *string)
{
   Eina_List *list;
   DBusMessage *msg;
   DBusMessageIter iter;
   int live_query_id = 0;
   int offset = 0;
   int max_hits = 50;
   char *service = "Files";
   char *match;
   Inst *inst = p->priv;
   
   _plug_tracker_cleanup(p); 

   match = malloc(sizeof(char) * strlen(string) + 2);
   sprintf(match, "%s*", string);

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
   e_dbus_message_send(inst->conn, msg, _plug_tracker_dbus_cb_reply, -1, p);
   dbus_message_unref(msg);

   free(match);

   return 0;
}

static void
_plug_tracker_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   char *item_path;

   if (!strcmp(it->mime, "Folder"))
     {
	it->o_icon = edje_object_add(e);
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman",
				"e/icons/fileman/folder");
     }
   else
     {
	item_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 32);

	if (item_path)
	  it->o_icon = e_util_icon_add(item_path, e);
     }
}

static void
_plug_tracker_item_add(Evry_Plugin *p, char *file, char *service, char *mime, int prio)
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
_plug_tracker_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter array, iter, item;
   char *val;
   Evry_Plugin *p = data;
   
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
		       _plug_tracker_item_add(p, uri, service, mime, 1); 
		    }
	       }
	     
	     dbus_message_iter_next(&item);
	  }
     }
   
   if (p->items) evry_plugin_async_update(p, 1); 
}


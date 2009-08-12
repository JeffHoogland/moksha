#include "e.h"
#include "e_mod_main.h"

/* TODO check if trackerd is running */

typedef struct _Inst Inst;

struct _Inst
{
  int active;
  char *condition;
  char *service;
  Eina_List *items;
};

static E_DBus_Connection *conn = NULL;

static Evry_Plugin *p1 = NULL;
static Evry_Plugin *p2 = NULL;
static Evry_Plugin *p3 = NULL;
static Evry_Plugin *p4 = NULL;


static int
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   Inst *inst = p->private;

   inst->active = 0;
   
   if (!strcmp(it->plugin->type_out, "APPLICATION"))
     {
	Efreet_Desktop *desktop;
	Eina_List *l;
	const char *mime;
	Evry_App *app = it->data[0];
	char mime_entry[256];
	char rdf_query[32768];
	int len = 0;

	inst->service = "Files";
	if (inst->condition[0]) free (inst->condition);
	inst->condition = "";

	if (!app->desktop || !app->desktop->mime_types)
	  return 1;
	
	rdf_query[0] = '\0';
	strcat(rdf_query, "<rdfq:Condition><rdfq:or>");
	
	EINA_LIST_FOREACH(app->desktop->mime_types, l, mime)
	  {
	     if (!strcmp(mime, "x-directory/normal"))
	       return 0;

	     snprintf(mime_entry, 256,
		      "<rdfq:contains>"
		      "<rdfq:Property name=\"File:Mime\" />"
		      "<rdf:String>%s</rdf:String> "
		      "</rdfq:contains>",
		      mime);

	     strcat(rdf_query, mime_entry);
	     len += 256;
	     if (len > 32000) break;
	  }
	strcat(rdf_query, "</rdfq:or></rdfq:Condition>");

	inst->condition = strdup(rdf_query);
     }

   return 1;
}

static void
_item_add(Evry_Plugin *p, char *file, char *mime, int prio)
{
   Evry_Item *it;   
   const char *filename;
   int folder = (!strcmp(mime, "Folder"));

   /* folders are specifically searched by p2 and p4 ;) */
   if (folder && ((p == p1) || (p == p3))) return;

   filename = ecore_file_file_get(file);

   if (!filename) return;
   
   it = evry_item_new(p, filename);
   it->priority = prio;
   it->uri = eina_stringshare_add(file);

   if (folder)
     {
	it->browseable = EINA_TRUE;
	it->mime = eina_stringshare_add("x-directory/normal");
     }
   else
     it->mime = eina_stringshare_add(mime);
   
   p->items = eina_list_append(p->items, it);
}


static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   Inst *inst = p->private;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->mime) eina_stringshare_del(it->mime);
	if (it->uri) eina_stringshare_del(it->uri);
	evry_item_free(it);
     }
   p->items = NULL;
}

static void
_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter array, iter, item;
   char *uri, *mime, *date;
   Evry_Plugin *p = data;
   Inst *inst = p->private;
   
   if (inst->active) inst->active--;
   if (inst->active) return;
   
   if (dbus_error_is_set(error))
     {
	printf("Error: %s - %s\n", error->name, error->message);
	return;
     }

   /* evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_CLEAR);
    * _cleanup(p);
    *  */
   
   dbus_message_iter_init(msg, &array);
   if(dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_ARRAY)
     {
	dbus_message_iter_recurse(&array, &item);
	while(dbus_message_iter_get_arg_type(&item) == DBUS_TYPE_ARRAY)
	  {	     
	     dbus_message_iter_recurse(&item, &iter);
	     
	     if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING)
	       {
		  dbus_message_iter_get_basic(&iter, &uri);
		  dbus_message_iter_next(&iter);
		  /* dbus_message_iter_get_basic(&iter, &service); */
		  dbus_message_iter_next(&iter);
		  dbus_message_iter_get_basic(&iter, &mime);
		  /* dbus_message_iter_next(&iter);
		   * dbus_message_iter_get_basic(&iter, &date); */
		  /* printf("date: %s\n",date); */
		  
		  if (uri && mime) _item_add(p, uri, mime, 1); 
	       }
	     dbus_message_iter_next(&item);
	  }
     }
   
   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Inst *inst = p->private;
   DBusMessage *msg;
   DBusMessageIter iter;
   int live_query_id = 0;
   int offset = 0;
   int max_hits = 100;
   int sort_descending = 1;
   int sort_by_service = 0;
   char *search_text;
   char *fields[2];
   char *keywords[1];
   char *sort_fields[1];   
   fields[0] = "File:Mime";
   fields[1] = "File:Accessed";
   keywords[0] = "";
   sort_fields[0] = "";
   
   char **_fields = fields;
   char **_keywords = keywords;
   char **_sort_fields = sort_fields;

   _cleanup(p);

   if (!conn) return 0;
   /* if (!input || (strlen(input) < 3)) return 0; */

   if (input && (strlen(input) > 2)) 
     {	
	search_text = malloc(sizeof(char) * strlen(input) + 3);
	sprintf(search_text, "*%s*", input);
     }
   else if (p == p2 || p == p4)
     {
	search_text = "";
     }
   else return 0;

   inst->active++;
   
   msg = dbus_message_new_method_call("org.freedesktop.Tracker",
				      "/org/freedesktop/Tracker/Search",
				      "org.freedesktop.Tracker.Search",
				      "Query");
   dbus_message_append_args(msg,
			    DBUS_TYPE_INT32,   &live_query_id,
			    DBUS_TYPE_STRING,  &inst->service,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING, &_fields, 1,
			    DBUS_TYPE_STRING,  &search_text,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING, &_keywords, 0,
			    DBUS_TYPE_STRING,  &inst->condition,
			    DBUS_TYPE_BOOLEAN, &sort_by_service,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING, &_sort_fields, 0,
			    DBUS_TYPE_BOOLEAN, &sort_descending,
			    DBUS_TYPE_INT32,   &offset,
			    DBUS_TYPE_INT32,   &max_hits,
			    DBUS_TYPE_INVALID);

   e_dbus_message_send(conn, msg, _dbus_cb_reply, -1, p);
   dbus_message_unref(msg);

   if (input && (strlen(input) > 2))
     free(search_text);

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   char *icon_path;
   Evas_Object *o = NULL;
   
   if (it->browseable)
     {
	o = e_icon_add(e); 
	evry_icon_theme_set(o, "folder");
     }
   else
     {
	icon_path = efreet_mime_type_icon_get(it->mime, e_config->icon_theme, 64);

	if (icon_path)
	  {
	     o = e_util_icon_add(icon_path, e);
	     free(icon_path);
	  }
	else
	  {
	     o = e_icon_add(e); 
	     evry_icon_theme_set(o, "none");
	  }
     }
   return o;
}

static Eina_Bool
_init(void)
{
   Inst *inst;
   conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   
   if (!conn) return EINA_FALSE;

   p1 = E_NEW(Evry_Plugin, 1);
   p1->name = "Find Files";
   p1->type = type_subject;
   p1->type_in = "NONE";
   p1->type_out = "FILE";
   p1->async_query = 1;
   p1->fetch = &_fetch;
   p1->cleanup = &_cleanup;
   p1->icon_get = &_item_icon_get;
   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = "Files";
   p1->private = inst;
   evry_plugin_register(p1);

   p2 = E_NEW(Evry_Plugin, 1);
   p2->name = "Folders";
   p2->type = type_subject;
   p2->type_in = "NONE";
   p2->type_out = "FILE";
   p2->async_query = 1;
   p2->fetch = &_fetch;
   p2->cleanup = &_cleanup;
   p2->icon_get = &_item_icon_get;
   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = "Folders";
   p2->private = inst;
   evry_plugin_register(p2);

   p3 = E_NEW(Evry_Plugin, 1);
   p3->name = "Find Files";
   p3->type = type_object;
   p3->type_in = "NONE";
   p3->type_out = "FILE";
   p3->async_query = 1;
   p3->begin = &_begin;
   p3->fetch = &_fetch;
   p3->cleanup = &_cleanup;
   p3->icon_get = &_item_icon_get;
   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = "Files";
   p3->private = inst;
   evry_plugin_register(p3);

   p4 = E_NEW(Evry_Plugin, 1);
   p4->name = "Folders";
   p4->type = type_object;
   p4->type_in = "NONE";
   p4->type_out = "FILE";
   p4->async_query = 1;
   p4->fetch = &_fetch;
   p4->cleanup = &_cleanup;
   p4->icon_get = &_item_icon_get;
   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = "Folders";
   p4->private = inst;
   evry_plugin_register(p4);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   Inst *inst;
   
   if (conn) e_dbus_connection_close(conn);

   if (p1)
     {	
	evry_plugin_unregister(p1);
	inst = p1->private;
	E_FREE(inst);
	E_FREE(p1);
     }

   if (p2)
     {	
	evry_plugin_unregister(p2);
	inst = p2->private;
	E_FREE(inst);
	E_FREE(p2);
     }

   if (p3)
     {
	evry_plugin_unregister(p3);
	inst = p3->private;
	if (inst->condition[0]) free(inst->condition);
	E_FREE(inst);
	E_FREE(p3);
     }

   if (p4)
     {	
	evry_plugin_unregister(p4);
	inst = p4->private;
	E_FREE(inst);
	E_FREE(p4);
     }
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

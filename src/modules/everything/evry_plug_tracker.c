#include "Evry.h"


/* TODO check if trackerd is running and version */

typedef struct _Inst Inst;

struct _Inst
{
  int active;
  char *condition;
  char *service;
  int max_hits;
  const char *input;
  const char *matched;
  Eina_List *items;
};

static E_DBus_Connection *conn = NULL;
static Eina_List *plugins = NULL;
static int _prio = 5;

static int
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   Inst *inst = p->private;

   inst->active = 0;

   if (!strcmp(it->plugin->type_out, "APPLICATION"))
     {
	Eina_List *l;
	const char *mime;
	Evry_App *app = it->data[0];
	char mime_entry[256];
	char rdf_query[32768];
	int len = 0;

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

static Evry_Item *
_item_add(Evry_Plugin *p, char *file, char *mime, int prio)
{
   Evry_Item *it;
   const char *filename;
   int folder = (!strcmp(mime, "Folder"));

   /* folders are specifically searched */
   if (folder && p->begin)
     return NULL;

   filename = ecore_file_file_get(file);

   if (!filename)
     return NULL;

   it = evry_item_new(p, filename, NULL);
   if (!it)
     return NULL;
   it->priority = prio;
   it->uri = eina_stringshare_add(file);

   if (folder)
     {
	it->browseable = EINA_TRUE;
	it->mime = eina_stringshare_add("x-directory/normal");
	it->priority = 1;
     }
   else
     it->mime = eina_stringshare_add(mime);

   return it;
}

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   Inst *inst = p->private;

   inst->active = 0;

   if (inst->input)
     eina_stringshare_del(inst->input);
   inst->input = NULL;

   if (inst->matched)
     eina_stringshare_del(inst->matched);
   inst->matched = NULL;

   EINA_LIST_FREE(inst->items, it)
     evry_item_free(it);

   if (p->items)
     eina_list_free(p->items);
   p->items = NULL;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;

   it1 = data1;
   it2 = data2;

   return (it2->priority - it1->priority);
}

static void
_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter array, iter, item;
   char *uri, *mime, *date;
   Evry_Item *it;
   Eina_List *items = NULL;
   Evry_Plugin *p = data;
   Inst *inst = p->private;

   if (inst->active) inst->active--;

   if (dbus_error_is_set(error))
     {
	_cleanup(p);
	printf("Error: %s - %s\n", error->name, error->message);
	return;
     }

   if (inst->active) return;

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

		  dbus_message_iter_next(&iter);
		  dbus_message_iter_get_basic(&iter, &date);

		  if (uri && mime && date)
		    {
		       it = _item_add(p, uri, mime, atoi(date));
		       if (it)
			 items = eina_list_append(items, it);
		    }
	       }
	     dbus_message_iter_next(&item);
	  }
     }

   if (p->items)
     eina_list_free(p->items);
   p->items = NULL;

   if (items)
     {
	Eina_List *l;

	EINA_LIST_FREE(inst->items, it)
	  evry_item_free(it);

	items = eina_list_sort(items, eina_list_count(items), _cb_sort);
	inst->items = items;

	EINA_LIST_FOREACH(inst->items, l, it)
	  p->items = eina_list_append(p->items, it);

	if (inst->matched)
	  eina_stringshare_del(inst->matched);
	if (inst->input)
	  inst->matched = eina_stringshare_add(inst->input);
	else
	  inst->matched = NULL;
     }
   else if (inst->items && inst->input && inst->matched &&
	    (strlen(inst->input) > strlen(inst->matched)))
     {
	Eina_List *l;

	EINA_LIST_FOREACH(inst->items, l, it)
	  if (evry_fuzzy_match(it->label, (inst->input + strlen(inst->matched))))
	    p->items = eina_list_append(p->items, it);
     }
   else
     {
	EINA_LIST_FREE(inst->items, it)
	  evry_item_free(it);

	if (inst->input)
	  eina_stringshare_del(inst->input);
	inst->input = NULL;

	if (inst->matched)
	  eina_stringshare_del(inst->matched);
	inst->matched = NULL;
     }

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Inst *inst = p->private;
   DBusMessage *msg;
   int live_query_id = 0;
   int max_hits = inst->max_hits;
   int offset = 0;
   int sort_descending = 1;
   int sort_by_service = 0;
   int sort_by_access = 0;
   char *search_text;
   char *fields[2];
   char *keywords[1];
   char *sort_fields[1];
   fields[0] = "File:Mime";
   fields[1] = "File:Accessed";
   keywords[0] = "";
   sort_fields[0] = "File:Accessed";
   char **_fields = fields;
   char **_keywords = keywords;
   char **_sort_fields = sort_fields;

   if (inst->input)
     eina_stringshare_del(inst->input);
   inst->input = NULL;

   if (!conn)
     {
	_cleanup(p);
	return 0;
     }

   if (input && (strlen(input) > 2))
     {
	inst->input = eina_stringshare_add(input);
	search_text = malloc(sizeof(char) * strlen(input) + 1);
	sprintf(search_text, "%s", input);
	max_hits = 50;
     }
   else if (!input && !p->begin && p->type == type_object)
     {
	sort_by_access = 1;
	search_text = "";
     }
   else
     {
	_cleanup(p);
	return 0;
     }

   inst->active++;

   msg = dbus_message_new_method_call("org.freedesktop.Tracker",
				      "/org/freedesktop/Tracker/Search",
				      "org.freedesktop.Tracker.Search",
				      "Query");
   dbus_message_append_args(msg,
			    DBUS_TYPE_INT32,   &live_query_id,
			    DBUS_TYPE_STRING,  &inst->service,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING,
			    &_fields, 2,
			    DBUS_TYPE_STRING,  &search_text,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING,
			    &_keywords, 0,
			    DBUS_TYPE_STRING,  &inst->condition,
			    DBUS_TYPE_BOOLEAN, &sort_by_service,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING,
			    &_sort_fields, sort_by_access,
			    DBUS_TYPE_BOOLEAN, &sort_descending,
			    DBUS_TYPE_INT32,   &offset,
			    DBUS_TYPE_INT32,   &max_hits,
			    DBUS_TYPE_INVALID);

   e_dbus_message_send(conn, msg, _dbus_cb_reply, -1, p);
   dbus_message_unref(msg);

   if (input && (strlen(input) > 2))
     free(search_text);

   if (p->items) return 1;

   return 0;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   if (it->browseable)
     return evry_icon_theme_get("folder", e);
   else
     return evry_icon_mime_get(it->mime, e);

   return NULL;
}

static void
_plugin_new(const char *name, int type, char *service, int max_hits, int begin)
{
   Evry_Plugin *p;
   Inst *inst;

   p = evry_plugin_new(name, type, "", "FILE", 0, NULL, NULL,
		       (begin ? _begin : NULL), _cleanup, _fetch,
		       NULL, NULL, _icon_get, NULL, NULL);

   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = service;
   inst->max_hits = max_hits;
   inst->active = 0;
   p->private = inst;
   evry_plugin_register(p, _prio++);
   plugins = eina_list_append(plugins, p);
}


static Eina_Bool
_init(void)
{
   conn = e_dbus_bus_get(DBUS_BUS_SESSION);

   if (!conn) return EINA_FALSE;

   _plugin_new("Folders",    type_subject, "Folders", 20, 0);
   _plugin_new("Images",     type_subject, "Images", 20, 0);
   _plugin_new("Music",      type_subject, "Music", 20, 0);
   _plugin_new("Videos",     type_subject, "Videos", 20, 0);
   _plugin_new("Documents",  type_subject, "Documents", 20, 0);

   _plugin_new("Find Files", type_object,  "Files", 20, 1);
   _plugin_new("Folders",    type_object,  "Folders", 20, 0);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   Inst *inst;
   Evry_Plugin *p;

   if (conn) e_dbus_connection_close(conn);

   EINA_LIST_FREE(plugins, p)
     {
	evry_plugin_unregister(p);
	inst = p->private;
	if (inst->condition[0]) free(inst->condition);
	E_FREE(inst);
	E_FREE(p);
     }
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

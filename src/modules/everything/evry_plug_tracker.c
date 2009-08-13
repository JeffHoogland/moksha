#include "e.h"
#include "e_mod_main.h"

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

   it = evry_item_new(p, filename);
   if (!it)
     return NULL;
   it->priority = prio;
   it->uri = eina_stringshare_add(file);

   if (folder)
     {
	it->browseable = EINA_TRUE;
	it->mime = eina_stringshare_add("x-directory/normal");
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

   EINA_LIST_FREE(p->items, it)
     {
	if (it->mime) eina_stringshare_del(it->mime);
	if (it->uri) eina_stringshare_del(it->uri);
	evry_item_free(it);
     }
   p->items = NULL;
   inst->items = NULL;
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

   p->items = inst->items;
   
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

   if (items)
     {
	EINA_LIST_FREE(p->items, it)
	  {
	     if (it->mime) eina_stringshare_del(it->mime);
	     if (it->uri) eina_stringshare_del(it->uri);
	     evry_item_free(it);
	  }
	p->items = items;

	if (inst->matched)
	  eina_stringshare_del(inst->matched);
	inst->matched = eina_stringshare_add(inst->input);
     }
   else if (p->items && inst->input)
     {
	char input[128];
	char *pos = input;

	snprintf(input, 128, "%s", inst->input + (strlen(inst->matched) - 1));
	for (; *pos != '\0'; pos++)
	  if (isspace(*pos)) *pos = '*';

	EINA_LIST_FREE(p->items, it)
	  {
	     if (e_util_glob_case_match(it->label, input))
	       items = eina_list_append(items, it);
	     else
	       {
		  if (it->mime) eina_stringshare_del(it->mime);
		  if (it->uri) eina_stringshare_del(it->uri);
		  evry_item_free(it);
	       }
	  }

	p->items = items;
     }
   else
     {
	EINA_LIST_FREE(p->items, it)
	  {
	     if (it->mime) eina_stringshare_del(it->mime);
	     if (it->uri) eina_stringshare_del(it->uri);
	     evry_item_free(it);
	  }
	p->items = NULL;

	if (inst->input)
	  eina_stringshare_del(inst->input);
	inst->input = NULL;

	if (inst->matched)
	  eina_stringshare_del(inst->matched);
	inst->matched = NULL;
     }

   if (p->items)
     p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);

   inst->items = p->items;
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

   p->items = NULL;

   if (input && (strlen(input) > 2))
     {
	search_text = malloc(sizeof(char) * strlen(input) + 3);
	sprintf(search_text, "*%s*", input);
	max_hits = 50;
	inst->input = eina_stringshare_add(search_text);
     }
   else if (!input && !p->begin && p->type == type_object)
     {
	sort_by_access = 1;
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

static void
_plugin_new(const char *name, int type, char *service, int max_hits, int begin)
{
   Evry_Plugin *p;
   Inst *inst;

   p = E_NEW(Evry_Plugin, 1);
   p->name = name;
   p->type = type;
   p->type_in = "NONE";
   p->type_out = "FILE";
   p->async_query = 1;
   if (begin)
     p->begin = &_begin;
   p->fetch = &_fetch;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   inst = E_NEW(Inst, 1);
   inst->condition = "";
   inst->service = service;
   inst->max_hits = max_hits;
   inst->active = 0;
   p->private = inst;
   evry_plugin_register(p);
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
   /* _plugin_new("Text",       type_subject, "TextFiles", 20, 0); */

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

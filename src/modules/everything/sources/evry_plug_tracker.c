#include "Evry.h"


/* TODO check if trackerd is running and version */

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;

  int active;
  char *condition;
  char *service;
  int max_hits;
  const char *input;
  const char *matched;
  Eina_List *files;
};

static E_DBus_Connection *conn = NULL;
static Eina_List *plugins = NULL;
static int _prio = 5;


static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   PLUGIN(p, plugin);

   p->active = 0;

   /* is APPLICATION ? */
   if (it && it->plugin->type_out == plugin->type_in)
     {
	ITEM_APP(app, it);
	Eina_List *l;
	const char *mime;
	char mime_entry[256];
	char rdf_query[32768];
	int len = 0;

	if (p->condition[0]) free (p->condition);
	p->condition = "";

	if (!app->desktop || !app->desktop->mime_types)
	  return NULL;

	rdf_query[0] = '\0';
	strcat(rdf_query, "<rdfq:Condition><rdfq:or>");

	EINA_LIST_FOREACH(app->desktop->mime_types, l, mime)
	  {
	     if (!strcmp(mime, "x-directory/normal"))
	       return NULL;

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

	p->condition = strdup(rdf_query);
     }

   return plugin;
}

static void
_item_free(Evry_Item *it)
{
   ITEM_FILE(file, it);
   if (file->uri) eina_stringshare_del(file->uri);
   if (file->mime) eina_stringshare_del(file->mime);

   E_FREE(file);
}

static Evry_Item_File *
_item_add(Plugin *p, char *path, char *mime, int prio)
{
   Evry_Item_File *file;

   const char *filename;
   int folder = (!strcmp(mime, "Folder"));

   /* folders are specifically searched */
   if (folder && EVRY_PLUGIN(p)->begin)
     return NULL;

   filename = ecore_file_file_get(path);

   if (!filename)
     return NULL;

   file = E_NEW(Evry_Item_File, 1);
   if (!file) return NULL;

   evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), filename, _item_free);
   file->uri = eina_stringshare_add(path);

   /* EVRY_ITEM(file)->priority = prio; */

   if (folder)
     {
	EVRY_ITEM(file)->browseable = EINA_TRUE;
	EVRY_ITEM(file)->priority = 1;
	file->mime = eina_stringshare_add("x-directory/normal");
     }
   else
     file->mime = eina_stringshare_add(mime);

   return file;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);
   Evry_Item_File *file;

   p->active = 0;

   if (p->input)
     eina_stringshare_del(p->input);
   p->input = NULL;

   if (p->matched)
     eina_stringshare_del(p->matched);
   p->matched = NULL;

   EINA_LIST_FREE(p->files, file)
     evry_item_free(EVRY_ITEM(file));

   EVRY_PLUGIN_ITEMS_CLEAR(p);
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;

   it1 = data1;
   it2 = data2;

   if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);

   return strcasecmp(it1->label, it2->label);
}

static void
_dbus_cb_reply(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter array, iter, item;
   char *uri, *mime, *date;
   Evry_Item_File *file;
   Eina_List *files = NULL;
   Plugin *p = data;

   if (p->active) p->active--;

   if (dbus_error_is_set(error))
     {
	_cleanup(EVRY_PLUGIN(p));
	printf("Error: %s - %s\n", error->name, error->message);
	return;
     }

   if (p->active) return;

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
		       file = _item_add(p, uri, mime, atoi(date));
		       if (file) files = eina_list_append(files, file);
		    }
	       }
	     dbus_message_iter_next(&item);
	  }
     }

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (files)
     {
	Eina_List *l;

	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));

	files = eina_list_sort(files, eina_list_count(files), _cb_sort);
	p->files = files;

	EINA_LIST_FOREACH(p->files, l, file)
	  EVRY_PLUGIN_ITEM_APPEND(p, file);

	if (p->matched)
	  eina_stringshare_del(p->matched);
	if (p->input)
	  p->matched = eina_stringshare_add(p->input);
	else
	  p->matched = NULL;
     }
   else if (p->files && p->input && p->matched &&
	    (strlen(p->input) > strlen(p->matched)))
     {
	Eina_List *l;

	EINA_LIST_FOREACH(p->files, l, file)
	  if (evry_fuzzy_match(EVRY_ITEM(file)->label, (p->input + strlen(p->matched))))
	    EVRY_PLUGIN_ITEM_APPEND(p, file);
     }
   else
     {
	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));

	if (p->input)
	  eina_stringshare_del(p->input);
	p->input = NULL;

	if (p->matched)
	  eina_stringshare_del(p->matched);
	p->matched = NULL;
     }

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);

   DBusMessage *msg;
   int live_query_id = 0;
   int max_hits = p->max_hits;
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

   if (p->input)
     eina_stringshare_del(p->input);
   p->input = NULL;

   if (!conn)
     {
	_cleanup(plugin);
	return 0;
     }

   if (input && (strlen(input) > 2))
     {
	p->input = eina_stringshare_add(input);
	search_text = malloc(sizeof(char) * strlen(input) + 1);
	sprintf(search_text, "%s", input);
	max_hits = 100;
     }
   else if (!input && !plugin->begin && plugin->type == type_object)
     {
	sort_by_access = 1;
	search_text = "";
     }
   else
     {
	_cleanup(plugin);
	return 0;
     }

   p->active++;

   msg = dbus_message_new_method_call("org.freedesktop.Tracker",
				      "/org/freedesktop/Tracker/Search",
				      "org.freedesktop.Tracker.Search",
				      "Query");
   dbus_message_append_args(msg,
			    DBUS_TYPE_INT32,   &live_query_id,
			    DBUS_TYPE_STRING,  &p->service,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING,
			    &_fields, 2,
			    DBUS_TYPE_STRING,  &search_text,
			    DBUS_TYPE_ARRAY,   DBUS_TYPE_STRING,
			    &_keywords, 0,
			    DBUS_TYPE_STRING,  &p->condition,
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

   if (p->files) return 1;

   return 0;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   ITEM_FILE(file, it);

   if (it->browseable)
     return evry_icon_theme_get("folder", e);
   else
     return evry_icon_mime_get(file->mime, e);

   return NULL;
}

static void
_plugin_new(const char *name, int type, char *service, int max_hits, int begin)
{
   Plugin *p;

   p = E_NEW(Plugin, 1);
   p->condition = "";
   p->service = service;
   p->max_hits = max_hits;
   p->active = 0;

   if (!begin)
     evry_plugin_new(EVRY_PLUGIN(p), name, type, "", "FILE", 1, NULL, NULL,
		     NULL, _cleanup, _fetch,
		     NULL, _icon_get, NULL, NULL);
   else if (type == type_object)
     evry_plugin_new(EVRY_PLUGIN(p), name, type, "APPLICATION", "FILE", 1, NULL, NULL,
		   _begin, _cleanup, _fetch,
		   NULL, _icon_get, NULL, NULL);
   
   plugins = eina_list_append(plugins, p);

   evry_plugin_register(EVRY_PLUGIN(p), _prio++);
}

static void
_dbus_cb_version(void *data, DBusMessage *msg, DBusError *error)
{
   DBusMessageIter iter;
   Plugin *p;
   int version = 0;

   if (dbus_error_is_set(error))
     {
	printf("Error: %s - %s\n", error->name, error->message);
	e_dbus_connection_close(conn);

	EINA_LIST_FREE(plugins, p)
	  {
	     if (p->condition[0]) free(p->condition);

	     EVRY_PLUGIN_FREE(p);
	  }
	return;
     }

   dbus_message_iter_init(msg, &iter);

   if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32)
     dbus_message_iter_get_basic(&iter, &version);

   printf("tracker version %d\n", version);

   if (version < 690)
     {
	e_dbus_connection_close(conn);

	EINA_LIST_FREE(plugins, p)
	  {
	     if (p->condition[0]) free(p->condition);

	     EVRY_PLUGIN_FREE(p);
	  }
     }
}


static void
_get_version(void)
{
   DBusMessage *msg;

   msg = dbus_message_new_method_call("org.freedesktop.Tracker",
				      "/org/freedesktop/Tracker",
				      "org.freedesktop.Tracker",
				      "GetVersion");

   e_dbus_message_send(conn, msg, _dbus_cb_version, -1, NULL);
   dbus_message_unref(msg);
}


/* static Evry_Plugin *
 * _begin_subject(Evry_Plugin *plugin, const Evry_Item *it)
 * {
 *    if (!conn) return NULL;
 *
 *    return plugin;
 * } */


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
   
   _get_version();

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   Plugin *p;

   if (conn) e_dbus_connection_close(conn);

   EINA_LIST_FREE(plugins, p)
     {
	if (p->condition[0]) free(p->condition);

	EVRY_PLUGIN_FREE(p);
     }
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

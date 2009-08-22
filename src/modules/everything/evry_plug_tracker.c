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
  Eina_List *items;
};

static E_DBus_Connection *conn = NULL;
static Eina_List *plugins = NULL;
static int _prio = 5;

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = (Plugin*) plugin;

   p->active = 0;

   /* is APPLICATION ? */
   if (it && it->plugin->type_out == plugin->type_in)
     {
	Eina_List *l;
	const char *mime;
	Evry_App *app = it->data[0];
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

static Evry_Item *
_item_add(Plugin *p, char *file, char *mime, int prio)
{
   Evry_Item *it;
   const char *filename;
   int folder = (!strcmp(mime, "Folder"));

   /* folders are specifically searched */
   if (folder && p->base.begin)
     return NULL;

   filename = ecore_file_file_get(file);

   if (!filename)
     return NULL;

   it = evry_item_new(&p->base, filename, NULL);
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
_cleanup(Evry_Plugin *plugin)
{
   Plugin *p = (Plugin*) plugin;
   Evry_Item *it;

   p->active = 0;

   if (p->input)
     eina_stringshare_del(p->input);
   p->input = NULL;

   if (p->matched)
     eina_stringshare_del(p->matched);
   p->matched = NULL;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   if (p->base.items)
     eina_list_free(p->base.items);
   p->base.items = NULL;
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
   Plugin *p = data;

   if (p->active) p->active--;

   if (dbus_error_is_set(error))
     {
	_cleanup(&p->base);
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
		       it = _item_add(p, uri, mime, atoi(date));
		       if (it)
			 items = eina_list_append(items, it);
		    }
	       }
	     dbus_message_iter_next(&item);
	  }
     }

   if (p->base.items)
     eina_list_free(p->base.items);
   p->base.items = NULL;

   if (items)
     {
	Eina_List *l;

	EINA_LIST_FREE(p->items, it)
	  evry_item_free(it);

	items = eina_list_sort(items, eina_list_count(items), _cb_sort);
	p->items = items;

	EINA_LIST_FOREACH(p->items, l, it)
	  p->base.items = eina_list_append(p->base.items, it);

	if (p->matched)
	  eina_stringshare_del(p->matched);
	if (p->input)
	  p->matched = eina_stringshare_add(p->input);
	else
	  p->matched = NULL;
     }
   else if (p->items && p->input && p->matched &&
	    (strlen(p->input) > strlen(p->matched)))
     {
	Eina_List *l;

	EINA_LIST_FOREACH(p->items, l, it)
	  if (evry_fuzzy_match(it->label, (p->input + strlen(p->matched))))
	    p->base.items = eina_list_append(p->base.items, it);
     }
   else
     {
	EINA_LIST_FREE(p->items, it)
	  evry_item_free(it);

	if (p->input)
	  eina_stringshare_del(p->input);
	p->input = NULL;

	if (p->matched)
	  eina_stringshare_del(p->matched);
	p->matched = NULL;
     }

   evry_plugin_async_update(&p->base, EVRY_ASYNC_UPDATE_ADD);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   Plugin *p = (Plugin*) plugin;
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
   Plugin *p;

   p = E_NEW(Plugin, 1);
   p->condition = "";
   p->service = service;
   p->max_hits = max_hits;
   p->active = 0;

   if (!begin)
     evry_plugin_new(&p->base, name, type, "", "FILE", 0, NULL, NULL,
		     NULL, _cleanup, _fetch,
		     NULL, _icon_get, NULL, NULL);
   else
     evry_plugin_new(&p->base, name, type, "APPLICATION", "FILE", 0, NULL, NULL,
		   _begin, _cleanup, _fetch,
		   NULL, _icon_get, NULL, NULL);

   plugins = eina_list_append(plugins, p);

   evry_plugin_register(&p->base, _prio++);
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
   Plugin *p;

   if (conn) e_dbus_connection_close(conn);

   EINA_LIST_FREE(plugins, p)
     {
	evry_plugin_free(&p->base, 0);

	if (p->condition[0]) free(p->condition);

	E_FREE(p);
     }
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);

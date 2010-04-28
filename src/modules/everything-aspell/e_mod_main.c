/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"
#include <ctype.h>

static const char TRIGGER[] = "aspell ";
static const char LANG_MODIFIER[] = "lang=";

typedef struct _Plugin Plugin;


struct _Plugin
{
  Evry_Plugin base;
  struct
  {
    Ecore_Event_Handler *data;
    Ecore_Event_Handler *del;
  } handler;
  Ecore_Exe *exe;
  const char *lang;
  const char *input;
  Eina_Bool is_first;
};

static Plugin *_plug = NULL;

static Eina_Bool
_exe_restart(Plugin *p)
{
   char cmd[1024];
   const char *lang_opt, *lang_val;
   int len;

   if (p->lang && (p->lang[0] != '\0'))
     {
	lang_opt = "-l";
	lang_val = p->lang;
     }
   else
     {
	lang_opt = "";
	lang_val = "";
     }

   len = snprintf(cmd, sizeof(cmd),
		  "aspell -a --encoding=UTF-8 %s%s",
		  lang_opt, lang_val);
   if (len >= (int)sizeof(cmd))
     return 0;

   if (p->exe)
     ecore_exe_quit(p->exe);
   p->exe = ecore_exe_pipe_run
     (cmd,
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED |
      ECORE_EXE_PIPE_WRITE,
      NULL);
   p->is_first = 1;
   return !!p->exe;
}

static const char *
_space_skip(const char *line)
{
   for (; *line != '\0'; line++)
     if (!isspace(*line))
       break;
   return line;
}

static const char *
_space_find(const char *line)
{
   for (; *line != '\0'; line++)
     if (isspace(*line))
       break;
   return line;
}

static void
_item_add(Plugin *p, const char *word, int word_size, int prio)
{
   Evry_Item *it;

   it = EVRY_ITEM_NEW(Evry_Item, p, NULL, NULL, NULL);
   if (!it) return;
   it->priority = prio;
   it->label = eina_stringshare_add_length(word, word_size);

   EVRY_PLUGIN_ITEM_APPEND(p, it);
}

static void
_suggestions_add(Plugin *p, const char *line)
{
   const char *s;

   s = strchr(line, ':');
   if (!s)
     {
	fprintf(stderr, "ASPELL: ERROR missing suggestion delimiter: '%s'\n",
		line);
	return;
     }
   s++;

   line = _space_skip(s);
   while (*line)
     {
	int len;

	s = strchr(line, ',');
	if (s)
	  len = s - line;
	else
	  len = strlen(line);

	_item_add(p, line, len, 1);

	if (s)
	  line = _space_skip(s + 1);
	else
	  break;
     }
}

static int
_cb_data(void *data, int type __UNUSED__, void *event)
{
   GET_PLUGIN(p, data);
   Ecore_Exe_Event_Data *e = event;
   Ecore_Exe_Event_Data_Line *l;
   const char *word;

   if (e->exe != p->exe)
     return 1;

   EVRY_PLUGIN_ITEMS_FREE(p);

   word = p->input;
   for (l = e->lines; l && l->line; l++)
     {
	const char *word_end;
	int word_size;

	if (p->is_first)
	  {
	     fprintf(stderr, "ASPELL: %s\n", l->line);
	     p->is_first = 0;
	     continue;
	  }

	word_end = _space_find(word);
	word_size = word_end - word;

	switch (l->line[0])
	  {
	   case '*':
	      _item_add(p, word, word_size, 1);
	      break;
	   case '&':
	      _item_add(p, word, word_size, 1);
	      _suggestions_add(p, l->line);
	      break;
	   case '#':
	      break;
	   case '\0':
	      break;
	   default:
	      fprintf(stderr, "ASPELL: unknown output: '%s'\n", l->line);
	  }

	if (*word_end)
	  word = _space_skip(word_end + 1);
     }

   if (EVRY_PLUGIN(p)->items)
     {
	evry_list_win_show();
     }

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);

   return 1;
}

static int
_cb_del(void *data, int type __UNUSED__, void *event)
{
   Plugin *p = data;
   Ecore_Exe_Event_Del *e = event;

   if (e->exe != p->exe)
     return 1;

   p->exe = NULL;
   return 1;
}

static int
_begin(Evry_Plugin *plugin, const Evry_Item *it __UNUSED__)
{
   GET_PLUGIN(p, plugin);

   if (!p->handler.data)
     p->handler.data = ecore_event_handler_add
       (ECORE_EXE_EVENT_DATA, _cb_data, p);
   if (!p->handler.del)
     p->handler.del = ecore_event_handler_add
       (ECORE_EXE_EVENT_DEL, _cb_del, p);

   return _exe_restart(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   const char *s;
   int len;

   if (!input) return 0;

   if (!p->handler.data && !_begin(plugin, NULL)) return 0;

   len = sizeof(LANG_MODIFIER) - 1;
   if (strncmp(input, LANG_MODIFIER, len) == 0)
     {
	const char *lang;

	EVRY_PLUGIN_ITEMS_FREE(p);

	input += len;
	for (s = input; *s != '\0'; s++)
	  if (isspace(*s) || *s == ';')
	    break;

	if (*s == '\0') /* just apply language on ' ' or ';' */
	  return 1;

	if (s - input > 0)
	  lang = eina_stringshare_add_length(input, s - input);
	else
	  lang = NULL;

	if (p->lang) eina_stringshare_del(p->lang);
	if (p->lang != lang)
	  {
	     p->lang = lang;
	     if (!_exe_restart(p))
	       return 0;
	  }

	if (*s == '\0')
	  return 1;

	input = s + 1;
     }

   input = _space_skip(input);
   for (s = input; *s != '\0'; s++)
     ;
   for (s--; s > input; s--)
     if (!isspace(*s))
       break;

   len = s - input + 1;
   if (len < 1)
     return 1;
   input = eina_stringshare_add_length(input, len);
   if (p->input) eina_stringshare_del(p->input);
   if (p->input == input)
     return 1;

   p->input = input;
   if (!p->exe)
     return 1;

   ecore_exe_send(p->exe, (char *)p->input, len);
   ecore_exe_send(p->exe, "\n", 1);

   return 1;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_FREE(p);

   if (p->handler.data)
     {
	ecore_event_handler_del(p->handler.data);
	p->handler.data = NULL;
     }
   if (p->handler.del)
     {
	ecore_event_handler_del(p->handler.del);
	p->handler.del = NULL;
     }
   if (p->exe)
     {
	ecore_exe_quit(p->exe);
	ecore_exe_free(p->exe);
	p->exe = NULL;
     }
   if (p->lang)
     {
	eina_stringshare_del(p->lang);
	p->lang = NULL;
     }
   if (p->input)
     {
	eina_stringshare_del(p->input);
	p->input = NULL;
     }
}

static Eina_Bool
_plugins_init(void)
{
   Evry_Plugin *p;

   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p = EVRY_PLUGIN_NEW(Plugin, N_("Spell Checker"), "accessories-dictionary", "TEXT",
		   NULL, _cleanup, _fetch, NULL);

   p->aggregate   = EINA_FALSE;
   p->history     = EINA_FALSE;
   p->async_fetch = EINA_TRUE;
   p->trigger     = TRIGGER;

   evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 100);
   _plug = (Plugin *) p;

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(_plug);
}

/***************************************************************************/

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "everything-aspell"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   module = m;

   if (e_datastore_get("everything_loaded"))
     active = _plugins_init();

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   module = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/***************************************************************************/

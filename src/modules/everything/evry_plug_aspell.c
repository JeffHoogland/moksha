#include "e.h"
#include "e_mod_main.h"
#include <ctype.h>

static const char TRIGGER[] = "aspell ";
static const char LANG_MODIFIER[] = "lang=";
static Ecore_X_Window clipboard_win = 0;

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

static Plugin *_singleton = NULL;

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
_item_add(Evry_Plugin *p, const char *word, int word_size, int prio)
{
   Evry_Item *it;

   it = E_NEW(Evry_Item, 1);
   if (!it) return;
   it->priority = prio;
   it->label = eina_stringshare_add_length(word, word_size);
   p->items = eina_list_append(p->items, it);
}

static void
_item_del(Evry_Item *it)
{
   eina_stringshare_del(it->label);
   E_FREE(it);
}

static void
_suggestions_add(Evry_Plugin *p, const char *line)
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

static void
_clear_list(Evry_Plugin *plugin)
{
   Evry_Item *it;
   evry_plugin_async_update(plugin, EVRY_ASYNC_UPDATE_CLEAR);
   EINA_LIST_FREE(plugin->items, it)
     _item_del(it);
}

static int
_cb_data(void *data, int type __UNUSED__, void *event)
{
   Plugin *p = data;
   Evry_Plugin *plugin = &p->base;
   Ecore_Exe_Event_Data *e = event;
   Ecore_Exe_Event_Data_Line *l;
   const char *word;

   if (e->exe != p->exe)
     return 1;

   _clear_list(plugin);

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
	      _item_add(plugin, word, word_size, 1);
	      break;
	   case '&':
	      _item_add(plugin, word, word_size, 1);
	      _suggestions_add(plugin, l->line);
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

   evry_plugin_async_update(plugin, EVRY_ASYNC_UPDATE_ADD);

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
_begin(Evry_Plugin *plugin, Evry_Item *it __UNUSED__)
{
   Plugin *p = (Plugin *)plugin;

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
   Plugin *p = (Plugin *)plugin;
   const char *s;
   int len;

   len = sizeof(TRIGGER) - 1;
   if (strncmp(input, TRIGGER, len) != 0)
     return 0;

   input += len;

   len = sizeof(LANG_MODIFIER) - 1;
   if (strncmp(input, LANG_MODIFIER, len) == 0)
     {
	const char *lang;

	_clear_list(plugin);

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

	eina_stringshare_del(p->lang);
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
   eina_stringshare_del(p->input);
   if (p->input == input)
     return 1;

   p->input = input;
   if (!p->exe)
     return 1;

   ecore_exe_send(p->exe, (char *)p->input, len);
   ecore_exe_send(p->exe, "\n", 1);

   return 1;
}

static int
_action(Evry_Plugin *plugin, Evry_Item *it, const char *input __UNUSED__)
{
   const char *label;
   int len;

   if (!it) return EVRY_ACTION_CONTINUE;

   label = it->label;
   len = eina_stringshare_strlen(label);

   if (!ecore_x_selection_primary_set(clipboard_win, label, len))
     fprintf(stderr, "ASPELL cannot set primary selection to %#x '%s'\n",
	     clipboard_win, label);
   if (!ecore_x_selection_clipboard_set(clipboard_win, label, len))
     fprintf(stderr, "ASPELL cannot set clipboard selection to %#x '%s'\n",
	     clipboard_win, label);

   if (plugin->items)
     {
	Eina_List *l, *l_next;
	Evry_Item *it2;

	evry_plugin_async_update(plugin, EVRY_ASYNC_UPDATE_CLEAR);

	EINA_LIST_FOREACH_SAFE(plugin->items, l, l_next, it2)
	  {
	     if (it == it2) continue;
	     plugin->items = eina_list_remove_list(plugin->items, l);
	     eina_stringshare_del(it2->label);
	     E_FREE(it2);
	  }
     }

   evry_plugin_async_update(plugin, EVRY_ASYNC_UPDATE_ADD);

   return EVRY_ACTION_FINISHED;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   Plugin *p = (Plugin *)plugin;
   Evry_Item *it;

   EINA_LIST_FREE(p->base.items, it)
     _item_del(it);

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

static void
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e __UNUSED__)
{
   it->o_icon = NULL;
}

EAPI int
evry_plug_aspell_init(void)
{
   Plugin *p;

   if (_singleton)
     return 1;

   p = E_NEW(Plugin, 1);
   p->base.name = "Spell Checker";
   p->base.type_in  = "NONE";
   p->base.type_out = "NONE";
   p->base.trigger = TRIGGER;
   p->base.need_query = 0;
   p->base.async_query = 1;
   p->base.begin = _begin;
   p->base.fetch = _fetch;
   p->base.action = _action;
   p->base.cleanup = _cleanup;
   p->base.icon_get = _item_icon_get;

   if (clipboard_win == 0)
     {
	clipboard_win = ecore_x_window_new(0, 0, 0, 1, 1);
	fprintf(stderr, "ASPELL clipboard_win=%#x\n", clipboard_win);
     }

   evry_plugin_register(&p->base);

   _singleton = p;
   return 1;
}

EAPI int
evry_plug_aspell_shutdown(void)
{
   Evry_Item *it;
   Plugin *p;

   if (!_singleton)
     return 0;

   p = _singleton;
   _singleton = NULL;

   EINA_LIST_FREE(p->base.items, it)
     _item_del(it);

   evry_plugin_unregister(&p->base);
   E_FREE(p);

   return 1;
}

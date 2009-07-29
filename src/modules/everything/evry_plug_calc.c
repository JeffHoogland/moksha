#include "e.h"
#include "e_mod_main.h"

/* TODO
 * - dc support?
 */

static int  _begin(Evry_Plugin *p, Evry_Item *it);
static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _cleanup(Evry_Plugin *p);
static void _item_add(Evry_Plugin *p, char *output, int prio);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);
static int  _cb_data(void *data, int type, void *event);
static int  _cb_error(void *data, int type, void *event);
static int  _cb_del(void *data, int type, void *event);

static Eina_Bool _init(void);
static void _shutdown(void);
EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);


static Evry_Plugin *p;
static Ecore_Exe *exe = NULL;
static Eina_List *history = NULL;
static Ecore_Event_Handler *data_handler = NULL;
static Ecore_Event_Handler *error_handler = NULL;
static Ecore_Event_Handler *del_handler = NULL;
static Ecore_X_Window clipboard_win = 0;
static int error = 0;

static Eina_Bool
_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->name = "Calculator";
   p->type_in  = "NONE";
   p->type_out = "NONE";
   p->trigger = "=";
   p->need_query = 0;
   p->async_query = 1;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   evry_plugin_register(p);

   clipboard_win = ecore_x_window_new(0, 0, 0, 1, 1);
   return EINA_TRUE;
}

static void
_shutdown(void)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	free(it);
     }

   evry_plugin_unregister(p);
   E_FREE(p);

   ecore_x_window_free(clipboard_win);
}

static int
_begin(Evry_Plugin *p, Evry_Item *it __UNUSED__)
{

   data_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _cb_data, p);
   error_handler = ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _cb_error, p);
   del_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _cb_del, p);
   exe = ecore_exe_pipe_run("bc -l",
			    ECORE_EXE_PIPE_READ |
			    ECORE_EXE_PIPE_READ_LINE_BUFFERED |
			    ECORE_EXE_PIPE_WRITE |
			    ECORE_EXE_PIPE_ERROR |
			    ECORE_EXE_PIPE_ERROR_LINE_BUFFERED,
			    NULL);
   return !!exe;
}

static void
_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   int i = 0;

   EINA_LIST_FREE(p->items, it)
     {
	if (i < 10)
	  {
	     history = eina_list_append(history, it);
	  }
	else
	  {
	     if (it->label) eina_stringshare_del(it->label);
	     free(it);
	  }
     }

   ecore_event_handler_del(data_handler);
   ecore_event_handler_del(error_handler);
   ecore_event_handler_del(del_handler);
   data_handler = NULL;

   ecore_exe_quit(exe);
   exe = NULL;
}

static int
_action(Evry_Plugin *p, Evry_Item *it, const char *input __UNUSED__)
{
   if (!it) return EVRY_ACTION_CONTINUE;

   if (p->items)
     {
	Eina_List *l;
	Evry_Item *it2;

	evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_CLEAR);

	/* remove duplicates */
	if (p->items->next)
	  {
	     it = p->items->data;
		  
	     EINA_LIST_FOREACH(p->items->next, l, it2)
	       {
		  if (!strcmp(it->label, it2->label))
		    break;
		  it2 = NULL;
	       }

	     if (it2)
	       {
		  p->items = eina_list_remove(p->items, it2);
		  eina_stringshare_del(it2->label);
		  E_FREE(it2);
	       }
	  }

	it = p->items->data;

	_item_add(p, (char *) it->label, 1);
     }

   /* evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD); */

   ecore_x_selection_primary_set(clipboard_win, it->label, strlen(it->label));
   ecore_x_selection_clipboard_set(clipboard_win, it->label, strlen(it->label));

   return EVRY_ACTION_FINISHED;
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   char buf[1024];
   
   if (history)
     {
	p->items = history;
	history = NULL;
     }


   if (!strncmp(input, "=scale=", 7))
     snprintf(buf, 1024, "%s\n", input + (strlen(p->trigger)));
   else
     snprintf(buf, 1024, "scale=3;%s\n", input + (strlen(p->trigger)));

   ecore_exe_send(exe, buf, strlen(buf));

   /* XXX after error we get no response for first input ?! - send a
      second time...*/
   if (error)
     {
	ecore_exe_send(exe, buf, strlen(buf));
	error = 0;
     }
	
   return 1;
}

static void
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e __UNUSED__)
{
   it->o_icon = NULL;
}

static void
_item_add(Evry_Plugin *p, char *output, int prio)
{
   Evry_Item *it;

   it = E_NEW(Evry_Item, 1);

   it->priority = prio;
   it->label = eina_stringshare_add(output);

   p->items = eina_list_prepend(p->items, it);
}

static int
_cb_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   Ecore_Exe_Event_Data_Line *l;

   if (ev->exe != exe) return 1;
   
   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_CLEAR);

   for (l = ev->lines; l && l->line; l++)
     {
	if (p->items)
	  {
	     Evry_Item *it = p->items->data;
	     if (it->label)
	       eina_stringshare_del(it->label);
	     it->label = eina_stringshare_add(l->line);
	  }
	else
	  _item_add(p, l->line, 1);
     }

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);

   return 1;
}

static int
_cb_error(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;

   if (ev->exe != exe)
     return 1;

   error = 1;

   return 1;
}

static int
_cb_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *e = event;

   if (e->exe != exe)
     return 1;

   exe = NULL;
   return 1;
}


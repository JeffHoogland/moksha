#include "e.h"
#include "evry_api.h"
// TODO - show error when input not parseable

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
};

static Eina_Bool  _cb_data(void *data, int type, void *event);
static Eina_Bool  _cb_error(void *data, int type, void *event);
static Eina_Bool  _cb_del(void *data, int type, void *event);

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;
static Evry_Plugin *_plug;
static Ecore_Event_Handler *action_handler = NULL;

static Ecore_Exe *exe = NULL;
static Eina_List *history = NULL;
static Eina_List *handlers = NULL;
static int error = 0;
static Eina_Bool active = EINA_FALSE;
static char _module_icon[] = "accessories-calculator";
static Evry_Item *cur_item = NULL;

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Evry_Item *it;
   Plugin *p;
   
   if (active)
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin)

   if (history)
     {
	const char *result;

	EINA_LIST_FREE(history, result)
	  {
	     it = EVRY_ITEM_NEW(Evry_Item, p, result, NULL, NULL);
	     it->context = eina_stringshare_ref(p->base.name);
	     p->base.items = eina_list_prepend(p->base.items, it);
	     eina_stringshare_del(result);
	  }
     }

   it = EVRY_ITEM_NEW(Evry_Item, p, "0", NULL, NULL);
   it->context = eina_stringshare_ref(p->base.name);
   cur_item = it;
   active = EINA_TRUE;
   
   return EVRY_PLUGIN(p);
}

static int
_run_bc(Plugin *p)
{
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_DATA, _cb_data, p));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_ERROR, _cb_error, p));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_DEL, _cb_del, p));

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
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);   
   Ecore_Event_Handler *h;
   Evry_Item *it;
   int items = 0;

   EINA_LIST_FREE(p->base.items, it)
     {
   	if ((items++ > 1) && (items < 10))
   	  history = eina_list_prepend(history, eina_stringshare_add(it->label));

	EVRY_ITEM_FREE(it);
     }

   EINA_LIST_FREE(handlers, h)
     ecore_event_handler_del(h);

   if (exe)
     {
	ecore_exe_quit(exe);
	ecore_exe_free(exe);
	exe = NULL;
     }
   active = EINA_FALSE;

   E_FREE(p);
}

static Eina_Bool
_cb_action_performed(__UNUSED__ void *data, __UNUSED__ int type, void *event)
{
   Eina_List *l;
   Evry_Item *it, *it2, *it_old;
   Evry_Event_Action_Performed *ev = event;
   Evry_Plugin *p = _plug;

   if (!ev->it1 || !(ev->it1->plugin == p))
     return ECORE_CALLBACK_PASS_ON;

   if (!p->items)
     return ECORE_CALLBACK_PASS_ON;

   /* remove duplicates */
   if (p->items->next)
     {
	it = p->items->data;

	EINA_LIST_FOREACH(p->items->next, l, it2)
	  {
	     if (!strcmp(it->label, it2->label))
	       {
		  p->items = eina_list_promote_list(p->items, l);
		  evry->item_changed(it, 0, 1);
		  EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
		  return ECORE_CALLBACK_PASS_ON;
	       }
	  }
     }

   it_old = p->items->data;
   it_old->selected = EINA_FALSE;

   it2 = EVRY_ITEM_NEW(Evry_Item, p, it_old->label, NULL, NULL);
   it2->context = eina_stringshare_ref(p->name);
   p->items = eina_list_prepend(p->items, it2);
   evry->item_changed(it2, 0, 1);
   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

   return ECORE_CALLBACK_PASS_ON;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   
   char buf[1024];

   if (!input) return 0;

   if (!exe && !_run_bc(p)) return 0;

   if (!strncmp(input, "scale=", 6))
     snprintf(buf, 1024, "%s\n", input);
   else
     snprintf(buf, 1024, "scale=3;%s\n", input);

   ecore_exe_send(exe, buf, strlen(buf));

   /* XXX after error we get no response for first input ?! - send a
      second time...*/
   if (error)
     {
	ecore_exe_send(exe, buf, strlen(buf));
	error = 0;
     }

   return !!(p->base.items);
}

static Eina_Bool
_cb_data(void *data, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   Evry_Plugin *p = data;
   Evry_Item *it;

   if (ev->exe != exe) return ECORE_CALLBACK_PASS_ON;

   if (ev->lines)
     {
	it = cur_item;
	eina_stringshare_del(it->label);
	it->label = eina_stringshare_add(ev->lines->line);

	if (!(it = eina_list_data_get(p->items)) || (it != cur_item))
	  {
	     p->items = eina_list_prepend(p->items, cur_item);
	     EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
	  }
	else if (it) evry->item_changed(it, 0, 0);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_error(void *data, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   Evry_Plugin *p = data;
   
   if (ev->exe != exe)
     return ECORE_CALLBACK_PASS_ON;

   p->items = eina_list_remove(p->items, cur_item);
   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
   error = 1;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *e = event;

   if (e->exe != exe)
     return ECORE_CALLBACK_PASS_ON;

   exe = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

static int
_plugins_init(const Evry_API *_api)
{
   if (evry_module->active)
     return EINA_TRUE;

   evry = _api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   action_handler = evry->event_handler_add(EVRY_EVENT_ACTION_PERFORMED,
					    _cb_action_performed, NULL);

   _plug = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Calculator"),
			_module_icon,
			EVRY_TYPE_TEXT,
			_begin, _finish, _fetch, NULL);

   _plug->history     = EINA_FALSE;
   _plug->async_fetch = EINA_TRUE;

   if (evry->plugin_register(_plug, EVRY_PLUGIN_SUBJECT, 0))
     {
	Plugin_Config *pc = _plug->config;
	pc->view_mode = VIEW_MODE_LIST;
	pc->trigger = eina_stringshare_add("=");
	pc->trigger_only = EINA_TRUE;
	pc->aggregate = EINA_FALSE;
	/* pc->top_level = EINA_FALSE; */
	/* pc->min_query = 3; */
     }

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   if (!evry_module->active) return;

   ecore_event_handler_del(action_handler);
   action_handler = NULL;

   EVRY_PLUGIN_FREE(_plug);

   evry_module->active = EINA_FALSE;
}

/***************************************************************************/

Eina_Bool
evry_plug_calc_init(E_Module *m)
{
   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   return EINA_TRUE;
}

void
evry_plug_calc_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);
}

void
evry_plug_calc_save(void){}

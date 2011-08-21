#include "e.h"

typedef struct _E_Msg_Event E_Msg_Event;

struct _E_Msg_Handler
{
   void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata);
   void *data;
   unsigned char delete_me : 1;
};

struct _E_Msg_Event
{
   char     *name;
   char     *info;
   int       val;
   E_Object *obj;
   void     *msgdata;
   void    (*afterfunc) (void *data, E_Object *obj, void *msgdata);
   void     *afterdata;
};

/* local subsystem functions */
static Eina_Bool _e_msg_event_cb(void *data, int ev_type, void *ev);
static void _e_msg_event_free(void *data, void *ev);

/* local subsystem globals */
static Eina_List *handlers = NULL;
static Eina_List *del_handlers = NULL;
static int processing_handlers = 0;
static int E_EVENT_MSG = 0;
static Ecore_Event_Handler *hand = NULL;

/* externally accessible functions */
EINTERN int
e_msg_init(void)
{
   E_EVENT_MSG = ecore_event_type_new();
   hand = ecore_event_handler_add(E_EVENT_MSG, _e_msg_event_cb, NULL);
   return 1;
}

EINTERN int
e_msg_shutdown(void)
{
   while (handlers) e_msg_handler_del(eina_list_data_get(handlers));
   E_EVENT_MSG = 0;
   if (hand) ecore_event_handler_del(hand);
   hand = NULL;
   return 1;
}

EAPI void
e_msg_send(const char *name, const char *info, int val, E_Object *obj, void *msgdata, void (*afterfunc) (void *data, E_Object *obj, void *msgdata), void *afterdata)
{
   unsigned int size, pos, name_len, info_len;
   E_Msg_Event *ev;
   
   name_len = info_len = 0;
   size = sizeof(E_Msg_Event);
   if (name) name_len = strlen(name) + 1;
   if (info) info_len = strlen(info) + 1;
   ev = malloc(size + name_len + info_len);
   if (!ev) return;
   pos = size;
   if (name)
     {
       	ev->name = ((char *)ev) + pos;
	pos += name_len;
	strcpy(ev->name, name);
     }
   if (info)
     {
       	ev->info = ((char *)ev) + pos;
	strcpy(ev->info, info);
     }
   ev->val = val;
   ev->obj = obj;
   ev->msgdata = msgdata;
   ev->afterfunc = afterfunc;
   ev->afterdata = afterdata;
   if (ev->obj) e_object_ref(ev->obj);
   ecore_event_add(E_EVENT_MSG, ev, _e_msg_event_free, NULL);
}

EAPI E_Msg_Handler *
e_msg_handler_add(void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata), void *data)
{
   E_Msg_Handler *emsgh;
   
   emsgh = calloc(1, sizeof(E_Msg_Handler));
   if (!emsgh) return NULL;
   emsgh->func = func;
   emsgh->data = data;
   handlers = eina_list_append(handlers, emsgh);
   return emsgh;
}

EAPI void
e_msg_handler_del(E_Msg_Handler *emsgh)
{
   if (processing_handlers > 0)
     {
	emsgh->delete_me = 1;
	del_handlers = eina_list_append(del_handlers, emsgh);
     }
   else
     {
	handlers = eina_list_remove(handlers, emsgh);
	free(emsgh);
     }
}

/* local subsystem functions */

static Eina_Bool
_e_msg_event_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   E_Msg_Event *e;
   Eina_List *l;
   E_Msg_Handler *emsgh;

   processing_handlers++;
   e = ev;
   EINA_LIST_FOREACH(handlers, l, emsgh)
     {
	if (!emsgh->delete_me)
	  emsgh->func(emsgh->data, e->name, e->info, e->val, e->obj, e->msgdata);
     }
   if (e->afterfunc) e->afterfunc(e->afterdata, e->obj, e->msgdata);
   processing_handlers--;
   if ((processing_handlers == 0) && (del_handlers))
     {
	E_FREE_LIST(del_handlers, e_msg_handler_del);
     }
   return 1;
}

static void
_e_msg_event_free(void *data __UNUSED__, void *ev)
{
   E_Msg_Event *e;

   e = ev;
   if (e->obj) e_object_unref(e->obj);

   E_FREE(ev);
}

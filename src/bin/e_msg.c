/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Msg_Event E_Msg_Event;

struct _E_Msg_Handler
{
   void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj);
   void *data;
   unsigned char delete_me : 1;
};

struct _E_Msg_Event
{
   char *name;
   char *info;
   int   val;
   E_Object *obj;
};

/* local subsystem functions */
static int _e_msg_event_cb(void *data, int ev_type, void *ev);
static void _e_msg_event_free(void *data, void *ev);

/* local subsystem globals */
static Evas_List *handlers = NULL;
static Evas_List *del_handlers = NULL;
static int processing_handlers = 0;
static int E_EVENT_MSG = 0;
static Ecore_Event_Handler *hand = NULL;

/* externally accessible functions */
EAPI int
e_msg_init(void)
{
   E_EVENT_MSG = ecore_event_type_new();
   hand = ecore_event_handler_add(E_EVENT_MSG, _e_msg_event_cb, NULL);
   return 1;
}

EAPI int
e_msg_shutdown(void)
{
   while (handlers) e_msg_handler_del(handlers->data);
   E_EVENT_MSG = 0;
   if (hand) ecore_event_handler_del(hand);
   hand = NULL;
   return 1;
}

EAPI void
e_msg_send(const char *name, const char *info, int val, E_Object *obj)
{
   unsigned int size, pos, name_len, info_len;
   E_Msg_Event *ev;
   
   name_len = info_len = size = 0;
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
   if (ev->obj) e_object_ref(ev->obj);
   ecore_event_add(E_EVENT_MSG, ev, _e_msg_event_free, NULL);
}

EAPI E_Msg_Handler *
e_msg_handler_add(void (*func) (void *data, const char *name, const char *info, int val, E_Object *obj), void *data)
{
   E_Msg_Handler *emsgh;
   
   emsgh = calloc(1, sizeof(E_Msg_Handler));
   if (!emsgh) return NULL;
   emsgh->func = func;
   emsgh->data = data;
   handlers = evas_list_append(handlers, emsgh);
   return emsgh;
}

EAPI void
e_msg_handler_del(E_Msg_Handler *emsgh)
{
   if (processing_handlers > 0)
     {
	emsgh->delete_me = 1;
	del_handlers = evas_list_append(del_handlers, emsgh);
     }
   else
     {
	handlers = evas_list_remove(handlers, emsgh);
	free(emsgh);
     }
}

/* local subsystem functions */

static int
_e_msg_event_cb(void *data, int ev_type, void *ev)
{
   E_Msg_Event *e;
   Evas_List *l;

   processing_handlers++;
   e = ev;
   for (l = handlers; l; l = l->next)
     {
	E_Msg_Handler *emsgh;
	
	emsgh = l->data;
	if (!emsgh->delete_me)
	  emsgh->func(emsgh->data, e->name, e->info, e->val, e->obj);
     }
   processing_handlers--;
   if ((processing_handlers == 0) && (del_handlers))
     {
	while (del_handlers)
	  {
	     e_msg_handler_del(del_handlers->data);
	     del_handlers = evas_list_remove_list(del_handlers, del_handlers);
	  }
     }
   return 1;
}

static void
_e_msg_event_free(void *data, void *ev)
{
   E_Msg_Event *e;

   e = ev;
   if (e->obj) e_object_unref(e->obj);
}

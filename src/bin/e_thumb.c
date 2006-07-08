/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Thumb E_Thumb;

struct _E_Thumb
{
   int   objid;
   int   w, h;
   char *file;
   char *key;
   unsigned char queued : 1;
   unsigned char busy : 1;
};

/* local subsystem functions */
static void _e_thumb_gen_begin(int objid, char *file, char *key, int w, int h);
static void _e_thumb_gen_end(int objid);
static void _e_thumb_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_thumb_hash_add(int objid, Evas_Object *obj);
static void _e_thumb_hash_del(int objid);
static Evas_Object *_e_thumb_hash_find(int objid);
static void _e_thumb_thumbnailers_kill(void);
static int _e_thumb_cb_exe_event_del(void *data, int type, void *event);

/* local subsystem globals */
static Evas_List *_thumbnailers = NULL;
static Evas_List *_thumbnailers_exe = NULL;
static Evas_List *_thumb_queue = NULL;
static int _objid = 0;
static Evas_Hash *_thumbs = NULL;
static int _pending = 0;
static int _num_thumbnailers = 2;
static Ecore_Event_Handler *_exe_del_handler = NULL;

/* externally accessible functions */
EAPI int
e_thumb_init(void)
{
   _exe_del_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
					      _e_thumb_cb_exe_event_del,
					      NULL);
   return 1;
}

EAPI int
e_thumb_shutdown(void)
{
   ecore_event_handler_del(_exe_del_handler);
   _exe_del_handler = NULL;
   return 1;
}

EAPI Evas_Object *
e_thumb_icon_add(Evas *evas)
{
   Evas_Object *obj;
   E_Thumb *eth;
   char buf[32];
   
   obj = e_icon_add(evas);
   _objid++;
   eth = E_NEW(E_Thumb, 1);
   eth->objid = _objid;
   eth->w = 64;
   eth->h = 64;
   evas_object_data_set(obj, "e_thumbdata", eth);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
				  _e_thumb_del_hook, NULL);
   _e_thumb_hash_add(eth->objid, obj);
   return obj;
}

EAPI void
e_thumb_icon_file_set(Evas_Object *obj, char *file, char *key)
{
   E_Thumb *eth;
   
   eth = evas_object_data_get(obj, "e_thumbdata");
   if (!eth) return;
   E_FREE(eth->file);
   E_FREE(eth->key);
   if (file) eth->file = strdup(file);
   if (key) eth->key = strdup(key);
}

EAPI void
e_thumb_icon_size_set(Evas_Object *obj, int w, int h)
{
   E_Thumb *eth;
   
   eth = evas_object_data_get(obj, "e_thumbdata");
   if (!eth) return;
   if ((w < 1) || (h <1)) return;
   eth->w = w;
   eth->h = h;
}

EAPI void
e_thumb_icon_begin(Evas_Object *obj)
{
   E_Thumb *eth, *eth2;
   char buf[4096];
   
   eth = evas_object_data_get(obj, "e_thumbdata");
   if (!eth) return;
   if (!eth->file) return;
   if (!_thumbnailers)
     {
	while (evas_list_count(_thumbnailers_exe) < _num_thumbnailers)
	  {
	     Ecore_Exe *exe;
	     
	     snprintf(buf, sizeof(buf), "%s/enlightenment_thumb", e_prefix_bin_get());
	     exe = ecore_exe_run(buf, NULL);
	     _thumbnailers_exe = evas_list_append(_thumbnailers_exe, exe);
	  }
	_thumb_queue = evas_list_append(_thumb_queue, eth);
	eth->queued = 1;
	return;
     }
   while (_thumb_queue)
     {
	eth2 = _thumb_queue->data;
	_thumb_queue = evas_list_remove_list(_thumb_queue, _thumb_queue);
	eth2->queued = 0;
	eth2->busy = 1;
	_pending++;
	_e_thumb_gen_begin(eth2->objid, eth2->file, eth2->key, eth2->w, eth2->h);
     }
   eth->busy = 1;
   _pending++;
   _e_thumb_gen_begin(eth->objid, eth->file, eth->key, eth->w, eth->h);
}

EAPI void
e_thumb_icon_end(Evas_Object *obj)
{
   E_Thumb *eth;
   
   eth = evas_object_data_get(obj, "e_thumbdata");
   if (!eth) return;
   if (eth->queued) _thumb_queue = evas_list_remove(_thumb_queue, eth);
   eth->queued = 0;
   if (eth->busy) _e_thumb_gen_end(eth->objid);
   eth->busy = 0;
   _pending--;
   if (_pending == 0) _e_thumb_thumbnailers_kill();
}

EAPI void
e_thumb_client_data(Ecore_Ipc_Event_Client_Data *e)
{
   int objid;
   char *icon;
   E_Thumb *eth;
   Evas_Object *obj;
   
   if (!evas_list_find(_thumbnailers, e->client))
     _thumbnailers = evas_list_prepend(_thumbnailers, e->client);
   if (e->minor == 2)
     {
	objid = e->ref;
	icon = e->data;
	if ((icon) && (e->size > 1) && (icon[e->size - 1] == 0))
	  {
	     obj = _e_thumb_hash_find(objid);
	     if (obj)
	       {
		  eth = evas_object_data_get(obj, "e_thumbdata");
		  if (eth)
		    {
		       eth->busy = 0;
		       _pending--;
		       if (_pending == 0) _e_thumb_thumbnailers_kill();
		       e_icon_file_key_set(obj, icon, "/thumbnail/data");
		       evas_object_smart_callback_call(obj, "e_thumb_gen", NULL);
		    }
	       }
	  }
     }
   if (e->minor == 1)
     {
	/* hello message */
	while (_thumb_queue)
	  {
	     eth = _thumb_queue->data;
	     _thumb_queue = evas_list_remove_list(_thumb_queue, _thumb_queue);
	     eth->queued = 0;
	     eth->busy = 1;
	     _pending++;
	     _e_thumb_gen_begin(eth->objid, eth->file, eth->key, eth->w, eth->h);
	  }
     }
}

EAPI void
e_thumb_client_del(Ecore_Ipc_Event_Client_Del *e)
{
   if (!evas_list_find(_thumbnailers, e->client)) return;
   _thumbnailers = evas_list_remove(_thumbnailers, e->client);
}

/* local subsystem functions */
static void
_e_thumb_gen_begin(int objid, char *file, char *key, int w, int h)
{
   char *buf;
   int l1, l2;
   Ecore_Ipc_Client *cli;

   /* send thumb req */
   l1 = strlen(file);
   l2 = 0;
   if (key) l2 = strlen(key);
   buf = alloca(l1 + 1 + l2 + 1);
   strcpy(buf, file);
   if (key) strcpy(buf + l1 + 1, key);
   else buf[l1 + 1] = 0;
   cli = _thumbnailers->data;
   if (!cli) return;
   _thumbnailers = evas_list_remove_list(_thumbnailers, _thumbnailers);
   _thumbnailers = evas_list_append(_thumbnailers, cli);
   ecore_ipc_client_send(cli, E_IPC_DOMAIN_THUMB, 1, objid, w, h, buf, l1 + 1 + l2 + 1);
}

static void
_e_thumb_gen_end(int objid)
{
   Evas_List *l;
   Ecore_Ipc_Client *cli;
   
   /* send thumb cancel */
   for (l = _thumbnailers; l; l = l->next)
     {
	cli = l->data;
	ecore_ipc_client_send(cli, E_IPC_DOMAIN_THUMB, 2, objid, 0, 0, NULL, 0);
     }
}

static void
_e_thumb_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Thumb *eth;
   
   eth = evas_object_data_get(obj, "e_thumbdata");
   if (!eth) return;
   evas_object_data_del(obj, "e_thumbdata");
   _e_thumb_hash_del(eth->objid);
   if (eth->busy)
     {
	_e_thumb_gen_end(eth->objid);
	_pending--;
	if (_pending == 0) _e_thumb_thumbnailers_kill();
     }
   if (eth->queued)
     _thumb_queue = evas_list_remove(_thumb_queue, eth);
   E_FREE(eth->file);
   E_FREE(eth->key);
   free(eth);
}

static void
_e_thumb_hash_add(int objid, Evas_Object *obj)
{
   char buf[32];
   
   snprintf(buf, sizeof(buf), "%i", objid);
   _thumbs = evas_hash_add(_thumbs, buf, obj);
}

static void
_e_thumb_hash_del(int objid)
{
   char buf[32];
   
   snprintf(buf, sizeof(buf), "%i", objid);
   _thumbs = evas_hash_del(_thumbs, buf, NULL);
}

static Evas_Object *
_e_thumb_hash_find(int objid)
{
   char buf[32];
   
   snprintf(buf, sizeof(buf), "%i", objid);
   return evas_hash_find(_thumbs, buf);
}

static void
_e_thumb_thumbnailers_kill(void)
{
   Evas_List *l;
   
   for (l = _thumbnailers_exe; l; l = l->next)
     ecore_exe_terminate(l->data);
}

static int
_e_thumb_cb_exe_event_del(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Evas_List *l;
   
   ev = event;
   for (l = _thumbnailers_exe; l; l = l->next)
     {
	if (l->data == ev->exe)
	  {
	     _thumbnailers_exe = evas_list_remove_list(_thumbnailers_exe, l);
	     break;
	  }
     }
   return 1;
}

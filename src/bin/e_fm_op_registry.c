/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI int E_EVENT_FM_OP_REGISTRY_ADD = 0;
EAPI int E_EVENT_FM_OP_REGISTRY_DEL = 0;
EAPI int E_EVENT_FM_OP_REGISTRY_CHANGED = 0;

static Eina_Hash *_e_fm2_op_registry = NULL;
static unsigned int _e_fm2_init_count = 0;

typedef struct _E_Fm2_Op_Registry_Entry_Listener E_Fm2_Op_Registry_Entry_Listener;
typedef struct _E_Fm2_Op_Registry_Entry_Internal E_Fm2_Op_Registry_Entry_Internal;

struct _E_Fm2_Op_Registry_Entry_Listener
{
   EINA_INLIST;
   void (*cb)(void *data, const E_Fm2_Op_Registry_Entry *entry);
   void *data;
   void (*free_data)(void *data);
};

struct _E_Fm2_Op_Registry_Entry_Internal
{
   E_Fm2_Op_Registry_Entry entry;
   Eina_Inlist *listeners;
   int references;
   Ecore_Event *changed_event;
};

static void
_e_fm2_op_registry_entry_e_fm_deleted(void *data, Evas *evas, Evas_Object *e_fm, void *event)
{
   E_Fm2_Op_Registry_Entry *entry = data;

   entry->e_fm = NULL;
   e_fm2_op_registry_entry_changed(entry);
}

static void
_e_fm2_op_registry_entry_e_fm_monitor_start(const E_Fm2_Op_Registry_Entry *entry)
{
   if (!entry->e_fm) return;
   evas_object_event_callback_add
     (entry->e_fm, EVAS_CALLBACK_DEL,
      _e_fm2_op_registry_entry_e_fm_deleted, entry);
}

static void
_e_fm2_op_registry_entry_e_fm_monitor_stop(const E_Fm2_Op_Registry_Entry *entry)
{
   if (!entry->e_fm) return;
   evas_object_event_callback_del_full
     (entry->e_fm, EVAS_CALLBACK_DEL,
      _e_fm2_op_registry_entry_e_fm_deleted, entry);
}


static inline E_Fm2_Op_Registry_Entry_Internal *
_e_fm2_op_registry_entry_internal_get(const E_Fm2_Op_Registry_Entry *entry)
{
   return (E_Fm2_Op_Registry_Entry_Internal *)entry;
}

static void
_e_fm2_op_registry_entry_internal_free(E_Fm2_Op_Registry_Entry_Internal *e)
{
   _e_fm2_op_registry_entry_e_fm_monitor_stop(&(e->entry));

   while (e->listeners)
     {
	E_Fm2_Op_Registry_Entry_Listener *listener = (void *)e->listeners;
	e->listeners = eina_inlist_remove(e->listeners, e->listeners);

	if (listener->free_data) listener->free_data(listener->data);
	free(listener);
     }

   eina_stringshare_del(e->entry.src);
   eina_stringshare_del(e->entry.dst);
   free(e);
}

static inline int
_e_fm2_op_registry_entry_internal_unref(E_Fm2_Op_Registry_Entry_Internal *e)
{
   if (e->references < 1)
     return 0;

   e->references--;
   if (e->references > 0)
     return e->references;

   _e_fm2_op_registry_entry_internal_free(e);
   return 0;
}

static inline int
_e_fm2_op_registry_entry_internal_ref(E_Fm2_Op_Registry_Entry_Internal *e)
{
   e->references++;
   return e->references;
}

static void
_e_fm2_op_registry_entry_listeners_call(const E_Fm2_Op_Registry_Entry_Internal *e)
{
   E_Fm2_Op_Registry_Entry_Listener *l, **shadow;
   const E_Fm2_Op_Registry_Entry *entry;
   unsigned int i, count;

   /* NB: iterate on a copy in order to allow listeners to be deleted
    * from callbacks.  number of listeners should be small, so the
    * following should do fine.
    */
   count = eina_inlist_count(e->listeners);
   if (count < 1) return;

   shadow = alloca(sizeof(*shadow) * count);
   if (!shadow) return;

   i = 0;
   EINA_INLIST_FOREACH(e->listeners, l)
     shadow[i++] = l;

   entry = &(e->entry);
   for (i = 0; i < count; i++)
     shadow[i]->cb(shadow[i]->data, entry);
}

static void
_e_fm2_op_registry_entry_internal_unref_on_event(void *data, void *event __UNUSED__)
{
   E_Fm2_Op_Registry_Entry_Internal *e = data;
   _e_fm2_op_registry_entry_internal_unref(e);
}

static void
_e_fm2_op_registry_entry_internal_event(E_Fm2_Op_Registry_Entry_Internal *e, int event_type)
{
   _e_fm2_op_registry_entry_internal_ref(e);
   ecore_event_add(event_type, &(e->entry),
		   _e_fm2_op_registry_entry_internal_unref_on_event, e);
}

Eina_Bool
e_fm2_op_registry_entry_add(int id, Evas_Object *e_fm, E_Fm_Op_Type op)
{
   E_Fm2_Op_Registry_Entry_Internal *e;

   e = E_NEW(E_Fm2_Op_Registry_Entry_Internal, 1);
   if (!e) return 0;

   e->entry.id = id;
   e->entry.e_fm = e_fm;
   e->entry.start_time = ecore_loop_time_get();
   e->entry.op = op;
   e->entry.status = E_FM2_OP_STATUS_IN_PROGRESS;
   e->references = 1;

   if (!eina_hash_add(_e_fm2_op_registry, &id, e))
     {
	free(e);
	return 0;
     }

   _e_fm2_op_registry_entry_e_fm_monitor_start(&(e->entry));
   _e_fm2_op_registry_entry_internal_event(e, E_EVENT_FM_OP_REGISTRY_ADD);

   return 1;
}

Eina_Bool
e_fm2_op_registry_entry_del(int id)
{
   E_Fm2_Op_Registry_Entry_Internal *e;

   e = eina_hash_find(_e_fm2_op_registry, &id);
   if (!e) return 0;
   eina_hash_del_by_key(_e_fm2_op_registry, &id);

   _e_fm2_op_registry_entry_internal_event(e, E_EVENT_FM_OP_REGISTRY_DEL);
   _e_fm2_op_registry_entry_internal_unref(e);

   return 1;
}

static void
_e_fm2_op_registry_entry_internal_unref_on_changed_event(void *data, void *event __UNUSED__)
{
   E_Fm2_Op_Registry_Entry_Internal *e = data;
   e->changed_event = NULL;
   _e_fm2_op_registry_entry_internal_unref(e);
}

void
e_fm2_op_registry_entry_changed(const E_Fm2_Op_Registry_Entry *entry)
{
   E_Fm2_Op_Registry_Entry_Internal *e;

   if (!entry) return;
   e = _e_fm2_op_registry_entry_internal_get(entry);

   _e_fm2_op_registry_entry_listeners_call(e);

   if (e->changed_event) return;
   _e_fm2_op_registry_entry_internal_ref(e);
   e->changed_event = ecore_event_add
     (E_EVENT_FM_OP_REGISTRY_CHANGED, &(e->entry),
      _e_fm2_op_registry_entry_internal_unref_on_changed_event, e);
}

/**
 * Set the new e_fm for this operation.
 *
 * Use this call instead of directly setting in order to have the
 * object to be monitored, when it is gone, the pointer will be made
 * NULL.
 *
 * @note: it will not call any listener or add any event, please use
 * e_fm2_op_registry_entry_changed().
 */
void
e_fm2_op_registry_entry_e_fm_set(E_Fm2_Op_Registry_Entry *entry, Evas_Object *e_fm)
{
   if (!entry) return;
   _e_fm2_op_registry_entry_e_fm_monitor_stop(entry);
   entry->e_fm = e_fm;
   _e_fm2_op_registry_entry_e_fm_monitor_start(entry);
}

/**
 * Set the new files for this operation.
 *
 * Use this call instead of directly setting in order to have
 * stringshare references right.
 *
 * @note: it will not call any listener or add any event, please use
 * e_fm2_op_registry_entry_changed().
 */
void
e_fm2_op_registry_entry_files_set(E_Fm2_Op_Registry_Entry *entry, const char *src, const char *dst)
{
   if (!entry) return;

   src = eina_stringshare_add(src);
   dst = eina_stringshare_add(dst);

   eina_stringshare_del(entry->src);
   eina_stringshare_del(entry->dst);

   entry->src = src;
   entry->dst = dst;
}

/**
 * Adds a reference to given entry.
 *
 * @return: new number of references after operation or -1 on error.
 */
EAPI int
e_fm2_op_registry_entry_ref(E_Fm2_Op_Registry_Entry *entry)
{
   E_Fm2_Op_Registry_Entry_Internal *e;

   if (!entry) return -1;

   e = _e_fm2_op_registry_entry_internal_get(entry);
   return _e_fm2_op_registry_entry_internal_ref(e);
}

/**
 * Releases a reference to given entry.
 *
 * @return: new number of references after operation or -1 on error,
 *    if 0 the entry was freed and pointer is then invalid.
 */
EAPI int
e_fm2_op_registry_entry_unref(E_Fm2_Op_Registry_Entry *entry)
{
   E_Fm2_Op_Registry_Entry_Internal *e;

   if (!entry) return -1;

   e = _e_fm2_op_registry_entry_internal_get(entry);
   return _e_fm2_op_registry_entry_internal_unref(e);
}

/**
 * Returns the X window associated to this operation.
 *
 * This will handle all bureaucracy to get X window based on e_fm evas
 * object.
 *
 * @return: 0 if no window, window identifier otherwise.
 */
EAPI Ecore_X_Window
e_fm2_op_registry_entry_xwin_get(const E_Fm2_Op_Registry_Entry *entry)
{
   Evas *e;
   Ecore_Evas *ee;

   if (!entry) return 0;
   if (!entry->e_fm) return 0;

   e = evas_object_evas_get(entry->e_fm);
   if (!e) return 0;

   ee = evas_data_attach_get(e);
   if (!ee) return 0;

   return (Ecore_X_Window)(long)ecore_evas_window_get(ee);
}

/**
 * Discover entry based on its identifier.
 *
 * @note: does not increment reference.
 */
EAPI E_Fm2_Op_Registry_Entry *
e_fm2_op_registry_entry_get(int id)
{
   return eina_hash_find(_e_fm2_op_registry, &id);
}

/**
 * Adds a function to be called when entry changes.
 *
 * When entry changes any attribute this function will be called.
 *
 * @param: entry entry to operate on.
 * @param: cb function to callback on changes.
 * @param: data extra data to give to @p cb
 * @param: free_data function to call when listener is removed, entry
 *    is deleted or any error occur in this function and listener
 *    cannot be added.
 *
 * @note: does not increment reference.
 * @note: on errors, @p free_data will be called.
 */
EAPI void
e_fm2_op_registry_entry_listener_add(E_Fm2_Op_Registry_Entry *entry, void (*cb)(void *data, const E_Fm2_Op_Registry_Entry *entry), const void *data, void (*free_data)(void *data))
{
   E_Fm2_Op_Registry_Entry_Internal *e;
   E_Fm2_Op_Registry_Entry_Listener *listener;
   Eina_Error err;

   if ((!entry) || (!cb))
     {
	if (free_data) free_data((void *)data);
	return;
     }

   listener = malloc(sizeof(*listener));
   if (!listener)
     {
	if (free_data) free_data((void *)data);
	return;
     }
   listener->cb = cb;
   listener->data = (void *)data;
   listener->free_data = free_data;

   e = _e_fm2_op_registry_entry_internal_get(entry);
   e->listeners = eina_inlist_append(e->listeners, EINA_INLIST_GET(listener));
   err = eina_error_get();
   if (err)
     {
	EINA_ERROR_PERR("could not add listener: %s\n",
			eina_error_msg_get(err));

	if (free_data) free_data((void *)data);
	free(listener);
	return;
     }
}

/**
 * Removes the function to be called when entry changes.
 *
 * @param: entry entry to operate on.
 * @param: cb function to callback on changes.
 * @param: data extra data to give to @p cb
 *
 * @note: does not decrement reference.
 * @see: e_fm2_op_registry_entry_listener_add()
 */
EAPI void
e_fm2_op_registry_entry_listener_del(E_Fm2_Op_Registry_Entry *entry, void (*cb)(void *data, const E_Fm2_Op_Registry_Entry *entry), const void *data)
{
   E_Fm2_Op_Registry_Entry_Internal *e;
   E_Fm2_Op_Registry_Entry_Listener *l;

   if ((!entry) || (!cb)) return;
   e = _e_fm2_op_registry_entry_internal_get(entry);

   EINA_INLIST_FOREACH(e->listeners, l)
     if ((l->cb == cb) && (l->data == data))
       {
	  e->listeners = eina_inlist_remove(e->listeners, EINA_INLIST_GET(l));
	  if (l->free_data) l->free_data(l->data);
	  return;
       }
}

/**
 * Returns an iterator over all the entries in the fm operations registry.
 *
 * @warning: this iterator is just valid until new entries are added
 *    or removed (usually happens from main loop). This is because
 *    when system is back to main loop it can report new events and
 *    operations can be added or removed from this registry. In other
 *    words, it is fine to call this function, immediately walk the
 *    iterator and do something, then free the iterator. You can use
 *    it to create a shadow list if you wish.
 *
 * @see e_fm2_op_registry_get_all()
 */
EAPI Eina_Iterator *
e_fm2_op_registry_iterator_new(void)
{
   return eina_hash_iterator_data_new(_e_fm2_op_registry);
}

/**
 * Returns a shadow list with all entries in the registry.
 *
 * All entries will have references incremented, so you must free the
 * list with e_fm2_op_registry_get_all_free() to free the list and
 * release these references.
 *
 * @note: List is unsorted!
 * @note: if you need a simple, immediate walk, use
 *    e_fm2_op_registry_iterator_new()
 */
EAPI Eina_List *
e_fm2_op_registry_get_all(void)
{
   Eina_List *list;
   Eina_Iterator *it;
   E_Fm2_Op_Registry_Entry_Internal *e;

   list = NULL;
   it = eina_hash_iterator_data_new(_e_fm2_op_registry);
   EINA_ITERATOR_FOREACH(it, e)
     {
	_e_fm2_op_registry_entry_internal_ref(e);
	list = eina_list_append(list, &(e->entry));
     }
   eina_iterator_free(it);

   return list;
}

EAPI void
e_fm2_op_registry_get_all_free(Eina_List *list)
{
   E_Fm2_Op_Registry_Entry *entry;
   EINA_LIST_FREE(list, entry)
     e_fm2_op_registry_entry_unref(entry);
}

EAPI Eina_Bool
e_fm2_op_registry_is_empty(void)
{
   return eina_hash_population(_e_fm2_op_registry) == 0;
}


EAPI unsigned int
e_fm2_op_registry_init(void)
{
   _e_fm2_init_count++;
   if (_e_fm2_init_count > 1) return _e_fm2_init_count;

   _e_fm2_op_registry = eina_hash_int32_new(NULL);
   if (!_e_fm2_op_registry)
     {
	_e_fm2_init_count = 0;
	return 0;
     }

   if (E_EVENT_FM_OP_REGISTRY_ADD == 0)
     E_EVENT_FM_OP_REGISTRY_ADD = ecore_event_type_new();
   if (E_EVENT_FM_OP_REGISTRY_DEL == 0)
     E_EVENT_FM_OP_REGISTRY_DEL = ecore_event_type_new();
   if (E_EVENT_FM_OP_REGISTRY_CHANGED == 0)
     E_EVENT_FM_OP_REGISTRY_CHANGED = ecore_event_type_new();

   return 1;
}

EAPI unsigned int
e_fm2_op_registry_shutdown(void)
{
   if (_e_fm2_init_count == 0) return 0;
   _e_fm2_init_count--;
   if (_e_fm2_init_count > 0) return _e_fm2_init_count;

   eina_hash_free(_e_fm2_op_registry);
   _e_fm2_op_registry = NULL;

   return 0;
}

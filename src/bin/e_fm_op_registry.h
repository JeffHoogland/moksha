/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Fm2_Op_Status
{
  E_FM2_OP_STATUS_UNKNOWN = 0,
  E_FM2_OP_STATUS_IN_PROGRESS,
  E_FM2_OP_STATUS_SUCCESSFUL,
  E_FM2_OP_STATUS_ABORTED,
  E_FM2_OP_STATUS_ERROR
} E_Fm2_Op_Status;

typedef struct _E_Fm2_Op_Registry_Entry E_Fm2_Op_Registry_Entry;

#else
#ifndef E_FM_OP_REGISTRY_H
#define E_FM_OP_REGISTRY_H

struct _E_Fm2_Op_Registry_Entry
{
   int id;
   int percent; /* XXX use char? */
   size_t done;
   size_t total;
   Evas_Object *e_fm;
   const char *src; /* stringshared */
   const char *dst; /* stringshared */
   double start_time;
   int duration; /* XXX use double? */
   E_Fm_Op_Type op;
   E_Fm2_Op_Status status;
   Eina_Bool needs_attention:1;
   Eina_Bool finished:1;
};

extern EAPI int E_EVENT_FM_OP_REGISTRY_ADD;
extern EAPI int E_EVENT_FM_OP_REGISTRY_DEL;
extern EAPI int E_EVENT_FM_OP_REGISTRY_CHANGED;

EAPI int e_fm2_op_registry_entry_ref(E_Fm2_Op_Registry_Entry *entry);
EAPI int e_fm2_op_registry_entry_unref(E_Fm2_Op_Registry_Entry *entry);

EAPI Ecore_X_Window e_fm2_op_registry_entry_xwin_get(const E_Fm2_Op_Registry_Entry *entry);

EAPI E_Fm2_Op_Registry_Entry *e_fm2_op_registry_entry_get(int id);

EAPI void e_fm2_op_registry_entry_listener_add(E_Fm2_Op_Registry_Entry *entry, void (*cb)(void *data, const E_Fm2_Op_Registry_Entry *entry), const void *data, void (*free_data)(void *data));
EAPI void e_fm2_op_registry_entry_listener_del(E_Fm2_Op_Registry_Entry *entry, void (*cb)(void *data, const E_Fm2_Op_Registry_Entry *entry), const void *data);

EAPI Eina_Iterator *e_fm2_op_registry_iterator_new(void);
EAPI Eina_List     *e_fm2_op_registry_get_all(void);
EAPI void           e_fm2_op_registry_get_all_free(Eina_List *list);
EAPI Eina_Bool      e_fm2_op_registry_is_empty(void);

EAPI unsigned int e_fm2_op_registry_init(void);
EAPI unsigned int e_fm2_op_registry_shutdown(void);

/* E internal/private functions, symbols not exported outside e binary (e_fm.c mainly) */
Eina_Bool e_fm2_op_registry_entry_add(int id, Evas_Object *e_fm, E_Fm_Op_Type op);
Eina_Bool e_fm2_op_registry_entry_del(int id);
void      e_fm2_op_registry_entry_changed(const E_Fm2_Op_Registry_Entry *entry);
void      e_fm2_op_registry_entry_e_fm_set(E_Fm2_Op_Registry_Entry *entry, Evas_Object *e_fm);
void      e_fm2_op_registry_entry_files_set(E_Fm2_Op_Registry_Entry *entry, const char *src, const char *dst);

#endif
#endif

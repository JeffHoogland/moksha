/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */
static Evas_Hash *store = NULL;

/* externally accessible functions */
EAPI void
e_datastore_set(char *key, void *data)
{
   store = evas_hash_del(store, key, NULL);
   store = evas_hash_add(store, key, data);
}

EAPI void *
e_datastore_get(char *key)
{
   return evas_hash_find(store, key);
}

EAPI void
e_datastore_del(char *key)
{
   store = evas_hash_del(store, key, NULL);
}

/* local subsystem functions */

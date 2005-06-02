/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_DATASTORE_H
#define E_DATASTORE_H

EAPI void        e_datastore_set(char *key, void *data);
EAPI void       *e_datastore_get(char *key);
EAPI void        e_datastore_del(char *key);

#endif
#endif

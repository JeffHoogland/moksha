/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* externally accessible functions */
EAPI E_Fm2_Custom_File *
e_fm2_custom_file_get(const char *path)
{
   /* get any custom info for the path in our metadata - if non exists,
    * return NULL. This may mean loading upa chunk of metadata off disk
    * on demand and caching it */
   return NULL;
}

EAPI void
e_fm2_custom_file_set(const char *path, E_Fm2_Custom_File *cf)
{
   /* set custom metadata for a file path - save it to the metadata (or queue it) */
}

EAPI void e_fm2_custom_file_del(const char *path)
{
   /* delete a custom metadata entry for a path - save changes (or queue it) */
}

EAPI void e_fm2_custom_file_rename(const char *path, const char *new_path)
{
   /* rename file path a to file paht b in the metadata - if the path exists */
}

EAPI void e_fm2_custom_file_flush(void)
{
   /* free any loaded custom file data, sync changes to disk etc. */
}


/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Path E_Path;
   
#else
#ifndef E_PATH_H
#define E_PATH_H

#define E_PATH_TYPE 0xE0b0100c

struct _E_Path
{
   E_Object   e_obj_inherit;
   
   Evas_Hash *hash;
   
   Evas_List *dir_list;
};

EAPI E_Path     *e_path_new(void);
EAPI void        e_path_path_append(E_Path *ep, const char *path);
EAPI void        e_path_path_prepend(E_Path *ep, const char *path);
EAPI void        e_path_path_remove(E_Path *ep, const char *path);
EAPI char        *e_path_find(E_Path *ep, const char *file); /* for conveience this doesnt return a malloc'd string. it's a static buffer, so a new call will replace this buffer, but thsi means there is no need to free the return */
EAPI void        e_path_evas_append(E_Path *ep, Evas *evas);

#endif
#endif

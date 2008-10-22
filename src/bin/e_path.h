/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Path E_Path;
typedef struct _E_Path_Dir E_Path_Dir;   

#else
#ifndef E_PATH_H
#define E_PATH_H

#define E_PATH_TYPE 0xE0b0100c

struct _E_Path_Dir
{
   const char *dir;
};

struct _E_Path
{
   E_Object   e_obj_inherit;
    
   Evas_Hash *hash;
   
   Eina_List *default_dir_list;
   /* keep track of the associated e_config path */
   Eina_List **user_dir_list;
};

/* init and setup */
EAPI E_Path     *e_path_new(void);
EAPI void        e_path_user_path_set(E_Path *ep, Eina_List **user_dir_list);
EAPI void        e_path_inherit_path_set(E_Path *ep, E_Path *path_inherit);
/* append a hardcoded path */
EAPI void        e_path_default_path_append(E_Path *ep, const char *path);
/* e_config path manipulation */
EAPI void        e_path_user_path_append(E_Path *ep, const char *path);
EAPI void        e_path_user_path_prepend(E_Path *ep, const char *path);
EAPI void        e_path_user_path_remove(E_Path *ep, const char *path);
EAPI void	 e_path_user_path_clear(E_Path *ep);
EAPI const char *e_path_find(E_Path *ep, const char *file); 
EAPI void        e_path_evas_append(E_Path *ep, Evas *evas);
EAPI Eina_List  *e_path_dir_list_get(E_Path *ep);
EAPI void	 e_path_dir_list_free(Eina_List *dir_list);

#endif
#endif

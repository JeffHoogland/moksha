/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_FILE_H
#define E_FILE_H

EAPI time_t     e_file_mod_time(char *file);
EAPI int        e_file_exists(char *file);
EAPI int        e_file_is_dir(char *file);
EAPI int        e_file_mkdir(char *dir);
EAPI int        e_file_mkpath(char *path);
EAPI int        e_file_cp(char *src, char *dst);
EAPI char      *e_file_realpath(char *file);
EAPI char      *e_file_get_file(char *path);
EAPI char      *e_file_get_dir(char *path);

EAPI int        e_file_can_exec(struct stat *st);
EAPI char      *e_file_readlink(char *link);
EAPI Evas_List *e_file_ls(char *dir);

#endif
#endif

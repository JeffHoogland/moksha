#ifndef E_UTIL_H
#define E_UTIL_H

#include "e.h"

time_t     e_file_modified_time(char *file);
void       e_set_env(char *variable, char *content);
int        e_file_exists(char *file);
int        e_file_is_dir(char *file);
char      *e_file_home(void);
int        e_file_mkdir(char *dir);
int        e_file_cp(char *src, char *dst);
char      *e_file_real(char *file);
char      *e_file_get_file(char *file);
char      *e_file_get_dir(char *file);
void      *e_memdup(void *data, int size);
int        e_glob_matches(char *str, char *glob);
int        e_file_can_exec(struct stat *st);
char      *e_file_link(char *link);
Evas_List  e_file_list_dir(char *dir);

#define e_strdup(__dest, __var) \
{ \
if (!__var) __dest = NULL; \
else { \
__dest = malloc(strlen(__var) + 1); \
if (__dest) strcpy(__dest, __var); \
} }

#endif

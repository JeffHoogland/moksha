#ifndef E_UTIL_H
#define E_UTIL_H

#include "e.h"

void       e_util_set_env(char *variable, char *content);
char      *e_util_get_user_home(void);
void      *e_util_memdup(void *data, int size);
int        e_util_glob_matches(char *str, char *glob);

#define e_strdup(__dest, __var) \
{ \
if (!__var) __dest = NULL; \
else { \
__dest = malloc(strlen(__var) + 1); \
if (__dest) strcpy(__dest, __var); \
} }

#endif

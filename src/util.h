#ifndef E_UTIL_H
#define E_UTIL_H

#include "e.h"

void                e_util_set_env(char *variable, char *content);
char               *e_util_get_user_home(void);
void               *e_util_memdup(void *data, int size);
int                 e_util_glob_matches(char *str, char *glob);
char               *e_util_de_url_and_verify(const char *fi);

#define e_strdup(__dest, __var) \
{ \
if (!__var) __dest = NULL; \
else { \
__dest = malloc(strlen(__var) + 1); \
if (__dest) strcpy(__dest, __var); \
} }

/* misc util macros */
#define INTERSECTS(x, y, w, h, xx, yy, ww, hh) \
(((x) < ((xx) + (ww))) && \
((y) < ((yy) + (hh))) && \
(((x) + (w)) > (xx)) && \
(((y) + (h)) > (yy)))
#define SPANS_COMMON(x1, w1, x2, w2) \
(!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
#define UN(_blah) _blah = 0

#endif

#ifndef E_UTIL_H
#define E_UTIL_H

#include "e.h"

void                e_util_set_env(char *variable, char *content);
char               *e_util_get_user_home(void);
void               *e_util_memdup(void *data, int size);
int                 e_util_glob_matches(char *str, char *glob);
char               *e_util_de_url_and_verify(const char *fi);
Window              e_evas_get_window(Evas *evas);
Evas_List          *e_evas_get_mask(Evas *evas, Pixmap pmap, Pixmap mask);
Evas               *e_evas_new_all(Display *disp, Window win,
	       int x, int y, int win_w, int win_h,
	       char *font_dir);

#define RENDER_METHOD_ALPHA_SOFTWARE 0

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
#define MIN(v1, v2) (v1 < v2 ? v1 : v2)

#define STRNCPY(d, s, l) \
do \
{ \
   int __c; \
   for (__c = 0; (__c < (l)) && ((d)[__c] = (s)[__c]); __c++); \
   (d)[(l) - 1] = 0; \
} \
while (0)

#endif

#ifndef E_H
#define E_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <dlfcn.h>

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>
#include <Ecore_Ipc.h>
#include <Ecore_Job.h>
#include <Ecore_Txt.h>
#include <Ecore_Config.h>
#include <Eet.h>
#include <Edje.h>

#include "config.h"

#include "e_object.h"
#include "e_file.h"
#include "e_user.h"
#include "e_manager.h"
#include "e_path.h"
#include "e_ipc.h"
#include "e_error.h"
#include "e_container.h"
#include "e_desk.h"
#include "e_border.h"
#include "e_pointer.h"
#include "e_config.h"
#include "e_menu.h"
#include "e_icon.h"
#include "e_box.h"
#include "e_init.h"
#include "e_int_menus.h"
#include "e_module.h"
#include "e_apps.h"
#include "e_utils.h"
#include "e_canvas.h"
#include "e_focus.h"
#include "e_place.h"

typedef struct _E_Before_Idler E_Before_Idler;

E_Before_Idler *e_main_idler_before_add(int (*func) (void *data), void *data, int once);
void            e_main_idler_before_del(E_Before_Idler *eb);

extern E_Path *path_data;
extern E_Path *path_images;
extern E_Path *path_fonts;
extern E_Path *path_themes;
extern E_Path *path_init;
extern int     restart;

/* convenience macro to compress code and avoid typos */
#define E_FN_DEL(_fn, _h) \
if (_h) \
{ \
   _fn(_h); \
   _h = NULL; \
}

#define E_INTERSECTS(x, y, w, h, xx, yy, ww, hh) \
(((x) < ((xx) + (ww))) && \
((y) < ((yy) + (hh))) && \
(((x) + (w)) > (xx)) && \
(((y) + (h)) > (yy)))

#define E_SPANS_COMMON(x1, w1, x2, w2) \
(!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))

#define E_REALLOC(p, s, n) \
       p = realloc(p, sizeof(s) * n)

#define E_NEW(s, n) \
       calloc(n, sizeof(s))

#define E_NEW_BIG(s, n) \
       malloc(n * sizeof(s))

#define E_FREE(p) \
       { if (p) {free(p); p = NULL;} }

typedef struct _E_Rect E_Rect;

struct _E_Rect
{
   int x, y, w, h;
};

#endif

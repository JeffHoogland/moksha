#ifndef ENLIGHTENMENT_H
#define ENLIGHTENMENT_H

#include "../config.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <fnmatch.h>
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif
#include <Imlib2.h>
#include <Evas.h>
#include <Ebits.h>
#include <Ecore.h>
#include <Edb.h>

/* macros for allowing sections of code to be runtime profiled */
#define E_PROF 1
#ifdef E_PROF
extern Evas_List __e_profiles;

typedef struct _e_prof
{
   char  *func;
   double total;
   double t1, t2;
} E_Prof;
#define E_PROF_START(_prof_func) \
{ \
E_Prof __p, *__pp; \
Evas_List __pl; \
__p.func = _prof_func; \
__p.total = 0.0; \
__p.t1 = e_get_time(); \
__p.t2 = 0.0;

#define E_PROF_STOP \
__p.t2 = e_get_time(); \
for (__pl = __e_profiles; __pl; __pl = __pl->next) \
{ \
__pp = __pl->data; \
if (!strcmp(__p.func, __pp->func)) \
{ \
__pp->total += (__p.t2 - __p.t1); \
break; \
} \
} \
if (!__pl) \
{ \
__pp = NEW(E_Prof, 1); \
__pp->func = __p.func; \
__pp->total = __p.t2 - __p.t1; \
__pp->t1 = 0.0; \
__pp->t2 = 0.0; \
__e_profiles = evas_list_append(__e_profiles, __pp); \
} \
}
#define E_PROF_DUMP \
{ \
Evas_List __pl; \
for (__pl = __e_profiles; __pl; __pl = __pl->next) \
{ \
E_Prof *__p; \
__p = __pl->data; \
printf("%3.3f : %s()\n", __p->total, __p->func); \
} \
}
#else
#define E_PROF_START(_prof_func)
#define E_PROF_STOP
#define E_PROF_DUMP
#endif

/* object macros */
#define OBJ_REF(_e_obj) _e_obj->references++
#define OBJ_UNREF(_e_obj) _e_obj->references--
#define OBJ_IF_FREE(_e_obj) if (_e_obj->references == 0)
#define OBJ_FREE(_e_obj) _e_obj->e_obj_free(_e_obj)
#define OBJ_DO_FREE(_e_obj) \
OBJ_UNREF(_e_obj); \
OBJ_IF_FREE(_e_obj) \
{ \
OBJ_FREE(_e_obj); \
}
#define OBJ_PROPERTIES \
int references; \
void (*e_obj_free) (void *e_obj);
#define OBJ_INIT(_e_obj, _e_obj_free_func) \
{ \
_e_obj->references = 1; \
_e_obj->e_obj_free = (void *) _e_obj_free_func; \
}

/* action type macros */
#define ACT_MOUSE_IN      0
#define ACT_MOUSE_OUT     1
#define ACT_MOUSE_CLICK   2
#define ACT_MOUSE_DOUBLE  3
#define ACT_MOUSE_TRIPLE  4
#define ACT_MOUSE_UP      5
#define ACT_MOUSE_CLICKED 6
#define ACT_MOUSE_MOVE    7
#define ACT_KEY_DOWN      8
#define ACT_KEY_UP        9

/* misc util macros */
#define INTERSECTS(x, y, w, h, xx, yy, ww, hh) \
((x < (xx + ww)) && \
(y < (yy + hh)) && \
((x + w) > xx) && \
((y + h) > yy))
#define SPANS_COMMON(x1, w1, x2, w2) \
(!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
#define UN(_blah) _blah = 0

/* data types */
typedef struct _E_Action              E_Action;
typedef struct _E_Action_Proto        E_Action_Proto;
typedef struct _E_Active_Action_Timer E_Active_Action_Timer;
typedef struct _E_Background          E_Background;
typedef struct _E_Border              E_Border;
typedef struct _E_Build_Menu          E_Build_Menu;
typedef struct _E_Config_File         E_Config_File;
typedef struct _E_Config_Element      E_Config_Element;
typedef struct _E_Desktop             E_Desktop;
typedef struct _E_Entry               E_Entry;
typedef struct _E_FS_Restarter        E_FS_Restarter;
typedef struct _E_Grab                E_Grab;
typedef struct _E_Icon                E_Icon;
typedef struct _E_Menu                E_Menu;
typedef struct _E_Menu_Item           E_Menu_Item;
typedef struct _E_Object              E_Object;
typedef struct _E_Rect                E_Rect;
typedef struct _E_View                E_View;

/* actual fdata struct members */
struct _E_Object
{
   OBJ_PROPERTIES;
};

#endif

#ifndef ENLIGHTENMENT_H
#define ENLIGHTENMENT_H

#include "../config.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
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

#ifndef PATH_MAX
#define PATH_MAX     4095
#endif

#if 0
#include <execinfo.h>
#define BT \
{ \
void *__BT_array[250]; \
int __BT_n = backtrace(__BT_array,250); \
backtrace_symbols_fd(__BT_array, __BT_n, fileno(stdout)); \
}
#endif

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

/* Observer macros */
#define OBS_PROPERTIES \
Evas_List watched; \
Eevent_Type e_event; \
void (*e_obs_notify)(void *_e_obs, void *_e_obj); \
void (*e_obs_free)(void *_e_obs);
#define OBS_INIT(_e_obs, _e_event, _e_obs_notify, _e_obs_free) \
{ \
    _e_obs->watched = NULL; \
    _e_obs->e_event = _e_event; \
    _e_obs->e_obs_notify = _e_obs_notify; \
    _e_obs->e_obs_free = _e_obs_free; \
}
#define OBS_REGISTER(_e_obs, _e_obj) \
{ \
    _e_obj->observers = evas_list_append(_e_obj->observers, _e_obs); \
    _e_obs->watched = evas_list_append(_e_obs->watched, _e_obj); \
}
#define OBS_UNREGISTER(_e_obj, _e_obs) \
{ \
    _e_obj->observers = evas_list_remove(_e_obj->observers, _e_obs); \
    _e_obs->watched = evas_list_remove(_e_obs->watched, _e_obj); \
}
#define OBS_NOTIFY(_e_obj, _e_event) \
{ \
    Evas_List obs; \
    obs = _e_obj->observers; \
    while (obs) { \
	if (((E_Observer *)obs->data)->e_event == EV_MAX || \
	    ((E_Observer *)obs->data)->e_event == _e_event) \
	  ((E_Observer *)obs->data)->e_obs_notify(obs->data, _e_obj); \
	obs = obs->next; \
    } \
}
#define OBS_FREE(_e_obs) \
{ \
    E_Object *_e_obj; \
    E_Observer *_notify; \
    while (_e_obs->watched) { \
	_e_obj = _e_obj->watched->data; \
	OBS_UNREGISTER(_e_obj, _e_obs); \
	_e_obs->e_obs_free(_notify); \
    } \
}

/* object macros */
#define OBJ_REF(_e_obj) _e_obj->references++
#define OBJ_UNREF(_e_obj) _e_obj->references--
#define OBJ_IF_FREE(_e_obj) if (_e_obj->references == 0)
#define OBJ_FREE(_e_obj) \
{ \
    E_Observer *act; \
    while (_e_obj->observers) { \
	act = _e_obj->observers->data; \
	OBS_UNREGISTER(_e_obj, act); \
    } \
    _e_obj->e_obj_free(_e_obj); \
}
#define OBJ_DO_FREE(_e_obj) \
OBJ_UNREF(_e_obj); \
OBJ_IF_FREE(_e_obj) \
{ \
OBJ_FREE(_e_obj); \
}
#define OBJ_PROPERTIES \
int references; \
void (*e_obj_free) (void *e_obj); \
Evas_List observers;
#define OBJ_INIT(_e_obj, _e_obj_free_func) \
{ \
_e_obj->references = 1; \
_e_obj->e_obj_free = (void *) _e_obj_free_func; \
}

/* misc util macros */
#define INTERSECTS(x, y, w, h, xx, yy, ww, hh) \
(((x) < ((xx) + (ww))) && \
((y) < ((yy) + (hh))) && \
(((x) + (w)) > (xx)) && \
(((y) + (h)) > (yy)))
#define SPANS_COMMON(x1, w1, x2, w2) \
(!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
#define UN(_blah) _blah = 0

/* data type prototypes... not actually used */
typedef struct _E_Object              E_Object;
typedef struct _E_Observer	      E_Observer;

/* actual data struct members */
struct _E_Object
{
   OBJ_PROPERTIES;
};

struct _E_Observer
{
   OBS_PROPERTIES;
};

#endif

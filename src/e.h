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
#define PATH_MAX     4096
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
__p.t1 = ecore_get_time(); \
__p.t2 = 0.0;

#define E_PROF_STOP \
__p.t2 = ecore_get_time(); \
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

#endif

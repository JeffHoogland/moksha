/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_H
#define E_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define USE_IPC
#if 0
#define OBJECT_PARANOIA_CHECK
#define OBJECT_CHECK
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS  64
#endif

#ifdef __linux__
#include <features.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <utime.h>
#include <dlfcn.h>
#include <math.h>
#include <fnmatch.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <glob.h>
#include <locale.h>
#include <libintl.h>
#include <errno.h>
#include <signal.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_VALGRIND
# include <memcheck.h>
#endif

#ifdef __GLIBC__
#ifdef OBJECT_PARANOIA_CHECK
#include <execinfo.h>
#include <setjmp.h>
#endif
#endif

#include <Eina.h>
#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Ecore.h>
#include <Ecore_Str.h>
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#include <Ecore_X_Cursor.h>
#include <Ecore_Evas.h>
#include <Ecore_Con.h>
#include <Ecore_Ipc.h>
#include <Ecore_Job.h>
#include <Ecore_File.h>
#include <Eet.h>
#include <Edje.h>
#include <Efreet.h>
#include <Efreet_Mime.h>
#ifdef HAVE_EDBUS
#include <E_DBus.h>
#endif

#ifdef EAPI
#undef EAPI
#endif
#ifdef WIN32
# ifdef BUILDING_DLL
#  define EAPI __declspec(dllexport)
# else
#  define EAPI __declspec(dllimport)
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
/* BROKEN in gcc 4 on amd64 */
#if 0
#   pragma GCC visibility push(hidden)
#endif
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

typedef struct _E_Before_Idler E_Before_Idler;
typedef struct _E_Rect E_Rect;

/* convenience macro to compress code and avoid typos */
#define E_FN_DEL(_fn, _h) if (_h) { _fn(_h); _h = NULL; }
#define E_INTERSECTS(x, y, w, h, xx, yy, ww, hh) (((x) < ((xx) + (ww))) && ((y) < ((yy) + (hh))) && (((x) + (w)) > (xx)) && (((y) + (h)) > (yy)))
#define E_INSIDE(x, y, xx, yy, ww, hh) (((x) < ((xx) + (ww))) && ((y) < ((yy) + (hh))) && ((x) >= (xx)) && ((y) >= (yy)))
#define E_CONTAINS(x, y, w, h, xx, yy, ww, hh) (((xx) >= (x)) && (((x) + (w)) >= ((xx) + (ww))) && ((yy) >= (y)) && (((y) + (h)) >= ((yy) + (hh))))
#define E_SPANS_COMMON(x1, w1, x2, w2) (!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
#define E_REALLOC(p, s, n) p = (s *)realloc(p, sizeof(s) * n)
#define E_NEW(s, n) (s *)calloc(n, sizeof(s))
#define E_NEW_BIG(s, n) (s *)malloc(n * sizeof(s))
#define E_FREE(p) do { if (p) {free(p); p = NULL;} } while (0)
#define E_FREE_LIST(list, free) \
  do \
    { \
       if (list) \
	 { \
	    Eina_List *tmp; \
	    tmp = list; \
	    list = NULL; \
	    while (tmp) \
	      { \
		 free(tmp->data); \
		 tmp = eina_list_remove_list(tmp, tmp); \
	      } \
	 } \
    } \
  while (0)

#define E_CLAMP(x, min, max) (x < min ? min : (x > max ? max : x))
#define E_RECTS_CLIP_TO_RECT(_x, _y, _w, _h, _cx, _cy, _cw, _ch) \
   { \
      if (E_INTERSECTS(_x, _y, _w, _h, _cx, _cy, _cw, _ch)) \
	{ \
	   if (_x < (_cx)) \
	     { \
		_w += _x - (_cx); \
		_x = (_cx); \
		if (_w < 0) _w = 0; \
	     } \
	   if ((_x + _w) > ((_cx) + (_cw))) \
	     _w = (_cx) + (_cw) - _x; \
	   if (_y < (_cy)) \
	     { \
		_h += _y - (_cy); \
		_y = (_cy); \
		if (_h < 0) _h = 0; \
	     } \
	   if ((_y + _h) > ((_cy) + (_ch))) \
	     _h = (_cy) + (_ch) - _y; \
	} \
      else \
	{ \
	   _w = 0; _h = 0; \
	} \
   }

#define E_REMOTE_OPTIONS 1
#define E_REMOTE_OUT     2
#define E_WM_IN          3
#define E_REMOTE_IN      4
#define E_ENUM           5
#define E_LIB_IN         6

#define E_TYPEDEFS 1
#include "e_includes.h"
#undef E_TYPEDEFS
#include "e_includes.h"

EAPI E_Before_Idler *e_main_idler_before_add(int (*func) (void *data), void *data, int once);
EAPI void            e_main_idler_before_del(E_Before_Idler *eb);


struct _E_Before_Idler
{
   int          (*func) (void *data);
   void          *data;
   unsigned char  once : 1;
   unsigned char  delete_me : 1;
};

struct _E_Rect
{
   int x, y, w, h;
};

extern EAPI E_Path *path_data;
extern EAPI E_Path *path_images;
extern EAPI E_Path *path_fonts;
extern EAPI E_Path *path_themes;
extern EAPI E_Path *path_icons;
extern EAPI E_Path *path_modules;
extern EAPI E_Path *path_backgrounds;
extern EAPI E_Path *path_messages;
extern EAPI int     restart;
extern EAPI int     good;
extern EAPI int     evil;
extern EAPI int     starting;
extern EAPI int     stopping;

#endif

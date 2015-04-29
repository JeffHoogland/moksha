#ifndef E_H
# define E_H

/**
 * @defgroup API Enlightenment API
 *
 * Application programming interface to be used by modules to extend
 * Enlightenment.
 *
 * @{
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# define USE_IPC
# if 0
#  define OBJECT_PARANOIA_CHECK
#  define OBJECT_CHECK
# endif

# ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 64
# endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif !defined alloca
# ifdef __GNUC__
#  define alloca __builtin_alloca
# elif defined _AIX
#  define alloca __alloca
# elif defined _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# elif !defined HAVE_ALLOCA
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
#endif

# ifdef __linux__
#  include <features.h>
# endif

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/param.h>
# include <sys/resource.h>
# include <utime.h>
# include <dlfcn.h>
# include <math.h>
# include <fcntl.h>
# include <fnmatch.h>
# include <limits.h>
# include <ctype.h>
# include <time.h>
# include <dirent.h>
# include <pwd.h>
# include <grp.h>
# include <glob.h>
# include <locale.h>
# include <errno.h>
# include <signal.h>
# include <inttypes.h>

# ifdef HAVE_GETTEXT
#  include <libintl.h>
# endif

# ifndef _POSIX_HOST_NAME_MAX
#  define _POSIX_HOST_NAME_MAX 255
# endif

# ifdef HAVE_VALGRIND
#  include <memcheck.h>
# endif

# ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
# endif

# include <setjmp.h>
# include <Eina.h>
# include <Eet.h>
# include <Evas.h>
# include <Evas_Engine_Buffer.h>
# include <Ecore.h>
# include <Ecore_X.h>
# include <Ecore_Evas.h>
# include <Ecore_Input.h>
# include <Ecore_Input_Evas.h>
# include <Ecore_Con.h>
# include <Ecore_Ipc.h>
# include <Ecore_File.h>
# include <Efreet.h>
# include <Efreet_Mime.h>
# include <Edje.h>
# include <E_DBus.h>
# include <Eio.h>

# ifdef HAVE_HAL
#  include <E_Hal.h>
# endif

# ifdef EAPI
#  undef EAPI
# endif
# ifdef WIN32
#  ifdef BUILDING_DLL
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI __declspec(dllimport)
#  endif
# else
#  ifdef __GNUC__
#   if __GNUC__ >= 4
/* BROKEN in gcc 4 on amd64 */
#    if 0
#     pragma GCC visibility push(hidden)
#    endif
#    define EAPI __attribute__ ((visibility("default")))
#   else
#    define EAPI
#   endif
#  else
#   define EAPI
#  endif
# endif

# ifdef EINTERN
#  undef EINTERN
# endif
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EINTERN __attribute__ ((visibility("hidden")))
#  else
#   define EINTERN
#  endif
# else
#  define EINTERN
# endif

#ifndef strdupa
# define strdupa(str)       strcpy(alloca(strlen(str) + 1), str)
#endif

#ifndef strndupa
# define strndupa(str, len) strncpy(alloca(len + 1), str, len)
#endif

typedef struct _E_Before_Idler E_Before_Idler;
typedef struct _E_Rect         E_Rect;

/* convenience macro to compress code and avoid typos */
#ifndef MAX
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
# define E_FN_DEL(_fn, _h) if (_h) { _fn((void*)_h); _h = NULL; }
# define E_INTERSECTS(x, y, w, h, xx, yy, ww, hh) \
  (((x) < ((xx) + (ww))) && ((y) < ((yy) + (hh))) && (((x) + (w)) > (xx)) && (((y) + (h)) > (yy)))
# define E_INSIDE(x, y, xx, yy, ww, hh) \
  (((x) < ((xx) + (ww))) && ((y) < ((yy) + (hh))) && ((x) >= (xx)) && ((y) >= (yy)))
# define E_CONTAINS(x, y, w, h, xx, yy, ww, hh) \
  (((xx) >= (x)) && (((x) + (w)) >= ((xx) + (ww))) && ((yy) >= (y)) && (((y) + (h)) >= ((yy) + (hh))))
# define E_SPANS_COMMON(x1, w1, x2, w2) \
  (!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
# define E_REALLOC(p, s, n)   p = (s *)realloc(p, sizeof(s) * n)
# define E_NEW(s, n)          (s *)calloc(n, sizeof(s))
# define E_NEW_RAW(s, n)      (s *)malloc(n * sizeof(s))
# define E_FREE(p)            do { free(p); p = NULL; } while (0)
# define E_FREE_LIST(list, free)    \
  do                                \
    {                               \
       void *_tmp_;                 \
       EINA_LIST_FREE(list, _tmp_) \
         {                          \
            free(_tmp_);            \
         }                          \
    }                               \
  while (0)

# define E_LIST_FOREACH(list, func)    \
  do                                \
    {                               \
       void *_tmp_;                 \
       const Eina_List *_list, *_list2;  \
       EINA_LIST_FOREACH_SAFE(list, _list, _list2, _tmp_) \
         {                          \
            func(_tmp_);            \
         }                          \
    }                               \
  while (0)

# define E_LIST_HANDLER_APPEND(list, type, callback, data) \
  do \
    { \
       Ecore_Event_Handler *_eh; \
       _eh = ecore_event_handler_add(type, (Ecore_Event_Handler_Cb)callback, data); \
       if (_eh) \
         list = eina_list_append(list, _eh); \
       else \
         fprintf(stderr, "E_LIST_HANDLER_APPEND\n"); \
    } \
  while (0)

# define E_CLAMP(x, min, max) (x < min ? min : (x > max ? max : x))
# define E_RECTS_CLIP_TO_RECT(_x, _y, _w, _h, _cx, _cy, _cw, _ch) \
  {                                                               \
     if (E_INTERSECTS(_x, _y, _w, _h, _cx, _cy, _cw, _ch))        \
       {                                                          \
          if ((int)_x < (int)(_cx))                               \
            {                                                     \
               _w += _x - (_cx);                                  \
               _x = (_cx);                                        \
               if ((int)_w < 0) _w = 0;                           \
            }                                                     \
          if ((int)(_x + _w) > (int)((_cx) + (_cw)))              \
            _w = (_cx) + (_cw) - _x;                              \
          if ((int)_y < (int)(_cy))                               \
            {                                                     \
               _h += _y - (_cy);                                  \
               _y = (_cy);                                        \
               if ((int)_h < 0) _h = 0;                           \
            }                                                     \
          if ((int)(_y + _h) > (int)((_cy) + (_ch)))              \
            _h = (_cy) + (_ch) - _y;                              \
       }                                                          \
     else                                                         \
       {                                                          \
          _w = 0; _h = 0;                                         \
       }                                                          \
  }

#if (EINA_VERSION_MAJOR == 1) && (EINA_VERSION_MINOR < 8)
# define eina_list_last_data_get(X) eina_list_data_get(eina_list_last(X))
#endif

# define E_REMOTE_OPTIONS 1
# define E_REMOTE_OUT     2
# define E_WM_IN          3
# define E_REMOTE_IN      4
# define E_ENUM           5
# define E_LIB_IN         6

# define E_TYPEDEFS       1
# include "e_includes.h"
# undef E_TYPEDEFS
# include "e_includes.h"

EAPI E_Before_Idler *e_main_idler_before_add(int (*func)(void *data), void *data, int once);
EAPI void            e_main_idler_before_del(E_Before_Idler *eb);
EAPI double          e_main_ts(const char *str);

struct _E_Before_Idler
{
   int       (*func)(void *data);
   void     *data;
   Eina_Bool once : 1;
   Eina_Bool delete_me : 1;
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
extern EAPI Eina_Bool good;
extern EAPI Eina_Bool evil;
extern EAPI Eina_Bool starting;
extern EAPI Eina_Bool stopping;
extern EAPI Eina_Bool restart;
extern EAPI Eina_Bool e_nopause;

extern EAPI Eina_Bool e_precache_end;
extern EAPI Eina_Bool x_fatal;

EAPI void e_alert_composite_win(Ecore_X_Window root, Ecore_X_Window win);

//#define SMARTERR(args...) abort()
#define SMARTERRNR() return
#define SMARTERR(x)  return x

/**
 * @}
 */

/**
 * @defgroup Optional_Modules Optional Modules
 * @{
 *
 * @defgroup Optional_Conf Configurations
 * @defgroup Optional_Control Controls
 * @defgroup Optional_Devices Devices & Hardware
 * @defgroup Optional_Fileman File Managers
 * @defgroup Optional_Gadgets Gadgets
 * @defgroup Optional_Launcher Launchers
 * @defgroup Optional_Layouts Layout Managers
 * @defgroup Optional_Look Look & Feel
 * @defgroup Optional_Monitors Monitors & Notifications
 * @defgroup Optional_Mobile Mobile Specific Extensions
 * @}
 */

#endif

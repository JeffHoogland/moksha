/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Stolen_Window E_Stolen_Window;

struct _E_Stolen_Window
{
   Ecore_X_Window win;
   int refcount;
};

static Evas_Hash *_e_stolen_windows = NULL;

/* externally accessible functions */
EAPI int
e_stolen_win_get(Ecore_X_Window win)
{
   E_Stolen_Window *esw;
   
   esw = evas_hash_find(_e_stolen_windows, e_util_winid_str_get(win));
   if (esw) return 1;
   return 0;
}

EAPI void
e_stolen_win_add(Ecore_X_Window win)
{
   E_Stolen_Window *esw;
   
   esw = evas_hash_find(_e_stolen_windows, e_util_winid_str_get(win));
   if (esw)
     {
	esw->refcount++;
     }
   else
     {
	esw = E_NEW(E_Stolen_Window, 1);
	esw->win = win;
	esw->refcount = 1;
	_e_stolen_windows = evas_hash_add(_e_stolen_windows, e_util_winid_str_get(win), esw);
     }
   return;
}

EAPI void
e_stolen_win_del(Ecore_X_Window win)
{
   E_Stolen_Window *esw;
   
   esw = evas_hash_find(_e_stolen_windows, e_util_winid_str_get(win));
   if (esw)
     {
	esw->refcount--;
	if (esw->refcount == 0)
	  {
	     _e_stolen_windows = evas_hash_del(_e_stolen_windows, e_util_winid_str_get(win), esw);
	     free(esw);
	  }
     }
   return;
}

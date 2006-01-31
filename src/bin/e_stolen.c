/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static char *_e_stolen_winid_str_get(Ecore_X_Window win);

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
   
   esw = evas_hash_find(_e_stolen_windows, _e_stolen_winid_str_get(win));
   if (esw) return 1;
   return 0;
}

EAPI void
e_stolen_win_add(Ecore_X_Window win)
{
   E_Stolen_Window *esw;
   
   esw = evas_hash_find(_e_stolen_windows, _e_stolen_winid_str_get(win));
   if (esw)
     {
	esw->refcount++;
     }
   else
     {
	esw = E_NEW(E_Stolen_Window, 1);
	esw->win = win;
	esw->refcount = 1;
	_e_stolen_windows = evas_hash_add(_e_stolen_windows, _e_stolen_winid_str_get(win), esw);
     }
   return;
}

EAPI void
e_stolen_win_del(Ecore_X_Window win)
{
   E_Stolen_Window *esw;
   
   esw = evas_hash_find(_e_stolen_windows, _e_stolen_winid_str_get(win));
   if (esw)
     {
	esw->refcount--;
	if (esw->refcount == 0)
	  {
	     _e_stolen_windows = evas_hash_del(_e_stolen_windows, _e_stolen_winid_str_get(win), esw);
	     free(esw);
	  }
     }
   return;
}

/* local subsystem functions */
static char *
_e_stolen_winid_str_get(Ecore_X_Window win)
{
   const char *vals = "qWeRtYuIoP5-$&<~";
   static char id[9];
   unsigned int val;
   
   val = (unsigned int)win;
   id[0] = vals[(val >> 28) & 0xf];
   id[1] = vals[(val >> 24) & 0xf];
   id[2] = vals[(val >> 20) & 0xf];
   id[3] = vals[(val >> 16) & 0xf];
   id[4] = vals[(val >> 12) & 0xf];
   id[5] = vals[(val >>  8) & 0xf];
   id[6] = vals[(val >>  4) & 0xf];
   id[7] = vals[(val      ) & 0xf];
   id[8] = 0;
   return id;
}

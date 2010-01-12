#include "e.h"
#include "e_mod_main.h"
#include "e_mod_win.h"

/* local function prototypes */
static int _cb_screens_sort(const void *data, const void *data2);

/* local variables */
static Eina_List *iwins = NULL;
const char *mod_dir = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Indicator" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Screen *screen;
   Eina_List *l, *screens;

   mod_dir = eina_stringshare_add(m->dir);
   screens = (Eina_List *)e_xinerama_screens_get();
   //   screens = eina_list_sort(screens, 0, _cb_screens_sort);
   EINA_LIST_FOREACH(screens, l, screen) 
     {
        Il_Ind_Win *iwin = NULL;

        if (!(iwin = e_mod_win_new(screen))) continue;
        iwins = eina_list_append(iwins, iwin);
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   Il_Ind_Win *iwin;

   EINA_LIST_FREE(iwins, iwin) 
     {
        e_object_del(E_OBJECT(iwin));
        iwin = NULL;
     }

   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}

/* local function prototypes */
static int 
_cb_screens_sort(const void *data, const void *data2) 
{
   E_Screen *s1, *s2;

   if (!(s1 = (E_Screen *)data)) return -1;
   if (!(s2 = (E_Screen *)data2)) return 1;
   return s2->escreen - s1->escreen;
}

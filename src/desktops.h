#ifndef E_DESKTOPS_H
#define E_DESKTOPS_H

#include "e.h"
#include "border.h"
#include "object.h"

#ifndef E_DESKTOPS_TYPEDEF
#define E_DESKTOPS_TYPEDEF
typedef struct _E_Desktop E_Desktop;
#endif

#include "view_layout.h"

#ifndef E_DESKTOPS_ADAPTOR_TYPEDEF
#define E_DESKTOPS_ADAPTOR_TYPEDEF
typedef struct _E_Desktop_Adaptor E_Desktop_Adaptor;
#endif

struct _E_Desktop
{
   E_Observee          obs;

   char               *name;
   char               *dir;
/*   struct
   {
      Window              main;
   }
   win; */
   int                 x, y;
   struct
   {
      int                 w, h;
   }
   real               , virt;
   Evas_List *          windows;
   struct
   {
      int                 desk;
      struct
      {
	 int                 x, y;
      }
      area;
   }
   desk;
   E_Desktop_Adaptor  *adaptor;
   Evas               *evas;
   E_View_Layout      *layout;
   E_View_Look        *look;
   E_Background        bg;
   E_Iconbar          *iconbar;
/*   int                 changed; */
};

struct _E_Desktop_Adaptor
{
   E_Observer          obsvr;
   E_Desktop          *desktop;
};

/**
 * e_desktops_init - Initializes desktop handling.
 *
 * This function creates the necessary windows for desktop handling,
 * and makes sure they're visible and lowered. It does not handle
 * the loading of background graphics information. It also makes
 * sure E rregisters itself as being compatible to GNOME, KDE etc.
 */
void                e_desktops_init(void);

void                e_desktops_scroll(E_Desktop * desk, int dx, int dy);
void                e_desktops_free(E_Desktop * desk);

E_Desktop          *e_desktops_new(int i);
void                e_desktops_add_border(E_Desktop * d, E_Border * b);
void                e_desktops_add_sticky(E_Border *b);
void                e_desktops_rm_sticky(E_Border *b);
void                e_desktops_del_border(E_Desktop * d, E_Border * b);
void                e_desktops_delete(E_Desktop * d);
void                e_desktops_show(E_Desktop * d);
void                e_desktops_hide(E_Desktop * d);

/**
 * e_desktops_get_num - Returns number of desktops.
 */
int                 e_desktops_get_num(void);

/**
 * e_desktops_get - Returns nth desktop
 * @d:       The number of the desktop to get
 *
 * The desktops are stored in a linked list. This function
 * returns the nth of those desktops, NULL if no desktop
 * was found at that index.
 */
E_Desktop          *e_desktops_get(int d);

int                 e_desktops_get_current(void);
void                e_desktops_goto_desk(int d);
void                e_desktops_goto(int d, int ax, int ay);
Evas_List          *e_desktops_get_desktops_list();
void                e_desktop_bg_reload(E_Desktop * d);
void                e_desktop_ib_reload(E_Desktop * d);
void                e_desktop_update(E_Desktop *d);
E_Border           *e_desktop_raise_next_border(void);
Window              e_desktop_window();

#endif

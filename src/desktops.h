#ifndef E_DESKTOPS_H
#define E_DESKTOPS_H

#include "e.h"
#include "view.h"
#include "border.h"

#ifndef E_DESKTOPS_TYPEDEF
#define E_DESKTOPS_TYPEDEF
typedef struct _E_Desktop             E_Desktop;
#endif

struct _E_Desktop
{
   OBJ_PROPERTIES;
   
   char *name;
   char *dir;
   struct {
      Window main;
      Window container;
   } win;
   int x, y;
   struct {
      int w, h;
   } real, virt;
   Evas_List windows;
   struct {
      int desk;
      struct {
	 int x, y;
      } area;
   } desk;
   int changed;
};

/**
 * e_desktops_init - Initializes desktop handling.
 *
 * This function creates the necessary windows for desktop handling,
 * and makes sure they're visible and lowered. It does not handle
 * the loading of background graphics information. It also makes
 * sure E rregisters itself as being compatible to GNOME, KDE etc.
 */
void         e_desktops_init(void);

void         e_desktops_scroll(E_Desktop *desk, int dx, int dy);
void         e_desktops_free(E_Desktop *desk);
void         e_desktops_init_file_display(E_Desktop *desk);
E_Desktop   *e_desktops_new(void);
void         e_desktops_add_border(E_Desktop *d, E_Border *b);
void         e_desktops_del_border(E_Desktop *d, E_Border *b);
void         e_desktops_delete(E_Desktop *d);
void         e_desktops_show(E_Desktop *d);
void         e_desktops_hide(E_Desktop *d);
int          e_desktops_get_num(void);
E_Desktop   *e_desktops_get(int d);
int          e_desktops_get_current(void);
void         e_desktops_goto_desk(int d);
void         e_desktops_goto(int d, int ax, int ay);

#endif

#ifndef E_MENU_BUILD_H
#define E_MENU_BUILD_H

#include "e.h"
#include "object.h"
#include "observer.h"

typedef struct _E_Build_Menu          E_Build_Menu;

struct _E_Build_Menu
{
   E_Observer o;

   char      *file;
   time_t     mod_time;
   
   E_Menu    *menu;
   
   Evas_List  menus;
   Evas_List  commands;

   int changed;
};

E_Build_Menu *e_build_menu_new_from_db(char *file);
E_Build_Menu *e_build_menu_new_from_gnome_apps(char *dir);
E_Build_Menu *e_build_menu_new_from_iconified_borders();

void e_build_menu_iconified_borders_rebuild(E_Build_Menu *bm);
#endif

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define DEFAULT_MENU_PLACEMENT "main/2"

/********************************************************************/
/** Data Structure used by e17's main menu functions               **/
/**   see .../src/bin/e_int_menus.c                                **/
/********************************************************************/

typedef struct _Main_Data Main_Data;

struct _Main_Data
{
   E_Menu *menu;
   E_Menu *apps;
   E_Menu *all_apps;
   E_Menu *desktops;
   E_Menu *clients;
   E_Menu *enlightenment;
   E_Menu *config;
   E_Menu *lost_clients;
};

/********************************************************************/
/** Setup the E Module Version                                     **/
/********************************************************************/

EAPI extern E_Module_Api e_modapi;

/********************************************************************/
/** E API Module Interface Declarations                            **/
/********************************************************************/

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m __UNUSED__);
EAPI int   e_modapi_save     (E_Module *m __UNUSED__);

#endif

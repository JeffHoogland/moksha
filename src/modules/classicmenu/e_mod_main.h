/*  Copyright (C) 2015 Bodhi Linux (see AUTHORS)
 *
 *  This file is a part of classicmenu.
 *  classicmenu is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  classicmenu is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with classicmenu.  If not, see <http://www.gnu.org/licenses/>.
 */

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

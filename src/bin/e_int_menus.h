#ifndef E_INT_MENUS_H
#define E_INT_MENUS_H

EAPI E_Menu *e_int_menus_main_new(void);    
EAPI E_Menu *e_int_menus_desktops_new(void);
EAPI E_Menu *e_int_menus_clients_new(void);
EAPI E_Menu *e_int_menus_apps_new(char *dir, int top);
EAPI E_Menu *e_int_menus_favorite_apps_new(int top);
    
#endif

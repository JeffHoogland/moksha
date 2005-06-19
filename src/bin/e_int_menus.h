/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_INT_MENUS_H
#define E_INT_MENUS_H

EAPI E_Menu *e_int_menus_main_new(void);    
EAPI E_Menu *e_int_menus_desktops_new(void);
EAPI E_Menu *e_int_menus_clients_new(void);
EAPI E_Menu *e_int_menus_apps_new(char *dir);
EAPI E_Menu *e_int_menus_favorite_apps_new(void);
EAPI E_Menu *e_int_menus_config_apps_new(void);
EAPI E_Menu *e_int_menus_gadgets_new(void);
EAPI E_Menu *e_int_menus_themes_new(void);
EAPI E_Menu *e_int_menus_lost_clients_new(void);
    
#endif
#endif

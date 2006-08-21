/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Int_Menu_Augmentation E_Int_Menu_Augmentation;

#else
#ifndef E_INT_MENUS_H
#define E_INT_MENUS_H

struct _E_Int_Menu_Augmentation
{
   struct {
	void (*func)(void *data, E_Menu *m);
	void *data;
   } add, del;
};

EAPI E_Menu *e_int_menus_main_new(void);    
EAPI E_Menu *e_int_menus_desktops_new(void);
EAPI E_Menu *e_int_menus_clients_new(void);
EAPI E_Menu *e_int_menus_apps_new(const char *dir);
EAPI E_Menu *e_int_menus_favorite_apps_new(void);
EAPI E_Menu *e_int_menus_all_apps_new(void);
EAPI E_Menu *e_int_menus_config_new(void);
EAPI E_Menu *e_int_menus_gadgets_new(void);
EAPI E_Menu *e_int_menus_themes_new(void);
EAPI E_Menu *e_int_menus_lost_clients_new(void);

EAPI E_Int_Menu_Augmentation *e_int_menus_menu_augmentation_add(const char *menu,
								void (*func_add) (void *data, E_Menu *m),
								void *data_add,
								void (*func_del) (void *data, E_Menu *m),
								void *data_del);
EAPI void                     e_int_menus_menu_augmentation_del(const char *menu,
								E_Int_Menu_Augmentation *maug);
    
#endif
#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Int_Menu_Augmentation E_Int_Menu_Augmentation;

#else
#ifndef E_INT_MENUS_H
#define E_INT_MENUS_H

#define E_CLIENTLIST_GROUP_NONE 0
#define E_CLIENTLIST_GROUP_DESK 1
#define E_CLIENTLIST_GROUP_CLASS 2

#define E_CLIENTLIST_GROUP_SEP_NONE 0
#define E_CLIENTLIST_GROUP_SEP_BAR 1
#define E_CLIENTLIST_GROUP_SEP_MENU 2

#define E_CLIENTLIST_SORT_NONE 0
#define E_CLIENTLIST_SORT_ALPHA 1
#define E_CLIENTLIST_SORT_ZORDER 2
#define E_CLIENTLIST_SORT_MOST_RECENT 3

#define E_CLIENTLIST_GROUPICONS_OWNER 0
#define E_CLIENTLIST_GROUPICONS_CURRENT 1
#define E_CLIENTLIST_GROUPICONS_SEP 2

#define E_CLIENTLIST_MAX_CAPTION_LEN 256

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
EAPI E_Menu *e_int_menus_lost_clients_new(void);
EAPI E_Menu *e_int_menus_shelves_new(void);
EAPI E_Menu *e_int_menus_sys_new(void);

EAPI E_Int_Menu_Augmentation *e_int_menus_menu_augmentation_add(const char *menu,
								void (*func_add) (void *data, E_Menu *m),
								void *data_add,
								void (*func_del) (void *data, E_Menu *m),
								void *data_del);
EAPI void                     e_int_menus_menu_augmentation_del(const char *menu,
								E_Int_Menu_Augmentation *maug);
    
#endif
#endif

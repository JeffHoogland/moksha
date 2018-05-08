#include <e.h>
#include "e_mod_main.h"
#include "config.h"

/* Local Function Prototypes */
static void _e_mod_menu_desktop_add(void *data __UNUSED__, E_Menu *m);
static void _e_mod_menu_win_add(void *data __UNUSED__, E_Menu *m);

/* Local Data */

Main_Data *dat;
static E_Int_Menu_Augmentation *maug_desk = NULL;
static E_Int_Menu_Augmentation *maug_win  = NULL;
E_Menu *subm;

/***************************************************************************/

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Classic Menu"
};

/* Initialize module */
EAPI void *
e_modapi_init(E_Module *m)
{
   maug_desk = e_int_menus_menu_augmentation_add
      (DEFAULT_MENU_PLACEMENT, _e_mod_menu_desktop_add, NULL, NULL, NULL);
   maug_win  = e_int_menus_menu_augmentation_add
      (DEFAULT_MENU_PLACEMENT, _e_mod_menu_win_add, NULL, NULL, NULL);
   return m;
}

/* Unload Module */
EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   if (maug_desk)
   {
      e_int_menus_menu_augmentation_del(DEFAULT_MENU_PLACEMENT, maug_desk);
      maug_desk = NULL;
   }
   if (maug_win)
   {
      e_int_menus_menu_augmentation_del(DEFAULT_MENU_PLACEMENT, maug_win);
      maug_win = NULL;
   }
   e_object_unref(E_OBJECT(dat));
   return 1;
}

/* Save Modules Configuration */
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

/***************************************************************************/

static void
_e_mod_menu_desktop_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   dat = e_object_data_get(E_OBJECT(m));
   subm = e_int_menus_desktops_new();
   dat->desktops = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Desktop"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop");
   e_menu_item_submenu_set(mi, subm);
   e_object_unref(E_OBJECT(subm));

}

static void
_e_mod_menu_win_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   subm = e_int_menus_clients_new();
   dat->desktops = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Windows"));
   e_util_menu_item_theme_icon_set(mi, "preferences-system-windows");
   e_menu_item_submenu_set(mi, subm);
   /* The necessary (??) to prevent
    *  the submenu from having a title.           */
   e_object_data_set(E_OBJECT(subm), dat);

}

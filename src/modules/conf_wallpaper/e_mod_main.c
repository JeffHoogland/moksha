/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static E_Fm2_Mime_Handler *import_hdl = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Wallpaper"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("appearance", 10, _("Look"), NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/wallpaper", 10, _("Wallpaper"), NULL, "enlightenment/background", e_int_config_wallpaper);
   e_configure_registry_category_add("internal", -1, _("Internal"), NULL, "enlightenment/internal");
   e_configure_registry_item_add("internal/wallpaper_desk", -1, _("Wallpaper"), NULL, "enlightenment/windows", e_int_config_wallpaper_desk);
   maug = e_int_menus_menu_augmentation_add("config/1", _e_mod_menu_add, NULL, NULL, NULL);

   import_hdl = e_fm2_mime_handler_new(_("Set As Background"), "enlightenment/background", 
				       e_int_config_wallpaper_handler_set, NULL,
				       e_int_config_wallpaper_handler_test, NULL);
   if (import_hdl) 
     {
	e_fm2_mime_handler_mime_add(import_hdl, "image/png");
	e_fm2_mime_handler_mime_add(import_hdl, "image/jpeg");
     }
   
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/1", maug);
	maug = NULL;
     }
   while ((cfd = e_config_dialog_get("E", "_config_wallpaper_dialog"))) 
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("internal/wallpaper_desk");
   e_configure_registry_category_del("internal");
   e_configure_registry_item_del("appearance/wallpaper");
   e_configure_registry_category_del("appearance");
   
   if (import_hdl) 
     {
	e_fm2_mime_handler_mime_del(import_hdl, "image/png");
	e_fm2_mime_handler_mime_del(import_hdl, "image/jpeg");
	e_fm2_mime_handler_free(import_hdl);
     }
   
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/* menu item callback(s) */
static void 
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_registry_call("appearance/wallpaper", m->zone->container, NULL);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Wallpaper"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-wallpaper");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}

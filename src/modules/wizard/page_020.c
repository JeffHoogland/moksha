/* Profile chooser */
#include "e_wizard.h"

static const char *profile = NULL;

/*
EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops __UNUSED__, Eina_Bool *need_xdg_icons __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   // actually apply profile
   if (ecore_file_exists("/etc/bodhi/iso"))
     {
         profile = "ISO";
     }
   profile = profile ? profile : "bodhi";

   e_config_profile_set(profile);
   e_config_profile_del(e_config_profile_get());
   e_config_load();
   e_config_save();
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}

/*
EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/

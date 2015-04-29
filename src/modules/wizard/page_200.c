/* Delete previous copy of config profile and save new one */
#include "e_wizard.h"

static void
_e_int_menus_bodhi_quick_start(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Zone *zone;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "enlightenment_open "
            "file:///usr/share/bodhi/quickstart/quickstartEN/index.html");
   zone = e_util_zone_current_get(e_manager_current_get());
   e_exec(zone, NULL, buff, NULL, NULL);
}

#if 0
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

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
#endif
EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   // save the config now everyone has modified it
   e_config_save();
   // diusable restart env so we actually start a whole new session properly
   e_util_env_set("E_RESTART", NULL);
   // restart e
   e_sys_action_do(E_SYS_RESTART, NULL);
   _e_int_menus_bodhi_quick_start(NULL, NULL, NULL);
   return 1;
}


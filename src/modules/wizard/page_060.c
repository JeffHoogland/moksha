/* Ask about focus mode */
#include "e_wizard.h"

static int focus_mode = 1;
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
wizard_page_show(E_Wizard_Page *pg)
{
  if (!focus_mode)
     {
        e_config->focus_policy = E_FOCUS_CLICK;
        e_config->focus_setting = E_FOCUS_NEW_WINDOW;
        e_config->pass_click_on = 1;
        e_config->always_click_to_raise = 0;
        e_config->always_click_to_focus = 0;
        e_config->focus_last_focused_per_desktop = 1;
        e_config->pointer_slide = 0;
        e_config->winlist_warp_while_selecting = 0;
        e_config->winlist_warp_at_end = 0;
     }
   else
     {
        e_config->focus_policy = E_FOCUS_SLOPPY;
        e_config->focus_setting = E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED;
        e_config->pass_click_on = 1;
        e_config->always_click_to_raise = 0;
        e_config->always_click_to_focus = 0;
        e_config->focus_last_focused_per_desktop = 1;
        e_config->pointer_slide = 1;
     }
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

/* Ask about focus mode */
#include "e.h"
#include "e_mod_main.h"

static int focus_mode = 1;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;

   if (e_config->focus_policy == E_FOCUS_CLICK) focus_mode = 0;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Window Focus"));
   
   of = e_widget_framelist_add(pg->evas, _("Focus by ..."), 0);

   rg = e_widget_radio_group_new(&focus_mode);
   
   ob = e_widget_radio_add(pg->evas, _("Click"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   ob = e_widget_radio_add(pg->evas, _("Mouse Over"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(of);

   e_wizard_page_show(o);
//   pg->data = o;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg)
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
//   evas_object_del(pg->data);
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

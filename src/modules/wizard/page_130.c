/* Setup if we need backlight? */
#include "e.h"
#include "e_mod_main.h"

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
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   int hav_backlight = 0;

   if (e_backlight_exists()) hav_backlight = 1;
   if (!hav_backlight)
     {
        E_Config_Module *em;
        Eina_List *l;
        
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em->name) continue;
             if (!strcmp(em->name, "backlight"))
               {
                  e_config->modules = eina_list_remove_list
                     (e_config->modules, l);
                  if (em->name) eina_stringshare_del(em->name);
                  free(em);
                  break;
               }
          }
        e_config_save_queue();
     }
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}

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

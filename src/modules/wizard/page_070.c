/* Setup of default icon theme */
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
   Eina_List *l, *themes = efreet_icon_theme_list_get();
   Efreet_Icon_Theme *th;
   int i;
   const char *selected = NULL;
   const char *search[] =
     {
        "gnome",
        "Humanity",
        "Humanity-Dark",
        "ubuntu-mono-light",
        "ubuntu-mono-dark",
        "ubuntu-mono-light",
        "unity-icon-theme",
        NULL
     };
   
   if (!themes) return 0;
   for (i = 0; search[i]; i++)
     {
        EINA_LIST_FOREACH(themes, l, th)
          {
             if (!strcasecmp(search[i], th->name.internal))
               {
                  selected = search[i];
                  goto done;
               }
          }
     }
done:
   if (selected)
     {
        if (e_config->icon_theme) eina_stringshare_del(e_config->icon_theme);
        e_config->icon_theme = eina_stringshare_add(selected);
     }
   eina_list_free(themes);
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

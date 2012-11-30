/* Setup favorites and desktop links */
#include "e_wizard.h"
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
   Eina_List *files;
   char buf[PATH_MAX], buf2[PATH_MAX], *file;

   // make desktop dir
   ecore_file_mkpath(efreet_desktop_dir_get());
   snprintf(buf, sizeof(buf), "%s/desktop", e_wizard_dir_get());
   files = ecore_file_ls(buf);
   if (!files) return 0;
   EINA_LIST_FREE(files, file)
     {
        snprintf(buf, sizeof(buf), "%s/desktop/%s",
                 e_wizard_dir_get(), file);
        snprintf(buf2, sizeof(buf2), "%s/%s",
                 efreet_desktop_dir_get(), file);
        ecore_file_cp(buf, buf2);
        free(file);
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

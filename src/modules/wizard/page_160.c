/* Setup favorites and desktop links */
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
   Eina_List *files;
   char buf[PATH_MAX], buf2[PATH_MAX], *file;

   // make dir for favorites and install ones shipped
   snprintf(buf, sizeof(buf), "%s/fileman/favorites", e_user_dir_get());
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/favorites", e_wizard_dir_get());
   files = ecore_file_ls(buf);
   if (!files) return 0;
   EINA_LIST_FREE(files, file)
     {
        snprintf(buf, sizeof(buf), "%s/favorites/%s", 
                 e_wizard_dir_get(), file);
        snprintf(buf2, sizeof(buf2), "%s/fileman/favorites/%s", 
                 e_user_dir_get(), file);
        ecore_file_cp(buf, buf2);
        free(file);
     }
   // make desktop dir
   e_user_homedir_concat(buf, sizeof(buf), _("Desktop"));
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/desktop", e_wizard_dir_get());
   files = ecore_file_ls(buf);
   if (!files) return 0;
   EINA_LIST_FREE(files, file)
     {
        snprintf(buf, sizeof(buf), "%s/desktop/%s",
                 e_wizard_dir_get(), file);
        snprintf(buf2, sizeof(buf2), "%s/%s/%s",
                 e_user_homedir_get(), _("Desktop"), file);
        ecore_file_cp(buf, buf2);
        free(file);
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

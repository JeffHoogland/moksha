/* Ibar setup */
#include "e_wizard.h"

static void _write_bodhi_desktops(char *usr);

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops, Eina_Bool *need_xdg_icons __UNUSED__)
{
   *need_xdg_desktops = EINA_TRUE;
   return 1;
}

static void 
_write_bodhi_desktops(char *usr) 
{
   FILE *f;
   char buf[PATH_MAX];

   e_user_dir_concat_static(buf, "applications/bar/default");
   ecore_file_mkpath(buf);
   e_user_dir_concat_static(buf, "applications/bar/default/.order");
   f = fopen(buf, "w");
   if (f) 
     {
        fprintf(f, "midori.desktop\n");
        fprintf(f, "bodhi-appcenter.desktop\n");
        fprintf(f, "pcmanfm.desktop\n");
        if (!strcmp(usr, "bodhi"))
           fprintf(f, "ubiquity.desktop\n");
        fclose(f);
     }
}

/*
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}
*/
EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   struct passwd *pw;
   char *usr;

   pw = getpwuid(getuid());
   usr = pw->pw_name;
   if (usr) 
     {
        //if (!strcmp(usr, "bodhi"))
          _write_bodhi_desktops(usr);
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

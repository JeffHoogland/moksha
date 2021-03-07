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
_write_bodhi_update(void) 
{
   FILE *f;
   char buf[PATH_MAX];

   e_user_dir_concat_static(buf, "applications/startup");
   ecore_file_mkpath(buf);
   e_user_dir_concat_static(buf, "applications/startup/startupcommands");
   f = fopen(buf, "w");
   if (f) 
     {  fprintf(f, "/usr/bin/mintupdate-launcher\n");
        fclose(f);
     }
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
     {  if (ecore_file_exists("/usr/share/applications/firefox.desktop"))
           fprintf(f, "firefox.desktop\n");
        else if (ecore_file_exists("/usr/share/applications/chromium-browser.desktop"))
           fprintf(f, "chromium-browser.desktop\n");
        else
           fprintf(f, "org.gnome.Epiphany.desktop\n");
        fprintf(f, "terminology.desktop\n");
        if (ecore_file_exists("/usr/share/applications/Thunar.desktop"))
            // Ubuntu 18.04
            fprintf(f, "Thunar.desktop\n");
        else
            if (ecore_file_exists("/usr/share/applications/thunar.desktop"))
                // Ubuntu 20.04
                fprintf(f, "thunar.desktop\n");
            else
                fprintf(f, "pcmanfm.desktop\n");
        /*if (!strcmp(usr, "bodhi"))
           fprintf(f, "ubiquity.desktop\n");*/
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

   if (ecore_file_exists("/usr/bin/mintupdate-launcher"))
      _write_bodhi_update(); 
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

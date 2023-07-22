/* Delete previous copy of config profile and save new one */
#include "e_wizard.h"

void 
append_files(char *src, char *dest)
{
  FILE *fp1, *fp2;
  char buf[100];
  // opening files
  fp1 = fopen(src, "r");
  fp2 = fopen(dest, "a+");

  // If file is not found then return.
  if (!fp1 && !fp2) {
    fprintf(stderr, "Moksha: wizard page 200 failure\n");
    return;
  }
  
  while (!feof(fp1)) {
    if (fgets(buf, sizeof(buf), fp1)) 
      fprintf(fp2, "%s", buf);

  }
  if (fp1) fclose(fp1);
  if (fp2) fclose(fp2);
  
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
   #ifdef HAVE_ELEMENTARY
       e_util_set_bodhi_elm_config();
   #endif
   // save the config now everyone has modified it
   e_config_save();
   char shfile[PATH_MAX];
   e_user_dir_concat_static(shfile, "applications/startup/startupcommands");

   if (ecore_file_exists("/usr/share/bodhi/distro/startupcommands"))
     append_files("/usr/share/bodhi/distro/startupcommands", shfile);
   else
     if (ecore_file_exists("/usr/bin/elaptopcheck")){
       FILE *f;
       f = fopen(shfile, "w");
       fputs("elaptopcheck", f);
       if (f) fclose(f);
     }
   // disable restart env so we actually start a whole new session properly
   e_util_env_set("E_RESTART", NULL);
   // disable shfile execution when e_wizard finishes
   //e_util_env_set("E_WIZARD", "1");
   // restart e
   e_sys_action_do(E_SYS_RESTART, NULL);

   return 1;
}


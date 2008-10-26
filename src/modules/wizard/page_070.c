/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   char buf[PATH_MAX];
   const char *homedir;
   
   if ((e_config_profile_get()) && (strlen(e_config_profile_get()) > 0))
     {
	// delete profile
	homedir = e_user_homedir_get();
	snprintf(buf, sizeof(buf), "%s/.e/e/config/%s", homedir, e_config_profile_get());
	printf("del %s\n", buf);
	if (ecore_file_is_dir(buf)) ecore_file_recursive_rm(buf);
     }
   // load profile as e_config
   e_config_load();
   return 0; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   char buf[PATH_MAX];
   const char *homedir;
   
   // setup ~/Desktop and ~/.e/e/fileman/favorites and 
   // ~/.e/e/applications/bar/default, maybe ~/.e/e/applications/startup/.order

   homedir = e_user_homedir_get();
   
   // setup default .desktop files
   snprintf(buf, sizeof(buf), "%s/applications", efreet_data_home_get());
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf),
	    "gzip -d -c < %s/data/other/desktop_files.tar.gz | "
	    "(cd %s/applications/ ; tar -xkf -)",
	    e_prefix_data_get(), efreet_data_home_get());
   system(buf);
   // setup ibar
   snprintf(buf, sizeof(buf),
	    "gzip -d -c < %s/data/other/desktop_order.tar.gz | "
	    "(cd %s/.e/e/ ; tar -xkf -)",
	    e_prefix_data_get(), homedir);
   system(buf);

   // setup fileman favorites
   snprintf(buf, sizeof(buf),
	    "gzip -d -c < %s/data/other/efm_favorites.tar.gz | "
	    "(cd %s/.e/e/ ; tar -xkf -)",
	    e_prefix_data_get(), homedir);
   system(buf);
   // ~/Desktop
   snprintf(buf, sizeof(buf), "%s/Desktop", homedir);
   ecore_file_mkpath(buf);
   // FIXME: ln -s the .desktop files in favorites	       
   snprintf(buf, sizeof(buf), "%s/Desktop/home.desktop", homedir);
   ecore_file_symlink("../.e/e/fileman/favorites/home.desktop", buf);
   snprintf(buf, sizeof(buf), "%s/Desktop/root.desktop", homedir);
   ecore_file_symlink("../.e/e/fileman/favorites/root.desktop", buf);
   snprintf(buf, sizeof(buf), "%s/Desktop/tmp.desktop", homedir);
   ecore_file_symlink("../.e/e/fileman/favorites/tmp.desktop", buf);
   
   // save the config now everyone has modified it
   e_config_save();
   // restart e
   e_sys_action_do(E_SYS_RESTART, NULL);
   return 1;
}

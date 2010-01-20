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

   if ((e_config_profile_get()) && (strlen(e_config_profile_get()) > 0))
     {
	if (e_user_dir_snprintf(buf, sizeof(buf), "config/%s", e_config_profile_get()) >= sizeof(buf))
	  return 0;
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
   // setup ~/Desktop and ~/.e/e/fileman/favorites and 
   // ~/.e/e/applications/bar/default, maybe ~/.e/e/applications/startup/.order

   // FIXME: should become a wizard page on its own
   // setup fileman favorites
   snprintf(buf, sizeof(buf),
	    "gzip -d -c < %s/data/other/efm_favorites.tar.gz | "
	    "(cd %s/.e/e/ ; tar -xkf -)",
	    e_prefix_data_get(), e_user_homedir_get());
   system(buf);
   // FIXME: efm favorites linked to desktop should be an option in another
   // wizard page
   // ~/Desktop
   e_user_homedir_concat(buf, sizeof(buf), _("Desktop"));
   ecore_file_mkpath(buf);
   e_user_homedir_snprintf(buf, sizeof(buf), "%s/%s", _("Desktop"), "home.desktop");
   ecore_file_symlink("../.e/e/fileman/favorites/home.desktop", buf);
   e_user_homedir_snprintf(buf, sizeof(buf), "%s/%s", _("Desktop"), "root.desktop");
   ecore_file_symlink("../.e/e/fileman/favorites/root.desktop", buf);
   e_user_homedir_snprintf(buf, sizeof(buf), "%s/%s", _("Desktop"), "tmp.desktop");
   ecore_file_symlink("../.e/e/fileman/favorites/tmp.desktop", buf);
   
   // save the config now everyone has modified it
   e_config_save();
   // restart e
   e_sys_action_do(E_SYS_RESTART, NULL);
   return 1;
}

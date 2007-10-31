/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *conf_module = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/

/*
 * These are the currently planned wizard pages:
 * 
 * o == interactive
 * . == automatic (no gui)
 * 
 * --- THE LIST
 * .  look for system global profile - if it exists just copy it and exit
 *    wizard now.
 * .  find fonts like sans and other known fonts that have full intl. support
 *    and use them if found.
 * o  ask for language (default selection is current locale).
 * o  ask for font size to use.
 * o  xrender/engine speed test and detect and suggest best engine.
 * o  ask for one of N default config profiles to be set up. if profile is
 *    marked as "final" end wizard now.
 * o  find XDG app menus/repositories and list them let user choose which
 *    one(s) are to be used.
 * o  find other secondary menus - like ubuntu's settings menu and build
 *    more app menus for this.
 * .  look for battery, cpufreq and temperature support - if there, enable the
 *    appropriate modules.
 * o  ask what apps you want in ibar by default (or none - no ibar).
 * o  ask if user wants desktop icons or not (enable fwin module but seed it
 *    with default config).
 * o  ask if the user wants virtual desktops (if so have 4x1 and page module
 *    loaded).
 * o  ask about what kind of default key and mouse bindings a user wants
 *    (current e defaults, windows-style or mac-style?).
 * o  ask for what default wallpaper to use.
 * o  ask if the user wants gnome or kde support
 *    (for gnome run gnome-settings-daemon, unknown for kde).
 * 
 * --- THINGS TO ADD?
 * o  choose one of n available default themes (if we have any).
 * o  do you want a taskbar or not.

 */

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Wizard"
};


EAPI void *
e_modapi_init(E_Module *m)
{
   Ecore_List *files;
   char buf[PATH_MAX];
   
   conf_module = m;
   e_wizard_init();
   
   snprintf(buf, sizeof(buf), "%s/%s", e_module_dir_get(m), MODULE_ARCH);
   files = ecore_file_ls(buf);
   if (files)
     {
	char *file;
	
	ecore_list_first_goto(files);
	while ((file = ecore_list_current(files)))
	  {
	     if (!strncmp(file, "page_", 5))
	       {
		  void *handle;
		  
		  snprintf(buf, sizeof(buf), "%s/%s/%s",
			   e_module_dir_get(m), MODULE_ARCH, file);
		  handle = dlopen(buf, RTLD_NOW | RTLD_GLOBAL);
		  if (handle)
		    {
		       e_wizard_page_add(handle,
					 dlsym(handle, "wizard_page_init"),
					 dlsym(handle, "wizard_page_shutdown"),
					 dlsym(handle, "wizard_page_show"),
					 dlsym(handle, "wizard_page_hide"),
					 dlsym(handle, "wizard_page_apply"));
		    }
	       }
	     ecore_list_next(files);
	  }
	ecore_list_destroy(files);
     }
   
   e_wizard_go();
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_wizard_shutdown();
   conf_module = NULL;
// FIXME: wrong place   
//   e_module_disable(m); /* disable - on restart this won't be loaded now */
//   e_sys_action_do(E_SYS_RESTART, NULL); /* restart e - cleanly try settings */
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

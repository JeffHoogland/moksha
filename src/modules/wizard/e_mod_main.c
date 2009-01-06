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
 * . == automatic (no gui - none implemented currently)
 * 
 * * = done
 * - = code here, but disabled in build
 * 
 * --- THE LIST
 * o *ask for language (default selection is current locale).
 * o *ask for initial profile
 * o *find XDG app menus/repositories and list them let user choose which
 *    one(s) are to be used.
 * o -ask for ibar initial app set
 * o -ask if user wants desktop icons or not (enable fwin module but seed it
 *    with default config icons on desktop and favorites).
 * o -ask click to focus or sloppy
 * . *take some of current config (language, fileman, profile) and load
 *    load profile, apply language to it and save, restart e.
 * 
 * why are some disabled? profiels take care of this and do a better job
 * at collecting all the things together. for example illume makes no sense
 * with pointer focus and ibar icons/desktop makes no sense.
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


static int _cb_sort_files(char *f1, char *f2)
{
   return strcmp(f1, f2);
}

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
	ecore_list_sort(files, ECORE_COMPARE_CB(_cb_sort_files), ECORE_SORT_MIN);
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
                  else
                    printf("%s\n", dlerror());
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

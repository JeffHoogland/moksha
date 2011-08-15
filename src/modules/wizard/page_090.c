/* Setup if we need battery? */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__
# include <sys/ioctl.h>
# include <sys/sysctl.h>
# ifdef __i386__
#  include <machine/apm_bios.h>
# endif
#endif

static char *
read_file(const char *file)
{
   FILE *f = fopen(file, "r");
   size_t len;
   char buf[4096], *p;
   if (!f) return NULL;
   len = fread(buf, 1, sizeof(buf) - 1, f);
   if (len == 0)
     {
        fclose(f);
        return NULL;
     }
   buf[len] = 0;
   for (p = buf; *p; p++)
     {
        if (p[0] == '\n') p[0] = 0;
     }
   fclose(f);
   return strdup(buf);
}

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
   int hav_bat = 0;
   Eina_List *dir;
   char buf[PATH_MAX], *file, *dname, *str;

   dname = "/sys/class/power_supply";
   dir = ecore_file_ls(dname);
   if (dir)
     {
        EINA_LIST_FREE(dir, file)
          {
             snprintf(buf, sizeof(buf), "%s/%s/type", dname, file);
             str = read_file(buf);
             if (str)
               {
                  if (!strcasecmp(str, "Battery")) hav_bat = 1;
                  free(str);
               }
          }
     }
   dname = "/proc/acpi/battery/";
   dir = ecore_file_ls(dname);
   if (dir)
     {
        EINA_LIST_FREE(dir, file)
          {
             snprintf(buf, sizeof(buf), "%s/%s/state", dname, file);
             str = read_file(buf);
             if (str)
               {
                  hav_bat = 1;
                  free(str);
               }
          }
     }
#ifdef __FreeBSD__
   do {
      int mib_state[4];
      int state = 0;
      size_t len;
      
      /* Read some information on first run. */
      len = 4;
      sysctlnametomib("hw.acpi.battery.state", mib_state, &len);
      len = sizeof(state);
      if (sysctl(mib_state, 4, &state, &len, NULL, 0) != -1)
         hav_bat = 1;
   } while (0);
#endif
   if (!hav_bat)
     {
        E_Config_Module *em;
        Eina_List *l;
        
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em->name) continue;
             if (!strcmp(em->name, "battery"))
               {
                  e_config->modules = eina_list_remove_list
                     (e_config->modules, l);
                  if (em->name) eina_stringshare_del(em->name);
                  free(em);
                  break;
               }
          }
        e_config_save_queue();
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

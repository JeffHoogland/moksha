/* Ibar setup */
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
   FILE *f, *fin;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/def-ibar.txt", e_wizard_dir_get());
   fin = fopen(buf, "r");
   if (!fin) return 0;
   e_user_dir_concat_static(buf, "applications/bar/default");
   ecore_file_mkpath(buf);
   e_user_dir_concat_static(buf, "applications/bar/default/.order");
   f = fopen(buf, "w");
   if (f)
     {
        while (fgets(buf, sizeof(buf), fin))
          {
             Efreet_Desktop *desk;
             char name[PATH_MAX], buf2[PATH_MAX], *p;
             int n;
             
             if (buf[0] == '#') continue;
             p = buf;
             while (isspace(*p)) p++;
             for (;;)
               {
                  n = sscanf(p, "%s", name);
                  if (n != 1) break;
                  p += strlen(name);
                  while (isspace(*p)) p++;
                  snprintf(buf2, sizeof(buf2), "%s.desktop", name);
                  desk = efreet_util_desktop_file_id_find(buf2);
                  if (desk)
                    {
                       fprintf(f, "%s\n", buf2);
                       efreet_desktop_free(desk);
                       break;
                    }
               }
          }
        fclose(f);
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

/* Extra desktop files setup */
#include "e.h"
#include "e_mod_main.h"

static Ecore_Timer *_next_timer = NULL;

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__, Eina_Bool *need_xdg_desktops, Eina_Bool *need_xdg_icons __UNUSED__)
{
   *need_xdg_desktops = EINA_TRUE;
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   if (_next_timer) ecore_timer_del(_next_timer);
   _next_timer = NULL;
   return 1;
}

static Eina_Bool
_next_page(void *data __UNUSED__)
{
   _next_timer = NULL;
   e_wizard_button_next_enable_set(1);
   e_wizard_next();
   return ECORE_CALLBACK_CANCEL;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   Eina_List *extra_desks, *desks;
   Efreet_Desktop *desk, *extra_desk;
   char buf[PATH_MAX], *file;
   int found, copies = 0;

   e_wizard_title_set(_("Adding missing App files"));
   e_wizard_button_next_enable_set(0);
   e_wizard_page_show(NULL);
   
   snprintf(buf, sizeof(buf), "%s/extra_desktops", e_wizard_dir_get());
   extra_desks = ecore_file_ls(buf);

   /* advance in 1 sec */
   _next_timer = ecore_timer_add(1.0, _next_page, NULL);
   
   EINA_LIST_FREE(extra_desks, file)
     {
        snprintf(buf, sizeof(buf), "%s/extra_desktops/%s",
                 e_wizard_dir_get(), file);
        extra_desk = efreet_desktop_uncached_new(buf);
        if (extra_desk)
          {
             if (extra_desk->exec)
               {
                  char abuf[4096], dbuf[4096];

                  found = 0;
                  if (sscanf(extra_desk->exec, "%4000s", abuf) == 1)
                    {
                       if (ecore_file_app_installed(abuf))
                         {
                            desks = efreet_util_desktop_name_glob_list("*");
                            EINA_LIST_FREE(desks, desk)
                              {
                                 if ((!found) && (desk->exec))
                                   {
                                      if (sscanf(desk->exec, "%4000s", dbuf) == 1)
                                        {
                                           char *p1, *p2;

                                           p1 = strrchr(dbuf, '/');
                                           if (p1) p1++;
                                           else p1 = dbuf;
                                           p2 = strrchr(abuf, '/');
                                           if (p2) p2++;
                                           else p2 = abuf;
                                           if (!strcmp(p1, p2)) found = 1;
                                        }
                                   }
                                 efreet_desktop_free(desk);
                              }
                         }
                    }
                  if (!found)
                    {
                       // copy file
                       snprintf(abuf, sizeof(abuf),
                                "%s/applications",
                                efreet_data_home_get());
                       ecore_file_mkpath(abuf);
                       snprintf(abuf, sizeof(abuf),
                                "%s/applications/%s",
                                efreet_data_home_get(), file);
                       ecore_file_cp(buf, abuf);
                       // trigger cache rebuild
                       efreet_desktop_free(efreet_desktop_get(abuf));
                       copies++;
                    }
               }
             efreet_desktop_free(extra_desk);
          }
        free(file);
     }
   if (copies == 0)
     {
        if (_next_timer) ecore_timer_del(_next_timer);
        _next_timer = NULL;
        return 0; /* we didnt copy anything so advance anyway */
     }
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
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


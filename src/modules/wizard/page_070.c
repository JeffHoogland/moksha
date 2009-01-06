/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

typedef struct _App App;
struct _App
{
   const char *file; // must be unique - should normally be app name
   const char *name, *generic, *comment; // fields we should provide
   const char *exec; // the exec line
   const char *icon; // icon file
   const char *extra; // extra fields (tranlations etc.)
   int found;
};

static App apps[] =
{
     { "firefox", "Firefox", "Web Browser", "Browse the Internet",
          "firefox %u", "web_browser.png",
          "StartupNotify=true\n"
          "StartupWMClass=Firefox-bin\n"
          "Categories=Application;Network;", 
          0 },
     { "xterm", "XTerm", "Terminal", "Run commands in a shell",
          "xterm", "xterm.png",
          "StartupWMClass=XTerm\n"
          "Categories=Utility;TerminalEmulator;", 
          0 },
     { "eterm", "ETerm", "Terminal", "Run commands in a shell",
          "Eterm", "xterm.png",
          "StartupWMClass=Eterm\n"
          "Categories=Utility;TerminalEmulator;", 
          0 },
     { "sylpheed", "Sylpheed", "E-Mail Client", "Read and write E-Mail",
          "sylpheed", "mail_client.png",
          "StartupNotify=true\n"
          "StartupWMClass=Sylpheed\n"
          "Categories=Application;Network;", 
          0 },
     { "xine", "Xine", "Movie Player", "Watch movies and videos",
          "xine %U", "video_player.png",
          "StartupWMClass=xine\n"
          "Categories=Application;AudioVideo;Player;",
          0 },
     { "mplayer", "MPlayer", "Movie Player", "Watch movies and videos",
          "mplayer %U", "video_player.png",
          "StartupWMClass=MPlayer\n"
          "Categories=Application;AudioVideo;Player;",
          0 },
     { "xmms", "XMMS", "Audio Player", "Listen to music",
          "xmms %U", "audio_player.png",
          "StartupWMClass=XMMS\n"
          "Categories=Application;AudioVideo;Player;",
          0 },
     { "beep-media-player", "BMP", "Audio Player", "Listen to music",
          "beep-media-player %U", "audio_player.png",
          "StartupWMClass=Beep-Media-Player\n"
          "Categories=Application;AudioVideo;Player;",
          0 },
     { "audacious", "Audacious", "Audio Player", "Listen to music",
          "audacious %U", "audio_player.png",
          "StartupWMClass=Audacious\n"
          "Categories=Application;AudioVideo;Player;",
          0 },
     { "gqview", "GQView", "Image Viewer", "Look at photos",
          "gqview %U", "image_viewer.png",
          "StartupWMClass=GQView\n"
          "Categories=Application;Graphics;Viewer;",
          0 },
     { "xjed", "X-Jed", "Text Editor", "Edit text files",
          "xjed %U", "text_editor.png",
          "StartupWMClass=XJed\n"
          "Categories=Application;Utility;TextEditor;\n"
          "MimeType=text/plain;",
          0 }
   // FIXME: add more apps to search for to add .desktops for OR add to ibar.
   // feel free to have a pretty big list here. only list from the above ones
   // that have a executable installed.
   // FIXME: make sure these .desktop handle MimteType lists like xjed
   // FIXME: this might be nice moved into a set of sample .desktops that get
   // loaded and parsed to make it easier to add more in some dir somewhere
};

static int
_cb_sort_desks(Efreet_Desktop *d1, Efreet_Desktop *d2)
{
   if (!d1->name) return 1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_app_write(App *a)
{
   FILE *f;
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/applications",
            efreet_data_home_get());
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/applications/%s.desktop",
            efreet_data_home_get(), a->file);
   f = fopen(buf, "w");
   if (!f) return;
   fprintf(f,
           "[Desktop Entry]\n"
           "Encoding=UTF-8\n"
           "Type=Application\n"
           "Name=%s\n"
           "GenericName=%s\n"
           "Comment=%s\n"
           "Exec=%s\n"
           "Icon=%s\n",
           a->name, a->generic, a->comment, a->exec, a->icon);
   if (a->extra) fprintf(f, "%s\n", a->extra);
   fclose(f);
   efreet_desktop_get(buf);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   Ecore_List *desks = NULL;
   int i;
   
   efreet_util_init();
                  
   desks = efreet_util_desktop_name_glob_list("*");
   if (desks)
     {
        Efreet_Desktop *desk;
        
        ecore_list_first_goto(desks);
        while ((desk = ecore_list_next(desks)))
          {
             char dbuf[4096];

             if (!desk->exec) continue;
             if (sscanf(desk->exec, "%4000s", dbuf) == 1)
               {
                  for (i = 0; i < (sizeof(apps) / sizeof(App)); i++)
                    {
                       if (apps[i].found == 0)
                         {
                            char abuf[4096];
                            
                            if (sscanf(apps[i].exec, "%4000s", abuf) == 1)
                              {
                                 char *p1, *p2;
                                 
                                 if (!ecore_file_app_installed(abuf))
                                   {
                                      /* can't find exe - mark as not available */
                                      apps[i].found = -1;
                                   }
                                 else
                                   {
                                      p1 = strrchr(dbuf, '/');
                                      if (p1) p1++;
                                      else p1 = dbuf;
                                      p2 = strrchr(abuf, '/');
                                      if (p2) p2++;
                                      else p2 = abuf;
                                      if (!strcmp(p1, p2))
                                        /* mark as found in .desktops */
                                        apps[i].found = 2;
                                   }
                              }
                         }
                    }
               }
          }
        ecore_list_destroy(desks);
     }
   efreet_util_shutdown();
   // FIXME: list all apps and of the apps either already installed, or to be
   // created, offer them to be added to ibar by default. (actually should be
   // page_080)
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
   Evas_Object *o, *of, *ob, *li, *ck;
   Evas_Coord mw, mh;
   int i;
   int appnum = 0;
   
   for (i = 0; i < (sizeof(apps) / sizeof(App)); i++)
     {
        if (apps[i].found == 0) appnum++;
     }
   if (appnum == 0) return 0;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Applications"));
   
   of = e_widget_framelist_add(pg->evas, _("Select Icons to Add"), 0);

   li = e_widget_list_add(pg->evas, 1, 0);
   ob = e_widget_scrollframe_simple_add(pg->evas, li);
   e_widget_min_size_set(ob, 140 * e_scale, 140 * e_scale);

   for (i = 0; i < (sizeof(apps) / sizeof(App)); i++)
     {
        if (apps[i].found == 0)
          {
             char *icon;

             apps[i].found = 1;
             icon = efreet_icon_path_find(e_config->icon_theme, 
                                          apps[i].icon, 48);
             ck = e_widget_check_icon_add(pg->evas, apps[i].name, 
                                          icon, 32 * e_scale, 32 * e_scale,
                                          &(apps[i].found));
             if (icon) free(icon);
             e_widget_list_object_append(li, ck, 1, 1, 0.0);
             evas_object_show(ck);
          }
     }

   e_widget_min_size_get(li, &mw, &mh);
   evas_object_resize(li, mw, mh);
   
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   evas_object_show(li);
   
   e_wizard_page_show(o);

   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   int i;
   
   /* .desktops are actually written here. this is because page_080 needs them
    * to exist in the efreet desktops list to select what is to go in ibar */
   for (i = 0; i < (sizeof(apps) / sizeof(App)); i++)
     {
        printf("%s %i\n", apps[i].name, apps[i].found);
        if (apps[i].found == 1)
          {
             _app_write(&(apps[i]));
          }
     }
   evas_object_del(pg->data);
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   // FIXME: write ~/.e/e/applications/bar/default/.order
   // which should contain whatever apps the user wants in their bar by
   // default (should be page_080)
   return 1;
}

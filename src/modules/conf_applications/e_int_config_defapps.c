#include "e.h"

typedef struct _Config_Glob Config_Glob;
typedef struct _Config_Mime Config_Mime;

struct _Config_Glob
{
   const char *name;
};

struct _Config_Mime
{
   const char *mime;
   Eina_List *globs;
};

struct _E_Config_Dialog_Data
{
   struct {
      Evas_Object *deflist, *mimelist, *entry;
   } obj;
   Efreet_Ini *ini;
   Eina_List *mimes;
   Eina_List *desks;

   const char *selmime;
   const char *selapp;
   
   const char **seldest;
   
   char *browser_custom;
   
   const char *browser_desktop;
   const char *mailto_desktop;
   const char *file_desktop;
   const char *trash_desktop;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);

static void _load_mimes(E_Config_Dialog_Data *cfdata, char *file);
static void _load_globs(E_Config_Dialog_Data *cfdata, char *file);
static int _sort_mimes(const void *data1, const void *data2);
static Config_Mime *_find_mime(E_Config_Dialog_Data *cfdata, char *mime);
static Config_Glob *_find_glob(Config_Mime *mime, char *glob);
static int _cb_desks_sort(const void *data1, const void *data2);
static void _fill_apps_list(E_Config_Dialog_Data *cfdata, Evas_Object *il, const char **desktop, int general);

E_Config_Dialog *
e_int_config_defapps(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "applications/default_applications"))
      return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(con, _("Default Applications"),
                             "E", "applications/default_applications",
			     "preferences-applications-default", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Efreet_Ini *ini, *myini;
   Eina_Iterator *it;
   char buf[PATH_MAX];
   const char *key, *s, *homedir;
   Eina_List *l;
   E_Config_Env_Var *evr;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   
   snprintf(buf, sizeof(buf), "%s/applications/defaults.list",
            efreet_data_home_get());
   myini = efreet_ini_new(buf);
   if (myini)
     {
        cfdata->ini = myini;
        if (!efreet_ini_section_set(myini, "Default Applications"))
          {
             efreet_ini_section_add(myini, "Default Applications");
             efreet_ini_section_set(myini, "Default Applications");
          }
        EINA_LIST_FOREACH(efreet_data_dirs_get(), l, s)
          {
             snprintf(buf, sizeof(buf), "%s/applications/defaults.list", s);
             ini = efreet_ini_new(buf);
             if ((ini) &&
                 (efreet_ini_section_set(ini, "Default Applications")) &&
                 (ini->section) &&
                 (it = eina_hash_iterator_key_new(ini->section)))
               {
                  EINA_ITERATOR_FOREACH(it, key)
                    {
                       if (!efreet_ini_string_get(myini, key))
                         {
                            s = efreet_ini_string_get(ini, key);
                            if (s) efreet_ini_string_set(myini, key, s);
                         }
                    }
                  eina_iterator_free(it);
               }
             if (ini) efreet_ini_free(ini);
          }
        s = efreet_ini_string_get(myini, "x-scheme-handler/http");
        if (!s) s = efreet_ini_string_get(myini, "x-scheme-handler/https");
        if (s) cfdata->browser_desktop = eina_stringshare_add(s);
        s = efreet_ini_string_get(myini, "x-scheme-handler/mailto");
        if (s) cfdata->mailto_desktop = eina_stringshare_add(s);
        s = efreet_ini_string_get(myini, "x-scheme-handler/file");
        if (s) cfdata->file_desktop = eina_stringshare_add(s);
        s = efreet_ini_string_get(myini, "x-scheme-handler/trash");
        if (s) cfdata->trash_desktop = eina_stringshare_add(s);
     }

   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        if (!strcmp(evr->var, "BROWSER"))
          {
             if ((evr->val) && (!evr->unset))
                cfdata->browser_custom = strdup(evr->val);
             break;
          }
     }
   
   homedir = e_user_homedir_get();
   
   snprintf(buf, sizeof(buf), "/usr/local/etc/mime.types");
   if (ecore_file_exists(buf)) _load_mimes(cfdata, buf);
   snprintf(buf, sizeof(buf), "/etc/mime.types");
   if (ecore_file_exists(buf)) _load_mimes(cfdata, buf);
   
   EINA_LIST_FOREACH(efreet_config_dirs_get(), l, s)
     {
        snprintf(buf, sizeof(buf), "%s/mime/globs", s);
        if (ecore_file_exists(buf)) _load_globs(cfdata, buf);
     }
   
   snprintf(buf, sizeof(buf), "%s/.mime.types", homedir);
   if (ecore_file_exists(buf)) _load_mimes(cfdata, buf);
   
   snprintf(buf, sizeof(buf), "%s/mime/globs", efreet_data_home_get());
   if (ecore_file_exists(buf)) _load_globs(cfdata, buf);
   
   cfdata->mimes = eina_list_sort(cfdata->mimes, 0, _sort_mimes);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Config_Mime *m;
   Efreet_Desktop *desk;

   if (cfdata->selmime) eina_stringshare_del(cfdata->selmime);
   if (cfdata->selapp) eina_stringshare_del(cfdata->selapp);
   free(cfdata->browser_custom);
   if (cfdata->browser_desktop) eina_stringshare_del(cfdata->browser_desktop);
   if (cfdata->mailto_desktop) eina_stringshare_del(cfdata->mailto_desktop);
   if (cfdata->file_desktop) eina_stringshare_del(cfdata->file_desktop);
   if (cfdata->trash_desktop) eina_stringshare_del(cfdata->trash_desktop);
   EINA_LIST_FREE(cfdata->mimes, m)
     {
        Config_Glob *g;
        
        if (!m) continue;
        EINA_LIST_FREE(m->globs, g)
          {
             if (!g) continue;
             eina_stringshare_del(g->name);
             E_FREE(g);
          }
        eina_stringshare_del(m->mime);
        E_FREE(m);
     }
   if (cfdata->ini) efreet_ini_free(cfdata->ini);
   EINA_LIST_FREE(cfdata->desks, desk)
      efreet_desktop_free(desk);
   E_FREE(cfdata);
}

static void
_def_browser_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->seldest = &(cfdata->browser_desktop);
   _fill_apps_list(cfdata, cfdata->obj.deflist, cfdata->seldest, 0);
}

static void
_def_mailto_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->seldest = &(cfdata->mailto_desktop);
   _fill_apps_list(cfdata, cfdata->obj.deflist, cfdata->seldest, 0);
}

static void
_def_file_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->seldest = &(cfdata->file_desktop);
   _fill_apps_list(cfdata, cfdata->obj.deflist, cfdata->seldest, 0);
}

static void
_def_trash_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   cfdata->seldest = &(cfdata->trash_desktop);
   _fill_apps_list(cfdata, cfdata->obj.deflist, cfdata->seldest, 0);
}

static void
_sel_mime_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   if (cfdata->selapp) eina_stringshare_del(cfdata->selapp);
   cfdata->selapp = NULL;
   if (cfdata->selmime)
     {
        const char *s = efreet_ini_string_get(cfdata->ini, cfdata->selmime);
        if (s) cfdata->selapp = eina_stringshare_add(s);
     }
   _fill_apps_list(cfdata, cfdata->obj.mimelist, &(cfdata->selapp), 1);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ot, *ob, *of, *il;
   Eina_List *l;
   Config_Mime *m;
   
   otb = e_widget_toolbook_add(evas, 24, 24);
   
   ot = e_widget_table_add(evas, EINA_FALSE);
   
   ob = e_widget_label_add(evas, _("Custom Browser Command"));
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 0, 0);
   
   ob = e_widget_entry_add(evas, &(cfdata->browser_custom), NULL, NULL, NULL);
   cfdata->obj.entry = ob;
   e_widget_table_object_append(ot, ob, 1, 0, 1, 1, 1, 1, 1, 0);
   
   of = e_widget_framelist_add(evas, _("Default Applications"), 0);
   il = e_widget_ilist_add(evas, 24, 24, NULL);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(il);
   e_widget_ilist_selector_set(il, 1);
   e_widget_ilist_append(il, NULL, _("Browser"), _def_browser_cb, cfdata, NULL);
   e_widget_ilist_append(il, NULL, _("E-Mail"), _def_mailto_cb, cfdata, NULL);
   e_widget_ilist_append(il, NULL, _("File"), _def_file_cb, cfdata, NULL);
   e_widget_ilist_append(il, NULL, _("Trash"), _def_trash_cb, cfdata, NULL);
   e_widget_ilist_go(il);
   e_widget_ilist_thaw(il);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_framelist_object_append_full(of, il, 1, 1, 1, 1, 0.5, 0.5, 120, 200, 9999, 9999);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 0, 1);
   
   of = e_widget_framelist_add(evas, _("Selected Application"), 0);
   il = e_widget_ilist_add(evas, 24, 24, &(cfdata->selapp));
   cfdata->obj.deflist = il;
   e_widget_ilist_selector_set(il, 1);
   e_widget_ilist_go(il);
   e_widget_framelist_object_append_full(of, il, 1, 1, 1, 1, 0.5, 0.5, 120, 200, 9999, 9999);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);

   e_widget_toolbook_page_append(otb, NULL, _("Core"), ot,
                                 1, 1, 1, 1, 0.5, 0.0);
   
   ot = e_widget_table_add(evas, EINA_FALSE);
   
   of = e_widget_framelist_add(evas, _("Types"), 0);
   il = e_widget_ilist_add(evas, 24, 24, &(cfdata->selmime));
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(il);
   e_widget_ilist_selector_set(il, 1);
   EINA_LIST_FOREACH(cfdata->mimes, l, m)
      e_widget_ilist_append(il, NULL, m->mime, _sel_mime_cb, cfdata, m->mime);
   e_widget_ilist_go(il);
   e_widget_ilist_thaw(il);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_framelist_object_append_full(of, il, 1, 1, 1, 1, 0.5, 0.5, 120, 200, 9999, 9999);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Selected Application"), 0);
   il = e_widget_ilist_add(evas, 24, 24, &(cfdata->selapp));
   cfdata->obj.mimelist = il;
   e_widget_ilist_selector_set(il, 1);
   e_widget_ilist_go(il);
   e_widget_framelist_object_append_full(of, il, 1, 1, 1, 1, 0.5, 0.5, 120, 200, 9999, 9999);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);

   e_widget_toolbook_page_append(otb, NULL, _("General"), ot,
                                 1, 1, 1, 1, 0.5, 0.0);
   
   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);
   e_win_centered_set(cfd->dia->win, 1);
   return otb;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   E_Config_Env_Var *evr = NULL;

   if (cfdata->ini)
     {
        char buf[PATH_MAX];
        
        if ((cfdata->browser_desktop) && (cfdata->browser_desktop[0]))
          {
             efreet_ini_string_set(cfdata->ini, "x-scheme-handler/http", 
                                   cfdata->browser_desktop);
             efreet_ini_string_set(cfdata->ini, "x-scheme-handler/https", 
                                   cfdata->browser_desktop);
          }
        if ((cfdata->mailto_desktop) && (cfdata->mailto_desktop[0]))
           efreet_ini_string_set(cfdata->ini, "x-scheme-handler/mailto", 
                                 cfdata->mailto_desktop);
        if ((cfdata->file_desktop) && (cfdata->file_desktop[0]))
           efreet_ini_string_set(cfdata->ini, "x-scheme-handler/file",
                                 cfdata->file_desktop);
        if ((cfdata->trash_desktop) && (cfdata->trash_desktop[0]))
           efreet_ini_string_set(cfdata->ini, "x-scheme-handler/trash",
                                 cfdata->trash_desktop);
        snprintf(buf, sizeof(buf), "%s/applications/defaults.list",
                 efreet_data_home_get());
        efreet_ini_save(cfdata->ini, buf);
     }
   if ((cfdata->browser_custom) && (cfdata->browser_custom[0]))
     {
        EINA_LIST_FOREACH(e_config->env_vars, l, evr)
          {
             if (!strcmp(evr->var, "BROWSER")) break;
             evr = NULL;
          }
        if (evr)
          {
             evr->unset =0;
             if (evr->val) eina_stringshare_del(evr->val);
          }
        else
          {
             evr = E_NEW(E_Config_Env_Var, 1);
             if (evr)
               {
                  evr->var = eina_stringshare_add("BROWSER");
                  evr->unset = 0;
                  e_config->env_vars = eina_list_append(e_config->env_vars, evr);
               }
          }
        if (evr)
          {
             evr->val = eina_stringshare_add(cfdata->browser_custom);
             e_env_set(evr->var, evr->val);
          }
     }
   else
     {
        EINA_LIST_FOREACH(e_config->env_vars, l, evr)
          {
             if (!strcmp(evr->var, "BROWSER"))
               {
                  e_config->env_vars = eina_list_remove_list(e_config->env_vars, l);
                  if (evr->val) eina_stringshare_del(evr->val);
                  if (evr->var) eina_stringshare_del(evr->var);
                  free(evr);
                  break;
               }
          }
        e_env_unset("BROWSER");
     }
   e_config_save_queue();
   return 1;
}

static void
_load_mimes(E_Config_Dialog_Data *cfdata, char *file)
{
   FILE *f;
   char buf[4096], mimetype[4096], ext[4096];
   char *p, *pp;
   Config_Mime *mime;
   Config_Glob *glob;
   
   if (!cfdata) return;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        p = buf;
        while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
        if (*p == '#') continue;
        if ((*p == '\n') || (*p == 0)) continue;
        pp = p;
        while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
        if ((*p == '\n') || (*p == 0)) continue;
        strncpy(mimetype, pp, (p - pp));
        mimetype[p - pp] = 0;
        do
          {
             while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
             if ((*p == '\n') || (*p == 0)) continue;
             pp = p;
             while (!isblank(*p) && (*p != 0) && (*p != '\n')) p++;
             strncpy(ext, pp, (p - pp));
             ext[p - pp] = 0;
             mime = _find_mime(cfdata, mimetype);
             if (!mime)
               {
                  mime = E_NEW(Config_Mime, 1);
                  if (mime)
                    {
                       mime->mime = eina_stringshare_add(mimetype);
                       if (!mime->mime)
                          free(mime);
                       else
                         {
                            glob = E_NEW(Config_Glob, 1);
                            glob->name = eina_stringshare_add(ext);
                            mime->globs = eina_list_append(mime->globs, glob);
                            cfdata->mimes = eina_list_append(cfdata->mimes, mime);
                         }
                    }
               }
             else
               {
                  glob = _find_glob(mime, ext);
                  if (!glob)
                    {
                       glob = E_NEW(Config_Glob, 1);
                       glob->name = eina_stringshare_add(ext);
                       mime->globs = eina_list_append(mime->globs, glob);
                    }
               }
          }
        while ((*p != '\n') && (*p != 0));
     }
   fclose(f);
}

static void
_load_globs(E_Config_Dialog_Data *cfdata, char *file)
{
   FILE *f;
   char buf[4096], mimetype[4096], ext[4096];
   char *p, *pp;
   Config_Mime *mime;
   Config_Glob *glob;
   
   if (!cfdata) return;
   
   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        p = buf;
        while (isblank(*p) && (*p != 0) && (*p != '\n')) p++;
        if (*p == '#') continue;
        if ((*p == '\n') || (*p == 0)) continue;
        pp = p;
        while ((*p != ':') && (*p != 0) && (*p != '\n')) p++;
        if ((*p == '\n') || (*p == 0)) continue;
        strncpy(mimetype, pp, (p - pp));
        mimetype[p - pp] = 0;
        p++;
        pp = ext;
        while ((*p != 0) && (*p != '\n'))
          {
             *pp = *p;
             pp++;
             p++;
          }
        *pp = 0;
        mime = _find_mime(cfdata, mimetype);
        if (!mime)
          {
             mime = E_NEW(Config_Mime, 1);
             if (mime)
               {
                  mime->mime = eina_stringshare_add(mimetype);
                  if (!mime->mime)
                     free(mime);
                  else
                    {
                       glob = E_NEW(Config_Glob, 1);
                       glob->name = eina_stringshare_add(ext);
                       mime->globs = eina_list_append(mime->globs, glob);
                       cfdata->mimes = eina_list_append(cfdata->mimes, mime);
                    }
               }
          }
        else
          {
             glob = _find_glob(mime, ext);
             if (!glob)
               {
                  glob = E_NEW(Config_Glob, 1);
                  glob->name = eina_stringshare_add(ext);
                  mime->globs = eina_list_append(mime->globs, glob);
               }
          }
     }
   fclose(f);
}

static int
_sort_mimes(const void *data1, const void *data2)
{
   const Config_Mime *m1 = data1, *m2 = data2;
   if (!m1) return 1;
   if (!m2) return -1;
   return (strcmp(m1->mime, m2->mime));
}

static Config_Mime *
_find_mime(E_Config_Dialog_Data *cfdata, char *mime)
{
   Config_Mime *cm;
   Eina_List *l;
   
   if (!cfdata) return NULL;
   EINA_LIST_FOREACH(cfdata->mimes, l, cm)
     {
        if (!cm) continue;
        if (!strcmp(cm->mime, mime)) return cm;
     }
   return NULL;
}

static Config_Glob *
_find_glob(Config_Mime *mime, char *glob)
{
   Config_Glob *g;
   Eina_List *l;
   
   if (!mime) return NULL;
   EINA_LIST_FOREACH(mime->globs, l, g)
     {
        if (!g) continue;
        if (strcmp(g->name, glob)) continue;
        return g;
     }
   return NULL;
}

static int
_cb_desks_sort(const void *data1, const void *data2)
{
   const Efreet_Desktop *d1, *d2;
   
   if (!(d1 = data1)) return 1;
   if (!d1->name) return 1;
   if (!(d2 = data2)) return -1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_sel_desk_gen_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   const char *s = e_widget_ilist_selected_value_get(cfdata->obj.mimelist);
   if ((s) && (cfdata->selmime))
     {
        if (cfdata->ini)
           efreet_ini_string_set(cfdata->ini, cfdata->selmime, s);
     }
}

static void
_sel_desk_cb(void *data)
{
   E_Config_Dialog_Data *cfdata = data;
   if (cfdata->seldest)
     {
        const char *s = e_widget_ilist_selected_value_get(cfdata->obj.deflist);
        if (*(cfdata->seldest)) eina_stringshare_del(*(cfdata->seldest));
        *(cfdata->seldest) = NULL;
        if (s) *(cfdata->seldest) = eina_stringshare_add(s);
        if ((*(cfdata->seldest)) &&
            (cfdata->seldest == &(cfdata->browser_desktop)))
          {
             Eina_List *l;
             Efreet_Desktop *desk;
             
             EINA_LIST_FOREACH(cfdata->desks, l, desk)
               {
                  if ((!strcmp(desk->orig_path, *(cfdata->seldest))) ||
                      (!strcmp(ecore_file_file_get(desk->orig_path), *(cfdata->seldest))))
                    {
                       if (desk->exec)
                         {
                            char *p;
                            
                            free(cfdata->browser_custom);
                            cfdata->browser_custom = strdup(desk->exec);
                            for (p = cfdata->browser_custom; *p; p++)
                              {
                                 if (p > cfdata->browser_custom)
                                   {
                                      if ((isspace(*p)) &&
                                          (p[-1] != '\\'))
                                        {
                                           *p = 0;
                                           break;
                                        }
                                   }
                              }
                            p = strdup(cfdata->browser_custom);
                            if (p)
                              {
                                 e_widget_entry_text_set(cfdata->obj.entry, p);
                                 free(p);
                              }
                         }
                       break;
                    }
               }
          }
     }
}

static void
_fill_apps_list(E_Config_Dialog_Data *cfdata, Evas_Object *il, const char **desktop, int general)
{
   Eina_List *desks = NULL, *l;
   Efreet_Desktop *desk = NULL;
   Evas *evas;
   int sel, i;
   
   if (!cfdata->desks)
     {
        desks = efreet_util_desktop_name_glob_list("*");
        EINA_LIST_FREE(desks, desk)
          {
             Eina_List *ll;
             
             if (desk->no_display)
               {
                  efreet_desktop_free(desk);
                  continue;
               }
             ll = eina_list_search_unsorted_list(cfdata->desks, _cb_desks_sort, desk);
             if (ll)
               {
                  Efreet_Desktop *old;
                  
                  old = eina_list_data_get(ll);
                  /*
                   * This fixes when we have several .desktop with the same name,
                   * and the only difference is that some of them are for specific
                   * desktops.
                   */
                  if ((old->only_show_in) && (!desk->only_show_in))
                    {
                       efreet_desktop_free(old);
                       eina_list_data_set(ll, desk);
                    }
                  else
                     efreet_desktop_free(desk);
               }
             else
                cfdata->desks = eina_list_append(cfdata->desks, desk);
          }
        cfdata->desks = eina_list_sort(cfdata->desks, -1, _cb_desks_sort);
     }
   
   evas = evas_object_evas_get(il);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(il);
   e_widget_ilist_clear(il);

   sel = -1;
   i = 0;
   EINA_LIST_FOREACH(cfdata->desks, l, desk)
     {
        Evas_Object *icon = NULL;
        
        if ((desktop) && (*desktop))
          {
             if ((!strcmp(desk->orig_path, *desktop)) ||
                 (!strcmp(ecore_file_file_get(desk->orig_path), *desktop)))
                sel = i;
          }
        
        icon = e_util_desktop_icon_add(desk, 24, evas);
        if (general)
           e_widget_ilist_append(il, icon, desk->name,
                                 _sel_desk_gen_cb, cfdata,
                                 ecore_file_file_get(desk->orig_path));
        else
           e_widget_ilist_append(il, icon, desk->name,
                                 _sel_desk_cb, cfdata,
                                 ecore_file_file_get(desk->orig_path));
        i++;
     }
   
   e_widget_ilist_go(il);
   e_widget_ilist_thaw(il);
   edje_thaw();
   evas_event_thaw(evas);
   if (sel >= 0)
     {
        e_widget_ilist_selected_set(il, sel);
        e_widget_ilist_nth_show(il, sel, 0);
     }
}

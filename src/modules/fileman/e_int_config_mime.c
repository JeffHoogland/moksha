#include "e_mod_main.h"

typedef struct _Config_Glob Config_Glob;
typedef struct _Config_Mime Config_Mime;
typedef struct _Config_Type Config_Type;
struct _Config_Glob
{
   const char *name;
};
struct _Config_Mime
{
   const char *mime;
   Eina_List  *globs;
};
struct _Config_Type
{
   const char *name;
   const char *type;
};
struct _E_Config_Dialog_Data
{
   Eina_List       *mimes;
   const char      *cur_type;
   struct
   {
      Evas_Object *tlist, *list;
   } gui;
   E_Config_Dialog *cfd, *edit_dlg;
};

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void         _fill_list(E_Config_Dialog_Data *cfdata, const char *mtype);
static void         _fill_tlist(E_Config_Dialog_Data *cfdata);
static void         _load_mimes(E_Config_Dialog_Data *cfdata, char *file);
static void         _load_globs(E_Config_Dialog_Data *cfdata, char *file);
static void         _fill_types(E_Config_Dialog_Data *cfdata);
static void         _tlist_cb_change(void *data);
static int          _sort_mimes(const void *data1, const void *data2);
static Config_Mime *_find_mime(E_Config_Dialog_Data *cfdata, char *mime);
static Config_Glob *_find_glob(Config_Mime *mime, char *glob);
static void         _cb_config(void *data, void *data2);

Eina_List *types = NULL;

E_Config_Dialog *
e_int_config_mime(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "fileman/file_icons")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;

   cfd = e_config_dialog_new(con, _("File Icons"), "E", "fileman/file_icons",
                             "preferences-file-icons", 0, v, NULL);
   return cfd;
}

void
e_int_config_mime_edit_done(void *data)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata) return;
   if (cfdata->edit_dlg)
     cfdata->edit_dlg = NULL;
   _tlist_cb_change(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   const char *s, *homedir;
   Eina_List *l;
   char buf[4096];

   if (!cfdata) return;
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
   _fill_types(cfdata);
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Config_Type *t;
   Config_Mime *m;

   if (cfdata->edit_dlg)
     {
        e_object_del(E_OBJECT(cfdata->edit_dlg));
        cfdata->edit_dlg = NULL;
     }

   EINA_LIST_FREE(types, t)
     {
        if (!t) continue;

        eina_stringshare_del(t->name);
        eina_stringshare_del(t->type);
        E_FREE(t);
     }

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

   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ol;
   Evas_Object *ob;

   e_dialog_resizable_set(cfd->dia, 1);
   
   o = e_widget_list_add(evas, 1, 1);
   of = e_widget_framelist_add(evas, _("Categories"), 0);
   ol = e_widget_ilist_add(evas, 16, 16, &(cfdata->cur_type));
   cfdata->gui.tlist = ol;
   _fill_tlist(cfdata);
   e_widget_framelist_object_append(of, ol);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_frametable_add(evas, _("File Types"), 0);
   ol = e_widget_ilist_add(evas, 16, 16, NULL);
   cfdata->gui.list = ol;
   e_widget_ilist_go(ol);
   e_widget_size_min_set(cfdata->gui.list, 250, 200);
   e_widget_frametable_object_append(of, ol, 0, 0, 3, 1, 1, 1, 1, 1);

   ob = e_widget_button_add(evas, _("Set"), "configure", _cb_config, cfdata, NULL);
   e_widget_frametable_object_append(of, ob, 1, 1, 1, 1, 1, 1, 1, 0);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static void
_fill_list(E_Config_Dialog_Data *cfdata, const char *mtype)
{
   Config_Mime *m;
   Eina_List *l;
   Evas_Coord w, h;
   Evas *evas;

   evas = evas_object_evas_get(cfdata->gui.list);
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.list);

   e_widget_ilist_clear(cfdata->gui.list);
   EINA_LIST_FOREACH(cfdata->mimes, l, m)
     {
        Evas_Object *icon = NULL;
        const char *tmp;
        char buf[4096];
        int edj = 0, img = 0;

        if (!m) return;
        if (!strstr(m->mime, mtype)) continue;
        tmp = e_fm_mime_icon_get(m->mime);
        if (!tmp)
          snprintf(buf, sizeof(buf), "e/icons/fileman/file");
        else if (!strcmp(tmp, "THUMB"))
          snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", m->mime);
        else if (!strncmp(tmp, "e/icons/fileman/mime/", 21))
          snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", m->mime);
        else
          {
             char *p;

             p = strrchr(tmp, '.');
             if ((p) && (!strcmp(p, ".edj")))
               edj = 1;
             else if (p)
               img = 1;
          }
        if (edj)
          {
             icon = edje_object_add(evas);
             if (!e_theme_edje_object_set(icon, tmp, "icon"))
               e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
          }
        else if (img)
          icon = e_widget_image_add_from_file(evas, tmp, 16, 16);
        else
          {
             icon = edje_object_add(evas);
             if (!e_theme_edje_object_set(icon, "base/theme/fileman", buf))
               e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
          }
        e_widget_ilist_append(cfdata->gui.list, icon, m->mime, NULL, NULL, NULL);
     }
   e_widget_ilist_go(cfdata->gui.list);
   e_widget_size_min_get(cfdata->gui.list, &w, &h);
   e_widget_size_min_set(cfdata->gui.list, w, 200);
   e_widget_ilist_thaw(cfdata->gui.list);
   edje_thaw();
   evas_event_thaw(evas);
}

static void
_fill_tlist(E_Config_Dialog_Data *cfdata)
{
   Config_Type *tmp;
   Eina_List *l;
   Evas_Coord w, h;

   evas_event_freeze(evas_object_evas_get(cfdata->gui.tlist));
   edje_freeze();
   e_widget_ilist_freeze(cfdata->gui.tlist);
   e_widget_ilist_clear(cfdata->gui.tlist);
   EINA_LIST_FOREACH(types, l, tmp)
     {
        Evas_Object *icon;
        char buf[4096];
        char *t;

        if (!tmp) continue;
        t = strdup(tmp->name);
        t[0] = tolower(t[0]);
        icon = edje_object_add(evas_object_evas_get(cfdata->gui.tlist));
        snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", t);
        if (!e_theme_edje_object_set(icon, "base/theme/fileman", buf))
          e_theme_edje_object_set(icon, "base/theme/fileman", "e/icons/fileman/file");
        e_widget_ilist_append(cfdata->gui.tlist, icon, tmp->name, _tlist_cb_change, cfdata, tmp->type);
        free(t);
     }

   e_widget_ilist_go(cfdata->gui.tlist);
   e_widget_size_min_get(cfdata->gui.tlist, &w, &h);
   e_widget_size_min_set(cfdata->gui.tlist, w, 225);
   e_widget_ilist_thaw(cfdata->gui.tlist);
   edje_thaw();
   evas_event_thaw(evas_object_evas_get(cfdata->gui.tlist));
}

static void
_load_mimes(E_Config_Dialog_Data *cfdata, char *file)
{
   FILE *f;
   char buf[4096], mimetype[4096], ext[4096];
   char *p, *pp;
   Config_Mime *config_mime;
   Config_Glob *config_glob;

   if (!cfdata) return;

   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        p = buf;
        while (isblank(*p) && (*p != 0) && (*p != '\n'))
          p++;
        if (*p == '#') continue;
        if ((*p == '\n') || (*p == 0)) continue;
        pp = p;
        while (!isblank(*p) && (*p != 0) && (*p != '\n'))
          p++;
        if ((*p == '\n') || (*p == 0)) continue;
        strncpy(mimetype, pp, (p - pp));
        mimetype[p - pp] = 0;
        do
          {
             while (isblank(*p) && (*p != 0) && (*p != '\n'))
               p++;
             if ((*p == '\n') || (*p == 0)) continue;
             pp = p;
             while (!isblank(*p) && (*p != 0) && (*p != '\n'))
               p++;
             if (p > pp)
               {
                  strncpy(ext, pp, (p - pp));
                  ext[p - pp] = 0;
                  config_mime = _find_mime(cfdata, mimetype);
                  if (!config_mime)
                    {
                       config_mime = E_NEW(Config_Mime, 1);
                       if (config_mime)
                         {
                            config_mime->mime = eina_stringshare_add(mimetype);
                            if (!config_mime->mime)
                              free(config_mime);
                            else
                              {
                                 config_glob = E_NEW(Config_Glob, 1);
                                 config_glob->name = eina_stringshare_add(ext);
                                 config_mime->globs = eina_list_append(config_mime->globs, config_glob);
                                 cfdata->mimes = eina_list_append(cfdata->mimes, config_mime);
                              }
                         }
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
   Config_Mime *config_mime;
   Config_Glob *config_glob;

   if (!cfdata) return;

   f = fopen(file, "rb");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
        p = buf;
        while (isblank(*p) && (*p != 0) && (*p != '\n'))
          p++;
        if (*p == '#') continue;
        if ((*p == '\n') || (*p == 0)) continue;
        pp = p;
        while ((*p != ':') && (*p != 0) && (*p != '\n'))
          p++;
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
        config_mime = _find_mime(cfdata, mimetype);
        if (!config_mime)
          {
             config_mime = E_NEW(Config_Mime, 1);
             if (config_mime)
               {
                  config_mime->mime = eina_stringshare_add(mimetype);
                  if (!config_mime->mime)
                    free(config_mime);
                  else
                    {
                       config_glob = E_NEW(Config_Glob, 1);
                       config_glob->name = eina_stringshare_add(ext);
                       config_mime->globs = eina_list_append(config_mime->globs, config_glob);
                       cfdata->mimes = eina_list_append(cfdata->mimes, config_mime);
                    }
               }
          }
        else
          {
             config_glob = _find_glob(config_mime, ext);
             if (!config_glob)
               {
                  config_glob = E_NEW(Config_Glob, 1);
                  config_glob->name = eina_stringshare_add(ext);
                  config_mime->globs = eina_list_append(config_mime->globs, config_glob);
               }
          }
     }
   fclose(f);
}

static void
_fill_types(E_Config_Dialog_Data *cfdata)
{
   Config_Mime *m;
   Eina_List *l;

   EINA_LIST_FOREACH(cfdata->mimes, l, m)
     {
        Config_Type *tmp;
        Eina_List *ll;
        char *tok, *str;
        int found = 0;

        if (!m) continue;
        str = strdup(m->mime);
        if (!str) continue;
        tok = strtok(str, "/");
        if (!tok)
          {
             free(str);
             continue;
          }

        EINA_LIST_FOREACH(types, ll, tmp)
          {
             if (!tmp) continue;

             if (strcmp(tmp->type, tok) >= 0)
               {
                  found = 1;
                  break;
               }
          }

        if (!found)
          {
             tmp = E_NEW(Config_Type, 1);
             tmp->type = eina_stringshare_add(tok);
             tok[0] = toupper(tok[0]);
             tmp->name = eina_stringshare_add(tok);

             types = eina_list_append(types, tmp);
          }
        free(str);
     }
}

static void
_tlist_cb_change(void *data)
{
   E_Config_Dialog_Data *cfdata;
   Config_Type *t;
   Eina_List *l;

   cfdata = data;
   if (!cfdata) return;

   EINA_LIST_FOREACH(types, l, t)
     {
        if (!t) continue;
        if (t->name != cfdata->cur_type /* Both string are stringshare. */
            && strcasecmp(t->name, cfdata->cur_type)) continue;
        _fill_list(cfdata, t->type);
        break;
     }
}

static int
_sort_mimes(const void *data1, const void *data2)
{
   const Config_Mime *m1, *m2;

   if (!data1) return 1;
   if (!data2) return -1;

   m1 = data1;
   m2 = data2;

   return strcmp(m1->mime, m2->mime);
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
_find_glob(Config_Mime *mime, char *globing)
{
   Config_Glob *g;
   Eina_List *l;

   if (!mime) return NULL;
   EINA_LIST_FOREACH(mime->globs, l, g)
     {
        if (!g) continue;
        if (strcmp(g->name, globing)) continue;
        return g;
     }
   return NULL;
}

static void
_cb_config(void *data, void *data2 __UNUSED__)
{
   Eina_List *l;
   E_Config_Dialog_Data *cfdata;
   E_Config_Mime_Icon *mi = NULL;
   const char *m;
   int found = 0;

   cfdata = data;
   if (!cfdata) return;
   m = e_widget_ilist_selected_label_get(cfdata->gui.list);
   if (!m) return;

   EINA_LIST_FOREACH(e_config->mime_icons, l, mi)
     {
        if (!mi) continue;
        if (!mi->mime) continue;
        if (strcmp(mi->mime, m)) continue;
        found = 1;
        break;
     }
   if (!found)
     {
        mi = E_NEW(E_Config_Mime_Icon, 1);
        mi->mime = eina_stringshare_add(m);
     }

   cfdata->edit_dlg = e_int_config_mime_edit(mi, cfdata);
}


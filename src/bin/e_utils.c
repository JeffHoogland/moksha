#include "e.h"

EAPI E_Path *path_data = NULL;
EAPI E_Path *path_images = NULL;
EAPI E_Path *path_fonts = NULL;
EAPI E_Path *path_themes = NULL;
EAPI E_Path *path_icons = NULL;
EAPI E_Path *path_modules = NULL;
EAPI E_Path *path_backgrounds = NULL;
EAPI E_Path *path_messages = NULL;

/* local subsystem functions */
static Eina_Bool    _e_util_cb_delayed_del(void *data);
static Eina_Bool    _e_util_wakeup_cb(void *data);

static Evas_Object *_e_util_icon_add(const char *path, Evas *evas, int size);

static void         _e_util_cb_delayed_cancel(void *data, void *obj);

/* local subsystem globals */
static Ecore_Timer *_e_util_dummy_timer = NULL;

/* externally accessible functions */
EAPI void
e_util_wakeup(void)
{
   if (_e_util_dummy_timer) return;
   _e_util_dummy_timer = ecore_timer_add(0.0, _e_util_wakeup_cb, NULL);
}

EAPI void
e_util_env_set(const char *var, const char *val)
{
   if (val)
     {
#ifdef HAVE_SETENV
        setenv(var, val, 1);
#else
        char buf[8192];

        snprintf(buf, sizeof(buf), "%s=%s", var, val);
        if (getenv(var))
          putenv(buf);
        else
          putenv(strdup(buf));
#endif
     }
   else
     {
#ifdef HAVE_UNSETENV
        unsetenv(var);
#else
        if (getenv(var)) putenv(var);
#endif
     }
}

EAPI E_Zone *
e_util_zone_current_get(E_Manager *man)
{
   E_Container *con;

   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   con = e_container_current_get(man);
   if (con)
     {
        E_Zone *zone;

        zone = e_zone_current_get(con);
        return zone;
     }
   return NULL;
}

EAPI int
e_util_glob_match(const char *str, const char *pattern)
{
   if ((!str) || (!pattern)) return 0;
   if (pattern[0] == 0)
     {
        if (str[0] == 0) return 1;
        return 0;
     }
   if (!strcmp(pattern, "*")) return 1;
   if (!fnmatch(pattern, str, 0)) return 1;
   return 0;
}

EAPI int
e_util_glob_case_match(const char *str, const char *pattern)
{
   const char *p;
   char *tstr, *tglob, *tp;

   if (pattern[0] == 0)
     {
        if (str[0] == 0) return 1;
        return 0;
     }
   if (!strcmp(pattern, "*")) return 1;
   tstr = alloca(strlen(str) + 1);
   for (tp = tstr, p = str; *p != 0; p++, tp++)
     *tp = tolower(*p);
   *tp = 0;
   tglob = alloca(strlen(pattern) + 1);
   for (tp = tglob, p = pattern; *p != 0; p++, tp++)
     *tp = tolower(*p);
   *tp = 0;
   if (!fnmatch(tglob, tstr, 0)) return 1;
   return 0;
}

EAPI E_Container *
e_util_container_number_get(int num)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Container *con;

        con = e_container_number_get(man, num);
        if (con) return con;
     }
   return NULL;
}

EAPI E_Zone *
e_util_container_zone_number_get(int con_num, int zone_num)
{
   E_Container *con;

   con = e_util_container_number_get(con_num);
   if (!con) return NULL;
   return e_container_zone_number_get(con, zone_num);
}

EAPI E_Zone *
e_util_container_zone_id_get(int con_num, int id)
{
   E_Container *con;

   con = e_util_container_number_get(con_num);
   if (!con) return NULL;
   return e_container_zone_id_get(con, id);
}

EAPI int
e_util_head_exec(int head, const char *cmd)
{
   char *penv_display;
   char *p1, *p2;
   char buf[4096], buf2[32];
   int ok = 0;
   Ecore_Exe *exe;

   penv_display = getenv("DISPLAY");
   if (!penv_display) return 0;
   penv_display = strdup(penv_display);
   /* set env vars */
   p1 = strrchr(penv_display, ':');
   p2 = strrchr(penv_display, '.');
   if ((p1) && (p2) && (p2 > p1)) /* "blah:x.y" */
     {
        /* yes it could overflow... but who will overflow DISPLAY eh? why? to
         * "exploit" your own applications running as you?
         */
        strcpy(buf, penv_display);
        buf[p2 - penv_display + 1] = 0;
        snprintf(buf2, sizeof(buf2), "%i", head);
        strcat(buf, buf2);
     }
   else if (p1) /* "blah:x */
     {
        strcpy(buf, penv_display);
        snprintf(buf2, sizeof(buf2), ".%i", head);
        strcat(buf, buf2);
     }
   else
     strcpy(buf, penv_display);

   ok = 1;
   exe = ecore_exe_run(cmd, NULL);
   if (!exe)
     {
        e_util_dialog_show(_("Run Error"),
                           _("Enlightenment was unable to fork a child process:<br>"
                             "<br>"
                             "%s<br>"),
                           cmd);
        ok = 0;
     }

   /* reset env vars */
   if (penv_display)
     {
        e_util_env_set("DISPLAY", penv_display);
        free(penv_display);
     }
   return ok;
}

EAPI int
e_util_strcmp(const char *s1, const char *s2)
{
   if ((s1) && (s2))
     {
        if (s1 == s2) return 0;
        return strcmp(s1, s2);
     }
   return 0x7fffffff;
}

EAPI int
e_util_strcasecmp(const char *s1, const char *s2)
{
   if ((!s1) && (!s2)) return 0;
   if (!s1) return -1;
   if (!s2) return 1;
   return strcasecmp(s1, s2);
}

EAPI int
e_util_both_str_empty(const char *s1, const char *s2)
{
   int empty = 0;

   if ((!s1) && (!s2)) return 1;
   if ((!s1) || ((s1) && (s1[0] == 0))) empty++;
   if ((!s2) || ((s2) && (s2[0] == 0))) empty++;
   if (empty == 2) return 1;
   return 0;
}

EAPI int
e_util_immortal_check(void)
{
   Eina_List *wins;

   wins = e_border_immortal_windows_get();
   if (wins)
     {
        e_util_dialog_show(_("Cannot exit - immortal windows."),
                           _("Some windows are left still around with the Lifespan lock enabled. This means<br>"
                             "that Enlightenment will not allow itself to exit until these windows have<br>"
                             "been closed or have the lifespan lock removed.<br>"));
        /* FIXME: should really display a list of these lifespan locked */
        /* windows in a dialog and let the user disable their locks in */
        /* this dialog */
        eina_list_free(wins);
        return 1;
     }
   return 0;
}

EAPI int
e_util_edje_icon_list_check(const char *list)
{
   char *buf;
   const char *p;
   const char *c;

   if ((!list) || (!list[0])) return 0;
   buf = alloca(strlen(list) + 1);
   p = list;
   while (p)
     {
        c = strchr(p, ',');
        if (c)
          {
             strncpy(buf, p, c - p);
             buf[c - p] = 0;
             if (e_util_edje_icon_check(buf)) return 1;
             p = c + 1;
             if (!*p) return 0;
          }
        else
          {
             strcpy(buf, p);
             if (e_util_edje_icon_check(buf)) return 1;
             return 0;
          }
     }
   return 0;
}

EAPI int
e_util_edje_icon_list_set(Evas_Object *obj, const char *list)
{
   char *buf;
   const char *p;
   const char *c;

   if ((!list) || (!list[0])) return 0;
   buf = alloca(strlen(list) + 1);
   p = list;
   while (p)
     {
        c = strchr(p, ',');
        if (c)
          {
             strncpy(buf, p, c - p);
             buf[c - p] = 0;
             if (e_util_edje_icon_set(obj, buf)) return 1;
             p = c + 1;
             if (!*p) return 0;
          }
        else
          {
             strcpy(buf, p);
             if (e_util_edje_icon_set(obj, buf)) return 1;
             return 0;
          }
     }
   return 0;
}

EAPI int
e_util_menu_item_edje_icon_list_set(E_Menu_Item *mi, const char *list)
{
   char *buf;
   const char *p;
   char *c;

   if ((!list) || (!list[0])) return 0;
   buf = alloca(strlen(list) + 1);
   p = list;
   while (p)
     {
        c = strchr(p, ',');
        if (c)
          {
             strncpy(buf, p, c - p);
             buf[c - p] = 0;
             if (e_util_menu_item_theme_icon_set(mi, buf)) return 1;
             p = c + 1;
             if (!*p) return 0;
          }
        else
          {
             strcpy(buf, p);
             if (e_util_menu_item_theme_icon_set(mi, buf)) return 1;
             return 0;
          }
     }
   return 0;
}

EAPI int
e_util_edje_icon_check(const char *name)
{
   const char *file;
   char buf[4096];

   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", name);
   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0]) return 1;
   return 0;
}

/* WARNING This function is deprecated,. must be made static.
 * You should use e_util_icon_theme_set instead
 */
EAPI int
e_util_edje_icon_set(Evas_Object *obj, const char *name)
{
   const char *file;
   char buf[4096];

   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", name);
   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
     {
        edje_object_file_set(obj, file, buf);
        return 1;
     }
   return 0;
}

static int
_e_util_icon_theme_set(Evas_Object *obj, const char *icon, Eina_Bool fallback)
{
   const char *file;
   char buf[4096];

   if ((!icon) || (!icon[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", icon);

   if (fallback)
     file = e_theme_edje_icon_fallback_file_get(buf);
   else
     file = e_theme_edje_file_get("base/theme/icons", buf);

   if (file[0])
     {
        e_icon_file_edje_set(obj, file, buf);
        return 1;
     }

   return 0;
}

static int
_e_util_icon_fdo_set(Evas_Object *obj, const char *icon)
{
   const char *path = NULL;
   unsigned int size;

   if ((!icon) || (!icon[0])) return 0;
   size = e_icon_scale_size_get(obj);
   if (size < 16) size = 16;
   size = e_util_icon_size_normalize(size * e_scale);

   path = efreet_icon_path_find(e_config->icon_theme, icon, size);
   if (!path) return 0;

   e_icon_file_set(obj, path);
   return 1;
}

/* use e_icon_size_scale_set(obj, size) to set the preferred icon size */
EAPI int
e_util_icon_theme_set(Evas_Object *obj, const char *icon)
{
   if (e_config->icon_theme_overrides)
     {
        if (_e_util_icon_fdo_set(obj, icon))
          return 1;
        if (_e_util_icon_theme_set(obj, icon, EINA_FALSE))
          return 1;
        return _e_util_icon_theme_set(obj, icon, EINA_TRUE);
     }
   else
     {
        if (_e_util_icon_theme_set(obj, icon, EINA_FALSE))
          return 1;
        if (_e_util_icon_fdo_set(obj, icon))
          return 1;
        return _e_util_icon_theme_set(obj, icon, EINA_TRUE);
     }
}

int
_e_util_menu_item_edje_icon_set(E_Menu_Item *mi, const char *name, Eina_Bool fallback)
{
   const char *file;
   char buf[4096];

   if ((!name) || (!name[0])) return 0;

   if ((!fallback) && (name[0] == '/') && ecore_file_exists(name))
     {
        e_menu_item_icon_edje_set(mi, name, "icon");
        return 1;
     }
   snprintf(buf, sizeof(buf), "e/icons/%s", name);

   if (fallback)
     file = e_theme_edje_icon_fallback_file_get(buf);
   else
     file = e_theme_edje_file_get("base/theme/icons", buf);

   if (file[0])
     {
        e_menu_item_icon_edje_set(mi, file, buf);
        return 1;
     }
   return 0;
}

EAPI unsigned int
e_util_icon_size_normalize(unsigned int desired)
{
   const unsigned int *itr, known_sizes[] =
   {
      16, 22, 24, 32, 36, 48, 64, 72, 96, 128, 192, 256, 0
   };

   for (itr = known_sizes; *itr > 0; itr++)
     if (*itr >= desired)
       return *itr;

   return 256; /* largest know size? */
}

static int
_e_util_menu_item_fdo_icon_set(E_Menu_Item *mi, const char *icon)
{
   const char *path = NULL;
   unsigned int size;

   if ((!icon) || (!icon[0])) return 0;
   size = e_util_icon_size_normalize(24 * e_scale);
   path = efreet_icon_path_find(e_config->icon_theme, icon, size);
   if (!path) return 0;
   e_menu_item_icon_file_set(mi, path);
   return 1;
}

EAPI int
e_util_menu_item_theme_icon_set(E_Menu_Item *mi, const char *icon)
{
   if (e_config->icon_theme_overrides)
     {
        if (_e_util_menu_item_fdo_icon_set(mi, icon))
          return 1;
        if (_e_util_menu_item_edje_icon_set(mi, icon, EINA_FALSE))
          return 1;
        return _e_util_menu_item_edje_icon_set(mi, icon, EINA_TRUE);
     }
   else
     {
        if (_e_util_menu_item_edje_icon_set(mi, icon, EINA_FALSE))
          return 1;
        if (_e_util_menu_item_fdo_icon_set(mi, icon))
          return 1;
        return _e_util_menu_item_edje_icon_set(mi, icon, EINA_TRUE);
     }
}

EAPI const char *
e_util_mime_icon_get(const char *mime, unsigned int size)
{
   char buf[1024];
   const char *file = NULL;

   if (e_config->icon_theme_overrides)
     file = efreet_mime_type_icon_get(mime, e_config->icon_theme, e_util_icon_size_normalize(size));
   if (file) return file;

   if (snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", mime) >= (int)sizeof(buf))
     return NULL;

   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file && file[0]) return file;
   return efreet_mime_type_icon_get(mime, e_config->icon_theme, e_util_icon_size_normalize(size));
}

EAPI E_Container *
e_util_container_window_find(Ecore_X_Window win)
{
   Eina_List *l, *ll;
   E_Manager *man;
   E_Container *con;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        EINA_LIST_FOREACH(man->containers, ll, con)
          {
             if ((con->win == win) || (con->bg_win == win) ||
                 (con->event_win == win))
               return con;
          }
     }
   return NULL;
}

EAPI E_Zone *
e_util_zone_window_find(Ecore_X_Window win)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     EINA_LIST_FOREACH(man->containers, ll, con)
       EINA_LIST_FOREACH(con->zones, lll, zone)
         if (zone->black_win == win) return zone;

   return NULL;
}

static int
_e_util_layer_map(int layer)
{
   int pos = 0;
   
   if (layer < 0) layer = 0;
   pos = 1 + (layer / 50);
   if (pos > 10) pos = 10;
   return pos;
}

EAPI E_Border *
e_util_desk_border_above(E_Border *bd)
{
   E_Border *bd2, *above = NULL;
   Eina_List *l;
   int pos, i;

   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);

   pos = _e_util_layer_map(bd->layer);

   EINA_LIST_FOREACH(eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd), l, bd2)
     {
        if (!eina_list_next(l) || above) break;
        above = eina_list_data_get(eina_list_next(l));
        if ((above->desk != bd->desk) && (!above->sticky))
          above = NULL;
     }
   if (!above)
     {
        /* Need to check the layers above */
        for (i = pos + 1; (i < 7) && (!above); i++)
          {
             EINA_LIST_FOREACH(bd->zone->container->layers[i].clients, l, bd2)
               {
                  if (above) break;
                  above = bd2;
                  if ((above->desk != bd->desk) && (!above->sticky))
                    above = NULL;
               }
          }
     }
   return above;
}

EAPI E_Border *
e_util_desk_border_below(E_Border *bd)
{
   E_Border *below = NULL, *bd2;
   Eina_List *l;
   int pos, i;

   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);

   pos = _e_util_layer_map(bd->layer);

   for (l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd); l; l = l->prev)
     {
        if (!eina_list_prev(l) || below) break;
        below = eina_list_data_get(eina_list_prev(l));
        if ((below->desk != bd->desk) && (!below->sticky))
          below = NULL;
     }
   if (!below)
     {
        /* Need to check the layers below */
        for (i = pos - 1; (i >= 0) && (!below); i--)
          {
             if (bd->zone->container->layers[i].clients)
               {
                  l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd);
                  for (; l && !below; l = l->prev)
                    {
                       bd2 = l->data;
                       below = bd2;
                       if ((below->desk != bd->desk) && (!below->sticky))
                         below = NULL;
                    }
               }
          }
     }

   return below;
}

EAPI int
e_util_edje_collection_exists(const char *file, const char *coll)
{
   Eina_List *clist, *l;
   char *str;

   clist = edje_file_collection_list(file);
   EINA_LIST_FOREACH(clist, l, str)
     {
        if (!strcmp(coll, str))
          {
             edje_file_collection_list_free(clist);
             return 1;
          }
     }
   edje_file_collection_list_free(clist);
   return 0;
}

EAPI E_Dialog *
e_util_dialog_internal(const char *title, const char *txt)
{
   E_Dialog *dia;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_error_dialog");
   if (!dia) return NULL;
   e_dialog_title_set(dia, title);
   e_dialog_text_set(dia, txt);
   e_dialog_icon_set(dia, "dialog-error", 64);
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   return dia;
}

EAPI const char *
e_util_filename_escape(const char *filename)
{
   const char *p;
   char *q;
   static char buf[PATH_MAX];

   if (!filename) return NULL;
   p = filename;
   q = buf;
   while (*p)
     {
        if ((q - buf) > 4090) return NULL;
        if (
          (*p == ' ') || (*p == '\t') || (*p == '\n') ||
          (*p == '\\') || (*p == '\'') || (*p == '\"') ||
          (*p == ';') || (*p == '!') || (*p == '#') ||
          (*p == '$') || (*p == '%') || (*p == '&') ||
          (*p == '*') || (*p == '(') || (*p == ')') ||
          (*p == '[') || (*p == ']') || (*p == '{') ||
          (*p == '}') || (*p == '|') || (*p == '<') ||
          (*p == '>') || (*p == '?')
          )
          {
             *q = '\\';
             q++;
          }
        *q = *p;
        q++;
        p++;
     }
   *q = 0;
   return buf;
}

EAPI int
e_util_icon_save(Ecore_X_Icon *icon, const char *filename)
{
   Ecore_Evas *ee;
   Evas *evas;
   Evas_Object *im;
   int ret;

   ee = ecore_evas_buffer_new(icon->width, icon->height);
   if (!ee) return 0;
   evas = ecore_evas_get(ee);
   evas_image_cache_set(evas, 0);
   evas_font_cache_set(evas, 0);

   im = evas_object_image_add(evas);
   if (!im)
     {
        ecore_evas_free(ee);
        return 0;
     }
   evas_object_move(im, 0, 0);
   evas_object_resize(im, icon->width, icon->height);
   evas_object_image_size_set(im, icon->width, icon->height);
   evas_object_image_data_copy_set(im, icon->data);
   evas_object_image_alpha_set(im, 1);
   evas_object_show(im);
   ret = evas_object_image_save(im, filename, NULL, NULL);
   evas_object_del(im);
   ecore_evas_free(ee);
   return ret;
}

EAPI char *
e_util_shell_env_path_eval(const char *path)
{
   /* evaluate things like:
    * $HOME/bling -> /home/user/bling
    * $HOME/bin/$HOSTNAME/blah -> /home/user/bin/localhost/blah
    * etc. etc.
    */
   const char *p, *v2, *v1 = NULL;
   char buf[PATH_MAX], *pd, *s, *vp;
   char *v = NULL;
   int esc = 0, invar = 0;

   for (p = path, pd = buf; (pd < (buf + sizeof(buf) - 1)); p++)
     {
        if (invar)
          {
             if (!((isalnum(*p)) || (*p == '_')))
               {
                  v2 = p;
                  invar = 0;
                  if ((v2 - v1) > 1)
                    {
                       s = alloca(v2 - v1);
                       strncpy(s, v1 + 1, v2 - v1 - 1);
                       s[v2 - v1 - 1] = 0;
                       if (strncmp(s, "XDG", 3))
                         v = getenv(s);
                       else
                         {
                            if (!strcmp(s, "XDG_CONFIG_HOME"))
                              v = (char *)efreet_config_home_get();
                            else if (!strcmp(s, "XDG_CACHE_HOME"))
                              v = (char *)efreet_cache_home_get();
                            else if (!strcmp(s, "XDG_DATA_HOME"))
                              v = (char *)efreet_data_home_get();
                         }

                       if (v)
                         {
                            vp = v;
                            while ((*vp) && (pd < (buf + sizeof(buf) - 1)))
                              {
                                 *pd = *vp;
                                 vp++;
                                 pd++;
                              }
                         }
                    }
                  if (pd < (buf + sizeof(buf) - 1))
                    {
                       *pd = *p;
                       pd++;
                    }
               }
          }
        else
          {
             if (esc)
               {
                  *pd = *p;
                  pd++;
               }
             else
               {
                  if (*p == '\\') esc = 1;
                  else if (*p == '$')
                    {
                       invar = 1;
                       v1 = p;
                    }
                  else
                    {
                       *pd = *p;
                       pd++;
                    }
               }
          }
        if (*p == 0) break;
     }
   *pd = 0;
   return strdup(buf);
}

EAPI char *
e_util_size_string_get(off_t size)
{
   double dsize;
   char buf[256];

   dsize = (double)size;
   if (dsize < 1024.0) snprintf(buf, sizeof(buf), _("%'.0f bytes"), dsize);
   else
     {
        dsize /= 1024.0;
        if (dsize < 1024) snprintf(buf, sizeof(buf), _("%'.0f KiB"), dsize);
        else
          {
             dsize /= 1024.0;
             if (dsize < 1024) snprintf(buf, sizeof(buf), _("%'.1f MiB"), dsize);
             else
               {
                  dsize /= 1024.0;
                  if (dsize < 1024) snprintf(buf, sizeof(buf), _("%'.1f GiB"), dsize);
                  else
                    {
                       dsize /= 1024.0;
                       snprintf(buf, sizeof(buf), _("%'.1f TiB"), dsize);
                    }
               }
          }
     }
   return strdup(buf);
}

EAPI char *
e_util_file_time_get(time_t ftime)
{
   time_t diff, ltime, test;
   char buf[256];
   char *s = NULL;

   ltime = time(NULL);
   diff = ltime - ftime;
   buf[0] = 0;
   if (ftime > ltime)
     snprintf(buf, sizeof(buf), _("In the future"));
   else
     {
        if (diff <= 60)
          snprintf(buf, sizeof(buf), _("In the last minute"));
        else if (diff >= 31526000)
          {
             test = diff / 31526000;
             snprintf(buf, sizeof(buf), P_("Last year", "%li Years ago", test), test);
          }
        else if (diff >= 2592000)
          {
             test = diff / 2592000;
             snprintf(buf, sizeof(buf), P_("Last month", "%li Months ago", test), test);
          }
        else if (diff >= 604800)
          {
             test = diff / 604800;
             snprintf(buf, sizeof(buf), P_("Last week", "%li Weeks ago", test), test);
          }
        else if (diff >= 86400)
          {
             test = diff / 86400;
             snprintf(buf, sizeof(buf), P_("Yesterday", "%li Days ago", test), test);
          }
        else if (diff >= 3600)
          {
             test = diff / 3600;
             snprintf(buf, sizeof(buf), P_("An hour ago", "%li Hours ago", test), test);
          }
        else if (diff > 60)
          {
             test = diff / 60;
             snprintf(buf, sizeof(buf), P_("A minute ago", "%li Minutes ago", test), test);
          }
     }

   if (buf[0])
     s = strdup(buf);
   else
     s = strdup(_("Unknown"));
   return s;
}

EAPI Evas_Object *
e_util_icon_add(const char *path, Evas *evas)
{
   return _e_util_icon_add(path, evas, 64);
}

EAPI Evas_Object *
e_util_desktop_icon_add(Efreet_Desktop *desktop, unsigned int size, Evas *evas)
{
   if ((!desktop) || (!desktop->icon)) return NULL;
   return e_util_icon_theme_icon_add(desktop->icon, size, evas);
}

EAPI Evas_Object *
e_util_icon_theme_icon_add(const char *icon_name, unsigned int size, Evas *evas)
{
   if (!icon_name) return NULL;
   if (icon_name[0] == '/') return _e_util_icon_add(icon_name, evas, size);
   else
     {
        Evas_Object *obj;
        const char *path;

        path = efreet_icon_path_find(e_config->icon_theme, icon_name, size);
        if (path)
          {
             obj = _e_util_icon_add(path, evas, size);
             return obj;
          }
     }
   return NULL;
}

EAPI void
e_util_desktop_menu_item_icon_add(Efreet_Desktop *desktop, unsigned int size, E_Menu_Item *mi)
{
   const char *path = NULL;

   if ((!desktop) || (!desktop->icon)) return;

   if (desktop->icon[0] == '/') path = desktop->icon;
   else path = efreet_icon_path_find(e_config->icon_theme, desktop->icon, size);

   if (path)
     {
        const char *ext;

        ext = strrchr(path, '.');
        if (ext)
          {
             if (strcmp(ext, ".edj") == 0)
               e_menu_item_icon_edje_set(mi, path, "icon");
             else
               e_menu_item_icon_file_set(mi, path);
          }
        else
          e_menu_item_icon_file_set(mi, path);
     }
}

EAPI int
e_util_dir_check(const char *dir)
{
   if (!ecore_file_exists(dir))
     {
        if (!ecore_file_mkpath(dir))
          {
             e_util_dialog_show(_("Error creating directory"), _("Failed to create directory: %s .<br>Check that you have correct permissions set."), dir);
             return 0;
          }
     }
   else
     {
        if (!ecore_file_is_dir(dir))
          {
             e_util_dialog_show(_("Error creating directory"), _("Failed to create directory: %s .<br>A file of that name already exists."), dir);
             return 0;
          }
     }
   return 1;
}

EAPI void
e_util_defer_object_del(E_Object *obj)
{
   if (stopping)
     e_object_del(obj);
   else
     {
        Ecore_Idle_Enterer *idler;

        idler = ecore_idle_enterer_before_add(_e_util_cb_delayed_del, obj);
        if (idler) e_object_delfn_add(obj, _e_util_cb_delayed_cancel, idler);
     }
}

EAPI const char *
e_util_winid_str_get(Ecore_X_Window win)
{
   const char *vals = "qWeRtYuIoP5-$&<~";
   static char id[9];
   unsigned int val;

   val = (unsigned int)win;
   id[0] = vals[(val >> 28) & 0xf];
   id[1] = vals[(val >> 24) & 0xf];
   id[2] = vals[(val >> 20) & 0xf];
   id[3] = vals[(val >> 16) & 0xf];
   id[4] = vals[(val >> 12) & 0xf];
   id[5] = vals[(val >> 8) & 0xf];
   id[6] = vals[(val >> 4) & 0xf];
   id[7] = vals[(val) & 0xf];
   id[8] = 0;
   return id;
}

static int
_win_auto_size_calc(int max, int min)
{
   const float *itr, scales[] = {0.25, 0.3, 0.5, 0.75, 0.8, 0.9, 0.95, -1};

   for (itr = scales; *itr > 0; itr++)
     {
        int value = *itr * max;
        if (value > min) /* not >=, try a bit larger */
          return value;
     }

   return min;
}

EAPI void
e_util_win_auto_resize_fill(E_Win *win)
{
   E_Zone *zone = NULL;

   if (win->border)
     zone = win->border->zone;
   if ((!zone) && (win->container))
     zone = e_util_zone_current_get(win->container->manager);

   if (zone)
     {
        int w, h;

        e_zone_useful_geometry_get(zone, NULL, NULL, &w, &h);

        w = _win_auto_size_calc(w, win->min_w);
        h = _win_auto_size_calc(h, win->min_h);
        e_win_resize(win, w, h);
     }
}

EAPI int
e_util_container_desk_count_get(E_Container *con)
{
   Eina_List *zl;
   E_Zone *zone;
   int count = 0;

   E_OBJECT_CHECK_RETURN(con, 0);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, 0);
   EINA_LIST_FOREACH(con->zones, zl, zone)
     {
        int x, y;
        int cx = 0, cy = 0;

        e_zone_desk_count_get(zone, &cx, &cy);
        for (x = 0; x < cx; x++)
          {
             for (y = 0; y < cy; y++)
               count += 1;
          }
     }
   return count;
}

/* local subsystem functions */

static Evas_Object *
_e_util_icon_add(const char *path, Evas *evas, int size)
{
   Evas_Object *o = NULL;
   const char *ext;

   if (!path) return NULL;
   if (!ecore_file_exists(path)) return NULL;

   o = e_icon_add(evas);
   e_icon_scale_size_set(o, size);
   e_icon_preload_set(o, 1);
   ext = strrchr(path, '.');
   if (ext)
     {
        if (!strcmp(ext, ".edj"))
          e_icon_file_edje_set(o, path, "icon");
        else
          e_icon_file_set(o, path);
     }
   else
     e_icon_file_set(o, path);
   e_icon_fill_inside_set(o, 1);

   return o;
}

static Eina_Bool
_e_util_cb_delayed_del(void *data)
{
   e_object_del(E_OBJECT(data));
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_util_cb_delayed_cancel(void *data, void *obj __UNUSED__)
{
   Ecore_Idle_Enterer *idler = data;

   ecore_idle_enterer_del(idler);
}

static Eina_Bool
_e_util_wakeup_cb(void *data __UNUSED__)
{
   _e_util_dummy_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_util_conf_timer_old(void *data)
{
   char *module_name = data;
   char buf[4096];
   char *msg = _("Configuration data needed upgrading. Your old configuration<br>"
                 "has been wiped and a new set of defaults initialized. This<br>"
                 "will happen regularly during development, so don't report a<br>"
                 "bug. This means the module needs new configuration<br>"
                 "data by default for usable functionality that your old<br>"
                 "configuration lacked. This new set of defaults will fix<br>"
                 "that by adding it in. You can re-configure things now to your<br>"
                 "liking. Sorry for the hiccup in your configuration.<br>");

   snprintf(buf, sizeof(buf), N_("%s Configuration Updated"), module_name);
   e_util_dialog_internal(buf, msg);
   E_FREE(module_name);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_util_conf_timer_new(void *data)
{
   char *module_name = data;
   char buf[4096];
   char *msg =
     _("Your module configuration is NEWER "
       "than the module version. This is "
       "very<br>strange. This should not happen unless"
       " you downgraded<br>the module or "
       "copied the configuration from a place where"
       "<br>a newer version of the module "
       "was running. This is bad and<br>as a "
       "precaution your configuration has been now "
       "restored to<br>defaults. Sorry for the "
       "inconvenience.<br>");

   snprintf(buf, sizeof(buf), _("%s Configuration Updated"), module_name);
   e_util_dialog_internal(buf, msg);
   E_FREE(module_name);

   return ECORE_CALLBACK_CANCEL;
}

EAPI Eina_Bool
e_util_module_config_check(const char *module_name, int loaded, int current)
{
   if (loaded > current)
     {
        ecore_timer_add(1.0, _e_util_conf_timer_new, strdup(module_name));
        return EINA_FALSE;
     }
   loaded -= loaded % 1000000, current -= current % 1000000;
   if (loaded < current)
     {
        ecore_timer_add(1.0, _e_util_conf_timer_old, strdup(module_name));
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * Checks whenever the current manager/container/zone have fullscreen windows.
 */
EAPI Eina_Bool
e_util_fullscreen_current_any(void)
{
   E_Manager *man = e_manager_current_get();
   E_Container *con = e_container_current_get(man);
   E_Zone *zone = e_zone_current_get(con);
   E_Desk *desk;

   if ((zone) && (zone->fullscreen > 0)) return EINA_TRUE;
   desk = e_desk_current_get(zone);
   if ((desk) && (desk->fullscreen_borders > 0)) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * Checks whenever any manager/container/zone have fullscreen windows.
 */
EAPI Eina_Bool
e_util_fullscreen_any(void)
{
   E_Zone *zone;
   Eina_List *lm, *lc, *lz;
   E_Container *con;
   E_Manager *man;
   E_Desk *desk;
   int x, y;

   EINA_LIST_FOREACH(e_manager_list(), lm, man)
     {
        EINA_LIST_FOREACH(man->containers, lc, con)
          {
             EINA_LIST_FOREACH(con->zones, lz, zone)
               {
                  if (zone->fullscreen > 0) return EINA_TRUE;

                  for (x = 0; x < zone->desk_x_count; x++)
                    for (y = 0; y < zone->desk_y_count; y++)
                      {
                         desk = e_desk_at_xy_get(zone, x, y);
                         if ((desk) && (desk->fullscreen_borders > 0))
                           return EINA_TRUE;
                      }
               }
          }
     }
   return EINA_FALSE;
}

EAPI const char *
e_util_time_str_get(long int seconds)
{
   static char buf[1024];
   long int test;

   if (seconds < 0)
     snprintf(buf, sizeof(buf), _("Never"));
   else
     {
        if (seconds <= 60)
          snprintf(buf, sizeof(buf), P_("A second", "%li Seconds", seconds), seconds);
        else if (seconds >= 31526000)
          {
             test = seconds / 31526000;
             snprintf(buf, sizeof(buf), P_("One year", "%li Years", test), test);
          }
        else if (seconds >= 2592000)
          {
             test = seconds / 2592000;
             snprintf(buf, sizeof(buf), P_("One month", "%li Months", test), test);
          }
        else if (seconds >= 604800)
          {
             test = seconds / 604800;
             snprintf(buf, sizeof(buf), P_("One week", "%li Weeks", test), test);
          }
        else if (seconds >= 86400)
          {
             test = seconds / 86400;
             snprintf(buf, sizeof(buf), P_("One day", "%li Days", test), test);
          }
        else if (seconds >= 3600)
          {
             test = seconds / 3600;
             snprintf(buf, sizeof(buf), P_("An hour", "%li Hours", test), test);
          }
        else if (seconds > 60)
          {
             test = seconds / 60;
             snprintf(buf, sizeof(buf), P_("A minute", "%li Minutes", test), test);
          }
     }
   return buf;
}

static void
_e_util_size_debug(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   int x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   fprintf(stderr, "OBJ[%p]: (%d,%d) - %dx%d\n", obj, x, y, w, h);
}

EAPI void
e_util_size_debug_set(Evas_Object *obj, Eina_Bool enable)
{
   if (enable)
     {
        evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, 
                                       _e_util_size_debug, NULL);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, 
                                       _e_util_size_debug, NULL);
     }
   else
     {
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE, 
                                            _e_util_size_debug, NULL);
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, 
                                            _e_util_size_debug, NULL);
     }
}

static Efreet_Desktop *
_e_util_default_terminal_get(const char *defaults_list)
{
   Efreet_Desktop *tdesktop = NULL;
   Efreet_Ini *ini;
   const char *s;
   
   ini = efreet_ini_new(defaults_list);
   if ((ini) && (ini->data) &&
       (efreet_ini_section_set(ini, "Default Applications")) &&
       (ini->section))
     {
        s = efreet_ini_string_get(ini, "x-scheme-handler/terminal");
        if (s) tdesktop = efreet_util_desktop_file_id_find(s);
     }
   if (ini) efreet_ini_free(ini);
   return tdesktop;
}

EAPI Efreet_Desktop *
e_util_terminal_desktop_get(void)
{
   const char *terms[] =
     {
        "terminology.desktop",
        "xterm.desktop",
        "rxvt.desktop",
        "gnome-terminal.desktop",
        "konsole.desktop",
        NULL
     };
   const char *s;
   char buf[PATH_MAX];
   Efreet_Desktop *tdesktop = NULL, *td;
   Eina_List *l;
   int i;

   s = efreet_data_home_get();
   if (s)
     {
        snprintf(buf, sizeof(buf), "%s/applications/defaults.list", s);
        tdesktop = _e_util_default_terminal_get(buf);
     }
   if (tdesktop) return tdesktop;
   EINA_LIST_FOREACH(efreet_data_dirs_get(), l, s)
     {
        snprintf(buf, sizeof(buf), "%s/applications/defaults.list", s);
        tdesktop = _e_util_default_terminal_get(buf);
        if (tdesktop) return tdesktop;
     }
   
   for (i = 0; terms[i]; i++)
     {
        tdesktop = efreet_util_desktop_file_id_find(terms[i]);
        if (tdesktop) return tdesktop;
     }
   if (!tdesktop)
     {
        l = efreet_util_desktop_category_list("TerminalEmulator");
        if (l)
          {
             // just take first one since above list doesn't work.
             tdesktop = l->data;
             EINA_LIST_FREE(l, td)
               {
                  // free/unref the desktosp we are not going to use
                  if (td != tdesktop) efreet_desktop_free(td);
               }
          }
     }
   return tdesktop;
}


EAPI E_Config_Binding_Key *
e_util_binding_match(const Eina_List *bindlist, Ecore_Event_Key *ev, unsigned int *num, const E_Config_Binding_Key *skip)
{
   E_Config_Binding_Key *bi;
   const Eina_List *l;
   unsigned int mod = E_BINDING_MODIFIER_NONE;

   if (num) *num = 0;

   if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
     mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
     mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
     mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
     mod |= E_BINDING_MODIFIER_WIN;
   /* see comment in e_bindings on numlock
      if (ev->modifiers & ECORE_X_LOCK_NUM)
      mod |= ECORE_X_LOCK_NUM;
    */
   EINA_LIST_FOREACH(bindlist ?: e_config->key_bindings, l, bi)
     {
        if (bi != skip)
          {
             if ((bi->modifiers == mod) && (!strcmp(bi->key, ev->key)))
               return bi;
          }
        if (num) (*num)++;
     }
   if (num) *num = 0;
   return NULL;
}

EAPI void
e_util_gadcon_orient_icon_set(E_Gadcon_Orient orient, Evas_Object *obj)
{
   switch (orient)
     {
      case E_GADCON_ORIENT_LEFT:
        e_util_icon_theme_set(obj, "preferences-position-left");
        break;

      case E_GADCON_ORIENT_RIGHT:
        e_util_icon_theme_set(obj, "preferences-position-right");
        break;

      case E_GADCON_ORIENT_TOP:
        e_util_icon_theme_set(obj, "preferences-position-top");
        break;

      case E_GADCON_ORIENT_BOTTOM:
        e_util_icon_theme_set(obj, "preferences-position-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        e_util_icon_theme_set(obj, "preferences-position-top-left");
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        e_util_icon_theme_set(obj, "preferences-position-top-right");
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        e_util_icon_theme_set(obj, "preferences-position-bottom-left");
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        e_util_icon_theme_set(obj, "preferences-position-bottom-right");
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        e_util_icon_theme_set(obj, "preferences-position-left-top");
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        e_util_icon_theme_set(obj, "preferences-position-right-top");
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        e_util_icon_theme_set(obj, "preferences-position-left-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        e_util_icon_theme_set(obj, "preferences-position-right-bottom");
        break;

      default:
        e_util_icon_theme_set(obj, "enlightenment");
        break;
     }
}

EAPI void
e_util_gadcon_orient_menu_item_icon_set(E_Gadcon_Orient orient, E_Menu_Item *mi)
{
   switch (orient)
     {
      case E_GADCON_ORIENT_LEFT:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-left");
        break;

      case E_GADCON_ORIENT_RIGHT:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-right");
        break;

      case E_GADCON_ORIENT_TOP:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-top");
        break;

      case E_GADCON_ORIENT_BOTTOM:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_TL:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-top-left");
        break;

      case E_GADCON_ORIENT_CORNER_TR:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-top-right");
        break;

      case E_GADCON_ORIENT_CORNER_BL:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-bottom-left");
        break;

      case E_GADCON_ORIENT_CORNER_BR:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-bottom-right");
        break;

      case E_GADCON_ORIENT_CORNER_LT:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-left-top");
        break;

      case E_GADCON_ORIENT_CORNER_RT:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-right-top");
        break;

      case E_GADCON_ORIENT_CORNER_LB:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-left-bottom");
        break;

      case E_GADCON_ORIENT_CORNER_RB:
        e_util_menu_item_theme_icon_set(mi, "preferences-position-right-bottom");
        break;

      default:
        e_util_menu_item_theme_icon_set(mi, "enlightenment");
        break;
     }
}


EAPI char *
e_util_string_append_char(char *str, size_t *size, size_t *len, char c)
{
   if (!str)
     {
        str = malloc(4096);
        if (!str) return NULL;
        str[0] = 0;
        *size = 4096;
        *len = 0;
     }

   if (*len >= *size - 1)
     {
        char *str2;
        
        *size += 1024;
        str2 = realloc(str, *size);
        if (!str2)
          {
             *size = 0;
             free(str);
             return NULL;
          }
        str = str2;
     }

   str[(*len)++] = c;
   str[*len] = 0;

   return str;
}

EAPI char *
e_util_string_append_quoted(char *str, size_t *size, size_t *len, const char *src)
{
   str = e_util_string_append_char(str, size, len, '\'');
   if (!str) return NULL;

   while (*src)
     {
        if (*src == '\'')
          {
             str = e_util_string_append_char(str, size, len, '\'');
             if (!str) return NULL;
             str = e_util_string_append_char(str, size, len, '\\');
             if (!str) return NULL;
             str = e_util_string_append_char(str, size, len, '\'');
             if (!str) return NULL;
             str = e_util_string_append_char(str, size, len, '\'');
             if (!str) return NULL;
          }
        else
          {
             str = e_util_string_append_char(str, size, len, *src);
             if (!str) return NULL;
          }

        src++;
     }

   str = e_util_string_append_char(str, size, len, '\'');
   if (!str) return NULL;

   return str;
}

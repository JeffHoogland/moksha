#include "e.h"

EAPI E_Path *path_data = NULL;
EAPI E_Path *path_images = NULL;
EAPI E_Path *path_fonts = NULL;
EAPI E_Path *path_themes = NULL;
EAPI E_Path *path_icons = NULL;
EAPI E_Path *path_modules = NULL;
EAPI E_Path *path_backgrounds = NULL;
EAPI E_Path *path_messages = NULL;

typedef struct _E_Util_Fake_Mouse_Up_Info E_Util_Fake_Mouse_Up_Info;
typedef struct _E_Util_Image_Import_Settings E_Util_Image_Import_Settings;

struct _E_Util_Fake_Mouse_Up_Info
{
   Evas *evas;
   int button;
};

struct _E_Util_Image_Import_Settings
{
   E_Dialog *dia;
   struct
     {
        void (*func)(void *data, const char *path, Eina_Bool ok, Eina_Bool external, int quality, E_Image_Import_Mode mode);
        void *data;
     } cb;
   const char *path;
   int quality;
   int external;
   int mode;
   Eina_Bool ok;
};

struct _E_Util_Image_Import_Handle
{
   Ecore_Exe *exe;
   Ecore_Event_Handler *handler;
   struct
     {
        void (*func)(void *data, Eina_Bool ok, const char *image_path, const char *edje_path);
        void *data;
     } cb;
   struct
     {
        const char *image, *edje, *temp;
     } path;
};

/* local subsystem functions */
static Eina_Bool _e_util_cb_delayed_del(void *data);
static Eina_Bool _e_util_wakeup_cb(void *data);

static void _e_util_image_import_settings_do(void *data, E_Dialog *dia);
static void _e_util_image_import_settings_del(void *obj);

static Eina_Bool _e_util_image_import_exit(void *data, int type __UNUSED__, void *event);
static void _e_util_image_import_handle_free(E_Util_Image_Import_Handle *handle);
static Evas_Object *_e_util_icon_add(const char *path, Evas *evas, int size);

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
e_util_glob_match(const char *str, const char *glob)
{
   if ((!str) || (!glob)) return 0;
   if (glob[0] == 0)
     {
	if (str[0] == 0) return 1;
	return 0;
     }
   if (!strcmp(glob, "*")) return 1;
   if (!fnmatch(glob, str, 0)) return 1;
   return 0;
}

EAPI int
e_util_glob_case_match(const char *str, const char *glob)
{
   const char *p;
   char *tstr, *tglob, *tp;

   if (glob[0] == 0)
     {
	if (str[0] == 0) return 1;
	return 0;
     }
   if (!strcmp(glob, "*")) return 1;
   tstr = alloca(strlen(str) + 1);
   for (tp = tstr, p = str; *p != 0; p++, tp++) *tp = tolower(*p);
   *tp = 0;
   tglob = alloca(strlen(glob) + 1);
   for (tp = tglob, p = glob; *p != 0; p++, tp++) *tp = tolower(*p);
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
   char buf[PATH_MAX], buf2[32];
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
   e_util_library_path_strip();
   exe = ecore_exe_run(cmd, NULL);
   e_util_library_path_restore();
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
     return strcmp(s1, s2);
   return 0x7fffffff;
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
   char buf[PATH_MAX];

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
   char buf[PATH_MAX];

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
   char buf[PATH_MAX];

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
   char buf[PATH_MAX];

   if ((!name) || (!name[0])) return 0;

   if ((!fallback) && (name[0]=='/') && ecore_file_exists(name))
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
        16, 22, 24, 32, 36, 48, 64, 72, 96, 128, 192, 256, -1
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

EAPI E_Border *
e_util_desk_border_above(E_Border *bd)
{
   E_Border *bd2, *above = NULL;
   Eina_List *l;
   int pos, i;

   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);

   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   EINA_LIST_FOREACH(eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd), l, bd2)
     {
	if(!eina_list_next(l) || above) break;
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

   if (bd->layer == 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

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

EAPI void
e_util_dialog_internal(const char *title, const char *txt)
{
   E_Dialog *dia;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_error_dialog");
   if (!dia) return;
   e_dialog_title_set(dia, title);
   e_dialog_text_set(dia, txt);
   e_dialog_icon_set(dia, "dialog-error", 64);
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
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
   if (dsize < 1024.0) snprintf(buf, sizeof(buf), _("%'.0f Bytes"), dsize);
   else
     {
	dsize /= 1024.0;
	if (dsize < 1024) snprintf(buf, sizeof(buf), _("%'.0f KB"), dsize);
	else
	  {
	     dsize /= 1024.0;
	     if (dsize < 1024) snprintf(buf, sizeof(buf), _("%'.0f MB"), dsize);
	     else
	       {
		  dsize /= 1024.0;
		  snprintf(buf, sizeof(buf), _("%'.1f GB"), dsize);
	       }
	  }
     }
   return strdup(buf);
}

EAPI char *
e_util_file_time_get(time_t ftime)
{
   time_t diff;
   time_t ltime;
   char buf[256];
   char *s = NULL;

   ltime = time(NULL);
   diff = ltime - ftime;
   buf[0] = 0;
   if (ftime > ltime)
     snprintf(buf, sizeof(buf), _("In the Future"));
   else
     {
	if (diff <= 60)
	  snprintf(buf, sizeof(buf), _("In the last Minute"));
	else if (diff >= 31526000)
	  snprintf(buf, sizeof(buf), _("%li Years ago"), (diff / 31526000));
	else if (diff >= 2592000)
	  snprintf(buf, sizeof(buf), _("%li Months ago"), (diff / 2592000));
	else if (diff >= 604800)
	  snprintf(buf, sizeof(buf), _("%li Weeks ago"), (diff / 604800));
	else if (diff >= 86400)
	  snprintf(buf, sizeof(buf), _("%li Days ago"), (diff / 86400));
	else if (diff >= 3600)
	  snprintf(buf, sizeof(buf), _("%li Hours ago"), (diff / 3600));
	else if (diff > 60)
	  snprintf(buf, sizeof(buf), _("%li Minutes ago"), (diff / 60));
     }

   if (buf[0])
     s = strdup(buf);
   else
     s = strdup(_("Unknown"));
   return s;
}

static char *prev_ld_library_path = NULL;
static char *prev_path = NULL;

EAPI void
e_util_library_path_strip(void)
{
   char *p, *p2;

   p = getenv("LD_LIBRARY_PATH");
   E_FREE(prev_ld_library_path);
   if (p)
     {
	prev_ld_library_path = strdup(p);
	p2 = strchr(p, ':');
	if (p2) p2++;
	e_util_env_set("LD_LIBRARY_PATH", p2);
     }
   p = getenv("PATH");
   E_FREE(prev_path);
   if (p)
     {
	prev_path = strdup(p);
	p2 = strchr(p, ':');
	if (p2) p2++;
	e_util_env_set("PATH", p2);
     }
}

EAPI void
e_util_library_path_restore(void)
{
   if (prev_ld_library_path)
     {
        e_util_env_set("LD_LIBRARY_PATH", prev_ld_library_path);
        E_FREE(prev_ld_library_path);
     }
   if (prev_path)
     {
        e_util_env_set("PATH", prev_path);
        E_FREE(prev_path);
     }
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
	     e_util_dialog_show("Error creating directory", "Failed to create directory: %s .<br>Check that you have correct permissions set.", dir);
	     return 0;
	  }
     }
   else
     {
	if (!ecore_file_is_dir(dir))
	  {
	     e_util_dialog_show("Error creating directory", "Failed to create directory: %s .<br>A file of that name already exists.", dir);
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
     ecore_idle_enterer_before_add(_e_util_cb_delayed_del, obj);
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
   id[5] = vals[(val >>  8) & 0xf];
   id[6] = vals[(val >>  4) & 0xf];
   id[7] = vals[(val      ) & 0xf];
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

/**
 * Creates a new dialog to query image import settings, report results.
 *
 * @param path may be used to display live preview (not used so far).
 * @param cb function to call before exit. Last parameter is mode of
 *        image filling.
 * @param data extra data to give to @a cb as first argument.
 *
 * @return newly allocated window on success, @c NULL on failure. If
 *         @c NULL is returned, then callback is never called!
 */
EAPI E_Dialog *
e_util_image_import_settings_new(const char *path, void (*cb)(void *data, const char *path, Eina_Bool ok, Eina_Bool external, int quality, E_Image_Import_Mode mode), const void *data)
{
   Evas *evas;
   E_Util_Image_Import_Settings *ctxt;
   Evas_Object *vlist, *frame, *radio, *check, *slider;
   E_Radio_Group *rg;
   Evas_Coord w, h;

   if (!path) return NULL;
   if (!cb) return NULL;

   ctxt = E_NEW(E_Util_Image_Import_Settings, 1);
   if (!ctxt) return NULL;

   ctxt->dia = e_dialog_new(NULL, "E", "_image_import_settings");
   if (!ctxt->dia)
     {
	E_FREE(ctxt);
	return NULL;
     }

   ctxt->dia->data = ctxt;

   e_object_del_attach_func_set
     (E_OBJECT(ctxt->dia), _e_util_image_import_settings_del);
   e_dialog_title_set(ctxt->dia, _("Image Import Settings"));
   e_dialog_border_icon_set(ctxt->dia, "dialog-ask");

   e_dialog_button_add
     (ctxt->dia, _("Import"), NULL, _e_util_image_import_settings_do, ctxt);
   e_dialog_button_add
     (ctxt->dia, _("Cancel"), NULL, NULL, ctxt);
   e_dialog_button_focus_num(ctxt->dia, 0);

   ctxt->cb.func = cb;
   ctxt->cb.data = (void *)data;
   ctxt->path = eina_stringshare_add(path);
   ctxt->quality = 90;
   ctxt->ok = EINA_FALSE;
   ctxt->external = EINA_FALSE;
   ctxt->mode = E_IMAGE_IMPORT_STRETCH;

   evas = e_win_evas_get(ctxt->dia->win);

   vlist = e_widget_list_add(evas, 0, 0);

   frame = e_widget_frametable_add(evas, _("Fill and Stretch Options"), 1);
   rg = e_widget_radio_group_new(&ctxt->mode);

#define RD(lbl, icon, val, col, row)					\
   radio = e_widget_radio_icon_add					\
     (evas, lbl, "enlightenment/wallpaper_"icon, 24, 24, val, rg);	\
   e_widget_frametable_object_append(frame, radio, col, row, 1, 1, 1, 1, 0, 0)

   RD(_("Stretch"), "stretch", E_IMAGE_IMPORT_STRETCH, 0, 0);
   RD(_("Center"), "center", E_IMAGE_IMPORT_CENTER, 1, 0);
   RD(_("Title"), "tile", E_IMAGE_IMPORT_TILE, 2, 0);
   RD(_("Within"), "scale_aspect_in", E_IMAGE_IMPORT_SCALE_ASPECT_IN, 3, 0);
   RD(_("Fill"), "scale_aspect_out", E_IMAGE_IMPORT_SCALE_ASPECT_OUT, 4, 0);
#undef RD

   e_widget_list_object_append(vlist, frame, 1, 1, 0.5);

   frame = e_widget_frametable_add(evas, _("File Quality"), 0);

   check = e_widget_check_add(evas, _("Use original file"), &ctxt->external);
   e_widget_frametable_object_append(frame, check, 0, 0, 1, 1, 1, 0, 1, 0);

   slider = e_widget_slider_add
     (evas, 1, 0, _("%3.0f%%"), 0.0, 100.0, 1.0, 0, NULL, &ctxt->quality, 150);
   e_widget_frametable_object_append(frame, slider, 0, 1, 1, 1, 1, 0, 1, 0);

   e_widget_list_object_append(vlist, frame, 1, 1, 0.5);

   e_widget_size_min_get(vlist, &w, &h);
   w += 50;
   e_dialog_content_set(ctxt->dia, vlist, w, h);

   e_win_centered_set(ctxt->dia->win, 1);

   return ctxt->dia;
}

/**
 * Request given image to be imported as an edje file.
 *
 * This is useful to convert images to icons and background.
 *
 * @param image_path path to source image to use.
 * @param edje_path path to destination edje to generate.
 * @param external if @c EINA_TRUE, then it will not embed image into edje,
 *        but reference the original @a image_path.
 * @param quality quality value from 0-100.
 * @param mode how to resize image with edje.
 * @param cb function to callback when process finishes.
 * @param data extra context to give to callback.
 *
 * @return handle so one can cancel the operation. This handle will be
 *         invalid after @a cb is called!
 */
EAPI E_Util_Image_Import_Handle *
e_util_image_import(const char *image_path, const char *edje_path, const char *edje_group, Eina_Bool external, int quality, E_Image_Import_Mode mode, void (*cb)(void *data, Eina_Bool ok, const char *image_path, const char *edje_path), const void *data)
{
   static const char *tmpdir = NULL;
   E_Util_Image_Import_Handle *handle;
   Ecore_Evas *ee;
   Evas_Object *img;
   int fd, w, h;
   const char *escaped_file;
   char cmd[PATH_MAX * 2], tmpn[PATH_MAX];
   FILE *f;

   if (!image_path) return NULL;
   if (!edje_path) return NULL;
   if (!edje_group) return NULL;
   if (!cb) return NULL;
   ee = ecore_evas_buffer_new(1, 1);
   img = evas_object_image_add(ecore_evas_get(ee));
   evas_object_image_file_set(img, image_path, NULL);
   if (evas_object_image_load_error_get(img) != EVAS_LOAD_ERROR_NONE)
     {
        ecore_evas_free(ee);
	printf("Error loading image '%s'\n", image_path);
	return NULL;
     }
   evas_object_image_size_get(img, &w, &h);
   ecore_evas_free(ee);

   if (!tmpdir)
     {
	tmpdir = getenv("TMPDIR");
	if (!tmpdir) tmpdir = "/tmp";
     }
   snprintf(tmpn, sizeof(tmpn), "%s/e_util_image_import-XXXXXX", tmpdir);
   fd = mkstemp(tmpn);
   if (fd < 0)
     {
	printf("Error Creating tmp file: %s\n", strerror(errno));
	return NULL;
     }

   f = fdopen(fd, "wb");
   if (!f)
     {
	printf("Cannot open %s for writing\n", tmpn);
	close(fd);
	return NULL;
     }

   escaped_file = e_util_filename_escape(image_path); // watch out ret buffer!

   fprintf(f, "images.image: \"%s\" ", escaped_file);
   if (external)
     fputs("USER", f);
   else if (quality >= 100)
     fputs("COMP", f);
   else
     fprintf(f, "LOSSY %d", (quality > 1) ? quality : 90);

   fprintf(f,
	   ";\n"
	   "collections {\n"
	   "   group {\n"
	   "      name: \"%s\";\n"
	   "      data.item: \"style\" \"%d\";\n"
	   "      max: %d %d;\n"
	   "      parts {\n",
	   edje_group, mode, w, h);

   if ((mode == E_IMAGE_IMPORT_CENTER) ||
       (mode == E_IMAGE_IMPORT_SCALE_ASPECT_IN))
     {
	fputs("         part {\n"
	      "            type: RECT;\n"
	      "            name: \"col\";\n"
	      "            mouse_events: 0;\n"
	      "            description {\n"
	      "               state: \"default\" 0.0;\n"
	      "               color: 255 255 255 255;\n"
	      "            }\n"
	      "         }\n",
	      f);
     }

   fprintf(f,
	   "         part {\n"
	   "            type: IMAGE;\n"
	   "            name: \"image\";\n"
	   "            mouse_events: 0;\n"
	   "            description {\n"
	   "               state: \"default\" 0.0;\n"
	   "               image {\n"
	   "                  normal: \"%s\";\n"
	   "                  scale_hint: STATIC;\n"
	   "               }\n",
	   escaped_file);

   if (mode == E_IMAGE_IMPORT_TILE)
     {
	fprintf(f,
		"               fill {\n"
		"                  size {\n"
		"                     relative: 0.0 0.0;\n"
		"                     offset: %d %d;\n"
		"                  }\n"
		"               }\n",
		w, h);
     }
   else if (mode == E_IMAGE_IMPORT_CENTER)
     {
	fprintf(f,
		"               min: %d %d;\n"
		"               max: %d %d;\n",
		w, h, w, h);

     }
   else if ((mode == E_IMAGE_IMPORT_SCALE_ASPECT_IN) ||
	    (mode == E_IMAGE_IMPORT_SCALE_ASPECT_OUT))
     {
	const char *locale = e_intl_language_get();
	double aspect = (double)w / (double)h;
	setlocale(LC_NUMERIC, "C");
	fprintf(f,
		"               aspect: %1.9f %1.9f;\n"
		"               aspect_preference: %s;\n",
		aspect, aspect,
		(mode == E_IMAGE_IMPORT_SCALE_ASPECT_IN) ? "BOTH" : "NONE");
	setlocale(LC_NUMERIC, locale);
     }

   fputs("            }\n" // description
	 "         }\n"    // part
	 "      }\n"       // parts
	 "   }\n"          // group
	 "}\n",            // collections
	 f);

   fclose(f); // fd gets closed here

   snprintf(cmd, sizeof(cmd), "edje_cc %s %s",
	    tmpn, e_util_filename_escape(edje_path));

   handle = E_NEW(E_Util_Image_Import_Handle, 1);
   if (!handle)
     {
	unlink(tmpn);
	return NULL;
     }

   handle->cb.func = cb;
   handle->cb.data = (void *)data;
   handle->path.image = eina_stringshare_add(image_path);
   handle->path.edje = eina_stringshare_add(edje_path);
   handle->path.temp = eina_stringshare_add(tmpn);
   handle->handler = ecore_event_handler_add
     (ECORE_EXE_EVENT_DEL, _e_util_image_import_exit, handle);
   handle->exe = ecore_exe_run(cmd, NULL);
   if (!handle->exe)
     {
	_e_util_image_import_handle_free(handle);
	return NULL;
     }

   return handle;
}

EAPI void
e_util_image_import_cancel(E_Util_Image_Import_Handle *handle)
{
   if (!handle) return;
   ecore_exe_kill(handle->exe);
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

static Eina_Bool
_e_util_wakeup_cb(void *data __UNUSED__)
{
   _e_util_dummy_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_util_image_import_settings_do(void *data, E_Dialog *dia)
{
   E_Util_Image_Import_Settings *ctxt = data;

   ctxt->ok = EINA_TRUE;
   e_util_defer_object_del(E_OBJECT(dia));
}

static void
_e_util_image_import_settings_del(void *obj)
{
   E_Dialog *dia = obj;
   E_Util_Image_Import_Settings *ctxt = dia->data;

   ctxt->cb.func(ctxt->cb.data, ctxt->path,
                 ctxt->ok, ctxt->external, ctxt->quality, ctxt->mode);

   eina_stringshare_del(ctxt->path);
   E_FREE(ctxt);
}

static Eina_Bool
_e_util_image_import_exit(void *data, int type __UNUSED__, void *event)
{
   E_Util_Image_Import_Handle *handle = data;
   Ecore_Exe_Event_Del *ev = event;
   Eina_Bool ok;

   if (ev->exe != handle->exe) return ECORE_CALLBACK_PASS_ON;

   ok = (ev->exit_code == 0);

   if (!ok) unlink(handle->path.edje);
   handle->cb.func(handle->cb.data, ok, handle->path.image, handle->path.edje);

   _e_util_image_import_handle_free(handle);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_util_image_import_handle_free(E_Util_Image_Import_Handle *handle)
{
   unlink(handle->path.temp);
   eina_stringshare_del(handle->path.image);
   eina_stringshare_del(handle->path.edje);
   eina_stringshare_del(handle->path.temp);
   if (handle->handler) ecore_event_handler_del(handle->handler);
   E_FREE(handle);
}

static Eina_Bool
_e_util_conf_timer_old(void *data)
{
   char *module_name = data;
   char buf[PATH_MAX];
   char *msg =
     _("Configuration data needed "
       "upgrading. Your old configuration<br> has been"
       " wiped and a new set of defaults initialized. "
       "This<br>will happen regularly during "
       "development, so don't report a<br>bug. "
       "This simply means the module needs "
       "new configuration<br>data by default for "
       "usable functionality that your old<br>"
       "configuration simply lacks. This new set of "
       "defaults will fix<br>that by adding it in. "
       "You can re-configure things now to your<br>"
       "liking. Sorry for the inconvenience.<br>");

   snprintf(buf, sizeof(buf),N_("%s Configuration Updated"), module_name);
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

   snprintf(buf, sizeof(buf),N_("%s Configuration Updated"), module_name);
   e_util_dialog_internal(buf, msg);
   E_FREE(module_name);

   return ECORE_CALLBACK_CANCEL;
}

EAPI Eina_Bool
e_util_module_config_check(const char *module_name, int conf, int epoch, int version)
{
   if ((conf >> 16) < epoch)
     {
	ecore_timer_add(1.0, _e_util_conf_timer_old, strdup(module_name));
	return EINA_FALSE;
     }
   else if (conf > version)
     {
	ecore_timer_add(1.0, _e_util_conf_timer_new, strdup(module_name));
	return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * Checks whenever the current manager/container/zone have fullscreen windows.
 */
EAPI Eina_Bool
e_util_fullscreen_curreny_any(void)
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

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI E_Path *path_data    = NULL;
EAPI E_Path *path_images  = NULL;
EAPI E_Path *path_fonts   = NULL;
EAPI E_Path *path_themes  = NULL;
EAPI E_Path *path_icons   = NULL;
EAPI E_Path *path_modules = NULL;
EAPI E_Path *path_backgrounds = NULL;
EAPI E_Path *path_messages = NULL;
EAPI int     restart      = 0;
EAPI int     good         = 0;
EAPI int     evil         = 0;
EAPI int     starting     = 1;
EAPI int     stopping     = 0;

typedef struct _E_Util_Fake_Mouse_Up_Info E_Util_Fake_Mouse_Up_Info;

struct _E_Util_Fake_Mouse_Up_Info
{
   Evas *evas;
   int   button;
};

/* local subsystem functions */
static int _e_util_cb_delayed_del(void *data);
static int _e_util_wakeup_cb(void *data);

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
   if (!str || !glob)
     return 0;
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

   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;
	E_Container *con;

	man = l->data;
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
   if (penv_display) penv_display = strdup(penv_display);
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
   char buf[4096];

   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", name);
   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
      return 1;
   return 0;
}

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

/* WARNING This function is deprecated, You should
 * use e_util_menu_item_theme_icon_set() instead.
 * It provide fallback (e theme <-> fdo theme) in both direction */
EAPI int
e_util_menu_item_edje_icon_set(E_Menu_Item *mi, const char *name)
{
   const char *file;
   char buf[4096];

   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", name);
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
   const unsigned int *itr, known_sizes[] = {
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
   char *path = NULL;
   unsigned int size;

   if ((!icon) || (!icon[0])) return 0;
   size = e_util_icon_size_normalize(16 * e_scale);
   path = efreet_icon_path_find(e_config->icon_theme, icon, size);
   if (!path) return 0;
   e_menu_item_icon_file_set(mi, path);
   E_FREE(path);
   return 1;
}

EAPI int
e_util_menu_item_theme_icon_set(E_Menu_Item *mi, const char *icon)
{
   if (e_config->icon_theme_overrides)
     {
	if (_e_util_menu_item_fdo_icon_set(mi, icon))
	  return 1;
	return e_util_menu_item_edje_icon_set(mi, icon);
     }
   else
     {
	if (e_util_menu_item_edje_icon_set(mi, icon))
	  return 1;
	return _e_util_menu_item_fdo_icon_set(mi, icon);
     }
}

EAPI E_Container *
e_util_container_window_find(Ecore_X_Window win)
{
   Eina_List *l, *ll;

   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
          {
	     E_Container *con;

	     con = ll->data;
	     if ((con->win == win) || (con->bg_win == win) ||
		 (con->event_win == win))
	       return con;
	  }
     }
   return NULL;
}

EAPI E_Border *
e_util_desk_border_above(E_Border *bd)
{
   E_Border *above = NULL;
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

   for (l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd);
	(l) && (l->next) && (!above);
	l = l->next)
     {
	above = l->next->data;
	if ((above->desk != bd->desk) && (!above->sticky))
	  above = NULL;
     }
   if (!above)
     {
	/* Need to check the layers above */
	for (i = pos + 1; (i < 7) && (!above); i++)
	  {
	     for (l = bd->zone->container->layers[i].clients;
		  (l) && (!above);
		  l = l->next)
	       {
		  above = l->data;
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
   E_Border *below = NULL;
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

   for (l = eina_list_data_find_list(bd->zone->container->layers[pos].clients, bd);
	(l) && (l->prev) && (!below);
	l = l->prev)
     {
	below = l->prev->data;
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
		  for (l = eina_list_last(bd->zone->container->layers[i].clients);
		       (l) && (!below);
		       l = l->prev)
		    {
		       below = l->data;
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

   clist = edje_file_collection_list(file);
   for (l = clist; l; l = l->next)
     {
	if (!strcmp(coll, l->data))
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
   e_dialog_icon_set(dia, "enlightenment/error", 64);
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
   static char buf[4096];

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
   Ecore_Evas  *ee;
   Evas        *evas;
   Evas_Object *im;
   int          ret;

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
e_util_shell_env_path_eval(char *path)
{
   /* evaluate things like:
    * $HOME/bling -> /home/user/bling
    * $HOME/bin/$HOSTNAME/blah -> /home/user/bin/localhost/blah
    * etc. etc.
    */
   char buf[4096], *pd, *p, *v2, *s, *vp;
   char *v = NULL;
   char *v1 = NULL;
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
     {
	snprintf(buf, sizeof(buf), _("In the Future"));
     }
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
   if (!prev_ld_library_path) return;
   e_util_env_set("LD_LIBRARY_PATH", prev_ld_library_path);
   E_FREE(prev_ld_library_path);
   if (!prev_path) return;
   e_util_env_set("PATH", prev_path);
   E_FREE(prev_path);
}

EAPI Evas_Object *
e_util_icon_add(const char *path, Evas *evas)
{
   Evas_Object *o = NULL;
   const char *ext;

   if (!path) return NULL;
   if (!ecore_file_exists(path)) return NULL;

   o = e_icon_add(evas);
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
   if (icon_name[0] == '/') return e_util_icon_add(icon_name, evas);
   else
     {
	Evas_Object *obj;
	char *path;

	path = efreet_icon_path_find(e_config->icon_theme, icon_name, size);
	if (path)
	  {
	     obj = e_util_icon_add(path, evas);
	     free(path);
	     return obj;
	  }
     }
   return NULL;
}

EAPI void
e_util_desktop_menu_item_icon_add(Efreet_Desktop *desktop, unsigned int size, E_Menu_Item *mi)
{
   char *path = NULL;

   if ((!desktop) || (!desktop->icon)) return;

   if (desktop->icon[0] == '/') path = strdup(desktop->icon);
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
	free(path);
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
        
        w = zone->w / 3;
        h = zone->h / 3;
        if (w < win->min_w) w = win->min_w;
        if (h < win->min_h) h = win->min_h;
        e_win_resize(win, w, h);
     }
}

/* local subsystem functions */
static int
_e_util_cb_delayed_del(void *data)
{
   e_object_del(E_OBJECT(data));
   return 0;
}

static int
_e_util_wakeup_cb(void *data)
{
   _e_util_dummy_timer = NULL;
   return 0;
}



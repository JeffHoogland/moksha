/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Util_Fake_Mouse_Up_Info E_Util_Fake_Mouse_Up_Info;

struct _E_Util_Fake_Mouse_Up_Info
{
   E_Container *con;
   int          button;
};

/* local subsystem functions */
static void _e_util_container_fake_mouse_up_cb(void *data);
static int _e_util_wakeup_cb(void *data);

/* local subsystem globals */
static Ecore_Timer *_e_util_dummy_timer = NULL;

/* externally accessible functions */
void
e_util_container_fake_mouse_up_later(E_Container *con, int button)
{
   E_Util_Fake_Mouse_Up_Info *info;
   
   info = calloc(1, sizeof(E_Util_Fake_Mouse_Up_Info));
   if (info)
     {
	info->con = con;
	info->button = button;
	e_object_ref(E_OBJECT(info->con));
	ecore_job_add(_e_util_container_fake_mouse_up_cb, info);
     }
}

void
e_util_container_fake_mouse_up_all_later(E_Container *con)
{
   e_util_container_fake_mouse_up_later(con, 1);
   e_util_container_fake_mouse_up_later(con, 2);
   e_util_container_fake_mouse_up_later(con, 3);
}

void
e_util_wakeup(void)
{
   if (_e_util_dummy_timer) return;
   _e_util_dummy_timer = ecore_timer_add(0.0, _e_util_wakeup_cb, NULL);
}

void
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

E_Zone *
e_util_zone_current_get(E_Manager *man)
{
   E_Container *con;
   
   con = e_container_current_get(man);
   if (con)
     {
	E_Zone *zone;
	
	zone = e_zone_current_get(con);
	return zone;
     }
   return NULL;
}

/* local subsystem functions */
static void
_e_util_container_fake_mouse_up_cb(void *data)
{
   E_Util_Fake_Mouse_Up_Info *info;
   
   info = data;
   if (info)
     {
	evas_event_feed_mouse_up(info->con->bg_evas, info->button, EVAS_BUTTON_NONE, ecore_x_current_time_get(), NULL);
	e_object_unref(E_OBJECT(info->con));
	free(info);
     }
}

static int
_e_util_wakeup_cb(void *data)
{
   _e_util_dummy_timer = NULL;
   return 0;
}

int
e_util_utils_installed(void)
{
   return ecore_file_app_installed("emblem");
}

int
e_util_glob_match(char *str, char *glob)
{
   if (glob[0] == 0)
     {
	if (str[0] == 0) return 1;
	return 0;
     }
   if (!strcmp(glob, "*")) return 1;
   if (!fnmatch(glob, str, 0)) return 1;
   return 0;
}

E_Container *
e_util_container_number_get(int num)
{
   Evas_List *l;
   
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man;
	E_Container *con;
	
	man = l->data;
	con = e_manager_container_number_get(man, num);
	if (con) return con;
     }
   return NULL;
}

E_Zone *
e_util_container_zone_number_get(int con_num, int zone_num)
{
   E_Container *con;
   
   con = e_util_container_number_get(con_num);
   if (!con) return NULL;
   return e_container_zone_number_get(con, zone_num);
}

int
e_util_head_exec(int head, char *cmd)
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
	/* yes it could overflow... but who will voerflow DISPLAY eh? why? to
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
	e_error_dialog_show(_("Run Error"),
			    _("Enlightenment was unable fork a child process:\n"
			      "\n"
			      "%s\n"
			      "\n"),
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

int
e_util_strcmp(char *s1, char *s2)
{
   if ((s1) && (s2))
     return strcmp(s1, s2);
   return 0x7fffffff;
}

int
e_util_both_str_empty(char *s1, char *s2)
{
   int empty = 0;
   
   if ((!s1) && (!s2)) return 1;
   if ((!s1) || ((s1) && (s1[0] == 0))) empty++;
   if ((!s2) || ((s2) && (s2[0] == 0))) empty++;
   if (empty == 2) return 1;
   return 0;
}

int
e_util_immortal_check(void)
{
   Evas_List *wins;
   
   wins = e_border_immortal_windows_get();
   if (wins)
     {
	e_error_dialog_show(_("Cannot exit - immortal windows."),
			    _("Some windows are left still around with the Lifespan lock enabled. This means\n"
			      "that Enlightenment will not allow itself to exit until these windows have\n"
			      "been closed or have the lifespan lock removed.\n"));
	/* FIXME: should really display a list of these lifespan locked */
	/* windows in a dialog and let the user disable their locks in */
	/* this dialog */
	evas_list_free(wins);
	return 1;
     }
   return 0;
}

int
e_util_ejde_icon_list_set(Evas_Object *obj, char *list)
{
   char *buf;
   char *p, *c;
   
   buf = malloc(strlen(list) + 1);
   p = list;
   while (p)
     {
	c = strchr(p, ',');
	if (c)
	  {
	     strncpy(buf, p, c - p);
	     buf[c - p] = 0;
	     if (e_util_edje_icon_set(obj, buf))
	       {
		  free(buf);
		  return 1;
	       }
	     p = c + 1;
	     if (!*p)
	       {
		  free(buf);
		  return 0;
	       }
	  }
	else
	  {
	     strcpy(buf, p);
	     if (e_util_edje_icon_set(obj, buf))
	       {
		  free(buf);
		  return 1;
	       }
	  }
     }
   free(buf);
   return 0;
}

int
e_util_menu_item_ejde_icon_list_set(E_Menu_Item *mi, char *list)
{
   char *buf;
   char *p, *c;
   
   buf = malloc(strlen(list) + 1);
   p = list;
   while (p)
     {
	c = strchr(p, ',');
	if (c)
	  {
	     strncpy(buf, p, c - p);
	     buf[c - p] = 0;
	     if (e_util_menu_item_edje_icon_set(mi, buf))
	       {
		  free(buf);
		  return 1;
	       }
	     p = c + 1;
	     if (!*p)
	       {
		  free(buf);
		  return 0;
	       }
	  }
	else
	  {
	     strcpy(buf, p);
	     if (e_util_menu_item_edje_icon_set(mi, buf))
	       {
		  free(buf);
		  return 1;
	       }
	  }
     }
   free(buf);
   return 0;
}

int
e_util_edje_icon_set(Evas_Object *obj, char *name)
{
   char *file;
   char buf[4096];

   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "icons/%s", name);
   file = (char *)e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
     {
	edje_object_file_set(obj, file, buf);
	return 1;
     }
   return 0;
}

int
e_util_menu_item_edje_icon_set(E_Menu_Item *mi, char *name)
{
   char *file;
   char buf[4096];
   
   if ((!name) || (!name[0])) return 0;
   snprintf(buf, sizeof(buf), "icons/%s", name);
   file = (char *)e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
     {
	e_menu_item_icon_edje_set(mi, file, buf);
	return 1;
     }
   return 0;
}

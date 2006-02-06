/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* currently default bind is alt+` buf alt+space has been suggested */

/* local subsystem functions */
typedef struct _E_Exebuf_Exe E_Exebuf_Exe;
   
struct _E_Exebuf_Exe
{
   Evas_Object *bg_object;
   Evas_Object *icon_object;
   E_App       *app;
   char        *file;
};

static void _e_exebuf_exe_free(E_Exebuf_Exe *exe);
static void _e_exebuf_matches_clear(void);
static int _e_exebuf_cb_sort_eap(void *data1, void *data2);
static int _e_exebuf_cb_sort_exe(void *data1, void *data2);
static void _e_exebuf_update(void);
static void _e_exebuf_exec(void);
static void _e_exebuf_exe_sel(E_Exebuf_Exe *exe);
static void _e_exebuf_exe_desel(E_Exebuf_Exe *exe);
static void _e_exebuf_exe_scroll_to(int i);
static void _e_exebuf_eap_scroll_to(int i);
static void _e_exebuf_next(void);
static void _e_exebuf_prev(void);
static void _e_exebuf_complete(void);
static void _e_exebuf_backspace(void);
static void _e_exebuf_matches_update(void);
static int _e_exebuf_cb_key_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_up(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_wheel(void *data, int type, void *event);
static int _e_exebuf_exe_scroll_timer(void *data);
static int _e_exebuf_eap_scroll_timer(void *data);
static int _e_exebuf_animator(void *data);
static int _e_exebuf_idler(void *data);

/* local subsystem globals */
static E_Popup *exebuf = NULL;
static Evas_Object *bg_object = NULL;
static Evas_Object *icon_object = NULL;
static Evas_Object *exe_list_object = NULL;
static Evas_Object *eap_list_object = NULL;
static Evas_List *handlers = NULL;
static Ecore_X_Window input_window = 0;
static char *cmd_buf = NULL;
static Evas_List *eap_matches = NULL;
static Evas_List *exe_matches = NULL;
static Evas_List *exe_path = NULL;
static DIR       *exe_dir = NULL;
static Evas_List *exe_list = NULL;
static Ecore_Idler *exe_list_idler = NULL;
static Evas_List *exes = NULL;
static Evas_List *eaps = NULL;
#define NO_LIST 0
#define EAP_LIST 1
#define EXE_LIST 2
static int which_list = NO_LIST;
static E_Exebuf_Exe *exe_sel = NULL;
static int exe_scroll_to = 0;
static double exe_scroll_align_to = 0.0;
static double exe_scroll_align = 0.0;
static Ecore_Timer *exe_scroll_timer = NULL;
static int eap_scroll_to = 0;
static double eap_scroll_align_to = 0.0;
static double eap_scroll_align = 0.0;
static Ecore_Timer *eap_scroll_timer = NULL;
static Ecore_Timer *animator = NULL;
 
#define EXEBUFLEN 2048

/* externally accessible functions */
EAPI int
e_exebuf_init(void)
{
   return 1;
}

EAPI int
e_exebuf_shutdown(void)
{
   e_exebuf_hide();
   return 1;
}

EAPI int
e_exebuf_show(E_Zone *zone)
{
   Evas_Object *o;
   int x, y, w, h;
   Evas_Coord mw, mh;
   char *path, *p, *last;
   
   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   if (e_winlist_active_get()) return 0;
   if (exebuf) return 0;

   input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
   ecore_x_window_show(input_window);
   e_grabinput_get(input_window, 0, input_window);

   x = zone->x + 20;
   y = zone->y + 20 + ((zone->h - 20 - 20 - 20) / 2);
   w = zone->w - 20 - 20;
   h = 20;
   
   exebuf = e_popup_new(zone, x, y, w, h); 
   if (!exebuf) return 0;
   
   cmd_buf = malloc(EXEBUFLEN);
   if (!cmd_buf)
     {
	e_object_del(E_OBJECT(exebuf));
	return 0;
     }
   cmd_buf[0] = 0;
   
   e_popup_layer_set(exebuf, 255);
   evas_event_freeze(exebuf->evas);
   o = edje_object_add(exebuf->evas);
   bg_object = o;
   e_theme_edje_object_set(o, "base/theme/exebuf",
			   "widgets/exebuf/main");
   edje_object_part_text_set(o, "label", cmd_buf);
   
   o = e_box_add(exebuf->evas);
   exe_list_object = o;
   e_box_align_set(o, 0.5, 1.0);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(bg_object, "exe_list_swallow", o);
   evas_object_show(o);
   
   o = e_box_add(exebuf->evas);
   eap_list_object = o;
   e_box_align_set(o, 0.5, 0.0);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(bg_object, "eap_list_swallow", o);
   evas_object_show(o);
   
   o = bg_object;
   edje_object_size_min_calc(o, &mw, &mh);
   
   w = (double)zone->w * e_config->exebuf_pos_size_w;
   if (w > e_config->exebuf_pos_max_w) w = e_config->exebuf_pos_max_w;
   else if (w < e_config->exebuf_pos_min_w) w = e_config->exebuf_pos_min_w;
   if (w < mw) w = mw;
   if (w > zone->w) w = zone->w;
   x = (double)(zone->w - w) * e_config->exebuf_pos_align_x;
   
   h = (double)zone->h * e_config->exebuf_pos_size_h;
   if (h > e_config->exebuf_pos_max_h) h = e_config->exebuf_pos_max_h;
   else if (h < e_config->exebuf_pos_min_h) h = e_config->exebuf_pos_min_h;
   if (h < mh) h = mh;
   if (h > zone->h) h = zone->h;
   y = (double)(zone->h - h) * e_config->exebuf_pos_align_y;
   
   e_popup_move_resize(exebuf, x, y, w, h);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(exebuf, o);

   evas_event_thaw(exebuf->evas);

   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_KEY_DOWN, _e_exebuf_cb_key_down, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_DOWN, _e_exebuf_cb_mouse_down, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_BUTTON_UP, _e_exebuf_cb_mouse_up, NULL));
   handlers = evas_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_MOUSE_WHEEL, _e_exebuf_cb_mouse_wheel, NULL));

   path = getenv("PATH");
   if (path)
     {
	path = strdup(path);
	last = path;
	for (p = path; p[0]; p++)
	  {
	     if (p[0] == ':') p[0] = '\0';
	     if (p[0] == 0)
	       {
		  exe_path = evas_list_append(exe_path, strdup(last));
		  last = p + 1;
	       }
	  }
	if (p > last)
	  exe_path = evas_list_append(exe_path, strdup(last));
     }
   exe_list_idler = ecore_idler_add(_e_exebuf_idler, NULL);
   
   e_popup_show(exebuf);
   return 1;
}

EAPI void
e_exebuf_hide(void)
{
   if (!exebuf) return;
   
   evas_event_freeze(exebuf->evas);
   _e_exebuf_matches_clear();
   e_popup_hide(exebuf);
   if (exe_scroll_timer) ecore_timer_del(exe_scroll_timer);
   exe_scroll_timer = NULL;
   if (eap_scroll_timer) ecore_timer_del(eap_scroll_timer);
   eap_scroll_timer = NULL;
   if (animator) ecore_animator_del(animator);
   animator = NULL;
   exe_scroll_to = 0;
   exe_scroll_align_to = 0.0;
   exe_scroll_align = 0.0;
   eap_scroll_to = 0;
   eap_scroll_align_to = 0.0;
   eap_scroll_align = 0.0;
   evas_object_del(eap_list_object);
   eap_list_object = NULL;
   evas_object_del(exe_list_object);
   exe_list_object = NULL;
   evas_object_del(bg_object);
   bg_object = NULL;
   if (icon_object) evas_object_del(icon_object);
   icon_object = NULL;
   evas_event_thaw(exebuf->evas);
   e_object_del(E_OBJECT(exebuf));
   exebuf = NULL;
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = evas_list_remove_list(handlers, handlers);
     }
   ecore_x_window_del(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;
   free(cmd_buf);
   cmd_buf = NULL;
   if (exe_dir)
     {
	closedir(exe_dir);
	exe_dir = NULL;
     }
   while (exe_path)
     {
	free(exe_path->data);
	exe_path = evas_list_remove_list(exe_path, exe_path);
     }
   if (exe_list_idler)
     {
	ecore_idler_del(exe_list_idler);
	exe_list_idler = NULL;
     }
   while (exe_list)
     {
	free(exe_list->data);
	exe_list = evas_list_remove_list(exe_list, exe_list);
     }
   which_list = NO_LIST;
   exe_sel = NULL;
}

/* local subsystem functions */

static void
_e_exebuf_exe_free(E_Exebuf_Exe *exe)
{
   if (exe->app) e_object_unref(E_OBJECT(exe->app));
   evas_object_del(exe->bg_object);
   if (exe->icon_object) evas_object_del(exe->icon_object);
   free(exe);
}

static void
_e_exebuf_matches_clear(void)
{
   while (eap_matches)
     {
	e_object_unref(E_OBJECT(eap_matches->data));
	eap_matches = evas_list_remove_list(eap_matches, eap_matches);
     }
   while (exe_matches)
     {
	free(exe_matches->data);
	exe_matches = evas_list_remove_list(exe_matches, exe_matches);
     }
   
   e_box_freeze(eap_list_object);
   e_box_freeze(exe_list_object);
   while (exes)
     {
	_e_exebuf_exe_free((E_Exebuf_Exe *)(exes->data));
	exes = evas_list_remove_list(exes, exes);
     }
   while (eaps)
     {
	_e_exebuf_exe_free((E_Exebuf_Exe *)(eaps->data));
	eaps = evas_list_remove_list(eaps, eaps);
     }
   e_box_thaw(exe_list_object);
   e_box_thaw(eap_list_object);
   
   e_box_align_set(eap_list_object, 0.5, 0.0);
   e_box_align_set(exe_list_object, 0.5, 1.0);
   exe_sel = NULL;
   which_list = NO_LIST;
}

static void
_e_exebuf_update(void)
{
   E_App *a;
   Evas_Object *o;
   
   edje_object_part_text_set(bg_object, "label", cmd_buf);
   if (icon_object) evas_object_del(icon_object);
   icon_object = NULL;
   a = e_app_exe_find(cmd_buf);
   if (!a) a = e_app_name_find(cmd_buf);
   if (!a) a = e_app_generic_find(cmd_buf);
   if (a)
     {
	o = e_app_icon_add(exebuf->evas, a);
	icon_object = o;
	edje_object_part_swallow(bg_object, "icon_swallow", o);
	evas_object_show(o);
     }
}

static void
_e_exebuf_exec(void)
{
   if (exe_sel)
     {
	if (exe_sel->app)
	  {
	     e_zone_app_exec(exebuf->zone, exe_sel->app);
	     e_exehist_add("exebuf", exe_sel->app->exe);
	  }
	else
	  {
	     e_zone_exec(exebuf->zone, exe_sel->file);
	     e_exehist_add("exebuf", exe_sel->file);
	  }
     }
   else
     {
	e_zone_exec(exebuf->zone, cmd_buf);
	e_exehist_add("exebuf", cmd_buf);
     }
   
   e_exebuf_hide();
}

static void
_e_exebuf_exe_sel(E_Exebuf_Exe *exe)
{
   edje_object_signal_emit(exe->bg_object, "active", "");
   if (exe->icon_object)
     edje_object_signal_emit(exe->icon_object, "active", "");
}

static void
_e_exebuf_exe_desel(E_Exebuf_Exe *exe)
{
   edje_object_signal_emit(exe->bg_object, "passive", "");
   if (exe->icon_object)
     edje_object_signal_emit(exe->icon_object, "passive", "");
}

static void
_e_exebuf_exe_scroll_to(int i)
{
   int n;
   
   n = evas_list_count(exes);
   if (n > 1)
     {
	exe_scroll_align_to = (double)i / (double)(n - 1);
	if (e_config->exebuf_scroll_animate)
	  {	
	     exe_scroll_to = 1;
	     if (!exe_scroll_timer)
	       exe_scroll_timer = ecore_timer_add(0.01, _e_exebuf_exe_scroll_timer, NULL);
	     if (!animator)
	       animator = ecore_animator_add(_e_exebuf_animator, NULL);
	  }
	else
	  {
	     exe_scroll_align = exe_scroll_align_to;
	     e_box_align_set(exe_list_object, 0.5, 1.0 - exe_scroll_align);
	  }
     }
   else
     e_box_align_set(exe_list_object, 0.5, 1.0);
}

static void
_e_exebuf_eap_scroll_to(int i)
{
   int n;
   
   n = evas_list_count(eaps);
   if (n > 1)
     {
	eap_scroll_align_to = (double)i / (double)(n - 1);
	if (e_config->exebuf_scroll_animate)
	  {	
	     eap_scroll_to = 1;
	     if (!eap_scroll_timer)
	       eap_scroll_timer = ecore_timer_add(0.01, _e_exebuf_eap_scroll_timer, NULL);
	     if (!animator)
	       animator = ecore_animator_add(_e_exebuf_animator, NULL);
	  }
	else
	  {
	     eap_scroll_align = eap_scroll_align_to;
	     e_box_align_set(eap_list_object, 0.5, eap_scroll_align);
	  }
     }
   else
     e_box_align_set(eap_list_object, 0.5, 0.0);
}

static void
_e_exebuf_next(void)
{
   Evas_List *l;
   int i;
   
   if (which_list == NO_LIST)
     {
	if (exes)
	  {
	     exe_sel = exes->data;
	     which_list = EXE_LIST;
	     if (exe_sel)
	       {
		  _e_exebuf_exe_sel(exe_sel);
		  _e_exebuf_exe_scroll_to(0);
	       }
	  }
     }
   else
     {
	if (which_list == EXE_LIST)
	  {
	     if (exe_sel)
	       {
		  for (i = 0, l = exes; l; l = l->next, i++)
		    {
		       if (l->data == exe_sel)
			 {
			    if (l->next)
			      {
				 _e_exebuf_exe_desel(exe_sel);
				 exe_sel = l->next->data;
				 _e_exebuf_exe_sel(exe_sel);
				 _e_exebuf_exe_scroll_to(i + 1);
			      }
			    break;
			 }
		    }
	       }
	  }
	else if (which_list == EAP_LIST)
	  {
	     if (exe_sel)
	       {
		  for (i = 0, l = eaps; l; l = l->next, i++)
		    {
		       if (l->data == exe_sel)
			 {
			    _e_exebuf_exe_desel(exe_sel);
			    if (l->prev)
			      {
				 exe_sel = l->prev->data;
				 _e_exebuf_exe_sel(exe_sel);
				 _e_exebuf_eap_scroll_to(i - 1);
			      }
			    else
			      {
				 exe_sel = NULL;
				 which_list = NO_LIST;
			      }
			    break;
			 }
		    }
	       }
	  }
     }
}

static void
_e_exebuf_prev(void)
{
   Evas_List *l;
   int i;

   if (which_list == NO_LIST)
     {
	if (eaps)
	  {
	     exe_sel = eaps->data;
	     which_list = EAP_LIST;
	     if (exe_sel)
	       {
		  _e_exebuf_exe_sel(exe_sel);
		  _e_exebuf_eap_scroll_to(0);
	       }
	  }
     }
   else
     {
	if (which_list == EXE_LIST)
	  {
	     if (exe_sel)
	       {
		  for (i = 0, l = exes; l; l = l->next, i++)
		    {
		       if (l->data == exe_sel)
			 {
			    _e_exebuf_exe_desel(exe_sel);
			    if (l->prev)
			      {
				 exe_sel = l->prev->data;
				 _e_exebuf_exe_sel(exe_sel);
				 _e_exebuf_exe_scroll_to(i - 1);
			      }
			    else
			      {
				 exe_sel = NULL;
				 which_list = NO_LIST;
			      }
			    break;
			 }
		    }
	       }
	  }
	else if (which_list == EAP_LIST)
	  {
	     if (exe_sel)
	       {
		  for (i = 0, l = eaps; l; l = l->next, i++)
		    {
		       if (l->data == exe_sel)
			 {
			    if (l->next)
			      {
				 _e_exebuf_exe_desel(exe_sel);
				 exe_sel = l->next->data;
				 _e_exebuf_exe_sel(exe_sel);
				 _e_exebuf_eap_scroll_to(i + 1);
			      }
			    break;
			 }
		    }
	       }
	  }
     }
}

static void
_e_exebuf_complete(void)
{
   char common[EXEBUFLEN], *exe = NULL;
   Evas_List *l;
   int orig_len = 0, common_len = 0, exe_len, next_char, val, pos, matches;
   
   if (exe_sel)
     {
	if (exe_sel->app)
	  {
	     strncpy(cmd_buf, exe_sel->app->name, EXEBUFLEN - 1);
	     cmd_buf[EXEBUFLEN - 1] = 0;
	  }
	else if (exe_sel->file)
	  {
	     strncpy(cmd_buf, exe_sel->file, EXEBUFLEN - 1);
	     cmd_buf[EXEBUFLEN - 1] = 0;
	  }
     }
   else
     {
	strcpy(common, cmd_buf);
	orig_len = common_len = strlen(common);
	matches = 1;
	while (matches)
	  {
	     next_char = 0;
	     matches = 0;
	     for (l = exe_matches; l; l = l->next)
	       {
		  matches = 1;
		  exe = l->data;
		  exe_len = strlen(exe);
		  if (exe_len > common_len)
		    {
		       val = 0;
		       pos = evas_string_char_next_get(exe, common_len, &val);
		       if (!next_char)
			 next_char = val;
		       else if (next_char != val)
			 {
			    matches = 0;
			    break;
			 }
		    }
		  else
		    {
		       matches = 0;
		       break;
		    }
	       }
	     if (matches) common_len++;
	  }
     }
   if ((exe) && (orig_len < common_len) && (common_len < (EXEBUFLEN - 1)))
     {
	strncpy(cmd_buf, exe, common_len);
	cmd_buf[common_len] = 0;
     }
   _e_exebuf_update();
   _e_exebuf_matches_update();
}

static void
_e_exebuf_backspace(void)
{
   int len, val, pos;
   
   len = strlen(cmd_buf);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(cmd_buf, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     cmd_buf[pos] = 0;
	     _e_exebuf_update();
	     _e_exebuf_matches_update();
	  }
     }
}

static int
_e_exebuf_cb_sort_eap(void *data1, void *data2)
{
   E_App *a1, *a2;
   double t1, t2;
   
   a1 = data1;
   a2 = data2;
   t1 = e_exehist_newest_run_get(a1->exe);
   t2 = e_exehist_newest_run_get(a2->exe);
   return (int)(t2 - t1);
}

static int
_e_exebuf_cb_sort_exe(void *data1, void *data2)
{
   char *e1, *e2;
   double t1, t2;
   
   e1 = data1;
   e2 = data2;
   t1 = e_exehist_newest_run_get(e1);
   t2 = e_exehist_newest_run_get(e2);
   return (int)(t2 - t1);
}

static void
_e_exebuf_matches_update(void)
{
   char *path, *file, buf[4096];
   Evas_Hash *added = NULL;
   Evas_List *l, *list;
   int i, max;
   
   _e_exebuf_matches_clear();
   if (strlen(cmd_buf) == 0) return;
   
   snprintf(buf, sizeof(buf), "*%s*", cmd_buf);
   list = e_app_name_glob_list(buf);
   for (l = list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (!evas_hash_find(added, a->exe))
	       {
		  e_object_ref(E_OBJECT(a));
		  eap_matches = evas_list_append(eap_matches, a);
		  added = evas_hash_direct_add(added, a->exe, a->exe);
	       }
	  }
     }
   evas_list_free(list);
   snprintf(buf, sizeof(buf), "%s*", cmd_buf);
   list = e_app_exe_glob_list(buf);
   for (l = list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (!evas_hash_find(added, a->exe))
	       {
		  e_object_ref(E_OBJECT(a));
		  eap_matches = evas_list_append(eap_matches, a);
		  added = evas_hash_direct_add(added, a->exe, a->exe);
	       }
	  }
     }
   evas_list_free(list);
   
   snprintf(buf, sizeof(buf), "*%s*", cmd_buf);
   list = e_app_generic_glob_list(buf);
   for (l = list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (!evas_hash_find(added, a->exe))
	       {
		  e_object_ref(E_OBJECT(a));
		  eap_matches = evas_list_append(eap_matches, a);
		  added = evas_hash_direct_add(added, a->exe, a->exe);
	       }
	  }
     }
   evas_list_free(list);
   
   snprintf(buf, sizeof(buf), "*%s*", cmd_buf);
   list = e_app_comment_glob_list(buf);
   for (l = list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (!evas_hash_find(added, a->exe))
	       {
		  e_object_ref(E_OBJECT(a));
		  eap_matches = evas_list_append(eap_matches, a);
		  added = evas_hash_direct_add(added, a->exe, a->exe);
	       }
	  }
     }
   evas_list_free(list);

   if (added) evas_hash_free(added);
   added = NULL;
   
   snprintf(buf, sizeof(buf), "%s*", cmd_buf);
   if (exe_list)
     {
	Evas_List *l;

	for (l = exe_list; l; l = l->next)
	  {
	     path = l->data;
	     file = (char *)ecore_file_get_file(path);
	     if (file)
	       {
		  if (e_util_glob_match(file, buf))
		    {
		       if (!evas_hash_find(added, file))
			 {
			    exe_matches = evas_list_append(exe_matches, strdup(file));
			    added = evas_hash_direct_add(added, file, file);
			 }
		    }
	       }
	  }
     }
   if (added) evas_hash_free(added);
   added = NULL;

   /* FIXME: sort eap matches with most recently selected matches at the
    * start and then from shortest to longest string
    */
   eap_matches = evas_list_sort(eap_matches, evas_list_count(eap_matches), _e_exebuf_cb_sort_eap);
   
   max = e_config->exebuf_max_eap_list;
   e_box_freeze(eap_list_object);
   for (i = 0, l = eap_matches; l && (i < max); l = l->next, i++)
     {
	E_Exebuf_Exe *exe;
	Evas_Coord mw, mh;
	Evas_Object *o;
	int opt = 0;
	
	exe = calloc(1, sizeof(E_Exebuf_Exe));
        eaps = evas_list_append(eaps, exe);
	exe->app = l->data;
	e_object_ref(E_OBJECT(exe->app));
	o = edje_object_add(exebuf->evas);
        exe->bg_object = o;
	e_theme_edje_object_set(o, "base/theme/exebuf",
				"widgets/exebuf/item");
	if (e_config->menu_eap_name_show && exe->app->name) opt |= 0x4;
	if (e_config->menu_eap_generic_show && exe->app->generic) opt |= 0x2;
	if (e_config->menu_eap_comment_show && exe->app->comment) opt |= 0x1;
	if      (opt == 0x7) snprintf(buf, sizeof(buf), "%s (%s) [%s]", exe->app->name, exe->app->generic, exe->app->comment);
	else if (opt == 0x6) snprintf(buf, sizeof(buf), "%s (%s)", exe->app->name, exe->app->generic);
	else if (opt == 0x5) snprintf(buf, sizeof(buf), "%s [%s]", exe->app->name, exe->app->comment);
	else if (opt == 0x4) snprintf(buf, sizeof(buf), "%s", exe->app->name);
	else if (opt == 0x3) snprintf(buf, sizeof(buf), "%s [%s]", exe->app->generic, exe->app->comment);
	else if (opt == 0x2) snprintf(buf, sizeof(buf), "%s", exe->app->generic);
	else if (opt == 0x1) snprintf(buf, sizeof(buf), "%s", exe->app->comment);
	else snprintf(buf, sizeof(buf), "%s", exe->app->name);
	edje_object_part_text_set(o, "title_text", buf);
	evas_object_show(o);
	if (edje_object_part_exists(exe->bg_object, "icon_swallow"))
	  {
	     o = e_app_icon_add(exebuf->evas, exe->app);
	     exe->icon_object = o;
	     edje_object_part_swallow(exe->bg_object, "icon_swallow", o);
	     evas_object_show(o);
	  }
	edje_object_size_min_calc(exe->bg_object, &mw, &mh);
	e_box_pack_start(eap_list_object, exe->bg_object);
	e_box_pack_options_set(exe->bg_object,
			       1, 1, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       mw, mh, /* min */
			       9999, mh /* max */
			       );
     }
   e_box_thaw(eap_list_object);
   
   /* FIXME: sort exe matches with most recently selected matches at the
    * start and then from shortest to longest string
    */
   exe_matches = evas_list_sort(exe_matches, evas_list_count(exe_matches), _e_exebuf_cb_sort_exe);
   
   max = e_config->exebuf_max_exe_list;
   e_box_freeze(exe_list_object);
   for (i = 0, l = exe_matches; l && (i < max); l = l->next, i++)
     {
	E_Exebuf_Exe *exe;
	Evas_Coord mw, mh;
	Evas_Object *o;
	
	exe = calloc(1, sizeof(E_Exebuf_Exe));
	exe->file = l->data;
        exes = evas_list_append(exes, exe);
	o = edje_object_add(exebuf->evas);
        exe->bg_object = o;
	e_theme_edje_object_set(o, "base/theme/exebuf",
				"widgets/exebuf/item");
	edje_object_part_text_set(o, "title_text", exe->file);
	evas_object_show(o);
	if (edje_object_part_exists(exe->bg_object, "icon_swallow"))
	  {
	     E_App *a;
	     
	     a = e_app_exe_find(exe->file);
	     if (a)
	       {
		  o = e_app_icon_add(exebuf->evas, a);
		  exe->icon_object = o;
		  edje_object_part_swallow(exe->bg_object, "icon_swallow", o);
		  evas_object_show(o);
		  exe->app = a;
		  e_object_ref(E_OBJECT(exe->app));
	       }
	  }
	edje_object_size_min_calc(exe->bg_object, &mw, &mh);
	e_box_pack_end(exe_list_object, exe->bg_object);
	e_box_pack_options_set(exe->bg_object,
			       1, 1, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       mw, mh, /* min */
			       9999, mh /* max */
			       );
     }
   e_box_thaw(exe_list_object);
}

static int
_e_exebuf_cb_key_down(void *data, int type, void *event)
{
   Ecore_X_Event_Key_Down *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   if      (!strcmp(ev->keysymbol, "Up"))
     _e_exebuf_prev();
   else if (!strcmp(ev->keysymbol, "Down"))
     _e_exebuf_next();
   else if (!strcmp(ev->keysymbol, "Prior"))
     _e_exebuf_prev();
   else if (!strcmp(ev->keysymbol, "Next"))
     _e_exebuf_next();
   else if (!strcmp(ev->keysymbol, "Left"))
     _e_exebuf_prev();
   else if (!strcmp(ev->keysymbol, "Right"))
     _e_exebuf_complete();
   else if (!strcmp(ev->keysymbol, "Tab"))
     _e_exebuf_complete();
   else if (!strcmp(ev->keysymbol, "Return"))
     _e_exebuf_exec();
   else if (!strcmp(ev->keysymbol, "KP_Enter"))
     _e_exebuf_exec();
   else if (!strcmp(ev->keysymbol, "Escape"))
     e_exebuf_hide();
   else if (!strcmp(ev->keysymbol, "BackSpace"))
     _e_exebuf_backspace();
   else if (!strcmp(ev->keysymbol, "Delete"))
     _e_exebuf_backspace();
   else
     {
	if (ev->key_compose)
	  {
	     if ((strlen(cmd_buf) < (EXEBUFLEN - strlen(ev->key_compose))))
	       {
		  strcat(cmd_buf, ev->key_compose);
		  _e_exebuf_update();
		  _e_exebuf_matches_update();
	       }
	  }
     }
   return 1;
}

static int
_e_exebuf_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Down *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   return 1;
}

static int
_e_exebuf_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   return 1;
}

static int
_e_exebuf_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   
   ev = event;
   if (ev->win != input_window) return 1;
   if (ev->z < 0) /* up */
     {
	int i;
	
	for (i = ev->z; i < 0; i++) _e_exebuf_prev();
     }
   else if (ev->z > 0) /* down */
     {
	int i;
	
	for (i = ev->z; i > 0; i--) _e_exebuf_next();
     }
   return 1;
}

static int
_e_exebuf_exe_scroll_timer(void *data)
{
   if (exe_scroll_to)
     {
	double spd;

	spd = e_config->exebuf_scroll_speed;
	exe_scroll_align = (exe_scroll_align * (1.0 - spd)) + (exe_scroll_align_to * spd);
	return 1;
     }
   exe_scroll_timer = NULL;
   return 0;
}

static int
_e_exebuf_eap_scroll_timer(void *data)
{
   if (eap_scroll_to)
     {
	double spd;

	spd = e_config->exebuf_scroll_speed;
	eap_scroll_align = (eap_scroll_align * (1.0 - spd)) + (eap_scroll_align_to * spd);
	return 1;
     }
   eap_scroll_timer = NULL;
   return 0;
}

static int
_e_exebuf_animator(void *data)
{
   if (exe_scroll_to)
     {
	double da;
	
	da = exe_scroll_align - exe_scroll_align_to;
	if (da < 0.0) da = -da;
	if (da < 0.01)
	  {
	     exe_scroll_align = exe_scroll_align_to;
	     exe_scroll_to = 0;
	  }
	e_box_align_set(exe_list_object, 0.5, 1.0 - exe_scroll_align);
     }
   if (eap_scroll_to)
     {
	double da;
	
	da = eap_scroll_align - eap_scroll_align_to;
	if (da < 0.0) da = -da;
	if (da < 0.01)
	  {
	     eap_scroll_align = eap_scroll_align_to;
	     eap_scroll_to = 0;
	  }
	e_box_align_set(eap_list_object, 0.5, eap_scroll_align);
     }
   if ((exe_scroll_to) || (eap_scroll_to)) return 1;
   animator = NULL;
   return 0;
}

static int
_e_exebuf_idler(void *data)
{
   struct stat st;
   struct dirent *dp;
   char *dir;
   char buf[4096];

   /* no more path items left - stop scanning */
   if (!exe_path)
     {
	exe_list_idler = NULL;
	return 0;
     }
   /* no dir is open - open the first path item */
   if (!exe_dir)
     {
	dir = exe_path->data;
	exe_dir = opendir(dir);
     }
   /* if we have an opened dir - scan the next item */
   if (exe_dir)
     {
	dir = exe_path->data;
	
	dp = readdir(exe_dir);
	if (dp)
	  {
	     if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, "..")))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);
		  if ((stat(buf, &st) == 0) &&
		      ((!S_ISDIR(st.st_mode)) &&
		       (!access(buf, X_OK))))
		    exe_list = evas_list_append(exe_list, strdup(buf));
	       }
	  }
	else
	  {
	     /* we reached the end of a dir - remove the dir at the head
	      * of the path list so we advance and next loop we will pick up
	      * the next item, or if null- abort
	      */
	     closedir(exe_dir);
	     exe_dir = NULL;
	     free(exe_path->data);
	     exe_path = evas_list_remove_list(exe_path, exe_path);
	  }
     }
   /* obviously the dir open failed - so remove the first path item */
   else
     {
	free(exe_path->data);
	exe_path = evas_list_remove_list(exe_path, exe_path);
     }
   /* we have mroe scannign to do */
   return 1;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* currently default bind is alt+` buf alt+space has been suggested */

/* local subsystem functions */

static void _e_exebuf_matches_clear(void);
static void _e_exebuf_update(void);
static void _e_exebuf_exec(void);
static void _e_exebuf_next(void);
static void _e_exebuf_prev(void);
static void _e_exebuf_complete(void);
static void _e_exebuf_backspace(void);
static void _e_exebuf_matches_update(void);
static int _e_exebuf_cb_key_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_down(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_up(void *data, int type, void *event);
static int _e_exebuf_cb_mouse_wheel(void *data, int type, void *event);

/* local subsystem globals */
static E_Popup *exebuf = NULL;
static Evas_Object *bg_object = NULL;
static Evas_List *handlers = NULL;
static Ecore_X_Window input_window = 0;
static char *cmd_buf = NULL;
static Evas_List *eap_matches = NULL;
static Evas_List *exe_matches = NULL;
static E_App *eap_sel = NULL;
static Ecore_List *exe_list = NULL;

#define EXEBUFLEN 2048

/* externally accessible functions */
int
e_exebuf_init(void)
{
   return 1;
}

int
e_exebuf_shutdown(void)
{
   e_exebuf_hide();
   return 1;
}

int
e_exebuf_show(E_Zone *zone)
{
   Evas_Object *o;
   int x, y, w, h;
   Evas_Coord mw, mh;
   
   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);
   
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
   edje_object_size_min_calc(o, &mw, &mh);
   
   x = zone->x + 20;
   y = zone->y + 20 + ((zone->h - 20 - 20 - mh) / 2);
   w = zone->w - 20 - 20;
   h = mh;
   
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
   
   exe_list = ecore_file_app_list();
   
   e_popup_show(exebuf);
   return 1;
}

void
e_exebuf_hide(void)
{
   if (!exebuf) return;
   
   evas_event_freeze(exebuf->evas);
   e_popup_hide(exebuf);
   evas_object_del(bg_object);
   bg_object = NULL;
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
   if (exe_list)
     {
	ecore_list_destroy(exe_list);
	exe_list = NULL;
     }
   _e_exebuf_matches_clear();
}

/* local subsystem functions */

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
   eap_sel = NULL;
}

static void
_e_exebuf_update(void)
{
   edje_object_part_text_set(bg_object, "label", cmd_buf);
   /* FIXME: if cmd_buf is an exact match for an eap or exe display an icon
    * to show it is */
   /* FIXME: display match lists. if match is exat match for cmd_buf dont
    * list it as it will be matched and indicated
    */
}

static void
_e_exebuf_exec(void)
{
   if (eap_sel)
     e_zone_app_exec(exebuf->zone, eap_sel);
   else
     e_zone_exec(exebuf->zone, cmd_buf);
   
   e_exebuf_hide();
}

static void
_e_exebuf_next(void)
{
   char *exe = NULL;
   
   if (exe_matches) exe = exe_matches->data;
   if (exe)
     {
	if (strlen(exe < (EXEBUFLEN - 1)))
	  {
	     strcpy(cmd_buf, exe);
	     _e_exebuf_update();
	  }
     }
}

static void
_e_exebuf_prev(void)
{
}

static void
_e_exebuf_complete(void)
{
   char common[EXEBUFLEN], *exe;
   Evas_List *l;
   int orig_len, common_len, exe_len, next_char, val, pos, matches;
   
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
   if ((exe) && (orig_len < common_len) && (common_len < (EXEBUFLEN - 1)))
     {
	strncpy(cmd_buf, exe, common_len);
	cmd_buf[common_len] = 0;
	_e_exebuf_update();
	_e_exebuf_matches_update();
     }
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
	  }
     }
}

static void
_e_exebuf_matches_update(void)
{
   char *path, *file, buf[4096];
   Evas_Hash *added = NULL;
   
   /* how to match:
    * 
    * eap_matches (above the exebuf)
    * match cmd_buf* for all eap->exe fields
    * match cmd_buf* for all eap->name fields
    * match *cmd_buf* for all eap->generic fields
    * match *cmd_buf* for all eap->comment fields
    * 
    * exe_matches (below the exebuf)
    * match cmd_buf* for all executables in $PATH (exclude duplicates in eap_matches)
    */
   _e_exebuf_matches_clear();
   snprintf(buf, sizeof(buf), "%s*", cmd_buf);
   if (exe_list)
     {
	ecore_list_goto_first(exe_list);
	while ((path = ecore_list_next(exe_list)) != NULL)
	  {
	     file = ecore_file_get_file(path);
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
	
	for (i = ev->z; i < 0; i++) e_exebuf_hide();
     }
   else if (ev->z > 0) /* down */
     {
	int i;
	
	for (i = ev->z; i > 0; i--) e_exebuf_hide();
     }
   return 1;
}

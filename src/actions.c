#include "actions.h"
#include "config.h"
#include "debug.h"
#include "border.h"
#include "desktops.h"
#include "exec.h"
#include "icccm.h"
#include "keys.h"
#include "view.h"
#include "util.h"
#include "guides.h"

static Evas_List action_impls = NULL;
static Evas_List current_actions = NULL;
static Evas_List current_timers = NULL;

static void e_action_find(char *action, E_Action_Type act, int button, char *key,
			  Ecore_Event_Key_Modifiers mods, E_Object *object);
static void e_action_cleanup(E_Action *a);

static void e_act_move_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_move_stop  (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_move_cont  (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_start (E_Object *object, E_Action *a, void *data,
				int x, int y, int rx, int ry);

static void e_act_resize_stop  (E_Object *object, E_Action *a, void *data,
				int x, int y, int rx, int ry);

static void e_act_resize_cont  (E_Object *object, E_Action *a, void *data,
				int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_h_start (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry);

static void e_act_resize_h_stop  (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry);

static void e_act_resize_h_cont  (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_v_start (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry);

static void e_act_resize_v_stop  (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry);

static void e_act_resize_v_cont  (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry, int dx, int dy);

static void e_act_close_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_kill_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_shade_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_raise_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_lower_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_raise_lower_start (E_Object *object, E_Action *a,
				     void *data, int x, int y, int rx, int ry);

static void e_act_exec_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_menu_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_exit_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_restart_start (E_Object *object, E_Action *a, void *data,
				 int x, int y, int rx, int ry);

static void e_act_stick_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_sound_start (E_Object *object, E_Action *a, void *data,
			       int x, int y, int rx, int ry);

static void e_act_iconify_start (E_Object *object, E_Action *a, void *data,
				 int x, int y, int rx, int ry);

static void e_act_max_start (E_Object *object, E_Action *a, void *data,
			     int x, int y, int rx, int ry);

static void e_act_snap_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_zoom_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_desk_start (E_Object *object, E_Action *a, void *data,
			      int x, int y, int rx, int ry);

static void e_act_raise_next_start (E_Object *object, E_Action *a, void *data,
				    int x, int y, int rx, int ry);

static void e_act_desk_rel_start (E_Object *object, E_Action *a, void *data,
				  int x, int y, int rx, int ry);


static void
e_action_find(char *action, E_Action_Type act, int button,
	      char *key, Ecore_Event_Key_Modifiers mods, E_Object *object)
{
   char *actions_db;
   E_DB_File *db;
   int i, num;
   char *a_name = NULL;
   char *a_action = NULL;
   char *a_params = NULL;
   int   a_event = 0;
   int   a_button = 0;
   char *a_key = NULL;
   int   a_modifiers = 0;
   Evas_List l;
   E_Action *a;
   static Evas_List actions = NULL;
   E_CFG_FILE(cfg_actions, "actions");

   D_ENTER;

   E_CONFIG_CHECK_VALIDITY(cfg_actions, "actions");

   /* if we had a previous list - nuke it */

   /* FIXME: this has potential to segfault if reference
      counting is actually used and those actions are
      referenced in more than one place --cK.
   */

   if (actions)
     {
	for (l = actions; l; l = l->next)
	  {
	     a = l->data;
	     if (a) e_action_cleanup(a);
	  }
	actions = evas_list_free(actions);
     }
   /* now build the list again */
   actions_db = e_config_get("actions");
   db = e_db_open_read(actions_db);
   if (!db) D_RETURN;
   if (!e_db_int_get(db, "/actions/count", &num)) goto error;
   for (i = 0; i < num; i++)
     {
	char buf[PATH_MAX];
	
	sprintf(buf, "/actions/%i/name", i);
	a_name = e_db_str_get(db, buf);
	sprintf(buf, "/actions/%i/action", i);
	a_action = e_db_str_get(db, buf);
	sprintf(buf, "/actions/%i/params", i);
	a_params = e_db_str_get(db, buf);
	sprintf(buf, "/actions/%i/event", i);
	e_db_int_get(db, buf, &a_event);
	sprintf(buf, "/actions/%i/button", i);
	e_db_int_get(db, buf, &a_button);
	sprintf(buf, "/actions/%i/key", i);
	a_key = e_db_str_get(db, buf);
	sprintf(buf, "/actions/%i/modifiers", i);
	e_db_int_get(db, buf, &a_modifiers);
	
	a = NEW(E_Action, 1);
	ZERO(a, E_Action, 1);
	
	e_object_init(E_OBJECT(a), (E_Cleanup_Func) e_action_cleanup);
	
	a->name = a_name;
	a->action = a_action;
	a->params = a_params;
	a->event = a_event;
	a->button = a_button;
	a->key = a_key;
	a->modifiers = a_modifiers;
	a->action_impl = NULL;
	a->object = NULL;
	a->started = 0;
	actions = evas_list_append(actions, a);
	/* it's a key? lets grab it! */
	if ((a->key) && (strlen(a->key) > 0))
	  {
	     if (a->modifiers == -1)
	       e_keys_grab(a->key, ECORE_EVENT_KEY_MODIFIER_NONE, 1);
	     else
	       e_keys_grab(a->key, (Ecore_Event_Key_Modifiers)a->modifiers, 0);
	     a->grabbed = 1;
	  }
     }
   error:
   e_db_close(db);
   E_CONFIG_CHECK_VALIDITY_END;
   /* run thru our actions list and match event, state and stuff with an */
   /* and action for it */
   for (l = actions; l; l = l->next)
     {
	Evas_List ll;
	
	a = l->data;
	if (act != a->event) goto next;
	if (!((a->name) && 
	      (action) && 
	      (!strcmp(a->name, action)))) goto next;
	if ((act >= ACT_MOUSE_CLICK) && 
	    (act <= ACT_MOUSE_CLICKED) && 
	    (!((a->button == -1) || 
	       (a->button == button)))) goto next;
	if ((act >= ACT_KEY_DOWN) && 
	    (act <= ACT_KEY_UP) &&
	    (!((a->key) && (key) &&
	       (!strcmp(a->key, key))))) goto next;
	if ((act >= ACT_MOUSE_CLICK) &&
	    (act <= ACT_KEY_UP) &&
	    (!((a->modifiers == -1) || 
	       (a->modifiers == (int)mods)))) goto next;
	for (ll = action_impls; ll; ll = ll->next)
	  {
	     E_Action_Impl *ap;
	     
	     ap = ll->data;
	     if (!strcmp(ap->action, a->action))
	       {
		  E_Action *aa;
		  
		  aa = NEW(E_Action, 1);
		  ZERO(aa, E_Action, 1);
		  
		  e_object_init(E_OBJECT(aa), (E_Cleanup_Func) e_action_cleanup);
		  
		  e_strdup(aa->name, a->name);
		  e_strdup(aa->action, a->action);
		  e_strdup(aa->params, a->params);
		  aa->event = a->event;
		  aa->button = a->button;
		  e_strdup(aa->key, a->key);
		  aa->modifiers = a->modifiers;

		  aa->action_impl = ap;
		  e_object_ref(E_OBJECT(ap));

		  aa->object = object;
		  e_object_ref(object);

		  aa->started = 0;
		  current_actions = evas_list_append(current_actions, aa);
	       }
	  }
	next:
     }

   D_RETURN;
}

static void
e_action_cleanup(E_Action *a)
{
   D_ENTER;

   /* it's a key? lets ungrab it! */
   if ((a->key) && (strlen(a->key) > 0) && (a->grabbed))
     {
	if (a->modifiers == -1)
	  e_keys_ungrab(a->key, ECORE_EVENT_KEY_MODIFIER_NONE, 1);
	else
	  e_keys_ungrab(a->key, (Ecore_Event_Key_Modifiers)a->modifiers, 0);
     }

   /* Clean up the strings by simply freeing them ... */
   IF_FREE(a->name);
   IF_FREE(a->action);
   IF_FREE(a->params);
   IF_FREE(a->key);

   /* Cleanup action implementations and objects. These
      we don't free directly, but just decrement their use counts.
   */

   if (a->action_impl)
     {
       e_object_unref(E_OBJECT(a->action_impl));
       a->action_impl = NULL;
     }

   if (a->object)
     {
       e_object_unref(a->object);
       a->object = NULL;
     }

   /* Cleanup superclass. */
   e_object_cleanup(E_OBJECT(a));

   D_RETURN;
}

int
e_action_start(char *action, E_Action_Type act, int button,
	       char *key, Ecore_Event_Key_Modifiers mods,
	       E_Object *object, void *data, int x, int y, int rx, int ry)
{
   Evas_List l;
   int started_long_action = 0;
   
   D_ENTER;

   e_action_find(action, act, button, key, mods, object);
   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if (!a->started)
	  {
	     if (a->action_impl->func_stop)
	       {
		  a->started = 1;
		  started_long_action = 1;
	       }
	     if (a->action_impl->func_start)
	       {
		  a->action_impl->func_start(a->object, a, data, x, y, rx, ry);
	       }
	  }
	if (!a->started)
	  {
	     current_actions = evas_list_remove(current_actions, a);
	     e_object_unref(E_OBJECT(a));
	     goto again;
	  }
     }

   D_RETURN_(started_long_action);
}

void
e_action_stop(char *action, E_Action_Type act, int button,
	      char *key, Ecore_Event_Key_Modifiers mods, E_Object *object,
	      void *data, int x, int y, int rx, int ry)
{
   Evas_List l;

   D_ENTER;

   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (a->action_impl->func_stop))
	  {
	     int ok = 0;
	     
	     if ((a->event == ACT_MOUSE_IN) && 
		 (act == ACT_MOUSE_OUT))
	       ok = 1;
	     if ((a->event == ACT_MOUSE_OUT) && 
		 (act == ACT_MOUSE_IN))
	       ok = 1;
	     if ((a->event >= ACT_MOUSE_CLICK) && 
		 (a->event <= ACT_MOUSE_TRIPLE) &&
		 (act >= ACT_MOUSE_UP) &&
		 (act <= ACT_MOUSE_CLICKED) &&
		 (a->button == button))
	       ok = 1;
	     if ((a->event == ACT_MOUSE_MOVE) && 
		 ((act == ACT_MOUSE_OUT) ||
		  (act == ACT_MOUSE_IN) ||
		  ((act >= ACT_MOUSE_CLICK) &&
		   (act <= ACT_MOUSE_TRIPLE)) ||
		  (act >= ACT_MOUSE_UP)))
	       ok = 1;
	     if ((a->event == ACT_KEY_DOWN) && 
		 (act == ACT_KEY_UP) &&
		 (key) && (a->key) && (!strcmp(key, a->key)))
	       ok = 1;
	     if ((a->event == ACT_KEY_UP) && 
		 (act == ACT_KEY_DOWN))
	       ok = 1;
	     if (ok)
	       {
		  a->action_impl->func_stop(a->object, a, data, x, y, rx, ry);
		  a->started = 0;
	       }
	  }
	if (!a->started)
	  {
	     current_actions = evas_list_remove(current_actions, a);	     
	     e_object_unref(E_OBJECT(a));
	     goto again;
	  }
     }

   D_RETURN;
   UN(action);
   UN(mods);
   UN(object);
}

void
e_action_cont(char *action, E_Action_Type act, int button, char *key,
	      Ecore_Event_Key_Modifiers mods, E_Object *object, void *data,
	      int x, int y, int rx, int ry, int dx, int dy)
{
   Evas_List l;

   D_ENTER;

   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (a->action_impl->func_cont))
	  a->action_impl->func_cont(a->object, a, data, x, y, rx, ry, dx, dy);
     }

   D_RETURN;
   UN(action);
   UN(act);
   UN(button);
   UN(key);
   UN(mods);
   UN(object);
}

void
e_action_stop_by_object(E_Object *object, void *data, int x, int y, int rx, int ry)
{
   Evas_List l;

   D_ENTER;

   e_action_del_timer_object(object);

   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (object == a->object))
	  {
	     if (a->action_impl->func_stop)
	       a->action_impl->func_stop(a->object, a, data, x, y, rx, ry);

	     a->started = 0;

	     current_actions = evas_list_remove(current_actions, a);
	     e_object_unref(E_OBJECT(a));

	     goto again;
	  }
     }

   D_RETURN;
}

void
e_action_stop_by_type(char *action)
{
   Evas_List l;

   D_ENTER;

   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (a->action_impl->func_stop) && 
	    (action) && (!strcmp(action, a->name)))
	  {
	     a->action_impl->func_stop(a->object, a, NULL, 0, 0, 0, 0);
	     a->started = 0;
	  }
     }

   D_RETURN;
}


static void
e_action_impl_cleanup(E_Action_Impl *eai)
{
   D_ENTER;

   e_object_cleanup(E_OBJECT(eai));

   D_RETURN;
}


void
e_action_add_impl(char *action,  E_Action_Start_Func func_start,
		  E_Action_Cont_Func func_cont, E_Action_Stop_Func func_stop)
{
   E_Action_Impl *ap;
   
   D_ENTER;

   ap = NEW(E_Action_Impl, 1);
   ZERO(ap, E_Action_Impl, 1); 

   e_object_init(E_OBJECT(ap), (E_Cleanup_Func) e_action_impl_cleanup);
   
   e_strdup(ap->action, action);
   ap->func_start = func_start;
   ap->func_cont = func_cont;
   ap->func_stop = func_stop;
   action_impls = evas_list_append(action_impls, ap);

   D_RETURN;
}


void
e_action_del_timer(E_Object *object, char *name)
{
   Evas_List l;

   D_ENTER;
   
   again:
   for (l = current_timers; l; l = l->next)
     {
	E_Active_Action_Timer *at;
	
	at = l->data;
	if ((at->object == object) && 
	    (name) && 
	    (at->name) && 
	    (!strcmp(at->name, name)))
	  {
	     e_object_unref(at->object);
	     ecore_del_event_timer(at->name);
	     current_timers = evas_list_remove(current_timers, at);
	     IF_FREE(at->name);
	     FREE(at);
	     goto again;
	  }
     }

   D_RETURN;
}

void
e_action_add_timer(E_Object *object, char *name)
{
   E_Active_Action_Timer *at;
   
   D_ENTER;

   at = NEW(E_Active_Action_Timer, 1);
   at->object = object;
   e_object_ref(object);
   e_strdup(at->name, name);
   current_timers = evas_list_append(current_timers, at);

   D_RETURN;
}

void
e_action_del_timer_object(E_Object *object)
{
   Evas_List l;
   
   D_ENTER;

   again:
   for (l = current_timers; l; l = l->next)
     {
	E_Active_Action_Timer *at;
	
	at = l->data;
	if (at->object == object)
	  {
	     e_object_unref(at->object);
	     ecore_del_event_timer(at->name);
	     current_timers = evas_list_remove(current_timers, at);
	     IF_FREE(at->name);
	     FREE(at);
	     goto again;
	  }
     }
   
   D_RETURN;
}

void
e_action_init(void)
{
   D_ENTER;

   e_action_add_impl("Window_Move", e_act_move_start, e_act_move_cont, e_act_move_stop);
   e_action_add_impl("Window_Resize", e_act_resize_start, e_act_resize_cont, e_act_resize_stop);
   e_action_add_impl("Window_Resize_Horizontal", e_act_resize_h_start, e_act_resize_h_cont, e_act_resize_h_stop);
   e_action_add_impl("Window_Resize_Vertical", e_act_resize_v_start, e_act_resize_v_cont, e_act_resize_v_stop);
   e_action_add_impl("Window_Close", e_act_close_start, NULL, NULL);
   e_action_add_impl("Window_Kill", e_act_kill_start, NULL, NULL);
   e_action_add_impl("Window_Shade", e_act_shade_start, NULL, NULL);
   e_action_add_impl("Window_Raise", e_act_raise_start, NULL, NULL);
   e_action_add_impl("Window_Lower", e_act_lower_start, NULL, NULL);
   e_action_add_impl("Window_Raise_Lower", e_act_raise_lower_start, NULL, NULL);
   e_action_add_impl("Execute", e_act_exec_start, NULL, NULL);
   e_action_add_impl("Menu", e_act_menu_start, NULL, NULL);
   e_action_add_impl("Exit", e_act_exit_start, NULL, NULL);
   e_action_add_impl("Restart", e_act_restart_start, NULL, NULL);
   e_action_add_impl("Window_Stick", e_act_stick_start, NULL, NULL);
   e_action_add_impl("Sound", e_act_sound_start, NULL, NULL);
   e_action_add_impl("Window_Iconify", e_act_iconify_start, NULL, NULL);
   e_action_add_impl("Window_Max_Size", e_act_max_start, NULL, NULL);
   e_action_add_impl("Winodw_Snap", e_act_snap_start, NULL, NULL);
   e_action_add_impl("Window_Zoom", e_act_zoom_start, NULL, NULL);
   e_action_add_impl("Desktop", e_act_desk_start, NULL, NULL);
   e_action_add_impl("Desktop_Relative", e_act_desk_rel_start, NULL, NULL);
   e_action_add_impl("Window_Next", e_act_raise_next_start, NULL, NULL);

   D_RETURN;
}


/* FIXME: these REALLY need to go into other file(s) but it's not worht it */
/* yet at this point. it can be done later */

/* Erm is that really true? They're all static, all called through the */
/* above functions -- so it's good to have them encapsulated here? --cK */

/* well i was thinking changing this to be a bunch of: */
/* #include "action_windows.c" */
/* #include "action_files.c" */
/* #include "action_general.c" */
/* etc. - group actions in files for their logical uses */
/* kind of evil to inlucde c files.. but it means breaking it up better */
/* probably moving these includes above the init and having hooks into the */
/* init func */

static void 
e_act_move_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   E_Guides_Mode move_mode = E_GUIDES_BOX;
   double align_x = 0.5;
   double align_y = 0.5;
   E_Guides_Location display_loc = E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE;
   E_CFG_INT(cfg_window_move_mode, "settings", "/window/move/mode", E_GUIDES_BOX);
   E_CFG_FLOAT(cfg_guides_display_x, "settings", "/guides/display/x", 0.5);
   E_CFG_FLOAT(cfg_guides_display_y, "settings", "/guides/display/y", 0.5);
   E_CFG_INT(cfg_guides_display_location, "settings", "/guides/display/location", E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE);
   
   D_ENTER;

   E_CONFIG_INT_GET(cfg_window_move_mode, move_mode);
   E_CONFIG_FLOAT_GET(cfg_guides_display_x, align_x);
   E_CONFIG_FLOAT_GET(cfg_guides_display_y, align_y);
   E_CONFIG_INT_GET(cfg_guides_display_location, display_loc);

   b = (E_Border*) object;

   if (!b)
     b = e_border_current_focused();

   if (!b)
     D_RETURN;

   if (b->client.fixed)
     D_RETURN;

   if (move_mode >= E_GUIDES_BOX)
     b->hold_changes = 1; /* if non opaque */
   b->mode.move = 1;
   b->current.requested.dx = 0;
   b->current.requested.dy = 0;
   b->previous.requested.dx = 0;
   b->previous.requested.dy = 0;

     {
	char buf[PATH_MAX];
	
	e_border_print_pos(buf, b);
	e_guides_set_display_alignment(align_x, align_y);
	e_guides_set_mode(move_mode);
	e_guides_set_display_location(display_loc);
	e_guides_display_text(buf);	
	sprintf(buf, "%s/%s", e_config_get("images"), "win_shadow_icon.png");
	e_guides_display_icon(buf);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_show();
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_move_stop  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;

   D_ENTER;

   b = (E_Border*) object;

   if (!b)
     b = e_border_current_focused();

   if (!b)
     D_RETURN;

   if (b->client.fixed)
     D_RETURN;

   b->hold_changes = 0; /* if non opaque */
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->changed = 1;
   b->mode.move = 0;
   b->current.requested.dx = 0;
   b->current.requested.dy = 0;
   b->previous.requested.dx = 0;
   b->previous.requested.dy = 0;
   b->changed = 1;
   b->current.requested.visible = 1;
   b->current.visible = 1;
   e_border_adjust_limits(b);
   e_guides_hide();
   e_desktops_add_border(b->desk, b);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_move_cont  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;

   D_ENTER;
   
   b = (E_Border*) object;

   if (!b)
     b = e_border_current_focused();

   if (!b)
     D_RETURN;

   if (b->client.fixed)
     D_RETURN;

   b->current.requested.x += dx;
   b->current.requested.y += dy;
   if (dx != 0) b->current.requested.dx = dx;
   if (dy != 0) b->current.requested.dy = dy;
   b->changed = 1;
   e_border_adjust_limits(b);
     {
	char buf[1024];
	
	e_border_print_pos(buf, b);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_display_text(buf);
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_resize_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   E_Guides_Mode resize_mode = E_GUIDES_BOX;
   double align_x = 0.5;
   double align_y = 0.5;
   E_Guides_Location display_loc = E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE;
   E_CFG_INT(cfg_window_resize_mode, "settings", "/window/resize/mode", E_GUIDES_BOX);
   E_CFG_FLOAT(cfg_guides_display_x, "settings", "/guides/display/x", 0.5);
   E_CFG_FLOAT(cfg_guides_display_y, "settings", "/guides/display/y", 0.5);
   E_CFG_INT(cfg_guides_display_location, "settings", "/guides/display/location", E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE);

   D_ENTER;
   
   E_CONFIG_INT_GET(cfg_window_resize_mode, resize_mode);
   E_CONFIG_FLOAT_GET(cfg_guides_display_x, align_x);
   E_CONFIG_FLOAT_GET(cfg_guides_display_y, align_y);   
   E_CONFIG_INT_GET(cfg_guides_display_location, display_loc);
   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (resize_mode >= E_GUIDES_BOX)
     b->hold_changes = 1; /* if non opaque */
   ecore_window_gravity_set(b->win.client, StaticGravity);
   ecore_window_gravity_set(b->win.l, NorthWestGravity);
   ecore_window_gravity_set(b->win.r, SouthEastGravity);
   ecore_window_gravity_set(b->win.t, NorthWestGravity);
   ecore_window_gravity_set(b->win.b, SouthEastGravity);
   ecore_window_gravity_set(b->win.input, NorthWestGravity);
   ecore_window_gravity_set(b->win.container, NorthWestGravity);
   /* 1 | 2 */
   /* --+-- */
   /* 3 | 4 */
   if (x > (b->current.w / 2)) 
     {
	if (y > (b->current.h / 2)) 
	  {
	     b->mode.resize = 4;
	     /*	     e_border_set_gravity(b, NorthWestGravity); */
	     /*	     ecore_window_gravity_set(b->win.container, SouthEastGravity);*/
	  }
	else 
	  {
	     b->mode.resize = 2;
	     /*	     e_border_set_gravity(b, SouthWestGravity);*/
	     /*	     ecore_window_gravity_set(b->win.container, NorthEastGravity);*/
	  }
     }
   else
     {
	if (y > (b->current.h / 2)) 
	  {
	     b->mode.resize = 3;
/*	     e_border_set_gravity(b, NorthEastGravity);*/
/*	     ecore_window_gravity_set(b->win.container, SouthWestGravity);*/
	  }
	else 
	  {
	     b->mode.resize = 1;
/*	     e_border_set_gravity(b, SouthEastGravity);*/
/*	     ecore_window_gravity_set(b->win.container, NorthWestGravity); */
	  }
     }
     {
	char buf[PATH_MAX];
	
	e_border_print_size(buf, b);
	e_guides_set_display_alignment(align_x, align_y);
	e_guides_set_mode(resize_mode);
	e_guides_set_display_location(display_loc);
	e_guides_display_text(buf);	
	sprintf(buf, "%s/%s", e_config_get("images"), "win_shadow_icon.png");
	e_guides_display_icon(buf);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_show();
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_stop  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   b->hold_changes = 0; /* if non opaque */
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   b->changed = 1;
   e_border_adjust_limits(b);
   ecore_window_gravity_set(b->win.client, NorthWestGravity);
   e_border_set_gravity(b, NorthWestGravity);
   e_guides_hide();

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_cont  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;

   D_ENTER;
   
   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (b->mode.resize == 1)
     {
	b->current.requested.w -= dx;
	b->current.requested.h -= dy;
	b->current.requested.x += dx;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 2)
     {
	b->current.requested.w += dx;
	b->current.requested.h -= dy;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 3)
     {
	b->current.requested.w -= dx;
	b->current.requested.h += dy;
	b->current.requested.x += dx;
     }
   else if (b->mode.resize == 4)
     {
	b->current.requested.w += dx;
	b->current.requested.h += dy;
     }
   b->changed = 1;
   e_border_adjust_limits(b);
     {
	char buf[1024];
	
	e_border_print_size(buf, b);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_display_text(buf);
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_resize_h_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   E_Guides_Mode resize_mode = E_GUIDES_BOX;
   double align_x = 0.5;
   double align_y = 0.5;
   E_Guides_Location display_loc = E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE;
   E_CFG_INT(cfg_window_resize_mode, "settings", "/window/resize/mode", E_GUIDES_BOX);
   E_CFG_FLOAT(cfg_guides_display_x, "settings", "/guides/display/x", 0.5);
   E_CFG_FLOAT(cfg_guides_display_y, "settings", "/guides/display/y", 0.5);
   E_CFG_INT(cfg_guides_display_location, "settings", "/guides/display/location", E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE);

   D_ENTER;
   
   E_CONFIG_INT_GET(cfg_window_resize_mode, resize_mode);
   E_CONFIG_FLOAT_GET(cfg_guides_display_x, align_x);
   E_CONFIG_FLOAT_GET(cfg_guides_display_y, align_y);   
   E_CONFIG_INT_GET(cfg_guides_display_location, display_loc);
   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (resize_mode >= E_GUIDES_BOX)
     b->hold_changes = 1; /* if non opaque */
   ecore_window_gravity_set(b->win.client, StaticGravity);
   ecore_window_gravity_set(b->win.l, NorthWestGravity);
   ecore_window_gravity_set(b->win.r, SouthEastGravity);
   ecore_window_gravity_set(b->win.t, NorthWestGravity);
   ecore_window_gravity_set(b->win.b, SouthEastGravity);
   ecore_window_gravity_set(b->win.input, NorthWestGravity);
   ecore_window_gravity_set(b->win.container, NorthWestGravity);
   /* 5 | 6 */
   if (x > (b->current.w / 2)) 
     {
	b->mode.resize = 6;
/*	e_border_set_gravity(b, NorthWestGravity);*/
     }
   else 
     {
	b->mode.resize = 5;
/*	e_border_set_gravity(b, NorthEastGravity);*/
     }
     {
	char buf[PATH_MAX];
	
	e_border_print_size(buf, b);
	e_guides_set_display_alignment(align_x, align_y);
	e_guides_set_mode(resize_mode);
	e_guides_set_display_location(display_loc);
	e_guides_display_text(buf);	
	sprintf(buf, "%s/%s", e_config_get("images"), "win_shadow_icon.png");
	e_guides_display_icon(buf);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_show();
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_h_stop  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;


   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   b->hold_changes = 0; /* if non opaque */
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   b->changed = 1;
   e_border_adjust_limits(b);
   ecore_window_gravity_set(b->win.client, NorthWestGravity);
   e_border_set_gravity(b, NorthWestGravity);
   e_guides_hide();

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_h_cont  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.w == b->client.max.w) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (b->mode.resize == 5)
     {
	b->current.requested.w -= dx;
	b->current.requested.x += dx;
     }
   else if (b->mode.resize == 6)
     {
	b->current.requested.w += dx;
     }
   b->changed = 1;
   e_border_adjust_limits(b);
     {
	char buf[1024];
	
	e_border_print_size(buf, b);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_display_text(buf);
     }
   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
   UN(dy);
}


static void 
e_act_resize_v_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   E_Guides_Mode resize_mode = E_GUIDES_BOX;
   double align_x = 0.5;
   double align_y = 0.5;
   E_Guides_Location display_loc = E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE;
   E_CFG_INT(cfg_window_resize_mode, "settings", "/window/resize/mode", E_GUIDES_BOX);
   E_CFG_FLOAT(cfg_guides_display_x, "settings", "/guides/display/x", 0.5);
   E_CFG_FLOAT(cfg_guides_display_y, "settings", "/guides/display/y", 0.5);
   E_CFG_INT(cfg_guides_display_location, "settings", "/guides/display/location", E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE);
   
   D_ENTER;

   E_CONFIG_INT_GET(cfg_window_resize_mode, resize_mode);
   E_CONFIG_FLOAT_GET(cfg_guides_display_x, align_x);
   E_CONFIG_FLOAT_GET(cfg_guides_display_y, align_y);   
   E_CONFIG_INT_GET(cfg_guides_display_location, display_loc);
   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (resize_mode >= E_GUIDES_BOX)
     b->hold_changes = 1; /* if non opaque */
   ecore_window_gravity_set(b->win.client, StaticGravity);
   ecore_window_gravity_set(b->win.l, NorthWestGravity);
   ecore_window_gravity_set(b->win.r, SouthEastGravity);
   ecore_window_gravity_set(b->win.t, NorthWestGravity);
   ecore_window_gravity_set(b->win.b, SouthEastGravity);
   ecore_window_gravity_set(b->win.input, NorthWestGravity);
   ecore_window_gravity_set(b->win.container, NorthWestGravity);
   /* 7 */
   /* - */
   /* 8 */
   if (y > (b->current.h / 2)) 
     {
	b->mode.resize = 8;
/*	e_border_set_gravity(b, NorthWestGravity);*/
     }
   else 
     {
	b->mode.resize = 7;
/*	e_border_set_gravity(b, SouthWestGravity);*/
     }
     {
	char buf[PATH_MAX];
	
	e_border_print_size(buf, b);
	e_guides_set_display_alignment(align_x, align_y);
	e_guides_set_mode(resize_mode);
	e_guides_set_display_location(display_loc);
	e_guides_display_text(buf);	
	sprintf(buf, "%s/%s", e_config_get("images"), "win_shadow_icon.png");
	e_guides_display_icon(buf);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_show();
     }
   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_v_stop  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   b->hold_changes = 0; /* if non opaque */
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   e_border_adjust_limits(b);
   ecore_window_gravity_set(b->win.client, NorthWestGravity);
   e_border_set_gravity(b, NorthWestGravity);
   b->changed = 1;
   e_guides_hide();
   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_v_cont  (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.min.h == b->client.max.h) D_RETURN;
   if (b->current.shaded != 0) D_RETURN;
   if (b->mode.resize == 7)
     {
	b->current.requested.h -= dy;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 8)
     {
	b->current.requested.h += dy;
     }
   e_border_adjust_limits(b);
   b->changed = 1;
     {
	char buf[1024];
	
	e_border_print_size(buf, b);
	e_guides_move(b->current.x, b->current.y);
	e_guides_resize(b->current.w, b->current.h);
	e_guides_display_text(buf);
     }
   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
   UN(dx);
}


static void 
e_act_close_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;


   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   if (b->win.client) e_icccm_delete(b->win.client);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_kill_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   if (b->win.client) ecore_window_kill_client(b->win.client);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void e_act_cb_shade(int val, void *data);
static void
e_act_cb_shade(int val, void *data)
{
   E_Border *b;
   static double t = 0.0;
   double dif;
   int si;
   int pix_per_sec = 3200;
   
   D_ENTER;

   b = data;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop)
     D_RETURN;

   if (val == 0) 
     {
	t = ecore_get_time();
	ecore_window_gravity_set(b->win.client, SouthWestGravity);
	e_action_del_timer(E_OBJECT(b), "shader");
	e_action_add_timer(E_OBJECT(b), "shader");
     }
   
   dif = ecore_get_time() - t;   
   
   si = (int)(dif * (double)pix_per_sec);
   if (si > b->client.h) si = b->client.h;
   b->current.shaded = si;
   b->changed = 1;
   e_border_adjust_limits(b);
   e_border_apply_border(b);
   if (si < b->client.h) 
     ecore_add_event_timer("shader", 0.01, e_act_cb_shade, 1, data);
   else
     {
	e_action_del_timer(E_OBJECT(b), "shader");
	ecore_window_gravity_reset(b->win.client);
     }

   D_RETURN;
}

static void e_act_cb_unshade(int val, void *data);
static void
e_act_cb_unshade(int val, void *data)
{
   E_Border *b;
   static double t = 0.0;
   double dif;
   int si;
   int pix_per_sec = 3200;
   
   D_ENTER;

   b = data;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   if (val == 0) 
     {
	t = ecore_get_time();
	ecore_window_gravity_set(b->win.client, SouthWestGravity);
	e_action_del_timer(E_OBJECT(b), "shader");
	e_action_add_timer(E_OBJECT(b), "shader");
     }
   
   dif = ecore_get_time() - t;   
   
   si = b->client.h - (int)(dif * (double)pix_per_sec);
   if (si < 0) si = 0;

   b->current.shaded = si;
   b->changed = 1;
   e_border_adjust_limits(b);
   e_border_apply_border(b);
   if (si > 0) 
     ecore_add_event_timer("shader", 0.01, e_act_cb_unshade, 1, data);
   else
     {
	e_action_del_timer(E_OBJECT(b), "shader");
	ecore_window_gravity_reset(b->win.client);
     }

   D_RETURN;
}

static void 
e_act_shade_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;

   if (b->client.is_desktop)
     D_RETURN;

   if (b->current.shaded == 0)
     e_act_cb_shade(0, b);
   else
     e_act_cb_unshade(0, b);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_raise_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   e_border_raise(b);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_lower_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   e_border_lower(b);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_raise_lower_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_exec_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   char *exe;
   
   D_ENTER;

   exe = (char *) a->params;
   if(!exe) D_RETURN;
   e_exec_run(exe);

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
   UN(object);
}


static void 
e_act_menu_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_exit_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   D_ENTER;

   exit(0);

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_restart_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   D_ENTER;

   e_exec_restart();

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_stick_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   if (b->client.sticky) b->client.sticky = 0;
   else b->client.sticky = 1;
   b->changed = 1;

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_sound_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   D_ENTER;

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_iconify_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_max_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;
   if (!b) b = e_border_current_focused();
   if (!b) D_RETURN;
   if (b->client.is_desktop) D_RETURN;
   if (b->current.shaded > 0) D_RETURN;
   if ((b->mode.move) || (b->mode.resize)) D_RETURN;
   b->mode.move = 0;
   b->mode.resize = 0;
   if (b->max.is)
     {
	b->current.requested.x = b->max.x;
	b->current.requested.y = b->max.y;
	b->current.requested.w = b->max.w;
	b->current.requested.h = b->max.h;
	b->changed = 1;
	b->max.is = 0;
	e_border_adjust_limits(b);
	b->current.requested.x = b->current.x;
	b->current.requested.y = b->current.y;
	b->current.requested.w = b->current.w;
	b->current.requested.h = b->current.h;
     }
   else
     {
	b->max.x = b->current.x;
	b->max.y = b->current.y;
	b->max.w = b->current.w;
	b->max.h = b->current.h;
	b->current.requested.x = 0;
	b->current.requested.y = 0;
	b->current.requested.w = b->desk->real.w;
	b->current.requested.h = b->desk->real.h;
	b->changed = 1;
	b->max.is = 1;
	e_border_adjust_limits(b);
	b->current.requested.x = b->current.x;
	b->current.requested.y = b->current.y;
	b->current.requested.w = b->current.w;
	b->current.requested.h = b->current.h;
     }

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_snap_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;

   if (!b)
     b = e_border_current_focused();
   if (!b)
     D_RETURN;

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_zoom_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   D_ENTER;

   b = (E_Border*) object;

   if (!b)
     b = e_border_current_focused();

   if (!b)
     D_RETURN;

   D_RETURN;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_desk_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   int desk = 0;
   
   D_ENTER;

   if (a->params)
     desk = atoi(a->params);

   e_desktops_goto_desk(desk);

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void
e_act_desk_rel_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   int desk = 0;
   int desk_max = e_desktops_get_num () - 1;

   D_ENTER;
   
   if (a->params)
     desk = atoi(a->params) + e_desktops_get_current();

   if (desk < 0)
     desk = desk_max;
   else if (desk > desk_max)
     desk = 0;

   e_desktops_goto_desk(desk);

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_raise_next_start (E_Object *object, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   D_ENTER;

   e_border_raise_next();

   D_RETURN;
   UN(object);
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

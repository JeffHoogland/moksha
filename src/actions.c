#include "e.h"

static Evas_List action_protos = NULL;
static Evas_List current_actions = NULL;

static void _e_action_find(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o);
static void _e_action_free(E_Action *a);

static void e_act_move_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_move_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_move_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_h_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_h_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_h_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_resize_v_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_v_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
static void e_act_resize_v_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);

static void e_act_close_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_kill_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_shade_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_raise_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_raise_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_exec_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_menu_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_exit_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_restart_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_stick_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_sound_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_iconify_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_max_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_snap_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void e_act_zoom_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);

static void
_e_action_find(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o)
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
   E_Action *a;

   actions_db = e_config_get("actions");
   db = e_db_open_read(actions_db);
   if (!db) return;
   if (!e_db_int_get(db, "/actions/count", &num)) goto error;
   for (i = 0; i < num; i++)
     {
	char buf[4096];
	Evas_List l;
	a = NULL;
	
	a = NULL;
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
	
	if (act != a_event) goto next;
	if (!((a_name) && 
	      (action) && 
	      (!strcmp(a_name, action)))) goto next;
	if ((act >= ACT_MOUSE_CLICK) && 
	    (act <= ACT_MOUSE_CLICKED) && 
	    (!((a_button == -1) || 
	       (a_button == button)))) goto next;
	if ((act >= ACT_KEY_DOWN) && 
	    (act <= ACT_KEY_UP) &&
	    (!((a_key) && (key) &&
	       (!strcmp(a_key, key))))) goto next;
	if ((act >= ACT_MOUSE_CLICK) &&
	    (act <= ACT_KEY_UP) &&
	    (!((a_modifiers == -1) || 
	       (a_modifiers == (int)mods)))) goto next;
	for (l = action_protos; l; l = l->next)
	  {
	     E_Action_Proto *ap;
	     
	     ap = l->data;
	     if (!strcmp(ap->action, a_action))
	       {
		  
		  a = NEW(E_Action, 1);
		  ZERO(a, E_Action, 1);
		  
		  OBJ_INIT(a, _e_action_free);
		  
		  a->name = a_name;
		  a->action = a_action;
		  a->params = a_params;
		  a->event = a_event;
		  a->button = a_button;
		  a->key = a_key;
		  a->modifiers = a_modifiers;
		  a->action_proto = ap;
		  a->object = o;
		  a->started = 0;
		  current_actions = evas_list_append(current_actions, a);
	       }
	  }
	next:
	if (!a)
	  {
	     IF_FREE(a_name);
	     IF_FREE(a_action);
	     IF_FREE(a_params);
	     IF_FREE(a_key);
	  }
     }
   error:
   e_db_close(db);
}

static void
_e_action_free(E_Action *a)
{
   IF_FREE(a->name);
   IF_FREE(a->action);
   IF_FREE(a->params);
   IF_FREE(a->key);
   FREE(a);
}

void
e_action_start(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry)
{
   Evas_List l;
   
   _e_action_find(action, act, button, key, mods, o);
   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if (!a->started)
	  {
	     if (a->action_proto->func_stop)
	       a->started = 1;
	     if (a->action_proto->func_start)
	       {
		  E_Object *obj;
		  
		  if (a->object)
		    {
		       obj = a->object;
		       if (a->started)
			 OBJ_REF(obj);
		    }
		  a->action_proto->func_start(a->object, a, data, x, y, rx, ry);
	       }
	  }
	if (!a->started)
	  {
	     current_actions = evas_list_remove(current_actions, a);
	     OBJ_DO_FREE(a);
	     goto again;
	  }
     }
}

void
e_action_stop(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry)
{
   Evas_List l;

   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (a->action_proto->func_stop))
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
		  E_Object *obj;
		  
		  if (a->object)
		    {
		       obj = a->object;
		       OBJ_UNREF(obj);
		    }
		  a->action_proto->func_stop(a->object, a, data, x, y, rx, ry);
		  a->started = 0;
	       }
	  }
	if (!a->started)
	  {
	     current_actions = evas_list_remove(current_actions, a);
	     OBJ_DO_FREE(a);
	     goto again;
	  }
     }
   return;
   UN(action);
   UN(mods);
   UN(o);
}

void
e_action_go(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   Evas_List l;

   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (a->action_proto->func_go))
	  a->action_proto->func_go(a->object, a, data, x, y, rx, ry, dx, dy);
     }
   return;
   UN(action);
   UN(act);
   UN(button);
   UN(key);
   UN(mods);
   UN(o);
}

void
e_action_stop_by_object(void *o, void *data, int x, int y, int rx, int ry)
{
   Evas_List l;

   again:
   for (l = current_actions; l; l = l->next)
     {
	E_Action *a;
	
	a = l->data;
	if ((a->started) && (o == a->object))
	  {
	     E_Object *obj;
	     
	     if (a->object)
	       {
		  obj = a->object;
		  OBJ_UNREF(obj);
	       }
	     if (a->action_proto->func_stop)
	       a->action_proto->func_stop(a->object, a, data, x, y, rx, ry);
	     a->started = 0;
	     current_actions = evas_list_remove(current_actions, a);
	     OBJ_DO_FREE(a);
	     goto again;
	  }
     }
}

void
e_action_add_proto(char *action, 
		   void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
		   void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
		   void (*func_go)    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy))
{
   E_Action_Proto *ap;
   
   ap = NEW(E_Action_Proto, 1);
   
   OBJ_INIT(ap, NULL);
   
   ap->action = strdup(action);
   ap->func_start = func_start;
   ap->func_stop = func_stop;
   ap->func_go = func_go;
   action_protos = evas_list_append(action_protos, ap);
}

void
e_actions_init(void)
{
   e_action_add_proto("Window_Move", e_act_move_start, e_act_move_stop, e_act_move_go);
   e_action_add_proto("Window_Resize", e_act_resize_start, e_act_resize_stop, e_act_resize_go);
   e_action_add_proto("Window_Resize_Horizontal", e_act_resize_h_start, e_act_resize_h_stop, e_act_resize_h_go);
   e_action_add_proto("Window_Resize_Vertical", e_act_resize_v_start, e_act_resize_v_stop, e_act_resize_v_go);
   e_action_add_proto("Window_Close", e_act_close_start, NULL, NULL);
   e_action_add_proto("Window_Kill", e_act_kill_start, NULL, NULL);
   e_action_add_proto("Window_Shade", e_act_shade_start, NULL, NULL);
   e_action_add_proto("Window_Raise", e_act_raise_start, NULL, NULL);
   e_action_add_proto("Window_Lower", e_act_lower_start, NULL, NULL);
   e_action_add_proto("Window_Raise_Lower", e_act_raise_lower_start, NULL, NULL);
   e_action_add_proto("Execute", e_act_exec_start, NULL, NULL);
   e_action_add_proto("Menu", e_act_menu_start, NULL, NULL);
   e_action_add_proto("Exit", e_act_exit_start, NULL, NULL);
   e_action_add_proto("Restart", e_act_restart_start, NULL, NULL);
   e_action_add_proto("Window_Stick", e_act_stick_start, NULL, NULL);
   e_action_add_proto("Sound", e_act_sound_start, NULL, NULL);
   e_action_add_proto("Window_Iconify", e_act_iconify_start, NULL, NULL);
   e_action_add_proto("Window_Max_Size", e_act_max_start, NULL, NULL);
   e_action_add_proto("Winodw_Snap", e_act_snap_start, NULL, NULL);
   e_action_add_proto("Window_Zoom", e_act_zoom_start, NULL, NULL);
}








static void 
e_act_move_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   b->mode.move = 1;
   b->current.requested.dx = 0;
   b->current.requested.dy = 0;
   b->previous.requested.dx = 0;
   b->previous.requested.dy = 0;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_move_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->changed = 1;
   b->mode.move = 0;
   b->current.requested.dx = 0;
   b->current.requested.dy = 0;
   b->previous.requested.dx = 0;
   b->previous.requested.dy = 0;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_move_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   b = o;
   b->current.requested.x += dx;
   b->current.requested.y += dy;
   if (dx != 0) b->current.requested.dx = dx;
   if (dy != 0) b->current.requested.dy = dy;
   b->changed = 1;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_resize_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   /* 0 | 1 */
   /* --+-- */
   /* 2 | 3 */
   if (x > (b->current.w / 2)) 
     {
	if (y > (b->current.h / 2)) 
	  {
	     b->mode.resize = 3;
	     SET_BORDER_GRAVITY(b, NorthWestGravity);
	  }
	else 
	  {
	     b->mode.resize = 1;
	     SET_BORDER_GRAVITY(b, SouthWestGravity);
	  }
     }
   else
     {
	if (y > (b->current.h / 2)) 
	  {
	     b->mode.resize = 2;
	     SET_BORDER_GRAVITY(b, NorthEastGravity);
	  }
	else 
	  {
	     b->mode.resize = 0;
	     SET_BORDER_GRAVITY(b, SouthEastGravity);
	  }
     }
   return;
   UN(a);
   UN(data);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   b->changed = 1;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   b = o;
   if (b->mode.resize == 0)
     {
	b->current.requested.w -= dx;
	b->current.requested.h -= dy;
	b->current.requested.x += dx;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 1)
     {
	b->current.requested.w += dx;
	b->current.requested.h -= dy;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 2)
     {
	b->current.requested.w -= dx;
	b->current.requested.h += dy;
	b->current.requested.x += dx;
     }
   else if (b->mode.resize == 3)
     {
	b->current.requested.w += dx;
	b->current.requested.h += dy;
     }
   b->changed = 1;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_resize_h_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   /* 4 | 5 */
   if (x > (b->current.w / 2)) 
     {
	b->mode.resize = 5;
	SET_BORDER_GRAVITY(b, NorthWestGravity);
     }
   else 
     {
	b->mode.resize = 4;
	SET_BORDER_GRAVITY(b, NorthEastGravity);
     }
   return;
   UN(a);
   UN(data);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_h_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   b->changed = 1;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_h_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   b = o;
   if (b->mode.resize == 4)
     {
	b->current.requested.w -= dx;
	b->current.requested.x += dx;
     }
   else if (b->mode.resize == 5)
     {
	b->current.requested.w += dx;
     }
   b->changed = 1;
   e_border_adjust_limits(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
   UN(dy);
}


static void 
e_act_resize_v_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   /* 6 */
   /* - */
   /* 7 */
   if (y > (b->current.h / 2)) 
     {
	b->mode.resize = 7;
	SET_BORDER_GRAVITY(b, NorthWestGravity);
     }
   else 
     {
	b->mode.resize = 6;
	SET_BORDER_GRAVITY(b, SouthWestGravity);
     }
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_v_stop  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   b->current.requested.x = b->current.x;
   b->current.requested.y = b->current.y;
   b->current.requested.w = b->current.w;
   b->current.requested.h = b->current.h;
   b->mode.resize = 0;
   e_border_adjust_limits(b);
   b->changed = 1;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}

static void
e_act_resize_v_go    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy)
{
   E_Border *b;
   
   b = o;
   if (b->mode.resize == 6)
     {
	b->current.requested.h -= dy;
	b->current.requested.y += dy;
     }
   else if (b->mode.resize == 7)
     {
	b->current.requested.h += dy;
     }
   e_border_adjust_limits(b);
   b->changed = 1;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
   UN(dx);
}


static void 
e_act_close_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   if (b->win.client) e_icccm_delete(b->win.client);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_kill_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   if (b->win.client) e_window_kill_client(b->win.client);
   return;
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
   int pix_per_sec = 1600;
   
   b = data;
   if (val == 0) t = e_get_time();
   
   dif = e_get_time() - t;   
   
   si = (int)(dif * (double)pix_per_sec);
   if (si > b->client.h) si = b->client.h;
   b->current.shaded = si;
   b->changed = 1;
   e_border_adjust_limits(b);
   if (si < b->client.h) 
     e_add_event_timer("shader", 0.01, e_act_cb_shade, 1, data);
}

static void e_act_cb_unshade(int val, void *data);
static void
e_act_cb_unshade(int val, void *data)
{
   E_Border *b;
   static double t = 0.0;
   double dif;
   int si;
   int pix_per_sec = 1600;
   
   b = data;
   if (val == 0) t = e_get_time();
   
   dif = e_get_time() - t;   
   
   si = b->client.h - (int)(dif * (double)pix_per_sec);
   if (si < 0) si = 0;
   b->current.shaded = si;
   b->changed = 1;
   e_border_adjust_limits(b);
   if (si > 0) 
     e_add_event_timer("shader", 0.01, e_act_cb_unshade, 1, data);
}

static void 
e_act_shade_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   if (b->current.shaded == 0) e_act_cb_shade(0, b);
   else e_act_cb_unshade(0, b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_raise_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   e_border_raise(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   e_border_lower(b);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_raise_lower_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_exec_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_menu_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_exit_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   exit(0);
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_restart_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   e_exec_restart();
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_stick_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   if (b->client.sticky) b->client.sticky = 0;
   else b->client.sticky = 1;
   b->changed = 1;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_sound_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_iconify_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_max_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_snap_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


static void 
e_act_zoom_start (void *o, E_Action *a, void *data, int x, int y, int rx, int ry)
{
   E_Border *b;
   
   b = o;
   return;
   UN(a);
   UN(data);
   UN(x);
   UN(y);
   UN(rx);
   UN(ry);
}


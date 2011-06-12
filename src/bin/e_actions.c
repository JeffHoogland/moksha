#include "e.h"

#ifndef MAX
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define INITS
#define ACT_GO(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go = _e_actions_act_##name##_go; \
   }
#define ACT_FN_GO(act, use) \
   static void _e_actions_act_##act##_go(E_Object *obj __UNUSED__, const char *params use)

#define ACT_GO_MOUSE(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_mouse = _e_actions_act_##name##_go_mouse; \
   }
#define ACT_FN_GO_MOUSE(act, use) \
   static void _e_actions_act_##act##_go_mouse(E_Object *obj __UNUSED__, const char *params use, Ecore_Event_Mouse_Button *ev __UNUSED__)

#define ACT_GO_WHEEL(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_wheel = _e_actions_act_##name##_go_wheel; \
   }
#define ACT_FN_GO_WHEEL(act, use) \
   static void _e_actions_act_##act##_go_wheel(E_Object *obj __UNUSED__, const char *params use, Ecore_Event_Mouse_Wheel *ev __UNUSED__)

#define ACT_GO_EDGE(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_edge = _e_actions_act_##name##_go_edge; \
   }
#define ACT_FN_GO_EDGE(act, use) \
   static void _e_actions_act_##act##_go_edge(E_Object *obj __UNUSED__, const char *params use, E_Event_Zone_Edge *ev __UNUSED__)

#define ACT_GO_SIGNAL(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_signal = _e_actions_act_##name##_go_signal; \
   }
#define ACT_FN_GO_SIGNAL(act, use) \
   static void _e_actions_act_##act##_go_signal(E_Object *obj __UNUSED__, const char *params use, const char *sig, const char *src)

#define ACT_GO_KEY(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_key = _e_actions_act_##name##_go_key; \
   }
#define ACT_FN_GO_KEY(act, use) \
   static void _e_actions_act_##act##_go_key(E_Object *obj __UNUSED__, const char *params use, Ecore_Event_Key *ev __UNUSED__)

#define ACT_END(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end = _e_actions_act_##name##_end; \
   }
#define ACT_FN_END(act, use) \
   static void _e_actions_act_##act##_end(E_Object *obj __UNUSED__, const char *params use)

#define ACT_END_MOUSE(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end_mouse = _e_actions_act_##name##_end_mouse; \
   }
#define ACT_FN_END_MOUSE(act, use) \
   static void _e_actions_act_##act##_end_mouse(E_Object *obj __UNUSED__, const char *params use, Ecore_Event_Mouse_Button *ev __UNUSED__)

#define ACT_END_KEY(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end_key = _e_actions_act_##name##_end_key; \
   }
#define ACT_FN_END_KEY(act, use) \
   static void _e_actions_act_##act##_end_key(E_Object *obj __UNUSED__, const char *params use, Ecore_Event_Key *ev __UNUSED__)

#define ACT_GO_ACPI(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_acpi = _e_actions_act_##name##_go_acpi; \
   }
#define ACT_FN_GO_ACPI(act, use) \
   static void _e_actions_act_##act##_go_acpi(E_Object *obj __UNUSED__, const char *params use, E_Event_Acpi *ev __UNUSED__)

/* local subsystem functions */
static void _e_action_free(E_Action *act);
static E_Maximize _e_actions_maximize_parse(const char *maximize);
static int _action_groups_sort_cb(const void *d1, const void *d2);

/* to save writing this in N places - the sections are defined here */
/***************************************************************************/
ACT_FN_GO(window_move, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_location)
     e_border_act_move_begin((E_Border *)obj, NULL);
}

ACT_FN_GO_MOUSE(window_move, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_begin((E_Border *)obj, ev);
}

ACT_FN_GO_SIGNAL(window_move, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_location)
     {
	if ((params) && (!strcmp(params, "end")))
          e_border_signal_move_end((E_Border *)obj, sig, src);
	else
	  {
	     if (((E_Border *)obj)->moving)
	       e_border_signal_move_end((E_Border *)obj, sig, src);
	     else
	       e_border_signal_move_begin((E_Border *)obj, sig, src);
	  }
     }
}

ACT_FN_END(window_move, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, NULL);
}

ACT_FN_END_MOUSE(window_move, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, ev);
}

ACT_FN_GO_KEY(window_move, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_location)
     e_border_act_move_keyboard((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_resize, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_size)
     e_border_act_resize_begin((E_Border *)obj, NULL);
}

ACT_FN_GO_MOUSE(window_resize, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_size)
     e_border_act_resize_begin((E_Border *)obj, ev);
}

ACT_FN_GO_SIGNAL(window_resize, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_size)
     {
	if ((params) && (!strcmp(params, "end")))
	  e_border_signal_resize_end((E_Border *)obj, params, sig, src);
	else
	  {
	     if (!params) params = "";
	     if (e_border_resizing_get((E_Border *)obj))
	       e_border_signal_resize_end((E_Border *)obj, params, sig, src);
	     else
	       e_border_signal_resize_begin((E_Border *)obj, params, sig, src);
	  }
     }
}

ACT_FN_END(window_resize, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, NULL);
}

ACT_FN_END_MOUSE(window_resize, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, ev);
}

ACT_FN_GO_KEY(window_resize, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_size)
     e_border_act_resize_keyboard((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_menu, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, NULL, 0);
}

ACT_FN_GO_MOUSE(window_menu, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, ev, 0);
}

ACT_FN_GO_KEY(window_menu, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, NULL, 1);
}

/***************************************************************************/
ACT_FN_GO(window_raise, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_stacking)
     e_border_raise((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_lower, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_stacking)
     e_border_lower((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_close, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_close)
     e_border_act_close_begin((E_Border *)obj);
}

/***************************************************************************/
static E_Dialog *kill_dialog = NULL;

static void
_e_actions_cb_kill_dialog_ok(void *data, E_Dialog *dia)
{
   E_Object *obj;

   obj = data;
   if (dia)
     {
	e_object_del(E_OBJECT(kill_dialog));
	kill_dialog = NULL;
     }
   if ((!((E_Border *)obj)->lock_close) && (!((E_Border *)obj)->internal))
     e_border_act_kill_begin((E_Border *)obj);
}

static void
_e_actions_cb_kill_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(kill_dialog));
   kill_dialog = NULL;
}

static void
_e_actions_cb_kill_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_kill_dialog_cancel(NULL, dia);
}

ACT_FN_GO(window_kill, __UNUSED__)
{
   E_Border *bd;
   char dialog_text[1024];

   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   bd = (E_Border *)obj;
   if ((bd->lock_close) || (bd->internal)) return;

   if (kill_dialog) e_object_del(E_OBJECT(kill_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_kill_dialog_ok(obj, NULL);
	return;
     }

   snprintf(dialog_text, sizeof(dialog_text),
	    _("You are about to kill %s.<br><br>"
	    "Please keep in mind that all data from this window<br>"
	    "which has not yet been saved will be lost!<br><br>"
	    "Are you sure you want to kill this window?"),
	    bd->client.icccm.name);

   kill_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()),
			      "E", "_kill_dialog");
   if (!kill_dialog) return;
   e_win_delete_callback_set(kill_dialog->win,
			     _e_actions_cb_kill_dialog_delete);
   e_dialog_title_set(kill_dialog,
		      _("Are you sure you want to kill this window?"));
   e_dialog_text_set(kill_dialog, _(dialog_text));
   e_dialog_icon_set(kill_dialog, "application-exit", 64);
   e_dialog_button_add(kill_dialog, _("Yes"), NULL,
		       _e_actions_cb_kill_dialog_ok, obj);
   e_dialog_button_add(kill_dialog, _("No"), NULL,
		       _e_actions_cb_kill_dialog_cancel, NULL);
   e_dialog_button_focus_num(kill_dialog, 1);
   e_win_centered_set(kill_dialog->win, 1);
   e_dialog_show(kill_dialog);
}

/***************************************************************************/
ACT_FN_GO(window_sticky_toggle, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_sticky)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (bd->sticky) e_border_unstick(bd);
	else e_border_stick(bd);
     }
}

/***************************************************************************/
ACT_FN_GO(window_sticky, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_sticky)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (params)
	  {
	     if (atoi(params) == 1)
	       e_border_stick(bd);  	
	     else if (atoi(params) == 0)
	       e_border_unstick(bd);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_iconic_toggle, __UNUSED__)
{
   E_Border *bd;

   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   bd = (E_Border *)obj;

   if ((!bd->lock_user_iconify) && (!bd->fullscreen) &&
       ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) ||
        (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UNKNOWN)))
     {
	if (bd->iconic) e_border_uniconify(bd);
	else e_border_iconify(bd);
     }
}

/***************************************************************************/
ACT_FN_GO(window_iconic, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_iconify)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (params)
	  {
	     if (atoi(params) == 1)
	       e_border_iconify(bd);
	     else if (atoi(params) == 0)
	       e_border_uniconify(bd);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_fullscreen_toggle, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_fullscreen)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (bd->fullscreen)
	  e_border_unfullscreen(bd);
	else if (!params || *params == '\0')
	  e_border_fullscreen(bd, e_config->fullscreen_policy);
	else if (! strcmp(params, "resize"))
	  e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
	else if (! strcmp(params, "zoom"))
	  e_border_fullscreen(bd, E_FULLSCREEN_ZOOM);
     }
}

/***************************************************************************/
ACT_FN_GO(window_fullscreen, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_fullscreen)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (params)
	  {
	     int v;
	     char buf[32];

	     buf[0] = 0;
	     if (sscanf(params, "%i %20s", &v, buf) == 2)
	       {
		  if (v == 1)
		    {
		      if (*buf == '\0')
			e_border_fullscreen(bd, e_config->fullscreen_policy);
		      else if (!strcmp(buf, "resize"))
			e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
		      else if (!strcmp(buf, "zoom"))
			e_border_fullscreen(bd, E_FULLSCREEN_ZOOM);
		    }
		  else if (v == 0)
		    e_border_unfullscreen(bd);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_maximized_toggle, )
{
   E_Border *bd;

   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   bd = (E_Border *)obj;

   if ((!bd->lock_user_maximize) && (!bd->fullscreen) &&
       ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) ||
        (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UNKNOWN)))
     {
	if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
	  {
	     if (!params)
	       e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
	     else
	       {
		  E_Maximize max;

		  max = _e_actions_maximize_parse(params);
		  max &= E_MAXIMIZE_DIRECTION;
		  if (max == E_MAXIMIZE_VERTICAL)
		    {
		       if (bd->maximized & E_MAXIMIZE_VERTICAL)
			 e_border_unmaximize(bd, E_MAXIMIZE_VERTICAL);
		       else
			 goto maximize;
		    }
		  else if (max == E_MAXIMIZE_HORIZONTAL)
		    {
		       if (bd->maximized & E_MAXIMIZE_HORIZONTAL)
			 e_border_unmaximize(bd, E_MAXIMIZE_HORIZONTAL);
		       else
			 goto maximize;
		    }
		  else
		    e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
	       }
	  }
	else
	  {
maximize:
	     e_border_maximize(bd, _e_actions_maximize_parse(params));
	  }
     }
}
/***************************************************************************/
ACT_FN_GO(window_maximized, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_maximize)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (params)
	  {
	     E_Maximize max;
	     int v, ret;
	     char s1[32], s2[32];

	     max = (e_config->maximize_policy & E_MAXIMIZE_DIRECTION);
	     ret = sscanf(params, "%i %20s %20s", &v, s1, s2);
	     if (ret == 3)
	       {
		  if (!strcmp(s2, "horizontal"))
		    max = E_MAXIMIZE_HORIZONTAL;
		  else if (!strcmp(s2, "vertical"))
		    max = E_MAXIMIZE_VERTICAL;
		  else
		    max = E_MAXIMIZE_BOTH;
	       }
	     if (ret > 1)
	       {
		  if (v == 1)
		    {
		       if (!strcmp(s1, "fullscreen"))
                         e_border_maximize(bd, E_MAXIMIZE_FULLSCREEN | max);
		       else if (!strcmp(s1, "smart"))
                         e_border_maximize(bd, E_MAXIMIZE_SMART | max);
		       else if (!strcmp(s1, "expand"))
                         e_border_maximize(bd, E_MAXIMIZE_EXPAND | max);
		       else if (!strcmp(s1, "fill"))
                         e_border_maximize(bd, E_MAXIMIZE_FILL | max);
		       else
                         e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) | max);
		    }
		  else if (v == 0)
		    e_border_unmaximize(bd, max);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_shaded_toggle, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_shade)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (bd->shaded)
	  {
	     if (!params)
	       e_border_unshade(bd, E_DIRECTION_UP);
	     else
	       {
		  if (!strcmp(params, "up"))
                    e_border_unshade(bd, E_DIRECTION_UP);
		  else if (!strcmp(params, "down"))
                    e_border_unshade(bd, E_DIRECTION_DOWN);
		  else if (!strcmp(params, "left"))
                    e_border_unshade(bd, E_DIRECTION_LEFT);
		  else if (!strcmp(params, "right"))
                    e_border_unshade(bd, E_DIRECTION_RIGHT);
		  else
                    e_border_unshade(bd, E_DIRECTION_UP);
	       }
	  }
	else
	  {
	     if (!params)
	       e_border_shade(bd, E_DIRECTION_UP);
	     else
	       {
		  if (!strcmp(params, "up"))
                    e_border_shade(bd, E_DIRECTION_UP);
		  else if (!strcmp(params, "down"))
                    e_border_shade(bd, E_DIRECTION_DOWN);
		  else if (!strcmp(params, "left"))
                    e_border_shade(bd, E_DIRECTION_LEFT);
		  else if (!strcmp(params, "right"))
                    e_border_shade(bd, E_DIRECTION_RIGHT);
		  else
                    e_border_shade(bd, E_DIRECTION_UP);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_shaded, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_shade)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if (params)
	  {
	     int v;
	     char buf[32];

	     if (sscanf(params, "%i %20s", &v, buf) == 2)
	       {
		  if (v == 1)
		    {
		       if (!strcmp(buf, "up"))
			 e_border_shade(bd, E_DIRECTION_UP);
		       else if (!strcmp(buf, "down"))
			 e_border_shade(bd, E_DIRECTION_DOWN);
		       else if (!strcmp(buf, "left"))
			 e_border_shade(bd, E_DIRECTION_LEFT);
		       else if (!strcmp(buf, "right"))
			 e_border_shade(bd, E_DIRECTION_RIGHT);
		    }
		  else if (v == 0)
		    {
		       if (!strcmp(buf, "up"))
			 e_border_unshade(bd, E_DIRECTION_UP);
		       else if (!strcmp(buf, "down"))
			 e_border_unshade(bd, E_DIRECTION_DOWN);
		       else if (!strcmp(buf, "left"))
			 e_border_unshade(bd, E_DIRECTION_LEFT);
		       else if (!strcmp(buf, "right"))
			 e_border_unshade(bd, E_DIRECTION_RIGHT);
		    }
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_borderless_toggle, __UNUSED__)
{
   if ((!obj) || (obj->type != E_BORDER_TYPE))
     obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (!((E_Border *)obj)->lock_border)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	bd->borderless = !bd->borderless;

	bd->client.border.changed = 1;
	bd->changed = 1;
     }
}

/***************************************************************************/
ACT_FN_GO(window_border_set, __UNUSED__)
{
   if ((!obj) || (obj->type != E_BORDER_TYPE))
     obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (!((E_Border *)obj)->lock_border)
     {
        E_Border *bd;

        bd = (E_Border *)obj;
        if (bd && params)
          {
             eina_stringshare_replace(&bd->bordername, params);
             bd->client.border.changed = 1;
             bd->changed = 1;
          }
     }
}

/***************************************************************************/
ACT_FN_GO(window_border_cycle, __UNUSED__)
{
   if ((!obj) || (obj->type != E_BORDER_TYPE))
     obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (!((E_Border *)obj)->lock_border)
     {
        E_Border *bd;

        bd = (E_Border *)obj;
        if (bd && params)
          {
             const char *space;

             while (*params == ' ')
                params++;

             if (bd->bordername)
               {
                  const char *bdname = params;

                  while (bdname && (space = strchr(bdname, ' ')))
                    {
                       if (strncmp(bd->bordername, bdname, space - bdname) == 0)
                         {
                            bdname = space + 1;
                            while (*bdname == ' ')
                               bdname++;
                            space = strchr(bdname, ' ');
                            if (space)
                              eina_stringshare_replace_length(
                                 &bd->bordername,
                                 bdname, space - bdname);
                            else
                              eina_stringshare_replace(&bd->bordername, bdname);
                            bd->client.border.changed = 1;
                            bd->changed = 1;
                            return;
                         }
                       bdname = space + 1;
                       while (*bdname == ' ')
                          bdname++;
                    }
               }

             space = strchr(params, ' ');
             if (space)
               eina_stringshare_replace_length(&bd->bordername,
                                               params, space - params);
             else
               eina_stringshare_replace(&bd->bordername, params);
             bd->client.border.changed = 1;
             bd->changed = 1;
          }
     }
}

 /***************************************************************************/
ACT_FN_GO(window_pinned_toggle, __UNUSED__)
{
   if ((!obj) || (obj->type != E_BORDER_TYPE))
     obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (!((E_Border *)obj)->lock_border)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if ((bd->client.netwm.state.stacking == E_STACKING_BELOW) &&
            (bd->user_skip_winlist) && (bd->borderless))
	  e_border_pinned_set(bd, 0);
	else
	  e_border_pinned_set(bd, 1);
     }
}

/***************************************************************************/
ACT_FN_GO(window_move_by, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
   if (params)
     {
	int dx, dy;

	if (sscanf(params, "%i %i", &dx, &dy) == 2)
	  {
	     E_Border *bd;

	     bd = (E_Border *)obj;
	     e_border_move(bd, bd->x + dx, bd->y + dy);

	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (bd->w / 2),
				    bd->y + (bd->h / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_move_to, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
   if (params)
     {
	E_Border *bd;
	int x, y, zx, zy, zw, zh;
	char cx, cy;

	bd = (E_Border *)obj;
	e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

	if (sscanf(params, "%c%i %c%i", &cx, &x, &cy, &y) == 4)
	  {
	     x += zx;
	     y += zy;
	  }
	else if (sscanf(params, "* %c%i", &cy, &y) == 2)
	  {
	     /* Updated y, keep x. */
	     y += zy;
	     x = bd->x;
	     cx = 0;
	  }
	else if (sscanf(params, "%c%i *", &cx, &x) == 2)
	  {
	     /* Updated x, keep y. */
	     x += zx;
	     y = bd->y;
	     cy = 0;
	  }
	else return;

	if (cx == '-') x = zw - bd->w - x + 2 * zx; /* compensate x with zx */
	if (cy == '-') y = zh - bd->h - y + 2 * zy; /* compensate y with zy */

	if ((x != bd->x) || (y != bd->y))
	  {
	     e_border_move(bd, x, y);

	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (bd->w / 2),
				    bd->y + (bd->h / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_move_to_center, __UNUSED__)
{
   E_Border *bd;
   int x, y;

   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   bd = (E_Border *)obj;
   e_border_center_pos_get(bd, &x, &y);

   if ((x != bd->x) || (y != bd->y))
     {
        e_border_move(bd, x, y);

        if (e_config->focus_policy != E_FOCUS_CLICK)
          ecore_x_pointer_warp(bd->zone->container->win,
                               x + (bd->w / 2),
                               y + (bd->h / 2));
     }
}

/***************************************************************************/
ACT_FN_GO(window_resize_by, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   if (params)
     {
	int dw, dh;

	if (sscanf(params, "%i %i", &dw, &dh) == 2)
	  {
	     E_Border *bd;

	     bd = (E_Border *)obj;

	     dw += bd->w;
	     dh += bd->h;
	     e_border_resize_limit(bd, &dw, &dh);
	     e_border_resize(bd, dw, dh);

	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (dw / 2), bd->y + (dh / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_push, )
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   if (params)
     {
        E_Border *bd, *cur;
        E_Border_List *bd_list;
        int hdir, vdir;
        int x, y, zx, zy, zw, zh;

        if (strcmp(params, "left") == 0)
	  {
	     hdir = -1;
	     vdir = 0;
	  }
        else if (strcmp(params, "right") == 0)
	  {
	     hdir = +1;
	     vdir = 0;
	  }
        else if (strcmp(params, "up") == 0)
	  {
	     hdir = 0;
	     vdir = -1;
	  }
        else if (strcmp(params, "down") == 0)
	  {
	     hdir = 0;
	     vdir = +1;
	  }
        else if (strcmp(params, "up-left") == 0)
	  {
	     hdir = -1;
	     vdir = -1;
	  }
        else if (strcmp(params, "up-right") == 0)
	  {
	     hdir = +1;
	     vdir = -1;
	  }
        else if (strcmp(params, "down-left") == 0)
	  {
	     hdir = -1;
	     vdir = +1;
	  }
        else if (strcmp(params, "down-right") == 0)
	  {
	     hdir = +1;
	     vdir = +1;
	  }
        else
          return;

        bd = (E_Border *)obj;
        e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

	if (hdir < 0)      x = zx;
	else if (hdir > 0) x = zx + zw - bd->w;
	else               x = bd->x;

	if (vdir < 0)      y = zy;
	else if (vdir > 0) y = zy + zh - bd->h;
	else               y = bd->y;

        bd_list = e_container_border_list_first(bd->zone->container);
        cur = e_container_border_list_next(bd_list);

        while (cur)
          {
            if ((bd->desk == cur->desk) && (bd != cur) && (!cur->iconic))
               {
		  if ((hdir < 0)
		      && (cur->x + cur->w < bd->x)
		      && (E_SPANS_COMMON(bd->y, bd->h, cur->y, cur->h)))
		    x = MAX(x, cur->x + cur->w);
		  else if ((hdir > 0)
			   && (cur->x > bd->x + bd->w)
			   && (E_SPANS_COMMON(bd->y, bd->h, cur->y, cur->h)))
		    x = MIN(x, zx + cur->x - bd->w);

		  if ((vdir < 0)
		      && (cur->y + cur->h < bd->y)
		      && (E_SPANS_COMMON(bd->x, bd->w, cur->x, cur->w)))
		    y = MAX(y, cur->y + cur->h);
		  else if ((vdir > 0)
			   && (cur->y > bd->y + bd->h)
			   && (E_SPANS_COMMON(bd->x, bd->w, cur->x, cur->w)))
		    y = MIN(y, zy + cur->y - bd->h);
               }
             cur = e_container_border_list_next(bd_list);
          }
        e_container_border_list_free(bd_list);

	if ((x != bd->x) || (y != bd->y))
	  {
             e_border_move(bd, x, y);
	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (bd->w / 2), bd->y + (bd->h / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_drag_icon, __UNUSED__)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	bd->drag.start = 1;
	bd->drag.x = -1;
	bd->drag.y = -1;
     }
}

/***************************************************************************/
ACT_FN_GO(window_desk_move_by, )
{
   E_Border *bd;
   int x, y;

   if (!params) return;
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   bd = (E_Border *)obj;
   if ((!bd->zone) || (!bd->desk)) return;
   if (sscanf(params, "%d %d", &x, &y) == 2)
     {
	E_Desk *desk;
	int dx, dy;
	int to_x = 0, to_y = 0;

	e_desk_xy_get(bd->desk, &dx, &dy);

	to_x = dx + x;
	to_y = dy + y;
	while (!(desk = e_desk_at_xy_get(bd->zone, to_x, to_y)))
	  {
	     /* here we are out of our desktop range */
	     while (to_x >= bd->zone->desk_x_count)
	       {
		  to_x -= bd->zone->desk_x_count;
		  to_y++;
	       }
	     while (to_x < 0)
	       {
		  to_x += bd->zone->desk_x_count;
		  to_y--;
	       }

	     while (to_y >= bd->zone->desk_y_count)
	       to_y -= bd->zone->desk_y_count;
	     while (to_y < 0)
	       to_y += bd->zone->desk_y_count;
	  }
	
	if (desk)
	  {
	     /* switch desktop. Quite useful from the interface point of view. */
	     e_zone_desk_flip_by(bd->zone, to_x - dx, to_y - dy);
	     /* send the border to the required desktop. */
	     e_border_desk_set(bd, desk);
	     if (!bd->lock_user_stacking)
	       e_border_raise(bd);
	     e_border_focus_set(bd, 1, 1);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_desk_move_to, )
{
   E_Border *bd;
   int x, y;

   if (!params) return;
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   bd = (E_Border *)obj;
   if ((!bd->zone) || (!bd->desk)) return;
   if (sscanf(params, "%d %d", &x, &y) == 2)
     {
	E_Desk *desk;

	desk = e_desk_at_xy_get(bd->zone, x, y);
	if (desk) e_border_desk_set(bd, desk);
     }
}

/***************************************************************************/
static E_Zone *
_e_actions_zone_get(E_Object *obj)
{
   if (obj)
     {
	if      (obj->type == (int)E_MANAGER_TYPE)   return e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == (int)E_CONTAINER_TYPE) return e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == (int)E_ZONE_TYPE)      return (E_Zone *)obj;
	else if (obj->type == (int)E_BORDER_TYPE)    return ((E_Border *)obj)->zone;
	else if (obj->type == (int)E_SHELF_TYPE)     return ((E_Shelf *)obj)->zone;
	else if (obj->type == (int)E_POPUP_TYPE)     return ((E_Popup *)obj)->zone;
	else if (obj->type == (int)E_WIN_TYPE)       return ((E_Win *)obj)->border->zone;
     }
   return e_util_zone_current_get(e_manager_current_get());
}

ACT_FN_GO(desk_flip_by, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0, dy = 0;

	     if (sscanf(params, "%i %i", &dx, &dy) == 2)
	       e_zone_desk_flip_by(zone, dx, dy);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_flip_to, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0, dy = 0;

	     if (sscanf(params, "%i %i", &dx, &dy) == 2)
	       e_zone_desk_flip_to(zone, dx, dy);
	  }
     }
}

/***************************************************************************/
#define ACT_FLIP_LEFT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || ((zone)->desk_x_current > 0))
#define ACT_FLIP_RIGHT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || (((zone)->desk_x_current + 1) < (zone)->desk_x_count))
#define ACT_FLIP_UP(zone) ((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || ((zone)->desk_y_current > 0))
#define ACT_FLIP_DOWN(zone) ((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || (((zone)->desk_y_current + 1) < (zone)->desk_y_count))
#define ACT_FLIP_UP_LEFT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1) && ((zone)->desk_y_count > 1)) || (((zone)->desk_x_current > 0) && ((zone)->desk_y_current > 0)))
#define ACT_FLIP_UP_RIGHT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1) && ((zone)->desk_y_count > 1)) || ((((zone)->desk_x_current + 1) < (zone)->desk_x_count) && ((zone)->desk_y_current > 0)))
#define ACT_FLIP_DOWN_RIGHT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1) && ((zone)->desk_y_count > 1)) || ((((zone)->desk_x_current + 1) < (zone)->desk_x_count) && (((zone)->desk_y_current + 1) < (zone)->desk_y_count)))
#define ACT_FLIP_DOWN_LEFT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1) && ((zone)->desk_y_count > 1)) || (((zone)->desk_x_current > 0) && (((zone)->desk_y_current + 1) < (zone)->desk_y_count)))

ACT_FN_GO_EDGE(desk_flip_in_direction, )
{
   E_Zone *zone;
   E_Desk *current = NULL;
   E_Event_Pointer_Warp *wev;
   int x, y, offset = 25;

   if (!ev) return; // with flip on _e_zone_cb_edge_timer we don't have ev!!!
   zone = _e_actions_zone_get(obj);
   wev = E_NEW(E_Event_Pointer_Warp, 1);
   if ((!wev) || (!zone)) return;
   ecore_x_pointer_xy_get(zone->container->win, &x, &y);
   wev->prev.x = x;
   wev->prev.y = y;
   if (params)
     {
	if (sscanf(params, "%i", &offset) != 1)
	  offset = 25;
     }
   switch (ev->edge)
     {
     case E_ZONE_EDGE_LEFT:
	if (ACT_FLIP_LEFT(zone))
	  {
	     e_zone_desk_flip_by(zone, -1, 0);
	     ecore_x_pointer_warp(zone->container->win, zone->w - offset, y);
	     wev->curr.y = y;
	     wev->curr.x = zone->w - offset;
	  }
        break;
     case E_ZONE_EDGE_RIGHT:
        if (ACT_FLIP_RIGHT(zone))
          {
             e_zone_desk_flip_by(zone, 1, 0);
             ecore_x_pointer_warp(zone->container->win, offset, y);
             wev->curr.y = y;
             wev->curr.x = offset;
          }
        break;
     case E_ZONE_EDGE_TOP:
        if (ACT_FLIP_UP(zone))
          {
             e_zone_desk_flip_by(zone, 0, -1);
             ecore_x_pointer_warp(zone->container->win, x, zone->h - offset);
             wev->curr.x = x;
             wev->curr.y = zone->h - offset;
          }
        break;
     case E_ZONE_EDGE_BOTTOM:
	if (ACT_FLIP_DOWN(zone))
	  {
	     e_zone_desk_flip_by(zone, 0, 1);
	     ecore_x_pointer_warp(zone->container->win, x, offset);
	     wev->curr.x = x;
	     wev->curr.y = offset;
	  }
        break;
     case E_ZONE_EDGE_TOP_LEFT:
        if (ACT_FLIP_UP_LEFT(zone))
          {
             e_zone_desk_flip_by(zone, -1, -1);
             ecore_x_pointer_warp(zone->container->win, zone->w - offset, zone->h - offset);
             wev->curr.x = zone->w - offset;
             wev->curr.y = zone->h - offset;
          }
        break;
     case E_ZONE_EDGE_TOP_RIGHT:
        if (ACT_FLIP_UP_RIGHT(zone))
          {
             e_zone_desk_flip_by(zone, 1, -1);
             ecore_x_pointer_warp(zone->container->win, offset, zone->h - offset);
             wev->curr.x = offset;
             wev->curr.y = zone->h - offset;
          }
        break;
     case E_ZONE_EDGE_BOTTOM_LEFT:
	if (ACT_FLIP_DOWN_LEFT(zone))
	  {
	     e_zone_desk_flip_by(zone, -1, 1);
	     ecore_x_pointer_warp(zone->container->win, zone->w - offset, offset);
	     wev->curr.y = offset;
	     wev->curr.x = zone->w - offset;
	  }
        break;
     case E_ZONE_EDGE_BOTTOM_RIGHT:
	if (ACT_FLIP_DOWN_RIGHT(zone))
	  {
	     e_zone_desk_flip_by(zone, 1, 1);
	     ecore_x_pointer_warp(zone->container->win, offset, offset);
	     wev->curr.y = offset;
	     wev->curr.x = offset;
	  }
        break;
     default:
        break;
     }

   current = e_desk_current_get(zone);
   if (current)
     ecore_event_add(E_EVENT_POINTER_WARP, wev, NULL, NULL);
   else
     free(wev);
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_by, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0;

	     if (sscanf(params, "%i", &dx) == 1)
	       e_zone_desk_linear_flip_by(zone, dx);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_to, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0;

	     if (sscanf(params, "%i", &dx) == 1)
	       e_zone_desk_linear_flip_to(zone, dx);
	  }
     }
}


#define DESK_ACTION_ALL(zone, act) \
  E_Zone *zone;			   \
  Eina_List *lm, *lc, *lz;	   \
  E_Container *con;		   \
  E_Manager *man;				\
  \
  EINA_LIST_FOREACH(e_manager_list(), lm, man) {	   \
    EINA_LIST_FOREACH(man->containers, lc, con) {	   \
      EINA_LIST_FOREACH(con->zones, lz, zone) {		   \
	act;						   \
      }							   \
    }							   \
  }


/***************************************************************************/
ACT_FN_GO(desk_flip_by_all, )
{
   if (params)
     {
        int dx = 0, dy = 0;

        if (sscanf(params, "%i %i", &dx, &dy) == 2)
          {
             DESK_ACTION_ALL(zone, e_zone_desk_flip_by(zone, dx, dy));
          }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_flip_to_all, )
{
   if (params)
     {
        int dx = 0, dy = 0;

        if (sscanf(params, "%i %i", &dx, &dy) == 2)
          {
             DESK_ACTION_ALL(zone, e_zone_desk_flip_to(zone, dx, dy));
          }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_by_all, )
{
   if (params)
     {
        int dx = 0;

        if (sscanf(params, "%i", &dx) == 1)
          {
             DESK_ACTION_ALL(zone, e_zone_desk_linear_flip_by(zone, dx));
          }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_to_all, )
{
   if (params)
     {
        int dx = 0;

        if (sscanf(params, "%i", &dx) == 1)
          {
             DESK_ACTION_ALL(zone, e_zone_desk_linear_flip_to(zone, dx));
          }
     }
}

/***************************************************************************/
ACT_FN_GO(screen_send_to, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params)
	  {
	     int scr = 0;

	     if (sscanf(params, "%i", &scr) == 1)
	       {
		  E_Zone *zone2 = NULL;

                  if (eina_list_count(e_manager_list()) > 1)
		    {
		       scr = scr % eina_list_count(e_manager_list());
		       if (scr < 0) scr += eina_list_count(e_manager_list());
		       zone2 = e_util_container_zone_number_get(scr, 0);
		    }
		  else
		    {
		       scr = scr % eina_list_count(zone->container->zones);
		       if (scr < 0) scr += eina_list_count(zone->container->zones);
		       zone2 = e_util_container_zone_number_get(0, scr);
		    }
		  if ((zone2) && (zone != zone2))
		    ecore_x_pointer_warp(zone2->container->win,
					 zone2->x + (zone2->w / 2),
					 zone2->y + (zone2->h / 2));
	       }
	  }
     }
}

ACT_FN_GO(screen_send_by, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
	if (params)
	  {
	     int scr = 0;

	     if (sscanf(params, "%i", &scr) == 1)
	       {
		  E_Zone *zone2 = NULL;

                  if (eina_list_count(e_manager_list()) > 1)
		    {
		       scr += zone->container->num;
		       scr = scr % eina_list_count(e_manager_list());
		       if (scr < 0) scr += eina_list_count(e_manager_list());
		       zone2 = e_util_container_zone_number_get(scr, 0);
		    }
		  else
		    {
                       scr += zone->num;
		       scr = scr % eina_list_count(zone->container->zones);
		       if (scr < 0) scr += eina_list_count(zone->container->zones);
		       zone2 = e_util_container_zone_number_get(0, scr);
		    }
		  if ((zone2) && (zone != zone2))
		    ecore_x_pointer_warp(zone2->container->win,
					 zone2->x + (zone2->w / 2),
					 zone2->y + (zone2->h / 2));
	       }
	  }
     }
}

#define ZONE_DESK_ACTION(con_num, zone_num, zone, act) \
   E_Zone *zone; \
   if ((con_num < 0) || (zone_num < 0)) { \
      Eina_List *l, *ll, *lll; \
      E_Container *con; \
      E_Manager *man; \
      if ((con_num >= 0) && (zone_num < 0)) /* con=1 zone=all */ { \
         con = e_util_container_number_get(con_num); \
         EINA_LIST_FOREACH(con->zones, l, zone) { \
            act; \
         } } \
      else if ((con_num < 0) && (zone_num >= 0)) /* con=all zone=1 */ { \
         EINA_LIST_FOREACH(e_manager_list(), l, man) { \
            EINA_LIST_FOREACH(man->containers, ll, con) { \
               zone = e_container_zone_number_get(con, zone_num); \
               if (zone) \
                 act; \
            } } } \
      else if ((con_num < 0) && (zone_num < 0)) /* con=all zone=all */ { \
         EINA_LIST_FOREACH(e_manager_list(), l, man) { \
            EINA_LIST_FOREACH(man->containers, ll, con) { \
               EINA_LIST_FOREACH(con->zones, lll, zone) { \
                  act; \
               } } } } } \
   else { \
      zone = e_util_container_zone_number_get(con_num, zone_num); \
      if (zone) act; \
   }

/***************************************************************************/
#if 0
ACT_FN_GO(zone_desk_flip_by, )
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0, dy = 0;

	if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &dx, &dy) == 4)
          ZONE_DESK_ACTION(con_num, zone_num, zone,
                           e_zone_desk_flip_by(zone, dx, dy));
     }
}
#endif

/***************************************************************************/
#if 0
ACT_FN_GO(zone_desk_flip_to, )
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0, dy = 0;

	if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &dx, &dy) == 4)
          ZONE_DESK_ACTION(con_num, zone_num, zone,
                           e_zone_desk_flip_to(zone, dx, dy));
     }
}
#endif

/***************************************************************************/
#if 0
ACT_FN_GO(zone_desk_linear_flip_by, )
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0;

	if (sscanf(params, "%i %i %i", &con_num, &zone_num, &dx) == 3)
          ZONE_DESK_ACTION(con_num, zone_num, zone,
                           e_zone_desk_linear_flip_by(zone, dx));
     }
}
#endif

/***************************************************************************/
#if 0
ACT_FN_GO(zone_desk_linear_flip_to, )
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0;

	if (sscanf(params, "%i %i %i", &con_num, &zone_num, &dx) == 3)
          ZONE_DESK_ACTION(con_num, zone_num, zone,
                           e_zone_desk_linear_flip_to(zone, dx));
     }
}
#endif

/***************************************************************************/
static void
_e_actions_cb_menu_end(void *data __UNUSED__, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static E_Menu *
_e_actions_menu_find(const char *name)
{
   if (!strcmp(name, "main"))
     return e_int_menus_main_new();
   else if (!strcmp(name, "favorites"))
     return e_int_menus_favorite_apps_new();
   else if (!strcmp(name, "all"))
     return e_int_menus_all_apps_new();
   else if (!strcmp(name, "clients"))
     return e_int_menus_clients_new();
   else if (!strcmp(name, "lost_clients"))
     return e_int_menus_lost_clients_new();
   else if (!strcmp(name, "configuration"))
     return e_int_menus_config_new();
   return NULL;
}

ACT_FN_GO(menu_show, )
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;

	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;

		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  ecore_x_pointer_xy_get(zone->container->win, &x, &y);
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_mouse(m, zone, x, y, 1, 1,
					E_MENU_POP_DIRECTION_DOWN,
					ecore_x_current_time_get());
	       }
	  }
     }
}

ACT_FN_GO_MOUSE(menu_show, )
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;

	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;

		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  x = ev->root.x;
		  y = ev->root.y;
		  x -= zone->container->x;
		  y -= zone->container->y;
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_mouse(m, zone, x, y, 1, 1,
					E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	       }
	  }
     }
}

ACT_FN_GO_KEY(menu_show, )
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;

	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;

		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  ecore_x_pointer_xy_get(zone->container->win, &x, &y);
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_key(m, zone, x, y, 1, 1,
				      E_MENU_POP_DIRECTION_DOWN);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(exec, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
          e_exec(zone, NULL, params, NULL, "action/exec");
     }
}

/***************************************************************************/
ACT_FN_GO(app, )
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     Efreet_Desktop *desktop = NULL;
	     char *p, *p2;

	     p2 = alloca(strlen(params) + 1);
	     strcpy(p2, params);
	     p = strchr(p2, ' ');
	     if (p)
	       {
		  *p = 0;
		  if (!strcmp(p2, "file:"))
		    desktop = efreet_util_desktop_file_id_find(p + 1);
		  else if (!strcmp(p2, "name:"))
		    desktop = efreet_util_desktop_name_find(p + 1);
		  else if (!strcmp(p2, "generic:"))
		    desktop = efreet_util_desktop_generic_name_find(p + 1);
		  else if (!strcmp(p2, "exe:"))
		    desktop = efreet_util_desktop_exec_find(p + 1);
		  if (desktop)
		    {
		       e_exec(zone, desktop, NULL, NULL, "action/app");
		       efreet_desktop_free(desktop);
		    }
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_deskshow_toggle, __UNUSED__)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone) e_desk_deskshow(zone);
}

ACT_FN_GO(cleanup_windows, __UNUSED__)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone) e_place_zone_region_smart_cleanup(zone);
}

/***************************************************************************/
static E_Dialog *exit_dialog = NULL;

static void
_e_actions_cb_exit_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(exit_dialog));
	exit_dialog = NULL;
     }
   e_sys_action_do(E_SYS_EXIT, NULL);
}

static void
_e_actions_cb_exit_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(exit_dialog));
   exit_dialog = NULL;
}

static void
_e_actions_cb_exit_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_exit_dialog_cancel(NULL, dia);
}

ACT_FN_GO(exit, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_EXIT, NULL);
	return;
     }
   if (exit_dialog) e_object_del(E_OBJECT(exit_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_exit_dialog_ok(NULL, NULL);
	return;
     }

   exit_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_exit_dialog");
   if (!exit_dialog) return;
   e_win_delete_callback_set(exit_dialog->win, _e_actions_cb_exit_dialog_delete);
   e_dialog_title_set(exit_dialog, _("Are you sure you want to exit?"));
   e_dialog_text_set(exit_dialog,
		     _("You requested to exit Enlightenment.<br>"
		       "<br>"
		       "Are you sure you want to exit?"));
   e_dialog_icon_set(exit_dialog, "application-exit", 64);
   e_dialog_button_add(exit_dialog, _("Yes"), NULL,
		       _e_actions_cb_exit_dialog_ok, NULL);
   e_dialog_button_add(exit_dialog, _("No"), NULL,
		       _e_actions_cb_exit_dialog_cancel, NULL);
   e_dialog_button_focus_num(exit_dialog, 1);
   e_win_centered_set(exit_dialog->win, 1);
   e_dialog_show(exit_dialog);
}

/***************************************************************************/
ACT_FN_GO(restart, __UNUSED__)
{
   e_sys_action_do(E_SYS_RESTART, NULL);
}

/***************************************************************************/
ACT_FN_GO(exit_now, __UNUSED__)
{
   e_sys_action_do(E_SYS_EXIT_NOW, NULL);
}

/***************************************************************************/
ACT_FN_GO(halt_now, __UNUSED__)
{
   e_sys_action_do(E_SYS_HALT_NOW, NULL);
}

/***************************************************************************/
ACT_FN_GO(mode_presentation_toggle, __UNUSED__)
{
   e_config->mode.presentation = !e_config->mode.presentation;
   e_config_mode_changed();
   e_config_save_queue();
}

/***************************************************************************/
ACT_FN_GO(mode_offline_toggle, __UNUSED__)
{
   e_config->mode.offline = !e_config->mode.offline;
   e_config_mode_changed();
   e_config_save_queue();
}

/***************************************************************************/
static E_Dialog *logout_dialog = NULL;

static void
_e_actions_cb_logout_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(logout_dialog));
	logout_dialog = NULL;
     }
   e_sys_action_do(E_SYS_LOGOUT, NULL);
}

static void
_e_actions_cb_logout_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(logout_dialog));
   logout_dialog = NULL;
}

static void
_e_actions_cb_logout_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_logout_dialog_cancel(NULL, dia);
}

ACT_FN_GO(logout, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_LOGOUT, NULL);
	return;
     }
   if (logout_dialog) e_object_del(E_OBJECT(logout_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_logout_dialog_ok(NULL, NULL);
	return;
     }

   logout_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_logout_dialog");
   if (!logout_dialog) return;
   e_win_delete_callback_set(logout_dialog->win, _e_actions_cb_logout_dialog_delete);
   e_dialog_title_set(logout_dialog, _("Are you sure you want to log out?"));
   e_dialog_text_set(logout_dialog,
		     _("You are about to log out.<br>"
		       "<br>"
		       "Are you sure you want to do this?"));
   e_dialog_icon_set(logout_dialog, "system-log-out", 64);
   e_dialog_button_add(logout_dialog, _("Yes"), NULL,
		       _e_actions_cb_logout_dialog_ok, NULL);
   e_dialog_button_add(logout_dialog, _("No"), NULL,
		       _e_actions_cb_logout_dialog_cancel, NULL);
   e_dialog_button_focus_num(logout_dialog, 1);
   e_win_centered_set(logout_dialog->win, 1);
   e_dialog_show(logout_dialog);
}

/***************************************************************************/
static E_Dialog *halt_dialog = NULL;

static void
_e_actions_cb_halt_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(halt_dialog));
	halt_dialog = NULL;
     }
   e_sys_action_do(E_SYS_HALT, NULL);
}

static void
_e_actions_cb_halt_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(halt_dialog));
   halt_dialog = NULL;
}

static void
_e_actions_cb_halt_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_halt_dialog_cancel(NULL, dia);
}

ACT_FN_GO(halt, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_HALT, NULL);
	return;
     }
   if (halt_dialog) e_object_del(E_OBJECT(halt_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_halt_dialog_ok(NULL, NULL);
	return;
     }

   halt_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_halt_dialog");
   if (!halt_dialog) return;
   e_win_delete_callback_set(halt_dialog->win, _e_actions_cb_halt_dialog_delete);
   e_dialog_title_set(halt_dialog, _("Are you sure you want to turn off?"));
   e_dialog_text_set(halt_dialog,
		     _("You requested to turn off your Computer.<br>"
		       "<br>"
		       "Are you sure you want to shut down?"));
   e_dialog_icon_set(halt_dialog, "system-shutdown", 64);
   e_dialog_button_add(halt_dialog, _("Yes"), NULL,
		       _e_actions_cb_halt_dialog_ok, NULL);
   e_dialog_button_add(halt_dialog, _("No"), NULL,
		       _e_actions_cb_halt_dialog_cancel, NULL);
   e_dialog_button_focus_num(halt_dialog, 1);
   e_win_centered_set(halt_dialog->win, 1);
   e_dialog_show(halt_dialog);
}

/***************************************************************************/
static E_Dialog *reboot_dialog = NULL;

static void
_e_actions_cb_reboot_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(reboot_dialog));
	reboot_dialog = NULL;
     }
   e_sys_action_do(E_SYS_REBOOT, NULL);
}

static void
_e_actions_cb_reboot_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(reboot_dialog));
   reboot_dialog = NULL;
}

static void
_e_actions_cb_reboot_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_reboot_dialog_cancel(NULL, dia);
}

ACT_FN_GO(reboot, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_REBOOT, NULL);
	return;
     }
   if (reboot_dialog) e_object_del(E_OBJECT(reboot_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_reboot_dialog_ok(NULL, NULL);
	return;
     }

   reboot_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_reboot_dialog");
   if (!reboot_dialog) return;
   e_win_delete_callback_set(reboot_dialog->win, _e_actions_cb_reboot_dialog_delete);
   e_dialog_title_set(reboot_dialog, _("Are you sure you want to reboot?"));
   e_dialog_text_set(reboot_dialog,
		     _("You requested to reboot your Computer.<br>"
		       "<br>"
		       "Are you sure you want to restart it?"));
   e_dialog_icon_set(reboot_dialog, "system-restart", 64);
   e_dialog_button_add(reboot_dialog, _("Yes"), NULL,
		       _e_actions_cb_reboot_dialog_ok, NULL);
   e_dialog_button_add(reboot_dialog, _("No"), NULL,
		       _e_actions_cb_reboot_dialog_cancel, NULL);
   e_dialog_button_focus_num(reboot_dialog, 1);
   e_win_centered_set(reboot_dialog->win, 1);
   e_dialog_show(reboot_dialog);
}

/***************************************************************************/
static E_Dialog *suspend_dialog = NULL;

static void
_e_actions_cb_suspend_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(suspend_dialog));
	suspend_dialog = NULL;
     }
   e_sys_action_do(E_SYS_SUSPEND, NULL);
}

static void
_e_actions_cb_suspend_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(suspend_dialog));
   suspend_dialog = NULL;
}

static void
_e_actions_cb_suspend_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_suspend_dialog_cancel(NULL, dia);
}

ACT_FN_GO(suspend_now, __UNUSED__)
{
   e_sys_action_do(E_SYS_SUSPEND, NULL);
}

ACT_FN_GO(suspend, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_SUSPEND, NULL);
	return;
     }
   if (suspend_dialog) e_object_del(E_OBJECT(suspend_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_suspend_dialog_ok(NULL, NULL);
	return;
     }

   suspend_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_suspend_dialog");
   if (!suspend_dialog) return;
   e_win_delete_callback_set(suspend_dialog->win, _e_actions_cb_suspend_dialog_delete);
   e_dialog_title_set(suspend_dialog, _("Are you sure you want to turn off?"));
   e_dialog_text_set(suspend_dialog,
		     _("You requested to suspend your Computer.<br>"
		       "<br>"
		       "Are you sure you want to suspend?"));
   e_dialog_icon_set(suspend_dialog, "system-suspend", 64);
   e_dialog_button_add(suspend_dialog, _("Yes"), NULL,
		       _e_actions_cb_suspend_dialog_ok, NULL);
   e_dialog_button_add(suspend_dialog, _("No"), NULL,
		       _e_actions_cb_suspend_dialog_cancel, NULL);
   e_dialog_button_focus_num(suspend_dialog, 1);
   e_win_centered_set(suspend_dialog->win, 1);
   e_dialog_show(suspend_dialog);
}

/***************************************************************************/
static E_Dialog *hibernate_dialog = NULL;

static void
_e_actions_cb_hibernate_dialog_ok(void *data __UNUSED__, E_Dialog *dia)
{
   if (dia)
     {
	e_object_del(E_OBJECT(hibernate_dialog));
	hibernate_dialog = NULL;
     }
   e_sys_action_do(E_SYS_HIBERNATE, NULL);
}

static void
_e_actions_cb_hibernate_dialog_cancel(void *data __UNUSED__, E_Dialog *dia __UNUSED__)
{
   e_object_del(E_OBJECT(hibernate_dialog));
   hibernate_dialog = NULL;
}

static void
_e_actions_cb_hibernate_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_hibernate_dialog_cancel(NULL, dia);
}

ACT_FN_GO(hibernate, )
{
   if ((params) && (!strcmp(params, "now")))
     {
	e_sys_action_do(E_SYS_HIBERNATE, NULL);
	return;
     }
   if (hibernate_dialog) e_object_del(E_OBJECT(hibernate_dialog));

   if (e_config->cnfmdlg_disabled)
     {
	_e_actions_cb_hibernate_dialog_ok(NULL, NULL);
	return;
     }

   hibernate_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_hibernate_dialog");
   if (!hibernate_dialog) return;
   e_win_delete_callback_set(hibernate_dialog->win, _e_actions_cb_hibernate_dialog_delete);
   e_dialog_title_set(hibernate_dialog, _("Are you sure you want to hibernate?"));
   e_dialog_text_set(hibernate_dialog,
		     _("You requested to hibernate your Computer.<br>"
		       "<br>"
		       "Are you sure you want to suspend to disk?"));
   e_dialog_icon_set(hibernate_dialog, "system-suspend-hibernate", 64);
   e_dialog_button_add(hibernate_dialog, _("Yes"), NULL,
		       _e_actions_cb_hibernate_dialog_ok, NULL);
   e_dialog_button_add(hibernate_dialog, _("No"), NULL,
		       _e_actions_cb_hibernate_dialog_cancel, NULL);
   e_dialog_button_focus_num(hibernate_dialog, 1);
   e_win_centered_set(hibernate_dialog->win, 1);
   e_dialog_show(hibernate_dialog);
}

/***************************************************************************/
ACT_FN_GO(pointer_resize_push, )
{
   if (!obj) return;
   if (obj->type == E_BORDER_TYPE)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if ((bd->lock_user_size) || (bd->shaded) || (bd->shading) ||
	    (bd->fullscreen) || ((bd->maximized) && (!e_config->allow_manip)))
	  return;
	e_pointer_type_push(bd->pointer, bd, params);
     }
}

/***************************************************************************/
ACT_FN_GO(pointer_resize_pop, )
{
   if (!obj) return;
   if (obj->type == E_BORDER_TYPE)
     {
	E_Border *bd;

	bd = (E_Border *)obj;
	if ((bd->lock_user_size) || (bd->shaded) || (bd->shading) ||
	    (bd->fullscreen) || ((bd->maximized) && (!e_config->allow_manip)))
	  return;
	e_pointer_type_pop(bd->pointer, bd, params);
     }
}

/***************************************************************************/
ACT_FN_GO(desk_lock, __UNUSED__)
{
/*  E_Zone *zone;

  zone = _e_actions_zone_get(obj);
  if (zone)*/
  e_desklock_show();
}

/***************************************************************************/
ACT_FN_GO(shelf_show, )
{
   Eina_List *l;
   E_Shelf *es;

   if (params)
     {
	for (; *params != '\0'; params++)
	  if (!isspace(*params))
	    break;
	if (*params == '\0')
	  params = NULL;
     }

   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
	if ((!params) || (params && (fnmatch(params, es->name, 0) == 0)))
	  {
	     e_shelf_toggle(es, 1);
	     e_shelf_toggle(es, 0);
	  }
     }
}
/***************************************************************************/
#define ACT_SHELF_SHOW(params, es) \
if ((!params) || (params && (fnmatch(params, es->name, 0) == 0))) \
  { \
     e_shelf_toggle(es, 1); \
     e_shelf_toggle(es, 0); \
  }

ACT_FN_GO_EDGE(shelf_show, )
{
   Eina_List *l;
   E_Shelf *es;

   if (params)
     {
	for (; *params != '\0'; params++)
          {
             if (!isspace(*params))
               break;
          }
	if (*params == '\0')
	  params = NULL;
     }

   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
	switch(ev->edge)
	  {
          case E_ZONE_EDGE_LEFT:
             if ((es->gadcon->orient == E_GADCON_ORIENT_LEFT ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_LT ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_LB) &&
                 (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
               ACT_SHELF_SHOW(params, es);
             break;
          case E_ZONE_EDGE_RIGHT:
             if ((es->gadcon->orient == E_GADCON_ORIENT_RIGHT ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_RT ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_RB) &&
                 (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
               ACT_SHELF_SHOW(params, es);
             break;
          case E_ZONE_EDGE_TOP:
             if ((es->gadcon->orient == E_GADCON_ORIENT_TOP ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_TL ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_TR) &&
                 (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
               ACT_SHELF_SHOW(params, es);
             break;
          case E_ZONE_EDGE_BOTTOM:
             if ((es->gadcon->orient == E_GADCON_ORIENT_BOTTOM ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_BL ||
                  es->gadcon->orient == E_GADCON_ORIENT_CORNER_BR) &&
                 (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
               ACT_SHELF_SHOW(params, es);
             break;
          default:
             break;
	  }
     }
}
#undef ACT_SHELF_SHOW

/***************************************************************************/
typedef struct _Delayed_Action Delayed_Action;

struct _Delayed_Action
{
   int mouse, button;
   const char *keyname;
   E_Object *obj;
   Ecore_Timer *timer;
   struct
     {
        const char *action, *params;
     } def, delayed;
};

static Eina_List *_delayed_actions = NULL;

static void
_delayed_action_free(Delayed_Action *da)
{
   if (da->obj) e_object_unref(da->obj);
   if (da->keyname) eina_stringshare_del(da->keyname);
   if (da->timer) ecore_timer_del(da->timer);
   if (da->def.action) eina_stringshare_del(da->def.action);
   if (da->def.params) eina_stringshare_del(da->def.params);
   if (da->delayed.action) eina_stringshare_del(da->delayed.action);
   if (da->delayed.params) eina_stringshare_del(da->delayed.params);
   free(da);
}

static Eina_Bool
_delayed_action_cb_timer(void *data)
{
   Delayed_Action *da;
   E_Action *act;

   da = data;
   da->timer = NULL;
   act = e_action_find(da->delayed.action);
   if (act)
     {
	if (act->func.go) act->func.go(da->obj, da->delayed.params);
     }
   _delayed_actions = eina_list_remove(_delayed_actions, da);
   _delayed_action_free(da);
   return ECORE_CALLBACK_CANCEL;
}

static void
_delayed_action_do(Delayed_Action *da)
{
   E_Action *act;

   act = e_action_find(da->def.action);
   if (act)
     {
	if (act->func.go) act->func.go(da->obj, da->def.params);
     }
}

static void
_delayed_action_list_parse_action(const char *str, double *delay, const char **action, const char **params)
{
   char fbuf[16];
   char buf[1024];
   const char *p;

   buf[0] = 0;
   sscanf(str, "%10s %1000s", fbuf, buf);
   *action = eina_stringshare_add(buf);
   *delay = atof(fbuf);
   p = strchr(str, ' ');
   if (p)
     {
	p++;
	p = strchr(p, ' ');
	if (p)
	  {
	     p++;
	     *params = eina_stringshare_add(p);
	  }
     }
}

static void
_delayed_action_list_parse(Delayed_Action *da, const char *params)
{
   double delay = 2.0;
   const char *p, *a1start = NULL, *a1stop = NULL;
   const char *a2start = NULL, *a2stop = NULL;

   // FORMAT: "[0.0 default_action param1 param2] [x.x action2 param1 param2]"
   p = params;
   while (*p)
     {
	if ((*p == '[') && ((p == params) || ((p > params) && (p[-1] != '\\')))) {a1start = p + 1; break;}
	p++;
     }
   while (*p)
     {
	if ((*p == ']') && ((p == params) || ((p > params) && (p[-1] != '\\')))) {a1stop = p; break;}
	p++;
     }
   while (*p)
     {
	if ((*p == '[') && ((p == params) || ((p > params) && (p[-1] != '\\')))) {a2start = p + 1; break;}
	p++;
     }
   while (*p)
     {
	if ((*p == ']') && ((p == params) || ((p > params) && (p[-1] != '\\')))) {a2stop = p; break;}
	p++;
     }
   if ((a1start) && (a2start) && (a1stop) && (a2stop))
     {
	char *a1, *a2;
	const char *action, *params;

	a1 = alloca(a1stop - a1start + 1);
	eina_strlcpy(a1, a1start, a1stop - a1start + 1);
	action = NULL;
	params = NULL;
	_delayed_action_list_parse_action(a1, &delay, &da->def.action, &da->def.params);

	a2 = alloca(a1stop - a1start + 1);
	eina_strlcpy(a2, a2start, a2stop - a2start + 1);
	_delayed_action_list_parse_action(a2, &delay, &da->delayed.action, &da->delayed.params);
     }
   da->timer = ecore_timer_add(delay, _delayed_action_cb_timer, da);
}

static void
_delayed_action_key_add(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   Delayed_Action *da;

   da = E_NEW(Delayed_Action, 1);
   if (!da) return;
   if (obj)
     {
	da->obj = obj;
	e_object_ref(da->obj);
     }
   da->mouse = 0;
   da->keyname = eina_stringshare_add(ev->keyname);
   if (params) _delayed_action_list_parse(da, params);
   _delayed_actions = eina_list_append(_delayed_actions, da);
}

static void
_delayed_action_key_del(E_Object *obj, const char *params __UNUSED__, Ecore_Event_Key *ev)
{
   Eina_List *l;
   Delayed_Action *da;

   EINA_LIST_FOREACH(_delayed_actions, l, da)
     {
	if ((da->obj == obj) && (!da->mouse) &&
	    (!strcmp(da->keyname, ev->keyname)))
	  {
	     _delayed_action_do(da);
	     _delayed_action_free(da);
	     _delayed_actions = eina_list_remove_list(_delayed_actions, l);
	     return;
	  }
     }
}

static void
_delayed_action_mouse_add(E_Object *obj, const char *params, Ecore_Event_Mouse_Button *ev)
{
   Delayed_Action *da;

   da = E_NEW(Delayed_Action, 1);
   if (!da) return;
   if (obj)
     {
	da->obj = obj;
	e_object_ref(da->obj);
     }
   da->mouse = 1;
   da->button = ev->buttons;
   if (params) _delayed_action_list_parse(da, params);
   _delayed_actions = eina_list_append(_delayed_actions, da);
}

static void
_delayed_action_mouse_del(E_Object *obj, const char *params __UNUSED__, Ecore_Event_Mouse_Button *ev)
{
   Eina_List *l;
   Delayed_Action *da;

   EINA_LIST_FOREACH(_delayed_actions, l, da)
     {
	if ((da->obj == obj) && (da->mouse) &&
	    ((int) ev->buttons == da->button))
	  {
	     _delayed_action_do(da);
	     _delayed_action_free(da);
	     _delayed_actions = eina_list_remove_list(_delayed_actions, l);
	     return;
	  }
     }
}

// obj , params  , ev
ACT_FN_GO_KEY(delayed_action, )
{
   _delayed_action_key_add(obj, params, ev);
}

ACT_FN_GO_MOUSE(delayed_action, )
{
   _delayed_action_mouse_add(obj, params, ev);
}

ACT_FN_END_KEY(delayed_action, )
{
   _delayed_action_key_del(obj, params, ev);
}

ACT_FN_END_MOUSE(delayed_action, )
{
   _delayed_action_mouse_del(obj, params, ev);
}

ACT_FN_GO_ACPI(dim_screen, __UNUSED__)
{
   E_Zone *zone = _e_actions_zone_get(obj);
   e_backlight_mode_set(zone, E_BACKLIGHT_MODE_DIM);
}

ACT_FN_GO_ACPI(undim_screen, __UNUSED__)
{
   E_Zone *zone = _e_actions_zone_get(obj);
   e_backlight_mode_set(zone, E_BACKLIGHT_MODE_NORMAL);
}

ACT_FN_GO_ACPI(backlight_set, )
{
   E_Zone *zone = _e_actions_zone_get(obj);
   double v = atof(params);
   e_backlight_mode_set(zone, E_BACKLIGHT_MODE_NORMAL);
   e_backlight_level_set(zone, v, -1.0);
}

ACT_FN_GO_ACPI(backlight_adjust, )
{
   E_Zone *zone = _e_actions_zone_get(obj);
   double v = atof(params);
   e_backlight_mode_set(zone, E_BACKLIGHT_MODE_NORMAL);
   e_backlight_level_set(zone, e_backlight_level_get(zone) + v, -1.0);
}

/* local subsystem globals */
static Eina_Hash *actions = NULL;
static Eina_List *action_list = NULL;
static Eina_List *action_names = NULL;
static Eina_List *action_groups = NULL;

/* externally accessible functions */

EINTERN int
e_actions_init(void)
{
   E_Action *act;

   actions = eina_hash_string_superfast_new(NULL);
   ACT_GO(window_move);
   e_action_predef_name_set(N_("Window : Actions"), N_("Move"),
			    "window_move", NULL, NULL, 0);

   ACT_GO_MOUSE(window_move);
   ACT_GO_SIGNAL(window_move);
   ACT_END(window_move);
   ACT_END_MOUSE(window_move);
   ACT_GO_KEY(window_move);

   /* window_resize */
   ACT_GO(window_resize);
   e_action_predef_name_set(N_("Window : Actions"), N_("Resize"),
			    "window_resize", NULL, NULL, 0);

   ACT_GO_MOUSE(window_resize);
   ACT_GO_SIGNAL(window_resize);
   ACT_END(window_resize);
   ACT_END_MOUSE(window_resize);
   ACT_GO_KEY(window_resize);

   /* window_menu */
   ACT_GO(window_menu);
   e_action_predef_name_set(N_("Menu"), N_("Window Menu"),
			    "window_menu", NULL, NULL, 0);

   ACT_GO_MOUSE(window_menu);
   ACT_GO_KEY(window_menu);

   /* window_raise */
   ACT_GO(window_raise);
   e_action_predef_name_set(N_("Window : Actions"), N_("Raise"),
			    "window_raise", NULL, NULL, 0);

   /* window_lower */
   ACT_GO(window_lower);
   e_action_predef_name_set(N_("Window : Actions"), N_("Lower"),
			    "window_lower", NULL, NULL, 0);

   /* window_close */
   ACT_GO(window_close);
   e_action_predef_name_set(N_("Window : Actions"), N_("Close"),
			    "window_close", NULL, NULL, 0);

   /* window_kill */
   ACT_GO(window_kill);
   e_action_predef_name_set(N_("Window : Actions"), N_("Kill"),
			    "window_kill", NULL, NULL, 0);

   /* window_sticky_toggle */
   ACT_GO(window_sticky_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Sticky Mode Toggle"),
			    "window_sticky_toggle", NULL, NULL, 0);

   ACT_GO(window_sticky);

   /* window_iconic_toggle */
   ACT_GO(window_iconic_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Iconic Mode Toggle"),
			    "window_iconic_toggle", NULL, NULL, 0);

   ACT_GO(window_iconic);

   /* window_fullscreen_toggle */
   ACT_GO(window_fullscreen_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Fullscreen Mode Toggle"),
			    "window_fullscreen_toggle", NULL, NULL, 0);

   ACT_GO(window_fullscreen);

   /* window_maximized_toggle */
   ACT_GO(window_maximized_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize"),
			    "window_maximized_toggle", NULL, NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Vertically"),
			    "window_maximized_toggle", "default vertical",
			    NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Horizontally"),
			    "window_maximized_toggle", "default horizontal",
			    NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Fullscreen"),
			    "window_maximized_toggle", "fullscreen", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Mode \"Smart\""),
			    "window_maximized_toggle", "smart", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Mode \"Expand\""),
			    "window_maximized_toggle", "expand", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Maximize Mode \"Fill\""),
			    "window_maximized_toggle", "fill", NULL, 0);

   ACT_GO(window_maximized);

   /* window_shaded_toggle */
   ACT_GO(window_shaded_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Shade Up Mode Toggle"),
			    "window_shaded_toggle", "up", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Shade Down Mode Toggle"),
			    "window_shaded_toggle", "down", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Shade Left Mode Toggle"),
			    "window_shaded_toggle", "left", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Shade Right Mode Toggle"),
			    "window_shaded_toggle", "right", NULL, 0);
   e_action_predef_name_set(N_("Window : State"), N_("Shade Mode Toggle"),
			    "window_shaded_toggle", NULL, NULL, 0);

   ACT_GO(window_shaded);

   /* window_borderless_toggle */
   ACT_GO(window_borderless_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Toggle Borderless State"),
			    "window_borderless_toggle", NULL, NULL, 0);

   /* window_border_set */
   ACT_GO(window_border_set);
   e_action_predef_name_set(N_("Window : State"), N_("Set Border"),
                            "window_border_set", NULL,
                            "syntax: BorderName, example: pixel", 1);

   /* window_border_cycle */
   ACT_GO(window_border_cycle);
   e_action_predef_name_set(N_("Window : State"), N_("Cycle between Borders"),
                            "window_border_cycle", NULL,
                            "syntax: BorderNames, example: default pixel", 1);

   /* window_pinned_toggle */
   ACT_GO(window_pinned_toggle);
   e_action_predef_name_set(N_("Window : State"), N_("Toggle Pinned State"),
			    "window_pinned_toggle", NULL, NULL, 0);

   /* desk_flip_by */
   ACT_GO(desk_flip_by);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Left"),
			    "desk_flip_by", "-1 0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Right"),
			    "desk_flip_by", "1 0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Up"),
			    "desk_flip_by", "0 -1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Down"),
			    "desk_flip_by", "0 1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop By..."),
			    "desk_flip_by", NULL,
			    "syntax: X-offset Y-offset, example: -1 0", 1);

   /* desk_deskshow_toggle */
   ACT_GO(desk_deskshow_toggle);
   e_action_predef_name_set(N_("Desktop"), N_("Show The Desktop"),
			    "desk_deskshow_toggle", NULL, NULL, 0);

   /* shelf_show */
   ACT_GO(shelf_show);
   ACT_GO_EDGE(shelf_show);
   e_action_predef_name_set(N_("Desktop"), N_("Show The Shelf"), "shelf_show",
			    NULL, "shelf name glob: Shelf-* ", 1);

   /* desk_linear_flip_to */
   ACT_GO(desk_flip_to);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop To..."),
			    "desk_flip_to", NULL,
			    "syntax: X Y, example: 1 2", 1);

   /* desk_linear_flip_by */
   ACT_GO(desk_linear_flip_by);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Linearly..."),
			    "desk_linear_flip_by",
			    NULL, "syntax: N-offset, example: -2", 1);

   /* desk_linear_flip_to */
   ACT_GO(desk_linear_flip_to);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 0"),
			    "desk_linear_flip_to", "0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 1"),
			    "desk_linear_flip_to", "1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 2"),
			    "desk_linear_flip_to", "2", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 3"),
			    "desk_linear_flip_to", "3", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 4"),
			    "desk_linear_flip_to", "4", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 5"),
			    "desk_linear_flip_to", "5", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 6"),
			    "desk_linear_flip_to", "6", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 7"),
			    "desk_linear_flip_to", "7", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 8"),
			    "desk_linear_flip_to", "8", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 9"),
			    "desk_linear_flip_to", "9", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 10"),
			    "desk_linear_flip_to", "10", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 11"),
			    "desk_linear_flip_to", "11", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop..."),
			    "desk_linear_flip_to", NULL,
			    "syntax: N, example: 1", 1);

   /* desk_flip_by_all */
   ACT_GO(desk_flip_by_all);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Left (All Screens)"),
			    "desk_flip_by_all", "-1 0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Right (All Screens)"),
			    "desk_flip_by_all", "1 0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Up (All Screens)"),
			    "desk_flip_by_all", "0 -1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Down (All Screens)"),
			    "desk_flip_by_all", "0 1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop By... (All Screens)"),
			    "desk_flip_by_all", NULL,
			    "syntax: X-offset Y-offset, example: -1 0", 1);

   /* desk_flip_to_all */
   ACT_GO(desk_flip_to_all);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop To... (All Screens)"),
			    "desk_flip_to_all", NULL,
			    "syntax: X Y, example: 1 2", 1);

   /* desk_linear_flip_by_all */
   ACT_GO(desk_linear_flip_by_all);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop Linearly... (All Screens)"),
			    "desk_linear_flip_by_all",
			    NULL, "syntax: N-offset, example: -2", 1);

   /* desk_flip_in_direction */
   ACT_GO_EDGE(desk_flip_in_direction);
   e_action_predef_name_set(N_("Desktop"), N_("Flip Desktop In Direction..."),
			    "desk_flip_in_direction", NULL, "syntax: N-pixel-offset, example: 25", 1);

   /* desk_linear_flip_to_all */
   ACT_GO(desk_linear_flip_to_all);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 0 (All Screens)"),
			    "desk_linear_flip_to_all", "0", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 1 (All Screens)"),
			    "desk_linear_flip_to_all", "1", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 2 (All Screens)"),
			    "desk_linear_flip_to_all", "2", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 3 (All Screens)"),
			    "desk_linear_flip_to_all", "3", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 4 (All Screens)"),
			    "desk_linear_flip_to_all", "4", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 5 (All Screens)"),
			    "desk_linear_flip_to_all", "5", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 6 (All Screens)"),
			    "desk_linear_flip_to_all", "6", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 7 (All Screens)"),
			    "desk_linear_flip_to_all", "7", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 8 (All Screens)"),
			    "desk_linear_flip_to_all", "8", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 9 (All Screens)"),
			    "desk_linear_flip_to_all", "9", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 10 (All Screens)"),
			    "desk_linear_flip_to_all", "10", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop 11 (All Screens)"),
			    "desk_linear_flip_to_all", "11", NULL, 0);
   e_action_predef_name_set(N_("Desktop"), N_("Switch To Desktop... (All Screens)"),
			    "desk_linear_flip_to_all", NULL,
			    "syntax: N, example: 1", 1);

   /* screen_send_to */
   ACT_GO(screen_send_to);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse To Screen 0"),
			    "screen_send_to", "0", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse To Screen 1"),
			    "screen_send_to", "1", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse To Screen..."),
			    "screen_send_to", NULL,
			    "syntax: N, example: 0", 1);

   /* screen_send_by */
   ACT_GO(screen_send_by);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse Forward 1 Screen"),
			    "screen_send_by", "1", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse Back 1 Screen"),
			    "screen_send_by", "-1", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Send Mouse Forward/Back Screens..."),
			    "screen_send_by", NULL,
			    "syntax: N-offset, example: -2", 1);

   ACT_GO_ACPI(dim_screen);
   e_action_predef_name_set(N_("Screen"), N_("Dim"), "dim_screen",
			    NULL, NULL, 0);
   ACT_GO_ACPI(undim_screen);
   e_action_predef_name_set(N_("Screen"), N_("Undim"), "undim_screen",
			    NULL, NULL, 0);
   ACT_GO_ACPI(backlight_set);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Set"), "backlight_set",
			    NULL, "syntax: brightness(0.0 - 1.0), example: 0.5", 1);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Min"), "backlight_set",
			    "0.0", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Mid"), "backlight_set",
			    "0.5", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Max"), "backlight_set",
			    "1.0", NULL, 0);
   ACT_GO_ACPI(backlight_adjust);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Adjust"), "backlight_adjust",
			    NULL, "syntax: brightness(-1.0 - 1.0), example: -0.2", 1);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Up"), "backlight_adjust",
			    "0.1", NULL, 0);
   e_action_predef_name_set(N_("Screen"), N_("Backlight Down"), "backlight_adjust",
			    "-0.1", NULL, 0);

   /* window_move_to_center */
   ACT_GO(window_move_to_center);
   e_action_predef_name_set(N_("Window : Actions"), N_("Move To Center"),
			    "window_move_to_center", NULL, NULL, 0);
   /* window_move_to */
   ACT_GO(window_move_to);
   e_action_predef_name_set(N_("Window : Actions"), N_("Move To..."),
			    "window_move_to", NULL,
			    "syntax: [+,-]X [+,-]Y or * [+,-]Y or [+,-]X *, example: -1 +1", 1);
   /* window_move_by */
   ACT_GO(window_move_by);
   e_action_predef_name_set(N_("Window : Actions"), N_("Move By..."),
			    "window_move_by", NULL,
			    "syntax: X-offset Y-offset, example: -1 0", 1);

   /* window_resize_by */
   ACT_GO(window_resize_by);
   e_action_predef_name_set(N_("Window : Actions"), N_("Resize By..."),
			    "window_resize_by", NULL,
			    "syntax: W H, example: 100 150", 1);

   /* window_push */
   ACT_GO(window_push);
   e_action_predef_name_set(N_("Window : Actions"), N_("Push in Direction..."),
			    "window_push", NULL,
			    "syntax: direction, example: up, down, left, right, up-left, up-right, down-left, down-right", 1);

   /* window_drag_icon */
   ACT_GO(window_drag_icon);
   e_action_predef_name_set(N_("Window : Actions"), N_("Drag Icon..."),
			    "window_drag_icon", NULL, NULL, 0);

   /* window_desk_move_by */
   ACT_GO(window_desk_move_by);
   e_action_predef_name_set(N_("Window : Moving"), N_("To Next Desktop"),
			    "window_desk_move_by", "1 0", NULL, 0);
   e_action_predef_name_set(N_("Window : Moving"), N_("To Previous Desktop"),
			    "window_desk_move_by", "-1 0", NULL, 0);
   e_action_predef_name_set(N_("Window : Moving"), N_("By Desktop #..."),
			    "window_desk_move_by", NULL,
			    "syntax: X-offset Y-offset, example: -2 2", 1);

   /* window_desk_move_to */
   ACT_GO(window_desk_move_to);
   e_action_predef_name_set(N_("Window : Moving"), N_("To Desktop..."),
			    "window_desk_move_to", NULL,
			    "syntax: X Y, example: 0 1", 1);

   /* menu_show */
   ACT_GO(menu_show);
   e_action_predef_name_set(N_("Menu"), N_("Show Main Menu"),
			    "menu_show", "main", NULL, 0);
   e_action_predef_name_set(N_("Menu"), N_("Show Favorites Menu"), "menu_show",
			    "favorites", NULL, 0);
   e_action_predef_name_set(N_("Menu"), N_("Show All Applications Menu"),
			    "menu_show", "all", NULL, 0);
   e_action_predef_name_set(N_("Menu"), N_("Show Clients Menu"), "menu_show",
			    "clients", NULL, 0);
   e_action_predef_name_set(N_("Menu"), N_("Show Menu..."), "menu_show", NULL,
			    "syntax: MenuName, example: MyMenu", 1);
   ACT_GO_MOUSE(menu_show);
   ACT_GO_KEY(menu_show);

   /* exec */
   ACT_GO(exec);
   e_action_predef_name_set(N_("Launch"), N_("Command"), "exec", NULL,
			    "syntax: CommandName, example: /usr/bin/xmms", 1);

   /* app */
   ACT_GO(app);
   e_action_predef_name_set(N_("Launch"), N_("Application"), "app", NULL,
			    "syntax: , example:", 1);

   ACT_GO(restart);
   e_action_predef_name_set(N_("Enlightenment"), N_("Restart"), "restart",
			    NULL, NULL, 0);

   ACT_GO(exit);
   e_action_predef_name_set(N_("Enlightenment"), N_("Exit"), "exit",
			    NULL, NULL, 0);

   ACT_GO(exit_now);
   e_action_predef_name_set(N_("Enlightenment"), N_("Exit Now"),
			    "exit_now", NULL, NULL, 0);

   ACT_GO(mode_presentation_toggle);
   e_action_predef_name_set(N_("Enlightenment : Mode"),
			    N_("Presentation Mode Toggle"),
			    "mode_presentation_toggle", NULL, NULL, 0);

   ACT_GO(mode_offline_toggle);
   e_action_predef_name_set(N_("Enlightenment : Mode"),
			    N_("Offline Mode Toggle"),
			    "mode_offline_toggle", NULL, NULL, 0);

   ACT_GO(logout);
   e_action_predef_name_set(N_("System"), N_("Log Out"), "logout",
			    NULL, NULL, 0);

   ACT_GO(halt_now);
   e_action_predef_name_set(N_("System"), N_("Power Off Now"),
			    "halt_now", NULL, NULL, 0);

   ACT_GO(halt);
   e_action_predef_name_set(N_("System"), N_("Power Off"), "halt",
			    NULL, NULL, 0);

   ACT_GO(reboot);
   e_action_predef_name_set(N_("System"), N_("Reboot"), "reboot",
			    NULL, NULL, 0);

   ACT_GO(suspend_now);
   e_action_predef_name_set(N_("System"), N_("Suspend Now"), "suspend_now",
			    NULL, NULL, 0);

   ACT_GO(suspend);
   e_action_predef_name_set(N_("System"), N_("Suspend"), "suspend",
			    NULL, NULL, 0);

   ACT_GO(hibernate);
   e_action_predef_name_set(N_("System"), N_("Hibernate"), "hibernate",
			    NULL, NULL, 0);

   ACT_GO(pointer_resize_push);
   ACT_GO(pointer_resize_pop);

   /* desk_lock */
   ACT_GO(desk_lock);
   e_action_predef_name_set(N_("Desktop"), N_("Lock"), "desk_lock",
			    NULL, NULL, 0);

   /* cleanup_windows */
   ACT_GO(cleanup_windows);
   e_action_predef_name_set(N_("Desktop"), N_("Cleanup Windows"),
			    "cleanup_windows", NULL, NULL, 0);

   /* delayed_action */
   ACT_GO_KEY(delayed_action);
   e_action_predef_name_set(N_("Generic : Actions"), N_("Delayed Action"),
			    "delayed_action", NULL, "[0.0 exec xterm] [0.3 exec xev]", 1);
   ACT_GO_MOUSE(delayed_action);
   ACT_END_KEY(delayed_action);
   ACT_END_MOUSE(delayed_action);

   return 1;
}

EINTERN int
e_actions_shutdown(void)
{
   e_action_predef_name_all_del();

   E_FREE_LIST(action_list, e_object_del);

   action_names = eina_list_free(action_names);
   eina_hash_free(actions);
   actions = NULL;

   return 1;
}

EAPI Eina_List *
e_action_name_list(void)
{
   return action_names;
}

EAPI E_Action *
e_action_add(const char *name)
{
   E_Action *act;

   act = e_action_find(name);
   if (!act)
     {
	act = E_OBJECT_ALLOC(E_Action, E_ACTION_TYPE, _e_action_free);
	if (!act) return NULL;
	act->name = name;
	eina_hash_direct_add(actions, act->name, act);
	action_names = eina_list_append(action_names, name);
	action_list = eina_list_append(action_list, act);
     }
   return act;
}

EAPI void
e_action_del(const char *name)
{
   E_Action *act;

   act = eina_hash_find(actions, name);
   if (act) _e_action_free(act);
}

EAPI E_Action *
e_action_find(const char *name)
{
   E_Action *act;

   act = eina_hash_find(actions, name);
   return act;
}

EAPI const char *
e_action_predef_label_get(const char *action, const char *params)
{
   E_Action_Group *actg = NULL;
   E_Action_Description *actd = NULL;
   Eina_List *l, *l2;

   EINA_LIST_FOREACH(action_groups, l, actg)
     {
	EINA_LIST_FOREACH(actg->acts, l2, actd)
          {
             if (!strcmp(actd->act_cmd, action))
               {
                  if ((params) && (actd->act_params))
                    {
                       if (!strcmp(params, actd->act_params))
                         return actd->act_name;
                    }
                  else return actd->act_name;
               }
          }
     }
   if (params) return e_action_predef_label_get(action, NULL);
   return NULL;
}

EAPI void
e_action_predef_name_set(const char *act_grp, const char *act_name, const char *act_cmd, const char *act_params, const char *param_example, int editable)
{
   E_Action_Group *actg = NULL;
   E_Action_Description *actd = NULL;
   Eina_List *l;

   if ((!act_grp) || (!act_name)) return;

   EINA_LIST_FOREACH(action_groups, l, actg)
     {
	if (!strcmp(actg->act_grp, act_grp)) break;
	actg = NULL;
     }

   if (!actg)
     {
	actg = E_NEW(E_Action_Group, 1);
	if (!actg) return;

	actg->act_grp = eina_stringshare_add(act_grp);
	actg->acts = NULL;

	action_groups = eina_list_append(action_groups, actg);
	action_groups =
          eina_list_sort(action_groups, -1, _action_groups_sort_cb);
     }

   EINA_LIST_FOREACH(actg->acts, l, actd)
     {
	if (!strcmp(actd->act_name, act_name)) break;
	actd = NULL;
     }

   if (actd) return;

   actd = E_NEW(E_Action_Description, 1);
   if (!actd) return;

   actd->act_name = eina_stringshare_add(act_name);
   actd->act_cmd = !act_cmd ? NULL : eina_stringshare_add(act_cmd);
   actd->act_params = !act_params ? NULL : eina_stringshare_add(act_params);
   actd->param_example = !param_example ? NULL : eina_stringshare_add(param_example);
   actd->editable = editable;

   actg->acts = eina_list_append(actg->acts, actd);
}

EAPI void
e_action_predef_name_del(const char *act_grp, const char *act_name)
{
   E_Action_Group *actg = NULL;
   E_Action_Description *actd = NULL;
   Eina_List *l;

   EINA_LIST_FOREACH(action_groups, l, actg)
     {
	if (!strcmp(actg->act_grp, act_grp)) break;
	actg = NULL;
     }

   if (!actg) return;

   EINA_LIST_FOREACH(actg->acts, l, actd)
     {
	if (!strcmp(actd->act_name, act_name))
	  {
	     actg->acts = eina_list_remove(actg->acts, actd);

	     if (actd->act_name) eina_stringshare_del(actd->act_name);
	     if (actd->act_cmd) eina_stringshare_del(actd->act_cmd);
	     if (actd->act_params) eina_stringshare_del(actd->act_params);
	     if (actd->param_example) eina_stringshare_del(actd->param_example);

	     E_FREE(actd);

	     if (!eina_list_count(actg->acts))
	       {
		  action_groups = eina_list_remove(action_groups, actg);
		  if (actg->act_grp) eina_stringshare_del(actg->act_grp);
		  E_FREE(actg);
	       }
	     break;
	  }
     }
}

EAPI void
e_action_predef_name_all_del(void)
{
   E_Action_Group *actg = NULL;
   E_Action_Description *actd = NULL;

   EINA_LIST_FREE(action_groups, actg)
     {
	EINA_LIST_FREE(actg->acts, actd)
	  {
	     if (actd->act_name) eina_stringshare_del(actd->act_name);
	     if (actd->act_cmd) eina_stringshare_del(actd->act_cmd);
	     if (actd->act_params) eina_stringshare_del(actd->act_params);
	     if (actd->param_example) eina_stringshare_del(actd->param_example);

	     E_FREE(actd);
	  }
	if (actg->act_grp) eina_stringshare_del(actg->act_grp);
	E_FREE(actg);
     }
   action_groups = NULL;
}

EAPI Eina_List *
e_action_groups_get(void)
{
   return action_groups;
}

/* local subsystem functions */

static void
_e_action_free(E_Action *act)
{
   eina_hash_del(actions, act->name, act);
   action_names = eina_list_remove(action_names, act->name);
   action_list = eina_list_remove(action_list, act);
   free(act);
}

static E_Maximize
_e_actions_maximize_parse(const char *params)
{
   E_Maximize max = 0;
   int ret;
   char s1[32], s2[32];

   if (!params) return e_config->maximize_policy;
   ret = sscanf(params, "%20s %20s", s1, s2);
   if (ret == 2)
     {
	if (!strcmp(s2, "horizontal"))
          max = E_MAXIMIZE_HORIZONTAL;
	else if (!strcmp(s2, "vertical"))
          max = E_MAXIMIZE_VERTICAL;
	else
          max = E_MAXIMIZE_BOTH;
     }
   if (ret >= 1)
     {
	if (!strcmp(s1, "fullscreen"))
          max |= E_MAXIMIZE_FULLSCREEN;
	else if (!strcmp(s1, "smart"))
          max |= E_MAXIMIZE_SMART;
	else if (!strcmp(s1, "expand"))
          max |= E_MAXIMIZE_EXPAND;
	else if (!strcmp(s1, "fill"))
          max |= E_MAXIMIZE_FILL;
	else
          max |= (e_config->maximize_policy & E_MAXIMIZE_TYPE);
     }
   else
     max = e_config->maximize_policy;
   return max;
}

static int
_action_groups_sort_cb(const void *d1, const void *d2)
{
   const E_Action_Group *g1, *g2;

   if (!(g1 = d1)) return 1;
   if (!(g2 = d2)) return -1;
   return strcmp(g1->act_grp, g2->act_grp);
}

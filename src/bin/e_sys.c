/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_sys_cb_timer(void *data);
static int _e_sys_cb_exit(void *data, int type, void *event);
static void _e_sys_cb_logout_logout(void *data, E_Dialog *dia);
static void _e_sys_cb_logout_wait(void *data, E_Dialog *dia);
static void _e_sys_cb_logout_abort(void *data, E_Dialog *dia);
static int _e_sys_cb_logout_timer(void *data);
static void _e_sys_logout_after(void);
static void _e_sys_logout_begin(E_Sys_Action a_after);
static void _e_sys_current_action(void);
static void _e_sys_action_failed(void);
static int _e_sys_action_do(E_Sys_Action a, char *param);
static void _e_sys_dialog_cb_delete(E_Obj_Dialog * od);

static Ecore_Event_Handler *_e_sys_exe_exit_handler = NULL;
static Ecore_Exe *_e_sys_halt_check_exe = NULL;
static Ecore_Exe *_e_sys_reboot_check_exe = NULL;
static Ecore_Exe *_e_sys_suspend_check_exe = NULL;
static Ecore_Exe *_e_sys_hibernate_check_exe = NULL;
static int _e_sys_can_halt = 0;
static int _e_sys_can_reboot = 0;
static int _e_sys_can_suspend = 0;
static int _e_sys_can_hibernate = 0;

static E_Sys_Action _e_sys_action_current = E_SYS_NONE;
static E_Sys_Action _e_sys_action_after = E_SYS_NONE;
static Ecore_Exe *_e_sys_exe = NULL;
static double _e_sys_logout_begin_time = 0.0;
static Ecore_Timer *_e_sys_logout_timer = NULL;
static E_Obj_Dialog *_e_sys_dialog = NULL;

/* externally accessible functions */
EAPI int
e_sys_init(void)
{
   /* this is not optimal - but it does work cleanly */
   _e_sys_exe_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
						     _e_sys_cb_exit, NULL);
   /* delay this for 1.0 seconds while the rest of e starts up */
   ecore_timer_add(1.0, _e_sys_cb_timer, NULL);
   return 1;
}

EAPI int
e_sys_shutdown(void)
{
   if (_e_sys_exe_exit_handler)
     ecore_event_handler_del(_e_sys_exe_exit_handler);
   _e_sys_exe_exit_handler = NULL;
   _e_sys_halt_check_exe = NULL;
   _e_sys_reboot_check_exe = NULL;
   _e_sys_suspend_check_exe = NULL;
   _e_sys_hibernate_check_exe = NULL;
   return 1;
}

EAPI int
e_sys_action_possible_get(E_Sys_Action a)
{
   switch (a)
     {
      case E_SYS_EXIT:
      case E_SYS_RESTART:
      case E_SYS_EXIT_NOW:
	return 1;
      case E_SYS_LOGOUT:
	return 0;
      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
	return _e_sys_can_halt;
      case E_SYS_REBOOT:
	return _e_sys_can_reboot;
      case E_SYS_SUSPEND:
	return _e_sys_can_suspend;
      case E_SYS_HIBERNATE:
	return _e_sys_can_hibernate;
      default:
	return 0;
     }
   return 0;
}

EAPI int
e_sys_action_do(E_Sys_Action a, char *param)
{
   int ret;

   if (_e_sys_action_current != E_SYS_NONE)
     {
	_e_sys_current_action();
	return 0;
     }
   switch (a)
     {
      case E_SYS_EXIT:
      case E_SYS_RESTART:
      case E_SYS_EXIT_NOW:
      case E_SYS_LOGOUT:
      case E_SYS_SUSPEND:
      case E_SYS_HIBERNATE:
      case E_SYS_HALT_NOW:
	ret = _e_sys_action_do(a, param);
	break;
      case E_SYS_HALT:
      case E_SYS_REBOOT:
	if (!e_util_immortal_check()) _e_sys_logout_begin(a);
	return 1;
	break;
      default:
	return 0;
     }
   _e_sys_action_current = a;
   return ret;
}

/* local subsystem functions */
static int
_e_sys_cb_timer(void *data)
{
   /* exec out sys helper and ask it to test if we are allowed to do these
    * things
    */
   char buf[4096];
   
   e_init_status_set(_("Checking System Permissions"));
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t halt", e_prefix_bin_get());
   _e_sys_halt_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t reboot", e_prefix_bin_get());
   _e_sys_reboot_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t suspend", e_prefix_bin_get());
   _e_sys_suspend_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t hibernate", e_prefix_bin_get());
   _e_sys_hibernate_check_exe = ecore_exe_run(buf, NULL);
   return 0;
}

static int
_e_sys_cb_exit(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if ((_e_sys_exe) && (ev->exe == _e_sys_exe))
     {
	if (ev->exit_code != 0) _e_sys_action_failed();
	if (((_e_sys_action_current != E_SYS_HALT) &&
	     (_e_sys_action_current != E_SYS_HALT_NOW) &&
	     (_e_sys_action_current != E_SYS_REBOOT)) ||
	    (ev->exit_code != 0))
	  {
	     if (_e_sys_dialog)
	       {
		  e_object_del(E_OBJECT(_e_sys_dialog));
		  _e_sys_dialog = NULL;
	       }
	  }
	_e_sys_action_current = E_SYS_NONE;
	_e_sys_exe = NULL;
	return 1;
     }
   if ((_e_sys_halt_check_exe) && (ev->exe == _e_sys_halt_check_exe))
     {
	e_init_status_set(_("System Check Done"));
	/* exit_code: 0 == OK, 5 == suid root removed, 7 == group id error
	 * 10 == permission denied, 20 == action undefined */
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_halt = 1;
	     _e_sys_halt_check_exe = NULL;
	  }
     }
   else if ((_e_sys_reboot_check_exe) && (ev->exe == _e_sys_reboot_check_exe))
     {
	e_init_status_set(_("System Check Done"));
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_reboot = 1;
	     _e_sys_reboot_check_exe = NULL;
	  }
     }
   else if ((_e_sys_suspend_check_exe) && (ev->exe == _e_sys_suspend_check_exe))
     {
	e_init_status_set(_("System Check Done"));
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_suspend = 1;
	     _e_sys_suspend_check_exe = NULL;
	  }
     }
   else if ((_e_sys_hibernate_check_exe) && (ev->exe == _e_sys_hibernate_check_exe))
     {
	e_init_status_set(_("System Check Done"));
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_hibernate = 1;
	     _e_sys_hibernate_check_exe = NULL;
	  }
     }
   return 1;
}

static void
_e_sys_cb_logout_logout(void *data, E_Dialog *dia)
{
   if (_e_sys_logout_timer)
     {
	ecore_timer_del(_e_sys_logout_timer);
	_e_sys_logout_timer = NULL;
     }
   _e_sys_logout_begin_time = 0.0;
   _e_sys_logout_after();
   e_object_del(E_OBJECT(dia));
}

static void
_e_sys_cb_logout_wait(void *data, E_Dialog *dia)
{
   if (_e_sys_logout_timer) ecore_timer_del(_e_sys_logout_timer);
   _e_sys_logout_timer = ecore_timer_add(0.5, _e_sys_cb_logout_timer, NULL);
   _e_sys_logout_begin_time = ecore_time_get();
   e_object_del(E_OBJECT(dia));
}

static void
_e_sys_cb_logout_abort(void *data, E_Dialog *dia)
{
   if (_e_sys_logout_timer)
     {
	ecore_timer_del(_e_sys_logout_timer);
	_e_sys_logout_timer = NULL;
     }
   _e_sys_logout_begin_time = 0.0;
   e_object_del(E_OBJECT(dia));
   _e_sys_action_current = E_SYS_NONE;
   _e_sys_action_after = E_SYS_NONE;
   if (_e_sys_dialog)
     {
	e_object_del(E_OBJECT(_e_sys_dialog));
	_e_sys_dialog = NULL;
     }
}

static int
_e_sys_cb_logout_timer(void *data)
{
   Eina_List *l;
   int pending = 0;

   for (l = e_border_client_list(); l; l = l->next)
     {
	E_Border *bd;

	bd = l->data;
	if (!bd->internal) pending++;
     }
   if (pending == 0) goto after;
   else
     {
	/* it has taken 15 seconds of waiting and we still have apps that
	 * will not go away
	 */
	if ((ecore_time_get() - _e_sys_logout_begin_time) > 15.0)
	  {
	     E_Dialog *dia;

	     dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_sys_error_logout_slow");
	     if (dia)
	       {
		  e_dialog_title_set(dia, _("Logout problems"));
		  e_dialog_icon_set(dia, "enlightenment/logout", 64);
		  e_dialog_text_set(dia,
				    _("Logout is taking too long. Some<br>"
				      "applications refuse to close.<br>"
				      "Do you want to finish the logout<br>"
				      "anyway without closing these<br>"
				      "applications first?")
				    );
		  e_dialog_button_add(dia, _("Logout now"), NULL, _e_sys_cb_logout_logout, NULL);
		  e_dialog_button_add(dia, _("Wait longer"), NULL, _e_sys_cb_logout_wait, NULL);
		  e_dialog_button_add(dia, _("Cancel Logout"), NULL, _e_sys_cb_logout_abort, NULL);
		  e_dialog_button_focus_num(dia, 1);
		  e_win_centered_set(dia->win, 1);
		  e_dialog_show(dia);
		  _e_sys_logout_begin_time = 0.0;
	       }
	     _e_sys_logout_timer = NULL;
	     return 0;
	  }
     }
   return 1;
   after:
   _e_sys_logout_after();
   _e_sys_logout_timer = NULL;
   return 0;
}

static void
_e_sys_logout_after(void)
{
   if (_e_sys_dialog)
     {
	e_object_del(E_OBJECT(_e_sys_dialog));
	_e_sys_dialog = NULL;
     }
   _e_sys_action_current = _e_sys_action_after;
   _e_sys_action_do(_e_sys_action_after, NULL);
   _e_sys_action_after = E_SYS_NONE;
}

static void
_e_sys_logout_begin(E_Sys_Action a_after)
{
   Eina_List *l;
   E_Obj_Dialog *od;

   /* start logout - at end do the a_after action */
   od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
			 _("Logout in progress"), "E", "_sys_logout");
   e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/logout");
   e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
				  _("Logout is currently in progress.<br>"
				    "<hilight>Please wait.</hilight>"));
   e_obj_dialog_show(od);
   e_obj_dialog_icon_set(od, "enlightenment/logout");
   if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
   _e_sys_dialog = od;
   _e_sys_action_after = a_after;
   for (l = e_border_client_list(); l; l = l->next)
     {
	E_Border *bd;

	bd = l->data;
	e_border_act_close_begin(bd);
     }
   /* and poll to see if all pending windows are gone yet every 0.5 sec */
   _e_sys_logout_begin_time = ecore_time_get();
   if (_e_sys_logout_timer) ecore_timer_del(_e_sys_logout_timer);
   _e_sys_logout_timer = ecore_timer_add(0.5, _e_sys_cb_logout_timer, NULL);
}

static void
_e_sys_current_action(void)
{
   /* display dialog that currently an action is in progress */
   E_Dialog *dia;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_sys_error_action_busy");
   if (!dia) return;

   e_dialog_title_set(dia, _("Enlightenment is busy with another request"));
   e_dialog_icon_set(dia, "enlightenment/sys", 64);
   switch (_e_sys_action_current)
     {
      case E_SYS_LOGOUT:
	e_dialog_text_set(dia,
			  _("Enlightenment is busy logging out.<br>"
			    "You cannot perform other system actions<br>"
			    "once a logout has begun.")
			  );
	break;
      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
	e_dialog_text_set(dia,
			  _("Enlightenment is shutting the system down.<br>"
			    "You cannot do any other system actions<br>"
			    "once a shutdown has been started.")
			  );
	break;
      case E_SYS_REBOOT:
	e_dialog_text_set(dia,
			  _("Enlightenment is rebooting the system.<br>"
			    "You cannot do any other system actions<br>"
			    "once a reboot has begun.")
			  );
	break;
      case E_SYS_SUSPEND:
	e_dialog_text_set(dia,
			  _("Enlightenment is suspending the system.<br>"
			    "Until suspend is complete you cannot perform<br>"
			    "any other system actions.")
			  );
	break;
      case E_SYS_HIBERNATE:
	e_dialog_text_set(dia,
			  _("Enlightenment is hibernating the system.<br>"
			    "You cannot perform an other system actions<br>"
			    "until this is complete.")
			  );
	break;
      default:
	e_dialog_text_set(dia,
			  _("EEK! This should not happen")
			  );
	break;
     }
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_e_sys_action_failed(void)
{
   /* display dialog that the current action failed */
   E_Dialog *dia;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_sys_error_action_failed");
   if (!dia) return;

   e_dialog_title_set(dia, _("Enlightenment is busy with another request"));
   e_dialog_icon_set(dia, "enlightenment/sys", 64);
   switch (_e_sys_action_current)
     {
      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
	e_dialog_text_set(dia,
			  _("Shutting down of your system failed.")
			  );
	break;
      case E_SYS_REBOOT:
	e_dialog_text_set(dia,
			  _("Rebooting your system failed.")
			  );
	break;
      case E_SYS_SUSPEND:
	e_dialog_text_set(dia,
			  _("Suspend of your system failed.")
			  );
	break;
      case E_SYS_HIBERNATE:
	e_dialog_text_set(dia,
			  _("Hibernating your system failed.")
			  );
	break;
      default:
	e_dialog_text_set(dia,
			  _("EEK! This should not happen")
			  );
	break;
     }
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static int
_e_sys_action_do(E_Sys_Action a, char *param)
{
   char buf[4096];
   E_Obj_Dialog *od;

   switch (a)
     {
      case E_SYS_EXIT:
	if (!e_util_immortal_check()) ecore_main_loop_quit();
	break;
      case E_SYS_RESTART:
	restart = 1;
	ecore_main_loop_quit();
	break;
      case E_SYS_EXIT_NOW:
	exit(0);
	break;
      case E_SYS_LOGOUT:
	_e_sys_logout_begin(E_SYS_EXIT);
	break;
      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
	/* shutdown -h now */
	if (e_util_immortal_check()) return 0;
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys halt",
		 e_prefix_bin_get());
	if (_e_sys_exe)
	  {
	     _e_sys_current_action();
	     return 0;
	  }
	else
	  {
	     _e_sys_exe = ecore_exe_run(buf, NULL);
	     od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
				   _("Shutting down"), "E", "_sys_halt");
	     e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/halt");
	     e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
					    _("Shutting down your Computer.<br>"
					      "<hilight>Please wait.</hilight>"));
	     e_obj_dialog_show(od);
	     e_obj_dialog_icon_set(od, "enlightenment/halt");
	     if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
	     e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
	     _e_sys_dialog = od;
	     /* FIXME: display halt status */
	  }
	break;
      case E_SYS_REBOOT:
	/* shutdown -r now */
	if (e_util_immortal_check()) return 0;
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys reboot",
		 e_prefix_bin_get());
	if (_e_sys_exe)
	  {
	     _e_sys_current_action();
	     return 0;
	  }
	else
	  {
	     _e_sys_exe = ecore_exe_run(buf, NULL);
	     od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
				   _("Rebooting"), "E", "_sys_reboot");
	     e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/reboot");
	     e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
					    _("Rebooting your Computer.<br>"
					      "<hilight>Please wait.</hilight>"));
	     e_obj_dialog_show(od);
	     e_obj_dialog_icon_set(od, "enlightenment/reboot");
	     if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
	     e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
	     _e_sys_dialog = od;
	     /* FIXME: display reboot status */
	  }
	break;
      case E_SYS_SUSPEND:
	/* /etc/acpi/sleep.sh force */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys suspend",
		 e_prefix_bin_get());
	if (_e_sys_exe)
	  {
	     _e_sys_current_action();
	     return 0;
	  }
	else
	  {
	     _e_sys_exe = ecore_exe_run(buf, NULL);
	     od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
				   _("Suspending"), "E", "_sys_suspend");
	     e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/suspend");
	     e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
					    _("Suspending your Computer.<br>"
					      "<hilight>Please wait.</hilight>"));
	     e_obj_dialog_show(od);
	     e_obj_dialog_icon_set(od, "enlightenment/suspend");
	     if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
	     e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
	     _e_sys_dialog = od;
	     /* FIXME: display suspend status */
	  }
	break;
      case E_SYS_HIBERNATE:
	/* /etc/acpi/hibernate.sh force */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys hibernate",
		 e_prefix_bin_get());
	if (_e_sys_exe)
	  {
	     _e_sys_current_action();
	     return 0;
	  }
	else
	  {
	     _e_sys_exe = ecore_exe_run(buf, NULL);
	     od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
				   _("Hibernating"), "E", "_sys_hibernate");
	     e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/hibernate");
	     e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
					    _("Hibernating your Computer.<br>"
					      "<hilight>Please wait.</hilight>"));
	     e_obj_dialog_show(od);
	     e_obj_dialog_icon_set(od, "enlightenment/hibernate");
	     if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
	     _e_sys_dialog = od;
	     e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
	     /* FIXME: display hibernate status */
	  }
	break;
      default:
	return 0;
     }
   return 1;
}

static void _e_sys_dialog_cb_delete(E_Obj_Dialog * od)
{
   /* If we don't NULL out the _e_sys_dialog, then the
    * ECORE_EXE_EVENT_DEL callback will trigger and segv if the window
    * is deleted in some other way. */
   _e_sys_dialog = NULL;
}

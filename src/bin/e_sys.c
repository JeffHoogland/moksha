/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_sys_cb_exit(void *data, int type, void *event);

static Ecore_Event_Handler *_e_sys_exe_exit_handler = NULL;
static int _e_sys_exe_pending = 0;
static Ecore_Exe *_e_sys_halt_check_exe = NULL;
static Ecore_Exe *_e_sys_reboot_check_exe = NULL;
static Ecore_Exe *_e_sys_suspend_check_exe = NULL;
static Ecore_Exe *_e_sys_hibernate_check_exe = NULL;
static int _e_sys_can_halt = 0;
static int _e_sys_can_reboot = 0;
static int _e_sys_can_suspend = 0;
static int _e_sys_can_hibernate = 0;

/* externally accessible functions */
EAPI int
e_sys_init(void)
{
   char buf[4096];
   
   /* this is not optimal - but it does work cleanly */
   _e_sys_exe_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
						     _e_sys_cb_exit, NULL);
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t halt", e_prefix_bin_get());
   _e_sys_halt_check_exe = ecore_exe_run(buf, NULL);
   _e_sys_exe_pending++;
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t reboot", e_prefix_bin_get());
   _e_sys_reboot_check_exe = ecore_exe_run(buf, NULL);
   _e_sys_exe_pending++;
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t suspend", e_prefix_bin_get());
   _e_sys_suspend_check_exe = ecore_exe_run(buf, NULL);
   _e_sys_exe_pending++;
   snprintf(buf, sizeof(buf), "%s/enlightenment_sys -t hibernate", e_prefix_bin_get());
   _e_sys_hibernate_check_exe = ecore_exe_run(buf, NULL);
   _e_sys_exe_pending++;
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
   char buf[4096];
   
   switch (a)
     {
      case E_SYS_EXIT:
	if (!e_util_immortal_check())
	  ecore_main_loop_quit();
	break;
      case E_SYS_RESTART:
	restart = 1;
	ecore_main_loop_quit();
	break;
      case E_SYS_EXIT_NOW:
	exit(0);
	break;
      case E_SYS_LOGOUT:
	/* FIXME: go through to every window and if it wants delete req - ask
	 * it to delete, otherwise just close it. set handler for window
	 * deletes, and once all windows are deleted - exit, OR if a timer
	 * expires - pop up dialog saying something is not responding
	 */
	break;
      case E_SYS_HALT:
	/* shutdown -h now */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys halt",
		 e_prefix_bin_get());
	ecore_exe_run(buf, NULL);
	/* FIXME: track command return value and have dialog */
	break;
      case E_SYS_REBOOT:
	/* shutdown -r now */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys reboot",
		 e_prefix_bin_get());
	ecore_exe_run(buf, NULL);
	/* FIXME: track command return value and have dialog */
	break;
      case E_SYS_SUSPEND:
	/* /etc/acpi/sleep.sh force */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys suspend",
		 e_prefix_bin_get());
	ecore_exe_run(buf, NULL);
	/* FIXME: track command return value and have dialog */
	break;
      case E_SYS_HIBERNATE:
	/* /etc/acpi/hibernate.sh force */
	snprintf(buf, sizeof(buf), "%s/enlightenment_sys hibernate",
		 e_prefix_bin_get());
	ecore_exe_run(buf, NULL);
	/* FIXME: track command return value and have dialog */
	break;
      default:
	return 0;
     }
   return 0;
}

/* local subsystem functions */
static int
_e_sys_cb_exit(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if ((_e_sys_halt_check_exe) && (ev->exe == _e_sys_halt_check_exe))
     {
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_halt = 1;
	     _e_sys_halt_check_exe = NULL;
	  }
	_e_sys_exe_pending--;
     }
   else if ((_e_sys_reboot_check_exe) && (ev->exe == _e_sys_reboot_check_exe))
     {
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_reboot = 1;
	     _e_sys_reboot_check_exe = NULL;
	  }
	_e_sys_exe_pending--;
     }
   else if ((_e_sys_suspend_check_exe) && (ev->exe == _e_sys_suspend_check_exe))
     {
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_suspend = 1;
	     _e_sys_suspend_check_exe = NULL;
	  }
	_e_sys_exe_pending--;
     }
   else if ((_e_sys_hibernate_check_exe) && (ev->exe == _e_sys_hibernate_check_exe))
     {
	if (ev->exit_code == 0)
	  {
	     _e_sys_can_hibernate = 1;
	     _e_sys_hibernate_check_exe = NULL;
	  }
	_e_sys_exe_pending--;
     }
   
   if (_e_sys_exe_pending <= 0)
     {
	if (_e_sys_exe_exit_handler)
	  ecore_event_handler_del(_e_sys_exe_exit_handler);
	_e_sys_exe_exit_handler = NULL;
     }
   return 1;
}

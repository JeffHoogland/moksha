#include "e.h"

/* local subsystem functions */
static Eina_Bool _e_sys_cb_timer(void *data);
static Eina_Bool _e_sys_cb_exit(void *data, int type, void *event);
static void      _e_sys_cb_logout_logout(void *data, E_Dialog *dia);
static void      _e_sys_cb_logout_wait(void *data, E_Dialog *dia);
static void      _e_sys_cb_logout_abort(void *data, E_Dialog *dia);
static Eina_Bool _e_sys_cb_logout_timer(void *data);
static void      _e_sys_logout_after(void);
static void      _e_sys_logout_begin(E_Sys_Action a_after, Eina_Bool raw);
static void      _e_sys_current_action(void);
static void      _e_sys_action_failed(void);
static int       _e_sys_action_do(E_Sys_Action a, char *param, Eina_Bool raw);
static void      _e_sys_dialog_cb_delete(E_Obj_Dialog *od);

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
static Eina_Bool _e_sys_action_after_raw = EINA_FALSE;
static Ecore_Exe *_e_sys_exe = NULL;
static double _e_sys_begin_time = 0.0;
static double _e_sys_logout_begin_time = 0.0;
static Ecore_Timer *_e_sys_logout_timer = NULL;
static E_Obj_Dialog *_e_sys_dialog = NULL;
static E_Dialog *_e_sys_logout_confirm_dialog = NULL;
static Ecore_Timer *_e_sys_susp_hib_check_timer = NULL;
static double _e_sys_susp_hib_check_last_tick = 0.0;
static void (*_e_sys_suspend_func)(void) = NULL;
static void (*_e_sys_hibernate_func)(void) = NULL;
static void (*_e_sys_reboot_func)(void) = NULL;
static void (*_e_sys_shutdown_func)(void) = NULL;
static void (*_e_sys_logout_func)(void) = NULL;
static void (*_e_sys_resume_func)(void) = NULL;

static const int E_LOGOUT_AUTO_TIME = 60;
static const int E_LOGOUT_WAIT_TIME = 15;

EAPI int E_EVENT_SYS_SUSPEND = -1;
EAPI int E_EVENT_SYS_HIBERNATE = -1;
EAPI int E_EVENT_SYS_RESUME = -1;

/* externally accessible functions */
EINTERN int
e_sys_init(void)
{
   E_EVENT_SYS_SUSPEND = ecore_event_type_new();
   E_EVENT_SYS_HIBERNATE = ecore_event_type_new();
   E_EVENT_SYS_RESUME = ecore_event_type_new();
   /* this is not optimal - but it does work cleanly */
   _e_sys_exe_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                     _e_sys_cb_exit, NULL);
   /* delay this for 1.0 seconds while the rest of e starts up */
   ecore_timer_add(1.0, _e_sys_cb_timer, NULL);
   return 1;
}

EINTERN int
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
        return 1;

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
   int ret = 0;

   if (_e_sys_action_current != E_SYS_NONE)
     {
        _e_sys_current_action();
        return 0;
     }
   e_config_save_flush();
   switch (a)
     {
      case E_SYS_EXIT:
      case E_SYS_RESTART:
      case E_SYS_EXIT_NOW:
      case E_SYS_LOGOUT:
      case E_SYS_SUSPEND:
      case E_SYS_HIBERNATE:
      case E_SYS_HALT_NOW:
        ret = _e_sys_action_do(a, param, EINA_FALSE);
        break;

      case E_SYS_HALT:
        if (!e_util_immortal_check())
          {
             if (_e_sys_shutdown_func) _e_sys_shutdown_func();
             else _e_sys_logout_begin(a, EINA_FALSE);
          }
        return 1;
        break;
      case E_SYS_REBOOT:
        if (!e_util_immortal_check())
          {
             if (_e_sys_reboot_func) _e_sys_reboot_func();
             else _e_sys_logout_begin(a, EINA_FALSE);
          }
        return 1;
        break;

      default:
        break;
     }

   if (ret) _e_sys_action_current = a;
   else _e_sys_action_current = E_SYS_NONE;

   return ret;
}

EAPI int
e_sys_action_raw_do(E_Sys_Action a, char *param)
{
   int ret = 0;

   if (_e_sys_action_current != E_SYS_NONE)
     {
        _e_sys_current_action();
        return 0;
     }
   e_config_save_flush();
   switch (a)
     {
      case E_SYS_EXIT:
      case E_SYS_RESTART:
      case E_SYS_EXIT_NOW:
      case E_SYS_LOGOUT:
      case E_SYS_SUSPEND:
      case E_SYS_HIBERNATE:
      case E_SYS_HALT_NOW:
        ret = _e_sys_action_do(a, param, EINA_TRUE);
        break;

      case E_SYS_HALT:
      case E_SYS_REBOOT:
        if (!e_util_immortal_check()) _e_sys_logout_begin(a, EINA_TRUE);
        return 1;
        break;

      default:
        break;
     }

   if (ret) _e_sys_action_current = a;
   else _e_sys_action_current = E_SYS_NONE;

   return ret;
}

static Eina_List *extra_actions = NULL;

EAPI E_Sys_Con_Action *
e_sys_con_extra_action_register(const char *label,
                                const char *icon_group,
                                const char *button_name,
                                void (*func)(void *data),
                                const void *data)
{
   E_Sys_Con_Action *sca;

   sca = E_NEW(E_Sys_Con_Action, 1);
   if (label)
     sca->label = eina_stringshare_add(label);
   if (icon_group)
     sca->icon_group = eina_stringshare_add(icon_group);
   if (button_name)
     sca->button_name = eina_stringshare_add(button_name);
   sca->func = func;
   sca->data = data;
   extra_actions = eina_list_append(extra_actions, sca);
   return sca;
}

EAPI void
e_sys_con_extra_action_unregister(E_Sys_Con_Action *sca)
{
   extra_actions = eina_list_remove(extra_actions, sca);
   if (sca->label) eina_stringshare_del(sca->label);
   if (sca->icon_group) eina_stringshare_del(sca->icon_group);
   if (sca->button_name) eina_stringshare_del(sca->button_name);
   free(sca);
}

EAPI const Eina_List *
e_sys_con_extra_action_list_get(void)
{
   return extra_actions;
}

EAPI void
e_sys_handlers_set(void (*suspend_func)(void),
                   void (*hibernate_func)(void),
                   void (*reboot_func)(void),
                   void (*shutdown_func)(void),
                   void (*logout_func)(void),
                   void (*resume_func)(void))
{
   _e_sys_suspend_func = suspend_func;
   _e_sys_hibernate_func = hibernate_func;
   _e_sys_reboot_func = reboot_func;
   _e_sys_shutdown_func = shutdown_func;
   _e_sys_logout_func = logout_func;
   _e_sys_resume_func = resume_func;
}

static Eina_Bool
_e_sys_susp_hib_check_timer_cb(void *data __UNUSED__)
{
   double t = ecore_time_unix_get();

   if ((t - _e_sys_susp_hib_check_last_tick) > 0.2)
     {
        _e_sys_susp_hib_check_timer = NULL;
        if (_e_sys_dialog)
          {
             e_object_del(E_OBJECT(_e_sys_dialog));
             _e_sys_dialog = NULL;
          }
        ecore_event_add(E_EVENT_SYS_RESUME, NULL, NULL, NULL);
        if (_e_sys_resume_func) _e_sys_resume_func();
        return EINA_FALSE;
     }
   _e_sys_susp_hib_check_last_tick = t;
   return EINA_TRUE;
}

static void
_e_sys_susp_hib_check(void)
{
   if (_e_sys_susp_hib_check_timer)
     ecore_timer_del(_e_sys_susp_hib_check_timer);
   _e_sys_susp_hib_check_last_tick = ecore_time_unix_get();
   _e_sys_susp_hib_check_timer =
     ecore_timer_add(0.1, _e_sys_susp_hib_check_timer_cb, NULL);
}

/* local subsystem functions */
static Eina_Bool
_e_sys_cb_timer(void *data __UNUSED__)
{
   /* exec out sys helper and ask it to test if we are allowed to do these
    * things
    */
   char buf[4096];

   e_init_status_set(_("Checking System Permissions"));
   snprintf(buf, sizeof(buf),
            "%s/enlightenment/utils/enlightenment_sys -t halt",
            e_prefix_lib_get());
   _e_sys_halt_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf),
            "%s/enlightenment/utils/enlightenment_sys -t reboot",
            e_prefix_lib_get());
   _e_sys_reboot_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf),
            "%s/enlightenment/utils/enlightenment_sys -t suspend",
            e_prefix_lib_get());
   _e_sys_suspend_check_exe = ecore_exe_run(buf, NULL);
   snprintf(buf, sizeof(buf),
            "%s/enlightenment/utils/enlightenment_sys -t hibernate",
            e_prefix_lib_get());
   _e_sys_hibernate_check_exe = ecore_exe_run(buf, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_sys_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
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
        return ECORE_CALLBACK_RENEW;
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
   return ECORE_CALLBACK_RENEW;
}

static void
_e_sys_cb_logout_logout(void *data __UNUSED__, E_Dialog *dia)
{
   if (_e_sys_logout_timer)
     {
        ecore_timer_del(_e_sys_logout_timer);
        _e_sys_logout_timer = NULL;
     }
   _e_sys_logout_begin_time = 0.0;
   _e_sys_logout_after();
   e_object_del(E_OBJECT(dia));
   _e_sys_logout_confirm_dialog = NULL;
}

static void
_e_sys_cb_logout_wait(void *data __UNUSED__, E_Dialog *dia)
{
   if (_e_sys_logout_timer) ecore_timer_del(_e_sys_logout_timer);
   _e_sys_logout_timer = ecore_timer_add(0.5, _e_sys_cb_logout_timer, NULL);
   _e_sys_logout_begin_time = ecore_time_get();
   e_object_del(E_OBJECT(dia));
   _e_sys_logout_confirm_dialog = NULL;
}

static void
_e_sys_cb_logout_abort(void *data __UNUSED__, E_Dialog *dia)
{
   if (_e_sys_logout_timer)
     {
        ecore_timer_del(_e_sys_logout_timer);
        _e_sys_logout_timer = NULL;
     }
   _e_sys_logout_begin_time = 0.0;
   e_object_del(E_OBJECT(dia));
   _e_sys_logout_confirm_dialog = NULL;
   _e_sys_action_current = E_SYS_NONE;
   _e_sys_action_after = E_SYS_NONE;
   _e_sys_action_after_raw = EINA_FALSE;
   if (_e_sys_dialog)
     {
        e_object_del(E_OBJECT(_e_sys_dialog));
        _e_sys_dialog = NULL;
     }
}

static void
_e_sys_logout_confirm_dialog_update(int remaining)
{
   char txt[4096];

   if (!_e_sys_logout_confirm_dialog)
     {
        fputs("ERROR: updating logout confirm dialog, but none exists!\n",
              stderr);
        return;
     }

   snprintf(txt, sizeof(txt),
            _("Logout is taking too long.<br>"
              "Some applications refuse to close.<br>"
              "Do you want to finish the logout<br>"
              "anyway without closing these<br>"
              "applications first?<br><br>"
              "Auto logout in %d seconds."), remaining);

   e_dialog_text_set(_e_sys_logout_confirm_dialog, txt);
}

static Eina_Bool
_e_sys_cb_logout_timer(void *data __UNUSED__)
{
   Eina_List *l;
   E_Border *bd;
   int pending = 0;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd->internal) pending++;
     }
   if (pending == 0) goto after;
   else if (_e_sys_logout_confirm_dialog)
     {
        int remaining = E_LOGOUT_AUTO_TIME -
          round(ecore_loop_time_get() - _e_sys_logout_begin_time);
        /* it has taken 60 (E_LOGOUT_AUTO_TIME) seconds of waiting the
         * confirm dialog and we still have apps that will not go
         * away. Do the action as user may be far away or forgot it.
         *
         * NOTE: this is the behavior for many operating systems and I
         *       guess the reason is people that hit "shutdown" and
         *       put their laptops in their backpacks in the hope
         *       everything will be turned off properly.
         */
        if (remaining > 0)
          {
             _e_sys_logout_confirm_dialog_update(remaining);
             return ECORE_CALLBACK_RENEW;
          }
        else
          {
             _e_sys_cb_logout_logout(NULL, _e_sys_logout_confirm_dialog);
             return ECORE_CALLBACK_CANCEL;
          }
     }
   else
     {
        /* it has taken 15 seconds of waiting and we still have apps that
         * will not go away
         */
        double now = ecore_loop_time_get();
        if ((now - _e_sys_logout_begin_time) > E_LOGOUT_WAIT_TIME)
          {
             E_Dialog *dia;

             dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_sys_error_logout_slow");
             if (dia)
               {
                  _e_sys_logout_confirm_dialog = dia;
                  e_dialog_title_set(dia, _("Logout problems"));
                  e_dialog_icon_set(dia, "system-log-out", 64);
                  e_dialog_button_add(dia, _("Logout now"), NULL,
                                      _e_sys_cb_logout_logout, NULL);
                  e_dialog_button_add(dia, _("Wait longer"), NULL,
                                      _e_sys_cb_logout_wait, NULL);
                  e_dialog_button_add(dia, _("Cancel Logout"), NULL,
                                      _e_sys_cb_logout_abort, NULL);
                  e_dialog_button_focus_num(dia, 1);
                  _e_sys_logout_confirm_dialog_update(E_LOGOUT_AUTO_TIME);
                  e_win_centered_set(dia->win, 1);
                  e_dialog_show(dia);
                  _e_sys_logout_begin_time = now;
               }
             if (_e_sys_resume_func) _e_sys_resume_func();
             return ECORE_CALLBACK_RENEW;
          }
     }
   return ECORE_CALLBACK_RENEW;
after:
   _e_sys_logout_after();
   _e_sys_logout_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
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
   _e_sys_action_do(_e_sys_action_after, NULL, _e_sys_action_after_raw);
   _e_sys_action_after = E_SYS_NONE;
   _e_sys_action_after_raw = EINA_FALSE;
}

static void
_e_sys_logout_begin(E_Sys_Action a_after, Eina_Bool raw)
{
   Eina_List *l;
   E_Border *bd;
   E_Obj_Dialog *od;

   /* start logout - at end do the a_after action */
   if (!raw)
     {
        od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
                              _("Logout in progress"), "E", "_sys_logout");
        e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/logout");
        e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
                                       _("Logout in progress.<br>"
                                         "<hilight>Please wait.</hilight>"));
        e_obj_dialog_show(od);
        e_obj_dialog_icon_set(od, "system-log-out");
        if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
        _e_sys_dialog = od;
     }
   _e_sys_action_after = a_after;
   _e_sys_action_after_raw = raw;
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
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

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                      "E", "_sys_error_action_busy");
   if (!dia) return;

   e_dialog_title_set(dia, _("Moksha is busy with another request"));
   e_dialog_icon_set(dia, "enlightenment/sys", 64);
   switch (_e_sys_action_current)
     {
      case E_SYS_LOGOUT:
        e_dialog_text_set(dia, _("Logging out.<br>"
                                 "You cannot perform other system actions<br>"
                                 "once a logout has begun."));
        break;

      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
        e_dialog_text_set(dia, _("Powering off.<br>"
                                 "You cannot do any other system actions<br>"
                                 "once a shutdown has been started."));
        break;

      case E_SYS_REBOOT:
        e_dialog_text_set(dia, _("Resetting.<br>"
                                 "You cannot do any other system actions<br>"
                                 "once a reboot has begun."));
        break;

      case E_SYS_SUSPEND:
        e_dialog_text_set(dia, _("Suspending.<br>"
                                 "Until suspend is complete you cannot perform<br>"
                                 "any other system actions."));
        break;

      case E_SYS_HIBERNATE:
        e_dialog_text_set(dia, _("Hibernating.<br>"
                                 "You cannot perform any other system actions<br>"
                                 "until this is complete."));
        break;

      default:
        e_dialog_text_set(dia, _("EEK! This should not happen"));
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

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                      "E", "_sys_error_action_failed");
   if (!dia) return;

   e_dialog_title_set(dia, _("Moksha is busy with another request"));
   e_dialog_icon_set(dia, "enlightenment/sys", 64);
   switch (_e_sys_action_current)
     {
      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
        e_dialog_text_set(dia, _("Power off failed."));
        break;

      case E_SYS_REBOOT:
        e_dialog_text_set(dia, _("Reset failed."));
        break;

      case E_SYS_SUSPEND:
        e_dialog_text_set(dia, _("Suspend failed."));
        break;

      case E_SYS_HIBERNATE:
        e_dialog_text_set(dia, _("Hibernate failed."));
        break;

      default:
        e_dialog_text_set(dia, _("EEK! This should not happen"));
        break;
     }
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static int
_e_sys_action_do(E_Sys_Action a, char *param __UNUSED__, Eina_Bool raw)
{
   char buf[PATH_MAX];
   E_Obj_Dialog *od;

   switch (a)
     {
      case E_SYS_EXIT:
        // XXX TODO: check for e_fm_op_registry entries and confirm
        if (!e_util_immortal_check())
          ecore_main_loop_quit();
        else
          return 0;
        break;

      case E_SYS_RESTART:
        // XXX TODO: check for e_fm_op_registry entries and confirm
        // FIXME: we dont   share out immortal info to restarted e. :(
//	if (!e_util_immortal_check())
      {
         restart = 1;
         ecore_main_loop_quit();
      }
//        else
//          return 0;
      break;

      case E_SYS_EXIT_NOW:
        exit(0);
        break;

      case E_SYS_LOGOUT:
        // XXX TODO: check for e_fm_op_registry entries and confirm
        if (raw)
          {
             _e_sys_logout_begin(E_SYS_EXIT, raw);
          }
        else
          {
             if (_e_sys_logout_func)
               {
                  _e_sys_logout_func();
                  return 0;
               }
             else
               {
                  _e_sys_logout_begin(E_SYS_EXIT, raw);
               }
          }
        break;

      case E_SYS_HALT:
      case E_SYS_HALT_NOW:
        /* shutdown -h now */
        if (e_util_immortal_check()) return 0;
        snprintf(buf, sizeof(buf),
                 "%s/enlightenment/utils/enlightenment_sys halt",
                 e_prefix_lib_get());
        if (_e_sys_exe)
          {
             if ((ecore_time_get() - _e_sys_begin_time) > 2.0)
               _e_sys_current_action();
             return 0;
          }
        else
          {
             if (raw)
               {
                  _e_sys_begin_time = ecore_time_get();
                  _e_sys_exe = ecore_exe_run(buf, NULL);
               }
             else
               {
                  if (_e_sys_shutdown_func)
                    {
                       _e_sys_shutdown_func();
                       return 0;
                    }
                  else
                    {
                       _e_sys_begin_time = ecore_time_get();
                       _e_sys_exe = ecore_exe_run(buf, NULL);
                       od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
                                             _("Power off"), "E", "_sys_halt");
                       e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/halt");
                       e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
                                                      _("Power off.<br>"
                                                        "<hilight>Please wait.</hilight>"));
                       e_obj_dialog_show(od);
                       e_obj_dialog_icon_set(od, "system-shutdown");
                       if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
                       e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
                       _e_sys_dialog = od;
                    }
               }
             /* FIXME: display halt status */
          }
        break;

      case E_SYS_REBOOT:
        /* shutdown -r now */
        if (e_util_immortal_check()) return 0;
        snprintf(buf, sizeof(buf),
                 "%s/enlightenment/utils/enlightenment_sys reboot",
                 e_prefix_lib_get());
        if (_e_sys_exe)
          {
             if ((ecore_time_get() - _e_sys_begin_time) > 2.0)
               _e_sys_current_action();
             return 0;
          }
        else
          {
             if (raw)
               {
                  _e_sys_begin_time = ecore_time_get();
                  _e_sys_exe = ecore_exe_run(buf, NULL);
               }
             else
               {
                  if (_e_sys_reboot_func)
                    {
                       _e_sys_reboot_func();
                       return 0;
                    }
                  else
                    {
                       _e_sys_begin_time = ecore_time_get();
                       _e_sys_exe = ecore_exe_run(buf, NULL);
                       od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
                                             _("Resetting"), "E", "_sys_reboot");
                       e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/reboot");
                       e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
                                                      _("Resetting.<br>"
                                                        "<hilight>Please wait.</hilight>"));
                       e_obj_dialog_show(od);
                       e_obj_dialog_icon_set(od, "system-restart");
                       if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
                       e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
                       _e_sys_dialog = od;
                    }
               }
             /* FIXME: display reboot status */
          }
        break;

      case E_SYS_SUSPEND:
        /* /etc/acpi/sleep.sh force */
        snprintf(buf, sizeof(buf),
                 "%s/enlightenment/utils/enlightenment_sys suspend",
                 e_prefix_lib_get());
        if (_e_sys_exe)
          {
             if ((ecore_time_get() - _e_sys_begin_time) > 2.0)
               _e_sys_current_action();
             return 0;
          }
        else
          {
             if (raw)
               {
                  _e_sys_susp_hib_check();
                  if (e_config->desklock_on_suspend)
                    e_desklock_show(EINA_TRUE);
                  _e_sys_begin_time = ecore_time_get();
                  _e_sys_exe = ecore_exe_run(buf, NULL);
               }
             else
               {
                  ecore_event_add(E_EVENT_SYS_SUSPEND, NULL, NULL, NULL);
                  if (_e_sys_suspend_func)
                    {
                       _e_sys_suspend_func();
                       return 0;
                    }
                  else
                    {
                       if (e_config->desklock_on_suspend)
                         e_desklock_show(EINA_TRUE);

                       _e_sys_susp_hib_check();

                       _e_sys_begin_time = ecore_time_get();
                       _e_sys_exe = ecore_exe_run(buf, NULL);
                       od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
                                             _("Suspending"), "E", "_sys_suspend");
                       e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/suspend");
                       e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
                                                      _("Suspending.<br>"
                                                        "<hilight>Please wait.</hilight>"));
                       e_obj_dialog_show(od);
                       e_obj_dialog_icon_set(od, "system-suspend");
                       if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
                       e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
                       _e_sys_dialog = od;
                    }
               }
             /* FIXME: display suspend status */
          }
        break;

      case E_SYS_HIBERNATE:
        /* /etc/acpi/hibernate.sh force */
        snprintf(buf, sizeof(buf),
                 "%s/enlightenment/utils/enlightenment_sys hibernate",
                 e_prefix_lib_get());
        if (_e_sys_exe)
          {
             if ((ecore_time_get() - _e_sys_begin_time) > 2.0)
               _e_sys_current_action();
             return 0;
          }
        else
          {
                  if (raw)
                    {
                  _e_sys_susp_hib_check();
                       if (e_config->desklock_on_suspend)
                         e_desklock_show(EINA_TRUE);
                       _e_sys_begin_time = ecore_time_get();
                       _e_sys_exe = ecore_exe_run(buf, NULL);
               }
             else
               {
                  ecore_event_add(E_EVENT_SYS_HIBERNATE, NULL, NULL, NULL);
                  if (_e_sys_hibernate_func)
                    {
                       _e_sys_hibernate_func();
                       return 0;
                    }
                  else
                    {
                       if (e_config->desklock_on_suspend)
                         e_desklock_show(EINA_TRUE);

                       _e_sys_susp_hib_check();

                       _e_sys_begin_time = ecore_time_get();
                       _e_sys_exe = ecore_exe_run(buf, NULL);
                       od = e_obj_dialog_new(e_container_current_get(e_manager_current_get()),
                                             _("Hibernating"), "E", "_sys_hibernate");
                       e_obj_dialog_obj_theme_set(od, "base/theme/sys", "e/sys/hibernate");
                       e_obj_dialog_obj_part_text_set(od, "e.textblock.message",
                                                      _("Hibernating.<br>"
                                                        "<hilight>Please wait.</hilight>"));
                       e_obj_dialog_show(od);
                       e_obj_dialog_icon_set(od, "system-suspend-hibernate");
                       if (_e_sys_dialog) e_object_del(E_OBJECT(_e_sys_dialog));
                       _e_sys_dialog = od;
                       e_obj_dialog_cb_delete_set(od, _e_sys_dialog_cb_delete);
                    }
               }
             /* FIXME: display hibernate status */
          }
        break;

      default:
        return 0;
     }
   return 1;
}

static void
_e_sys_dialog_cb_delete(E_Obj_Dialog *od __UNUSED__)
{
   /* If we don't NULL out the _e_sys_dialog, then the
    * ECORE_EXE_EVENT_DEL callback will trigger and segv if the window
    * is deleted in some other way. */
   _e_sys_dialog = NULL;
}

#include "e.h"

struct _E_Powersave_Deferred_Action
{
   void          (*func)(void *data);
   const void   *data;
   unsigned char delete_me :1;
};

struct _E_Powersave_Sleeper
{
   Ecore_Pipe *pipe;
   int         fd;
};

/* local subsystem functions */
static void      _e_powersave_sleeper_cb_dummy(void *data __UNUSED__, void *buffer __UNUSED__, unsigned int bytes __UNUSED__);
static Eina_Bool _e_powersave_cb_deferred_timer(void *data);
static void      _e_powersave_mode_eval(void);
static void      _e_powersave_event_update_free(void *data __UNUSED__, void *event);
static void      _e_powersave_event_change_send(E_Powersave_Mode mode);
static void      _e_powersave_sleepers_wake(void);

/* local subsystem globals */
EAPI int E_EVENT_POWERSAVE_UPDATE = 0;
EAPI int E_EVENT_POWERSAVE_CONFIG_UPDATE = 0;
static int walking_deferred_actions = 0;
static Eina_List *deferred_actions = NULL;
static Ecore_Timer *deferred_timer = NULL;
static E_Powersave_Mode powersave_mode = E_POWERSAVE_MODE_LOW;
static E_Powersave_Mode powersave_mode_force = E_POWERSAVE_MODE_NONE;
static double defer_time = 5.0;
static Eina_Bool powersave_force = EINA_FALSE;
static Eina_List *powersave_sleepers = NULL;
static Eina_Bool powersave_deferred_suspend = EINA_FALSE;
static Eina_Bool powersave_deferred_hibernate = EINA_FALSE;

/* externally accessible functions */
EINTERN int
e_powersave_init(void)
{
   _e_powersave_mode_eval();
   E_EVENT_POWERSAVE_UPDATE = ecore_event_type_new();
   E_EVENT_POWERSAVE_CONFIG_UPDATE = ecore_event_type_new();
   return 1;
}

EINTERN int
e_powersave_shutdown(void)
{
   return 1;
}

EAPI E_Powersave_Deferred_Action *
e_powersave_deferred_action_add(void (*func)(void *data), const void *data)
{
   E_Powersave_Deferred_Action *pa;

   pa = calloc(1, sizeof(E_Powersave_Deferred_Action));
   if (!pa) return NULL;
   if (deferred_timer) ecore_timer_del(deferred_timer);
   deferred_timer = ecore_timer_loop_add(defer_time,
                                         _e_powersave_cb_deferred_timer,
                                         NULL);
   pa->func = func;
   pa->data = data;
   deferred_actions = eina_list_append(deferred_actions, pa);
   return pa;
}

EAPI void
e_powersave_deferred_action_del(E_Powersave_Deferred_Action *pa)
{
   if (walking_deferred_actions)
     {
        pa->delete_me = 1;
        return;
     }
   else
     {
        deferred_actions = eina_list_remove(deferred_actions, pa);
        free(pa);
        if (!deferred_actions)
          {
             if (deferred_timer)
               {
                  ecore_timer_del(deferred_timer);
                  deferred_timer = NULL;
               }
          }
     }
}

EAPI void
e_powersave_mode_set(E_Powersave_Mode mode)
{
   if (mode < e_config->powersave.min) mode = e_config->powersave.min;
   else if (mode > e_config->powersave.max) mode = e_config->powersave.max;

   if (powersave_mode == mode) return;
   powersave_mode = mode;

   if (powersave_force) return;
   _e_powersave_event_change_send(powersave_mode);
   _e_powersave_mode_eval();
}

EAPI E_Powersave_Mode
e_powersave_mode_get(void)
{
   if (powersave_force) return powersave_mode_force;
   return powersave_mode;
}

EAPI void
e_powersave_mode_force(E_Powersave_Mode mode)
{
   if (mode == powersave_mode_force) return;
   powersave_force = EINA_TRUE;
   powersave_mode_force = mode;
   _e_powersave_event_change_send(powersave_mode_force);
   _e_powersave_mode_eval();
}

EAPI void
e_powersave_mode_unforce(void)
{
   if (!powersave_force) return;
   powersave_force = EINA_FALSE;
   if (powersave_mode_force != powersave_mode)
     {
        _e_powersave_event_change_send(powersave_mode);
        _e_powersave_mode_eval();
     }
   powersave_mode_force = E_POWERSAVE_MODE_NONE;
}

EAPI E_Powersave_Sleeper *
e_powersave_sleeper_new(void)
{
   E_Powersave_Sleeper *sleeper;

   sleeper = E_NEW(E_Powersave_Sleeper, 1);
   sleeper->pipe = ecore_pipe_add(_e_powersave_sleeper_cb_dummy, NULL);
   if (!sleeper->pipe)
     {
        eina_freeq_ptr_add(eina_freeq_main_get(), sleeper, free, sizeof(*sleeper));
        return NULL;
     }
   sleeper->fd = ecore_pipe_read_fd(sleeper->pipe);
   ecore_pipe_freeze(sleeper->pipe);
   powersave_sleepers = eina_list_append(powersave_sleepers, sleeper);
   return (E_Powersave_Sleeper *)sleeper;
}

EAPI void
e_powersave_sleeper_free(E_Powersave_Sleeper *sleeper)
{
   if (!sleeper) return;
   ecore_pipe_del(sleeper->pipe);
   powersave_sleepers = eina_list_remove(powersave_sleepers, sleeper);
   eina_freeq_ptr_add(eina_freeq_main_get(), sleeper, free, sizeof(*sleeper));
}

EAPI void
e_powersave_sleeper_sleep(E_Powersave_Sleeper *sleeper, int poll_interval)
{
   double timf;
   unsigned int tim;
   fd_set rfds, wfds, exfds;
   struct timeval tv;
   int ret;
   char buf[1] = { 1 };

   if (!sleeper) return;
   if (e_powersave_mode_get() == E_POWERSAVE_MODE_FREEZE) timf = 3600;
   else timf = (double)poll_interval / 8.0;
   FD_ZERO(&rfds);
   FD_ZERO(&wfds);
   FD_ZERO(&exfds);
   FD_SET(sleeper->fd, &rfds);

   tim = ((timf - fmod(ecore_time_get(), timf)) * 1000000.0);
   tv.tv_sec = tim / 1000000;
   tv.tv_usec = tim % 1000000;
   for (;;)
     {
        ret = select(sleeper->fd + 1, &rfds, &wfds, &exfds, &tv);
        if ((ret == 1) && (FD_ISSET(sleeper->fd, &rfds)))
          {
             if (read(sleeper->fd, buf, 1) < 0)
               fprintf(stderr, "%s: ERROR READING FROM FD\n", __func__);
             return;
          }
        else if (ret == 0)
          {
             return;
          }
     }
}

EAPI void
e_powersave_defer_suspend(void)
{
   powersave_deferred_suspend = EINA_TRUE;
}

EAPI void
e_powersave_defer_hibernate(void)
{
   powersave_deferred_hibernate = EINA_TRUE;
}

EAPI void
e_powersave_defer_cancel(void)
{
   powersave_deferred_suspend = EINA_FALSE;
   powersave_deferred_hibernate = EINA_FALSE;
}

/* local subsystem functions */

static void
_e_powersave_sleepers_wake(void)
{
   E_Powersave_Sleeper *sleeper;
   Eina_List *l;
   char buf[1] = { 1 };

   EINA_LIST_FOREACH(powersave_sleepers, l, sleeper)
     {
        if (write(ecore_pipe_write_fd(sleeper->pipe), buf, 1) < 0)
          fprintf(stderr, "%s: ERROR WRITING TO FD\n", __func__);
        ecore_pipe_write(sleeper->pipe, buf, 1);
     }
}

static void
_e_powersave_sleeper_cb_dummy(void *data __UNUSED__, void *buffer __UNUSED__, unsigned int bytes __UNUSED__)
{
}

/* local subsystem functions */

static Eina_Bool
_e_powersave_cb_deferred_timer(void *data __UNUSED__)
{
   E_Powersave_Deferred_Action *pa;

   walking_deferred_actions++;
   EINA_LIST_FREE(deferred_actions, pa)
     {
        if (!pa->delete_me) pa->func((void *)pa->data);
        free(pa);
     }
   walking_deferred_actions--;
   if (!deferred_actions) deferred_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_powersave_mode_eval(void)
{
   double t = 0.0;
   E_Powersave_Mode mode;

   if (powersave_force) mode = powersave_mode_force;
   else mode = powersave_mode;

   switch (mode)
     {
      case E_POWERSAVE_MODE_NONE:
        t = e_config->powersave.none; /* time to defer "power expensive" activities */
        break;

      case E_POWERSAVE_MODE_LOW:
        t = e_config->powersave.low;
        break;

      case E_POWERSAVE_MODE_MEDIUM:
        t = e_config->powersave.medium;
        break;

      case E_POWERSAVE_MODE_HIGH:
        t = e_config->powersave.high;
        break;

      case E_POWERSAVE_MODE_EXTREME:
        t = e_config->powersave.extreme;
        break;

      case E_POWERSAVE_MODE_FREEZE:
        t = 3600;
        break;

      default:
        return;
        break;
     }
   if (!EINA_DBL_EQ(t, defer_time))
     {
        if (deferred_timer) ecore_timer_del(deferred_timer);
        deferred_timer = ecore_timer_loop_add(defer_time,
                                              _e_powersave_cb_deferred_timer,
                                              NULL);
        defer_time = t;
     }
}

static void
_e_powersave_event_update_free(void *data __UNUSED__, void *event)
{
   E_Powersave_Mode mode;

   if (powersave_force) mode = powersave_mode_force;
   else mode = powersave_mode;
   free(event);

   if (mode > E_POWERSAVE_MODE_LOW)
     {
        if (powersave_deferred_hibernate)
          e_sys_action_do(E_SYS_HIBERNATE, NULL);
        else if (powersave_deferred_suspend)
          e_sys_action_do(E_SYS_SUSPEND, NULL);
     }
   powersave_deferred_hibernate = EINA_FALSE;
   powersave_deferred_suspend = EINA_FALSE;
}

static void
_e_powersave_event_change_send(E_Powersave_Mode mode)
{
   E_Event_Powersave_Update *ev;

   printf("CHANGE PW SAVE MODE TO %i / %i\n",
          (int)mode, E_POWERSAVE_MODE_EXTREME);
   ev = E_NEW(E_Event_Powersave_Update, 1);
   ev->mode = mode;
   ecore_event_add(E_EVENT_POWERSAVE_UPDATE, ev, _e_powersave_event_update_free, NULL);
   _e_powersave_sleepers_wake();
}

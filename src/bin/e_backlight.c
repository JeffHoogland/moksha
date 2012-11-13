#include "e.h"
#ifdef HAVE_EEZE
# include <Eeze.h>
#endif

// FIXME: backlight should be tied per zone but this implementation is just
// a signleton right now as thats 99% of use cases. but api supports
// doing more. for now make it work in the singleton

#define MODE_NONE  -1
#define MODE_RANDR 0
#define MODE_SYS   1

static double bl_val = 1.0;
static double bl_animval = 1.0;
static int sysmode = MODE_NONE;
static Ecore_Animator *bl_anim = NULL;
static Eina_List *bl_devs = NULL;

static Ecore_Event_Handler *_e_backlight_handler_config_mode = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_fullscreen = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_unfullscreen = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_remove = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_iconify = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_uniconify = NULL;
static Ecore_Event_Handler *_e_backlight_handler_border_desk_set = NULL;
static Ecore_Event_Handler *_e_backlight_handler_desk_show = NULL;

static Ecore_Timer *_e_backlight_timer = NULL;

static void _e_backlight_update(E_Zone *zone);
static void _e_backlight_set(E_Zone *zone, double val);
static Eina_Bool _bl_anim(void *data, double pos);
static Eina_Bool bl_avail = EINA_FALSE;
static Eina_Bool _e_backlight_handler(void *d, int type, void *ev);
static Eina_Bool _e_backlight_timer_cb(void *d);
#ifdef HAVE_EEZE
static const char *bl_sysval = NULL;
static Ecore_Event_Handler *bl_sys_exit_handler = NULL;
static Ecore_Exe *bl_sys_set_exe = NULL;
static Eina_Bool bl_sys_pending_set = EINA_FALSE;
static Eina_Bool bl_sys_set_exe_ready = EINA_TRUE;

static void _bl_sys_find(void);
static void _bl_sys_level_get(void);
static Eina_Bool _e_bl_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _bl_sys_level_set(double val);
#endif

EAPI int E_EVENT_BACKLIGHT_CHANGE = -1;

EINTERN int
e_backlight_init(void)
{
#ifdef HAVE_EEZE
   eeze_init();
#endif
// why did someone do this? this makes it ONLY work if xrandr has bl support.
// WRONG!
//   bl_avail = ecore_x_randr_output_backlight_available();
   bl_avail = EINA_TRUE;

   _e_backlight_handler_config_mode = ecore_event_handler_add
     (E_EVENT_CONFIG_MODE_CHANGED, _e_backlight_handler, NULL);

   _e_backlight_handler_border_fullscreen = ecore_event_handler_add
     (E_EVENT_BORDER_FULLSCREEN, _e_backlight_handler, NULL);

   _e_backlight_handler_border_unfullscreen = ecore_event_handler_add
     (E_EVENT_BORDER_UNFULLSCREEN, _e_backlight_handler, NULL);

   _e_backlight_handler_border_remove = ecore_event_handler_add
     (E_EVENT_BORDER_REMOVE, _e_backlight_handler, NULL);

   _e_backlight_handler_border_iconify = ecore_event_handler_add
     (E_EVENT_BORDER_ICONIFY, _e_backlight_handler, NULL);

   _e_backlight_handler_border_uniconify = ecore_event_handler_add
     (E_EVENT_BORDER_UNICONIFY, _e_backlight_handler, NULL);

   _e_backlight_handler_border_desk_set = ecore_event_handler_add
     (E_EVENT_BORDER_DESK_SET, _e_backlight_handler, NULL);

   _e_backlight_handler_desk_show = ecore_event_handler_add
     (E_EVENT_DESK_SHOW, _e_backlight_handler, NULL);

   if (bl_avail)
     {
        e_backlight_update();
        if (!getenv("E_RESTART"))
          {
             e_backlight_level_set(NULL, 0.0, 0.0);
             e_backlight_level_set(NULL, e_config->backlight.normal, 1.0);
          }
     }

   E_EVENT_BACKLIGHT_CHANGE = ecore_event_type_new();

   return 1;
}

EINTERN int
e_backlight_shutdown(void)
{
   const char *s;
   
   if (bl_anim) ecore_animator_del(bl_anim);
   bl_anim = NULL;
   EINA_LIST_FREE(bl_devs, s) eina_stringshare_del(s);
#ifdef HAVE_EEZE
   if (bl_sysval) eina_stringshare_del(bl_sysval);
   bl_sysval = NULL;
   if (bl_sys_exit_handler) ecore_event_handler_del(bl_sys_exit_handler);
   bl_sys_exit_handler = NULL;
   bl_sys_set_exe = NULL;
   bl_sys_pending_set = EINA_FALSE;
   eeze_shutdown();
#endif
   if (_e_backlight_handler_config_mode)
     {
        ecore_event_handler_del(_e_backlight_handler_config_mode);
        _e_backlight_handler_config_mode = NULL;
     }

   if (_e_backlight_handler_border_fullscreen)
     {
        ecore_event_handler_del(_e_backlight_handler_border_fullscreen);
        _e_backlight_handler_border_fullscreen = NULL;
     }

   if (_e_backlight_handler_border_unfullscreen)
     {
        ecore_event_handler_del(_e_backlight_handler_border_unfullscreen);
        _e_backlight_handler_border_unfullscreen = NULL;
     }

   if (_e_backlight_handler_border_remove)
     {
        ecore_event_handler_del(_e_backlight_handler_border_remove);
        _e_backlight_handler_border_remove = NULL;
     }

   if (_e_backlight_handler_border_iconify)
     {
        ecore_event_handler_del(_e_backlight_handler_border_iconify);
        _e_backlight_handler_border_iconify = NULL;
     }

   if (_e_backlight_handler_border_uniconify)
     {
        ecore_event_handler_del(_e_backlight_handler_border_uniconify);
        _e_backlight_handler_border_uniconify = NULL;
     }

   if (_e_backlight_handler_border_desk_set)
     {
        ecore_event_handler_del(_e_backlight_handler_border_desk_set);
        _e_backlight_handler_border_desk_set = NULL;
     }

   if (_e_backlight_handler_desk_show)
     {
        ecore_event_handler_del(_e_backlight_handler_desk_show);
        _e_backlight_handler_desk_show = NULL;
     }
   return 1;
}

EAPI Eina_Bool
e_backlight_exists(void)
{
   if (sysmode == MODE_NONE) return EINA_FALSE;
   return EINA_TRUE;
}

EAPI void
e_backlight_update(void)
{
   Eina_List *m, *c, *z;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   if (bl_avail == EINA_FALSE) return;

   EINA_LIST_FOREACH(e_manager_list(), m, man)
     {
        EINA_LIST_FOREACH(man->containers, c, con)
          {
             EINA_LIST_FOREACH(con->zones, z, zone)
               {
                  _e_backlight_update(zone);
               }
          }
     }

   /* idle dimming disabled: clear timer */
   if (!e_config->backlight.idle_dim)
     {
        if (_e_backlight_timer)
          ecore_timer_del(_e_backlight_timer);
        _e_backlight_timer = NULL;
        return;
     }
   /* dimming enabled, timer active: update interval and reset */
   if (_e_backlight_timer)
     {
        if (e_config->backlight.timer != ecore_timer_interval_get(_e_backlight_timer))
          ecore_timer_interval_set(_e_backlight_timer, e_config->backlight.timer);
        ecore_timer_reset(_e_backlight_timer);
        return;
     }
   /* dimming enabled, timer inactive: */

   /* timer is 0 seconds: return */
   if (!e_config->backlight.timer) return;
   /* current mode is dimmed: undim */
   if (e_config->backlight.mode == E_BACKLIGHT_MODE_DIM)
     e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_NORMAL);
   _e_backlight_timer = ecore_timer_add(e_config->backlight.timer, _e_backlight_timer_cb, NULL);
}

EAPI void
e_backlight_level_set(E_Zone *zone, double val, double tim)
{
   double bl_now;
   // zone == NULL == everything
   // set backlight associated with zone to val over period of tim
   // if tim == 0.0 - then do it instantnly, if time == -1 use some default
   // transition time
   if (val < 0.0) val = 0.0;
   else if (val > 1.0) val = 1.0;
   if (val == bl_val) return;
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   bl_now = bl_val;
   bl_val = val;
   if (e_config->backlight.mode != E_BACKLIGHT_MODE_NORMAL) return;
   if (tim < 0.0) tim = e_config->backlight.transition;
   ecore_event_add(E_EVENT_BACKLIGHT_CHANGE, NULL, NULL, NULL);
   if (tim == 0.0)
     {
        if (bl_anim)
          {
             ecore_animator_del(bl_anim);
             bl_anim = NULL;
          }
        _e_backlight_set(zone, val);
       return;
     }
   if (bl_anim) ecore_animator_del(bl_anim);
   bl_anim = ecore_animator_timeline_add(tim, _bl_anim, zone);
   bl_animval = bl_now;
}

EAPI double
e_backlight_level_get(E_Zone *zone __UNUSED__)
{
   // zone == NULL == everything
   return bl_val;
}

EAPI void
e_backlight_mode_set(E_Zone *zone, E_Backlight_Mode mode)
{
   // zone == NULL == everything
   if (e_config->backlight.mode == mode) return;
   e_config->backlight.mode = mode;
   if      (e_config->backlight.mode == E_BACKLIGHT_MODE_NORMAL)
      e_backlight_level_set(zone, bl_val, -1.0);
   else if (e_config->backlight.mode == E_BACKLIGHT_MODE_OFF)
      e_backlight_level_set(zone, 0.0, -1.0);
   else if (e_config->backlight.mode == E_BACKLIGHT_MODE_DIM)
      e_backlight_level_set(zone, e_config->backlight.dim, -1.0);
   else if (e_config->backlight.mode == E_BACKLIGHT_MODE_MAX)
      e_backlight_level_set(zone, 1.0, -1.0);
}

EAPI E_Backlight_Mode
e_backlight_mode_get(E_Zone *zone __UNUSED__)
{
   // zone == NULL == everything
   return e_config->backlight.mode;
}

EAPI const Eina_List *
e_backlight_devices_get(void)
{
   return bl_devs;
}

/* local subsystem functions */

static Eina_Bool
_e_backlight_handler(void *d __UNUSED__, int type __UNUSED__, void *ev __UNUSED__)
{
   e_backlight_update();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_backlight_timer_cb(void *d __UNUSED__)
{
   e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_DIM);
   _e_backlight_timer = NULL;
   return EINA_FALSE;
}

static void
_e_backlight_update(E_Zone *zone)
{
   double x_bl = -1.0;
   Ecore_X_Window root;
   Ecore_X_Randr_Output *out;
   int i, num = 0;

   root = zone->container->manager->root;
   // try randr
   out = ecore_x_randr_window_outputs_get(root, &num);
   if ((out) && (num > 0) && (ecore_x_randr_output_backlight_available()))
     {
        char *name;
        const char *s;
        Eina_Bool gotten = EINA_FALSE;
        
        EINA_LIST_FREE(bl_devs, s) eina_stringshare_del(s);
        for (i = 0; i < num; i++)
          {
             name = ecore_x_randr_output_name_get(root, out[i], NULL);
             bl_devs = eina_list_append(bl_devs, eina_stringshare_add(name));
             if ((name) && (e_config->backlight.sysdev) &&
                 (!strcmp(name, e_config->backlight.sysdev)))
               {
                  x_bl = ecore_x_randr_output_backlight_level_get(root, out[i]);
                  gotten = EINA_TRUE;
               }
             if (name) free(name);
          }
        if (!gotten)
          x_bl = ecore_x_randr_output_backlight_level_get(root, out[0]);
     }
   if (out) free(out);
   if (x_bl >= 0.0)
     {
        bl_val = x_bl;
        sysmode = MODE_RANDR;
        return;
     }
#ifdef HAVE_EEZE
   _bl_sys_find();
   if (bl_sysval)
     {
        sysmode = MODE_SYS;
        _bl_sys_level_get();
        return;
     }
#endif
}

static void
_e_backlight_set(E_Zone *zone, double val)
{
   if (sysmode == MODE_RANDR)
     {
        Ecore_X_Window root;
        Ecore_X_Randr_Output *out;
        int num = 0, i;
        char *name;

        root = zone->container->manager->root;
        out = ecore_x_randr_window_outputs_get(root, &num);
        if ((out) && (num > 0))
          {
             Eina_Bool gotten = EINA_FALSE;
             for (i = 0; i < num; i++)
               {
                  name = ecore_x_randr_output_name_get(root, out[i], NULL);
                  if (name)
                    {
                       if ((e_config->backlight.sysdev) &&
                           (!strcmp(name, e_config->backlight.sysdev)))
                         {
                            ecore_x_randr_output_backlight_level_set(root, out[i], val);
                            gotten = EINA_TRUE;
                         }
                       free(name);
                    }
               }
             if (!gotten)
               {
                  for (i = 0; i < num; i++)
                    ecore_x_randr_output_backlight_level_set(root, out[i], val);
               }
          }
        if (out) free(out);
     }
#ifdef HAVE_EEZE
   else if (sysmode == MODE_SYS)
     {
        if (bl_sysval)
          {
             _bl_sys_level_set(val);
          }
     }
#endif
}

static Eina_Bool
_bl_anim(void *data, double pos)
{
   E_Zone *zone = data;
   double v;

   // FIXME: if zone is deleted while anim going... bad things.
   pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_DECELERATE, 0.0, 0.0);
   v = (bl_animval * (1.0 - pos)) + (bl_val *pos);
   _e_backlight_set(zone, v);
   if (pos >= 1.0)
     {
        bl_anim = NULL;
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

#ifdef HAVE_EEZE
static void
_bl_sys_find(void)
{
   Eina_List *l, *devs, *pdevs = NULL;
   Eina_Bool use;
   const char *f, *s;
   int v;

   devs = eeze_udev_find_by_filter("backlight", NULL, NULL);
   if (!devs)
     {
        /* FIXME: need to make this more precise so we don't set keyboard LEDs or something */
        devs = eeze_udev_find_by_filter("leds", NULL, NULL);
        if (!devs) return;
     }
   if (eina_list_count(devs) > 1)
     {
        /* prefer backlights of type "firmware" where available */
        EINA_LIST_FOREACH(devs, l, f)
          {
             s = eeze_udev_syspath_get_sysattr(f, "type");
             use = (s && (!strcmp(s, "firmware")));
             eina_stringshare_del(s);
             if (!use) continue;
             s = eeze_udev_syspath_get_sysattr(f, "brightness");
             if (!s) continue;
             v = atoi(s);
             eina_stringshare_del(s);
             if (v < 0) continue;
             pdevs = eina_list_append(pdevs, eina_stringshare_add(f));
             eina_stringshare_del(f);
             l->data = NULL;
          }
        EINA_LIST_FOREACH(devs, l, f)
          {
             if (!l->data) continue;
             s = eeze_udev_syspath_get_sysattr(f, "brightness");
             if (!s) continue;
             v = atoi(s);
             eina_stringshare_del(s);
             if (v < 0) continue;
             pdevs = eina_list_append(pdevs, eina_stringshare_add(f));
          }
     }
   if (!pdevs)
     {
        /* add the other backlight or led's if none found */
        EINA_LIST_FOREACH(devs, l, f)
          {
             use = EINA_FALSE;
             s = eeze_udev_syspath_get_sysattr(f, "brightness");
             if (!s) continue;
             v = atoi(s);
             eina_stringshare_del(s);
             if (v < 0) continue;
             pdevs = eina_list_append(pdevs, eina_stringshare_add(f));
          }
     }
   /* clear out original devs list now we've filtered */
   EINA_LIST_FREE(devs, f)
     {
        if (f) eina_stringshare_del(f);
     }
   /* clear out old configured bl sysval */
   if (bl_sysval)
     {
        eina_stringshare_del(bl_sysval);
        bl_sysval = NULL;
     }
   EINA_LIST_FREE(bl_devs, s) eina_stringshare_del(s);
   /* if configured backlight is there - use it, or if not use first */
   EINA_LIST_FOREACH(pdevs, l, f)
     {
        bl_devs = eina_list_append(bl_devs, eina_stringshare_add(f));
        if (!bl_sysval)
          {
             if ((e_config->backlight.sysdev) &&
                 (!strcmp(e_config->backlight.sysdev, f)))
               bl_sysval = eina_stringshare_add(f);
          }
     }
   if (!bl_sysval)
     {
        EINA_LIST_FOREACH(pdevs, l, f)
          {
             if (!bl_sysval)
               bl_sysval = eina_stringshare_add(f);
          }
     }
   /* clear out preferred devs list */
   EINA_LIST_FREE(pdevs, f)
     {
        eina_stringshare_del(f);
     }
}

static void
_bl_sys_level_get(void)
{
   int maxval, val;
   const char *str;

   str = eeze_udev_syspath_get_sysattr(bl_sysval, "max_brightness");
   if (!str) return;

   maxval = atoi(str);
   eina_stringshare_del(str);
   if (maxval <= 0) maxval = 255;
   str = eeze_udev_syspath_get_sysattr(bl_sysval, "brightness");
   if (!str) return;

   val = atoi(str);
   eina_stringshare_del(str);
   if ((val >= 0) && (val <= maxval))
     bl_val = (double)val / (double)maxval;
// printf("GET: %i/%i (%1.3f)\n", val, maxval, bl_val);
}

static Eina_Bool
_e_bl_cb_ext_delay(void *data __UNUSED__)
{
   bl_sys_set_exe_ready = EINA_TRUE;
   if (bl_sys_pending_set)
     {
        bl_sys_pending_set = EINA_FALSE;
        _bl_sys_level_set(bl_val);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_bl_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe == bl_sys_set_exe)
     {
        bl_sys_set_exe_ready = EINA_FALSE;
        bl_sys_set_exe = NULL;
        ecore_timer_add(0.1, _e_bl_cb_ext_delay, NULL);
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_bl_sys_level_set(double val)
{
   char buf[PATH_MAX];

   if (!bl_sys_exit_handler)
      bl_sys_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                    _e_bl_cb_exit, NULL);
   if ((bl_sys_set_exe) || (!bl_sys_set_exe_ready))
     {
        bl_sys_pending_set = EINA_TRUE;
        return;
     }
//   printf("SET: %1.3f\n", val);
   snprintf(buf, sizeof(buf),
            "%s/enlightenment/utils/enlightenment_backlight %i %s",
            e_prefix_lib_get(), (int)(val * 1000.0), bl_sysval);
   bl_sys_set_exe = ecore_exe_run(buf, NULL);
}
#endif

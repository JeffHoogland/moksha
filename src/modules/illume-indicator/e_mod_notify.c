#include "e.h"
#include "e_mod_main.h"
#include "e_mod_notify.h"

/* local function prototypes */
static int _e_mod_notify_cb_add(E_Notification_Daemon *daemon __UNUSED__, E_Notification *n);
static void _e_mod_notify_cb_del(E_Notification_Daemon *daemon __UNUSED__, unsigned int id);
static Ind_Notify_Win *_e_mod_notify_find(unsigned int id);
static void _e_mod_notify_refresh(Ind_Notify_Win *nwin);
static Ind_Notify_Win *_e_mod_notify_merge(E_Notification *n);
static Ind_Notify_Win *_e_mod_notify_new(E_Notification *n);
static Eina_Bool _e_mod_notify_cb_timeout(void *data);
static void _e_mod_notify_cb_free(Ind_Notify_Win *nwin);
static void _e_mod_notify_cb_resize(E_Win *win);

/* local variables */
static E_Notification_Daemon *_notify_daemon = NULL;
static Eina_List *_nwins = NULL;
static int _notify_id = 0;

int 
e_mod_notify_init(void) 
{
   /* init notification subsystem */
   if (!e_notification_daemon_init()) return 0;

   _notify_daemon = 
     e_notification_daemon_add("illume-indicator", "Enlightenment");

   e_notification_daemon_callback_notify_set(_notify_daemon, 
                                             _e_mod_notify_cb_add);
   e_notification_daemon_callback_close_notification_set(_notify_daemon, 
                                                         _e_mod_notify_cb_del);

   return 1;
}

int 
e_mod_notify_shutdown(void) 
{
   Ind_Notify_Win *nwin;

   EINA_LIST_FREE(_nwins, nwin) 
     e_object_del(E_OBJECT(nwin));

   if (_notify_daemon) e_notification_daemon_free(_notify_daemon);

   /* shutdown notification subsystem */
   e_notification_daemon_shutdown();

   return 1;
}

static int 
_e_mod_notify_cb_add(E_Notification_Daemon *daemon __UNUSED__, E_Notification *n) 
{
   Ind_Notify_Win *nwin;
   unsigned int replace;
   double timeout;

   replace = e_notification_replaces_id_get(n);
   if (!replace) 
     {
        _notify_id++;
        e_notification_id_set(n, _notify_id);
     }
   else
     e_notification_id_set(n, replace);

   if ((replace) && (nwin = _e_mod_notify_find(replace)))
     {
        e_notification_ref(n);
        if (nwin->notify) e_notification_unref(nwin->notify);
        nwin->notify = n;
     }
   else if (!replace) 
     nwin = _e_mod_notify_merge(n);

   if (!nwin) 
     {
        if (!(nwin = _e_mod_notify_new(n)))
          return _notify_id;
        _nwins = eina_list_append(_nwins, nwin);
     }

   if (nwin)
     _e_mod_notify_refresh(nwin);

   /* show it */
   ecore_x_e_illume_quickpanel_state_send(nwin->zone->black_win, 
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_ON);

   if (nwin->timer) ecore_timer_del(nwin->timer);
   nwin->timer = NULL;

   timeout = e_notification_timeout_get(nwin->notify);
   if (timeout < 0) timeout = 3000.0;
   timeout = timeout / 1000.0;

   if (timeout > 0)
     nwin->timer = ecore_timer_add(timeout, _e_mod_notify_cb_timeout, nwin);

   return _notify_id;
}

static void 
_e_mod_notify_cb_del(E_Notification_Daemon *daemon __UNUSED__, unsigned int id) 
{
   const Eina_List *l;
   Ind_Notify_Win *nwin;

   EINA_LIST_FOREACH(_nwins, l, nwin) 
     {
        if (e_notification_id_get(nwin->notify) == id) 
          {
             e_object_del(E_OBJECT(nwin));
             _nwins = eina_list_remove_list(_nwins, (Eina_List *)l);
          }
     }
}

static 
Ind_Notify_Win *
_e_mod_notify_find(unsigned int id) 
{
   const Eina_List *l;
   Ind_Notify_Win *nwin;

   EINA_LIST_FOREACH(_nwins, l, nwin) 
     if ((e_notification_id_get(nwin->notify) == id))
       return nwin;
   return NULL;
}

static void 
_e_mod_notify_refresh(Ind_Notify_Win *nwin) 
{
   const char *icon, *tmp;
   Evas_Coord mw, mh;
   int size;

   if (!nwin) return;

   if (nwin->o_icon) 
     {
        edje_object_part_unswallow(nwin->o_base, nwin->o_icon);
        evas_object_del(nwin->o_icon);
     }

   size = (48 * e_scale);
   if ((icon = e_notification_app_icon_get(nwin->notify)))
     {
        if (!strncmp(icon, "file://", 7)) 
          {
             icon += 7;
             nwin->o_icon = e_util_icon_add(icon, nwin->win->evas);
          }
        else 
          {
             nwin->o_icon = 
               e_util_icon_theme_icon_add(icon, size, nwin->win->evas);
          }
     }
   else 
     {
        E_Notification_Image *img;

        if ((img = e_notification_hint_icon_data_get(nwin->notify))) 
          {
             nwin->o_icon = 
               e_notification_image_evas_object_add(nwin->win->evas, img);
             evas_object_image_fill_set(nwin->o_icon, 0, 0, size, size);
          }
     }

   if (nwin->o_icon) 
     {
        evas_object_resize(nwin->o_icon, size, size);
        edje_extern_object_min_size_set(nwin->o_icon, size, size);
        edje_extern_object_max_size_set(nwin->o_icon, size, size);
        edje_object_part_swallow(nwin->o_base, "e.swallow.icon", nwin->o_icon);
     }

   tmp = e_notification_summary_get(nwin->notify);
   edje_object_part_text_set(nwin->o_base, "e.text.title", tmp);

   tmp = e_notification_body_get(nwin->notify);
   edje_object_part_text_set(nwin->o_base, "e.text.message", tmp);

   edje_object_calc_force(nwin->o_base);
   edje_object_size_min_calc(nwin->o_base, &mw, &mh);

   e_win_size_min_set(nwin->win, nwin->zone->w, mh);
}

static Ind_Notify_Win *
_e_mod_notify_merge(E_Notification *n) 
{
   Ind_Notify_Win *nwin;
   const Eina_List *l;
   const char *appname, *bold, *bnew;

   if (!n) return NULL;
   if (!(appname = e_notification_app_name_get(n))) return NULL;
   EINA_LIST_FOREACH(_nwins, l, nwin) 
     {
        const char *name;

        if (!nwin->notify) continue;
        if (!(name = e_notification_app_name_get(nwin->notify))) continue;
        if (!strcmp(appname, name)) break;
     }
   if (!nwin) return NULL;

   bold = e_notification_body_get(nwin->notify);
   bnew = e_notification_body_get(n);

   /* if the bodies are the same, skip merging */
   if (!strcmp(bold, bnew)) return nwin;

   e_notification_body_set(n, bnew);

   e_notification_unref(nwin->notify);
   nwin->notify = n;
   e_notification_ref(nwin->notify);

   return nwin;
}

static Ind_Notify_Win *
_e_mod_notify_new(E_Notification *n) 
{
   Ind_Notify_Win *nwin;
   Ecore_X_Window_State states[2];
   E_Zone *zone;

   nwin = E_OBJECT_ALLOC(Ind_Notify_Win, IND_NOTIFY_WIN_TYPE, 
                         _e_mod_notify_cb_free);
   if (!nwin) return NULL;

   e_notification_ref(n);
   nwin->notify = n;

   zone = e_util_zone_current_get(e_manager_current_get());
   nwin->zone = zone;

   nwin->win = e_win_new(zone->container);
   nwin->win->data = nwin;

   e_win_name_class_set(nwin->win, "Illume-Notify", "Illume-Notify");
   e_win_no_remember_set(nwin->win, EINA_TRUE);
   e_win_resize_callback_set(nwin->win, _e_mod_notify_cb_resize);

   ecore_x_e_illume_quickpanel_set(nwin->win->evas_win, EINA_TRUE);
   ecore_x_e_illume_quickpanel_priority_major_set(nwin->win->evas_win, 
                                                  e_notification_hint_urgency_get(n));
   ecore_x_e_illume_quickpanel_zone_set(nwin->win->evas_win, zone->id);

   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(nwin->win->evas_win, states, 2);
   ecore_x_icccm_hints_set(nwin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);

   nwin->o_base = edje_object_add(nwin->win->evas);
   if (!e_theme_edje_object_set(nwin->o_base, 
                                "base/theme/modules/illume-indicator", 
                                "modules/illume-indicator/notify")) 
     {
        char buff[PATH_MAX];

        snprintf(buff, sizeof(buff), 
                 "%s/e-module-illume-indicator.edj", _ind_mod_dir);
        edje_object_file_set(nwin->o_base, buff, 
                             "modules/illume-indicator/notify");
     }
   evas_object_move(nwin->o_base, 0, 0);
   evas_object_show(nwin->o_base);

   e_win_size_min_set(nwin->win, zone->w, 48);
   e_win_show(nwin->win);
   e_border_zone_set(nwin->win->border, zone);
   nwin->win->border->user_skip_winlist = 1;

   return nwin;
}

static Eina_Bool 
_e_mod_notify_cb_timeout(void *data) 
{
   Ind_Notify_Win *nwin;

   if (!(nwin = data)) return EINA_FALSE;

   /* hide it */
   ecore_x_e_illume_quickpanel_state_send(nwin->zone->black_win, 
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);

   _nwins = eina_list_remove(_nwins, nwin);
   e_object_del(E_OBJECT(nwin));
   return EINA_FALSE;
}

static void 
_e_mod_notify_cb_free(Ind_Notify_Win *nwin) 
{
   if (nwin->timer) ecore_timer_del(nwin->timer);
   nwin->timer = NULL;
   if (nwin->o_icon) evas_object_del(nwin->o_icon);
   nwin->o_icon = NULL;
   if (nwin->o_base) evas_object_del(nwin->o_base);
   nwin->o_base = NULL;
   if (nwin->win) e_object_del(E_OBJECT(nwin->win));
   nwin->win = NULL;
   e_notification_closed_set(nwin->notify, EINA_TRUE);
   e_notification_daemon_signal_notification_closed(_notify_daemon, 
                                                    e_notification_id_get(nwin->notify), 
                                                    E_NOTIFICATION_CLOSED_REQUESTED);
   e_notification_unref(nwin->notify);
   E_FREE(nwin);
}

static void 
_e_mod_notify_cb_resize(E_Win *win) 
{
   Ind_Notify_Win *nwin;

   if (!(nwin = win->data)) return;
   if (nwin->o_base) evas_object_resize(nwin->o_base, win->w, win->h);
}

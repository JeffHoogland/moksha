#include "e.h"
#ifdef HAVE_PAM
# include <security/pam_appl.h>
# include <pwd.h>
#endif

#define E_DESKLOCK_STATE_DEFAULT  0
#define E_DESKLOCK_STATE_CHECKING 1
#define E_DESKLOCK_STATE_INVALID  2

#define PASSWD_LEN                256

/**************************** private data ******************************/
typedef struct _E_Desklock_Data       E_Desklock_Data;
typedef struct _E_Desklock_Popup_Data E_Desklock_Popup_Data;
#ifdef HAVE_PAM
typedef struct _E_Desklock_Auth       E_Desklock_Auth;
#endif

typedef struct _E_Desklock_Run  E_Desklock_Run;

struct _E_Desklock_Popup_Data
{
   E_Popup     *popup_wnd;
   Evas_Object *bg_object;
   Evas_Object *login_box;
};

struct _E_Desklock_Data
{
   Eina_List     *elock_wnd_list;
   Ecore_X_Window elock_wnd;
   Eina_List     *handlers;
   Ecore_Event_Handler *move_handler;
   Ecore_X_Window elock_grab_break_wnd;
   char           passwd[PASSWD_LEN];
   int            state;
   Eina_Bool      selected : 1;
};

struct _E_Desklock_Run
{
   E_Order *desk_run;
   int position;
};

#ifdef HAVE_PAM
struct _E_Desklock_Auth
{
   struct
   {
      struct pam_conv conv;
      pam_handle_t   *handle;
   } pam;

   char user[4096];
   char passwd[4096];
};
#endif

static E_Desklock_Data *edd = NULL;
static E_Zone *last_active_zone = NULL;
#ifdef HAVE_PAM
static Ecore_Event_Handler *_e_desklock_exit_handler = NULL;
static pid_t _e_desklock_child_pid = -1;
#endif
static Ecore_Exe *_e_custom_desklock_exe = NULL;
static Ecore_Event_Handler *_e_custom_desklock_exe_handler = NULL;
static Ecore_Poller *_e_desklock_idle_poller = NULL;
static int _e_desklock_user_idle = 0;
static double _e_desklock_autolock_time = 0.0;
static E_Dialog *_e_desklock_ask_presentation_dia = NULL;
static int _e_desklock_ask_presentation_count = 0;

static Ecore_Event_Handler *_e_desklock_run_handler = NULL;
static Ecore_Job *job = NULL;
static Eina_List *tasks = NULL;

/***********************************************************************/

static Eina_Bool _e_desklock_cb_key_down(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_mouse_move(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_custom_desklock_exit(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_idle_poller(void *data);
static Eina_Bool _e_desklock_cb_window_stack(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_zone_add(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_zone_del(void *data, int type, void *event);
static Eina_Bool _e_desklock_cb_zone_move_resize(void *data, int type, void *event);

static Eina_Bool _e_desklock_cb_run(void *data, int type, void *event);

static void      _e_desklock_popup_free(E_Desklock_Popup_Data *edp);
static void      _e_desklock_popup_add(E_Zone *zone);
static void      _e_desklock_login_box_add(E_Desklock_Popup_Data *edp);
static void      _e_desklock_select(void);
static void      _e_desklock_unselect(void);
static void      _e_desklock_null(void);
static void      _e_desklock_passwd_update(void);
static void      _e_desklock_backspace(void);
static void      _e_desklock_delete(void);
static int       _e_desklock_zone_num_get(void);
static int       _e_desklock_check_auth(void);
static void      _e_desklock_state_set(int state);

static Eina_Bool _e_desklock_state = EINA_FALSE;
#ifdef HAVE_PAM
static Eina_Bool _e_desklock_cb_exit(void *data, int type, void *event);
static int       _desklock_auth(char *passwd);
static int       _desklock_pam_init(E_Desklock_Auth *da);
static int       _desklock_auth_pam_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr);
static char     *_desklock_auth_get_current_user(void);
static char     *_desklock_auth_get_current_host(void);
#endif

static void      _e_desklock_ask_presentation_mode(void);

EAPI int E_EVENT_DESKLOCK = 0;

EINTERN int
e_desklock_init(void)
{
   Eina_List *l;
   E_Config_Desklock_Background *bg;
   /* A poller to tick every 256 ticks, watching for an idle user */
   _e_desklock_idle_poller = ecore_poller_add(ECORE_POLLER_CORE, 256,
                                              _e_desklock_cb_idle_poller, NULL);

   if (e_config->desklock_background)
     e_filereg_register(e_config->desklock_background);
   EINA_LIST_FOREACH(e_config->desklock_backgrounds, l, bg)
     e_filereg_register(bg->file);

   E_EVENT_DESKLOCK = ecore_event_type_new();

   _e_desklock_run_handler = ecore_event_handler_add(E_EVENT_DESKLOCK,
						     _e_desklock_cb_run, NULL);

   return 1;
}

EINTERN int
e_desklock_shutdown(void)
{
   Eina_Bool waslocked = EINA_FALSE;
   E_Desklock_Run *task;
   Eina_List *l;
   E_Config_Desklock_Background *bg;

   if (edd) waslocked = EINA_TRUE;
   if (!x_fatal)
     e_desklock_hide();
   if (e_config->desklock_background)
     e_filereg_deregister(e_config->desklock_background);

   if (waslocked) e_util_env_set("E_DESKLOCK_LOCKED", "locked");

   ecore_event_handler_del(_e_desklock_run_handler);
   _e_desklock_run_handler = NULL;

   if (job) ecore_job_del(job);
   job = NULL;

   if (e_config->desklock_background)
     e_filereg_deregister(e_config->desklock_background);
   EINA_LIST_FOREACH(e_config->desklock_backgrounds, l, bg)
     e_filereg_deregister(bg->file);

   EINA_LIST_FREE(tasks, task)
     {
        e_object_del(E_OBJECT(task->desk_run));
        free(task);
     }

   return 1;
}

static const char *
_user_wallpaper_get(E_Zone *zone)
{
   const E_Config_Desktop_Background *cdbg;
   const Eina_List *l;
   E_Desk *desk;

   desk = e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cdbg)
     {
        if ((cdbg->container > -1) && (cdbg->container != (int)zone->container->num)) continue;
        if ((cdbg->zone > -1) && (cdbg->zone != (int)zone->num)) continue;
        if ((cdbg->desk_x > -1) && (cdbg->desk_x != desk->x)) continue;
        if ((cdbg->desk_y > -1) && (cdbg->desk_y != desk->y)) continue;
        if (cdbg->file) return cdbg->file;
     }

   if (e_config->desktop_default_background)
     return e_config->desktop_default_background;

   return e_theme_edje_file_get("base/theme/desklock", "e/desklock/background");
}

EAPI int
e_desklock_show_autolocked(void)
{
   if (e_util_fullscreen_current_any()) return 0;
   if (_e_desklock_autolock_time < 1.0)
     _e_desklock_autolock_time = ecore_loop_time_get();
   return e_desklock_show(EINA_FALSE);
}

EAPI int
e_desklock_show(Eina_Bool suspend)
{
   Eina_List *managers, *l, *l2, *l3;
   E_Manager *man;
   int total_zone_num;
   E_Event_Desklock *ev;

   if (_e_custom_desklock_exe) return 0;
   /* Disable desk_lock for guest account with no password
    *   It is assumed that any user with HOME in tmp is guest session */
   if(strncmp(getenv("HOME"), "/tmp/", 5) == 0) return 0;
   if (e_config->desklock_use_custom_desklock && e_config->desklock_custom_desklock_cmd && e_config->desklock_custom_desklock_cmd[0])
     {
        _e_custom_desklock_exe_handler =
          ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                  _e_desklock_cb_custom_desklock_exit, NULL);
        if (e_config->desklock_language)
          e_intl_language_set(e_config->desklock_language);

        if (e_config->xkb.lock_layout)
          e_xkb_layout_set(e_config->xkb.lock_layout);
        _e_custom_desklock_exe =
          e_util_exe_safe_run(e_config->desklock_custom_desklock_cmd, NULL);
        _e_desklock_state = EINA_TRUE;
        return 1;
     }

#ifndef HAVE_PAM
   e_util_dialog_show(_("Error - no PAM support"),
                      _("No PAM support was built into Moksha, so<br>"
                        "desk locking is disabled."));
   return 0;
#endif

   if (edd) return 0;

#ifdef HAVE_PAM
   if (e_config->desklock_auth_method == 1)
     {
#endif
   if (!e_config->desklock_personal_passwd)
     {
        E_Zone *zone;

        zone = e_util_zone_current_get(e_manager_current_get());
        if (zone)
          e_configure_registry_call("screen/screen_lock", zone->container, NULL);
        return 0;
     }
#ifdef HAVE_PAM
}

#endif

   edd = E_NEW(E_Desklock_Data, 1);
   if (!edd) return 0;
   edd->elock_wnd = ecore_x_window_input_new(e_manager_current_get()->root, 0, 0, 1, 1);
   ecore_x_window_show(edd->elock_wnd);
   managers = e_manager_list();
   if (!e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
     {
        EINA_LIST_FOREACH(managers, l, man)
          {
             Ecore_X_Window *windows;
             int wnum;

             windows = ecore_x_window_children_get(man->root, &wnum);
             if (windows)
               {
                  int i;

                  for (i = 0; i < wnum; i++)
                    {
                       Ecore_X_Window_Attributes att;

                       memset(&att, 0, sizeof(Ecore_X_Window_Attributes));
                       ecore_x_window_attributes_get(windows[i], &att);
                       if (att.visible)
                         {
                            ecore_x_window_hide(windows[i]);
                            if (e_grabinput_get(edd->elock_wnd, 0, edd->elock_wnd))
                              {
                                 edd->elock_grab_break_wnd = windows[i];
                                 free(windows);
                                 goto works;
                              }
                            ecore_x_window_show(windows[i]);
                         }
                    }
                  free(windows);
               }
          }
        /* everything failed - can't lock */
        e_util_dialog_show(_("Lock Failed"),
                           _("Locking the desktop failed because some application<br>"
                             "has grabbed either the keyboard or the mouse or both<br>"
                             "and their grab is unable to be broken."));
        ecore_x_window_free(edd->elock_wnd);
        E_FREE(edd);
        return 0;
     }
works:
   if (e_config->desklock_language)
     e_intl_language_set(e_config->desklock_language);

   if (e_config->xkb.lock_layout)
     e_xkb_layout_set(e_config->xkb.lock_layout);

   total_zone_num = _e_desklock_zone_num_get();
   EINA_LIST_FOREACH(managers, l, man)
     {
        E_Container *con;

        EINA_LIST_FOREACH(man->containers, l2, con)
          {
             E_Zone *zone;
             EINA_LIST_FOREACH(con->zones, l3, zone)
               _e_desklock_popup_add(zone);
          }
     }

   /* handlers */
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                              _e_desklock_cb_key_down, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK,
                                              _e_desklock_cb_window_stack, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,
                                              _e_desklock_cb_window_stack, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,
                                              _e_desklock_cb_window_stack, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(E_EVENT_ZONE_ADD,
                                              _e_desklock_cb_zone_add, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(E_EVENT_ZONE_DEL,
                                              _e_desklock_cb_zone_del, NULL));
   edd->handlers =
     eina_list_append(edd->handlers,
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
                                              _e_desklock_cb_zone_move_resize, NULL));

   if ((total_zone_num > 1) && (e_config->desklock_login_box_zone == -2))
     edd->move_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _e_desklock_cb_mouse_move, NULL);

   _e_desklock_passwd_update();

   ev = E_NEW(E_Event_Desklock, 1);
   ev->on = 1;
   ev->suspend = suspend;
   ecore_event_add(E_EVENT_DESKLOCK, ev, NULL, NULL);

   e_util_env_set("E_DESKLOCK_LOCKED", "locked");
   _e_desklock_state = EINA_TRUE;
   return 1;
}

EAPI void
e_desklock_hide(void)
{
   E_Desklock_Popup_Data *edp;
   E_Event_Desklock *ev;

   if ((!edd) && (!_e_custom_desklock_exe)) return;

   if (e_config->desklock_language)
     e_intl_language_set(e_config->language);

   if (e_config_xkb_layout_eq(e_config->xkb.current_layout, e_config->xkb.lock_layout))
     {
        if (e_config->xkb.sel_layout)
          e_xkb_layout_set(e_config->xkb.sel_layout);
     }

   _e_desklock_state = EINA_FALSE;
   ev = E_NEW(E_Event_Desklock, 1);
   ev->on = 0;
   ev->suspend = 1;
   ecore_event_add(E_EVENT_DESKLOCK, ev, NULL, NULL);

   if (e_config->desklock_use_custom_desklock)
     {
        _e_custom_desklock_exe = NULL;
        return;
     }

   if (!edd) return;
   if (edd->elock_grab_break_wnd)
     ecore_x_window_show(edd->elock_grab_break_wnd);

   EINA_LIST_FREE(edd->elock_wnd_list, edp)
     _e_desklock_popup_free(edp);

   E_FREE_LIST(edd->handlers, ecore_event_handler_del);
   if (edd->move_handler) ecore_event_handler_del(edd->move_handler);

   e_grabinput_release(edd->elock_wnd, edd->elock_wnd);
   ecore_x_window_free(edd->elock_wnd);

   E_FREE(edd);
   edd = NULL;

   if (_e_desklock_autolock_time > 0.0)
     {
        if ((e_config->desklock_ask_presentation) &&
            (e_config->desklock_ask_presentation_timeout > 0.0))
          {
             double max, now;

             now = ecore_loop_time_get();
             max = _e_desklock_autolock_time + e_config->desklock_ask_presentation_timeout;
             if (now <= max)
               _e_desklock_ask_presentation_mode();
          }
        else
          _e_desklock_ask_presentation_count = 0;

        _e_desklock_autolock_time = 0.0;
     }
   e_util_env_set("E_DESKLOCK_LOCKED", "freefreefree");
}

static void
_e_desklock_popup_add(E_Zone *zone)
{
   E_Desklock_Popup_Data *edp;
   E_Config_Desklock_Background *cbg;
   const char *bg;

   cbg = eina_list_nth(e_config->desklock_backgrounds, zone->num);
   bg = cbg ? cbg->file : NULL;
   edp = E_NEW(E_Desklock_Popup_Data, 1);
   if (!edp)
     {
        CRI("DESKLOCK FAILED FOR ZONE %d!", zone->num);
        return;
     }

   edp->popup_wnd = e_popup_new(zone, 0, 0, zone->w, zone->h);
   e_popup_name_set(edp->popup_wnd, "_e_popup_desklock");
   ecore_evas_title_set(edp->popup_wnd->ecore_evas, "E Desklock");
   evas_event_feed_mouse_move(edp->popup_wnd->evas, -1000000, -1000000,
                              ecore_x_current_time_get(), NULL);

   e_popup_layer_set(edp->popup_wnd, E_LAYER_PRIO);
   ecore_evas_raise(edp->popup_wnd->ecore_evas);

   evas_event_freeze(edp->popup_wnd->evas);
   edp->bg_object = edje_object_add(edp->popup_wnd->evas);

   if ((!bg) || (!strcmp(bg, "theme_desklock_background")))
     {
        e_theme_edje_object_set(edp->bg_object,
                                "base/theme/desklock",
                                "e/desklock/background");
     }
   else if (!strcmp(bg, "theme_background"))
     {
        e_theme_edje_object_set(edp->bg_object,
                                "base/theme/backgrounds",
                                "e/desktop/background");
     }
   else
     {
        const char *f;

        if (!strcmp(bg, "user_background"))
          f = _user_wallpaper_get(zone);
        else
          f = bg;

        if (e_util_edje_collection_exists(f, "e/desklock/background"))
          {
             edje_object_file_set(edp->bg_object, f, "e/desklock/background");
          }
        else
          {
             if (!edje_object_file_set(edp->bg_object,
                                       f, "e/desktop/background"))
               {
                  edje_object_file_set(edp->bg_object,
                                       e_theme_edje_file_get("base/theme/desklock",
                                                             "e/desklock/background"),
                                       "e/desklock/background");
               }
          }
     }

   evas_object_move(edp->bg_object, 0, 0);
   evas_object_resize(edp->bg_object, zone->w, zone->h);
   evas_object_show(edp->bg_object);

   _e_desklock_login_box_add(edp);
   e_popup_edje_bg_object_set(edp->popup_wnd, edp->bg_object);
   evas_event_thaw(edp->popup_wnd->evas);

   e_popup_show(edp->popup_wnd);

   edd->elock_wnd_list = eina_list_append(edd->elock_wnd_list, edp);
}

static void
_e_desklock_login_box_add(E_Desklock_Popup_Data *edp)
{
   int mw, mh;
   E_Zone *zone = edp->popup_wnd->zone;
   E_Zone *current_zone;
   int total_zone_num;

   last_active_zone = current_zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
   total_zone_num = _e_desklock_zone_num_get();
   if (total_zone_num > 1)
     {
        if ((e_config->desklock_login_box_zone == -2) && (zone != current_zone))
          return;
        if ((e_config->desklock_login_box_zone > -1) && (e_config->desklock_login_box_zone != (int)eina_list_count(edd->elock_wnd_list)))
          return;
     }

   edp->login_box = edje_object_add(edp->popup_wnd->evas);
   e_theme_edje_object_set(edp->login_box,
                           "base/theme/desklock",
                           "e/desklock/login_box");
   edje_object_part_text_set(edp->login_box, "e.text.title",
                             _("Please enter your unlock password"));
   edje_object_size_min_calc(edp->login_box, &mw, &mh);
   if (edje_object_part_exists(edp->bg_object, "e.swallow.login_box"))
     {
        evas_object_size_hint_min_set(edp->login_box, mw, mh);
        edje_object_part_swallow(edp->bg_object, "e.swallow.login_box", edp->login_box);
     }
   else
     {
        evas_object_resize(edp->login_box, mw, mh);
        evas_object_move(edp->login_box,
                         ((zone->w - mw) / 2),
                         ((zone->h - mh) / 2));
     }
   evas_object_show(edp->login_box);
}

static void
_e_desklock_popup_free(E_Desklock_Popup_Data *edp)
{
   if (!edp) return;
   e_popup_hide(edp->popup_wnd);

   evas_event_freeze(edp->popup_wnd->evas);
   evas_object_del(edp->bg_object);
   evas_object_del(edp->login_box);
   evas_event_thaw(edp->popup_wnd->evas);

   e_object_del(E_OBJECT(edp->popup_wnd));
   free(edp);
}

static Eina_Bool
_e_desklock_cb_window_stack(void *data __UNUSED__,
                            int type,
                            void *event)
{
   Ecore_X_Window win, win2 = 0;
   E_Desklock_Popup_Data *edp;
   Eina_List *l;
   Eina_Bool raise_win = EINA_TRUE;

   if (!edd) return ECORE_CALLBACK_PASS_ON;

   if (type == ECORE_X_EVENT_WINDOW_STACK)
     win = ((Ecore_X_Event_Window_Stack *)event)->event_win;
   else if (type == ECORE_X_EVENT_WINDOW_CONFIGURE)
     {
        win = ((Ecore_X_Event_Window_Configure *)event)->event_win;
        win2 = ((Ecore_X_Event_Window_Configure *)event)->win;
     }
   else if (type == ECORE_X_EVENT_WINDOW_CREATE)
     win = ((Ecore_X_Event_Window_Create *)event)->win;
   else
     return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     {
        if ((win == edp->popup_wnd->evas_win) ||
            ((win2) && (win2 == edp->popup_wnd->evas_win)))
          {
             raise_win = EINA_FALSE;
             break;
          }
     }

   if (raise_win)
     {
        EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
          ecore_evas_raise(edp->popup_wnd->ecore_evas);
     }

   return ECORE_CALLBACK_PASS_ON;
}

EAPI Eina_Bool
e_desklock_state_get(void)
{
   return _e_desklock_state;
}

static Eina_List *
_e_desklock_popup_find(E_Zone *zone)
{
   Eina_List *l;
   E_Desklock_Popup_Data *edp;

   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     if (edp->popup_wnd->zone == zone) return l;
   return NULL;
}

static Eina_Bool
_e_desklock_cb_zone_add(void *data __UNUSED__,
                        int type __UNUSED__,
                        void *event)
{
   E_Event_Zone_Add *ev = event;
   if (!edd) return ECORE_CALLBACK_PASS_ON;
   if ((!edd->move_handler) && (e_config->desklock_login_box_zone == -2))
     edd->move_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _e_desklock_cb_mouse_move, NULL);
   if (!_e_desklock_popup_find(ev->zone)) _e_desklock_popup_add(ev->zone);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_desklock_cb_zone_del(void *data __UNUSED__,
                        int type __UNUSED__,
                        void *event)
{
   E_Event_Zone_Del *ev = event;
   Eina_List *l;
   if (!edd) return ECORE_CALLBACK_PASS_ON;
   if ((eina_list_count(e_container_current_get(e_manager_current_get())->zones) == 1) && (e_config->desklock_login_box_zone == -2))
     edd->move_handler = ecore_event_handler_del(edd->move_handler);
        
   l = _e_desklock_popup_find(ev->zone);
   if (l)
     {
        _e_desklock_popup_free(l->data);
        edd->elock_wnd_list = eina_list_remove_list(edd->elock_wnd_list, l);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_desklock_cb_zone_move_resize(void *data __UNUSED__,
                                int type __UNUSED__,
                                void *event)
{
   E_Desklock_Popup_Data *edp;
   Eina_List *l;
   E_Event_Zone_Move_Resize *ev = event;

   if (!edd) return ECORE_CALLBACK_PASS_ON;
   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     if (edp->popup_wnd->zone == ev->zone)
       {
          e_popup_move_resize(edp->popup_wnd, 0, 0, ev->zone->w, ev->zone->h);
          evas_object_resize(edp->bg_object, ev->zone->w, ev->zone->h);
          break;
       }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_desklock_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;

   if ((ev->window != edd->elock_wnd) ||
       (edd->state == E_DESKLOCK_STATE_CHECKING)) return 1;

   if (!strcmp(ev->key, "Escape"))
     {
        if (edd->selected)
          {
             _e_desklock_unselect();
             return ECORE_CALLBACK_RENEW;
          }
     }
   else if (!strcmp(ev->key, "KP_Enter"))
     _e_desklock_check_auth();
   else if (!strcmp(ev->key, "Return"))
     _e_desklock_check_auth();
   else if (!strcmp(ev->key, "BackSpace"))
     {
        if (edd->selected)
          {
             _e_desklock_null();
             _e_desklock_unselect();
             return ECORE_CALLBACK_RENEW;
          }
        _e_desklock_backspace();
     }
   else if (!strcmp(ev->key, "Delete"))
     {
        if (edd->selected)
          {
             _e_desklock_null();
             _e_desklock_unselect();
             return ECORE_CALLBACK_RENEW;
          }
        _e_desklock_delete();
     }
   else if ((!strcmp(ev->key, "u") &&
             (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     _e_desklock_null();
   else if ((!strcmp(ev->key, "a") &&
             (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     _e_desklock_select();
   else
     {
        /* here we have to grab a password */
        if (ev->compose)
          {
             if (edd->selected)
               {
                  _e_desklock_null();
                  _e_desklock_unselect();
               }
             if ((strlen(edd->passwd) < (PASSWD_LEN - strlen(ev->compose))))
               {
                  strcat(edd->passwd, ev->compose);
                  _e_desklock_passwd_update();
               }
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_desklock_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   E_Desklock_Popup_Data *edp;
   E_Zone *current_zone;
   Eina_List *l;

   current_zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));

   if (current_zone == last_active_zone)
     return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     {
        if (!edp) continue;

        if (edp->popup_wnd->zone != current_zone)
          {
             if (edp->login_box) evas_object_hide(edp->login_box);
             continue;
          }
        if (edp->login_box)
          evas_object_show(edp->login_box);
        else
          _e_desklock_login_box_add(edp);
     }
   _e_desklock_passwd_update();
   last_active_zone = current_zone;
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_desklock_passwd_update(void)
{
   int len, i;
   char passwd_hidden[PASSWD_LEN] = "", *pp;
   E_Desklock_Popup_Data *edp;
   Eina_List *l;

   if (!edd) return;

   len = eina_unicode_utf8_get_len(edd->passwd);
   for (i = 0, pp = passwd_hidden; i < len; i++, pp++)
     *pp = '*';
   *pp = 0;

   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     if (edp->login_box) edje_object_part_text_set(edp->login_box, "e.text.password",
                               passwd_hidden);
}

static void
_e_desklock_select(void)
{
   E_Desklock_Popup_Data *edp;
   Eina_List *l;
   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     if (edp->login_box)
       edje_object_signal_emit(edp->login_box, "e,state,selected", "e");
   edd->selected = EINA_TRUE;
}

static void
_e_desklock_unselect(void)
{
   E_Desklock_Popup_Data *edp;
   Eina_List *l;
   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     if (edp->login_box)
       edje_object_signal_emit(edp->login_box, "e,state,unselected", "e");
   edd->selected = EINA_FALSE;
}

static void
_e_desklock_null(void)
{
   memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
   _e_desklock_passwd_update();
}

static void
_e_desklock_backspace(void)
{
   int len, val, pos;

   if (!edd) return;

   len = strlen(edd->passwd);
   if (len > 0)
     {
        pos = evas_string_char_prev_get(edd->passwd, len, &val);
        if ((pos < len) && (pos >= 0))
          {
             edd->passwd[pos] = 0;
             _e_desklock_passwd_update();
          }
     }
}

static void
_e_desklock_delete(void)
{
   _e_desklock_backspace();
}

static int
_e_desklock_zone_num_get(void)
{
   int num;
   Eina_List *l, *l2;
   E_Manager *man;

   num = 0;
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Container *con;
        EINA_LIST_FOREACH(man->containers, l2, con)
          num += eina_list_count(con->zones);
     }

   return num;
}

static int
_e_desklock_check_auth(void)
{
   if (!edd) return 0;
#ifdef HAVE_PAM
   if (e_config->desklock_auth_method == 0)
     {
        int ret;
        
        ret = _desklock_auth(edd->passwd);
        // passwd off in child proc now - null out from parent
        memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
        return ret;
     }
   else if (e_config->desklock_auth_method == 1)
     {
#endif
        if ((e_config->desklock_personal_passwd) &&
            (!strcmp((!edd->passwd[0] == '\0') ? "" : edd->passwd,
                     !e_config->desklock_personal_passwd ? "" :
                     e_config->desklock_personal_passwd)))
          {
             /* password ok */
             /* security - null out passwd string once we are done with it */
             memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
             e_desklock_hide();
             return 1;
          }
#ifdef HAVE_PAM
     }
   
#endif
   /* password is definitely wrong */
   _e_desklock_state_set(E_DESKLOCK_STATE_INVALID);
   _e_desklock_null();
   return 0;
}

static void
_e_desklock_state_set(int state)
{
   Eina_List *l;
   E_Desklock_Popup_Data *edp;
   const char *signal_desklock, *text;
   if (!edd) return;

   edd->state = state;
   if (state == E_DESKLOCK_STATE_CHECKING)
     {
        signal_desklock = "e,state,checking";
        text = _("Authenticating...");
     }
   else if (state == E_DESKLOCK_STATE_INVALID)
     {
        signal_desklock = "e,state,invalid";
        text = _("The password you entered is invalid. Try again.");
     }
   else
     return;

   EINA_LIST_FOREACH(edd->elock_wnd_list, l, edp)
     {
        edje_object_signal_emit(edp->login_box, signal_desklock, "e.desklock"); // compat
        edje_object_signal_emit(edp->bg_object, signal_desklock, "e.desklock"); // compat
        edje_object_signal_emit(edp->login_box, signal_desklock, "e");
        edje_object_signal_emit(edp->bg_object, signal_desklock, "e");
        edje_object_part_text_set(edp->login_box, "e.text.title", text);
     }
}

#ifdef HAVE_PAM
static Eina_Bool
_e_desklock_cb_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev = event;

   if (ev->pid == _e_desklock_child_pid)
     {
        _e_desklock_child_pid = -1;
        /* ok */
        if (ev->exit_code == 0)
          {
             /* security - null out passwd string once we are done with it */
             memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
             e_desklock_hide();
          }
        /* error */
        else if (ev->exit_code < 128)
          {
             /* security - null out passwd string once we are done with it */
             memset(edd->passwd, 0, sizeof(char) * PASSWD_LEN);
             e_desklock_hide();
             e_util_dialog_show(_("Authentication System Error"),
                                _("Authentication via PAM had errors setting up the<br>"
                                  "authentication session. The error code was <hilight>%i</hilight>.<br>"
                                  "This is bad and should not be happening. Please report this bug.")
                                , ev->exit_code);
          }
        /* failed auth */
        else
          {
             _e_desklock_state_set(E_DESKLOCK_STATE_INVALID);
             /* security - null out passwd string once we are done with it */
             _e_desklock_null();
          }
        if (_e_desklock_exit_handler)
          ecore_event_handler_del(_e_desklock_exit_handler);
        _e_desklock_exit_handler = NULL;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static int
_desklock_auth(char *passwd)
{
   _e_desklock_state_set(E_DESKLOCK_STATE_CHECKING);
   _e_desklock_child_pid = fork();
   if (_e_desklock_child_pid > 0)
     {
        /* parent */
        _e_desklock_exit_handler =
          ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_desklock_cb_exit,
                                  NULL);
     }
   else if (_e_desklock_child_pid == 0)
     {
        /* child */
        int pamerr;
        E_Desklock_Auth da;
        char *current_user, *p;
        struct sigaction action;

        action.sa_handler = SIG_DFL;
        action.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(SIGSEGV, &action, NULL);
        sigaction(SIGILL, &action, NULL);
        sigaction(SIGFPE, &action, NULL);
        sigaction(SIGBUS, &action, NULL);
        sigaction(SIGABRT, &action, NULL);

        current_user = _desklock_auth_get_current_user();
        eina_strlcpy(da.user, current_user, sizeof(da.user));
        eina_strlcpy(da.passwd, passwd, sizeof(da.passwd));
        /* security - null out passwd string once we are done with it */
        for (p = passwd; *p; p++)
          *p = 0;
        da.pam.handle = NULL;
        da.pam.conv.conv = NULL;
        da.pam.conv.appdata_ptr = NULL;

        pamerr = _desklock_pam_init(&da);
        if (pamerr != PAM_SUCCESS)
          {
             free(current_user);
             exit(1);
          }
        pamerr = pam_authenticate(da.pam.handle, 0);
        pam_end(da.pam.handle, pamerr);
        /* security - null out passwd string once we are done with it */
        memset(da.passwd, 0, sizeof(da.passwd));
        if (pamerr == PAM_SUCCESS)
          {
             free(current_user);
             exit(0);
          }
        free(current_user);
        exit(-1);
     }
   else
     {
        _e_desklock_state_set(E_DESKLOCK_STATE_INVALID);
        return 0;
     }
   return 1;
}

static char *
_desklock_auth_get_current_user(void)
{
   char *user;
   struct passwd *pwent = NULL;

   pwent = getpwuid(getuid());
   user = strdup(pwent->pw_name);
   return user;
}

static int
_desklock_pam_init(E_Desklock_Auth *da)
{
   int pamerr;
   const char *pam_prof;
   char *current_host;
   char *current_user;

   if (!da) return -1;

   da->pam.conv.conv = _desklock_auth_pam_conv;
   da->pam.conv.appdata_ptr = da;
   da->pam.handle = NULL;

   /* try other pam profiles - and system-auth (login for fbsd users) is a fallback */
   pam_prof = "login";
   if (ecore_file_exists("/etc/pam.d/enlightenment"))
     pam_prof = "enlightenment";
   else if (ecore_file_exists("/etc/pam.d/xscreensaver"))
     pam_prof = "xscreensaver";
   else if (ecore_file_exists("/etc/pam.d/kscreensaver"))
     pam_prof = "kscreensaver";
   else if (ecore_file_exists("/etc/pam.d/system-auth"))
     pam_prof = "system-auth";
   else if (ecore_file_exists("/etc/pam.d/system"))
     pam_prof = "system";
   else if (ecore_file_exists("/etc/pam.d/xdm"))
     pam_prof = "xdm";
   else if (ecore_file_exists("/etc/pam.d/gdm"))
     pam_prof = "gdm";
   else if (ecore_file_exists("/etc/pam.d/kdm"))
     pam_prof = "kdm";

   if ((pamerr = pam_start(pam_prof, da->user, &(da->pam.conv),
                           &(da->pam.handle))) != PAM_SUCCESS)
     return pamerr;

   current_user = _desklock_auth_get_current_user();

   if ((pamerr = pam_set_item(da->pam.handle, PAM_USER, current_user)) != PAM_SUCCESS)
     {
        free(current_user);
        return pamerr;
     }

   current_host = _desklock_auth_get_current_host();
   if ((pamerr = pam_set_item(da->pam.handle, PAM_RHOST, current_host)) != PAM_SUCCESS)
     {
        free(current_user);
        free(current_host);
        return pamerr;
     }

   free(current_user);
   free(current_host);
   return 0;
}

static int
_desklock_auth_pam_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
   int replies = 0;
   E_Desklock_Auth *da = (E_Desklock_Auth *)appdata_ptr;
   struct pam_response *reply = NULL;

   reply = (struct pam_response *)malloc(sizeof(struct pam_response) * num_msg);

   if (!reply) return PAM_CONV_ERR;

   for (replies = 0; replies < num_msg; replies++)
     {
        switch (msg[replies]->msg_style)
          {
           case PAM_PROMPT_ECHO_ON:
             reply[replies].resp_retcode = PAM_SUCCESS;
             reply[replies].resp = strdup(da->user);
             break;

           case PAM_PROMPT_ECHO_OFF:
             reply[replies].resp_retcode = PAM_SUCCESS;
             reply[replies].resp = strdup(da->passwd);
             break;

           case PAM_ERROR_MSG:
           case PAM_TEXT_INFO:
             reply[replies].resp_retcode = PAM_SUCCESS;
             reply[replies].resp = NULL;
             break;

           default:
             free(reply);
             return PAM_CONV_ERR;
          }
     }
   *resp = reply;
   return PAM_SUCCESS;
}

static char *
_desklock_auth_get_current_host(void)
{
   return strdup("localhost");
}

#endif

static Eina_Bool
_e_desklock_cb_custom_desklock_exit(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev = event;

   if (ev->exe != _e_custom_desklock_exe) return ECORE_CALLBACK_PASS_ON;

   if (ev->exit_code != 0)
     {
        /* do something profound here... like notify someone */
     }

   e_desklock_hide();

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_e_desklock_cb_idle_poller(void *data __UNUSED__)
{
   if ((e_config->desklock_autolock_idle) && (!e_config->mode.presentation) &&
       (!e_util_fullscreen_current_any()))
     {
        double idle, max;

        /* If a desklock is already up, bail */
        if ((_e_custom_desklock_exe) || (edd)) return ECORE_CALLBACK_RENEW;

        idle = ecore_x_screensaver_idle_time_get();
        max = e_config->desklock_autolock_idle_timeout;
        if (_e_desklock_ask_presentation_count > 0)
          max *= (1 + _e_desklock_ask_presentation_count);

        /* If we have exceeded our idle time... */
        if (idle >= max)
          {
             /*
              * Unfortunately, not all "desklocks" stay up for as long as
              * the user is idle or until it is unlocked.
              *
              * 'xscreensaver-command -lock' for example sends a command
              * to xscreensaver and then terminates.  So, we have another
              * check (_e_desklock_user_idle) which lets us know that we
              * have locked the screen due to idleness.
              */
             if (!_e_desklock_user_idle)
               {
                  _e_desklock_user_idle = 1;
                  e_desklock_show_autolocked();
               }
          }
        else
          _e_desklock_user_idle = 0;
     }

   /* Make sure our poller persists. */
   return ECORE_CALLBACK_RENEW;
}

static void
_e_desklock_ask_presentation_del(void *data)
{
   if (_e_desklock_ask_presentation_dia == data)
     _e_desklock_ask_presentation_dia = NULL;
}

static void
_e_desklock_ask_presentation_yes(void *data __UNUSED__, E_Dialog *dia)
{
   e_config->mode.presentation = 1;
   e_config_mode_changed();
   e_config_save_queue();
   e_object_del(E_OBJECT(dia));
   _e_desklock_ask_presentation_count = 0;
}

static void
_e_desklock_ask_presentation_no(void *data __UNUSED__, E_Dialog *dia)
{
   e_object_del(E_OBJECT(dia));
   _e_desklock_ask_presentation_count = 0;
}

static void
_e_desklock_ask_presentation_no_increase(void *data __UNUSED__, E_Dialog *dia)
{
   int timeout, interval, blanking, expose;

   _e_desklock_ask_presentation_count++;
   timeout = e_config->screensaver_timeout * _e_desklock_ask_presentation_count;
   interval = e_config->screensaver_interval;
   blanking = e_config->screensaver_blanking;
   expose = e_config->screensaver_expose;

   ecore_x_screensaver_set(timeout, interval, blanking, expose);
   e_object_del(E_OBJECT(dia));
}

static void
_e_desklock_ask_presentation_no_forever(void *data __UNUSED__, E_Dialog *dia)
{
   e_config->desklock_ask_presentation = 0;
   e_config_save_queue();
   e_object_del(E_OBJECT(dia));
   _e_desklock_ask_presentation_count = 0;
}

static void
_e_desklock_ask_presentation_key_down(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;
   E_Dialog *dia = data;

   if (strcmp(ev->key, "Return") == 0)
     _e_desklock_ask_presentation_yes(NULL, dia);
   else if (strcmp(ev->key, "Escape") == 0)
     _e_desklock_ask_presentation_no(NULL, dia);
}

static void
_e_desklock_ask_presentation_mode(void)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dia;

   if (_e_desklock_ask_presentation_dia) return;

   if (!(man = e_manager_current_get())) return;
   if (!(con = e_container_current_get(man))) return;
   if (!(dia = e_dialog_new(con, "E", "_desklock_ask_presentation"))) return;

   e_dialog_title_set(dia, _("Activate Presentation Mode?"));
   e_dialog_icon_set(dia, "dialog-ask", 64);
   e_dialog_text_set(dia,
                     _("You unlocked your desktop too fast.<br><br>"
                       "Would you like to enable <b>presentation</b> mode and "
                       "temporarily disable screen saver, lock and power saving?"));

   e_object_del_attach_func_set(E_OBJECT(dia),
                                _e_desklock_ask_presentation_del);
   e_dialog_button_add(dia, _("Yes"), NULL,
                       _e_desklock_ask_presentation_yes, NULL);
   e_dialog_button_add(dia, _("No"), NULL,
                       _e_desklock_ask_presentation_no, NULL);
   e_dialog_button_add(dia, _("No, but increase timeout"), NULL,
                       _e_desklock_ask_presentation_no_increase, NULL);
   e_dialog_button_add(dia, _("No, and stop asking"), NULL,
                       _e_desklock_ask_presentation_no_forever, NULL);

   e_dialog_button_focus_num(dia, 0);
   e_widget_list_homogeneous_set(dia->box_object, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);

   evas_object_event_callback_add(dia->bg_object, EVAS_CALLBACK_KEY_DOWN,
                                  _e_desklock_ask_presentation_key_down, dia);

   _e_desklock_ask_presentation_dia = dia;
}

static Eina_Bool
_e_desklock_run(E_Desklock_Run *task)
{
   Efreet_Desktop *desktop;

   desktop = eina_list_nth(task->desk_run->desktops, task->position++);
   if (!desktop)
     {
        e_object_del(E_OBJECT(task->desk_run));
        free(task);
        return EINA_FALSE;
     }

   e_exec(NULL, desktop, NULL, NULL, NULL);
   return EINA_TRUE;
}

static void
_e_desklock_job(void *data __UNUSED__)
{
   E_Desklock_Run *task;

   job = NULL;
   if (!tasks) return ;

   task = eina_list_data_get(tasks);
   if (!_e_desklock_run(task))
     tasks = eina_list_remove_list(tasks, tasks);

   if (tasks) job = ecore_job_add(_e_desklock_job, NULL);
}

static Eina_Bool
_e_desklock_cb_run(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Desklock_Run *task;
   E_Event_Desklock *ev = event;
   E_Order *desk_run;
   char buf[PATH_MAX];

   if (!ev->suspend) return ECORE_CALLBACK_PASS_ON;

   if (ev->on)
     {
        e_user_dir_concat_static(buf, "applications/desk-lock/.order");
        if (!ecore_file_exists(buf))
          e_prefix_data_concat_static(buf, "applications/desk-lock/.order");
     }
   else
     {
        e_user_dir_concat_static(buf, "applications/desk-unlock/.order");
        if (!ecore_file_exists(buf))
          e_prefix_data_concat_static(buf, "applications/desk-unlock/.order");
     }

   desk_run = e_order_new(buf);
   if (!desk_run) return ECORE_CALLBACK_PASS_ON;

   task = calloc(1, sizeof (E_Desklock_Run));
   if (!task)
     {
        e_object_del(E_OBJECT(desk_run));
        return ECORE_CALLBACK_PASS_ON;
     }

   task->desk_run = desk_run;
   tasks = eina_list_append(tasks, task);

   if (!job) ecore_job_add(_e_desklock_job, NULL);

   return ECORE_CALLBACK_PASS_ON;
}

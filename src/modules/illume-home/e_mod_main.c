#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_busycover.h"

#define IL_HOME_WIN_TYPE 0xE0b0102f

/* local structures */
typedef struct _Il_Home_Win Il_Home_Win;
typedef struct _Il_Home_Exec Il_Home_Exec;

struct _Il_Home_Win 
{
   E_Object e_obj_inherit;

   E_Win *win;
   Evas_Object *o_bg, *o_sf, *o_fm, *o_cover;
   E_Busycover *cover;
   E_Zone *zone;
};

struct _Il_Home_Exec 
{
   E_Busycover *cover;
   Efreet_Desktop *desktop;
   Ecore_Exe *exec;
   E_Border *border;
   E_Zone *zone;
   Ecore_Timer *timeout;
   int startup_id;
   pid_t pid;
   void *handle;
};

/* local function prototypes */
static void _il_home_apps_populate(void);
static void _il_home_apps_unpopulate(void);
static void _il_home_desks_populate(void);
static void _il_home_desktop_run(Il_Home_Win *hwin, Efreet_Desktop *desktop);
static E_Border *_il_home_desktop_find_border(E_Zone *zone, Efreet_Desktop *desktop);
static void _il_home_win_new(E_Zone *zone);
static void _il_home_win_cb_free(Il_Home_Win *hwin);
static void _il_home_win_cb_resize(E_Win *win);
static void _il_home_fmc_set(Evas_Object *obj);
static Eina_Bool _il_home_update_deferred(void *data __UNUSED__);
static void _il_home_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _il_home_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _il_home_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _il_home_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void _il_home_cb_selected(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__);
static Eina_Bool _il_home_desktop_cache_update(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool _il_home_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _il_home_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _il_home_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _il_home_cb_exe_timeout(void *data);
static Eina_Bool _il_home_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _il_home_cb_prop_change(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _il_home_cb_bg_change(void *data __UNUSED__, int type __UNUSED__, void *event);

/* local variables */
static Eina_List *hwins = NULL;
static Eina_List *hdls = NULL;
static Eina_List *desks = NULL;
static Eina_List *exes = NULL;
static Ecore_Timer *defer = NULL;

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Home" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Manager *man;
   Eina_List *ml;

   if (!il_home_config_init(m)) return NULL;

   _il_home_apps_unpopulate();
   _il_home_apps_populate();

   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(EFREET_EVENT_DESKTOP_CACHE_UPDATE, 
                                              _il_home_desktop_cache_update, 
                                              NULL));

   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _il_home_cb_border_add, NULL));
   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _il_home_cb_border_del, NULL));
   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
                                              _il_home_cb_exe_del, NULL));
   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _il_home_cb_client_message, 
                                              NULL));
   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _il_home_cb_prop_change, 
                                              NULL));
   hdls = 
     eina_list_append(hdls, 
                      ecore_event_handler_add(E_EVENT_BG_UPDATE, 
                                              _il_home_cb_bg_change, NULL));

   EINA_LIST_FOREACH(e_manager_list(), ml, man) 
     {
        E_Container *con;
        Eina_List *cl;

        EINA_LIST_FOREACH(man->containers, cl, con) 
          {
             E_Zone *zone;
             Eina_List *zl;

             EINA_LIST_FOREACH(con->zones, zl, zone) 
               {
                  Ecore_X_Illume_Mode mode;

                  mode = ecore_x_e_illume_mode_get(zone->black_win);
                  _il_home_win_new(zone);
                  if (mode > ECORE_X_ILLUME_MODE_SINGLE)
                    _il_home_win_new(zone);
               }
          }
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   Ecore_Event_Handler *hdl;
   Il_Home_Win *hwin;
   Il_Home_Exec *exe;

   EINA_LIST_FREE(hwins, hwin)
     e_object_del(E_OBJECT(hwin));

   EINA_LIST_FREE(exes, exe) 
     {
        if (exe->exec) 
          {
             ecore_exe_terminate(exe->exec);
             ecore_exe_free(exe->exec);
          }
        if (exe->handle) e_busycover_pop(exe->cover, exe->handle);
        if (exe->timeout) ecore_timer_del(exe->timeout);
        if (exe->desktop) efreet_desktop_free(exe->desktop);
        E_FREE(exe);
     }

   EINA_LIST_FREE(hdls, hdl)
     ecore_event_handler_del(hdl);

   _il_home_apps_unpopulate();

   il_home_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return il_home_config_save();
}

void 
il_home_win_cfg_update(void) 
{
   _il_home_apps_unpopulate();
   _il_home_apps_populate();
}

/* local function prototypes */
static void 
_il_home_apps_populate(void) 
{
   Il_Home_Win *hwin;
   Eina_List *l;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "appshadow");
   ecore_file_mkpath(buff);

   _il_home_desks_populate();

   EINA_LIST_FOREACH(hwins, l, hwin) 
     {
        _il_home_fmc_set(hwin->o_fm);
        e_fm2_path_set(hwin->o_fm, NULL, buff);
     }
}

static void 
_il_home_apps_unpopulate(void) 
{
   Efreet_Desktop *desktop;
   Eina_List *files;
   char buff[PATH_MAX], *file;
   size_t len;

   EINA_LIST_FREE(desks, desktop) 
     efreet_desktop_free(desktop);

   len = e_user_dir_concat_static(buff, "appshadow");
   if ((len + 2) >= sizeof(buff)) return;

   files = ecore_file_ls(buff);
   buff[len] = '/';
   len++;

   EINA_LIST_FREE(files, file) 
     {
        if (eina_strlcpy(buff + len, file, sizeof(buff) - len) >= sizeof(buff) - len)
          continue;
        ecore_file_unlink(buff);
        free(file);
     }
}

static void 
_il_home_desks_populate(void) 
{
   Efreet_Menu *menu, *entry;
   Eina_List *ml, *settings, *sys, *kbd;
   Efreet_Desktop *desktop;
   int num = 0;

   if (!(menu = efreet_menu_get())) return;

   settings = efreet_util_desktop_category_list("Settings");
   sys = efreet_util_desktop_category_list("System");
   kbd = efreet_util_desktop_category_list("Keyboard");

   EINA_LIST_FOREACH(menu->entries, ml, entry) 
     {
        Eina_List *sl;
        Efreet_Menu *sm;

        if (entry->type != EFREET_MENU_ENTRY_MENU) continue;
        EINA_LIST_FOREACH(entry->entries, sl, sm) 
          {
             char buff[PATH_MAX];

             if (sm->type != EFREET_MENU_ENTRY_DESKTOP) continue;
             if (!(desktop = sm->desktop)) continue;
             if ((settings) && (sys) && 
                 (eina_list_data_find(settings, desktop)) && 
                 (eina_list_data_find(sys, desktop))) continue;
             if ((kbd) && (eina_list_data_find(kbd, desktop))) 
               continue;
             efreet_desktop_ref(desktop);
             desks = eina_list_append(desks, desktop);
             e_user_dir_snprintf(buff, sizeof(buff), 
                                 "appshadow/%04x.desktop", num);
             ecore_file_symlink(desktop->orig_path, buff);
             num++;
          }
     }

   efreet_menu_free(menu);

   EINA_LIST_FREE(settings, desktop)
     efreet_desktop_free(desktop);
   EINA_LIST_FREE(sys, desktop)
     efreet_desktop_free(desktop);
   EINA_LIST_FREE(kbd, desktop)
     efreet_desktop_free(desktop);
}

static void 
_il_home_desktop_run(Il_Home_Win *hwin, Efreet_Desktop *desktop) 
{
   E_Exec_Instance *eins;
   Il_Home_Exec *exec;
   Eina_List *l;
   E_Border *bd;
   char buff[PATH_MAX];

   if ((!hwin) || (!desktop) || (!desktop->exec)) return;
   EINA_LIST_FOREACH(exes, l, exec) 
     {
        if (exec->desktop != desktop) continue;
        if ((exec->border) && (exec->border->zone == hwin->zone)) 
          {
             e_border_uniconify(exec->border);
             e_border_raise(exec->border);
             e_border_focus_set(exec->border, 1, 1);
             return;
          }
     }
   if ((bd = _il_home_desktop_find_border(hwin->zone, desktop))) 
     {
        e_border_uniconify(bd);
        e_border_raise(bd);
        e_border_focus_set(bd, 1, 1);
        return;
     }

   exec = E_NEW(Il_Home_Exec, 1);
   if (!exec) return;
   exec->cover = hwin->cover;
   eins = e_exec(hwin->zone, desktop, NULL, NULL, "illume-home");
   exec->desktop = desktop;
   exec->zone = hwin->zone;
   if (eins) 
     {
        exec->exec = eins->exe;
        exec->startup_id = eins->startup_id;
        if (eins->exe) 
          exec->pid = ecore_exe_pid_get(eins->exe);
     }
   exec->timeout = ecore_timer_add(2.0, _il_home_cb_exe_timeout, exec);
   snprintf(buff, sizeof(buff), "Starting %s", desktop->name);
   exec->handle = e_busycover_push(hwin->cover, buff, NULL);
   exes = eina_list_append(exes, exec);
}

static E_Border *
_il_home_desktop_find_border(E_Zone *zone, Efreet_Desktop *desktop) 
{
   Eina_List *l;
   E_Border *bd;
   char *exe = NULL, *p;

   if (!desktop) return NULL;
   if (!desktop->exec) return NULL;
   p = strchr(desktop->exec, ' ');
   if (!p)
     exe = strdup(desktop->exec);
   else 
     {
        size_t s = p - desktop->exec + 1;
        exe = calloc(1, s);
        if (exe) eina_strlcpy(exe, desktop->exec, s);
     }
   if (exe) 
     {
        p = strrchr(exe, '/');
        if (p) strcpy(exe, p + 1);
     }

   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        if (bd->zone != zone) continue;
        if (e_exec_startup_id_pid_find(bd->client.netwm.pid, 
                                       bd->client.netwm.startup_id) == desktop) 
          {
             if (exe) free(exe);
             return bd;
          }
        if (exe) 
          {
             if (bd->client.icccm.command.argv) 
               {
                  char *pp;

                  pp = strrchr(bd->client.icccm.command.argv[0], '/');
                  if (!pp) pp = bd->client.icccm.command.argv[0];
                  if (!strcmp(exe, pp)) 
                    {
                       free(exe);
                       return bd;
                    }
               }
             if ((bd->client.icccm.name) && 
                 (!strcasecmp(bd->client.icccm.name, exe))) 
               {
                  free(exe);
                  return bd;
               }
          }
     }
   if (exe) free(exe);
   return NULL;
}

static void 
_il_home_win_new(E_Zone *zone) 
{
   Il_Home_Win *hwin;
   Evas *evas;
   E_Desk *desk;
   char buff[PATH_MAX];
   const char *bgfile;

   hwin = E_OBJECT_ALLOC(Il_Home_Win, IL_HOME_WIN_TYPE, _il_home_win_cb_free);
   if (!hwin) return;

   hwin->zone = zone;
   hwin->win = e_win_new(zone->container);
   if (!hwin->win) 
     {
        e_object_del(E_OBJECT(hwin));
        return;
     }
   hwin->win->data = hwin;
   e_win_title_set(hwin->win, _("Illume Home"));
   e_win_name_class_set(hwin->win, "Illume-Home", "Illume-Home");
   e_win_resize_callback_set(hwin->win, _il_home_win_cb_resize);
   e_win_no_remember_set(hwin->win, EINA_TRUE);

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);

   evas = e_win_evas_get(hwin->win);

   desk = e_desk_current_get(zone);
   if (desk)
     bgfile = e_bg_file_get(zone->container->num, zone->num, desk->x, desk->y);
   else
     bgfile = e_bg_file_get(zone->container->num, zone->num, -1, -1);

   hwin->o_bg = edje_object_add(evas);
   edje_object_file_set(hwin->o_bg, bgfile, "e/desktop/background");
   evas_object_move(hwin->o_bg, 0, 0);
   evas_object_show(hwin->o_bg);

   hwin->o_sf = e_scrollframe_add(evas);
   e_scrollframe_single_dir_set(hwin->o_sf, EINA_TRUE);
   e_scrollframe_custom_edje_file_set(hwin->o_sf, buff, 
                                      "modules/illume-home/launcher/scrollview");
   evas_object_move(hwin->o_sf, 0, 0);
   evas_object_show(hwin->o_sf);

   hwin->o_fm = e_fm2_add(evas);
   _il_home_fmc_set(hwin->o_fm);
   evas_object_show(hwin->o_fm);
   e_user_dir_concat_static(buff, "appshadow");
   e_fm2_path_set(hwin->o_fm, NULL, buff);
   e_fm2_window_object_set(hwin->o_fm, E_OBJECT(hwin->win));
   e_scrollframe_extern_pan_set(hwin->o_sf, hwin->o_fm, 
                                _il_home_pan_set, 
                                _il_home_pan_get, 
                                _il_home_pan_max_get, 
                                _il_home_pan_child_size_get);
   evas_object_propagate_events_set(hwin->o_fm, 0);
   evas_object_smart_callback_add(hwin->o_fm, "selected", 
                                  _il_home_cb_selected, hwin);

   hwin->cover = e_busycover_new(hwin->win);

   e_win_move_resize(hwin->win, zone->x, zone->y, zone->w, (zone->h / 2));
   e_win_show(hwin->win);
   e_border_zone_set(hwin->win->border, zone);
   if (hwin->win->evas_win) 
     e_drop_xdnd_register_set(hwin->win->evas_win, EINA_TRUE);

   hwins = eina_list_append(hwins, hwin);
}

static void 
_il_home_win_cb_free(Il_Home_Win *hwin) 
{
   if (hwin->win->evas_win) e_drop_xdnd_register_set(hwin->win->evas_win, 0);
   if (hwin->cover) e_object_del(E_OBJECT(hwin->cover));
   if (hwin->o_bg) evas_object_del(hwin->o_bg);
   if (hwin->o_sf) evas_object_del(hwin->o_sf);
   if (hwin->o_fm) evas_object_del(hwin->o_fm);
   if (hwin->win) e_object_del(E_OBJECT(hwin->win));
   E_FREE(hwin);
}

static void 
_il_home_win_cb_resize(E_Win *win) 
{
   Il_Home_Win *hwin;

   if (!(hwin = win->data)) return;
   if (hwin->o_bg) evas_object_resize(hwin->o_bg, win->w, win->h);
   if (hwin->o_sf) evas_object_resize(hwin->o_sf, win->w, win->h);
   if (hwin->cover) e_busycover_resize(hwin->cover, win->w, win->h);
}

static void 
_il_home_fmc_set(Evas_Object *obj) 
{
   E_Fm2_Config fmc;

   if (!obj) return;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_GRID_ICONS;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 0;
   fmc.view.single_click = il_home_cfg->single_click;
   fmc.view.single_click_delay = il_home_cfg->single_click_delay;
   fmc.view.no_subdir_jump = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.icon.w = il_home_cfg->icon_size * e_scale / 2.0;
   fmc.icon.icon.h = il_home_cfg->icon_size * e_scale / 2.0;
   fmc.icon.fixed.w = il_home_cfg->icon_size * e_scale / 2.0;
   fmc.icon.fixed.h = il_home_cfg->icon_size * e_scale / 2.0;
   fmc.list.sort.no_case = 0;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(obj, &fmc);
}

static Eina_Bool
_il_home_update_deferred(void *data __UNUSED__) 
{
   _il_home_apps_unpopulate();
   _il_home_apps_populate();
   defer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void 
_il_home_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y) 
{
   e_fm2_pan_set(obj, x, y);
}

static void 
_il_home_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y) 
{
   e_fm2_pan_get(obj, x, y);
}

static void 
_il_home_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y) 
{
   e_fm2_pan_max_get(obj, x, y);
}

static void 
_il_home_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h) 
{
   e_fm2_pan_child_size_get(obj, w, h);
}

static void 
_il_home_cb_selected(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   Il_Home_Win *hwin;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;

   if (!(hwin = data)) return;
   if (!(selected = e_fm2_selected_list_get(hwin->o_fm))) return;
   EINA_LIST_FREE(selected, ici) 
     {
        Efreet_Desktop *desktop;

        if ((!ici) || (!ici->real_link)) continue;
        if (!(desktop = efreet_desktop_get(ici->real_link))) continue;
        _il_home_desktop_run(hwin, desktop);
     }
}

static Eina_Bool
_il_home_desktop_cache_update(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__) 
{
   if (defer) ecore_timer_del(defer);
   defer = ecore_timer_add(0.5, _il_home_update_deferred, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Add *ev;
   Il_Home_Exec *exe;
   Eina_List *l;

   ev = event;
   EINA_LIST_FOREACH(exes, l, exe) 
     {
        if (!exe->border) 
          {
             if ((exe->startup_id == ev->border->client.netwm.startup_id) || 
                 (exe->pid == ev->border->client.netwm.pid)) 
               {
                  exe->border = ev->border;
               }
          }
        if (!exe->border) continue;
        if (exe->border->zone != exe->zone) 
          {
             exe->border->zone = exe->zone;
             exe->border->x = exe->zone->x;
             exe->border->y = exe->zone->y;
             exe->border->changes.pos = 1;
             exe->border->changed = 1;
          }
        if (exe->handle) 
          {
             e_busycover_pop(exe->cover, exe->handle);
             exe->handle = NULL;
          }
        if (exe->timeout) ecore_timer_del(exe->timeout);
        exe->timeout = NULL;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Remove *ev;
   Il_Home_Exec *exe;
   Eina_List *l;

   ev = event;
   EINA_LIST_FOREACH(exes, l, exe) 
     {
        if (exe->border == ev->border) 
          {
             if (exe->exec) ecore_exe_free(exe->exec);
             exe->exec = NULL;
             if (exe->handle) e_busycover_pop(exe->cover, exe->handle);
             exe->handle = NULL;
             exe->border = NULL;
             exes = eina_list_remove(exes, exe);
             E_FREE(exe);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Il_Home_Exec *exe;
   Ecore_Exe_Event_Del *ev;
   Eina_List *l;

   ev = event;
   EINA_LIST_FOREACH(exes, l, exe) 
     {
        if (exe->pid == ev->pid) 
          {
             if (exe->handle) 
               {
                  e_busycover_pop(exe->cover, exe->handle);
                  exe->handle = NULL;
               }
             exes = eina_list_remove_list(exes, l);
             if (exe->timeout) ecore_timer_del(exe->timeout);
             if (exe->desktop) efreet_desktop_free(exe->desktop);
             E_FREE(exe);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_exe_timeout(void *data) 
{
   Il_Home_Exec *exe;

   if (!(exe = data)) return ECORE_CALLBACK_CANCEL;
   if (exe->handle) e_busycover_pop(exe->cover, exe->handle);
   exe->handle = NULL;
   if (!exe->border) 
     {
        exes = eina_list_remove(exes, exe);
        if (exe->desktop) efreet_desktop_free(exe->desktop);
        E_FREE(exe);
        return ECORE_CALLBACK_CANCEL;
     }
   exe->timeout = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_il_home_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_HOME_NEW) 
     {
        E_Zone *zone;

        zone = e_util_zone_window_find(ev->win);
        if (zone->black_win != ev->win) return ECORE_CALLBACK_PASS_ON;
        _il_home_win_new(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_HOME_DEL) 
     {
        E_Border *bd;
        Eina_List *l;
        Il_Home_Win *hwin;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        EINA_LIST_FOREACH(hwins, l, hwin) 
          {
             if (hwin->win->border == bd) 
               {
                  hwins = eina_list_remove_list(hwins, hwins);
                  e_object_del(E_OBJECT(hwin));
                  break;
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_prop_change(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Window_Property *ev;
   Il_Home_Win *hwin;
   Eina_List *l;

   ev = event;
   if (ev->atom != ATM_ENLIGHTENMENT_SCALE) return ECORE_CALLBACK_PASS_ON;
   EINA_LIST_FOREACH(hwins, l, hwin) 
     if (hwin->o_fm) 
       {
          _il_home_fmc_set(hwin->o_fm);
          e_fm2_refresh(hwin->o_fm);
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_il_home_cb_bg_change(void *data __UNUSED__, int type, void *event __UNUSED__) 
{
   Il_Home_Win *hwin;
   Eina_List *l;

   if (type != E_EVENT_BG_UPDATE) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(hwins, l, hwin) 
     {
        E_Zone *zone;
        E_Desk *desk;
        const char *bgfile;

        zone = hwin->zone;
        desk = e_desk_current_get(zone);
        if (desk)
          bgfile = e_bg_file_get(zone->container->num, zone->num, desk->x, desk->y);
        else
          bgfile = e_bg_file_get(zone->container->num, zone->num, -1, -1);
        edje_object_file_set(hwin->o_bg, bgfile, "e/desktop/background");
     }

   return ECORE_CALLBACK_PASS_ON;
}

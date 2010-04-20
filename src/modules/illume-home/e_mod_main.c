#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_busycover.h"

#define IL_HOME_WIN_TYPE 0xE0b0102f

/* local structures */
typedef struct _Instance Instance;
typedef struct _Il_Home_Win Il_Home_Win;
typedef struct _Il_Home_Exec Il_Home_Exec;

struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_btn;
   Eina_List *wins, *handlers;
};

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
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _il_home_btn_cb_click(void *data, void *data2);
static void _il_home_win_new(Instance *inst);
static void _il_home_win_cb_free(Il_Home_Win *hwin);
static void _il_home_win_cb_resize(E_Win *win);
static void _il_home_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _il_home_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _il_home_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _il_home_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void _il_home_cb_selected(void *data, Evas_Object *obj, void *event);
static void _il_home_desktop_run(Il_Home_Win *hwin, Efreet_Desktop *desktop);
static void _il_home_apps_populate(void);
static void _il_home_apps_unpopulate(void);
static void _il_home_fmc_set(Evas_Object *obj);
static void _il_home_desks_populate(void);
static int _il_home_desktop_cache_update(void *data, int type, void *event);
static int _il_home_update_deferred(void *data);
static int _il_home_win_cb_exe_del(void *data, int type, void *event);
static E_Border *_il_home_desktop_find_border(E_Zone *zone, Efreet_Desktop *desktop);
static int _il_home_win_cb_timeout(void *data);
static int _il_home_border_add(void *data, int type, void *event);
static int _il_home_border_remove(void *data, int type, void *event);
static int _il_home_cb_client_message(void *data, int type, void *event);
static int _il_home_cb_prop_change(void *data, int type, void *event);

/* local variables */
static Eina_List *instances = NULL;
static Eina_List *desks = NULL;
static Eina_List *handlers = NULL;
static Eina_List *exes = NULL;
static Ecore_Timer *defer = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-home", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Home" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_home_config_init(m)) return NULL;

   _il_home_apps_unpopulate();
   _il_home_apps_populate();

   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(EFREET_EVENT_DESKTOP_CACHE_UPDATE, 
                                              _il_home_desktop_cache_update, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _il_home_border_add, NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _il_home_border_remove, NULL));

   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_EXE_EVENT_DEL, 
                                              _il_home_win_cb_exe_del, NULL));

   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   Ecore_Event_Handler *handle;
   Il_Home_Exec *exe;

   EINA_LIST_FREE(exes, exe) 
     {
        if (exe->exec) 
          {
             ecore_exe_terminate(exe->exec);
             ecore_exe_free(exe->exec);
             exe->exec = NULL;
          }
        if (exe->handle) 
          {
             e_busycover_pop(exe->cover, exe->handle);
             exe->handle = NULL;
          }
        if (exe->timeout) ecore_timer_del(exe->timeout);
	if (exe->desktop) efreet_desktop_free(exe->desktop);
	E_FREE(exe);
     }

   _il_home_apps_unpopulate();

   EINA_LIST_FREE(handlers, handle)
     ecore_event_handler_del(handle);

   e_gadcon_provider_unregister(&_gc_class);

   il_home_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_home_config_save();
}

void 
il_home_win_cfg_update(void) 
{
   _il_home_apps_unpopulate();
   _il_home_apps_populate();
}

/* local functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;
   Evas_Object *icon;
   Ecore_X_Window xwin;
   Ecore_X_Illume_Mode mode;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);

   inst = E_NEW(Instance, 1);
   inst->o_btn = e_widget_button_add(gc->evas, NULL, NULL, 
                                     _il_home_btn_cb_click, inst, NULL);
   icon = e_icon_add(evas_object_evas_get(inst->o_btn));
   e_icon_file_edje_set(icon, buff, "icon");
   e_widget_button_icon_set(inst->o_btn, icon);

   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_btn);
   inst->gcc->data = inst;

   _il_home_win_new(inst);

   xwin = inst->gcc->gadcon->zone->black_win;
   mode = ecore_x_e_illume_mode_get(xwin);
   if (mode > ECORE_X_ILLUME_MODE_SINGLE)
     _il_home_win_new(inst);

   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _il_home_cb_client_message, inst));
   inst->handlers = 
     eina_list_append(inst->handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _il_home_cb_prop_change, inst));

   instances = eina_list_append(instances, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;
   Il_Home_Win *hwin;
   Ecore_Event_Handler *hdl;

   if (!(inst = gcc->data)) return;

   instances = eina_list_remove(instances, inst);

   EINA_LIST_FREE(inst->handlers, hdl)
     ecore_event_handler_del(hdl);

   if (inst->o_btn) evas_object_del(inst->o_btn);

   EINA_LIST_FREE(inst->wins, hwin)
     e_object_del(E_OBJECT(hwin));

   E_FREE(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *cc) 
{
   return _("Illume-Home");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buff, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *cc) 
{
   static char buff[32];

   snprintf(buff, sizeof(buff), "%s.%d", _gc_class.name, 
            eina_list_count(instances));
   return buff;
}

static void 
_il_home_btn_cb_click(void *data, void *data2) 
{
   Instance *inst;
   E_Zone *zone;
   Ecore_X_Illume_Mode mode;
   int hcount = 0;

   if (!(inst = data)) return;

   /* we need to check current illume mode here first */
   zone = inst->gcc->gadcon->zone;
   mode = ecore_x_e_illume_mode_get(zone->black_win);

   if (mode <= ECORE_X_ILLUME_MODE_SINGLE) 
     hcount = 1;
   else 
     hcount = 2;

   /* if there are less than 2 home windows, create a new one */
   if (eina_list_count(inst->wins) < hcount) 
     _il_home_win_new(inst);
   else 
     {
        /* already 2 home windows, so tell illume to focus one */
        ecore_x_e_illume_focus_home_send(zone->black_win);
     }
}

static void 
_il_home_win_new(Instance *inst) 
{
   Il_Home_Win *hwin;
   E_Zone *zone;
   E_Desk *desk;
   char buff[PATH_MAX];
   const char *bgfile;

   if (!inst) return;

   hwin = E_OBJECT_ALLOC(Il_Home_Win, IL_HOME_WIN_TYPE, 
                         _il_home_win_cb_free);
   if (!hwin) return;
   inst->wins = eina_list_append(inst->wins, hwin);

   zone = inst->gcc->gadcon->zone;
   hwin->zone = zone;

   hwin->win = e_win_new(zone->container);
   if (!hwin->win) 
     {
        e_object_del(E_OBJECT(hwin));
        return;
     }
   hwin->win->data = inst;
   e_win_title_set(hwin->win, _("Illume Home"));
   e_win_name_class_set(hwin->win, "Illume-Home", "Illume-Home");
   e_win_resize_callback_set(hwin->win, _il_home_win_cb_resize);
   e_win_no_remember_set(hwin->win, EINA_TRUE);

   snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", 
            il_home_cfg->mod_dir);

   desk = e_desk_current_get(zone);
   if (desk)
     bgfile = e_bg_file_get(zone->container->num, zone->num, desk->x, desk->y);
   else
     bgfile = e_bg_file_get(zone->container->num, zone->num, -1, -1);

   hwin->o_bg = edje_object_add(e_win_evas_get(hwin->win));
   edje_object_file_set(hwin->o_bg, bgfile, "e/desktop/background");
   evas_object_move(hwin->o_bg, 0, 0);
   evas_object_show(hwin->o_bg);

   hwin->o_sf = e_scrollframe_add(e_win_evas_get(hwin->win));
   e_scrollframe_single_dir_set(hwin->o_sf, 1);
   evas_object_move(hwin->o_sf, 0, 0);
   evas_object_show(hwin->o_sf);

   e_scrollframe_custom_edje_file_set(hwin->o_sf, buff, 
                                      "modules/illume-home/launcher/scrollview");

   hwin->o_fm = e_fm2_add(e_win_evas_get(hwin->win));
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
     e_drop_xdnd_register_set(hwin->win->evas_win, 1);
}

static void 
_il_home_win_cb_free(Il_Home_Win *hwin) 
{
   if (hwin->win->evas_win)
     e_drop_xdnd_register_set(hwin->win->evas_win, 0);

   if (hwin->cover) e_object_del(E_OBJECT(hwin->cover));
   hwin->cover = NULL;

   if (hwin->o_bg) evas_object_del(hwin->o_bg);
   hwin->o_bg = NULL;
   if (hwin->o_sf) evas_object_del(hwin->o_sf);
   hwin->o_sf = NULL;
   if (hwin->o_fm) evas_object_del(hwin->o_fm);
   hwin->o_fm = NULL;

   if (hwin->win) e_object_del(E_OBJECT(hwin->win));
   hwin->win = NULL;
}

static void 
_il_home_win_cb_resize(E_Win *win) 
{
   Instance *inst;
   Il_Home_Win *hwin;
   Eina_List *l;

   if (!(inst = win->data)) return;
   EINA_LIST_FOREACH(inst->wins, l, hwin) 
     {
        if (hwin->win != win) 
          {
             hwin = NULL;
             continue;
          }
        else break;
     }
   if (!hwin) return;

   if (hwin->o_bg) evas_object_resize(hwin->o_bg, win->w, win->h);
   if (hwin->o_sf) evas_object_resize(hwin->o_sf, win->w, win->h);
   if (hwin->cover) e_busycover_resize(hwin->cover, win->w, win->h);
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
_il_home_cb_selected(void *data, Evas_Object *obj, void *event) 
{
   Il_Home_Win *hwin;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;

   if (!(hwin = data)) return;
   if (!(selected = e_fm2_selected_list_get(hwin->o_fm))) return;
   EINA_LIST_FREE(selected, ici) 
     {
        Efreet_Desktop *desktop;

        if (ici) 
          {
             if (ici->real_link) 
               {
                  desktop = efreet_desktop_get(ici->real_link);
                  if (desktop) 
                    _il_home_desktop_run(hwin, desktop);
               }
          }
     }
}

static void 
_il_home_desktop_run(Il_Home_Win *hwin, Efreet_Desktop *desktop) 
{
   E_Exec_Instance *eins;
   Il_Home_Exec *exe;
   Eina_List *l;
   E_Border *b;
   char buff[PATH_MAX];

   if ((!desktop) || (!desktop->exec)) return;

   EINA_LIST_FOREACH(exes, l, exe) 
     {
        if (exe->desktop == desktop) 
          {
             if ((exe->border) && 
                 (exe->border->zone == hwin->win->border->zone))
               {
                  e_border_uniconify(exe->border);
                  e_border_show(exe->border);
                  e_border_raise(exe->border);
                  e_border_focus_set(exe->border, 1, 1);
                  return;
               }
          }
     }

   b = _il_home_desktop_find_border(hwin->win->border->zone, desktop);
   if (b) 
     {
        e_border_uniconify(b);
        e_border_show(b);
        e_border_raise(b);
        e_border_focus_set(b, 1, 1);
        return;
     }

   exe = E_NEW(Il_Home_Exec, 1);
   if (!exe) return;
   exe->cover = hwin->cover;

   eins = e_exec(hwin->win->border->zone, desktop, NULL, NULL, "illume-home");
   exe->desktop = desktop;
   exe->zone = hwin->win->border->zone;
   if (eins) 
     {
        exe->exec = eins->exe;
        exe->startup_id = eins->startup_id;
        if (eins->exe) 
          exe->pid = ecore_exe_pid_get(eins->exe);
     }

   exe->timeout = ecore_timer_add(20.0, _il_home_win_cb_timeout, exe);
   snprintf(buff, sizeof(buff), "Starting %s", desktop->name);
   exe->handle = e_busycover_push(hwin->cover, buff, NULL);
   exes = eina_list_append(exes, exe);
}

static void 
_il_home_apps_populate(void) 
{
   Eina_List *l, *ll;
   Instance *inst;
   char buff[PATH_MAX];

   e_user_dir_concat_static(buff, "appshadow");
   ecore_file_mkpath(buff);

   _il_home_desks_populate();

   EINA_LIST_FOREACH(instances, l, inst) 
     {
        Il_Home_Win *hwin;

        EINA_LIST_FOREACH(inst->wins, ll, hwin) 
          {
             if (!hwin) continue;
             _il_home_fmc_set(hwin->o_fm);
             e_fm2_path_set(hwin->o_fm, NULL, buff);
          }
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

static void 
_il_home_desks_populate(void) 
{
   Efreet_Menu *menu;

   menu = efreet_menu_get();
   if (menu) 
     {
        Eina_List *l, *ll;
        Efreet_Menu *entry, *subentry;
        Eina_List *settings, *sys, *kbd;
        int num = 0;

        settings = efreet_util_desktop_category_list("Settings");
        sys = efreet_util_desktop_category_list("System");
        kbd = efreet_util_desktop_category_list("Keyboard");
        EINA_LIST_FOREACH(menu->entries, l, entry) 
          {
             if (entry->type != EFREET_MENU_ENTRY_MENU) continue;
             EINA_LIST_FOREACH(entry->entries, ll, subentry) 
               {
		  Efreet_Desktop *desktop;

                  if (subentry->type != EFREET_MENU_ENTRY_DESKTOP) continue;
                  if (!(desktop = subentry->desktop)) continue;
                  if ((settings) && (sys) && 
                      (eina_list_data_find(settings, desktop)) && 
                      (eina_list_data_find(sys, desktop))) continue;
                  if ((kbd) && (eina_list_data_find(kbd, desktop)))
                    continue;
                  if (desktop) 
                    {
                       char buff[PATH_MAX];

                       desks = eina_list_append(desks, desktop);
                       efreet_desktop_ref(desktop);
                       e_user_dir_snprintf(buff, sizeof(buff), 
                                           "appshadow/%04x.desktop", num);
                       ecore_file_symlink(desktop->orig_path, buff);
                       num++;
                    }
               }
          }
	efreet_menu_free(menu);
     }
}

static int 
_il_home_desktop_cache_update(void *data, int type, void *event) 
{
   _il_home_apps_unpopulate();
   if (defer) ecore_timer_del(defer);
   defer = ecore_timer_add(1.0, _il_home_update_deferred, NULL);
   return 1;
}

static int 
_il_home_update_deferred(void *data) 
{
   _il_home_apps_populate();
   defer = NULL;
   return 0;
}

static int 
_il_home_win_cb_exe_del(void *data, int type, void *event) 
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
             return 1;
          }
     }
   return 1;
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
        exe = malloc(p - desktop->exec + 1);
        memset(exe, 0, sizeof(exe));
        if (exe) eina_strlcpy(exe, desktop->exec, p - desktop->exec + 1);
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

static int 
_il_home_win_cb_timeout(void *data) 
{
   Il_Home_Exec *exe;

   if (!(exe = data)) return 1;

   if (exe->handle) e_busycover_pop(exe->cover, exe->handle);
   exe->handle = NULL;

   if (!exe->border) 
     {
        exes = eina_list_remove(exes, exe);
	if (exe->desktop) efreet_desktop_free(exe->desktop);
	E_FREE(exe);
        return 0;
     }
   exe->timeout = NULL;
   return 0;
}

static int 
_il_home_border_add(void *data, int type, void *event) 
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
   return 1;
}

static int 
_il_home_border_remove(void *data, int type, void *event) 
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
             break;
          }
     }
   return 1;
}

static int 
_il_home_cb_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;
   Instance *inst;

   ev = event;
   if (!(inst = data)) return 1;
   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_HOME_NEW) 
     {
        E_Zone *zone;

        zone = inst->gcc->gadcon->zone;
        if (zone->black_win != ev->win) return 1;
        _il_home_win_new(inst);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_HOME_DEL) 
     {
        E_Border *bd;
        Eina_List *l;
        Il_Home_Win *hwin;

        if (!(bd = e_border_find_by_client_window(ev->win))) return 1;
        EINA_LIST_FOREACH(inst->wins, l, hwin) 
          {
             if (hwin->win->border == bd) 
               {
                  inst->wins = eina_list_remove_list(inst->wins, inst->wins);
                  e_object_del(E_OBJECT(hwin));
                  break;
               }
          }
     }

   return 1;
}

static int 
_il_home_cb_prop_change(void *data, int type, void *event) 
{
   Instance *inst;
   Ecore_X_Event_Window_Property *ev;
   Eina_List *l;
   Il_Home_Win *hwin;

   ev = event;
   if (!(inst = data)) return 1;
//   if (ev->win != ecore_x_window_root_first_get()) return 1;
   if (ev->atom != ATM_ENLIGHTENMENT_SCALE) return 1;

   EINA_LIST_FOREACH(inst->wins, l, hwin) 
     if (hwin->o_fm) 
       {
          _il_home_fmc_set(hwin->o_fm);
          e_fm2_refresh(hwin->o_fm);
       }

   return 1;
}

#include "e_mod_main.h"

/* FIXME: fwin - the fm2 filemanager wrapped with a window and scrollframe.
 * primitive BUT enough to test generic dnd and fm stuff more easily. don't
 * play with this unless u want to help with it. NOT COMPLETE! BEWARE!
 */
/* FIXME: multiple selected files across different fwins - you can only dnd the
 * ones in the 1 window src - not all selected ones. also selecting a new file
 * in a new fwin doesn't deseclect other selections in other fwin's (unless
 * multi-selecting)
 */

#define DEFAULT_WIDTH  600
#define DEFAULT_HEIGHT 350

typedef struct _E_Fwin             E_Fwin;
typedef struct _E_Fwin_Page        E_Fwin_Page;
typedef struct _E_Fwin_Apps_Dialog E_Fwin_Apps_Dialog;

#define E_FWIN_TYPE 0xE0b0101f

struct _E_Fwin
{
   E_Object             e_obj_inherit;

   E_Win               *win;
   E_Zone              *zone;
   Fileman_Path *path; /* not freed with fwin, actually attached to config */
   Evas_Object         *bg_obj;
   E_Fwin_Apps_Dialog  *fad;

   E_Fwin_Page         *cur_page;

   Evas_Object         *under_obj;
   Evas_Object         *over_obj;

   const char          *wallpaper_file;
   Eina_Bool            wallpaper_is_edj : 1;
   const char          *overlay_file;
   const char          *scrollframe_file;
   const char          *theme_file;

   Ecore_Timer *popup_timer;
   Eina_List *popup_handlers;
   E_Fm2_Icon_Info *popup_icon;
   E_Popup *popup;

   Ecore_Timer *spring_timer;
   Ecore_Timer *spring_close_timer;
   E_Fwin *spring_parent;
   E_Fwin *spring_child;
   
   Ecore_Event_Handler *zone_handler;
   Ecore_Event_Handler *zone_del_handler;
};

struct _E_Fwin_Page
{
   E_Fwin              *fwin;
   Ecore_Event_Handler *fm_op_entry_add_handler;

   Evas_Object         *flist;
   Evas_Object         *flist_frame;
   Evas_Object         *scrollframe_obj;
   Evas_Object         *scr;
   Evas_Object         *fm_obj;
   E_Toolbar           *tbar;

   struct
   {
      Evas_Coord x, y, max_x, max_y, w, h;
   } fm_pan, fm_pan_last;

   int                  index;
   Eina_Bool           setting : 1;
};

struct _E_Fwin_Apps_Dialog
{
   E_Dialog    *dia;
   E_Fwin      *fwin;
   const char  *app2;
   Evas_Object *o_filepreview;
   Evas_Object *o_all;
   Evas_Object *o_entry;
   char        *exec_cmd;
};

typedef enum
{
   E_FWIN_EXEC_NONE,
   E_FWIN_EXEC_DIRECT,
   E_FWIN_EXEC_SH,
   E_FWIN_EXEC_TERMINAL_DIRECT,
   E_FWIN_EXEC_TERMINAL_SH,
   E_FWIN_EXEC_DESKTOP
} E_Fwin_Exec_Type;

/* local subsystem prototypes */
static int _e_fwin_cb_dir_handler_test(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path);
static void _e_fwin_cb_dir_handler(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path);
static void _e_fwin_page_favorites_add(E_Fwin_Page *page);
static void _e_fwin_icon_mouse_out(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static void _e_fwin_icon_mouse_in(void *data, Evas_Object *obj __UNUSED__, void *event_info);
static E_Fwin *_e_fwin_open(E_Fwin_Page *page, E_Fm2_Icon_Info *ici, Eina_Bool force, int *need_dia);
static E_Fwin          *_e_fwin_new(E_Container *con,
                                    const char *dev,
                                    const char *path);
static void             _e_fwin_free(E_Fwin *fwin);
static E_Fwin_Page     *_e_fwin_page_create(E_Fwin *fwin);
static void             _e_fwin_page_free(E_Fwin_Page *page);
static void             _e_fwin_cb_delete(E_Win *win);
static void             _e_fwin_cb_move(E_Win *win);
static void             _e_fwin_cb_resize(E_Win *win);
static void             _e_fwin_deleted(void *data,
                                        Evas_Object *obj,
                                        void *event_info);
static const char      *_e_fwin_custom_file_path_eval(E_Fwin *fwin,
                                                      Efreet_Desktop *ef,
                                                      const char *prev_path,
                                                      const char *key);
static void             _e_fwin_desktop_run(Efreet_Desktop *desktop,
                                            E_Fwin_Page *page,
                                            Eina_Bool skip_history);
static Eina_List       *_e_fwin_suggested_apps_list_get(Eina_List *files,
                                                        Eina_List **mime_list,
                                                        Eina_Bool *has_default);
static void             _e_fwin_changed(void *data,
                                        Evas_Object *obj,
                                        void *event_info);
static void             _e_fwin_favorite_selected(void *data,
                                                   Evas_Object *obj,
                                                   void *event_info);
static void             _e_fwin_selected(void *data,
                                         Evas_Object *obj,
                                         void *event_info);
static void             _e_fwin_selection_change(void *data,
                                                 Evas_Object *obj,
                                                 void *event_info);
static void             _e_fwin_cb_menu_extend_open_with(void *data,
                                                         E_Menu *m);
static void             _e_fwin_cb_menu_open_fast(void *data,
                                                  E_Menu *m,
                                                  E_Menu_Item *mi);
static void             _e_fwin_parent(void *data,
                                       E_Menu *m,
                                       E_Menu_Item *mi);
static void             _e_fwin_cb_key_down(void *data,
                                            Evas *e,
                                            Evas_Object *obj,
                                            void *event_info);
static void             _e_fwin_cb_menu_extend_start(void *data,
                                                     Evas_Object *obj,
                                                     E_Menu *m,
                                                     E_Fm2_Icon_Info *info);
static void             _e_fwin_cb_menu_open(void *data,
                                             E_Menu *m,
                                             E_Menu_Item *mi);
static void             _e_fwin_cb_menu_open_with(void *data,
                                                  E_Menu *m,
                                                  E_Menu_Item *mi);
static void             _e_fwin_cb_all_change(void *data,
                                              Evas_Object *obj);
static void             _e_fwin_cb_exec_cmd_changed(void *data,
                                                    void *data2);
static void             _e_fwin_cb_open(void *data,
                                        E_Dialog *dia);
static void             _e_fwin_cb_close(void *data,
                                         E_Dialog *dia);
static void             _e_fwin_cb_dialog_free(void *obj);
static E_Fwin_Exec_Type _e_fwin_file_is_exec(E_Fm2_Icon_Info *ici);
static void             _e_fwin_file_exec(E_Fwin_Page *page,
                                          E_Fm2_Icon_Info *ici,
                                          E_Fwin_Exec_Type ext);
static void            _e_fwin_file_open_dialog(E_Fwin_Page *page,
                                                 Eina_List *files,
                                                 int always);
static void             _e_fwin_file_open_dialog_preview_set(void *data1,
                                                             void *data2);
static void             _e_fwin_file_open_dialog_cb_key_down(void *data,
                                                             Evas *e,
                                                             Evas_Object *obj,
                                                             void *event_info);

static void             _e_fwin_pan_set(Evas_Object *obj,
                                        Evas_Coord x,
                                        Evas_Coord y);
static void             _e_fwin_pan_get(Evas_Object *obj,
                                        Evas_Coord *x,
                                        Evas_Coord *y);
static void             _e_fwin_pan_max_get(Evas_Object *obj,
                                            Evas_Coord *x,
                                            Evas_Coord *y);
static void             _e_fwin_pan_child_size_get(Evas_Object *obj,
                                                   Evas_Coord *w,
                                                   Evas_Coord *h);
static void             _e_fwin_pan_scroll_update(E_Fwin_Page *page);
static void             _e_fwin_cb_page_obj_del(void *data, Evas *evas,
                                                Evas_Object *obj, void *event_info);
static void             _e_fwin_zone_cb_mouse_down(void *data,
                                                   Evas *evas,
                                                   Evas_Object *obj,
                                                   void *event_info);
static void             _e_fwin_zone_focus_out(void *data,
                                                   Evas *evas,
                                                   Evas_Object *obj,
                                                   void *event_info);
static void             _e_fwin_zone_focus_in(void *data,
                                                   Evas *evas,
                                                   void *event_info);
static Eina_Bool        _e_fwin_zone_move_resize(void *data,
                                                 int type,
                                                 void *event);
static Eina_Bool        _e_fwin_zone_del(void *data,
                                         int type,
                                         void *event);
static void             _e_fwin_config_set(E_Fwin_Page *page);
static void             _e_fwin_window_title_set(E_Fwin_Page *page);
static void             _e_fwin_toolbar_resize(E_Fwin_Page *page);
static int              _e_fwin_dlg_cb_desk_sort(const void *p1,
                                                 const void *p2);
static int              _e_fwin_dlg_cb_desk_list_sort(const void *data1,
                                                      const void *data2);

static void             _e_fwin_op_registry_listener_cb(void *data,
                                                        const E_Fm2_Op_Registry_Entry *ere);
static Eina_Bool        _e_fwin_op_registry_entry_add_cb(void *data,
                                                         int type,
                                                         void *event);
static void             _e_fwin_op_registry_entry_iter(E_Fwin_Page *page);
static void             _e_fwin_op_registry_abort_cb(void *data,
                                                     Evas_Object *obj,
                                                     const char *emission,
                                                     const char *source);

static E_Fwin *drag_fwin = NULL;

/* local subsystem globals */
static Eina_List *fwins = NULL;
static E_Fm2_Mime_Handler *dir_handler = NULL;
static Efreet_Desktop *tdesktop = NULL;

/* externally accessible functions */
int
e_fwin_init(void)
{
   tdesktop = e_util_terminal_desktop_get();
   if (!tdesktop) return 1;
   dir_handler = e_fm2_mime_handler_new(_("Open Terminal here"),
                                   tdesktop->icon,
                                   _e_fwin_cb_dir_handler, NULL,
                                   _e_fwin_cb_dir_handler_test, NULL);
   e_fm2_mime_handler_mime_add(dir_handler, "inode/directory");
   return 1;
}

int
e_fwin_shutdown(void)
{
   E_Fwin *fwin;

   EINA_LIST_FREE(fwins, fwin)
     e_object_del(E_OBJECT(fwin));

   if (dir_handler)
     {
        e_fm2_mime_handler_mime_del(dir_handler, "inode/directory");
        e_fm2_mime_handler_free(dir_handler);
     }
   efreet_desktop_free(tdesktop);

   tdesktop = NULL;
   dir_handler = NULL;

   return 1;
}

/* FIXME: this opens a new window - we need a way to inherit a zone as the
 * "fwin" window
 */
void
e_fwin_new(E_Container *con,
           const char *dev,
           const char *path)
{
   _e_fwin_new(con, dev, path);
}

static Eina_Bool
_e_fwin_spring_cb(E_Fwin *fwin)
{
   E_Fm2_Icon_Info *ici;
   E_Fwin *f;

   if (fwin->spring_child)
     _e_fwin_free(fwin->spring_child);

   ici = e_fm2_drop_icon_get(fwin->cur_page->fm_obj);
   if (!ici)
     ici = e_fm2_drop_icon_get(fwin->cur_page->flist);
   while (ici)
     {
        /* FIXME: could use an animation here */
        f = _e_fwin_open(fwin->cur_page, ici, EINA_TRUE, NULL);
        if (!f) break;
        f->spring_parent = fwin;
        fwin->spring_child = f;
        break;
     }
   if (fwin->spring_timer) ecore_timer_del(fwin->spring_timer);
   fwin->spring_timer = NULL;
   return EINA_FALSE;
}

/* called on the drop source */
static void
_e_fwin_dnd_end_cb(E_Fwin *fwin, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if (fwin->spring_timer) ecore_timer_del(fwin->spring_timer);
   fwin->spring_timer = NULL;
   if (!drag_fwin) return;
   if (drag_fwin->spring_timer) ecore_timer_del(drag_fwin->spring_timer);
   drag_fwin->spring_timer = NULL;

   /* NOTE: closing the drop target window here WILL break things */
   fwin = drag_fwin->spring_parent;
   if (!fwin) return;

   fwin->spring_child->spring_parent = NULL;
   fwin->spring_child = NULL;
   while (fwin->spring_parent)
     {
        /* FIXME: needs closing animation? */
        fwin = fwin->spring_parent;
        _e_fwin_free(fwin->spring_child);
     }
   drag_fwin = NULL;
}

static void
_e_fwin_dnd_change_cb(E_Fwin *fwin, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   drag_fwin = fwin;
   if (fwin->spring_timer)
     ecore_timer_reset(fwin->spring_timer);
   else
     fwin->spring_timer = ecore_timer_add(fileman_config->view.spring_delay, (Ecore_Task_Cb)_e_fwin_spring_cb, fwin);
}

static void
_e_fwin_dnd_enter_cb(E_Fwin *fwin, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if (drag_fwin == fwin) return;
   if (fwin->spring_timer) ecore_timer_del(fwin->spring_timer);
   fwin->spring_timer = NULL;
   if (fwin->spring_child && (drag_fwin == fwin->spring_child)) _e_fwin_free(fwin->spring_child);
   drag_fwin = fwin;
   if (fwin->spring_close_timer) ecore_timer_del(fwin->spring_close_timer);
   fwin->spring_close_timer = NULL;
}

static Eina_Bool
_e_fwin_dnd_close_cb(E_Fwin *fwin)
{
   _e_fwin_free(fwin);
   return EINA_FALSE;
}

static void
_e_fwin_dnd_leave_cb(E_Fwin *fwin, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if (fwin->spring_timer) ecore_timer_del(fwin->spring_timer);
   fwin->spring_timer = NULL;
   if (fwin->spring_parent && (!fwin->spring_child))
     {
        if (!fwin->spring_close_timer)
          fwin->spring_close_timer = ecore_timer_add(0.01, (Ecore_Task_Cb)_e_fwin_dnd_close_cb, fwin);
     }
   drag_fwin = NULL;
}

static void
_e_fwin_dnd_begin_cb(E_Fwin *fwin __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   drag_fwin = NULL;
}

void
e_fwin_zone_new(E_Zone *zone, void *p)
{
   E_Fwin *fwin;
   Fileman_Path *path = p;
   E_Fwin_Page *page;
   Evas_Object *o;
   int x, y, w, h;

   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return;
   fwin->zone = zone;

   page = E_NEW(E_Fwin_Page, 1);
   page->fwin = fwin;
   fwin->path = path;

   /* Add Event Handler for zone move/resize & del */
   fwin->zone_handler =
     ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
                             _e_fwin_zone_move_resize, fwin);
   fwin->zone_del_handler =
     ecore_event_handler_add(E_EVENT_ZONE_DEL,
                             _e_fwin_zone_del, fwin);

   /* Trap the mouse_down on zone so we can unselect */
   evas_object_event_callback_add(zone->bg_event_object,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_fwin_zone_cb_mouse_down, fwin);

   fwins = eina_list_append(fwins, fwin);

   o = e_fm2_add(zone->container->bg_evas);
   page->fm_obj = o;
   evas_event_callback_add(zone->container->bg_evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _e_fwin_zone_focus_in, fwin);
   evas_object_event_callback_add(o, EVAS_CALLBACK_FOCUS_OUT, _e_fwin_zone_focus_out, fwin);
   _e_fwin_config_set(page);

   e_fm2_custom_theme_content_set(o, "desktop");

   evas_object_smart_callback_add(o, "changed",
                                  _e_fwin_icon_mouse_out, fwin);
   evas_object_smart_callback_add(o, "dir_changed",
                                  _e_fwin_changed, page);
   evas_object_smart_callback_add(o, "dir_deleted",
                                  _e_fwin_deleted, page);
   evas_object_smart_callback_add(o, "selected",
                                  _e_fwin_selected, page);
   evas_object_smart_callback_add(o, "selection_change",
                                  _e_fwin_selection_change, page);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                  _e_fwin_cb_page_obj_del, page);
   evas_object_smart_callback_add(o, "dnd_enter", (Evas_Smart_Cb)_e_fwin_dnd_enter_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_leave", (Evas_Smart_Cb)_e_fwin_dnd_leave_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_changed", (Evas_Smart_Cb)_e_fwin_dnd_change_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_begin", (Evas_Smart_Cb)_e_fwin_dnd_begin_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_end", (Evas_Smart_Cb)_e_fwin_dnd_end_cb, fwin);
   evas_object_smart_callback_add(o, "icon_mouse_in", (Evas_Smart_Cb)_e_fwin_icon_mouse_in, fwin);
   evas_object_smart_callback_add(o, "icon_mouse_out", (Evas_Smart_Cb)_e_fwin_icon_mouse_out, fwin);
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, page);
   e_fm2_underlay_hide(o);
   evas_object_show(o);

   o = e_scrollframe_add(zone->container->bg_evas);
   ecore_x_icccm_state_set(zone->container->bg_win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
   e_drop_xdnd_register_set(zone->container->event_win, 1);
   e_scrollframe_custom_theme_set(o, "base/theme/fileman",
                                  "e/fileman/desktop/scrollframe");
   /* FIXME: this theme object will have more versions and options later
    * for things like swallowing widgets/buttons ot providing them - a
    * gadcon for starters for fm widgets. need to register the owning
    * e_object of the gadcon so gadcon clients can get it and thus do
    * things like find out what dirs/path the fwin is for etc. this will
    * probably be how you add optional gadgets to fwin views like empty/full
    * meters for disk usage, and other dir info/stats or controls. also it
    * might be possible that we can have custom frames per dir later so need
    * a way to set an edje file directly
    */
   /* FIXME: allow specialised scrollframe obj per dir - get from e config,
    * then look in the dir itself for a magic dot-file, if not - use theme.
    * same as currently done for bg & overlay. also add to fm2 the ability
    * to specify the .edj files to get the list and icon theme stuff from
    */
   evas_object_data_set(page->fm_obj, "fm_page", page);
   evas_object_data_set(page->fm_obj, "page_is_zone", page);
   e_scrollframe_extern_pan_set(o, page->fm_obj,
                                _e_fwin_pan_set,
                                _e_fwin_pan_get,
                                _e_fwin_pan_max_get,
                                _e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(page->fm_obj, 0);
   e_widget_can_focus_set(o, 0);
   page->scrollframe_obj = page->scr = o;

   e_zone_useful_geometry_get(zone, &x, &y, &w, &h);
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);
   evas_object_show(o);

   e_fm2_window_object_set(page->fm_obj, E_OBJECT(fwin->zone));

   evas_object_focus_set(page->fm_obj, 1);

   e_fm2_path_set(page->fm_obj, path->dev, path->path);

   fwin->cur_page = page;
}

void
e_fwin_all_unsel(void *data)
{
   E_Fwin *fwin;

   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   e_fm2_all_unsel(fwin->cur_page->fm_obj);
}

void
e_fwin_zone_shutdown(E_Zone *zone)
{
   Eina_List *f, *fn;
   E_Fwin *win;
   const char *dev, *path;

   EINA_LIST_FOREACH_SAFE(fwins, f, fn, win)
     {
        if (win->zone != zone) continue;
        win->path->desktop_mode = e_fm2_view_mode_get(win->cur_page->fm_obj);
        e_fm2_path_get(win->cur_page->fm_obj, &dev, &path);
        eina_stringshare_replace(&win->path->dev, dev);
        eina_stringshare_replace(&win->path->path, path);
        evas_event_callback_del_full(zone->container->bg_evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _e_fwin_zone_focus_in, win);
        e_object_del(E_OBJECT(win));
        win = NULL;
     }
}

void
e_fwin_reload_all(void)
{
   Eina_List *l, *ll, *lll;
   E_Container *con;
   E_Manager *man;
   E_Fwin *fwin;
   E_Zone *zone;

   /* Reload/recreate zones cause of property changes */
   EINA_LIST_FOREACH(fwins, l, fwin)
     {
        if (!fwin) continue;  //safety
        if (fwin->zone)
          e_fwin_zone_shutdown(fwin->zone);
        else
          {
             _e_fwin_config_set(fwin->cur_page);
             if (fileman_config->view.show_toolbar)
               {
                  if (!fwin->cur_page->tbar)
                    {
                       fwin->cur_page->tbar = e_toolbar_new
                         (e_win_evas_get(fwin->win), "toolbar",
                             fwin->win, fwin->cur_page->fm_obj);
                       e_toolbar_orient(fwin->cur_page->tbar, fileman_config->view.toolbar_orient);
                    }
               }
             else
               {
                  if (fwin->cur_page->tbar)
                    {
                       fileman_config->view.toolbar_orient = fwin->cur_page->tbar->gadcon->orient;
                       e_object_del(E_OBJECT(fwin->cur_page->tbar));
                       fwin->cur_page->tbar = NULL;
                    }
               }
             if (fileman_config->view.show_sidebar)
               {
                  if (!fwin->cur_page->flist_frame)
                    {
                       _e_fwin_page_favorites_add(fwin->cur_page);
                       edje_object_signal_emit(fwin->bg_obj, "e,favorites,enabled", "e");
                       edje_object_message_signal_process(fwin->bg_obj);
                    }
               }
             else
               {
                  if (fwin->cur_page->flist_frame)
                    {
                       evas_object_del(fwin->cur_page->flist_frame);
                       fwin->cur_page->flist_frame = fwin->cur_page->flist = NULL;
                       edje_object_signal_emit(fwin->bg_obj, "e,favorites,disabled", "e");
                       edje_object_message_signal_process(fwin->bg_obj);
                    }
               }
             _e_fwin_window_title_set(fwin->cur_page);
             _e_fwin_cb_resize(fwin->win);
             _e_fwin_toolbar_resize(fwin->cur_page);
             e_fm2_refresh(fwin->cur_page->fm_obj);
          }
     }

   /* Hook into zones */
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     EINA_LIST_FOREACH(man->containers, ll, con)
       EINA_LIST_FOREACH(con->zones, lll, zone)
         {
            if (e_fwin_zone_find(zone)) continue;
            if (fileman_config->view.show_desktop_icons)
              e_fwin_zone_new(zone, e_mod_fileman_path_find(zone));
         }
}

int
e_fwin_zone_find(E_Zone *zone)
{
   Eina_List *f;
   E_Fwin *win;

   EINA_LIST_FOREACH(fwins, f, win)
     if (win->zone == zone) return 1;
   return 0;
}

/* local subsystem functions */
static void
_e_fwin_bg_mouse_down(E_Fwin *fwin, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   int x, y, w, h, zx, zy, zw, zh;
   e_zone_useful_geometry_get(fwin->win->border->zone, &zx, &zy, &zw, &zh);
   x = fwin->win->border->x, y = fwin->win->border->y;
   e_fm2_optimal_size_calc(fwin->cur_page->fm_obj, zw - x, zh - y, &w, &h);
   if (x + w > zx + zw)
     w = zx + zw - x;
   if (y + x > zy + zh)
     h = zy + zh - y;
   e_win_resize(fwin->win, w, h);
}

static E_Fwin *
_e_fwin_new(E_Container *con,
            const char *dev,
            const char *path)
{
   E_Fwin *fwin;
   E_Fwin_Page *page;
   Evas_Object *o;
   E_Zone *zone;

   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return NULL;
   fwin->win = e_win_new(con);
   if (!fwin->win)
     {
        free(fwin);
        return NULL;
     }
   fwins = eina_list_append(fwins, fwin);
   e_win_delete_callback_set(fwin->win, _e_fwin_cb_delete);
   e_win_move_callback_set(fwin->win, _e_fwin_cb_move);
   e_win_resize_callback_set(fwin->win, _e_fwin_cb_resize);
   fwin->win->data = fwin;

   o = edje_object_add(e_win_evas_get(fwin->win));
   e_theme_edje_object_set(o, "base/theme/fileman",
                           "e/fileman/default/window/main");
   evas_object_show(o);
   fwin->bg_obj = o;

   page = _e_fwin_page_create(fwin);
   fwin->cur_page = page;

/*
   o = edje_object_add(fwin->win->evas);
   //   o = e_icon_add(e_win_evas_get(fwin->win));
   //   e_icon_scale_size_set(o, 0);
   //   e_icon_fill_inside_set(o, 0);
   edje_object_part_swallow(fwin->bg_obj, "e.swallow.bg", o);
   evas_object_pass_events_set(o, 1);
   fwin->under_obj = o;
 */

   o = edje_object_add(fwin->win->evas);
//   o = e_icon_add(e_win_evas_get(fwin->win));
//   e_icon_scale_size_set(o, 0);
//   e_icon_fill_inside_set(o, 0);
   edje_object_part_swallow(e_scrollframe_edje_object_get(page->scr), 
                            "e.swallow.overlay", o);
   evas_object_pass_events_set(o, 1);
   fwin->over_obj = o;

   e_fm2_path_set(page->fm_obj, dev, path);
   _e_fwin_window_title_set(page);

   e_win_size_min_set(fwin->win, 360, 250);

   zone = e_util_zone_current_get(e_manager_current_get());
   if ((zone) && (zone->w < DEFAULT_WIDTH))
     {
        int w, h;
        e_zone_useful_geometry_get(zone, NULL, NULL, &w, &h);
        e_win_resize(fwin->win, w, h);
     }
   else
     e_win_resize(fwin->win, DEFAULT_WIDTH, DEFAULT_HEIGHT);

   e_win_show(fwin->win);
   if (fwin->win->evas_win)
     e_drop_xdnd_register_set(fwin->win->evas_win, 1);
   if (fwin->win->border)
     eina_stringshare_replace(&fwin->win->border->internal_icon,
                              "system-file-manager");

   return fwin;
}

static void
_e_fwin_free(E_Fwin *fwin)
{
   if (!fwin) return;  //safety

   _e_fwin_page_free(fwin->cur_page);

   if (fwin->zone)
     evas_object_event_callback_del(fwin->zone->bg_event_object,
                                    EVAS_CALLBACK_MOUSE_DOWN,
                                    _e_fwin_zone_cb_mouse_down);
   if (fwin->zone_handler)
     ecore_event_handler_del(fwin->zone_handler);
   if (fwin->zone_del_handler)
     ecore_event_handler_del(fwin->zone_del_handler);
   if (fwin->spring_timer) ecore_timer_del(fwin->spring_timer);
   fwin->spring_timer = NULL;
   if (fwin->spring_close_timer) ecore_timer_del(fwin->spring_close_timer);
   fwin->spring_close_timer = NULL;
   fwins = eina_list_remove(fwins, fwin);
   if (fwin->wallpaper_file) eina_stringshare_del(fwin->wallpaper_file);
   if (fwin->overlay_file) eina_stringshare_del(fwin->overlay_file);
   if (fwin->scrollframe_file) eina_stringshare_del(fwin->scrollframe_file);
   if (fwin->theme_file) eina_stringshare_del(fwin->theme_file);
   if (fwin->fad)
     {
        e_object_del(E_OBJECT(fwin->fad->dia));
        fwin->fad = NULL;
     }
   if (fwin->popup) e_object_del(E_OBJECT(fwin->popup));
   if (fwin->popup_timer) ecore_timer_del(fwin->popup_timer);
   fwin->popup_timer = NULL;
   E_FREE_LIST(fwin->popup_handlers, ecore_event_handler_del);
   if (fwin->spring_parent) fwin->spring_parent->spring_child = NULL;
   if (fwin->win) e_object_del(E_OBJECT(fwin->win));
   free(fwin);
}

static Eina_Bool
_e_fwin_icon_popup_handler(void *data, int type, void *event)
{
   E_Fwin *fwin = data;
   Ecore_Event_Mouse_IO *ev = event;

   if (type == ECORE_X_EVENT_MOUSE_IN)
     {
        if (fwin->zone)
          {
             if (ev->event_window == fwin->zone->container->event_win) return ECORE_CALLBACK_RENEW;
          }
        else
          {
             if (ev->event_window == fwin->win->border->client.win) return ECORE_CALLBACK_RENEW;
          }
     }
   if (fwin->popup_timer) ecore_timer_del(fwin->popup_timer);
   if (fwin->popup) e_object_del(E_OBJECT(fwin->popup));
   E_FREE_LIST(fwin->popup_handlers, ecore_event_handler_del);
   fwin->popup_icon = NULL;
   fwin->popup_timer = NULL;
   fwin->popup = NULL;
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_fwin_icon_popup(void *data)
{
   E_Fwin *fwin = data;
   Evas_Object *bg, *list, *o;
   E_Zone *zone;
   char buf[4096];
   int x, y, w, h, mw, mh, fx, fy, px, py;

   fwin->popup_timer = NULL;
   if (!fwin->popup_icon) return EINA_FALSE;
   snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->cur_page->fm_obj), fwin->popup_icon->file);
   if (!ecore_file_can_read(buf)) return EINA_FALSE;
   if (fwin->popup) e_object_del(E_OBJECT(fwin->popup));
   zone = fwin->zone ?: fwin->win->border->zone;
   e_fm2_icon_geometry_get(fwin->popup_icon->ic, &x, &y, &w, &h);
   if (fwin->zone)
     {
        evas_object_geometry_get(fwin->popup_icon->fm, &fx, &fy, NULL, NULL);
        fx -= fwin->zone->x;
        x -= fwin->zone->x;
        fy -= fwin->zone->y;
        y -= fwin->zone->y;
     }
   else
     fx = fwin->win->x, fy = fwin->win->y;
   fwin->popup = e_popup_new(zone, 0, 0, 1, 1);
   e_popup_ignore_events_set(fwin->popup, 1);
   ecore_x_window_shape_input_rectangle_set(fwin->popup->evas_win, 0, 0, 0, 0);
   
   bg = edje_object_add(fwin->popup->evas);
   e_theme_edje_object_set(bg, "base/theme/fileman",
                           "e/fileman/popup/default");
   e_popup_edje_bg_object_set(fwin->popup, bg);
   mw = zone->w * fileman_config->tooltip.size / 100.0;
   mh = zone->h * fileman_config->tooltip.size / 100.0;

   edje_object_part_text_set(bg, "e.text.title", 
                             fwin->popup_icon->label ?
                             fwin->popup_icon->label : fwin->popup_icon->file);
   
   list = e_widget_list_add(fwin->popup->evas, 0, 0);
   
   o = e_widget_filepreview_add(fwin->popup->evas, mw, mh, 0);
   e_widget_filepreview_path_set(o, buf, fwin->popup_icon->mime);
   e_widget_list_object_append(list, o, 1, 0, 0.5);
   e_widget_size_min_get(list, &mw, &mh);
   evas_object_size_hint_min_set(list, mw, mh);
   edje_object_part_swallow(bg, "e.swallow.content", list);
   
   edje_object_size_min_calc(bg, &mw, &mh);
   evas_object_show(o);
   evas_object_show(list);
   evas_object_show(bg);

   /* prefer tooltip left of icon */
   px = (fx + x) - mw - 3;
   /* if it's offscreen, try right of icon */
   if (px < 0) px = (fx + x + w) + 3;
   /* fuck this, stick it right on the icon */
   if (px + mw + 3 > zone->w)
     px = (x + w / 2) - (mw / 2);
   /* give up */
   if (px < 0) px = 0;

   /* prefer tooltip above icon */
   py = (fy + y) - mh - 3;
   /* if it's offscreen, try below icon */
   if (py < 0) py = (fy + y + h) + 3;
   /* fuck this, stick it right on the icon */
   if (py + mh + 3 > zone->h)
     py = (y + h / 2) - (mh / 2);
   /* give up */
   if (py < 0) py = 0;
   e_popup_move_resize(fwin->popup, px, py, mw, mh);
   evas_object_resize(bg, mw, mh);
   if (!fwin->popup_handlers)
     {
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_XDND_ENTER, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_XDND_POSITION, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_MOUSE_IN, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_MOUSE_OUT, _e_fwin_icon_popup_handler, fwin);
     }
   e_popup_show(fwin->popup);
   return EINA_FALSE;
}

static void
_e_fwin_icon_mouse_out(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Fwin *fwin = data;

   if (fwin->popup_timer) ecore_timer_del(fwin->popup_timer);
   if (fwin->popup) e_object_del(E_OBJECT(fwin->popup));
   fwin->popup = NULL;
   fwin->popup_timer = NULL;
   fwin->popup_icon = NULL;
}

static void
_e_fwin_icon_mouse_in(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   E_Fwin *fwin = data;
   E_Fm2_Icon_Info *ici = event_info;

   if (fwin->popup_timer) ecore_timer_del(fwin->popup_timer);
   fwin->popup_timer = NULL;
   if (!fileman_config->tooltip.enable) return;
   fwin->popup_timer = ecore_timer_add(fileman_config->tooltip.delay, _e_fwin_icon_popup, fwin);
   fwin->popup_icon = ici;
   if (!fwin->popup_handlers)
     {
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_XDND_ENTER, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_XDND_POSITION, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_MOUSE_IN, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_fwin_icon_popup_handler, fwin);
        E_LIST_HANDLER_APPEND(fwin->popup_handlers, ECORE_X_EVENT_MOUSE_OUT, _e_fwin_icon_popup_handler, fwin);
     }
}

static void
_e_fwin_page_favorites_add(E_Fwin_Page *page)
{
   E_Fm2_Config fmc;
   Evas_Object *o;
   Evas *evas = evas_object_evas_get(page->fwin->bg_obj);

   o = e_fm2_add(evas);
   evas_object_data_set(o, "fm_page", page);
   page->flist = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 1;
   fmc.view.no_subdir_jump = 1;
   fmc.view.link_drop = 1;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   fmc.view.no_click_rename = 1;
   e_fm2_config_set(o, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_NEW | E_FM2_MENU_NO_ACTIVATE_CHANGE | E_FM2_MENU_NO_VIEW_CHANGE);
   evas_object_smart_callback_add(o, "selected", _e_fwin_favorite_selected, page);
   evas_object_smart_callback_add(o, "dnd_enter", (Evas_Smart_Cb)_e_fwin_dnd_enter_cb, page->fwin);
   evas_object_smart_callback_add(o, "dnd_leave", (Evas_Smart_Cb)_e_fwin_dnd_leave_cb, page->fwin);
   evas_object_smart_callback_add(o, "dnd_changed", (Evas_Smart_Cb)_e_fwin_dnd_change_cb, page->fwin);
   evas_object_smart_callback_add(o, "dnd_begin", (Evas_Smart_Cb)_e_fwin_dnd_begin_cb, page->fwin);
   evas_object_smart_callback_add(o, "dnd_end", (Evas_Smart_Cb)_e_fwin_dnd_end_cb, page->fwin);
   e_fm2_path_set(o, "favorites", "/");

   o = e_widget_scrollframe_pan_add(evas, page->flist,
                                    e_fm2_pan_set,
                                    e_fm2_pan_get,
                                    e_fm2_pan_max_get,
                                    e_fm2_pan_child_size_get);
   e_scrollframe_custom_theme_set(e_widget_scrollframe_object_get(o), 
                                  "base/theme/fileman",
                                  "e/fileman/default/scrollframe");
   evas_object_propagate_events_set(page->flist, 0);
   e_widget_can_focus_set(o, EINA_FALSE);
   e_fm2_window_object_set(page->flist, E_OBJECT(page->fwin->win));
   e_widget_scrollframe_focus_object_set(o, page->flist);

   page->flist_frame = o;
   evas_object_size_hint_min_set(o, 128, 0);
   edje_object_part_swallow(page->fwin->bg_obj, "e.swallow.favorites", o);
}

static E_Fwin_Page *
_e_fwin_page_create(E_Fwin *fwin)
{
   Evas_Object *o;
   E_Fwin_Page *page;
   Evas *evas;

   page = E_NEW(E_Fwin_Page, 1);
   page->fwin = fwin;
   evas = e_win_evas_get(fwin->win);

   if (fileman_config->view.show_sidebar)
     {
        _e_fwin_page_favorites_add(page);
        edje_object_signal_emit(fwin->bg_obj, "e,favorites,enabled", "e");
        edje_object_message_signal_process(fwin->bg_obj);
     }

   o = e_fm2_add(evas);
   page->fm_obj = o;
   e_fm2_view_flags_set(o, E_FM2_VIEW_DIR_CUSTOM);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_fwin_cb_key_down, page);

   evas_object_smart_callback_add(o, "changed", _e_fwin_icon_mouse_out, fwin);
   evas_object_smart_callback_add(o, "dir_changed",
                                  _e_fwin_changed, page);
   evas_object_smart_callback_add(o, "dir_deleted",
                                  _e_fwin_deleted, page);
   evas_object_smart_callback_add(o, "selected",
                                  _e_fwin_selected, page);
   evas_object_smart_callback_add(o, "selection_change",
                                  _e_fwin_selection_change, page);
   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                  _e_fwin_cb_page_obj_del, page);
   evas_object_smart_callback_add(o, "dnd_enter", (Evas_Smart_Cb)_e_fwin_dnd_enter_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_leave", (Evas_Smart_Cb)_e_fwin_dnd_leave_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_changed", (Evas_Smart_Cb)_e_fwin_dnd_change_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_begin", (Evas_Smart_Cb)_e_fwin_dnd_begin_cb, fwin);
   evas_object_smart_callback_add(o, "dnd_end", (Evas_Smart_Cb)_e_fwin_dnd_end_cb, fwin);
   evas_object_smart_callback_add(o, "double_clicked", (Evas_Smart_Cb)_e_fwin_bg_mouse_down, fwin);
   evas_object_smart_callback_add(o, "icon_mouse_in", (Evas_Smart_Cb)_e_fwin_icon_mouse_in, fwin);
   evas_object_smart_callback_add(o, "icon_mouse_out", (Evas_Smart_Cb)_e_fwin_icon_mouse_out, fwin);
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, page);
   e_fm2_window_object_set(o, E_OBJECT(fwin->win));
   evas_object_focus_set(o, 1);
   _e_fwin_config_set(page);

   /* FIXME: this theme object will have more versions and options later
    * for things like swallowing widgets/buttons ot providing them - a
    * gadcon for starters for fm widgets. need to register the owning
    * e_object of the gadcon so gadcon clients can get it and thus do
    * things like find out what dirs/path the fwin is for etc. this will
    * probably be how you add optional gadgets to fwin views like empty/full
    * meters for disk usage, and other dir info/stats or controls. also it
    * might be possible that we can have custom frames per dir later so need
    * a way to set an edje file directly
    */
   /* FIXME: allow specialised scrollframe obj per dir - get from e config,
    * then look in the dir itself for a magic dot-file, if not - use theme.
    * same as currently done for bg & overlay. also add to fm2 the ability
    * to specify the .edj files to get the list and icon theme stuff from
    */
   evas_object_data_set(page->fm_obj, "fm_page", page);
   evas_object_data_set(page->fm_obj, "page_is_window", page);
   o = e_widget_scrollframe_pan_add(evas, page->fm_obj,
                                    e_fm2_pan_set,
                                    e_fm2_pan_get,
                                    e_fm2_pan_max_get,
                                    e_fm2_pan_child_size_get);
   evas_object_propagate_events_set(page->fm_obj, 0);
   e_widget_can_focus_set(o, EINA_FALSE);
   page->scrollframe_obj = o;
   page->scr = e_widget_scrollframe_object_get(o);
   e_scrollframe_key_navigation_set(o, EINA_FALSE);
   e_scrollframe_custom_theme_set(o, "base/theme/fileman",
                                  "e/fileman/default/scrollframe");
   edje_object_part_swallow(fwin->bg_obj, "e.swallow.content", o);
   e_widget_scrollframe_focus_object_set(o, page->fm_obj);

   if (fileman_config->view.show_toolbar)
     {
        page->tbar = e_toolbar_new(evas, "toolbar",
                                   fwin->win, page->fm_obj);
        e_toolbar_orient(page->tbar, fileman_config->view.toolbar_orient);
     }

   page->fm_op_entry_add_handler =
     ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_ADD,
                             _e_fwin_op_registry_entry_add_cb, page);
   _e_fwin_op_registry_entry_iter(page);
   _e_fwin_toolbar_resize(page);
   return page;
}

static void
_e_fwin_page_free(E_Fwin_Page *page)
{
   if (page->fm_obj) evas_object_del(page->fm_obj);
   if (page->tbar)
     {
        fileman_config->view.toolbar_orient = page->tbar->gadcon->orient;
        e_object_del(E_OBJECT(page->tbar));
     }
   else evas_object_del(page->scrollframe_obj);

   if (page->fm_op_entry_add_handler)
     ecore_event_handler_del(page->fm_op_entry_add_handler);

   E_FREE(page);
}

static const char *
_e_fwin_custom_file_path_eval(E_Fwin *fwin,
                              Efreet_Desktop *ef,
                              const char *prev_path,
                              const char *key)
{
   char buf[PATH_MAX];
   const char *res, *ret = NULL;

   /* get a X-something custom tage from the .desktop for the dir */
   res = eina_hash_find(ef->x, key);
   /* free the old path */
   if (prev_path) eina_stringshare_del(prev_path);
   /* if there was no key found - return NULL */
   if (!res) return NULL;

   /* it's a full path */
   if (res[0] == '/')
     ret = eina_stringshare_add(res);
   /* relative path to the dir */
   else
     {
        snprintf(buf, sizeof(buf), "%s/%s",
                 e_fm2_real_path_get(fwin->cur_page->fm_obj), res);
        ret = eina_stringshare_add(buf);
     }
   return ret;
}

static Eina_List *
_e_fwin_defaults_apps_get(const char *mime, const char *path)
{
   Efreet_Ini *ini;
   const char *str;
   Eina_List *apps = NULL;
   char **array, **itr;

   if (!ecore_file_exists(path)) return NULL;

   ini = efreet_ini_new(path);
   if (!ini) return NULL;

   efreet_ini_section_set(ini, "Default Applications");
   str = efreet_ini_string_get(ini, mime);
   if (!str) goto end;

   array = eina_str_split(str, ";", 0);
   if (!array) goto end;

   for (itr = array; *itr != NULL; itr++)
     {
        const char *name = *itr;
        Efreet_Desktop *desktop;

        if (name[0] == '/')
          desktop = efreet_desktop_new(name);
        else
          desktop = efreet_util_desktop_file_id_find(name);

        if (!desktop) continue;
        if (!desktop->exec)
          {
             efreet_desktop_free(desktop);
             continue;
          }

        apps = eina_list_append(apps, desktop);
     }

   free(array[0]);
   free(array);
 end:
   efreet_ini_free(ini);
   return apps;
}

static Eina_List *
_e_fwin_suggested_apps_list_sort(const char *mime, Eina_List *desktops, Eina_Bool *has_default)
{
   char path[PATH_MAX];
   Eina_List *order, *l;
   Efreet_Desktop *desktop;

   snprintf(path, sizeof(path), "%s/applications/defaults.list",
            efreet_data_home_get());
   order = _e_fwin_defaults_apps_get(mime, path);

   if (!order)
     {
        const Eina_List *n, *dirs = efreet_data_dirs_get();
        const char *d;
        EINA_LIST_FOREACH(dirs, n, d)
          {
             snprintf(path, sizeof(path), "%s/applications/defaults.list", d);
             order = _e_fwin_defaults_apps_get(mime, path);
             if (order)
               break;
          }
     }

   if (!order)
     {
        if (has_default) *has_default = EINA_FALSE;
        return desktops;
     }

   EINA_LIST_FOREACH(order, l, desktop)
     {
        Eina_List *node = eina_list_data_find_list(desktops, desktop);
        if (!node) continue;
        desktops = eina_list_remove_list(desktops, node);
        efreet_desktop_free(desktop);
     }

   if (has_default) *has_default = EINA_TRUE;

   return eina_list_merge(order, desktops);
}

static Eina_List *
_e_fwin_suggested_apps_list_get(Eina_List *files,
                                Eina_List **mime_list,
                                Eina_Bool *has_default)
{
   E_Fm2_Icon_Info *ici;
   Eina_Hash *set_mimes;
   Eina_List *apps = NULL, *l;

   set_mimes = eina_hash_string_small_new(NULL);
   EINA_LIST_FOREACH(files, l, ici)
     if (!((ici->link) && (ici->mount)))
       {
          if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE)
            {
               const char *key = ici->mime;
               if (ici->link)
                 key = efreet_mime_globs_type_get(ici->link);

               if (!eina_hash_find(set_mimes, key))
                 eina_hash_direct_add(set_mimes, key, (void *)1);
            }
       }

   if (mime_list) *mime_list = NULL;
   if (has_default) *has_default = EINA_FALSE;

   if (eina_hash_population(set_mimes) > 0)
     {
        Eina_Hash *set_apps = eina_hash_pointer_new(NULL);
        Eina_Iterator *itr = eina_hash_iterator_key_new(set_mimes);
        const char *mime;

        EINA_ITERATOR_FOREACH(itr, mime)
          {
             Eina_List *desktops = efreet_util_desktop_mime_list(mime);
             Efreet_Desktop *d;
             Eina_Bool hd = EINA_FALSE;

             if (mime_list) *mime_list = eina_list_append(*mime_list, mime);

             desktops = _e_fwin_suggested_apps_list_sort(mime, desktops, &hd);
             if ((hd) && (has_default))
               *has_default = EINA_TRUE;

             EINA_LIST_FREE(desktops, d)
               {
                  if (eina_hash_find(set_apps, &d)) efreet_desktop_free(d);
                  else
                    {
                       eina_hash_add(set_apps, &d, (void *)1);
                       apps = eina_list_append(apps, d);
                    }
               }
          }
        eina_iterator_free(itr);
        eina_hash_free(set_apps);
     }
   eina_hash_free(set_mimes);

   return apps;
}

static void
_e_fwin_desktop_run(Efreet_Desktop *desktop,
                    E_Fwin_Page *page,
                    Eina_Bool skip_history)
{
   char pcwd[4096], buf[4096];
   Eina_List *selected, *l, *files = NULL;
   E_Fwin *fwin = page->fwin;
   E_Fm2_Icon_Info *ici;
   char *file;
   
   skip_history = EINA_FALSE;

   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;

   if (!getcwd(pcwd, sizeof(pcwd))) return;
   if (0 > chdir(e_fm2_real_path_get(page->fm_obj))) return;

   EINA_LIST_FOREACH(selected, l, ici)
     {
        E_Fwin_Exec_Type ext;

        /* this snprintf is silly - but it's here in case i really do
         * need to provide full paths (seems silly since we chdir
         * into the dir)
         */
        buf[0] = 0;
        ext = _e_fwin_file_is_exec(ici);
        if ((ext == E_FWIN_EXEC_NONE) || (desktop))
          {
             if (!((ici->link) && (ici->mount)))
               eina_strlcpy(buf, ici->file, sizeof(buf));
          }
        else
          _e_fwin_file_exec(page, ici, ext);
        if (buf[0] != 0)
          {
             if ((ici->mime) && (desktop) && !(skip_history))
               e_exehist_mime_desktop_add(ici->mime, desktop);
             files = eina_list_append(files, strdup(ici->file));
          }
     }
   if ((fwin->win) && (desktop))
     {
        e_exec(fwin->win->border->zone, desktop, NULL, files, "fwin");
        ici = selected->data;
        if ((ici) && (ici->mime) && (desktop) && !(skip_history))
          e_exehist_mime_desktop_add(ici->mime, desktop);
     }
   else if (fwin->zone && desktop)
     e_exec(fwin->zone, desktop, NULL, files, "fwin");

   eina_list_free(selected);

   EINA_LIST_FREE(files, file) free(file);

   chdir(pcwd);
}

static E_Fwin_Exec_Type
_e_fwin_file_is_exec(E_Fm2_Icon_Info *ici)
{
   /* special file or dir - can't exec anyway */
   if ((S_ISCHR(ici->statinfo.st_mode)) ||
       (S_ISBLK(ici->statinfo.st_mode)) ||
       (S_ISFIFO(ici->statinfo.st_mode)) ||
       (S_ISSOCK(ici->statinfo.st_mode)))
     return E_FWIN_EXEC_NONE;
   /* it is executable */
   if ((ici->statinfo.st_mode & S_IXOTH) ||
       ((getgid() == ici->statinfo.st_gid) &&
        (ici->statinfo.st_mode & S_IXGRP)) ||
       ((getuid() == ici->statinfo.st_uid) &&
        (ici->statinfo.st_mode & S_IXUSR)))
     {
        /* no mimetype */
        if (!ici->mime)
          return E_FWIN_EXEC_DIRECT;
        /* mimetype */
        else
          {
             /* FIXME: - this could be config */
             if (!strcmp(ici->mime, "application/x-desktop"))
               return E_FWIN_EXEC_DESKTOP;
             else if ((!strcmp(ici->mime, "application/x-sh")) ||
                      (!strcmp(ici->mime, "application/x-shellscript")) ||
                      (!strcmp(ici->mime, "application/x-csh")) ||
                      (!strcmp(ici->mime, "application/x-perl")) ||
                      (!strcmp(ici->mime, "application/x-shar")) ||
                      (!strcmp(ici->mime, "application/x-executable")) ||
                      (!strcmp(ici->mime, "text/x-csh")) ||
                      (!strcmp(ici->mime, "text/x-python")) ||
                      (!strcmp(ici->mime, "text/x-sh"))
                      )
               {
                  return E_FWIN_EXEC_DIRECT;
               }
          }
     }
   else
     {
        /* mimetype */
        if (ici->mime)
          {
             /* FIXME: - this could be config */
             if (!strcmp(ici->mime, "application/x-desktop"))
               return E_FWIN_EXEC_DESKTOP;
             else if ((!strcmp(ici->mime, "application/x-sh")) ||
                      (!strcmp(ici->mime, "application/x-shellscript")) ||
                      (!strcmp(ici->mime, "text/x-sh"))
                      )
               {
                  return E_FWIN_EXEC_TERMINAL_SH;
               }
          }
        else if ((e_util_glob_match(ici->file, "*.desktop")) ||
                 (e_util_glob_match(ici->file, "*.kdelink"))
                 )
          {
             return E_FWIN_EXEC_DESKTOP;
          }
        else if (e_util_glob_match(ici->file, "*.run"))
          return E_FWIN_EXEC_TERMINAL_SH;
     }
   return E_FWIN_EXEC_NONE;
}

static void
_e_fwin_file_exec(E_Fwin_Page *page,
                  E_Fm2_Icon_Info *ici,
                  E_Fwin_Exec_Type ext)
{
   E_Fwin *fwin = page->fwin;
   char buf[4096];
   Efreet_Desktop *desktop;

   /* FIXME: execute file ici with either a terminal, the shell, or directly
    * or open the .desktop and exec it */
   switch (ext)
     {
      case E_FWIN_EXEC_NONE:
        break;

      case E_FWIN_EXEC_DIRECT:
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->cur_page->fm_obj), ici->file);
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, "fwin");
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, "fwin");
        break;

      case E_FWIN_EXEC_SH:
        snprintf(buf, sizeof(buf), "/bin/sh %s", e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, "fwin");
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, "fwin");
        break;

      case E_FWIN_EXEC_TERMINAL_DIRECT:
        snprintf(buf, sizeof(buf), "%s %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, "fwin");
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, "fwin");
        break;

      case E_FWIN_EXEC_TERMINAL_SH:
        snprintf(buf, sizeof(buf), "%s /bin/sh %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, "fwin");
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, "fwin");
        break;

      case E_FWIN_EXEC_DESKTOP:
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
        desktop = efreet_desktop_new(buf);
        if (desktop)
          {
             if (fwin->win)
               e_exec(fwin->win->border->zone, desktop, NULL, NULL, "fwin");
             else if (fwin->zone)
               e_exec(fwin->zone, desktop, NULL, NULL, "fwin");
             e_exehist_mime_desktop_add(ici->mime, desktop);
             efreet_desktop_free(desktop);
          }
        break;

      default:
        break;
     }
}

static void
_e_fwin_config_set(E_Fwin_Page *page)
{
   E_Fm2_Config fmc;

   memset(&fmc, 0, sizeof(E_Fm2_Config));
   if (!page->fwin->zone)
     {
#if 0
        fmc.view.mode = E_FM2_VIEW_MODE_LIST;
        fmc.icon.list.w = 24 * e_scale;
        fmc.icon.list.h = 24 * e_scale;
        fmc.icon.fixed.w = 1;
        fmc.icon.fixed.h = 1;
#else
        fmc.view.mode = fileman_config->view.mode;
        fmc.icon.icon.w = fileman_config->icon.icon.w * e_scale;
        fmc.icon.icon.h = fileman_config->icon.icon.h * e_scale;
        fmc.icon.fixed.w = 0;
        fmc.icon.fixed.h = 0;
#endif
        fmc.view.open_dirs_in_place = fileman_config->view.open_dirs_in_place;
     }
   else
     {
#if 0
        fmc.view.mode = E_FM2_VIEW_MODE_LIST;
        fmc.icon.list.w = 24 * e_scale;
        fmc.icon.list.h = 24 * e_scale;
        fmc.icon.fixed.w = 1;
        fmc.icon.fixed.h = 1;
#else
        fmc.view.mode = page->fwin->path->desktop_mode;
        fmc.icon.icon.w = fileman_config->icon.icon.w * e_scale;
        fmc.icon.icon.h = fileman_config->icon.icon.h * e_scale;
        fmc.icon.fixed.w = 0;
        fmc.icon.fixed.h = 0;
#endif
        fmc.view.no_typebuf_set = !fileman_config->view.desktop_navigation;
        fmc.view.open_dirs_in_place = 0;
        fmc.view.fit_custom_pos = 1;
     }

   fmc.view.selector = 0;
   fmc.view.single_click = fileman_config->view.single_click;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.max_thumb_size = fileman_config->icon.max_thumb_size;
   fmc.icon.extension.show = fileman_config->icon.extension.show;
   fmc.list.sort.no_case = fileman_config->list.sort.no_case;
   fmc.list.sort.extension = fileman_config->list.sort.extension;
   fmc.list.sort.mtime = fileman_config->list.sort.mtime;
   fmc.list.sort.size = fileman_config->list.sort.size;
   fmc.list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   fmc.list.sort.dirs.last = fileman_config->list.sort.dirs.last;
   fmc.selection.single = fileman_config->selection.single;
   fmc.selection.windows_modifiers = fileman_config->selection.windows_modifiers;
   e_fm2_config_set(page->fm_obj, &fmc);
}

static void
_e_fwin_window_title_set(E_Fwin_Page *page)
{
   char buf[PATH_MAX + sizeof("e_fwin::")];
   const char *file;

   if (!page) return;
   if (page->fwin->zone) return;  //safety

   if (fileman_config->view.show_full_path)
     file = e_fm2_real_path_get(page->fm_obj);
   else
     file = ecore_file_file_get(e_fm2_real_path_get(page->fm_obj));

   if (file)
     {
        eina_strlcpy(buf, file, sizeof(buf));
        e_win_title_set(page->fwin->win, buf);
     }

   snprintf(buf, sizeof(buf), "e_fwin::%s", e_fm2_real_path_get(page->fm_obj));
   e_win_name_class_set(page->fwin->win, "E", buf);
}

static void
_e_fwin_toolbar_resize(E_Fwin_Page *page)
{
   if (!page->tbar)
     {
        edje_object_signal_emit(page->fwin->bg_obj, "e,toolbar,disabled", "e");
        return;
     }
   switch (page->tbar->gadcon->orient)
     {
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
        evas_object_size_hint_min_set(page->tbar->o_base, 0, page->tbar->minh);
        edje_object_part_swallow(page->fwin->bg_obj, "e.swallow.toolbar", page->tbar->o_base);
        edje_object_signal_emit(page->fwin->bg_obj, "e,toolbar,top", "e");
        break;
      case E_GADCON_ORIENT_BOTTOM:
        evas_object_size_hint_min_set(page->tbar->o_base, 0, page->tbar->minh);
        edje_object_part_swallow(page->fwin->bg_obj, "e.swallow.toolbar", page->tbar->o_base);
        edje_object_signal_emit(page->fwin->bg_obj, "e,toolbar,bottom", "e");
        break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
        evas_object_size_hint_min_set(page->tbar->o_base, page->tbar->minw, 0);
        edje_object_part_swallow(page->fwin->bg_obj, "e.swallow.toolbar", page->tbar->o_base);
        edje_object_signal_emit(page->fwin->bg_obj, "e,toolbar,left", "e");
        break;
      case E_GADCON_ORIENT_RIGHT:
        evas_object_size_hint_min_set(page->tbar->o_base, page->tbar->minw, 0);
        edje_object_part_swallow(page->fwin->bg_obj, "e.swallow.toolbar", page->tbar->o_base);
        edje_object_signal_emit(page->fwin->bg_obj, "e,toolbar,right", "e");
        break;
      default:
        break;
     }
   edje_object_message_signal_process(page->fwin->bg_obj);
}

/* fwin callbacks */
static void
_e_fwin_cb_delete(E_Win *win)
{
   E_Fwin *fwin;

   if (!win) return;  //safety
   fwin = win->data;
   e_object_del(E_OBJECT(fwin));
}

/* static void
 * _e_fwin_geom_save(E_Fwin *fwin)
 * {
 *    char buf[PATH_MAX];
 *    E_Fm2_Custom_File *cf;
 *
 *    if (!fwin->geom_save_ready) return;
 *    snprintf(buf, sizeof(buf), "dir::%s", e_fm2_real_path_get(fwin->cur_page->fm_obj));
 *    cf = e_fm2_custom_file_get(buf);
 *    if (!cf)
 *      {
 *  cf = alloca(sizeof(E_Fm2_Custom_File));
 *  memset(cf, 0, sizeof(E_Fm2_Custom_File));
 *      }
 *    cf->geom.x = fwin->win->x;
 *    cf->geom.y = fwin->win->y;
 *    cf->geom.w = fwin->win->w;
 *    cf->geom.h = fwin->win->h;
 *    cf->geom.valid = 1;
 *    e_fm2_custom_file_set(buf, cf);
 * } */

static void
_e_fwin_cb_move(E_Win *win)
{
//   E_Fwin *fwin;

   if (!win) return;  //safety
//   fwin = win->data;
/* _e_fwin_geom_save(fwin); */
}

static void
_e_fwin_cb_resize(E_Win *win)
{
   E_Fwin *fwin;

   if (!win) return;  //safety
   fwin = win->data;
   if (fwin->bg_obj)
     {
        if (fwin->win)
          evas_object_resize(fwin->bg_obj, fwin->win->w, fwin->win->h);
        else if (fwin->zone)
          evas_object_resize(fwin->bg_obj, fwin->zone->w, fwin->zone->h);
     }
   _e_fwin_toolbar_resize(fwin->cur_page);
   if (fwin->zone)
     evas_object_resize(fwin->cur_page->scrollframe_obj, fwin->zone->w, fwin->zone->h);
   /* _e_fwin_geom_save(fwin); */
}

static void
_e_fwin_deleted(void *data,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   E_Fwin_Page *page;

   page = data;
   e_object_del(E_OBJECT(page->fwin));
}

static void
_e_fwin_changed(void *data,
                Evas_Object *obj,
                void *event_info __UNUSED__)
{
   E_Fwin *fwin;
   E_Fwin_Page *page;
   E_Fm2_Config *cfg;
   Efreet_Desktop *ef;
   const char *dev, *path;
   char buf[PATH_MAX];
   Eina_Bool need_free = EINA_TRUE;

   page = data;
   fwin = page->fwin;
   if (!fwin) return;  //safety

   _e_fwin_icon_mouse_out(fwin, NULL, NULL);
   cfg = e_fm2_config_get(page->fm_obj);
   e_fm2_path_get(page->fm_obj, &dev, NULL);
   e_user_dir_concat_static(buf, "fileman/favorites");
   path = e_fm2_real_path_get(page->fm_obj);
   if ((dev && (!strcmp(dev, "favorites"))) || (path && (!strcmp(buf, path))))
     cfg->view.link_drop = 1;
   else
     cfg->view.link_drop = 0;

   /* FIXME: first look in E config for a special override for this dir's bg
    * or overlay
    */
   ef = e_fm2_desktop_get(page->fm_obj);
   if (ef) need_free = EINA_FALSE;
   else
     {
        snprintf(buf, sizeof(buf), "%s/.directory.desktop", e_fm2_real_path_get(page->fm_obj));
        ef = efreet_desktop_new(buf);
     }
   //printf("EF=%p for %s\n", ef, buf);
   if (ef)
     {
        fwin->wallpaper_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->wallpaper_file, "X-Enlightenment-Directory-Wallpaper");
        fwin->overlay_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->overlay_file, "X-Enlightenment-Directory-Overlay");
        fwin->scrollframe_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->scrollframe_file, "X-Enlightenment-Directory-Scrollframe");
        fwin->theme_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->theme_file, "X-Enlightenment-Directory-Theme");
        //printf("fwin->wallpaper_file = %s\n", fwin->wallpaper_file);
        if (need_free) efreet_desktop_free(ef);
     }
   else
     {
#define RELEASE_STR(x) if (x) {eina_stringshare_del(x); (x) = NULL; }
        RELEASE_STR(fwin->wallpaper_file);
        RELEASE_STR(fwin->overlay_file);
        RELEASE_STR(fwin->scrollframe_file);
        RELEASE_STR(fwin->theme_file);
#undef RELEASE_STR
     }
   if (fwin->under_obj) evas_object_hide(fwin->under_obj);
   if (fwin->wallpaper_file)
     {
        if (eina_str_has_extension(fwin->wallpaper_file, "edj"))
          {
             if (!fwin->wallpaper_is_edj)
               {
                  if (fwin->under_obj) evas_object_del(fwin->under_obj);
                  fwin->under_obj = edje_object_add(fwin->win->evas);
                  fwin->wallpaper_is_edj = EINA_TRUE;
               }
             edje_object_file_set(fwin->under_obj, fwin->wallpaper_file, "e/desktop/background");
          }
        else
          {
             if (fwin->wallpaper_is_edj) evas_object_del(fwin->under_obj);
             fwin->wallpaper_is_edj = EINA_FALSE;
             fwin->under_obj = e_icon_add(e_win_evas_get(fwin->win));
             e_icon_scale_size_set(fwin->under_obj, 0);
             e_icon_fill_inside_set(fwin->under_obj, 0);
             e_icon_file_set(fwin->under_obj, fwin->wallpaper_file);
          }
        if (fwin->under_obj)
          {
             edje_object_part_swallow(e_scrollframe_edje_object_get(page->scr), 
                                      "e.swallow.bg", fwin->under_obj);
             evas_object_pass_events_set(fwin->under_obj, 1);
             evas_object_show(fwin->under_obj);
          }
     }
   else
     edje_object_part_swallow(e_scrollframe_edje_object_get(page->scr), 
                                      "e.swallow.bg", NULL);
   if (fwin->over_obj)
     {
        //printf("over obj\n");
        evas_object_hide(fwin->over_obj);
        if (fwin->overlay_file)
          {
             edje_object_file_set(fwin->over_obj, fwin->overlay_file, 
                                  "e/desktop/background");
//             ext = strrchr(fwin->overlay_file, '.');
//             if (ext && !strcasecmp(ext, ".edj"))
//               e_icon_file_edje_set(fwin->over_obj, fwin->overlay_file, "e/desktop/background");
//             else
//               e_icon_file_set(fwin->over_obj, fwin->overlay_file);
          }
//        else
//          e_icon_file_edje_set(fwin->over_obj, NULL, NULL);
        evas_object_show(fwin->over_obj);
     }
   if (page->scrollframe_obj)
     {
        if ((fwin->scrollframe_file) &&
            (e_util_edje_collection_exists(fwin->scrollframe_file, 
                                           "e/fileman/default/scrollframe")))
          e_scrollframe_custom_edje_file_set(page->scr,
                                             (char *)fwin->scrollframe_file,
                                             "e/fileman/default/scrollframe");
        else
          {
             if (fwin->zone)
               e_scrollframe_custom_theme_set(page->scr,
                                              "base/theme/fileman",
                                              "e/fileman/desktop/scrollframe");
             else
               e_scrollframe_custom_theme_set(page->scr,
                                              "base/theme/fileman",
                                              "e/fileman/default/scrollframe");
          }
        e_scrollframe_child_pos_set(page->scr, 0, 0);
     }
   if ((fwin->theme_file) && (ecore_file_exists(fwin->theme_file)))
     e_fm2_custom_theme_set(obj, fwin->theme_file);
   else
     e_fm2_custom_theme_set(obj, NULL);

   _e_fwin_icon_mouse_out(fwin, NULL, NULL);
   if (fwin->zone)
     {
        e_fm2_path_get(page->fm_obj, &dev, &path);
        eina_stringshare_replace(&fwin->path->dev, dev);
        eina_stringshare_replace(&fwin->path->path, path);
        return;
     }
   _e_fwin_window_title_set(page);
   if (page->setting) return;
   if (page->flist) e_fm2_deselect_all(page->flist);
}

static void
_e_fwin_favorite_selected(void *data,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   E_Fwin_Page *page;
   Eina_List *selected;

   page = data;
   selected = e_fm2_selected_list_get(page->flist);
   if (!selected) return;
   page->setting = 1;
   _e_fwin_file_open_dialog(page, selected, 0);
   eina_list_free(selected);
   page->setting = 0;
}

static void
_e_fwin_selected(void *data,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   E_Fwin_Page *page;
   Eina_List *selected;

   page = data;
   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(page, selected, 0);
   eina_list_free(selected);
}

static void
_e_fwin_selection_change(void *data,
                         Evas_Object *obj,
                         void *event_info __UNUSED__)
{
   Eina_List *l;
   E_Fwin_Page *page;

   page = data;
   for (l = fwins; l; l = l->next)
     {
        if (l->data != page->fwin)
          e_fwin_all_unsel(l->data);
     }
   evas_object_focus_set(obj, 1);
   _e_fwin_icon_mouse_out(page->fwin, NULL, NULL);
}

static void
_e_fwin_cb_all_change(void *data,
                      Evas_Object *obj __UNUSED__)
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;

   fad = data;
   desktop = efreet_util_desktop_file_id_find(fad->app2);
   if ((desktop) && (desktop->exec))
     e_widget_entry_text_set(fad->o_entry, desktop->exec);
   if (desktop)
     efreet_desktop_free(desktop);
   else
     e_widget_entry_text_set(fad->o_entry, fad->app2);
}

static void
_e_fwin_cb_key_down(void *data,
                    Evas *e          __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Fwin *fwin;
   E_Fwin_Page *page;

   page = data;
   fwin = page->fwin;
   ev = event_info;

   if (evas_key_modifier_is_set(ev->modifiers, "Control"))
     {
        if (!strcmp(ev->key, "n"))
          {
             E_Container *con;
             const char *dev, *path;

             con = e_container_current_get(e_manager_current_get());
             e_fm2_path_get(page->fm_obj, &dev, &path);
             e_fwin_new(con, dev, path);
             return;
          }
        if (!strcmp(ev->key, "w"))
          {
             e_object_del(E_OBJECT(fwin));
             return;
          }
        if (!strcmp(ev->key, "a"))
          {
             e_fm2_all_sel(page->fm_obj);
             return;
          }
     }
}

static void
_e_fwin_cb_page_obj_del(void *data,
                        Evas *evas       __UNUSED__,
                        Evas_Object *obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   E_Fwin_Page *page;

   page = data;

   evas_object_smart_callback_del(page->fm_obj, "dir_changed",
                                  _e_fwin_changed);
   evas_object_smart_callback_del(page->fm_obj, "dir_deleted",
                                  _e_fwin_deleted);
   evas_object_smart_callback_del(page->fm_obj, "selected",
                                  _e_fwin_selected);
   evas_object_smart_callback_del(page->fm_obj, "selection_change",
                                  _e_fwin_selection_change);
   evas_object_event_callback_del(page->fm_obj, EVAS_CALLBACK_DEL,
                                  _e_fwin_cb_page_obj_del);
}

/* fwin zone callbacks */
static void
_e_fwin_zone_cb_mouse_down(void *data,
                           Evas *evas       __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   E_Fwin *fwin;

   fwin = data;
   if (!fwin) return;
   e_fwin_all_unsel(fwin);
   e_fm2_typebuf_clear(fwin->cur_page->fm_obj);
}

static void
_e_fwin_zone_focus_out(void *data __UNUSED__,
                       Evas *evas       __UNUSED__,
                       Evas_Object *obj,
                       void *event_info __UNUSED__)
{
   evas_object_focus_set(obj, EINA_TRUE);
}

static void
_e_fwin_zone_focus_in(void *data,
                       Evas *evas       __UNUSED__,
                       void *event_info __UNUSED__)
{
   E_Fwin *fwin;

   fwin = data;
   if ((!fwin) || (!fwin->cur_page) || (!fwin->cur_page->fm_obj)) return;
   evas_object_focus_set(fwin->cur_page->fm_obj, EINA_TRUE);
}

static Eina_Bool
_e_fwin_zone_move_resize(void *data, int type __UNUSED__, void *event)
{
   E_Event_Zone_Move_Resize *ev = event;
   E_Fwin *fwin = data;
   int x, y, w, h, sx, sy, sw, sh;

   if (!fwin) return ECORE_CALLBACK_PASS_ON;
   if (fwin->zone != ev->zone) return ECORE_CALLBACK_PASS_ON;
   if (!fwin->cur_page->scrollframe_obj) return ECORE_CALLBACK_RENEW;
   e_zone_useful_geometry_get(ev->zone, &x, &y, &w, &h);
   evas_object_geometry_get(fwin->cur_page->scrollframe_obj, &sx, &sy, &sw, &sh);
   /* if same, do nothing */
   if ((sx == x) && (sy == y) && (sw == w) && (sh == h)) return ECORE_CALLBACK_RENEW;
   evas_object_move(fwin->cur_page->scrollframe_obj, x, y);
   evas_object_resize(fwin->cur_page->scrollframe_obj, w, h);
   e_fm2_refresh(fwin->cur_page->fm_obj);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_fwin_zone_del(void *data,
                 int type,
                 void *event)
{
   E_Event_Zone_Del *ev;
   E_Fwin *fwin;

   if (type != E_EVENT_ZONE_DEL) return ECORE_CALLBACK_PASS_ON;
   fwin = data;
   ev = event;
   if (!fwin) return ECORE_CALLBACK_PASS_ON;
   if (fwin->zone != ev->zone) return ECORE_CALLBACK_PASS_ON;
   e_object_del(E_OBJECT(fwin));
   return ECORE_CALLBACK_PASS_ON;
}

static int
_e_fwin_cb_dir_handler_test(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path)
{
   return ecore_file_is_dir(path);
}

static void
_e_fwin_cb_dir_handler(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *path)
{
   char buf[PATH_MAX];

   if (!getcwd(buf, sizeof(buf))) return;

   chdir(path);
   e_exec(e_util_zone_current_get(e_manager_current_get()), tdesktop, NULL, NULL, "fileman");
   chdir(buf);
   /* FIXME: if info != null then check mime type and offer options based
    * on that
    */
}

static void
_e_fwin_parent(void *data,
               E_Menu *m       __UNUSED__,
               E_Menu_Item *mi __UNUSED__)
{
   e_fm2_parent_go(data);
}

static void
_e_fwin_cb_menu_open_fast(void *data,
                          E_Menu *m    __UNUSED__,
                          E_Menu_Item *mi)
{
   E_Fwin_Page *page;
   Efreet_Desktop *desk;

   page = data;
   desk = e_object_data_get(E_OBJECT(mi));

   if ((page) && (desk))
     _e_fwin_desktop_run(desk, page, EINA_TRUE);
}

static void
_e_fwin_cb_menu_extend_open_with(void *data,
                                 E_Menu *m)
{
   Eina_List *selected = NULL, *apps = NULL, *l;
   E_Menu_Item *mi;
   E_Fwin_Page *page;
   Efreet_Desktop *desk = NULL;

   page = data;

   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;

   apps = _e_fwin_suggested_apps_list_get(selected, NULL, NULL);
   EINA_LIST_FOREACH(apps, l, desk)
     {
        if (!desk) continue;
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, desk->name);
        e_util_desktop_menu_item_icon_add(desk, 24, mi);
        e_menu_item_callback_set(mi, _e_fwin_cb_menu_open_fast, page);
        e_object_data_set(E_OBJECT(mi), desk);
     }

   if (apps)
     {
        mi = e_menu_item_new(m);
        e_menu_item_separator_set(mi, 1);
     }

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Other application..."));
   e_util_menu_item_theme_icon_set(mi, "document-open");
   e_menu_item_callback_set(mi, _e_fwin_cb_menu_open_with, page);

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   eina_list_free(apps);
   eina_list_free(selected);
}

static void
_e_fwin_path(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   const char *path;
   E_Fwin_Page *page;
   Ecore_X_Window xwin;

   path = e_fm2_real_path_get(data);
   page = evas_object_data_get(data, "fm_page");
   if (page->fwin->win)
     xwin = page->fwin->win->border->client.win;
   else
     xwin = page->fwin->zone->container->event_win;
   ecore_x_selection_clipboard_set(xwin, path, strlen(path) + 1);
}

static void
_e_fwin_clone(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Fwin *fwin = data;
   E_Action *act_fm;

   /* path of least resistance! */
   act_fm = e_action_find("fileman");
   if (!act_fm) return;
   act_fm->func.go(NULL, fwin->win->border->client.icccm.class + 8);
}

static void
_e_fwin_cb_menu_extend_start(void *data,
                             Evas_Object *obj,
                             E_Menu *m,
                             E_Fm2_Icon_Info *info)
{
   E_Menu_Item *mi = NULL;
   E_Fwin_Page *page;
   E_Menu *subm;
   Eina_List *selected = NULL;
   Eina_Bool set = EINA_FALSE;
   char buf[PATH_MAX] = {0};

   page = data;

   selected = e_fm2_selected_list_get(page->fm_obj);

#ifdef ENABLE_FILES
   if (info && info->file && (info->link || S_ISDIR(info->statinfo.st_mode)))
     snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), info->file);
   subm = e_mod_menu_add(m, (buf[0]) ? buf : e_fm2_real_path_get(page->fm_obj));

   if (((!page->fwin->zone) || fileman_config->view.desktop_navigation) && e_fm2_has_parent_get(obj))
     {
        mi = e_menu_item_new_relative(subm, NULL);
        e_menu_item_label_set(mi, _("Go To Parent Directory"));
        e_menu_item_icon_edje_set(mi,
                                  e_theme_edje_file_get("base/theme/fileman",
                                                        "e/fileman/default/button/parent"),
                                  "e/fileman/default/button/parent");
        e_menu_item_callback_set(mi, _e_fwin_parent, obj);
     }
   if (!page->fwin->zone)
     {
        mi = e_menu_item_new_relative(subm, mi);
        e_menu_item_label_set(mi, _("Clone Window"));
        e_util_menu_item_theme_icon_set(mi, "window-duplicate");
        e_menu_item_callback_set(mi, _e_fwin_clone, page->fwin);
     }

   mi = e_menu_item_new_relative(subm, mi);
   e_menu_item_label_set(mi, _("Copy Path"));
   e_util_menu_item_theme_icon_set(mi, "edit-copy");
   e_menu_item_callback_set(mi, _e_fwin_path, obj);

   mi = e_menu_item_new_relative(subm, mi);
   e_menu_item_separator_set(mi, EINA_TRUE);
   
   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, EINA_TRUE);
#endif
   if (!selected) return;
   mi = e_menu_item_new(m);
   while (eina_list_count(selected) == 1)
     {
        if (_e_fwin_file_is_exec(eina_list_data_get(selected)) == E_FWIN_EXEC_NONE) break;
        
        e_menu_item_label_set(mi, _("Run"));
        e_util_menu_item_theme_icon_set(mi, "system-run");
        set = EINA_TRUE;
        break;
     }
   if (!set)
     {
        e_menu_item_label_set(mi, _("Open"));
        e_util_menu_item_theme_icon_set(mi, "document-open");
     }

   e_menu_item_callback_set(mi, _e_fwin_cb_menu_open, page);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open with..."));
   e_util_menu_item_theme_icon_set(mi, "document-open");

   subm = e_menu_new();
   e_menu_item_submenu_set(mi, subm);
   e_menu_pre_activate_callback_set(subm, _e_fwin_cb_menu_extend_open_with, page);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);
   eina_list_free(selected);
}

static void
_e_fwin_cb_menu_open(void *data,
                     E_Menu *m       __UNUSED__,
                     E_Menu_Item *mi __UNUSED__)
{
   E_Fwin_Page *page;
   Eina_List *selected;

   page = data;
   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(page, selected, 0);
   eina_list_free(selected);
}

static void
_e_fwin_cb_menu_open_with(void *data,
                          E_Menu *m       __UNUSED__,
                          E_Menu_Item *mi __UNUSED__)
{
   E_Fwin_Page *page;
   Eina_List *selected = NULL;

   page = data;
   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(page, selected, 1);
   eina_list_free(selected);
}

static void
_e_fwin_border_set(E_Fwin_Page *page, E_Fwin *fwin, E_Fm2_Icon_Info *ici)
{
   Evas_Object *oic;
   const char *itype = NULL;
   int ix, iy, iw, ih, nx, ny, found = 0;
   E_Remember *rem = NULL;
   Eina_List *ll;
   const char *file = NULL, *group = NULL, *class;
   int w, h, zw, zh;
   /* E_Fm2_Custom_File *cf; */

   if (ici->label)
     e_win_title_set(fwin->win, ici->label);
   else if (ici->file)
     e_win_title_set(fwin->win, ici->file);
   oic = e_fm2_icon_get(evas_object_evas_get(ici->fm),
                        ici->ic, NULL, NULL, 0, &itype);
   if (!oic) return;
   if (fwin->win->border->internal_icon)
     eina_stringshare_del(fwin->win->border->internal_icon);
   fwin->win->border->internal_icon = NULL;
   if (fwin->win->border->internal_icon_key)
     eina_stringshare_del(fwin->win->border->internal_icon_key);
   fwin->win->border->internal_icon_key = NULL;

   if (!strcmp(evas_object_type_get(oic), "edje"))
     {
        edje_object_file_get(oic, &file, &group);
        if (file)
          {
             fwin->win->border->internal_icon =
               eina_stringshare_add(file);
             if (group)
               fwin->win->border->internal_icon_key =
                 eina_stringshare_add(group);
          }
     }
   else
     {
        e_icon_file_get(oic, &file, &group);
        fwin->win->border->internal_icon = eina_stringshare_add(file);
        fwin->win->border->internal_icon_key = eina_stringshare_add(group);
     }
   evas_object_del(oic);
   if (fwin->win->border->placed) return;

   class = eina_stringshare_printf("e_fwin::%s", e_fm2_real_path_get(fwin->cur_page->fm_obj));
   e_zone_useful_geometry_get(fwin->win->border->zone,
                              NULL, NULL, &zw, &zh);
   EINA_LIST_FOREACH(e_config->remembers, ll, rem)
     if (rem->class == class)
       {
          found = 1;
          rem->prop.w = E_CLAMP(rem->prop.w, DEFAULT_WIDTH, zw);
          rem->prop.h = E_CLAMP(rem->prop.h, DEFAULT_HEIGHT, zh);
          rem->prop.pos_x = E_CLAMP(rem->prop.pos_x, 0, zw - rem->prop.w);
          rem->prop.pos_y = E_CLAMP(rem->prop.pos_y, 0, zh - rem->prop.h);
          break;
       }
   eina_stringshare_del(class);

   if (found) return;

   /* No custom info, so just put window near icon */
   e_fm2_icon_geometry_get(ici->ic, &ix, &iy, &iw, &ih);
   nx = (ix + (iw / 2));
   ny = (iy + (ih / 2));
   if (page->fwin->win)
     {
        nx += page->fwin->win->x;
        ny += page->fwin->win->y;
     }

   /* checking width and height */
   /* TODO add config for preffered
      initial size? */
   w = DEFAULT_WIDTH;
   if (w > zw)
     w = zw;

   h = DEFAULT_HEIGHT;
   if (h > zh)
     h = zh;

   /* iff going out of zone - adjust to be in */
   if ((fwin->win->border->zone->x + fwin->win->border->zone->w) < (w + nx))
     nx -= w;
   if ((fwin->win->border->zone->y + fwin->win->border->zone->h) < (h + ny))
     ny -= h;

   e_win_move_resize(fwin->win, nx, ny, w, h);
   fwin->win->border->placed = 1;
}

static E_Fwin *
_e_fwin_open(E_Fwin_Page *page, E_Fm2_Icon_Info *ici, Eina_Bool force, int *need_dia)
{
   E_Fwin *fwin = NULL;
   char buf[PATH_MAX + sizeof("removable:")];
   Eina_Bool new_fwin;

   new_fwin = (force || ((!fileman_config->view.open_dirs_in_place || page->fwin->zone)));

   if ((ici->link) && (ici->mount))
     {
        if (new_fwin)
          {
             if (page->fwin->win)
               fwin = _e_fwin_new(page->fwin->win->container, ici->link, "/");
             else if (page->fwin->zone)
               fwin = _e_fwin_new(page->fwin->zone->container, ici->link, "/");
          }
        else
          {
             _e_fwin_border_set(page, page->fwin, ici);
             e_fm2_path_set(page->fm_obj, ici->link, "/");
             return page->fwin;
          }
     }
   else if ((ici->link) && (ici->removable))
     {
        snprintf(buf, sizeof(buf), "removable:%s", ici->link);
        if (new_fwin)
          {
             if (page->fwin->win)
               fwin = _e_fwin_new(page->fwin->win->container, buf, "/");
             else if (page->fwin->zone)
               fwin = _e_fwin_new(page->fwin->zone->container, buf, "/");
          }
        else
          {
             _e_fwin_border_set(page, page->fwin, ici);
             e_fm2_path_set(page->fm_obj, buf, "/");
             return page->fwin;
          }
     }
   else if (ici->real_link)
     {
        if (S_ISDIR(ici->statinfo.st_mode))
          {
             if (new_fwin)
               {
                  if (page->fwin->win)
                    fwin = _e_fwin_new(page->fwin->win->container, NULL, ici->real_link);
                  else if (page->fwin->zone)
                    fwin = _e_fwin_new(page->fwin->zone->container, NULL, ici->real_link);
               }
             else
               {
                  _e_fwin_border_set(page, page->fwin, ici);
                  e_fm2_path_set(page->fm_obj, NULL, ici->real_link);
                  return page->fwin;
               }
          }
        else
          {
             if (need_dia) *need_dia = 1;
          }
     }
   else
     {
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
        if (S_ISDIR(ici->statinfo.st_mode))
          {
             if (new_fwin)
               {
                  if (page->fwin->win)
                    fwin = _e_fwin_new(page->fwin->win->container, NULL, ici->link ?: buf);
                  else if (page->fwin->zone)
                    fwin = _e_fwin_new(page->fwin->zone->container, NULL, ici->link ?: buf);
               }
             else
               {
                  _e_fwin_border_set(page, page->fwin, ici);
                  e_fm2_path_set(page->fm_obj, NULL, ici->link ?: buf);
                  return page->fwin;
               }
          }
        else
          {
             if (need_dia) *need_dia = 1;
          }
     }
   if (!fwin) return NULL;
   if ((!fwin->win) || (!fwin->win->border))
     {
        _e_fwin_free(fwin);
        return NULL;
     }
   _e_fwin_border_set(page, fwin, ici);
        
   return fwin;
}

/* 'open with' dialog*/
static void
_e_fwin_file_open_dialog(E_Fwin_Page *page,
                         Eina_List *files,
                         int always)
{
   E_Fwin *fwin = page->fwin;
   E_Dialog *dia;
   Evas_Coord mw, mh;
   Evas_Object *o, *of, *ol, *ot;
   Evas *evas;
   Eina_List *l = NULL, *ll, *apps = NULL, *mlist = NULL;
   Eina_List *cats = NULL;
   Efreet_Desktop *desk = NULL;
   E_Fwin_Apps_Dialog *fad;
   E_Fm2_Icon_Info *ici;
   char buf[PATH_MAX + sizeof("removable:")];
   Eina_Bool has_default = EINA_FALSE;
   int need_dia = 0, n = 0;;

   n = eina_list_count(files);

   if (fwin->fad)
     {
        e_object_del(E_OBJECT(fwin->fad->dia));
        fwin->fad = NULL;
     }
   if (!always)
     {
        if ((fileman_config->view.open_dirs_in_place) && (!page->fwin->zone))
          _e_fwin_open(page, eina_list_data_get(files), EINA_FALSE, &need_dia);
        else
          {
             EINA_LIST_FOREACH(files, l, ici)
               _e_fwin_open(page, ici, EINA_FALSE, &need_dia);
          }
        if (!need_dia) return;
     }

   apps = _e_fwin_suggested_apps_list_get(files, &mlist, &has_default);

//   fprintf(stderr, "GOGOGOGOOGOGOG\n");
   if (!always)
     {
        /* FIXME: well this is simplisitic - if only 1 mime type is being
         * opened then look for the previously used app for that mimetype and
         * if found, use that.
         *
         * we could get more sophisitcated.
         * 1. find apps for each mimetype in mlist. if all prev used apps are
         * the same, then use previously used app.
         * OR if this fails
         * 2. find all apps for each mimetype. find the one used the most.
         * if that app can handle all mimetypes in the list - use that. if not
         * find the next most often listed app - if that can handle all apps,
         * use it, if not fall back again - and so on - if all apps listed do
         * not contain 1 that handles all the mime types - fall back to dialog
         */
        if ((has_default) || (eina_list_count(mlist) <= 1))
          {
             char *file;
             char pcwd[4096];
             Eina_List *files_list = NULL;

             need_dia = 1;
             //fprintf(stderr, "XXXXX %i %p\n", has_default, apps);
             if ((has_default) && (apps)) desk = apps->data;
             else if (mlist) desk = e_exehist_mime_desktop_get(mlist->data);
             //fprintf(stderr, "mlist = %p\n", mlist);
             getcwd(pcwd, sizeof(pcwd));
             chdir(e_fm2_real_path_get(page->fm_obj));

             files_list = NULL;
             EINA_LIST_FOREACH(files, l, ici)
               if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE)
                 files_list = eina_list_append(files_list, strdup(ici->file));
             EINA_LIST_FOREACH(files, l, ici)
               {
                  E_Fwin_Exec_Type ext;

                  ext = _e_fwin_file_is_exec(ici);
                  if (ext != E_FWIN_EXEC_NONE)
                    {
                       _e_fwin_file_exec(page, ici, ext);
                       need_dia = 0;
                    }
               }
             if (desk)
               {
                  if (fwin->win)
                    {
                       if (e_exec(fwin->win->border->zone, desk, NULL, files_list, "fwin"))
                         need_dia = 0;
                    }
                  else if (fwin->zone)
                    {
                       if (e_exec(fwin->zone, desk, NULL, files_list, "fwin"))
                         need_dia = 0;
                    }
                  if (!need_dia)
                    {
                       if (mlist)
                         e_exehist_mime_desktop_add(mlist->data, desk);
                    }
               }
             EINA_LIST_FREE(files_list, file)
               free(file);

             chdir(pcwd);
             if (!need_dia)
               {
                  EINA_LIST_FREE(apps, desk) efreet_desktop_free(desk);
                  mlist = eina_list_free(mlist);
                  return;
               }
          }
     }

   if (fwin->win)
     dia = e_dialog_new(fwin->win->border->zone->container,
                        "E", "_fwin_open_apps");
   else if (fwin->zone)
     dia = e_dialog_new(fwin->zone->container,
                        "E", "_fwin_open_apps");
   else return;  /* make clang happy */
   
   e_dialog_resizable_set(dia, 1);
   
   fad = E_NEW(E_Fwin_Apps_Dialog, 1);
   e_dialog_title_set(dia, _("Open with..."));
   e_dialog_button_add(dia, _("Open"), "document-open",
                       _e_fwin_cb_open, fad);
   e_dialog_button_add(dia, _("Close"), "window-close",
                       _e_fwin_cb_close, fad);

   fad->dia = dia;
   fad->fwin = fwin;
   fwin->fad = fad;
   dia->data = fad;
   e_object_free_attach_func_set(E_OBJECT(dia), _e_fwin_cb_dialog_free);

   evas = e_win_evas_get(dia->win);

   ol = e_widget_list_add(evas, 0, 0);

   l = eina_list_free(l);

#if 1
   o = e_widget_filepreview_add(evas, 96 * e_scale, 96 * e_scale, 1);
   fad->o_filepreview = o;
   if (n == 1)
     {
        ici = eina_list_data_get(files);
        of = e_widget_framelist_add(evas, ici->file, 0);
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
        e_widget_filepreview_path_set(o, buf, ici->mime);
     }
   else
     {
        snprintf(buf, sizeof(buf), P_("%d file", "%d files", n), n);
        of = e_widget_framelist_add(evas, buf, 0);
        ot = e_widget_toolbar_add(evas, 24 * e_scale, 24 * e_scale);
        e_widget_toolbar_scrollable_set(ot, 1);
        EINA_LIST_FOREACH(files, l, ici)
          e_widget_toolbar_item_append(ot, NULL, ici->file, _e_fwin_file_open_dialog_preview_set, page, ici);
        e_widget_toolbar_item_select(ot, 0);
        e_widget_framelist_object_append(of, ot);
     }

   e_widget_framelist_object_append(of, o);
#else
   // Here each file has its own widget, created at start, resulting on a longish time to show the dialog.
   // However, this doesn't display artefacts as the previous method, so is kept here for reference.
   if (n == 1)
     {
        ici = eina_list_data_get(files);
        of = e_widget_framelist_add(evas, ici->file, 0);
        o = e_widget_filepreview_add(evas, 96 * e_scale, 96 * e_scale, 1);
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
        e_widget_filepreview_path_set(o, buf, ici->mime);
        e_widget_framelist_object_append(of, o);
     }
   else
     {
        snprintf(buf, sizeof(buf), P_("%d file", "%d files", n), n);
        of = e_widget_framelist_add(evas, buf, 0);
        ot = e_widget_toolbook_add(evas, 24 * e_scale, 24 * e_scale);
        EINA_LIST_FOREACH(files, l, ici)
          {
             o = e_widget_filepreview_add(evas, 96 * e_scale, 96 * e_scale, 1);
             snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
             e_widget_filepreview_path_set(o, buf, ici->mime);
             e_widget_toolbook_page_append(ot, NULL, ici->file, o, 1, 1, 1, 1, .5, .5);
          }
        e_widget_toolbook_page_show(ot, 0);
        e_widget_framelist_object_append(of, ot);
     }
#endif

   e_widget_list_object_append(ol, of, 1, 0, 0.5);

   // Make frame with list of applications
   of = e_widget_framelist_add(evas, _("Known Applications"), 0);
   o = e_widget_ilist_add(evas, 24, 24, &(fad->app2));
   e_widget_on_change_hook_set(o, _e_fwin_cb_all_change, fad);
   fad->o_all = o;
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);

   // Adding Specific Applications list into widget
   if (apps)
     e_widget_ilist_header_append(o, NULL, _("Suggested Applications"));
   mlist = eina_list_free(mlist);
   EINA_LIST_FOREACH_SAFE(apps, l, ll, desk)
     {
        Evas_Object *icon = NULL;
        const char *val;

        if (!desk) continue;
        icon = e_util_desktop_icon_add(desk, 24, evas);
        val = efreet_util_path_to_file_id(desk->orig_path);
        if (!val) val = eina_stringshare_add(desk->exec);
        if (val)
          e_widget_ilist_append(o, icon, desk->name, NULL, NULL, val);
        else
          {
             /* not executable, don't keep in list */
             apps = eina_list_remove_list(apps, l);
             efreet_desktop_free(desk);
          }
     }

   // Building All Applications list
   cats = efreet_util_desktop_name_glob_list("*");
   cats = eina_list_sort(cats, 0, _e_fwin_dlg_cb_desk_sort);
   EINA_LIST_FREE(cats, desk)
     {
        if (!eina_list_data_find(l, desk) && !eina_list_data_find(apps, desk))
          l = eina_list_append(l, desk);
        else
          efreet_desktop_free(desk);
     }
   l = eina_list_sort(l, -1, _e_fwin_dlg_cb_desk_list_sort);

   // Adding All Applications list into widget
   if (l)
     e_widget_ilist_header_append(o, NULL, _("All Applications"));
   EINA_LIST_FREE(l, desk)
     {
        Evas_Object *icon = NULL;

        if (!desk) continue;
        icon = e_util_desktop_icon_add(desk, 24, evas);
        e_widget_ilist_append(o, icon, desk->name, NULL, NULL,
                              efreet_util_path_to_file_id(desk->orig_path));
        efreet_desktop_free(desk);
     }

   EINA_LIST_FREE(apps, desk)
     efreet_desktop_free(desk);

   e_widget_ilist_go(o);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_size_min_set(o, 160, 160);
   e_widget_framelist_object_append(of, o);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   o = e_widget_label_add(evas, _("Custom Command"));
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   fad->o_entry = e_widget_entry_add(evas, &(fad->exec_cmd),
                                     _e_fwin_cb_exec_cmd_changed, fad, NULL);
   e_widget_list_object_append(ol, fad->o_entry, 1, 0, 0.5);

   e_widget_size_min_get(ol, &mw, &mh);
   e_dialog_content_set(dia, ol, mw, mh);
   evas_object_event_callback_add(ol, EVAS_CALLBACK_KEY_DOWN, _e_fwin_file_open_dialog_cb_key_down, page);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "preferences-applications");
   e_widget_focus_steal(fad->o_entry);
}

static void
_e_fwin_file_open_dialog_preview_set(void *data1, void *data2)
{
   E_Fwin_Page *page = data1;
   E_Fm2_Icon_Info *ici = data2;
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
   e_widget_filepreview_path_set(page->fwin->fad->o_filepreview, buf, ici->mime);
}

static void
_e_fwin_file_open_dialog_cb_key_down(void *data,
                                     Evas *e        __UNUSED__,
                                     Evas_Object *o __UNUSED__,
                                     void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   E_Fwin_Page *page = data;
   E_Fwin *fwin = page->fwin;

   if (!strcmp(ev->keyname, "Escape"))
     _e_fwin_cb_close(fwin->fad, fwin->fad->dia);
   else if (!strcmp(ev->keyname, "Return"))
     _e_fwin_cb_open(fwin->fad, fwin->fad->dia);
}

static int
_e_fwin_dlg_cb_desk_sort(const void *p1,
                         const void *p2)
{
   Efreet_Desktop *d1, *d2;

   d1 = (Efreet_Desktop *)p1;
   d2 = (Efreet_Desktop *)p2;

   if (!d1->name) return 1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static int
_e_fwin_dlg_cb_desk_list_sort(const void *data1,
                              const void *data2)
{
   const Efreet_Desktop *d1, *d2;

   if (!(d1 = data1)) return 1;
   if (!(d2 = data2)) return -1;
   return strcmp(d1->name, d2->name);
}

static void
_e_fwin_cb_exec_cmd_changed(void *data,
                            void *data2 __UNUSED__)
{
   E_Fwin_Apps_Dialog *fad = NULL;
   Efreet_Desktop *desktop = NULL;

   if (!(fad = data)) return;

   if (fad->app2)
     desktop = efreet_util_desktop_file_id_find(fad->app2);

   if (!desktop) return;
   if (strcmp(desktop->exec, fad->exec_cmd))
     {
        eina_stringshare_del(fad->app2);
        fad->app2 = NULL;
        if (fad->o_all) e_widget_ilist_unselect(fad->o_all);
     }
   efreet_desktop_free(desktop);
}

static void
_e_fwin_cb_open(void *data,
                E_Dialog *dia __UNUSED__)
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;

   fad = data;
   if (fad->app2)
     desktop = efreet_util_desktop_file_id_find(fad->app2);

   if ((!desktop) && (!fad->exec_cmd))
     {
        if (desktop) efreet_desktop_free(desktop);
        return;
     }

   // Create a fake .desktop for custom command.
   if (!desktop)
     {
        desktop = efreet_desktop_empty_new("");
        if (strchr(fad->exec_cmd, '%'))
          {
             desktop->exec = strdup(fad->exec_cmd);
          }
        else
          {
             desktop->exec = malloc(strlen(fad->exec_cmd) + 4);
             if (desktop->exec)
               snprintf(desktop->exec, strlen(fad->exec_cmd) + 4, "%s %%U", fad->exec_cmd);
          }
     }

   if ((desktop) || (strcmp(fad->exec_cmd, "")))
     _e_fwin_desktop_run(desktop, fad->fwin->cur_page, EINA_FALSE);

   efreet_desktop_free(desktop);

   e_object_del(E_OBJECT(fad->dia));
}

static void
_e_fwin_cb_close(void *data,
                 E_Dialog *dia __UNUSED__)
{
   E_Fwin_Apps_Dialog *fad;

   fad = data;
   e_object_del(E_OBJECT(fad->dia));
}

static void
_e_fwin_cb_dialog_free(void *obj)
{
   E_Dialog *dia;
   E_Fwin_Apps_Dialog *fad;

   dia = (E_Dialog *)obj;
   fad = dia->data;
   eina_stringshare_del(fad->app2);
   E_FREE(fad->exec_cmd);
   fad->fwin->fad = NULL;
   E_FREE(fad);
}

/* scrolling ability */
static void
_e_fwin_pan_set(Evas_Object *obj,
                Evas_Coord x,
                Evas_Coord y)
{
   E_Fwin_Page *page;

   page = evas_object_data_get(obj, "fm_page");
   e_fm2_pan_set(obj, x, y);
   if (x > page->fm_pan.max_x) x = page->fm_pan.max_x;
   if (y > page->fm_pan.max_y) y = page->fm_pan.max_y;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   page->fm_pan.x = x;
   page->fm_pan.y = y;
   _e_fwin_pan_scroll_update(page);
}

static void
_e_fwin_pan_get(Evas_Object *obj,
                Evas_Coord *x,
                Evas_Coord *y)
{
   E_Fwin_Page *page;

   page = evas_object_data_get(obj, "fm_page");
   e_fm2_pan_get(obj, x, y);
   page->fm_pan.x = *x;
   page->fm_pan.y = *y;
}

static void
_e_fwin_pan_max_get(Evas_Object *obj,
                    Evas_Coord *x,
                    Evas_Coord *y)
{
   E_Fwin_Page *page;

   page = evas_object_data_get(obj, "fm_page");
   e_fm2_pan_max_get(obj, x, y);
   page->fm_pan.max_x = *x;
   page->fm_pan.max_y = *y;
   _e_fwin_pan_scroll_update(page);
}

static void
_e_fwin_pan_child_size_get(Evas_Object *obj,
                           Evas_Coord *w,
                           Evas_Coord *h)
{
   E_Fwin_Page *page;

   page = evas_object_data_get(obj, "fm_page");
   e_fm2_pan_child_size_get(obj, w, h);
   page->fm_pan.w = *w;
   page->fm_pan.h = *h;
   _e_fwin_pan_scroll_update(page);
}

static void
_e_fwin_pan_scroll_update(E_Fwin_Page *page)
{
   Edje_Message_Int_Set *msg;

   if ((page->fm_pan.x == page->fm_pan_last.x) &&
       (page->fm_pan.y == page->fm_pan_last.y) &&
       (page->fm_pan.max_x == page->fm_pan_last.max_x) &&
       (page->fm_pan.max_y == page->fm_pan_last.max_y) &&
       (page->fm_pan.w == page->fm_pan_last.w) &&
       (page->fm_pan.h == page->fm_pan_last.h)) return;
   msg = alloca(sizeof(Edje_Message_Int_Set) -
                sizeof(int) + (6 * sizeof(int)));
   msg->count = 6;
   msg->val[0] = page->fm_pan.x;
   msg->val[1] = page->fm_pan.y;
   msg->val[2] = page->fm_pan.max_x;
   msg->val[3] = page->fm_pan.max_y;
   msg->val[4] = page->fm_pan.w;
   msg->val[5] = page->fm_pan.h;
//   printf("SEND MSG %i %i | %i %i | %ix%i\n",
//	  page->fm_pan.x, page->fm_pan.y,
//	  page->fm_pan.max_x, page->fm_pan.max_y,
//	  page->fm_pan.w, page->fm_pan.h);
   if (page->fwin->under_obj && page->fwin->wallpaper_is_edj)
     edje_object_message_send(page->fwin->under_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (page->fwin->over_obj)
     edje_object_message_send(page->fwin->over_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (page->scr)
     edje_object_message_send(e_scrollframe_edje_object_get(page->scr), EDJE_MESSAGE_INT_SET, 1, msg);
   page->fm_pan_last.x = page->fm_pan.x;
   page->fm_pan_last.y = page->fm_pan.y;
   page->fm_pan_last.max_x = page->fm_pan.max_x;
   page->fm_pan_last.max_y = page->fm_pan.max_y;
   page->fm_pan_last.w = page->fm_pan.w;
   page->fm_pan_last.h = page->fm_pan.h;
}

/* e_fm_op_registry */
static void
_e_fwin_op_registry_listener_cb(void *data,
                                const E_Fm2_Op_Registry_Entry *ere)
{
   Evas_Object *o = data;
   char buf[4096];
   char *total;
   int mw, mh;
   Edje_Message_Float msg;
   
   // Don't show if the operation keep less than 1 second
   if (ere->start_time + 1.0 > ecore_loop_time_get()) return;

   // Update icon
   switch (ere->op)
     {
      case E_FM_OP_COPY:
        edje_object_signal_emit(o, "e,action,icon,copy", "e");
        break;

      case E_FM_OP_MOVE:
        edje_object_signal_emit(o, "e,action,icon,move", "e");
        break;

      case E_FM_OP_REMOVE:
        edje_object_signal_emit(o, "e,action,icon,delete", "e");
        break;

      case E_FM_OP_SECURE_REMOVE:
        edje_object_signal_emit(o, "e,action,icon,secure_delete", "e");
        break;

      default:
        edje_object_signal_emit(o, "e,action,icon,unknown", "e");
     }

   // Update information text
   switch (ere->status)
     {
      case E_FM2_OP_STATUS_ABORTED:
        switch (ere->op)
          {
           case E_FM_OP_COPY:
             snprintf(buf, sizeof(buf), _("Copying is aborted"));
             break;

           case E_FM_OP_MOVE:
             snprintf(buf, sizeof(buf), _("Moving is aborted"));
             break;

           case E_FM_OP_REMOVE:
             snprintf(buf, sizeof(buf), _("Deleting is aborted"));
             break;

           case E_FM_OP_SECURE_REMOVE:
             snprintf(buf, sizeof(buf), _("Secure deletion is aborted"));
             break;

           default:
             snprintf(buf, sizeof(buf), _("Unknown operation from slave is aborted"));
          }
        break;

      default:
        total = e_util_size_string_get(ere->total);
        switch (ere->op)
          {
           case E_FM_OP_COPY:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Copy of %s done"), total);
             else
               snprintf(buf, sizeof(buf), _("Copying %s (eta: %s)"), total, e_util_time_str_get(ere->eta));
             break;

           case E_FM_OP_MOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Move of %s done"), total);
             else
               snprintf(buf, sizeof(buf), _("Moving %s (eta: %s)"), total, e_util_time_str_get(ere->eta));
             break;

           case E_FM_OP_REMOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Delete done"));
             else
               snprintf(buf, sizeof(buf), _("Deleting files..."));
             break;

           case E_FM_OP_SECURE_REMOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Secure delete done"));
             else
               snprintf(buf, sizeof(buf), _("Securely deleting files..."));
             break;

           default:
             snprintf(buf, sizeof(buf), _("Unknown operation from slave %d"), ere->id);
          }
        E_FREE(total);
     }
   edje_object_part_text_set(o, "e.text.info", buf);

   if (ere->needs_attention)
     edje_object_signal_emit(o, "e,action,set,need_attention", "e");
   else
     edje_object_signal_emit(o, "e,action,set,normal", "e");

   if ((ere->finished) || (ere->status == E_FM2_OP_STATUS_ABORTED))
     {
        if (!evas_object_data_get(o, "stopped"))
          {
             evas_object_data_set(o, "stopped", o);
             edje_object_signal_emit(o, "e,state,busy,stop", "e");
          }
     }
   if (ere->percent > 0)
     {
        if (!evas_object_data_get(o, "started"))
          {
             evas_object_data_set(o, "started", o);
             edje_object_signal_emit(o, "e,state,busy,start", "e");
          }
     }
   
   // Update element
   edje_object_part_drag_size_set(o, "e.gauge.bar",
                                  ((double)(ere->percent)) / 100.0, 1.0);
   msg.val = ((double)(ere->percent)) / 100.0;
   edje_object_message_send(o, EDJE_MESSAGE_FLOAT, 1, &msg);
   edje_object_size_min_get(o, &mw, &mh);
   if ((mw == 0) || (mh == 0))
     edje_object_size_min_calc(o, &mw, &mh);
   else
     {
        mw *= e_scale;
        mh *= e_scale;
     }
   evas_object_resize(o, mw, mh);
   evas_object_show(o);
}

static void
_e_fwin_op_registry_free_check(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Ecore_Timer *t;

   t = evas_object_data_get(obj, "_e_fwin_op_registry_thingy");
   if (!t) return;
   ecore_timer_del(t);
}

static Eina_Bool
_e_fwin_op_registry_free_data_delayed(void *data)
{
   evas_object_event_callback_del_full(data, EVAS_CALLBACK_FREE, _e_fwin_op_registry_free_check, data);
   evas_object_del((Evas_Object *)data);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fwin_op_registry_free_data(void *data)
{
   Ecore_Timer *t;
   t = ecore_timer_add(5.0, _e_fwin_op_registry_free_data_delayed, data);
   evas_object_data_set(data, "_e_fwin_op_registry_thingy", t);
   evas_object_event_callback_add(data, EVAS_CALLBACK_FREE, _e_fwin_op_registry_free_check, data);
}

static Eina_Bool
_e_fwin_op_registry_entry_add_cb(void *data,
                                 __UNUSED__ int type,
                                 void *event)
{
   E_Fm2_Op_Registry_Entry *ere = (E_Fm2_Op_Registry_Entry *)event;
   E_Fwin_Page *page = data;
   Evas_Object *o;

   if ((ere->op != E_FM_OP_COPY) && (ere->op != E_FM_OP_MOVE) &&
       (ere->op != E_FM_OP_REMOVE) && (ere->op != E_FM_OP_SECURE_REMOVE))
     return ECORE_CALLBACK_RENEW;

   o = edje_object_add(evas_object_evas_get(page->scrollframe_obj));
   e_theme_edje_object_set(o, "base/theme/fileman",
                           "e/fileman/default/progress");

   // Append the element to the box
   edje_object_part_box_append(e_scrollframe_edje_object_get(page->scr),
                               "e.box.operations", o);
   evas_object_size_hint_align_set(o, 1.0, 1.0); //FIXME this should be theme-configurable

   // add abort button callback with id of operation in registry
   edje_object_signal_callback_add(o, "e,fm,operation,abort", "",
                                   _e_fwin_op_registry_abort_cb, (void *)(long)ere->id);

   //Listen to progress changes
   e_fm2_op_registry_entry_listener_add(ere, _e_fwin_op_registry_listener_cb,
                                        o, _e_fwin_op_registry_free_data);

   return ECORE_CALLBACK_RENEW;
}

static void
_e_fwin_op_registry_entry_iter(E_Fwin_Page *page)
{
   Eina_Iterator *itr;
   E_Fm2_Op_Registry_Entry *ere;

   itr = e_fm2_op_registry_iterator_new();
   EINA_ITERATOR_FOREACH(itr, ere)
     _e_fwin_op_registry_entry_add_cb(page, 0, ere);
   eina_iterator_free(itr);
}

static void
_e_fwin_op_registry_abort_cb(void *data,
                             Evas_Object *obj     __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source   __UNUSED__)
{
   int id;

   id = (long)data;
   if (!id) return;
   e_fm2_operation_abort(id);
}


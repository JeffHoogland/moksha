#include "e.h"
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

#define DEFAULT_WIDTH 600
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
   Evas_Object         *tb_obj;
   Evas_Object         *bg_obj;
   E_Fwin_Apps_Dialog  *fad;

   Eina_List           *pages;
   E_Fwin_Page         *cur_page;
   int                  page_index;

   Evas_Object         *under_obj;
   Evas_Object         *over_obj;

   const char          *wallpaper_file;
   const char          *overlay_file;
   const char          *scrollframe_file;
   const char          *theme_file;

   Ecore_Event_Handler *zone_handler;
   Ecore_Event_Handler *zone_del_handler;
};

struct _E_Fwin_Page
{
   E_Fwin              *fwin;
   Ecore_Event_Handler *fm_op_entry_add_handler;

   Evas_Object         *scrollframe_obj;
   Evas_Object         *fm_obj;
   E_Toolbar           *tbar;

   struct
   {
      Evas_Coord x, y, max_x, max_y, w, h;
   } fm_pan, fm_pan_last;

   int index;
};

struct _E_Fwin_Apps_Dialog
{
   E_Dialog    *dia;
   E_Fwin      *fwin;
   const char  *app2;
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
static E_Fwin *_e_fwin_new(E_Container *con,
                           const char  *dev,
                           const char  *path);
static void         _e_fwin_free(E_Fwin *fwin);
static E_Fwin_Page *_e_fwin_page_create(E_Fwin *fwin);
static void         _e_fwin_page_free(E_Fwin_Page *page);
static void         _e_fwin_page_new(E_Fwin *fwin);
static void         _e_fwin_cb_page_change(void *data1,
                                           void *data2);
static void         _e_fwin_cb_delete(E_Win *win);
static void         _e_fwin_cb_move(E_Win *win);
static void         _e_fwin_cb_resize(E_Win *win);
static void         _e_fwin_deleted(void        *data,
                                    Evas_Object *obj,
                                    void        *event_info);
static const char *_e_fwin_custom_file_path_eval(E_Fwin         *fwin,
                                                 Efreet_Desktop *ef,
                                                 const char     *prev_path,
                                                 const char     *key);
static void _e_fwin_desktop_run(Efreet_Desktop *desktop,
                                E_Fwin_Page    *page,
                                Eina_Bool       skip_history);
static Eina_List *_e_fwin_suggested_apps_list_get(Eina_List  *files,
                                                  Eina_List **mime_list);
static void       _e_fwin_changed(void        *data,
                                  Evas_Object *obj,
                                  void        *event_info);
static void _e_fwin_selected(void        *data,
                             Evas_Object *obj,
                             void        *event_info);
static void _e_fwin_selection_change(void        *data,
                                     Evas_Object *obj,
                                     void        *event_info);
static void _e_fwin_menu_extend(void            *data,
                                Evas_Object     *obj,
                                E_Menu          *m,
                                E_Fm2_Icon_Info *info);
static void _e_fwin_cb_menu_extend_open_with(void   *data,
                                             E_Menu *m);
static void _e_fwin_cb_menu_open_fast(void        *data,
                                      E_Menu      *m,
                                      E_Menu_Item *mi);
static void _e_fwin_parent(void        *data,
                           E_Menu      *m,
                           E_Menu_Item *mi);
static void _e_fwin_cb_key_down(void        *data,
                                Evas        *e,
                                Evas_Object *obj,
                                void        *event_info);
static void _e_fwin_cb_menu_extend_start(void            *data,
                                         Evas_Object     *obj,
                                         E_Menu          *m,
                                         E_Fm2_Icon_Info *info);
static void _e_fwin_cb_menu_open(void        *data,
                                 E_Menu      *m,
                                 E_Menu_Item *mi);
static void _e_fwin_cb_menu_open_with(void        *data,
                                      E_Menu      *m,
                                      E_Menu_Item *mi);
static void      _e_fwin_cb_all_change(void        *data,
                                       Evas_Object *obj);
static void      _e_fwin_cb_exec_cmd_changed(void *data,
                                             void *data2);
static void      _e_fwin_cb_open(void     *data,
                                 E_Dialog *dia);
static void      _e_fwin_cb_close(void     *data,
                                  E_Dialog *dia);
static void      _e_fwin_cb_dialog_free(void *obj);
static Eina_Bool _e_fwin_cb_hash_foreach(const Eina_Hash *hash __UNUSED__,
                                         const void           *key,
                                         void *data            __UNUSED__,
                                         void                 *fdata);
static E_Fwin_Exec_Type _e_fwin_file_is_exec(E_Fm2_Icon_Info *ici);
static void             _e_fwin_file_exec(E_Fwin_Page     *page,
                                          E_Fm2_Icon_Info *ici,
                                          E_Fwin_Exec_Type ext);
static void _e_fwin_file_open_dialog(E_Fwin_Page *page,
                                     Eina_List   *files,
                                     int          always);
static void _e_fwin_file_open_dialog_cb_key_down(void        *data,
                                                 Evas        *e,
                                                 Evas_Object *obj,
                                                 void        *event_info);

static void _e_fwin_pan_set(Evas_Object *obj,
                            Evas_Coord   x,
                            Evas_Coord   y);
static void _e_fwin_pan_get(Evas_Object *obj,
                            Evas_Coord  *x,
                            Evas_Coord  *y);
static void _e_fwin_pan_max_get(Evas_Object *obj,
                                Evas_Coord  *x,
                                Evas_Coord  *y);
static void _e_fwin_pan_child_size_get(Evas_Object *obj,
                                       Evas_Coord  *w,
                                       Evas_Coord  *h);
static void _e_fwin_pan_scroll_update(E_Fwin_Page *page);
static void _e_fwin_cb_page_obj_del(void *data, Evas *evas,
                                         Evas_Object *obj, void *event_info);
static void _e_fwin_zone_cb_mouse_down(void        *data,
                                       Evas        *evas,
                                       Evas_Object *obj,
                                       void        *event_info);
static Eina_Bool _e_fwin_zone_move_resize(void *data,
                                          int   type,
                                          void *event);
static Eina_Bool _e_fwin_zone_del(void *data,
                                  int   type,
                                  void *event);
static void _e_fwin_config_set(E_Fwin_Page *page);
static void _e_fwin_window_title_set(E_Fwin_Page *page);
static void _e_fwin_page_resize(E_Fwin_Page *page);
static void _e_fwin_toolbar_resize(E_Fwin_Page *page);
static int  _e_fwin_dlg_cb_desk_sort(const void *p1,
                                     const void *p2);
static int  _e_fwin_dlg_cb_desk_list_sort(const void *data1,
                                          const void *data2);

static void      _e_fwin_op_registry_listener_cb(void                          *data,
                                                 const E_Fm2_Op_Registry_Entry *ere);
static Eina_Bool _e_fwin_op_registry_entry_add_cb(void *data,
                                                  int   type,
                                                  void *event);
static void _e_fwin_op_registry_entry_iter(E_Fwin_Page *page);
static void _e_fwin_op_registry_abort_cb(void        *data,
                                         Evas_Object *obj,
                                         const char  *emission,
                                         const char  *source);

/* local subsystem globals */
static Eina_List *fwins = NULL;

/* externally accessible functions */
int
e_fwin_init(void)
{
   eina_init();

   return 1;
}

int
e_fwin_shutdown(void)
{
   E_Fwin *fwin;

   EINA_LIST_FREE(fwins, fwin)
     e_object_del(E_OBJECT(fwin));

   eina_shutdown();

   return 1;
}

/* FIXME: this opens a new window - we need a way to inherit a zone as the
 * "fwin" window
 */
void
e_fwin_new(E_Container *con,
           const char  *dev,
           const char  *path)
{
   _e_fwin_new(con, dev, path);
}

void
e_fwin_zone_new(E_Zone     *zone,
                const char *dev,
                const char *path)
{
   E_Fwin *fwin;
   E_Fwin_Page *page;
   Evas_Object *o;
   int x, y, w, h;

   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return;
   fwin->zone = zone;

   page = E_NEW(E_Fwin_Page, 1);
   page->fwin = fwin;

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
   _e_fwin_config_set(page);

   e_fm2_custom_theme_content_set(o, "desktop");

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
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, page);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend, page);
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
   e_scrollframe_extern_pan_set(o, page->fm_obj,
                                _e_fwin_pan_set,
                                _e_fwin_pan_get,
                                _e_fwin_pan_max_get,
                                _e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(page->fm_obj, 0);
   page->scrollframe_obj = o;

   e_zone_useful_geometry_get(zone, &x, &y, &w, &h);
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);
   evas_object_show(o);

   e_fm2_window_object_set(page->fm_obj, E_OBJECT(fwin->zone));

   evas_object_focus_set(page->fm_obj, 1);

   e_fm2_path_set(page->fm_obj, dev, path);

   fwin->pages = eina_list_append(fwin->pages, page);
   fwin->cur_page = fwin->pages->data;
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

   EINA_LIST_FOREACH_SAFE(fwins, f, fn, win)
     {
        if (win->zone != zone) continue;
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
             Eina_List *l2;
             E_Fwin_Page *page;

             EINA_LIST_FOREACH(fwin->pages, l2, page)
               {
                  _e_fwin_config_set(page);
                  e_fm2_refresh(page->fm_obj);
                  _e_fwin_window_title_set(page);
               }
          }
     }

   /* Hook into zones */
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     EINA_LIST_FOREACH(man->containers, ll, con)
       EINA_LIST_FOREACH(con->zones, lll, zone)
         {
            if (e_fwin_zone_find(zone)) continue;
            if ((zone->container->num == 0) && (zone->num == 0) &&
                (fileman_config->view.show_desktop_icons))
              e_fwin_zone_new(zone, "desktop", "/");
            else
              {
                 char buf[256];

                 if (fileman_config->view.show_desktop_icons)
                   {
                      snprintf(buf, sizeof(buf), "%i",
                               (zone->container->num + zone->num));
                      e_fwin_zone_new(zone, "desktop", buf);
                   }
              }
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
static E_Fwin *
_e_fwin_new(E_Container *con,
            const char  *dev,
            const char  *path)
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
   fwin->pages = eina_list_append(fwin->pages, page);
   fwin->cur_page = page;

   o = e_icon_add(e_win_evas_get(fwin->win));
   e_icon_scale_size_set(o, 0);
   e_icon_fill_inside_set(o, 0);
   edje_object_part_swallow(fwin->bg_obj, "e.swallow.bg", o);
   evas_object_pass_events_set(o, 1);
   fwin->under_obj = o;

   o = e_icon_add(e_win_evas_get(fwin->win));
   e_icon_scale_size_set(o, 0);
   e_icon_fill_inside_set(o, 0);
   edje_object_part_swallow(e_scrollframe_edje_object_get(page->scrollframe_obj), "e.swallow.overlay", o);
   evas_object_pass_events_set(o, 1);
   fwin->over_obj = o;

   e_fm2_path_set(page->fm_obj, dev, path);
   _e_fwin_window_title_set(page);

   e_win_size_min_set(fwin->win, 24, 24);

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
     {
        if (fwin->win->border->internal_icon)
          eina_stringshare_del(fwin->win->border->internal_icon);
        fwin->win->border->internal_icon =
          eina_stringshare_add("system-file-manager");
     }

   return fwin;
}

static void
_e_fwin_free(E_Fwin *fwin)
{
   E_Fwin_Page *page;

   if (!fwin) return;  //safety

   EINA_LIST_FREE(fwin->pages, page)
     _e_fwin_page_free(page);

   if (fwin->zone)
     {
        evas_object_event_callback_del(fwin->zone->bg_event_object,
                                       EVAS_CALLBACK_MOUSE_DOWN,
                                       _e_fwin_zone_cb_mouse_down);
     }

   if (fwin->zone_handler)
     ecore_event_handler_del(fwin->zone_handler);
   if (fwin->zone_del_handler)
     ecore_event_handler_del(fwin->zone_del_handler);

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
   if (fwin->win) e_object_del(E_OBJECT(fwin->win));
   free(fwin);
}

static E_Fwin_Page *
_e_fwin_page_create(E_Fwin *fwin)
{
   Evas_Object *o;
   E_Fwin_Page *page;

   page = E_NEW(E_Fwin_Page, 1);
   page->fwin = fwin;

   o = e_fm2_add(e_win_evas_get(fwin->win));
   page->fm_obj = o;
   e_fm2_view_flags_set(o, E_FM2_VIEW_DIR_CUSTOM);
   evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, _e_fwin_cb_key_down, page);

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
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, page);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend, page);
   e_fm2_window_object_set(o, E_OBJECT(fwin->win));
   evas_object_focus_set(o, 1);

   evas_object_show(o);

   o = e_scrollframe_add(e_win_evas_get(fwin->win));
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
   e_scrollframe_custom_theme_set(o, "base/theme/fileman",
                                  "e/fileman/default/scrollframe");
   evas_object_data_set(page->fm_obj, "fm_page", page);
   e_scrollframe_extern_pan_set(o, page->fm_obj,
                                _e_fwin_pan_set,
                                _e_fwin_pan_get,
                                _e_fwin_pan_max_get,
                                _e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(page->fm_obj, 0);
   page->scrollframe_obj = o;
   evas_object_move(o, 0, 0);
   evas_object_show(o);

   if (fileman_config->view.show_toolbar)
     {
        page->tbar = e_toolbar_new(e_win_evas_get(fwin->win), "toolbar",
                                   fwin->win, page->fm_obj);
        e_toolbar_show(page->tbar);
     }

   page->index = eina_list_count(fwin->pages);

   _e_fwin_config_set(page);

   page->fm_op_entry_add_handler =
     ecore_event_handler_add(E_EVENT_FM_OP_REGISTRY_ADD,
                             _e_fwin_op_registry_entry_add_cb, page);
   _e_fwin_op_registry_entry_iter(page);
   return page;
}

static void
_e_fwin_page_free(E_Fwin_Page *page)
{
   if (page->fm_obj) evas_object_del(page->fm_obj);
   if (page->tbar) e_object_del(E_OBJECT(page->tbar));
   if (page->scrollframe_obj) evas_object_del(page->scrollframe_obj);

   if (page->fm_op_entry_add_handler)
     ecore_event_handler_del(page->fm_op_entry_add_handler);

   E_FREE(page);
}

static void
_e_fwin_page_new(E_Fwin *fwin)
{
   E_Fwin_Page *page;
   const char *real;
   const char *dev, *path;

   if (!fwin->tb_obj)
     {
        page = fwin->pages->data;

        /* There is no toolbar yet */
        fwin->tb_obj = e_widget_toolbar_add(evas_object_evas_get(page->fm_obj),
                                            48 * e_scale, 48 * e_scale);

        e_widget_toolbar_focus_steal_set(fwin->tb_obj, 0);
        real = ecore_file_file_get(e_fm2_real_path_get(page->fm_obj));
        e_widget_toolbar_item_append(fwin->tb_obj, NULL, real,
                                     _e_fwin_cb_page_change, fwin, page);

        evas_object_move(fwin->tb_obj, 0, 0);
        evas_object_show(fwin->tb_obj);
     }

   page = _e_fwin_page_create(fwin);
   fwin->pages = eina_list_append(fwin->pages, page);
   real = ecore_file_file_get(e_fm2_real_path_get(fwin->cur_page->fm_obj));
   e_widget_toolbar_item_append(fwin->tb_obj, NULL, real,
                                _e_fwin_cb_page_change, fwin, page);
   e_fm2_path_get(fwin->cur_page->fm_obj, &dev, &path);
   e_fm2_path_set(page->fm_obj, dev, path);

   e_widget_toolbar_item_select(fwin->tb_obj, page->index);
   _e_fwin_cb_resize(fwin->win);
}

static void
_e_fwin_cb_page_change(void *data1,
                       void *data2)
{
   E_Fwin *fwin = data1;
   E_Fwin_Page *page = data2, *prev;

   if ((!fwin) || (!page)) return;
   prev = eina_list_nth(fwin->pages, fwin->page_index);
   fwin->page_index = page->index;

   if (prev)
     {
        evas_object_hide(prev->scrollframe_obj);
        if (prev->tbar)
          e_toolbar_hide(prev->tbar);
     }

   evas_object_show(page->scrollframe_obj);
   if (page->tbar)
     e_toolbar_show(page->tbar);

   fwin->cur_page = page;
   evas_object_focus_set(page->fm_obj, 1);
}

static const char *
_e_fwin_custom_file_path_eval(E_Fwin         *fwin,
                              Efreet_Desktop *ef,
                              const char     *prev_path,
                              const char     *key)
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
_e_fwin_suggested_apps_list_get(Eina_List  *files,
                                Eina_List **mime_list)
{
   E_Fm2_Icon_Info *ici;
   const char *f = NULL;
   char *mime;
   Eina_Hash *mimes = NULL;
   Eina_List *mlist = NULL, *apps = NULL, *ret = NULL, *l;
   Efreet_Desktop *desk = NULL;

   /* 1. build hash of mimetypes */
   EINA_LIST_FOREACH(files, l, ici)
     if (!((ici->link) && (ici->mount)))
       {
          if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE)
            {
               if (ici->link)
                 f = efreet_mime_globs_type_get(ici->link);
               if (!mimes)
                 mimes = eina_hash_string_superfast_new(NULL);
               eina_hash_del(mimes, ici->link ? f : ici->mime, (void *)1);
               eina_hash_direct_add(mimes, ici->link ? f : ici->mime, (void *)1);
            }
       }
   if (!mimes) return NULL;

   /* 2. add apps to a list so its a unique app list */
   eina_hash_foreach(mimes, _e_fwin_cb_hash_foreach, &mlist);
   eina_hash_free(mimes);

   /* 3. for each mimetype list apps that handle it */
   EINA_LIST_FOREACH(mlist, l, mime)
     apps = eina_list_merge(apps, efreet_util_desktop_mime_list(mime));

   /* 4. create a new list without duplicates */
   EINA_LIST_FREE(apps, desk)
     {
        if (!eina_list_data_find(ret, desk))
          ret = eina_list_append(ret, desk);
        else
          efreet_desktop_free(desk);
     }

   if (mime_list)
     *mime_list = mlist;
   else
   if (mlist)
     mlist = eina_list_free(mlist);

   return ret;
}

static void
_e_fwin_desktop_run(Efreet_Desktop *desktop,
                    E_Fwin_Page    *page,
                    Eina_Bool       skip_history)
{
   char pcwd[4096], buf[4096];
   Eina_List *selected, *l, *files = NULL;
   E_Fwin *fwin = page->fwin;
   E_Fm2_Icon_Info *ici;
   char *file;

   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;

   getcwd(pcwd, sizeof(pcwd));
   chdir(e_fm2_real_path_get(page->fm_obj));

   EINA_LIST_FOREACH(selected, l, ici)
     {
        E_Fwin_Exec_Type ext;

        /* this snprintf is silly - but it's here in case i really do
         * need to provide full paths (seems silly since we chdir
         * into the dir)
         */
        buf[0] = 0;
        ext = _e_fwin_file_is_exec(ici);
        if (ext == E_FWIN_EXEC_NONE)
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
   eina_list_free(selected);

   if ((fwin->win) && (desktop))
     e_exec(fwin->win->border->zone, desktop, NULL, files, "fwin");
   else if (fwin->zone && desktop)
     e_exec(fwin->zone, desktop, NULL, files, "fwin");

   EINA_LIST_FREE(files, file)
     free(file);

   chdir(pcwd);
}

static Eina_Bool
_e_fwin_cb_hash_foreach(const Eina_Hash *hash __UNUSED__,
                        const void           *key,
                        void *data            __UNUSED__,
                        void                 *fdata)
{
   Eina_List **mlist;

   mlist = fdata;
   *mlist = eina_list_append(*mlist, key);
   return 1;
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
_e_fwin_file_exec(E_Fwin_Page     *page,
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
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, ici->file, NULL, "fwin");
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, ici->file, NULL, "fwin");
        break;

      case E_FWIN_EXEC_SH:
        snprintf(buf, sizeof(buf), "/bin/sh %s", e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, NULL);
        break;

      case E_FWIN_EXEC_TERMINAL_DIRECT:
        snprintf(buf, sizeof(buf), "%s %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, NULL);
        break;

      case E_FWIN_EXEC_TERMINAL_SH:
        snprintf(buf, sizeof(buf), "%s /bin/sh %s", e_config->exebuf_term_cmd, e_util_filename_escape(ici->file));
        if (fwin->win)
          e_exec(fwin->win->border->zone, NULL, buf, NULL, NULL);
        else if (fwin->zone)
          e_exec(fwin->zone, NULL, buf, NULL, NULL);
        break;

      case E_FWIN_EXEC_DESKTOP:
        snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(page->fm_obj), ici->file);
        desktop = efreet_desktop_new(buf);
        if (desktop)
          {
             if (fwin->win)
               e_exec(fwin->win->border->zone, desktop, NULL, NULL, NULL);
             else if (fwin->zone)
               e_exec(fwin->zone, desktop, NULL, NULL, NULL);
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
        fmc.view.mode = E_FM2_VIEW_MODE_CUSTOM_ICONS;
        fmc.icon.icon.w = fileman_config->icon.icon.w * e_scale;
        fmc.icon.icon.h = fileman_config->icon.icon.h * e_scale;
        fmc.icon.fixed.w = 0;
        fmc.icon.fixed.h = 0;
#endif

        fmc.view.open_dirs_in_place = 0;
        fmc.view.fit_custom_pos = 1;
     }

   fmc.view.selector = 0;
   fmc.view.single_click = fileman_config->view.single_click;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.extension.show = fileman_config->icon.extension.show;
   fmc.list.sort.no_case = fileman_config->list.sort.no_case;
   fmc.list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   fmc.list.sort.dirs.last = fileman_config->list.sort.dirs.last;
   fmc.selection.single = fileman_config->selection.single;
   fmc.selection.windows_modifiers = fileman_config->selection.windows_modifiers;
   e_fm2_config_set(page->fm_obj, &fmc);
}

static void
_e_fwin_window_title_set(E_Fwin_Page *page)
{
   char buf[PATH_MAX];
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
_e_fwin_page_resize(E_Fwin_Page *page)
{
   if (page->tbar)
     _e_fwin_toolbar_resize(page);
   else
     {
        int offset = 0;

        if (page->fwin->tb_obj)
          evas_object_geometry_get(page->fwin->tb_obj, NULL, NULL, NULL, &offset);
        evas_object_move(page->scrollframe_obj, 0, offset);
        evas_object_resize(page->scrollframe_obj, page->fwin->win->w, page->fwin->win->h - offset);
     }
}

static void
_e_fwin_toolbar_resize(E_Fwin_Page *page)
{
   int tx, ty, tw, th, offset = 0;
   int x, y, w, h;

   if (page->fwin->tb_obj)
     evas_object_geometry_get(page->fwin->tb_obj, NULL, NULL, NULL, &offset);
   w = page->fwin->win->w;
   h = page->fwin->win->h;
   switch (page->tbar->gadcon->orient)
     {
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
        tx = 0;
        ty = offset;
        th = 32;
        tw = w;

        x = 0;
        y = offset + th;
        h = (h - offset - th);
        break;

      case E_GADCON_ORIENT_BOTTOM:
        tx = 0;
        th = 32;
        tw = w;
        ty = h - th;

        x = 0;
        y = offset;
        h = (h - offset - th);
        break;

      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
        tx = 0;
        tw = 32;
        th = h - offset;
        ty = offset;

        x = tw;
        y = offset;
        w = (w - tw);
        break;

      case E_GADCON_ORIENT_RIGHT:
        ty = offset;
        tw = 32;
        tx = w - tw;
        th = h - offset;

        x = 0;
        y = offset;
        w = (w - tw);
        break;

      default:
        return;
     }
   e_toolbar_move_resize(page->tbar, tx, ty, tw, th);
   evas_object_move(page->scrollframe_obj, x, y);
   evas_object_resize(page->scrollframe_obj, w, h);
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
   E_Fwin *fwin;

   if (!win) return;  //safety
   fwin = win->data;
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
   if (fwin->win)
     {
        E_Fwin_Page *page;
        Eina_List *l;

        if (fwin->tb_obj)
          {
             int height;

             e_widget_size_min_get(fwin->tb_obj, NULL, &height);
             evas_object_resize(fwin->tb_obj, fwin->win->w, height);
          }
        EINA_LIST_FOREACH(fwin->pages, l, page)
          _e_fwin_page_resize(page);
     }
   else if (fwin->zone)
     evas_object_resize(fwin->cur_page->scrollframe_obj, fwin->zone->w, fwin->zone->h);
   /* _e_fwin_geom_save(fwin); */
}

static void
_e_fwin_deleted(void            *data,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   E_Fwin_Page *page;

   page = data;
   e_object_del(E_OBJECT(page->fwin));
}

static void
_e_fwin_changed(void            *data,
                Evas_Object     *obj,
                void *event_info __UNUSED__)
{
   E_Fwin *fwin;
   E_Fwin_Page *page;
   Efreet_Desktop *ef;
   char buf[PATH_MAX], *ext;

   page = data;
   fwin = page->fwin;
   if (!fwin) return;  //safety

   /* FIXME: first look in E config for a special override for this dir's bg
    * or overlay
    */
   snprintf(buf, sizeof(buf), "%s/.directory.desktop", e_fm2_real_path_get(page->fm_obj));
   ef = efreet_desktop_new(buf);
   if (ef)
     {
        fwin->wallpaper_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->wallpaper_file, "X-Enlightenment-Directory-Wallpaper");
        fwin->overlay_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->overlay_file, "X-Enlightenment-Directory-Overlay");
        fwin->scrollframe_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->scrollframe_file, "X-Enlightenment-Directory-Scrollframe");
        fwin->theme_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->theme_file, "X-Enlightenment-Directory-Theme");
        efreet_desktop_free(ef);
     }
   else
     {
#define RELEASE_STR(x) if (x) {eina_stringshare_del(x); (x) = NULL;}
        RELEASE_STR(fwin->wallpaper_file);
        RELEASE_STR(fwin->overlay_file);
        RELEASE_STR(fwin->scrollframe_file);
        RELEASE_STR(fwin->theme_file);
#undef RELEASE_STR
     }
   if (fwin->under_obj)
     {
        evas_object_hide(fwin->under_obj);
        if (fwin->wallpaper_file)
          {
             ext = strrchr(fwin->wallpaper_file, '.');
             if (ext && !strcasecmp(ext, ".edj"))
               e_icon_file_edje_set(fwin->under_obj, fwin->wallpaper_file, "e/desktop/background");
             else
               e_icon_file_set(fwin->under_obj, fwin->wallpaper_file);
          }
        else
          e_icon_file_edje_set(fwin->under_obj, NULL, NULL);
        evas_object_show(fwin->under_obj);
     }
   if (fwin->over_obj)
     {
        evas_object_hide(fwin->over_obj);
        if (fwin->overlay_file)
          {
             ext = strrchr(fwin->overlay_file, '.');
             if (ext && !strcasecmp(ext, ".edj"))
               e_icon_file_edje_set(fwin->over_obj, fwin->overlay_file, "e/desktop/background");
             else
               e_icon_file_set(fwin->over_obj, fwin->overlay_file);
          }
        else
          e_icon_file_edje_set(fwin->over_obj, NULL, NULL);
        evas_object_show(fwin->over_obj);
     }
   if (page->scrollframe_obj)
     {
        if ((fwin->scrollframe_file) &&
            (e_util_edje_collection_exists(fwin->scrollframe_file, "e/fileman/default/scrollframe")))
          e_scrollframe_custom_edje_file_set(page->scrollframe_obj,
                                             (char *)fwin->scrollframe_file,
                                             "e/fileman/default/scrollframe");
        else
          {
             if (fwin->zone)
               e_scrollframe_custom_theme_set(page->scrollframe_obj,
                                              "base/theme/fileman",
                                              "e/fileman/desktop/scrollframe");
             else
               e_scrollframe_custom_theme_set(page->scrollframe_obj,
                                              "base/theme/fileman",
                                              "e/fileman/default/scrollframe");
          }
        e_scrollframe_child_pos_set(page->scrollframe_obj, 0, 0);
     }
   if (fwin->tb_obj)
     {
        const char *file;

        file = ecore_file_file_get(e_fm2_real_path_get(page->fm_obj));
        e_widget_toolbar_item_label_set(fwin->tb_obj, fwin->page_index, file);
     }
   if ((fwin->theme_file) && (ecore_file_exists(fwin->theme_file)))
     e_fm2_custom_theme_set(obj, fwin->theme_file);
   else
     e_fm2_custom_theme_set(obj, NULL);

   if (fwin->zone) return;
   _e_fwin_window_title_set(page);
}

static void
_e_fwin_selected(void            *data,
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
_e_fwin_selection_change(void            *data,
                         Evas_Object *obj __UNUSED__,
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
}

static void
_e_fwin_cb_all_change(void            *data,
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
}

static void
_e_fwin_cb_key_down(void            *data,
                    Evas *e          __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void            *event_info)
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
             int count = eina_list_count(fwin->pages);
             E_Fwin_Page *page;

             if (count > 2)
               {
                  Eina_List *l;
                  int i = 0;

                  page = fwin->cur_page;
                  if (fwin->page_index > 0)
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj,
                                                      fwin->page_index - 1);
                    }
                  else
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj, 1);
                    }
                  if (fwin->tb_obj)
                    e_widget_toolbar_item_remove(fwin->tb_obj, page->index);
                  fwin->pages = eina_list_remove(fwin->pages, page);
                  _e_fwin_page_free(page);
                  EINA_LIST_FOREACH(fwin->pages, l, page)
                    page->index = i++;
               }
             else if (count > 1)
               {
                  if (fwin->tb_obj)
                    evas_object_del(fwin->tb_obj);
                  fwin->tb_obj = NULL;
                  fwin->page_index = 0;
                  fwin->pages = eina_list_remove(fwin->pages, fwin->cur_page);
                  _e_fwin_page_free(fwin->cur_page);
                  page = fwin->pages->data;
                  page->index = 0;
                  _e_fwin_cb_page_change(fwin, page);
                  _e_fwin_cb_resize(fwin->win);
               }
             else
               e_object_del(E_OBJECT(fwin));
             return;
          }
        if (!strcmp(ev->key, "a"))
          {
             e_fm2_all_sel(page->fm_obj);
             return;
          }
        if (!strcmp(ev->key, "t"))
          {
             _e_fwin_page_new(fwin);
             return;
          }
        if (!strcmp(ev->key, "Tab"))
          {
             Eina_List *l;

             if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               {
                  l = eina_list_nth_list(fwin->pages, fwin->page_index);
                  if (l->prev)
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj,
                                                      fwin->page_index - 1);
                    }
                  else
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj,
                                                      eina_list_count(fwin->pages) - 1);
                    }
               }
             else
               {
                  l = eina_list_nth_list(fwin->pages, fwin->page_index);
                  if (l->next)
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj,
                                                      fwin->page_index + 1);
                    }
                  else
                    {
                       if (fwin->tb_obj)
                         e_widget_toolbar_item_select(fwin->tb_obj, 0);
                    }
               }
             return;
          }
     }
}

static void
_e_fwin_cb_page_obj_del(void            *data,
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
_e_fwin_zone_cb_mouse_down(void            *data,
                           Evas *evas       __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   E_Fwin *fwin;

   fwin = data;
   if (!fwin) return;
   e_fwin_all_unsel(fwin);
}

static Eina_Bool
_e_fwin_zone_move_resize(void *data,
                         int   type,
                         void *event)
{
   E_Event_Zone_Move_Resize *ev;
   E_Fwin *fwin;

   if (type != E_EVENT_ZONE_MOVE_RESIZE) return ECORE_CALLBACK_PASS_ON;
   fwin = data;
   ev = event;
   if (!fwin) return ECORE_CALLBACK_PASS_ON;
   if (fwin->zone != ev->zone) return ECORE_CALLBACK_PASS_ON;
   if (fwin->bg_obj)
     {
        evas_object_move(fwin->bg_obj, ev->zone->x, ev->zone->y);
        evas_object_resize(fwin->bg_obj, ev->zone->w, ev->zone->h);
     }
   if (fwin->cur_page->scrollframe_obj)
     {
        int x, y, w, h;
        e_zone_useful_geometry_get(ev->zone, &x, &y, &w, &h);
        evas_object_move(fwin->cur_page->scrollframe_obj, x, y);
        evas_object_resize(fwin->cur_page->scrollframe_obj, w, h);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_fwin_zone_del(void *data,
                 int   type,
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

/* fm menu extend */
static void
_e_fwin_menu_extend(void                 *data,
                    Evas_Object          *obj,
                    E_Menu               *m,
                    E_Fm2_Icon_Info *info __UNUSED__)
{
   E_Fwin_Page *page;
   E_Menu_Item *mi;

   page = data;
   if (e_fm2_has_parent_get(obj))
     {
        mi = e_menu_item_new(m);
        e_menu_item_separator_set(mi, 1);

        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Go to Parent Directory"));
        e_menu_item_icon_edje_set(mi,
                                  e_theme_edje_file_get("base/theme/fileman",
                                                        "e/fileman/default/button/parent"),
                                  "e/fileman/default/button/parent");
        e_menu_item_callback_set(mi, _e_fwin_parent, obj);
     }
   /* FIXME: if info != null then check mime type and offer options based
    * on that
    */
}

static void
_e_fwin_parent(void           *data,
               E_Menu *m       __UNUSED__,
               E_Menu_Item *mi __UNUSED__)
{
   e_fm2_parent_go(data);
}

static void
_e_fwin_cb_menu_open_fast(void        *data,
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
_e_fwin_cb_menu_extend_open_with(void   *data,
                                 E_Menu *m)
{
   Eina_List *selected = NULL, *apps = NULL, *l;
   E_Menu_Item *mi;
   E_Fwin_Page *page;
   Efreet_Desktop *desk = NULL;

   page = data;

   selected = e_fm2_selected_list_get(page->fm_obj);
   if (!selected) return;

   apps = _e_fwin_suggested_apps_list_get(selected, NULL);
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
_e_fwin_cb_menu_extend_start(void                 *data,
                             Evas_Object *obj      __UNUSED__,
                             E_Menu               *m,
                             E_Fm2_Icon_Info *info __UNUSED__)
{
   E_Menu_Item *mi;
   E_Fwin_Page *page;
   E_Menu *subm;

   page = data;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open"));
   e_util_menu_item_theme_icon_set(mi, "document-open");
   e_menu_item_callback_set(mi, _e_fwin_cb_menu_open, page);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open with..."));
   e_util_menu_item_theme_icon_set(mi, "document-open");

   subm = e_menu_new();
   e_menu_item_submenu_set(mi, subm);
   e_menu_pre_activate_callback_set(subm, _e_fwin_cb_menu_extend_open_with, page);
}

static void
_e_fwin_cb_menu_open(void           *data,
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
_e_fwin_cb_menu_open_with(void           *data,
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

/* 'open with' dialog*/
static void
_e_fwin_file_open_dialog(E_Fwin_Page *page,
                         Eina_List   *files,
                         int          always)
{
   E_Fwin *fwin = page->fwin, *fwin2 = NULL;
   E_Dialog *dia;
   Evas_Coord mw, mh;
   Evas_Object *o, *of, *ot;
   Evas *evas;
   Eina_List *l = NULL, *apps = NULL, *mlist = NULL;
   Eina_List *cats = NULL;
   Efreet_Desktop *desk = NULL;
   E_Fwin_Apps_Dialog *fad;
   E_Fm2_Icon_Info *ici;
   char buf[PATH_MAX];
   int need_dia = 0;

   if (fwin->fad)
     {
        e_object_del(E_OBJECT(fwin->fad->dia));
        fwin->fad = NULL;
     }
   if (!always)
     {
        EINA_LIST_FOREACH(files, l, ici)
          {
             if ((ici->link) && (ici->mount))
               {
                  if (!fileman_config->view.open_dirs_in_place || fwin->zone)
                    {
                       if (fwin->win)
                         fwin2 = _e_fwin_new(fwin->win->container, ici->link, "/");
                       else if (fwin->zone)
                         fwin2 = _e_fwin_new(fwin->zone->container, ici->link, "/");
                    }
                  else
                    {
                       e_fm2_path_set(page->fm_obj, ici->link, "/");
                       _e_fwin_window_title_set(page);
                    }
               }
             else if ((ici->link) && (ici->removable))
               {
                  snprintf(buf, sizeof(buf), "removable:%s", ici->link);
                  if (!fileman_config->view.open_dirs_in_place || fwin->zone)
                    {
                       if (fwin->win)
                         fwin2 = _e_fwin_new(fwin->win->container, buf, "/");
                       else if (fwin->zone)
                         fwin2 = _e_fwin_new(fwin->zone->container, buf, "/");
                    }
                  else
                    {
                       e_fm2_path_set(page->fm_obj, buf, "/");
                       _e_fwin_window_title_set(page);
                    }
               }
             else if (ici->real_link)
               {
                  if (S_ISDIR(ici->statinfo.st_mode))
                    {
                       if ((!fileman_config->view.open_dirs_in_place) || (fwin->zone))
                         {
                            if (fwin->win)
                              fwin2 = _e_fwin_new(fwin->win->container, NULL, ici->real_link);
                            else if (fwin->zone)
                              fwin2 = _e_fwin_new(fwin->zone->container, NULL, ici->real_link);
                         }
                       else
                         {
                            e_fm2_path_set(page->fm_obj, NULL, ici->real_link);
                            _e_fwin_window_title_set(page);
                         }
                    }
                  else
                    need_dia = 1;
               }
             else
               {
                  snprintf(buf, sizeof(buf), "%s/%s",
                           e_fm2_real_path_get(page->fm_obj), ici->file);
                  if (S_ISDIR(ici->statinfo.st_mode))
                    {
                       if ((!fileman_config->view.open_dirs_in_place) || (fwin->zone))
                         {
                            if (fwin->win)
                              fwin2 = _e_fwin_new(fwin->win->container, NULL, buf);
                            else if (fwin->zone)
                              fwin2 = _e_fwin_new(fwin->zone->container, NULL, buf);
                         }
                       else
                         {
                            e_fm2_path_set(page->fm_obj, NULL, buf);
                            _e_fwin_window_title_set(page);
                         }
                    }
                  else
                    need_dia = 1;
               }
             if (fwin2)
               {
                  if ((fwin2->win) && (fwin2->win->border))
                    {
                       Evas_Object *oic;
                       const char *itype = NULL;
                       int ix, iy, iw, ih, nx, ny, found = 0;
                       E_Remember *rem = NULL;
                       Eina_List *ll;

                       oic = e_fm2_icon_get(evas_object_evas_get(page->fm_obj),
                                            ici->ic, NULL, NULL, 0, &itype);
                       if (oic)
                         {
                            const char *file = NULL, *group = NULL;
     /* E_Fm2_Custom_File *cf; */

                            if (fwin2->win->border->internal_icon)
                              eina_stringshare_del(fwin2->win->border->internal_icon);
                            fwin2->win->border->internal_icon = NULL;
                            if (fwin2->win->border->internal_icon_key)
                              eina_stringshare_del(fwin2->win->border->internal_icon_key);
                            fwin2->win->border->internal_icon_key = NULL;

                            if (!strcmp(evas_object_type_get(oic), "edje"))
                              {
                                 edje_object_file_get(oic, &file, &group);
                                 if (file)
                                   {
                                      fwin2->win->border->internal_icon =
                                        eina_stringshare_add(file);
                                      if (group)
                                        fwin2->win->border->internal_icon_key =
                                          eina_stringshare_add(group);
                                   }
                              }
                            else
                              {
                                 file = e_icon_file_get(oic);
                                 fwin2->win->border->internal_icon =
                                   eina_stringshare_add(file);
                              }
                            evas_object_del(oic);

                            snprintf(buf, sizeof(buf), "e_fwin::%s", e_fm2_real_path_get(fwin2->cur_page->fm_obj));
                            EINA_LIST_FOREACH(e_config->remembers, ll, rem)
                              if (rem->class && !strcmp(rem->class, buf))
                                {
                                   found = 1;
                                   break;
                                }

                            if (!found)
                              {
                                 int w, h, zw, zh;

                                 e_zone_useful_geometry_get(fwin2->win->border->zone,
                                                            NULL, NULL, &zw, &zh);

     /* No custom info, so just put window near icon */
                                 e_fm2_icon_geometry_get(ici->ic, &ix, &iy, &iw, &ih);
                                 nx = (ix + (iw / 2));
                                 ny = (iy + (ih / 2));
                                 if (fwin->win)
                                   {
                                      nx += fwin->win->x;
                                      ny += fwin->win->y;
                                   }

     /* checking width and height */
                                 /* TODO add config for preffered
                                    initial size? */
                                 w = 5 * iw * e_scale;
                                 if (w > DEFAULT_WIDTH)
                                   w = DEFAULT_WIDTH;
                                 if (w > zw)
                                   w = zw;

                                 h = 4 * ih * e_scale;
                                 if (h > DEFAULT_HEIGHT)
                                   h = DEFAULT_HEIGHT;
                                 if (h > zh)
                                   h = zh;

     /* iff going out of zone - adjust to be in */
                                 if ((fwin2->win->border->zone->x + fwin2->win->border->zone->w) < (w + nx))
                                   nx -= w;
                                 if ((fwin2->win->border->zone->y + fwin2->win->border->zone->h) < (h + ny))
                                   ny -= h;

                                 e_win_move_resize(fwin2->win, nx, ny, w, h);
                                 fwin2->win->border->placed = 1;
                              }
                         }
                       if (ici->label)
                         e_win_title_set(fwin2->win, ici->label);
                       else if (ici->file)
                         e_win_title_set(fwin2->win, ici->file);
                    }
                  fwin2 = NULL;
               }
          }
        if (!need_dia) return;
        need_dia = 0;
     }

   apps = _e_fwin_suggested_apps_list_get(files, &mlist);

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
          if (eina_list_count(mlist) <= 1)
            {
               char *file;
               char pcwd[4096];
               Eina_List *files_list = NULL;

               need_dia = 1;
               if (mlist) desk = e_exehist_mime_desktop_get(mlist->data);
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
                 }
               EINA_LIST_FREE(files_list, file)
                 free(file);

               chdir(pcwd);
               if (!need_dia)
                 {
                    apps = eina_list_free(apps);
                    mlist = eina_list_free(mlist);
                    return;
                 }
            }
     }
   mlist = eina_list_free(mlist);

   fad = E_NEW(E_Fwin_Apps_Dialog, 1);
   if (fwin->win)
     dia = e_dialog_new(fwin->win->border->zone->container,
                        "E", "_fwin_open_apps");
   else if (fwin->zone)
     dia = e_dialog_new(fwin->zone->container,
                        "E", "_fwin_open_apps");
   else return;  /* make clang happy */

   e_dialog_title_set(dia, _("Open with..."));
   e_dialog_resizable_set(dia, 1);
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

   ot = e_widget_table_add(evas, 0);

   l = eina_list_free(l);

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
     e_widget_ilist_header_append(o, NULL, _("Specific Applications"));
   EINA_LIST_FOREACH(apps, l, desk)
     {
        Evas_Object *icon = NULL;

        if (!desk) continue;
        icon = e_util_desktop_icon_add(desk, 24, evas);
        e_widget_ilist_append(o, icon, desk->name, NULL, NULL,
                              efreet_util_path_to_file_id(desk->orig_path));
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
   e_widget_size_min_set(o, 160, 240);
   e_widget_framelist_object_append(of, o);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Custom Command"));
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 1, 0);
   fad->o_entry = e_widget_entry_add(evas, &(fad->exec_cmd),
                                     _e_fwin_cb_exec_cmd_changed, fad, NULL);
   e_widget_table_object_append(ot, fad->o_entry, 0, 2, 1, 1, 1, 1, 1, 0);

   e_widget_size_min_get(ot, &mw, &mh);
   e_dialog_content_set(dia, ot, mw, mh);
   evas_object_event_callback_add(ot, EVAS_CALLBACK_KEY_DOWN, _e_fwin_file_open_dialog_cb_key_down, page);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "preferences-applications");
   e_widget_focus_steal(fad->o_entry);
}

static void
_e_fwin_file_open_dialog_cb_key_down(void          *data,
                                     Evas *e        __UNUSED__,
                                     Evas_Object *o __UNUSED__,
                                     void          *event_info)
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
_e_fwin_cb_exec_cmd_changed(void       *data,
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
_e_fwin_cb_open(void         *data,
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
_e_fwin_cb_close(void         *data,
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
                Evas_Coord   x,
                Evas_Coord   y)
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
                Evas_Coord  *x,
                Evas_Coord  *y)
{
   E_Fwin_Page *page;

   page = evas_object_data_get(obj, "fm_page");
   e_fm2_pan_get(obj, x, y);
   page->fm_pan.x = *x;
   page->fm_pan.y = *y;
}

static void
_e_fwin_pan_max_get(Evas_Object *obj,
                    Evas_Coord  *x,
                    Evas_Coord  *y)
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
                           Evas_Coord  *w,
                           Evas_Coord  *h)
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
   if (page->fwin->under_obj)
     edje_object_message_send(page->fwin->under_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (page->fwin->over_obj)
     edje_object_message_send(page->fwin->over_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (page->scrollframe_obj)
     edje_object_message_send(e_scrollframe_edje_object_get(page->scrollframe_obj), EDJE_MESSAGE_INT_SET, 1, msg);
   page->fm_pan_last.x = page->fm_pan.x;
   page->fm_pan_last.y = page->fm_pan.y;
   page->fm_pan_last.max_x = page->fm_pan.max_x;
   page->fm_pan_last.max_y = page->fm_pan.max_y;
   page->fm_pan_last.w = page->fm_pan.w;
   page->fm_pan_last.h = page->fm_pan.h;
}

/* e_fm_op_registry */
static void
_e_fwin_op_registry_listener_cb(void                          *data,
                                const E_Fm2_Op_Registry_Entry *ere)
{
   Evas_Object *o = data;
   char buf[PATH_MAX];
   char *total;
   int mw, mh;

   // Don't show if the operation keep less than 1 second
   if (ere->start_time + 1.0 > ecore_loop_time_get()) return;

   // Update element
   edje_object_part_drag_size_set(o, "e.gauge.bar", ((double)(ere->percent)) / 100, 1.0);
   edje_object_size_min_get(o, &mw, &mh);
   evas_object_resize(o, mw * e_scale, mh * e_scale);
   evas_object_show(o);

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

      default:
        edje_object_signal_emit(o, "e,action,icon,unknow", "e");
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
               snprintf(buf, sizeof(buf), _("Copying %s (eta: %d sec)"), total, ere->eta);
             break;

           case E_FM_OP_MOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Move of %s done"), total);
             else
               snprintf(buf, sizeof(buf), _("Moving %s (eta: %d sec)"), total, ere->eta);
             break;

           case E_FM_OP_REMOVE:
             if (ere->finished)
               snprintf(buf, sizeof(buf), _("Delete done"));
             else
               snprintf(buf, sizeof(buf), _("Deleting files..."));
             break;

           default:
             snprintf(buf, sizeof(buf), _("Unknow operation from slave %d"), ere->id);
          }
        E_FREE(total);
     }
   edje_object_part_text_set(o, "e.text.info", buf);

   if (ere->needs_attention)
     edje_object_signal_emit(o, "e,action,set,need_attention", "e");
   else
     edje_object_signal_emit(o, "e,action,set,normal", "e");
}

static Eina_Bool
_e_fwin_op_registry_free_data_delayed(void *data)
{
   evas_object_del((Evas_Object *)data);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_fwin_op_registry_free_data(void *data)
{
   ecore_timer_add(5.0, _e_fwin_op_registry_free_data_delayed, data);
}

static Eina_Bool
_e_fwin_op_registry_entry_add_cb(void          *data,
                                 __UNUSED__ int type,
                                 void          *event)
{
   E_Fm2_Op_Registry_Entry *ere = (E_Fm2_Op_Registry_Entry *)event;
   E_Fwin_Page *page = data;
   Evas_Object *o;

   if (!(ere->op == E_FM_OP_COPY || ere->op == E_FM_OP_MOVE ||
         ere->op == E_FM_OP_REMOVE))
     return ECORE_CALLBACK_RENEW;

   o = edje_object_add(evas_object_evas_get(page->scrollframe_obj));
   e_theme_edje_object_set(o, "base/theme/fileman",
                           "e/fileman/default/progress");

   // Append the element to the box
   edje_object_part_box_append(e_scrollframe_edje_object_get(page->scrollframe_obj),
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
_e_fwin_op_registry_abort_cb(void                *data,
                             Evas_Object *obj     __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source   __UNUSED__)
{
   int id;

   id = (long)data;
   if (!id) return;
   e_fm2_operation_abort(id);
}


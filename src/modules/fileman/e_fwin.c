/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* FIXME: fwin - he fm2 filemanager wrapped with a window and scrollframe.
 * primitive BUT enough to test generic dnd and fm stuff more easily. don't
 * play with this unless u want to help with it. NOT COMPLETE! BEWARE!
 */
/* FIXME: multiple selected files across different fwins - you can only dnd the
 * ones in the 1 window src - not all selected ones. also selecting a new file
 * in a new fwin doesnt deseclect other selections in other fwin's (unless
 * multi-selecting)
 */

typedef struct _E_Fwin E_Fwin;
typedef struct _E_Fwin_Apps_Dialog E_Fwin_Apps_Dialog;

#define E_FWIN_TYPE 0xE0b0101f

struct _E_Fwin
{
   E_Object             e_obj_inherit;
   
   E_Win               *win;
   E_Zone              *zone;
   Evas_Object         *scrollframe_obj;
   Evas_Object         *fm_obj;
   Evas_Object         *bg_obj;
   E_Fwin_Apps_Dialog  *fad;

   Evas_Object         *under_obj;
   Evas_Object         *over_obj;
   struct {
      Evas_Coord        x, y, max_x, max_y, w, h;
   } fm_pan, fm_pan_last;
   
   const char         *wallpaper_file;
   const char         *overlay_file;
   const char         *scrollframe_file;
   const char         *theme_file;

   E_Toolbar          *tbar;
   Ecore_Event_Handler *zone_handler;
   Ecore_Event_Handler *zone_del_handler;
   
   unsigned char        geom_save_ready : 1;
};

struct _E_Fwin_Apps_Dialog
{
   E_Dialog    *dia;
   E_Fwin      *fwin;
   char        *app1, *app2;
   Evas_Object *o_specific, *o_all;
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

/* local subsystem functions */
static E_Fwin *_e_fwin_new(E_Container *con, const char *dev, const char *path);
static void _e_fwin_free(E_Fwin *fwin);
static void _e_fwin_cb_delete(E_Win *win);
static void _e_fwin_cb_move(E_Win *win);
static void _e_fwin_cb_resize(E_Win *win);
static void _e_fwin_deleted(void *data, Evas_Object *obj, void *event_info);
static const char *_e_fwin_custom_file_path_eval(E_Fwin *fwin, Efreet_Desktop *ef, const char *prev_path, const char *key);
static void _e_fwin_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_selection_change(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_menu_extend(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
static void _e_fwin_parent(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_cb_menu_extend_start(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info);
static void _e_fwin_cb_menu_open(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_cb_menu_open_with(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_fwin_cb_all_change(void *data, Evas_Object *obj);
static void _e_fwin_cb_specific_change(void *data, Evas_Object *obj);
static void _e_fwin_cb_exec_cmd_changed(void *data, void *data2);
static void _e_fwin_cb_open(void *data, E_Dialog *dia);
static void _e_fwin_cb_close(void *data, E_Dialog *dia);
static void _e_fwin_cb_dialog_free(void *obj);
static Eina_Bool _e_fwin_cb_hash_foreach(const Eina_Hash *hash __UNUSED__, const void *key, void *data __UNUSED__, void *fdata);
static E_Fwin_Exec_Type _e_fwin_file_is_exec(E_Fm2_Icon_Info *ici);
static void _e_fwin_file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext);
static void _e_fwin_file_open_dialog(E_Fwin *fwin, Eina_List *files, int always);

static void _e_fwin_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_fwin_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fwin_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fwin_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
static void _e_fwin_pan_scroll_update(E_Fwin *fwin);

static void _e_fwin_zone_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static int  _e_fwin_zone_move_resize(void *data, int type, void *event);
static int  _e_fwin_zone_del(void *data, int type, void *event);
static void _e_fwin_config_set(E_Fwin *fwin);
static void _e_fwin_window_title_set(E_Fwin *fwin);
static void _e_fwin_toolbar_resize(E_Fwin *fwin);
static int _e_fwin_dlg_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2);
static int _e_fwin_dlg_cb_desk_list_sort(const void *data1, const void *data2);

/* local subsystem globals */
static Eina_List *fwins = NULL;

/* externally accessible functions */
EAPI int
e_fwin_init(void)
{
   eina_stringshare_init();

   return 1;
}

EAPI int
e_fwin_shutdown(void)
{
   E_Fwin *fwin;

   EINA_LIST_FREE(fwins, fwin)
     e_object_del(E_OBJECT(fwin));

   eina_stringshare_shutdown();

   return 1;
}

/* FIXME: this opens a new window - we need a way to inherit a zone as the
 * "fwin" window
 */
EAPI void
e_fwin_new(E_Container *con, const char *dev, const char *path)
{
   E_Fwin *fwin;

   fwin = _e_fwin_new(con, dev, path);
}

EAPI void
e_fwin_zone_new(E_Zone *zone, const char *dev, const char *path)
{
   E_Fwin *fwin;
   Evas_Object *o;
   
   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return;
   fwin->zone = zone;
   
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
   fwin->fm_obj = o;
   _e_fwin_config_set(fwin);

   e_fm2_custom_theme_content_set(o, "desktop");
   evas_object_smart_callback_add(o, "dir_changed",
				  _e_fwin_changed, fwin);
   evas_object_smart_callback_add(o, "dir_deleted",
				  _e_fwin_deleted, fwin);
   evas_object_smart_callback_add(o, "selected",
				  _e_fwin_selected, fwin);
   evas_object_smart_callback_add(o, "selection_change",
				  _e_fwin_selection_change, fwin);
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, fwin);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend, fwin);
   e_fm2_underlay_hide(o);
   evas_object_show(o);
   
   o = e_scrollframe_add(zone->container->bg_evas);
   ecore_x_icccm_state_set(zone->container->bg_win, ECORE_X_WINDOW_STATE_HINT_NORMAL);
   e_drop_xdnd_register_set(zone->container->bg_win, 1);
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
   evas_object_data_set(fwin->fm_obj, "fwin", fwin);
   e_scrollframe_extern_pan_set(o, fwin->fm_obj,
				_e_fwin_pan_set,
				_e_fwin_pan_get,
				_e_fwin_pan_max_get,
				_e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(fwin->fm_obj, 0);
   fwin->scrollframe_obj = o;
   evas_object_move(o, fwin->zone->x, fwin->zone->y);
   evas_object_resize(o, fwin->zone->w, fwin->zone->h);
   evas_object_show(o);

   e_fm2_window_object_set(fwin->fm_obj, E_OBJECT(fwin->zone));
   
   evas_object_focus_set(fwin->fm_obj, 1);

   e_fm2_path_set(fwin->fm_obj, dev, path);
}

EAPI void
e_fwin_all_unsel(void *data)
{
   E_Fwin *fwin;
   
   fwin = data;
   E_OBJECT_CHECK(fwin);
   E_OBJECT_TYPE_CHECK(fwin, E_FWIN_TYPE);
   e_fm2_all_unsel(fwin->fm_obj);
}

EAPI void 
e_fwin_zone_shutdown(E_Zone *zone) 
{
   Eina_List *f;
	E_Fwin *win;
	
   EINA_LIST_FOREACH(fwins, f, win)
     {
	if (win->zone != zone) continue;
	e_object_del(E_OBJECT(win));
	win = NULL;
     }
}

EAPI void 
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
	if (!fwin) continue; //safety
	if (fwin->zone)
	  e_fwin_zone_shutdown(fwin->zone);
	else 
	  {
	     _e_fwin_config_set(fwin);
	     e_fm2_refresh(fwin->fm_obj);
	     _e_fwin_window_title_set(fwin);
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

EAPI int
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
_e_fwin_new(E_Container *con, const char *dev, const char *path) 
{
   E_Fwin *fwin;
   Evas_Object *o;
   char buf[4096];
   
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

   o = e_fm2_add(e_win_evas_get(fwin->win));
   fwin->fm_obj = o;
   _e_fwin_config_set(fwin);
   
   evas_object_smart_callback_add(o, "dir_changed",
				  _e_fwin_changed, fwin);
   evas_object_smart_callback_add(o, "dir_deleted",
				  _e_fwin_deleted, fwin);
   evas_object_smart_callback_add(o, "selected",
				  _e_fwin_selected, fwin);
   evas_object_smart_callback_add(o, "selection_change",
				  _e_fwin_selection_change, fwin);
   e_fm2_icon_menu_start_extend_callback_set(o, _e_fwin_cb_menu_extend_start, fwin);
   e_fm2_icon_menu_end_extend_callback_set(o, _e_fwin_menu_extend, fwin);
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
   evas_object_data_set(fwin->fm_obj, "fwin", fwin);
   e_scrollframe_extern_pan_set(o, fwin->fm_obj,
				_e_fwin_pan_set,
				_e_fwin_pan_get,
				_e_fwin_pan_max_get,
				_e_fwin_pan_child_size_get);
   evas_object_propagate_events_set(fwin->fm_obj, 0);
   fwin->scrollframe_obj = o;
   evas_object_move(o, 0, 0);
   evas_object_show(o);

   if (fileman_config->view.show_toolbar) 
     {
	fwin->tbar = e_toolbar_new(e_win_evas_get(fwin->win), "toolbar", 
				   fwin->win, fwin->fm_obj);
	e_toolbar_show(fwin->tbar);
     }

   o = edje_object_add(e_win_evas_get(fwin->win));
   edje_object_part_swallow(fwin->bg_obj, "e.swallow.bg", o);
   evas_object_pass_events_set(o, 1);
   fwin->under_obj = o;
   
   o = edje_object_add(e_win_evas_get(fwin->win));
   edje_object_part_swallow(e_scrollframe_edje_object_get(fwin->scrollframe_obj), "e.swallow.overlay", o);
   evas_object_pass_events_set(o, 1);
   fwin->over_obj = o;
   
   e_fm2_window_object_set(fwin->fm_obj, E_OBJECT(fwin->win));
   
   evas_object_focus_set(fwin->fm_obj, 1);

   e_fm2_path_set(fwin->fm_obj, dev, path);
   
   snprintf(buf, sizeof(buf), "_fwin::/%s", e_fm2_real_path_get(fwin->fm_obj));
   e_win_name_class_set(fwin->win, "E", buf);
   
   _e_fwin_window_title_set(fwin);
   
   e_win_size_min_set(fwin->win, 24, 24);
   e_win_resize(fwin->win, 280 * e_scale, 200 * e_scale);
   e_win_show(fwin->win);
   if (fwin->win->evas_win)
     e_drop_xdnd_register_set(fwin->win->evas_win, 1);
   if (fwin->win->border)
     {
	if (fwin->win->border->internal_icon)
	  eina_stringshare_del(fwin->win->border->internal_icon);
	fwin->win->border->internal_icon = 
	  eina_stringshare_add("enlightenment/fileman");
     }
   
   return fwin;
}

static void
_e_fwin_free(E_Fwin *fwin)
{
   if (!fwin) return; //safety

   if (fwin->fm_obj) evas_object_del(fwin->fm_obj);
   if (fwin->tbar) e_object_del(E_OBJECT(fwin->tbar));
   if (fwin->scrollframe_obj) evas_object_del(fwin->scrollframe_obj);
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

static void
_e_fwin_cb_delete(E_Win *win)
{
   E_Fwin *fwin;
   
   if (!win) return; //safety
   fwin = win->data;
   e_object_del(E_OBJECT(fwin));
}

static void
_e_fwin_geom_save(E_Fwin *fwin)
{
   char buf[PATH_MAX];
   E_Fm2_Custom_File *cf;

   if (!fwin->geom_save_ready) return;
   snprintf(buf, sizeof(buf), "dir::%s", e_fm2_real_path_get(fwin->fm_obj));
   cf = e_fm2_custom_file_get(buf);
   if (!cf)
     {
	cf = alloca(sizeof(E_Fm2_Custom_File));
	memset(cf, 0, sizeof(E_Fm2_Custom_File));
     }
   cf->geom.x = fwin->win->x - fwin->win->border->client_inset.l;
   cf->geom.y = fwin->win->y - fwin->win->border->client_inset.t;
   cf->geom.w = fwin->win->w;
   cf->geom.h = fwin->win->h;
   cf->geom.valid = 1;
   e_fm2_custom_file_set(buf, cf);
}

static void
_e_fwin_cb_move(E_Win *win)
{
   E_Fwin *fwin;

   if (!win) return; //safety
   fwin = win->data;
   _e_fwin_geom_save(fwin);
}

static void
_e_fwin_cb_resize(E_Win *win)
{
   E_Fwin *fwin;

   if (!win) return; //safety
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
	if (fwin->tbar)
	  _e_fwin_toolbar_resize(fwin);
	else 
	  evas_object_resize(fwin->scrollframe_obj, fwin->win->w, fwin->win->h);
     }
   else if (fwin->zone)
     evas_object_resize(fwin->scrollframe_obj, fwin->zone->w, fwin->zone->h);
   _e_fwin_geom_save(fwin);
}

static void
_e_fwin_deleted(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   
   fwin = data;
   e_object_del(E_OBJECT(fwin));
}

static const char *
_e_fwin_custom_file_path_eval(E_Fwin *fwin, Efreet_Desktop *ef, const char *prev_path, const char *key)
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
	snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->fm_obj), res);
	ret = eina_stringshare_add(buf);
     }
   return ret;
}

static void
_e_fwin_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   Efreet_Desktop *ef;
   char buf[PATH_MAX];
   
   fwin = data;
   if (!fwin) return; //safety

   /* FIXME: first look in E config for a special override for this dir's bg
    * or overlay
    */
   snprintf(buf, sizeof(buf), "%s/.directory.desktop", e_fm2_real_path_get(fwin->fm_obj));
   ef = efreet_desktop_new(buf);
   if (ef)
     {
	fwin->wallpaper_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->wallpaper_file, "X-Enlightenment-Directory-Wallpaper");
	fwin->overlay_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->overlay_file, "X-Enlightenment-Directory-Overlay");
	fwin->scrollframe_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->scrollframe_file, "X-Enlightenment-Directory-Scrollframe");
	fwin->theme_file = _e_fwin_custom_file_path_eval(fwin, ef, fwin->theme_file, "X-Enlightenment-Directory-Theme");
	// FIXME: there is no way to just unref an efreet desktop - free completely
	// frees - doesnt just unref.
 	efreet_desktop_free(ef);
     }
   if (fwin->under_obj)
     {
	evas_object_hide(fwin->under_obj);
	edje_object_file_set(fwin->under_obj, NULL, NULL);
	if (fwin->wallpaper_file)
	  edje_object_file_set(fwin->under_obj, fwin->wallpaper_file, "e/desktop/background");
	evas_object_show(fwin->under_obj);
     }
   if (fwin->over_obj)
     {
	evas_object_hide(fwin->over_obj);
	edje_object_file_set(fwin->over_obj, NULL, NULL);
	if (fwin->overlay_file)
	  edje_object_file_set(fwin->over_obj, fwin->overlay_file, "e/desktop/background");
	evas_object_show(fwin->over_obj);
     }
   if (fwin->scrollframe_obj)
     {
	if ((fwin->scrollframe_file) && 
	    (e_util_edje_collection_exists(fwin->scrollframe_file, "e/fileman/default/scrollframe")))
	  e_scrollframe_custom_edje_file_set(fwin->scrollframe_obj, 
					     (char *)fwin->scrollframe_file,
					     "e/fileman/default/scrollframe");
	else
	  {
	     if (fwin->zone)
	       e_scrollframe_custom_theme_set(fwin->scrollframe_obj,
					      "base/theme/fileman",
					      "e/fileman/desktop/scrollframe");
	     else
	       e_scrollframe_custom_theme_set(fwin->scrollframe_obj,
					      "base/theme/fileman",
					      "e/fileman/default/scrollframe");
	  }
	e_scrollframe_child_pos_set(fwin->scrollframe_obj, 0, 0);
     }
   if ((fwin->theme_file) && (ecore_file_exists(fwin->theme_file)))
     e_fm2_custom_theme_set(obj, fwin->theme_file);
   else
     e_fm2_custom_theme_set(obj, NULL);
   
   if (fwin->zone) return;
   _e_fwin_window_title_set(fwin);
}

static void
_e_fwin_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   Eina_List *selected;
   
   fwin = data;
   selected = e_fm2_selected_list_get(fwin->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(fwin, selected, 0);
   eina_list_free(selected);
}

static void
_e_fwin_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   Eina_List *l;
   E_Fwin *fwin;
   
   fwin = data;
   for (l = fwins; l; l = l->next)
     {
	if (l->data != fwin)
	  e_fwin_all_unsel(l->data);
     }
}

static void
_e_fwin_menu_extend(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info)
{
   E_Fwin *fwin;
   E_Menu_Item *mi;
   
   fwin = data;
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
_e_fwin_parent(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_fm2_parent_go(data);
}

static void
_e_fwin_cb_menu_extend_start(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info)
{
   E_Menu_Item *mi;
   E_Fwin *fwin;
   
   fwin = data;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open"));
   e_menu_item_icon_edje_set(mi,
			     e_theme_edje_file_get("base/theme/fileman",
						   "e/fileman/default/button/open"),
			     "e/fileman/default/button/open");
   e_menu_item_callback_set(mi, _e_fwin_cb_menu_open, fwin);
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Open with..."));
   e_menu_item_icon_edje_set(mi,
			     e_theme_edje_file_get("base/theme/fileman",
						   "e/fileman/default/button/open"),
			     "e/fileman/default/button/open");
   e_menu_item_callback_set(mi, _e_fwin_cb_menu_open_with, fwin);
}

static void
_e_fwin_cb_menu_open(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fwin *fwin;
   Eina_List *selected;
   
   fwin = data;
   selected = e_fm2_selected_list_get(fwin->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(fwin, selected, 0);
   eina_list_free(selected);
}

static void
_e_fwin_cb_menu_open_with(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fwin *fwin;
   Eina_List *selected = NULL;
   
   fwin = data;
   selected = e_fm2_selected_list_get(fwin->fm_obj);
   if (!selected) return;
   _e_fwin_file_open_dialog(fwin, selected, 1);
   eina_list_free(selected);
}

static void 
_e_fwin_cb_all_change(void *data, Evas_Object *obj) 
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;

   fad = data;
   E_FREE(fad->app1);
   if (fad->o_specific) e_widget_ilist_unselect(fad->o_specific);
   desktop = efreet_util_desktop_file_id_find(fad->app2);
   if ((desktop) && (desktop->exec)) 
     e_widget_entry_text_set(fad->o_entry, desktop->exec);
}

static void
_e_fwin_cb_specific_change(void *data, Evas_Object *obj)
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;

   fad = data;
   E_FREE(fad->app2);
   if (fad->o_all) e_widget_ilist_unselect(fad->o_all);
   desktop = efreet_util_desktop_file_id_find(fad->app1);
   if ((desktop) && (desktop->exec)) 
     e_widget_entry_text_set(fad->o_entry, desktop->exec);
}

static void
_e_fwin_cb_exec_cmd_changed(void *data, void *data2)
{
   E_Fwin_Apps_Dialog *fad = NULL;
   Efreet_Desktop *desktop = NULL;
   
   if (!(fad = data)) return;
   if ((!fad->app1) && (!fad->app2)) return;

   if (fad->app1) 
     desktop = efreet_util_desktop_file_id_find(fad->app1);
   else if (fad->app2) 
     desktop = efreet_util_desktop_file_id_find(fad->app2);

   if (!desktop) return;
   if (!strcmp(desktop->exec, fad->exec_cmd)) return;

   E_FREE(fad->app1);
   E_FREE(fad->app2);
   if (fad->o_specific) e_widget_ilist_unselect(fad->o_specific);
   if (fad->o_all) e_widget_ilist_unselect(fad->o_all);
}

static void
_e_fwin_cb_open(void *data, E_Dialog *dia)
{
   E_Fwin_Apps_Dialog *fad;
   Efreet_Desktop *desktop = NULL;
   char pcwd[4096], buf[4096];
   Eina_List *selected, *l;
   E_Fm2_Icon_Info *ici;
   Eina_List *files = NULL;
   char *file;
   
   fad = data;
   if (fad->app1) 
     desktop = efreet_util_desktop_file_id_find(fad->app1);
   else if (fad->app2) 
     desktop = efreet_util_desktop_file_id_find(fad->app2);

   if ((!desktop) && (!fad->exec_cmd)) return;

   if ((desktop) || (strcmp(fad->exec_cmd, "")))
     {
	getcwd(pcwd, sizeof(pcwd));
	chdir(e_fm2_real_path_get(fad->fwin->fm_obj));

	selected = e_fm2_selected_list_get(fad->fwin->fm_obj);
	if (selected)
	  {
	     files = NULL;
	     for (l = selected; l; l = l->next)
	       {
		  E_Fwin_Exec_Type ext;
		  
		  ici = l->data;
		  /* this snprintf is silly - but it's here in case i really do
		   * need to provide full paths (seems silly since we chdir
		   * into the dir)
		   */
		  buf[0] = 0;
		  ext = _e_fwin_file_is_exec(ici);
		  if (ext == E_FWIN_EXEC_NONE)
		    {
		       if (!((ici->link) && (ici->mount)))
			 {
			    if (ici->link)
			      {
				 if (!S_ISDIR(ici->statinfo.st_mode))
				   snprintf(buf, sizeof(buf), "%s", ici->file);
			      }
			    else
			      {
				 if (!S_ISDIR(ici->statinfo.st_mode))
				   snprintf(buf, sizeof(buf), "%s", ici->file);
			      }
			 }
		    }
		  else
		    _e_fwin_file_exec(fad->fwin, ici, ext);
		  if (buf[0] != 0)
		    {
		       if (ici->mime && desktop)
			 e_exehist_mime_desktop_add(ici->mime, desktop);
		       files = eina_list_append(files, strdup(ici->file));
		    }
	       }
	     eina_list_free(selected);
	     
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

	     if (fad->fwin->win)
	       {
		  if (desktop)
                    e_exec(fad->fwin->win->border->zone, desktop, NULL, files,
                           "fwin");
	       }
	     else if (fad->fwin->zone)
	       {
		  if (desktop)
                    e_exec(fad->fwin->zone, desktop, NULL, files, "fwin");
	       }

	     // Free fake .desktop
	     if (!strcmp(fad->exec_cmd, ""))
               efreet_desktop_free(desktop);

	     EINA_LIST_FREE(files, file)
	       free(file);
	  }
	chdir(pcwd);
     }
   e_object_del(E_OBJECT(fad->dia));
}
    
static void
_e_fwin_cb_close(void *data, E_Dialog *dia)
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
   E_FREE(fad->app1);
   E_FREE(fad->app2);
   E_FREE(fad->exec_cmd);
   fad->fwin->fad = NULL;
   E_FREE(fad);
}

static Eina_Bool
_e_fwin_cb_hash_foreach(const Eina_Hash *hash __UNUSED__, const void *key, void *data __UNUSED__, void *fdata)
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
_e_fwin_file_exec(E_Fwin *fwin, E_Fm2_Icon_Info *ici, E_Fwin_Exec_Type ext)
{
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
	snprintf(buf, sizeof(buf), "%s/%s", e_fm2_real_path_get(fwin->fm_obj), ici->file);
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
_e_fwin_file_open_dialog(E_Fwin *fwin, Eina_List *files, int always)
{
   E_Fwin *fwin2 = NULL;
   E_Dialog *dia;
   Evas_Coord mw, mh;
   Evas_Object *o, *of, *oi, *ot;
   Evas *evas;
   Eina_List *l = NULL, *ll, *apps = NULL, *mlist = NULL;
   Eina_List *ml;
   Eina_List *cats = NULL;
   Eina_Hash *mimes = NULL;
   Efreet_Desktop *desk = NULL;
   E_Fwin_Apps_Dialog *fad;
   E_Fm2_Icon_Info *ici;
   char buf[PATH_MAX];
   const char *f;
   char *mime;
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
		       e_fm2_path_set(fwin->fm_obj, ici->link, "/");
		       _e_fwin_window_title_set(fwin);
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
		       e_fm2_path_set(fwin->fm_obj, buf, "/");
		       _e_fwin_window_title_set(fwin);
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
			    e_fm2_path_set(fwin->fm_obj, NULL, ici->real_link);
			    _e_fwin_window_title_set(fwin);
			 }
		    }
		  else
		    need_dia = 1;
	       }
	     else
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", 
			   e_fm2_real_path_get(fwin->fm_obj), ici->file);
		  if (S_ISDIR(ici->statinfo.st_mode))
		    {
		       if ((!fileman_config->view.open_dirs_in_place) || (fwin->zone)) 
			 {
			    if (fwin->win)
			      fwin2 = _e_fwin_new(fwin->win->container, NULL, buf);
			    else
			      fwin2 = _e_fwin_new(fwin->zone->container, NULL, buf);
			 }
		       else 
			 {
			    e_fm2_path_set(fwin->fm_obj, NULL, buf);
			    _e_fwin_window_title_set(fwin);
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
		       int ix, iy, iw, ih, nx, ny, nw, nh;

		       oic = e_fm2_icon_get(evas_object_evas_get(fwin->fm_obj),
					    ici->ic, NULL, NULL, 0, &itype);
		       if (oic)
			 {
			    const char *file = NULL, *group = NULL;
			    E_Fm2_Custom_File *cf;
			    
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
			    
			    snprintf(buf, sizeof(buf), "dir::%s",
				     e_fm2_real_path_get(fwin2->fm_obj));
			    cf = e_fm2_custom_file_get(buf);
			    if ((cf) && (cf->geom.valid))
			      {
				 nx = cf->geom.x;
				 ny = cf->geom.y;
				 nw = cf->geom.w;
				 nh = cf->geom.h;
				 /* if it ended up too small - fix to a decent size  */
				 if (nw < 24) nw = 200 * e_scale;
				 if (nh < 24) nh = 280 * e_scale;
				 printf("load @ %i %i, %ix%i inset %i %i\n",
					nx, ny, nw, nh,
					fwin2->win->border->client_inset.l,
					fwin2->win->border->client_inset.t);
				 /* if it ended up out of the zone */
				 if (nx < fwin2->win->border->zone->x)
				   nx = fwin2->win->border->zone->x +
				   fwin2->win->border->client_inset.l;
				 if (ny < fwin2->win->border->zone->y)
				   ny = fwin2->win->border->zone->y + 
				   fwin2->win->border->client_inset.t;
				 if ((fwin2->win->border->zone->x + 
				      fwin2->win->border->zone->w) <
				     (fwin2->win->border->w + nx))
				   nx = fwin2->win->border->zone->x + 
				   fwin2->win->border->zone->w - 
				   fwin2->win->border->w - 
				   fwin2->win->border->client_inset.l;
				 if ((fwin2->win->border->zone->y + 
				      fwin2->win->border->zone->h) <
				     (fwin2->win->border->h + ny))
				   ny = fwin2->win->border->zone->y + 
				   fwin2->win->border->zone->h - 
				   fwin2->win->border->h - 
				   fwin2->win->border->client_inset.t;
				 e_win_move_resize
				   (fwin2->win, 
				    nx - fwin2->win->border->client_inset.l, 
				    ny - fwin2->win->border->client_inset.t,
				    nw, nh);
			      }
			    else
			      {
				 /* No custom info, so just put window near icon */
				 e_fm2_icon_geometry_get(ici->ic, &ix, &iy, &iw, &ih);
				 nx = (ix + (iw / 2));
				 ny = (iy + (ih / 2));
				 if (fwin->win) 
				   {
				      nx += fwin->win->x;
				      ny += fwin->win->y;
				   }
				 /* iff going out of zone - adjust to be in */
				 if ((fwin2->win->border->zone->x + 
				      fwin2->win->border->zone->w) <
				     (fwin2->win->border->w + nx))
				   nx -= fwin2->win->border->w;
				 if ((fwin2->win->border->zone->y + 
				      fwin2->win->border->zone->h) <
				     (fwin2->win->border->h + ny))
				   ny -= fwin2->win->border->h;
				 e_win_move(fwin2->win, nx, ny);
			      }
			 }
		       fwin2->geom_save_ready = 1;
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
   
   /* 1. build hash of mimetypes */
   EINA_LIST_FOREACH(files, l, ici)
        if (!((ici->link) && (ici->mount)))
	  {
	     if (_e_fwin_file_is_exec(ici) == E_FWIN_EXEC_NONE)
	       {
		  if (ici->link)
		    {
		      f = e_fm_mime_filename_get(ici->link);
		      if (!mimes)
			mimes = eina_hash_string_superfast_new(NULL);
		      eina_hash_del(mimes, f, (void *)1);
		      eina_hash_direct_add(mimes, f, (void *)1);
		    }
		  else
		    {
		      snprintf(buf, sizeof(buf), "%s/%s",
			       e_fm2_real_path_get(fwin->fm_obj), ici->file);
		      if (!mimes)
			mimes = eina_hash_string_superfast_new(NULL);
		      eina_hash_del(mimes, ici->mime, (void *)1);
		      eina_hash_direct_add(mimes, ici->mime, (void *)1);
		    }
	       }
	  }
   /* 2. for each mimetype list apps that handle it */
   if (mimes)
     {
	eina_hash_foreach(mimes, _e_fwin_cb_hash_foreach, &mlist);
	eina_hash_free(mimes);
     }
   /* 3. add apps to a list so its a unique app list */
   apps = NULL;
   EINA_LIST_FOREACH(mlist, l, mime)
	  {
	ml = efreet_util_desktop_mime_list(mime);
	apps = eina_list_merge(apps, ml);
     }

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
	     chdir(e_fm2_real_path_get(fwin->fm_obj));
	     
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
		       _e_fwin_file_exec(fwin, ici, ext);
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
   else return; /* make clang happy */

   e_dialog_title_set(dia, _("Open with..."));
   e_dialog_resizable_set(dia, 1);
   e_dialog_button_add(dia, _("Open"), "widget/open",
		       _e_fwin_cb_open, fad);
   e_dialog_button_add(dia, _("Close"), "widget/close",
		       _e_fwin_cb_close, fad);

   fad->dia = dia;
   fad->fwin = fwin;
   fwin->fad = fad;
   dia->data = fad;
   e_object_free_attach_func_set(E_OBJECT(dia), _e_fwin_cb_dialog_free);
   
   evas = e_win_evas_get(dia->win);
   
   ot = e_widget_table_add(evas, 0);
   if (apps)
     {
	of = e_widget_framelist_add(evas, _("Specific Applications"), 0);
	o = e_widget_ilist_add(evas, 24, 24, &(fad->app1));
	e_widget_on_change_hook_set(o, _e_fwin_cb_specific_change, fad);
	evas_event_freeze(evas);
	edje_freeze();
	e_widget_ilist_freeze(o);
	fad->o_specific = o;
	for (l = apps; l; l = l->next)
	  {
	     desk = l->data;
	     oi = e_util_desktop_icon_add(desk, 24, evas);
	     e_widget_ilist_append(o, oi, desk->name, NULL, NULL, 
				   efreet_util_path_to_file_id(desk->orig_path));
	  }
	e_widget_ilist_go(o);
	e_widget_ilist_thaw(o);
	edje_thaw();
	evas_event_thaw(evas);
	e_widget_min_size_set(o, 160, 240);
	e_widget_framelist_object_append(of, o);
	e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
     }

   l = eina_list_free(l);

   of = e_widget_framelist_add(evas, _("All Applications"), 0);
   o = e_widget_ilist_add(evas, 24, 24, &(fad->app2));
   e_widget_on_change_hook_set(o, _e_fwin_cb_all_change, fad);
   fad->o_all = o;
   evas_event_freeze(evas);
   edje_freeze();
   e_widget_ilist_freeze(o);
   cats = efreet_util_desktop_name_glob_list("*");
   cats = eina_list_sort(cats, 0, _e_fwin_dlg_cb_desk_sort);
   EINA_LIST_FREE(cats, desk)
	if (!eina_list_data_find(l, desk))
	  l = eina_list_append(l, desk);
   l = eina_list_sort(l, -1, _e_fwin_dlg_cb_desk_list_sort);

   EINA_LIST_FREE(l, desk)
     {
	Evas_Object *icon = NULL;

	if (!desk) continue;
	icon = e_util_desktop_icon_add(desk, 24, evas);
	e_widget_ilist_append(o, icon, desk->name, NULL, NULL, 
			      efreet_util_path_to_file_id(desk->orig_path));
     }

   e_widget_ilist_go(o);
   e_widget_ilist_thaw(o);
   edje_thaw();
   evas_event_thaw(evas);
   e_widget_min_size_set(o, 160, 240);
   e_widget_framelist_object_append(of, o);
   if (apps) 
     {
	e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
	eina_list_free(apps);
     }
   else
     e_widget_table_object_append(ot, of, 0, 0, 2, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Custom Command"));
   e_widget_table_object_append(ot, o, 0, 1, 1, 1, 1, 1, 1, 0);
   fad->o_entry = e_widget_entry_add(evas, &(fad->exec_cmd), 
				     _e_fwin_cb_exec_cmd_changed, fad, NULL);
   e_widget_table_object_append(ot, fad->o_entry, 0, 2, 2, 1, 1, 1, 1, 0);

   e_widget_min_size_get(ot, &mw, &mh);
   e_dialog_content_set(dia, ot, mw, mh);
   e_dialog_show(dia);
   e_dialog_border_icon_set(dia, "enlightenment/applications");
}

static void
_e_fwin_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_set(obj, x, y);
   if (x > fwin->fm_pan.max_x) x = fwin->fm_pan.max_x;
   if (y > fwin->fm_pan.max_y) y = fwin->fm_pan.max_y;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   fwin->fm_pan.x = x;
   fwin->fm_pan.y = y;
   _e_fwin_pan_scroll_update(fwin);
}

static void
_e_fwin_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_get(obj, x, y);
   fwin->fm_pan.x = *x;
   fwin->fm_pan.y = *y;
}

static void
_e_fwin_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_max_get(obj, x, y);
   fwin->fm_pan.max_x = *x;
   fwin->fm_pan.max_y = *y;
   _e_fwin_pan_scroll_update(fwin);
}

static void
_e_fwin_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Fwin *fwin;
   
   fwin = evas_object_data_get(obj, "fwin");
   e_fm2_pan_child_size_get(obj, w, h);
   fwin->fm_pan.w = *w;
   fwin->fm_pan.h = *h;
   _e_fwin_pan_scroll_update(fwin);
}

static void
_e_fwin_pan_scroll_update(E_Fwin *fwin)
{
   Edje_Message_Int_Set *msg;
 
   if ((fwin->fm_pan.x == fwin->fm_pan_last.x) &&
       (fwin->fm_pan.y == fwin->fm_pan_last.y) &&
       (fwin->fm_pan.max_x == fwin->fm_pan_last.max_x) &&
       (fwin->fm_pan.max_y == fwin->fm_pan_last.max_y) &&
       (fwin->fm_pan.w == fwin->fm_pan_last.w) &&
       (fwin->fm_pan.h == fwin->fm_pan_last.h)) return;
   msg = alloca(sizeof(Edje_Message_Int_Set) -
		sizeof(int) + (6 * sizeof(int)));
   msg->count = 6;
   msg->val[0] = fwin->fm_pan.x;
   msg->val[1] = fwin->fm_pan.y;
   msg->val[2] = fwin->fm_pan.max_x;
   msg->val[3] = fwin->fm_pan.max_y;
   msg->val[4] = fwin->fm_pan.w;
   msg->val[5] = fwin->fm_pan.h;
//   printf("SEND MSG %i %i | %i %i | %ix%i\n",
//	  fwin->fm_pan.x, fwin->fm_pan.y,
//	  fwin->fm_pan.max_x, fwin->fm_pan.max_y,
//	  fwin->fm_pan.w, fwin->fm_pan.h);
   if (fwin->under_obj)
     edje_object_message_send(fwin->under_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (fwin->over_obj)
     edje_object_message_send(fwin->over_obj, EDJE_MESSAGE_INT_SET, 1, msg);
   if (fwin->scrollframe_obj)
     edje_object_message_send(e_scrollframe_edje_object_get(fwin->scrollframe_obj), EDJE_MESSAGE_INT_SET, 1, msg);
   fwin->fm_pan_last.x = fwin->fm_pan.x;
   fwin->fm_pan_last.y = fwin->fm_pan.y;
   fwin->fm_pan_last.max_x = fwin->fm_pan.max_x;
   fwin->fm_pan_last.max_y = fwin->fm_pan.max_y;
   fwin->fm_pan_last.w = fwin->fm_pan.w;
   fwin->fm_pan_last.h = fwin->fm_pan.h;
}

static void 
_e_fwin_zone_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Fwin *fwin;
   
   fwin = data;
   if (!fwin) return;
   e_fwin_all_unsel(fwin);
}

static int 
_e_fwin_zone_move_resize(void *data, int type, void *event) 
{
   E_Event_Zone_Move_Resize *ev;
   E_Fwin *fwin;
   
   if (type != E_EVENT_ZONE_MOVE_RESIZE) return 1;
   fwin = data;
   ev = event;
   if (!fwin) return 1;
   if (fwin->zone != ev->zone) return 1;
   if (fwin->bg_obj) 
     {
	evas_object_move(fwin->bg_obj, ev->zone->x, ev->zone->y);
	evas_object_resize(fwin->bg_obj, ev->zone->w, ev->zone->h);
     }
   if (fwin->scrollframe_obj) 
     {
	evas_object_move(fwin->scrollframe_obj, ev->zone->x, ev->zone->y);
	evas_object_resize(fwin->scrollframe_obj, ev->zone->w, ev->zone->h);
     }
   return 1;
}

static int 
_e_fwin_zone_del(void *data, int type, void *event) 
{
   E_Event_Zone_Del *ev;
   E_Fwin *fwin;
   
   if (type != E_EVENT_ZONE_DEL) return 1;
   fwin = data;
   ev = event;
   if (!fwin) return 1;
   if (fwin->zone != ev->zone) return 1;
   e_object_del(E_OBJECT(fwin));
   return 1;
}

static void 
_e_fwin_config_set(E_Fwin *fwin) 
{
   E_Fm2_Config fmc;

   memset(&fmc, 0, sizeof(E_Fm2_Config));
   if (!fwin->zone) 
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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   fmc.list.sort.dirs.last = fileman_config->list.sort.dirs.last;
   fmc.selection.single = 0;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(fwin->fm_obj, &fmc);
}

static void 
_e_fwin_window_title_set(E_Fwin *fwin) 
{
   char buf[4096];
   const char *file;
   
   if (!fwin) return;
   if (fwin->zone) return; //safety

   if (fileman_config->view.show_full_path) 
     file = e_fm2_real_path_get(fwin->fm_obj);
   else
     file = ecore_file_file_get(e_fm2_real_path_get(fwin->fm_obj));
   
   if (file) 
     {
	snprintf(buf, sizeof(buf), "%s", file);
	e_win_title_set(fwin->win, buf);
     }
}

static void 
_e_fwin_toolbar_resize(E_Fwin *fwin) 
{
   int x, y, w, h;

   e_toolbar_position_calc(fwin->tbar);
   w = fwin->win->w;
   h = fwin->win->h;
   switch (fwin->tbar->gadcon->orient) 
     {
      case E_GADCON_ORIENT_TOP:
	x = 0;
	y = fwin->tbar->h;
	h = (h - fwin->tbar->h);
	break;
      case E_GADCON_ORIENT_BOTTOM:
	x = 0;
	y = 0;
	h = (h - fwin->tbar->h);
	break;
      case E_GADCON_ORIENT_LEFT:
	x = (fwin->tbar->x + fwin->tbar->w);
	y = 0;
	w = (w - fwin->tbar->w);
	break;
      case E_GADCON_ORIENT_RIGHT:
	x = 0;
	y = 0;
	w = (fwin->win->w - fwin->tbar->w);
	break;
      default:
	return;
     }
   evas_object_move(fwin->scrollframe_obj, x, y);
   evas_object_resize(fwin->scrollframe_obj, w, h);
}

static int 
_e_fwin_dlg_cb_desk_sort(Efreet_Desktop *d1, Efreet_Desktop *d2) 
{
   if (!d1->name) return 1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

static int 
_e_fwin_dlg_cb_desk_list_sort(const void *data1, const void *data2) 
{
   const Efreet_Desktop *d1, *d2;

   if (!(d1 = data1)) return 1;
   if (!(d2 = data2)) return -1;
   return strcmp(d1->name, d2->name);
}

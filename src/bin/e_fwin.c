/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: fwin - he fm2 filemanager wrapped with a window and scrollframe.
 * primitive BUT enough to test generic dnd and fm stuff more easily. don't
 * play with this unless u want to help with it. NOT COMPLETE! BEWARE!
 */

/* local subsystem functions */
static void _e_fwin_free(E_Fwin *fwin);
static void _e_fwin_cb_delete(E_Win *win);
static void _e_fwin_cb_resize(E_Win *win);
static void _e_fwin_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_fwin_selected(void *data, Evas_Object *obj, void *event_info);

/* local subsystem globals */
static Evas_List *fwins = NULL;

/* externally accessible functions */
EAPI int
e_fwin_init(void)
{
   return 1;
}

EAPI int
e_fwin_shutdown(void)
{
   return 1;
}

EAPI E_Fwin *
e_fwin_new(E_Container *con, const char *dev, const char *path)
{
   E_Fwin *fwin;
   char buf[4096], *file;
   Evas_Object *o;
   E_Fm2_Config fmc;
   
   fwin = E_OBJECT_ALLOC(E_Fwin, E_FWIN_TYPE, _e_fwin_free);
   if (!fwin) return NULL;
   fwin->win = e_win_new(con);
   if (!fwin->win)
     {
	free(fwin);
	return NULL;
     }
   fwins = evas_list_append(fwins, fwin);
   e_win_delete_callback_set(fwin->win, _e_fwin_cb_delete);
   e_win_resize_callback_set(fwin->win, _e_fwin_cb_resize);
   fwin->win->data = fwin;

   /* fm issues: */
   /* FIXME: need a way of going to parent dir (menu extn.) */
   /* FIXME: bug: drop on file on another dir doesnt do drop all */
   /* FIXME: bug: on shift-click then drag - don't deselect */
   /* FIXME: bug: drag multiple files doesnt work */
   
   /* FIXME: temporary - a white bg until we have a proper fm specific
    * scrollframe etc.
    */
   o = evas_object_rectangle_add(e_win_evas_get(fwin->win));
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_show(o);
   fwin->bg_obj = o;
   
   o = e_fm2_add(e_win_evas_get(fwin->win));
   fwin->fm_obj = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 0;
   fmc.view.selector = 0;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 1;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 0;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _e_fwin_changed, fwin);
   evas_object_smart_callback_add(o, "selected",
				  _e_fwin_selected, fwin);
   e_fm2_path_set(o, dev, path);
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   o = e_widget_scrollframe_pan_add(e_win_evas_get(fwin->win), fwin->fm_obj,
				    e_fm2_pan_set,
				    e_fm2_pan_get,
				    e_fm2_pan_max_get,
				    e_fm2_pan_child_size_get);
   evas_object_propagate_events_set(fwin->fm_obj, 0);
   e_widget_scrollframe_focus_object_set(o, fwin->fm_obj);
   fwin->scrollframe_obj = o;
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   e_fm2_window_object_set(fwin->fm_obj, E_OBJECT(fwin->win));
   
   e_widget_focus_set(fwin->scrollframe_obj, 1);
   
   snprintf(buf, sizeof(buf), "_fwin::/%s", e_fm2_real_path_get(fwin->fm_obj));
   e_win_name_class_set(fwin->win, "E", buf);
   file = ecore_file_get_file(e_fm2_real_path_get(fwin->fm_obj));
   if (file)
     snprintf(buf, sizeof(buf), "%s", file);
   else
     snprintf(buf, sizeof(buf), "%s", e_fm2_real_path_get(fwin->fm_obj));
   e_win_title_set(fwin->win, buf);
   e_win_size_min_set(fwin->win, 24, 24);
   e_win_resize(fwin->win, 280, 200);
   e_win_show(fwin->win);
   
   return fwin;
}

/* local subsystem functions */
static void
_e_fwin_free(E_Fwin *fwin)
{
   e_object_del(E_OBJECT(fwin->win));
   fwins = evas_list_remove(fwins, fwin);
   free(fwin);
}

static void
_e_fwin_cb_delete(E_Win *win)
{
   E_Fwin *fwin;
   
   fwin = win->data;
   e_object_del(E_OBJECT(fwin));
}

static void
_e_fwin_cb_resize(E_Win *win)
{
   E_Fwin *fwin;
   
   fwin = win->data;
   evas_object_resize(fwin->bg_obj, fwin->win->w, fwin->win->h);
   evas_object_resize(fwin->scrollframe_obj, fwin->win->w, fwin->win->h);
}

static void
_e_fwin_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   
   fwin = data;
   if (fwin->scrollframe_obj)
     e_widget_scrollframe_child_pos_set(fwin->scrollframe_obj, 0, 0);
}

static void
_e_fwin_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Fwin *fwin;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   char buf[4096];
   const *rp;
   
   fwin = data;
   selected = e_fm2_selected_list_get(fwin->fm_obj);
   if (!selected) return;
   ici = selected->data;
   if ((ici->link) && (ici->mount))
     e_fwin_new(fwin->win->container, ici->link, "/");
   else if (ici->link)
     e_fwin_new(fwin->win->container, NULL, ici->link);
   else
     {
	snprintf(buf, sizeof(buf), "%s/%s", 
		 e_fm2_real_path_get(fwin->fm_obj), ici->file);
	e_fwin_new(fwin->win->container, NULL, buf);
     }
   evas_list_free(selected);
}

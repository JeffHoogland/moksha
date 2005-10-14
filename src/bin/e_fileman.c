/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/****
 * TODO:
 * - dont allow for menus on ".."
 * - add scrollers.
 * - xdnd
 * - thumb fork / cache
 * - proper mime system
 * - create x, y, w, h in canvas struct and make them auto update
 ****/

static void _e_fileman_vscrollbar_drag_cb(Evas_Object *object, double value, void *data);
static void _e_fileman_reconf_cb(void *data, Evas_Object *obj, E_Fm_Event_Reconfigure *ev);
static void _e_fileman_cb_resize(E_Win *win);
static void _e_fileman_cb_delete(E_Win *win);
static void _e_fileman_free(E_Fileman *fileman);

E_Fileman *
e_fileman_new(E_Container *con)
{
   E_Fileman *fileman;
   E_Manager *man;
   char dir[PATH_MAX];

   if (!con)
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con) con = e_container_number_get(man, 0);
	if (!con) return NULL;
     }
   if (!getcwd(dir, sizeof(dir)))
     return NULL;

   fileman = E_OBJECT_ALLOC(E_Fileman, E_FILEMAN_TYPE, _e_fileman_free);
   if (!fileman) return NULL;

   fileman->con = con;
   e_object_ref(E_OBJECT(fileman->con));
   fileman->win = e_win_new(con);

   if (!fileman->win)
     {
	free(fileman);
	return NULL;
     }

   fileman->xpos = 0;
   fileman->ypos = 0;

   e_win_delete_callback_set(fileman->win, _e_fileman_cb_delete);
   fileman->win->data = fileman;

   fileman->evas = e_win_evas_get(fileman->win);

   e_win_name_class_set(fileman->win, "Efm ", "_fileman");
   e_win_title_set(fileman->win, dir);

   fileman->main = edje_object_add(fileman->evas);
   e_theme_edje_object_set(fileman->main, "base/theme/fileman/main",
			   "fileman/main");

   fileman->vscrollbar = e_scrollbar_add(fileman->evas);
   e_scrollbar_direction_set(fileman->vscrollbar, E_SCROLLBAR_VERTICAL);
   e_scrollbar_callback_drag_add(fileman->vscrollbar, _e_fileman_vscrollbar_drag_cb, fileman);

   e_win_resize_callback_set(fileman->win, _e_fileman_cb_resize);
   e_win_resize(fileman->win, 640, 480);

   fileman->smart = e_fm_add(fileman->evas);
   e_fm_e_win_set(fileman->smart, fileman->win);
   //e_fm_dir_set(fileman->smart, dir);
   edje_object_part_swallow(fileman->main, "icon_area", fileman->smart);

   ecore_x_dnd_aware_set(fileman->win->evas_win, 1);

   e_fm_reconfigure_callback_add(fileman->smart, _e_fileman_reconf_cb, fileman);
   
   return fileman;
}

void
e_fileman_show(E_Fileman *fileman)
{
   if (!fileman) return;

   e_win_show(fileman->win);
   evas_object_show(fileman->main);
}

void
e_fileman_hide(E_Fileman *fileman)
{
   if (!fileman) return;

   e_win_hide(fileman->win);
   evas_object_hide(fileman->main);
}

static void
_e_fileman_free(E_Fileman *fileman)
{
   e_object_unref(E_OBJECT(fileman->con));
   evas_object_del(fileman->vscrollbar);
   evas_object_del(fileman->smart);
   evas_object_del(fileman->main);
   e_object_del(E_OBJECT(fileman->win));
   free(fileman);
}

static void
_e_fileman_cb_resize(E_Win *win)
{
   E_Fileman *fileman;
   Evas_Coord w, h;
   
   fileman = win->data;
   evas_object_resize(fileman->main, win->w, win->h);
   e_fm_geometry_virtual_get(fileman->smart, &w, &h);
   if(h > win->h)
     edje_object_part_swallow(fileman->main, "vscrollbar", fileman->vscrollbar);
   else 
    {
       edje_object_part_unswallow(fileman->main, fileman->vscrollbar);
       evas_object_hide(fileman->vscrollbar);
    }
}

static void
_e_fileman_cb_delete(E_Win *win)
{
   E_Fileman *fileman;

   fileman = win->data;
   e_object_del(E_OBJECT(fileman));
   e_object_del(E_OBJECT(win));
}

static void
_e_fileman_vscrollbar_drag_cb(Evas_Object *object, double value, void *data)
{
   E_Fileman *fileman;

   fileman = data;
   e_fm_scroll_vertical(fileman->smart, value);
}

static void
_e_fileman_reconf_cb(void *data, Evas_Object *obj, E_Fm_Event_Reconfigure *ev)
{   
   E_Fileman *fileman;
   
   fileman = data;
   
   if(ev->h > fileman->win->h)
     edje_object_part_swallow(fileman->main, "vscrollbar", fileman->vscrollbar);
   else {
      edje_object_part_unswallow(fileman->main, fileman->vscrollbar);
      evas_object_hide(fileman->vscrollbar);
   }
}

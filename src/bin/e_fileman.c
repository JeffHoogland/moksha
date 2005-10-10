#include "e.h"

/****
 * TODO:
 * - dont allow for menus on ".."
 * - add scrollers.
 * - xdnd
 * - thumb fork / cache
 * - proper mime system
 * - create x, y, w, h in canvas struct and make them auto update
 * - get_current_dir_name() will give a memleak
 ****/

static void _e_fileman_hscrollbar_drag_cb (Evas_Object *object, double value, void *data);
static void _e_fileman_vscrollbar_drag_cb (Evas_Object *object, double value, void *data);
static void _e_fileman_cb_resize (E_Win *win);    
static void _e_fileman_cb_delete(E_Win *win);    
static void _e_fileman_free (E_Fileman *fileman);
  
static void
_e_fileman_cb_resize (E_Win *win)
{
   E_Fileman *fileman;
      
   fileman = win->data;   
   evas_object_resize (fileman->main, win->w, win->h);
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
_e_fileman_hscrollbar_drag_cb (Evas_Object *object, double value, void *data)
{
   E_Fileman *fileman;
   
   fileman = data;   
}

static void 
_e_fileman_vscrollbar_drag_cb (Evas_Object *object, double value, void *data)
{
   E_Fileman *fileman;
   
   fileman = data;   
   e_fm_scroll_vertical (fileman->smart, value);   
}

E_Fileman *
e_fileman_new (E_Container *con)
{
   E_Fileman *fileman;
   E_Manager *man;   
   char *dir;
   
   if (!con)
   {
      man = e_manager_current_get();
      if (!man) return NULL;
      con = e_container_current_get(man);
      if (!con) con = e_container_number_get(man, 0);
      if (!con) return NULL;
   }   
   
   fileman = E_OBJECT_ALLOC (E_Fileman, E_FILEMAN_TYPE, _e_fileman_free);
   if (!fileman) return NULL;

   fileman->con = con;
   fileman->win = e_win_new (con);
   
   if (!fileman->win)
   {
      free (fileman);
      return NULL;
   }

   fileman->xpos = 0;
   fileman->ypos = 0;      
   
   e_win_delete_callback_set(fileman->win, _e_fileman_cb_delete);
   fileman->win->data = fileman;
   
   fileman->evas = e_win_evas_get(fileman->win);
      
   dir = get_current_dir_name();
   e_win_name_class_set(fileman->win, "Efm ", "_fileman");
   e_win_title_set (fileman->win, dir);
   
   fileman->main = edje_object_add (fileman->evas);
   e_theme_edje_object_set(fileman->main, "base/theme/fileman/main",
			   "fileman/main");      
   
   fileman->hscrollbar = e_scrollbar_add (fileman->evas);
   e_scrollbar_callback_drag_add (fileman->hscrollbar, _e_fileman_hscrollbar_drag_cb, fileman);
   edje_object_part_swallow (fileman->main, "hscrollbar", fileman->hscrollbar);
   
   fileman->vscrollbar = e_scrollbar_add (fileman->evas);
   e_scrollbar_direction_set (fileman->vscrollbar, E_SCROLLBAR_VERTICAL);
   e_scrollbar_callback_drag_add (fileman->vscrollbar, _e_fileman_vscrollbar_drag_cb, fileman);
   edje_object_part_swallow (fileman->main, "vscrollbar", fileman->vscrollbar);
   
   e_win_resize_callback_set (fileman->win, _e_fileman_cb_resize);
   e_win_resize (fileman->win, 640, 480);
   
   fileman->smart = e_fm_add(fileman->evas);
   e_fm_e_win_set (fileman->smart, fileman->win); 
   e_fm_dir_set (fileman->smart, dir);
   free(dir);
   edje_object_part_swallow (fileman->main, "icon_area", fileman->smart);   
   
   ecore_x_dnd_aware_set(fileman->win->evas_win, 1);
      
   return fileman;   
}

static void
_e_fileman_free(E_Fileman *fileman)
{   
   evas_object_del(fileman->hscrollbar);
   evas_object_del(fileman->vscrollbar);
   evas_object_del(fileman->smart);
   evas_object_del(fileman->main);   
   e_object_del(E_OBJECT(fileman->win));           
   free(fileman);
}


void
e_fileman_show (E_Fileman *fileman)
{
   if (fileman) {
      e_win_show(fileman->win);      
      evas_object_show(fileman->main);      
   }
}
   

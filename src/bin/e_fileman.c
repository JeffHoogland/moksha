/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* e_fileman.c
 * ===========
 *
 * fileman is the FILE MANager for e.
 * its composed of several entities:
 * (check each .c for more reference)
 *
 * +-------------+-+
 * | x  x  x  x  |s|
 * |             | |
 * | x  x  x  x  | |
 * |            o| |
 * | x  x  x  x  | |
 * +-------------+-+
 * l-s-----------i-i
 * 
 *
 * graphically:
 * o = icon_canvas
 * x = icon
 * s = scrollframe
 *
 * internally:
 * a fileman has a fileman_smart. 
 * 
 * fileman_smart
 * ============= 
 * has a list of fileman_icon of that directory (files).
 * is created above an icon_canvas
 * has a list of fileman_file of that directory?
 * has a list of icons
 * 
 * fileman_icon
 * ============
 * a fileman_icon has a fileman_file associated to it.
 * 
 * fileman_file
 * ============
 * a fileman_file has a fileman_mime_entry
 * 
 * fileman_mime
 * ============
 * a fileman_mime_entry has a list of fileman_mime_actions
 * 
 */

#include "e.h"

/****
 * TODO:
 * - reset scrollbar positions on dir changes
 * - dont allow for menus on ".."
 * - xdnd
 * - proper mime system
 * - create x, y, w, h in canvas struct and make them auto update
 ****/

#ifdef EFM_DEBUG
# define D(x)  do {printf(__FILE__ ":%d:  ", __LINE__); printf x; fflush(stdout);} while (0)
#else
# define D(x)  ((void) 0)
#endif

static void _e_fileman_resize_cb(E_Win *win);
static void _e_fileman_delete_cb(E_Win *win);
static void _e_fileman_selector_cb(Evas_Object *object, char *file, void *data);
static void _e_fileman_free(E_Fileman *fileman);
static void _e_fileman_scroll_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_fileman_scroll_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fileman_scroll_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static void _e_fileman_scroll_child_size_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
static int  _e_fileman_reconfigure_cb(void *data, int type, void *event);

EAPI E_Fileman *
e_fileman_new(E_Container *con)
{
   char dir[PATH_MAX];
   
   if (!getcwd(dir, sizeof(dir)))
     return NULL;
   
   return e_fileman_new_to_dir(con, dir);
}

EAPI E_Fileman *
e_fileman_new_to_dir(E_Container *con, char *path)
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

   snprintf(dir, PATH_MAX, "%s", path);
   
   if(!ecore_file_is_dir(dir))
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

   e_win_delete_callback_set(fileman->win, _e_fileman_delete_cb);
   fileman->win->data = fileman;

   fileman->evas = e_win_evas_get(fileman->win);

   e_win_name_class_set(fileman->win, "Efm ", "_fileman");
   e_win_title_set(fileman->win, dir);

   evas_event_freeze(fileman->evas);   
   fileman->smart = e_fm_add(fileman->evas);
   e_fm_e_win_set(fileman->smart, fileman->win);
   
   fileman->main = e_scrollframe_add(fileman->evas);
   e_scrollframe_custom_theme_set(fileman->main, "base/themes/fileman",
				  "fileman/main");
   e_scrollframe_extern_pan_set(fileman->main, fileman->smart, 
				_e_fileman_scroll_set,
				_e_fileman_scroll_get,
				_e_fileman_scroll_max_get,
				_e_fileman_scroll_child_size_get);

   e_win_resize_callback_set(fileman->win, _e_fileman_resize_cb);
   e_win_resize(fileman->win, 570, 355);
   
   e_fm_dir_set(fileman->smart, dir);
   
   ecore_x_dnd_aware_set(fileman->win->evas_win, 1);
   
   evas_event_thaw(fileman->evas);
   
   evas_object_focus_set(fileman->main, 0);
   evas_object_focus_set(fileman->smart, 1);
   evas_object_propagate_events_set(fileman->smart, 0);
   
   fileman->event_handlers = evas_list_append(fileman->event_handlers,
					      ecore_event_handler_add(E_EVENT_FM_RECONFIGURE,
								      _e_fileman_reconfigure_cb,
								      fileman));
   
   
   D(("e_fileman_new: ok\n"));
   
   return fileman;
}


static void
_e_fileman_scroll_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   e_fm_scroll_set(obj, x, y);
}

static void
_e_fileman_scroll_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   e_fm_scroll_get(obj, x, y);
}

static void 
_e_fileman_scroll_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   e_fm_scroll_max_get(obj, x, y);
}

static void 
_e_fileman_scroll_child_size_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{   
   e_fm_geometry_virtual_get(obj, x, y);
}

EAPI void
e_fileman_selector_enable(E_Fileman *fileman, void (*func)(E_Fileman *fileman, char *file, void *data), void *data)
{   
   fileman->selector.func = func;
   fileman->selector.data = data;
   e_fm_selector_enable(fileman->smart, _e_fileman_selector_cb, NULL, fileman);
}

EAPI void
e_fileman_show(E_Fileman *fileman)
{
   if (!fileman) return;
   
   D(("e_fileman_show: (%p)\n", fileman));
   e_win_show(fileman->win);
   evas_object_show(fileman->main);
}

EAPI void
e_fileman_hide(E_Fileman *fileman)
{
   if (!fileman) return;

   D(("e_fileman_hide: (%p)\n", fileman));
   e_win_hide(fileman->win);
   evas_object_hide(fileman->main);
}

static void
_e_fileman_free(E_Fileman *fileman)
{
   D(("e_fileman_free: (%p)\n", fileman));
   while (fileman->event_handlers)
    {
       ecore_event_handler_del(fileman->event_handlers->data);
       fileman->event_handlers = evas_list_remove_list(fileman->event_handlers, fileman->event_handlers);
    }

   evas_object_del(fileman->smart);
   evas_object_del(fileman->main);
   e_object_del(E_OBJECT(fileman->win));
   free(fileman);
}

static void
_e_fileman_resize_cb(E_Win *win)
{
   E_Fileman *fileman;
 
   fileman = win->data;
   evas_object_resize(fileman->main, win->w, win->h);
}

static void
_e_fileman_delete_cb(E_Win *win)
{
   E_Fileman *fileman;

   fileman = win->data;
   D(("e_fileman_delete_cb: (%p)\n", fileman));
   e_object_del(E_OBJECT(fileman));
}

static void 
_e_fileman_selector_cb(Evas_Object *object, char *file, void *data)
{
   E_Fileman *fileman;
   
   fileman = data;
   fileman->selector.func(fileman, file, fileman->selector.data);
   //e_object_del(E_OBJECT(fileman));
}

static int
_e_fileman_reconfigure_cb(void *data, int type, void *event)
{
   E_Event_Fm_Reconfigure *ev;
   E_Fileman *fileman;
   
   fileman = data;
   ev = event;
   
   e_scrollframe_child_region_show(fileman->main, ev->x, ev->y, ev->w, ev->h);
   return 1;
}

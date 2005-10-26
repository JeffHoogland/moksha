/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
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

static void _e_fileman_vscrollbar_drag_cb(Evas_Object *object, double value, void *data);
static int _e_fileman_reconf_cb(void *data, int type, void *event);
static int _e_fileman_dir_change_cb(void *data, int type, void *event);
static int _e_fileman_mouse_wheel_cb(void *data, int type, void *event);
static void _e_fileman_resize_cb(E_Win *win);
static void _e_fileman_delete_cb(E_Win *win);
static void _e_fileman_selector_cb(Evas_Object *object, char *file, void *data);
static void _e_fileman_vscrollbar_show_cb(void *data, Evas *e, Evas_Object *obj, void *ev);
static void _e_fileman_vscrollbar_hide_cb(void *data, Evas *e, Evas_Object *obj, void *ev);
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

   e_win_delete_callback_set(fileman->win, _e_fileman_delete_cb);
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
   evas_object_event_callback_add(fileman->vscrollbar, EVAS_CALLBACK_SHOW, _e_fileman_vscrollbar_show_cb, fileman);
   evas_object_event_callback_add(fileman->vscrollbar, EVAS_CALLBACK_HIDE, _e_fileman_vscrollbar_hide_cb, fileman);

   e_win_resize_callback_set(fileman->win, _e_fileman_resize_cb);
   e_win_resize(fileman->win, 640, 480);

   evas_event_freeze(fileman->evas);   
   
   fileman->smart = e_fm_add(fileman->evas);
   e_fm_e_win_set(fileman->smart, fileman->win);
   edje_object_part_swallow(fileman->main, "icon_area", fileman->smart);

   ecore_x_dnd_aware_set(fileman->win->evas_win, 1);

   fileman->event_handlers = evas_list_append(fileman->event_handlers,
					      ecore_event_handler_add(E_EVENT_FM_RECONFIGURE,
								      _e_fileman_reconf_cb,
								      fileman));
   
   fileman->event_handlers = evas_list_append(fileman->event_handlers,
					      ecore_event_handler_add(E_EVENT_FM_DIRECTORY_CHANGE,
								      _e_fileman_dir_change_cb,
								      fileman));
   
   fileman->event_handlers = evas_list_append(fileman->event_handlers,
					      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_WHEEL,
								      _e_fileman_mouse_wheel_cb,
								      fileman));   
   
   evas_event_thaw(fileman->evas);
   
   D(("e_fileman_new: ok\n"));
   
   return fileman;
}

void
e_fileman_selector_enable(E_Fileman *fileman, void (*func)(E_Fileman *fileman, char *file, void *data), void *data)
{   
   fileman->selector.func = func;
   fileman->selector.data = data;
   e_fm_selector_enable(fileman->smart, _e_fileman_selector_cb, fileman);
}

void
e_fileman_show(E_Fileman *fileman)
{
   if (!fileman) return;
   
   D(("e_fileman_show: (%p)\n", fileman));
   e_win_show(fileman->win);
   evas_object_show(fileman->main);
}

void
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
   evas_object_del(fileman->vscrollbar);
   evas_object_del(fileman->smart);
   evas_object_del(fileman->main);
   e_object_del(E_OBJECT(fileman->win));
   free(fileman);
}

static void
_e_fileman_resize_cb(E_Win *win)
{
   E_Fileman *fileman;
   Evas_Coord w, h;
   int frozen;
   
   fileman = win->data;
   evas_object_resize(fileman->main, win->w, win->h);
   e_fm_geometry_virtual_get(fileman->smart, &w, &h);    
   
   D(("_e_fileman_resize_cb: e_fm_freeze\n"));
   frozen = e_fm_freeze(fileman->smart);
   if (frozen > 1)
     e_fm_thaw(fileman->smart);
       
   if (h > win->h)
     {
	D(("e_fileman_resize_cb: show (%p)\n", fileman));
	edje_object_part_swallow(fileman->main, "vscrollbar", fileman->vscrollbar);
     }
   else 
     {
	D(("e_fileman_resize_cb: hide (%p)\n", fileman));
	edje_object_part_unswallow(fileman->main, fileman->vscrollbar);
	evas_object_hide(fileman->vscrollbar);
     }   
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
_e_fileman_vscrollbar_drag_cb(Evas_Object *object, double value, void *data)
{
   E_Fileman *fileman;

   fileman = data;
   D(("e_fileman_vscrollbar_drag_cb: %f (%p)\n", value, fileman));
   e_fm_scroll_vertical(fileman->smart, value);
}

static int
_e_fileman_reconf_cb(void *data, int type, void *event)
{   
   E_Event_Fm_Reconfigure *ev;
   E_Fileman *fileman;
   int frozen;

   if (!data) return 1;

   fileman = data;
   ev = event;

   D(("_e_fileman_reconf_cb: e_fm_freeze\n"));
   frozen = e_fm_freeze(fileman->smart);
   if (frozen > 1)
     e_fm_thaw(fileman->smart);

   if (ev->h > fileman->win->h)
     {
	D(("e_fileman_reconf_cb: show (%p)\n", fileman));
	edje_object_part_swallow(fileman->main, "vscrollbar", fileman->vscrollbar);
     }
   else
     {
	D(("e_fileman_reconf_cb: hide (%p)\n", fileman));	
	edje_object_part_unswallow(fileman->main, fileman->vscrollbar);
	evas_object_hide(fileman->vscrollbar);
     }
   return 1;
}

static int
_e_fileman_dir_change_cb(void *data, int type, void *event)
{
   E_Event_Fm_Directory_Change *ev;
   E_Fileman *fileman;
   
   if (!data) return 1;
      
   fileman = data;
   ev = event;
   
   D(("_e_fileman_dir_change_cb:\n"));
   e_scrollbar_value_set(fileman->vscrollbar,  0.0);
   return 1;
}

static int
_e_fileman_mouse_wheel_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Wheel *ev;
   E_Fileman *fileman;
   double pos;
   
   ev = event;   
   fileman = data;
   
   pos = e_scrollbar_value_get(fileman->vscrollbar);

   if (ev->z < 0)
     {
	pos -= 0.05;
	if (pos < 0.0)
	  pos = 0.0;       
     }

   if (ev->z > 0)
     {
	pos += 0.05;
	if (pos > 1.0)
	  pos = 1.0;
     }  
   
   e_scrollbar_value_set(fileman->vscrollbar,  pos);   
   e_fm_scroll_vertical(fileman->smart, pos);
   return 1;
}

static void 
_e_fileman_selector_cb(Evas_Object *object, char *file, void *data)
{
   E_Fileman *fileman;
   
   fileman = data;
   fileman->selector.func(fileman, file, fileman->selector.data);
   //e_object_del(E_OBJECT(fileman));
}

static void
_e_fileman_vscrollbar_show_cb(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   E_Fileman *fileman;
   
   fileman = data;
   
   D(("_e_fileman_vscrollbar_show_cb: thaw (%p)\n", fileman));
   e_fm_thaw(fileman->smart);
}

static void
_e_fileman_vscrollbar_hide_cb(void *data, Evas *e, Evas_Object *obj, void *ev)
{
   E_Fileman *fileman;
   
   fileman = data;
   
   D(("_e_fileman_vscrollbar_hide_cb: thaw (%p)\n", fileman));
   e_fm_thaw(fileman->smart);
}

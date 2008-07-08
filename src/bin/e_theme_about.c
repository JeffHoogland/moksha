/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_theme_about_free(E_Theme_About *about);
static void _e_theme_about_cb_delete(E_Win *win);
static void _e_theme_about_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_theme_about_cb_resize(E_Win *win);

static E_Theme_About *_e_theme_about = NULL;

/* local subsystem globals */

/* externally accessible functions */

EAPI E_Theme_About *
e_theme_about_new(E_Container *con)
{
   E_Theme_About *about;
   E_Manager *man;
   Evas_Object *o;
   
   if (_e_theme_about) 
     {
	E_Zone *z, *z2;
	
	about = _e_theme_about;
	z = e_util_zone_current_get(e_manager_current_get());
	z2 = about->win->border->zone;
	e_win_show(about->win);
	e_win_raise(about->win);
	if (z->container == z2->container)
	  e_border_desk_set(about->win->border, e_desk_current_get(z));
	else 
	  {
	     if (!about->win->border->sticky)
	       e_desk_show(about->win->border->desk);
	     ecore_x_pointer_warp(z2->container->win,
				  z2->x + (z2->w / 2), z2->y + (z2->h / 2));
	  }
	e_border_unshade(about->win->border, E_DIRECTION_DOWN);
	return NULL;
     }
   
   if (!con)
     {
	man = e_manager_current_get();
	if (!man) return NULL;
	con = e_container_current_get(man);
	if (!con) con = e_container_number_get(man, 0);
	if (!con) return NULL;
     }
   about = E_OBJECT_ALLOC(E_Theme_About, E_THEME_ABOUT_TYPE, _e_theme_about_free);
   if (!about) return NULL;
   about->win = e_win_new(con);
   if (!about->win)
     {
	free(about);
	return NULL;
     }
   e_win_delete_callback_set(about->win, _e_theme_about_cb_delete);
   e_win_resize_callback_set(about->win, _e_theme_about_cb_resize);
   about->win->data = about;
   e_win_dialog_set(about->win, 1);
   e_win_name_class_set(about->win, "E", "_theme_about");
   e_win_title_set(about->win, _("About This Theme"));
   
   o = edje_object_add(e_win_evas_get(about->win));
   about->bg_object = o;
   e_theme_edje_object_set(o, "base/theme",
			   "e/theme/about");
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   
   edje_object_signal_callback_add(about->bg_object, "e,action,close", "",
				   _e_theme_about_cb_close, about);
   e_win_centered_set(about->win, 1);

   _e_theme_about = about;
   return about;
}

EAPI void
e_theme_about_show(E_Theme_About *about)
{
   Evas_Coord w, h, mw, mh;
   
   edje_object_size_min_get(about->bg_object, &mw, &mh);
   evas_object_resize(about->bg_object, mw, mh);
   e_win_resize(about->win, mw, mh);
   e_win_size_min_set(about->win, mw, mh);
   
   edje_object_size_max_get(about->bg_object, &w, &h);
   if ((w > 0) && (h > 0))
     {
	if (w < mw) w = mw;
	if (h < mh) h = mh;
	e_win_size_max_set(about->win, mw, mh);
     }
   e_win_show(about->win);
   e_win_border_icon_set(about->win, "enlightenment/themes");
}

/* local subsystem functions */
static void
_e_theme_about_free(E_Theme_About *about)
{
   _e_theme_about = NULL;
   if (about->bg_object) evas_object_del(about->bg_object);
   e_object_del(E_OBJECT(about->win));
   free(about);
}

static void
_e_theme_about_cb_delete(E_Win *win)
{
   E_Theme_About *about;
   
   about = win->data;
   if (!about) return;
   e_object_del(E_OBJECT(about));
}

static void
_e_theme_about_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Theme_About *about;
   
   about = data;
   if (!about) return;
   e_util_defer_object_del(E_OBJECT(about));
}

static void
_e_theme_about_cb_resize(E_Win *win)
{
   E_Theme_About *about;
   
   about = win->data;
   evas_object_resize(about->bg_object, about->win->w, about->win->h);
}

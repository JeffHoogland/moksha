/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_about_free(E_About *about);
static void _e_about_cb_delete(E_Win *win);
static void _e_about_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_about_cb_resize(E_Win *win);

static E_About *_e_about = NULL;

/* local subsystem globals */

/* externally accessible functions */

EAPI E_About *
e_about_new(E_Container *con)
{
   E_Manager *man;
   Evas_Object *o;
   E_About *about;

   if (_e_about) 
     {
	E_Zone *z, *z2;

	about = _e_about;
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
   about = E_OBJECT_ALLOC(E_About, E_ABOUT_TYPE, _e_about_free);
   if (!about) return NULL;
   about->win = e_win_new(con);
   if (!about->win)
     {
	free(about);
	return NULL;
     }
   e_win_delete_callback_set(about->win, _e_about_cb_delete);
   e_win_resize_callback_set(about->win, _e_about_cb_resize);
   about->win->data = about;
   e_win_dialog_set(about->win, 1);
   e_win_name_class_set(about->win, "E", "_about");
   e_win_title_set(about->win, _("About Enlightenment"));

   o = edje_object_add(e_win_evas_get(about->win));
   about->bg_object = o;
   e_theme_edje_object_set(o, "base/theme/about", "e/widgets/about/main");
   evas_object_move(o, 0, 0);
   evas_object_show(o);

   edje_object_part_text_set(about->bg_object, "e.text.title", _("Enlightenment"));
   edje_object_part_text_set(about->bg_object, "e.text.version", VERSION);
   edje_object_part_text_set
     (about->bg_object, "e.textblock.about",
      _(
	"Copyright &copy; 1999-2008, by the Enlightenment Development Team.<br>"
	"<br>"
	"We hope you enjoy using this software as much as we enjoyed "
	"writing it.<br>"
	"<br>"
	"This software is provided as-is with no explicit or implied "
	"warranty. This software is governed by licensing conditions, "
	"so please see the COPYING and COPYING-PLAIN licence files "
	"installed on your system.<br>"
	"<br>"
	"Enlightenment is under <hilight>HEAVY DEVELOPMENT</hilight> and it "
	"is not stable. Many features are incomplete or even non-existant "
	"yet and may have many bugs. You have been <hilight>WARNED!</hilight>"
	)
      );
   edje_object_signal_callback_add(about->bg_object, "e,action,close", "",
				   _e_about_cb_close, about);
     {
	FILE *f;
	char buf[4096], buf2[4096], *tbuf;

	snprintf(buf, sizeof(buf), "%s/AUTHORS", e_prefix_data_get());
	f = fopen(buf, "r");
	if (f)
	  {
	     tbuf = strdup(_("<title>The Team</title>"));
	     while (fgets(buf, sizeof(buf), f))
	       {
		  int len;

		  len = strlen(buf);
		  if (len > 0)
		    {  
		       if (buf[len - 1] == '\n')
			 {
			    buf[len - 1] = 0;
			    len--;
			 }
		       if (len > 0)
			 {
			    char *p;

			    do
			      {
				 p = strchr(buf, '<');
				 if (p) *p = 0;
			      }
			    while (p);
			    do
			      {
				 p = strchr(buf, '>');
				 if (p) *p = 0;
			      }
			    while (p);
			    snprintf(buf2, sizeof(buf2), "%s<br>", buf);
			    tbuf = realloc(tbuf, strlen(tbuf) + strlen(buf2) + 1);
			    strcat(tbuf, buf2);
			 }
		    }
	       }
	     fclose(f);
	     if (tbuf)
	       {
		  edje_object_part_text_set(about->bg_object, 
                                            "e.textblock.authors", tbuf);
		  free(tbuf);
	       }
	  }
     }
   e_win_centered_set(about->win, 1);
   _e_about = about;
   return about;
}

EAPI void
e_about_show(E_About *about)
{
   Evas_Coord w, h, mw, mh;

   edje_object_size_min_get(about->bg_object, &w, &h);
   edje_object_size_min_restricted_calc(about->bg_object, &mw, &mh, w, h);
   if (w > mw) mw = w;
   if (h > mh) mh = h;
   evas_object_resize(about->bg_object, mw, mh);
   e_win_resize(about->win, mw, mh);
   e_win_size_min_set(about->win, mw, mh);

   edje_object_size_max_get(about->bg_object, &w, &h);
   if ((w > 0) && (h > 0))
     {
	if (w < mw) w = mw;
	if (h < mh) h = mh;
	e_win_size_max_set(about->win, w, h);
     }
   e_win_show(about->win);
}

/* local subsystem functions */
static void
_e_about_free(E_About *about)
{
   _e_about = NULL;
   if (about->bg_object) evas_object_del(about->bg_object);
   e_object_del(E_OBJECT(about->win));
   free(about);
}

static void
_e_about_cb_delete(E_Win *win)
{
   E_About *about;

   if (!(about = win->data)) return;
   e_object_del(E_OBJECT(about));
}

static void
_e_about_cb_close(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_About *about;

   if (!(about = data)) return;
   e_util_defer_object_del(E_OBJECT(about));
}

static void
_e_about_cb_resize(E_Win *win)
{
   E_About *about;

   about = win->data;
   evas_object_resize(about->bg_object, about->win->w, about->win->h);
}

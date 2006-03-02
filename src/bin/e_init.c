/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_init_icons_del(void);
static void _e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source);

/* local subsystem globals */
static Ecore_X_Window  _e_init_win = 0;
static Ecore_Evas     *_e_init_ecore_evas = NULL;
static Evas           *_e_init_evas = NULL;
static Evas_Object    *_e_init_object = NULL;
static Evas_Object    *_e_init_icon_box = NULL;
static E_Pointer      *_e_init_pointer = NULL;

/* startup icons */
static Evas_Coord _e_init_icon_size = 0;
static Evas_List *_e_init_icon_list = NULL;

/* externally accessible functions */
EAPI int
e_init_init(void)
{
   int w, h;
   Ecore_X_Window root;
   Ecore_X_Window *roots;
   int num;
   Evas_Object *o;
   Evas_List *l, *screens;
   const char *s;
   
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
        e_error_message_show(_("X reports there are no root windows and %i screens!\n"),
			     num);
	return 0;
     }
   root = roots[0];
   
   ecore_x_window_size_get(root, &w, &h);
   _e_init_ecore_evas = e_canvas_new(e_config->evas_engine_init, root,
				     0, 0, w, h, 1, 1,
				     &(_e_init_win), NULL);
   e_canvas_add(_e_init_ecore_evas);
   _e_init_evas = ecore_evas_get(_e_init_ecore_evas);
   ecore_evas_name_class_set(_e_init_ecore_evas, "E", "Init_Window");
   ecore_evas_title_set(_e_init_ecore_evas, "Enlightenment Init");

   _e_init_pointer = e_pointer_window_new(_e_init_win);

   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);

   screens = (Evas_List *)e_xinerama_screens_get();
   if (screens)
     {
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;
	     
	     scr = l->data;
	     o = edje_object_add(_e_init_evas);
	     /* first screen */
	     if (l == screens)
	       {
		  /* FIXME: "init.edj" needs to come from config */
		  s = e_path_find(path_init, "init.edj"),
		  edje_object_file_set(o, s, "init/splash");
		  if (s) evas_stringshare_del(s);
		  _e_init_object = o;
	       }
	     /* other screens */
	     else
	       {
		  /* FIXME: "init.edj" needs to come from config */
		  s = e_path_find(path_init, "init.edj"),
		  edje_object_file_set(o, s, "init/extra_screen");
		  if (s) evas_stringshare_del(s);
	       }
	     evas_object_move(o, scr->x, scr->y);
	     evas_object_resize(o, scr->w, scr->h);
	     evas_object_show(o);
	  }
     }
   else
     {
	o = edje_object_add(_e_init_evas);
	/* FIXME: "init.edj" needs to come from config */
	s = e_path_find(path_init, "init.edj"),
	edje_object_file_set(o, s, "init/splash");
	if (s) evas_stringshare_del(s);
	_e_init_object = o;
	evas_object_move(o, 0, 0);
	evas_object_resize(o, w, h);
	evas_object_show(o);
     }
   edje_object_part_text_set(_e_init_object, "disable_text", 
			     _("Disable this splash screen in the future?"));
   edje_object_signal_callback_add(_e_init_object, "disable_state", "*",
				   _e_init_cb_signal_disable, NULL);
   edje_object_signal_callback_add(_e_init_object, "done_ok", "*",
				   _e_init_cb_signal_done_ok, NULL);
   free(roots);
   return 1;
}

EAPI int
e_init_shutdown(void)
{
   e_init_hide();
   e_canvas_cache_flush();
   return 1;
}

EAPI void
e_init_show(void)
{
   if (!_e_init_ecore_evas) return;
   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);
}

EAPI void
e_init_hide(void)
{
   /* FIXME: emit signal to edje and wait for it to respond or until a */
   /* in case the edje was badly created and never responds */
   if (!_e_init_ecore_evas) return;
   _e_init_icons_del();
   ecore_evas_hide(_e_init_ecore_evas);
   evas_object_del(_e_init_object);
   e_canvas_del(_e_init_ecore_evas);
   ecore_evas_free(_e_init_ecore_evas);

   if (_e_init_pointer)
     {
	e_object_del(E_OBJECT(_e_init_pointer));
	_e_init_pointer = NULL;
     }

   _e_init_ecore_evas = NULL;
   _e_init_evas = NULL;
   _e_init_win = 0;
   _e_init_object = NULL;
   e_canvas_cache_flush();
}

EAPI void
e_init_title_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "title", str);
}

EAPI void
e_init_version_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "version", str);
}

EAPI void
e_init_status_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "status", str);
}

EAPI Ecore_X_Window
e_init_window_get(void)
{
   return _e_init_win;
}

EAPI void
e_init_done(void)
{
   if (!_e_init_object) return;
   edje_object_signal_emit(_e_init_object, "done", "");
}

EAPI void
e_init_icons_app_add(E_App *app)
{
   Evas_Object *o;

   E_OBJECT_CHECK(app);
   E_OBJECT_TYPE_CHECK(app, E_APP_TYPE);

   if (!_e_init_evas) return;
   
   if (!_e_init_icon_box)
     {
	Evas_Coord w, h;
	
	o = e_box_add(_e_init_evas);
	_e_init_icon_box = o;
	e_box_homogenous_set(o, 1);
	e_box_align_set(o, 0.5, 0.5);
	edje_object_part_swallow(_e_init_object, "icons", o);
	evas_object_geometry_get(o, NULL, NULL, &w, &h);
	if (w > h)
	  {
	     _e_init_icon_size = h;
	     e_box_orientation_set(o, 1);
	  }
	else
	  {
	     _e_init_icon_size = w;
	     e_box_orientation_set(o, 0);
	  }
	evas_object_show(o);
     }
   
   o = e_app_icon_add(_e_init_evas, app);
   evas_object_resize(o, _e_init_icon_size, _e_init_icon_size);
   e_box_pack_end(_e_init_icon_box, o);
   e_box_pack_options_set(o, 
			  0, 0, 
			  0, 0, 
			  0.5, 0.5,
			  _e_init_icon_size, _e_init_icon_size,
			  _e_init_icon_size, _e_init_icon_size);
   evas_object_show(o);
   _e_init_icon_list = evas_list_append(_e_init_icon_list, o);
}

static void
_e_init_icons_del(void)
{
   Evas_Object *next;

   while (_e_init_icon_list)
     {
	next = _e_init_icon_list->data;
	evas_object_del(next);
	_e_init_icon_list = evas_list_remove(_e_init_icon_list, next);
     }
   if (_e_init_icon_box)
     evas_object_del(_e_init_icon_box);
   _e_init_icon_box = NULL;
}

static void
_e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   if (!strcmp(source, "disable")) e_config->show_splash = 0;
   else e_config->show_splash = 1;
   e_config_save_queue();
}

static void
_e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_init_hide();
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI void _e_init_icons_del(void);

/* local subsystem globals */
static Ecore_X_Window  _e_init_win = 0;
static Ecore_Evas     *_e_init_ecore_evas = NULL;
static Evas           *_e_init_evas = NULL;
static Evas_Object    *_e_init_object = NULL;
static Evas_Object    *_e_init_icon_box = NULL;

/* startup icons */
static Evas_Coord _e_init_icon_size;
static Evas_List *_e_init_icon_list;

/* externally accessible functions */
int
e_init_init(void)
{
   int x, y, w, h;
   Ecore_X_Window root;
   Ecore_X_Window *roots;
   int num;
   Evas_Object *o;
   int n;
   
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
        e_error_message_show("X reports there are no root windows and %i screens!\n",
			     num);
	return 0;
     }
   root = roots[0];
   
   ecore_x_window_size_get(root, &w, &h);
   _e_init_ecore_evas = ecore_evas_software_x11_new(NULL, root, 0, 0, w, h);
   e_canvas_add(_e_init_ecore_evas);
   _e_init_evas = ecore_evas_get(_e_init_ecore_evas);
   _e_init_win = ecore_evas_software_x11_window_get(_e_init_ecore_evas);
   ecore_evas_override_set(_e_init_ecore_evas, 1);
   ecore_evas_name_class_set(_e_init_ecore_evas, "E", "Init_Window");
   ecore_evas_title_set(_e_init_ecore_evas, "Enlightenment Init");
   e_path_evas_append(path_fonts, _e_init_ecore_evas);
   e_pointer_ecore_evas_set(_e_init_ecore_evas);
   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);
   
   n = ecore_x_xinerama_screen_count_get();
   if (n == 0)
     {
	o = edje_object_add(_e_init_evas);
	edje_object_file_set(o,
			     /* FIXME: "init.eet" needs to come from config */
			     e_path_find(path_init, "init.eet"),
			     "init/splash");
	evas_object_move(o, 0, 0);
	evas_object_resize(o, w, h);
	evas_object_show(o);
	_e_init_object = o;
     }
   else
     {
	int i;
	int mx, my, mw, mh;
	
	for (i = 0; i < n; i++)
	  {
	     ecore_x_xinerama_screen_geometry_get(i, &x, &y, &w, &h);
	     if (i == 0)
	       {
		  /* Remeber the size and placement of the first window */
		  mx = x;
		  my = y;
		  mw = w;
		  mh = h;
		  /* Init splash */
		  o = edje_object_add(_e_init_evas);
		  edje_object_file_set(o,
				       /* FIXME: "init.eet" needs to come from config */
				       e_path_find(path_init, "init.eet"),
				       "init/splash");
		  evas_object_move(o, x, y);
		  evas_object_resize(o, w, h);
		  evas_object_show(o);
		  _e_init_object = o;
	       }
	     /* Only add extra screen if it doesn't overlap with the main screen */
	     /* FIXME: What if extra screens overlap? Maybe zones should be
	      * initialized before we come here? */
	     else if (!E_INTERSECTS(x, y, w, h,
				    mx, my, mw, mh))
	       {
		  o = edje_object_add(_e_init_evas);
		  edje_object_file_set(o,
				       /* FIXME: "init.eet" needs to come from config */
				       e_path_find(path_init, "init.eet"),
				       "init/extra_screen");
		  evas_object_move(o, x, y);
		  evas_object_resize(o, w, h);
		  evas_object_show(o);
	       }
	  }
     }
   
   free(roots);
   return 1;
}

int
e_init_shutdown(void)
{
   e_init_hide();
   e_canvas_cache_flush();
   return 1;
}

void
e_init_show(void)
{
   if (!_e_init_ecore_evas) return;
   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);
}

void
e_init_hide(void)
{
   /* FIXME: emit signal to edje and wait for it to respond or until a */
   /* in case the edje was badly created and never responds */
   if (!_e_init_ecore_evas) return;
   ecore_evas_hide(_e_init_ecore_evas);
   evas_object_del(_e_init_object);
   e_canvas_del(_e_init_ecore_evas);
   ecore_evas_free(_e_init_ecore_evas);
   _e_init_ecore_evas = NULL;
   _e_init_evas = NULL;
   _e_init_win = 0;
   _e_init_object = NULL;

   _e_init_icons_del();
}

void
e_init_title_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "title", str);
}

void
e_init_version_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "version", str);
}

void
e_init_status_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "status", str);
}

Ecore_X_Window
e_init_window_get(void)
{
   return _e_init_win;
}

/* code for displaying startup icons */

void
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
   
   o = edje_object_add(_e_init_evas);
   edje_object_file_set(o,app->path, "icon");
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

void
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

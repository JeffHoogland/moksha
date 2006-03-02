/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 *
 * * edjify error dialogs if edje data can be found for them
 * * current gui dialg needs to resize to fit contents if they are bigger
 */

/* local subsystem functions */
static void _e_error_message_show_x(char *txt);

static void _e_error_cb_ok_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_error_cb_ok_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_error_edje_cb_ok_up(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_error_cb_job_ecore_evas_free(void *data);

/* local subsystem globals */
static int error_gui = 0;

/* externally accessible functions */
EAPI void
e_error_message_show_internal(char *txt)
{
   printf("_______                     _______\n"
	  "|:::::| Enlightenment Error |:::::|\n"
	  "~~~~~~~                     ~~~~~~~\n"
	  "%s\n",
	  txt);
   if (error_gui) _e_error_message_show_x(txt);
}

EAPI void
e_error_dialog_show_internal(char *title, char *txt)
{
   Evas_List *l;
   E_Manager *man;

   l = e_manager_list();
   if (!l) return;
   man = l->data;
   e_error_message_manager_show(man, title, txt);
}

EAPI void
e_error_gui_set(int on)
{
   error_gui = on;
}

EAPI void
e_error_message_manager_show(E_Manager *man, char *title, char *txt)
{
   Ecore_Evas  *ee;
   Evas        *e;
   Evas_Object *o;
   int          error_w, error_h;
   Evas_List   *l, *shapelist = NULL;
   Evas_Coord   maxw, maxh;
   E_Container *con;
   Ecore_X_Window win;
   int          x, y;
   const char  *s;

   con = e_container_current_get(man);

   error_w = 400;
   error_h = 200;
   x = (man->w - error_w) / 2;
   y = (man->h - error_h) / 2;
   ee = e_canvas_new(e_config->evas_engine_errors, man->win,
		     x, y, error_w, error_h, 1, 0,
		     &win, NULL);
   e_canvas_add(ee);
   ecore_evas_override_set(ee, 1);
   e_container_window_raise(con, win, 999);

   ecore_evas_name_class_set(ee, "E", "Low_Level_Dialog");
   ecore_evas_title_set(ee, "Enlightenment: Low Level Dialog");
//   ecore_evas_avoid_damage_set(ee, 1);
   e = ecore_evas_get(ee);

   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/error", "error/main"))
     {
	Evas_Coord tw, th;
	char *newstr;

	if (o) evas_object_del(o);

	maxw = 0;
	maxh = 0;

	o = evas_object_image_add(e);
	s = e_path_find(path_images, "e.png");
	evas_object_image_file_set(o, s, NULL);
	if (s) evas_stringshare_del(s);
	evas_object_move(o, 16, 16);
	evas_object_resize(o, 64, 64);
	evas_object_image_fill_set(o, 0, 0, 64, 64);
	evas_object_pass_events_set(o, 1);
	evas_object_show(o);

	o = evas_object_text_add(e);
	evas_object_color_set(o, 255, 255, 255, 128);
	evas_object_text_font_set(o, "Vera-Bold", 12);
	evas_object_text_text_set(o, title);
	evas_object_geometry_get(o, NULL, NULL, &tw, &th);
	evas_object_move(o,
			 (16 + 64 + 16) + 1,
			 (16 + ((64 - th) / 2)) + 1);
	evas_object_pass_events_set(o, 1);
	evas_object_show(o);

	maxw = 16 + 64 + 16 + tw + 16;
	maxh = 16 + 64;

	o = evas_object_text_add(e);
	evas_object_color_set(o, 0, 0, 0, 255);
	evas_object_text_font_set(o, "Vera-Bold", 12);
	evas_object_text_text_set(o, title);
	evas_object_geometry_get(o, NULL, NULL, &tw, &th);
	evas_object_move(o,
			 16 + 64 + 16,
			 16 + ((64 - th) / 2));
	evas_object_pass_events_set(o, 1);
	evas_object_show(o);

	newstr = strdup(txt);
	if (newstr)
	  {
	     char *p;
	     Evas_Coord y;
	     char *fname;
	     int fsize;
	     
	     y = 16 + 64 + 16;
	     fname = (char *)e_font_default_string_get("default", &fsize);
	     for (p = newstr; p;)
	       {
		  char *pp;

		  pp = strchr(p, '\n');
		  if (pp) *pp = 0;
		  o = evas_object_text_add(e);
		  evas_object_color_set(o, 255, 255, 255, 128);
		  evas_object_text_font_set(o, fname, fsize);
		  evas_object_text_text_set(o, p);
		  evas_object_geometry_get(o, NULL, NULL, &tw, &th);
		  evas_object_move(o, 16 + 1, y + 1);
		  evas_object_pass_events_set(o, 1);
		  evas_object_show(o);

		  o = evas_object_text_add(e);
		  evas_object_color_set(o, 0, 0, 0, 255);
		  evas_object_text_font_set(o, fname, fsize);
		  evas_object_text_text_set(o, p);
		  evas_object_geometry_get(o, NULL, NULL, &tw, &th);
		  evas_object_move(o, 16, y);
		  evas_object_pass_events_set(o, 1);
		  evas_object_show(o);

		  if ((16 + tw + 16) > maxw) maxw = 16 + tw + 16;
		  y += th;
		  if (pp) p = pp + 1;
		  else p = NULL;
	       }
	     free(newstr);
	     maxh = y;
	  }

	maxh += 16 + 32 + 16;
	error_w = maxw;
	error_h = maxh;

	if (error_w > man->w) error_w = man->w;
	if (error_h > man->h) error_h = man->h;

	o = evas_object_image_add(e);
	s = e_path_find(path_images, "button_out.png");
	evas_object_image_file_set(o, s, NULL);
	if (s) evas_stringshare_del(s);
	evas_object_move(o, (error_w - 64) / 2, error_h - 16 - 32);
	evas_object_resize(o, 64, 32);
	evas_object_image_fill_set(o, 0, 0, 64, 32);
	evas_object_image_border_set(o, 8, 8, 8, 8);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_error_cb_ok_down, ee);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_error_cb_ok_up, ee);
	evas_object_show(o);

	o = evas_object_text_add(e);
	evas_object_color_set(o, 255, 255, 255, 128);
	evas_object_text_font_set(o, "Vera-Bold", 12);
	evas_object_text_text_set(o, _("OK"));
	evas_object_geometry_get(o, NULL, NULL, &tw, &th);
	evas_object_move(o, ((error_w - tw) / 2) + 1, (error_h - 16 - 32 + ((32 - th) / 2)) + 1);
	evas_object_pass_events_set(o, 1);
	evas_object_show(o);

	o = evas_object_text_add(e);
	evas_object_color_set(o, 0, 0, 0, 255);
	evas_object_text_font_set(o, "Vera-Bold", 12);
	evas_object_text_text_set(o, _("OK"));
	evas_object_geometry_get(o, NULL, NULL, &tw, &th);
	evas_object_move(o, (error_w - tw) / 2, error_h - 16 - 32 + ((32 - th) / 2));
	evas_object_pass_events_set(o, 1);
	evas_object_show(o);

	o = evas_object_image_add(e);
	s = e_path_find(path_images, "error_bg.png");
	evas_object_image_file_set(o, s, NULL);
	if (s) evas_stringshare_del(s);
	evas_object_move(o, 0, 0);
	evas_object_image_fill_set(o, 0, 0, error_w, error_h);
	evas_object_resize(o, error_w, error_h);
	evas_object_image_border_set(o, 3, 3, 3, 3);
	evas_object_pass_events_set(o, 1);
	evas_object_layer_set(o, -10);
	evas_object_show(o);

	x = (man->w - error_w) / 2;
	y = (man->h - error_h) / 2;
	con = e_container_current_get(man);
	if (con)
	  {
	     E_Zone *zone;
	     
	     zone = e_container_zone_number_get(con, 0);
	     if (zone)
	       {
		  x = zone->x + ((zone->w - error_w) / 2);
		  y = zone->y + ((zone->h - error_h) / 2);
	       }
	  }
	ecore_evas_move(ee, x, y);
	ecore_evas_resize(ee, error_w, error_h);

	for (l = man->containers; l; l = l->next)
	  {
	     E_Container_Shape *es;

	     con = l->data;
	     es = e_container_shape_add(con);
	     e_container_shape_move(es, x, y);
	     e_container_shape_resize(es, error_w, error_h);
	     e_container_shape_show(es);
	     shapelist = evas_list_append(shapelist, es);
	  }
	ecore_evas_data_set(ee, "shapes", shapelist);

	o = evas_object_rectangle_add(e);
	evas_object_name_set(o, "allocated");
     }
   else
     {
	int x, y;
	Evas_Coord ow, oh;

	evas_object_move(o, 0, 0);
	evas_object_resize(o, error_w, error_h);
	edje_object_signal_callback_add(o, "close", "",
					_e_error_edje_cb_ok_up, ee);
	evas_object_show(o);

	edje_object_part_text_set(o, "title", title);
	
	  {
	     char *pp, *newstr, *p, *markup = NULL;
	     
	     newstr = strdup(txt);
	     p = newstr;
	     while (p)
	       {
		  pp = strchr(p, '\n');
		  if (pp) *pp = 0;
		  if (markup)
		    {
		       markup = realloc(markup, strlen(markup) + strlen(p) + 1);
		       strcat(markup, p);
		    }
		  else
		    markup = strdup(p);
		  if (pp)
		    {
		       p = pp + 1;
		       if (markup)
			 {
			    markup = realloc(markup, strlen(markup) + strlen("<br>") + 1);
			    strcat(markup, "<br>");
			 }
		       else
			 markup = strdup("<br>");
		    }
		  else
		    p = NULL;
	       }
	     edje_object_part_text_set(o, "text", markup);
	     free(markup);
	     free(newstr);
	  }
	edje_object_size_min_calc(o, &ow, &oh);
	error_w = ow;
	error_h = oh;
	
	evas_object_move(o, 0, 0);
	evas_object_resize(o, error_w, error_h);
	evas_object_show(o);

	x = (man->w - error_w) / 2;
	y = (man->h - error_h) / 2;
	con = e_container_current_get(man);
	if (con)
	  {
	     E_Zone *zone;
	     
	     zone = e_container_zone_number_get(con, 0);
	     if (zone)
	       {
		  x = zone->x + ((zone->w - error_w) / 2);
		  y = zone->y + ((zone->h - error_h) / 2);
	       }
	  }
	ecore_evas_move(ee, x, y);
	ecore_evas_resize(ee, error_w, error_h);

	for (l = man->containers; l; l = l->next)
	  {
	     E_Container *con;
	     E_Container_Shape *es;

	     con = l->data;
	     es = e_container_shape_add(con);
	     e_container_shape_move(es, x, y);
	     e_container_shape_resize(es, error_w, error_h);
	     e_container_shape_show(es);
	     shapelist = evas_list_append(shapelist, es);
	  }
	ecore_evas_data_set(ee, "shapes", shapelist);

	o = evas_object_rectangle_add(e);
	evas_object_name_set(o, "allocated");
     }
   ecore_evas_show(ee);
}

/* local subsystem functions */
static void
_e_error_message_show_x(char *txt)
{
   e_error_dialog_show_internal(_("Enlightenment: Error!"), txt);
}

static void
_e_error_cb_ok_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Ecore_Evas *ee;
   const char *s;
   
   ev = event_info;
   if (ev->button != 1) return;
   ee = data;
   s = e_path_find(path_images, "button_in.png");
   evas_object_image_file_set(obj, s, NULL);
   if (s) evas_stringshare_del(s);
}

static void
_e_error_cb_ok_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Ecore_Evas *ee;
   Evas_Object *o;
   const char *s;

   ev = event_info;
   if (ev->button != 1) return;
   ee = data;
   s = e_path_find(path_images, "button_out.png");
   evas_object_image_file_set(obj, s, NULL);
   if (s) evas_stringshare_del(s);
   o = evas_object_name_find(ecore_evas_get(ee), "allocated");
   if (o)
     {
	evas_object_del(o);
	ecore_job_add(_e_error_cb_job_ecore_evas_free, ee);
     }
}

static void
_e_error_edje_cb_ok_up(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Ecore_Evas *ee;
   Evas_Object *o;

   ee = data;
   o = evas_object_name_find(ecore_evas_get(ee), "allocated");
   if (o)
     {
	evas_object_del(o);
	ecore_job_add(_e_error_cb_job_ecore_evas_free, ee);
     }
}

static void
_e_error_cb_job_ecore_evas_free(void *data)
{
   Ecore_Evas *ee;
   Evas_List *shapelist, *l;

   ee = data;
   shapelist = ecore_evas_data_get(ee, "shapes");
   for (l = shapelist; l; l = l->next) e_object_del(E_OBJECT(l->data));
   evas_list_free(shapelist);

   e_canvas_del(ee);
   ecore_evas_free(ee);
}

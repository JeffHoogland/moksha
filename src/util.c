#include "debug.h"
#include "util.h"
#include "Evas.h"
#include "Evas_Engine_Software_X11.h"

void
e_util_set_env(char *variable, char *content)
{
   char                env[PATH_MAX];

   D_ENTER;

   snprintf(env, PATH_MAX, "%s=%s", variable, content);
   putenv(env);

   D_RETURN;
}

char               *
e_util_get_user_home(void)
{
   static char        *home = NULL;

   D_ENTER;

   if (home)
      D_RETURN_(home);
   home = getenv("HOME");
   if (!home)
      home = getenv("TMPDIR");
   if (!home)
      home = "/tmp";

   D_RETURN_(home);
}

void               *
e_util_memdup(void *data, int size)
{
   void               *data_dup;

   D_ENTER;

   data_dup = malloc(size);
   if (!data_dup)
      D_RETURN_(NULL);
   memcpy(data_dup, data, size);

   D_RETURN_(data_dup);
}

int
e_util_glob_matches(char *str, char *glob)
{
   D_ENTER;

   if (!fnmatch(glob, str, 0))
      D_RETURN_(1);

   D_RETURN_(0);
}

/*
 * Function to take a URL of the form
 *  file://dir1/dir2/file
 *
 * Test that 'file:' exists.
 * Return a pointer to /dir1/...
 *
 * todo: 
 */
char               *
e_util_de_url_and_verify(const char *fi)
{
   char               *wk;

   D_ENTER;

   wk = strstr(fi, "file:");

   /* Valid URL contains "file:" */
   if (!wk)
      D_RETURN_(NULL);

   /* Need some form of hostname to continue */
   /*  if( !hostn )
    * *   D_RETURN_ (NULL);
    */

   /* Do we contain hostname? */
   /*  wk = strstr( fi, hostn );
    */

   /* Hostname mismatch, reject file */
   /*  if( !wk )
    * *   D_RETURN_ (NULL);
    */

   /* Local file name starts after "hostname" */
   wk = strchr(wk, '/');

   if (!wk)
      D_RETURN_(NULL);

   D_RETURN_(wk);
}


Evas *
e_evas_new_all(Display *disp, Window parent_window,
	       int x, int y, int win_w, int win_h,
	       char *font_dir)
{
  Evas *e;

   e = evas_new();
   evas_output_method_set(e, evas_render_method_lookup("software_x11"));
   evas_output_size_set(e, win_w, win_h);
   evas_output_viewport_set(e, 0, 0, win_w, win_h);
   {
      Evas_Engine_Info_Software_X11 *einfo;
      XSetWindowAttributes att;
      Window window;

      einfo = (Evas_Engine_Info_Software_X11 *) evas_engine_info_get(e);

      /* the following is specific to the engine */
      einfo->info.display = disp;
      einfo->info.visual = DefaultVisual(disp, DefaultScreen(disp));
      einfo->info.colormap = DefaultColormap(disp, DefaultScreen(disp));

      att.background_pixmap = None;
      att.colormap = /*colormap*/ DefaultColormap(disp, DefaultScreen(disp));
      att.border_pixel = 0;
      att.event_mask = 0;
      window = XCreateWindow(disp,
			     parent_window,
			     x, y, win_w, win_h, 0,
			     DefaultDepth(disp, DefaultScreen(disp)),
			     /*imlib_get_visual_depth(display, visual),*/
			     InputOutput,
			     einfo->info.visual,
			     CWColormap | CWBorderPixel | CWEventMask | CWBackPixmap,
			     &att);

      einfo->info.drawable = window /*win*/;
      einfo->info.depth = DefaultDepth(disp, DefaultScreen(disp));
      einfo->info.rotation = 0;
      einfo->info.debug = 0;
      evas_engine_info_set(e, (Evas_Engine_Info *) einfo);
   }

   evas_object_image_cache_set(e, 0);
   evas_object_font_cache_set(e, 0);
   evas_object_font_path_append(e, font_dir);

   return e;
}

Evas_List *
e_evas_get_mask(Evas *evas, Pixmap pmap, Pixmap mask)
{
   int                            w, h;
   Pixmap                         old_pmap, old_mask;
   Evas_List                     *updates;
   Evas_Engine_Info_Software_X11 *info;

   info = (Evas_Engine_Info_Software_X11 *) evas_engine_info_get(evas);

   old_pmap = info->info.drawable;
   old_mask = info->info.mask;

   info->info.drawable = pmap;
   info->info.mask = mask;

   evas_engine_info_set(evas, (Evas_Engine_Info *) info);
   evas_output_size_get(evas, &w, &h);
   evas_damage_rectangle_add(evas, 0, 0, w, h);
   evas_render(evas);
   evas_damage_rectangle_add(evas, 0, 0, w, h);

   info->info.drawable = old_pmap;
   info->info.mask = old_mask;
   evas_engine_info_set(evas, (Evas_Engine_Info *) info);

   return updates;
}


Window
e_evas_get_window(Evas *evas)
{
  Window              win;
  Evas_Engine_Info_Software_X11 *einfo;

  einfo = (Evas_Engine_Info_Software_X11 *) evas_engine_info_get(evas);

  /* the following is specific to the engine */
  win = einfo->info.drawable;

  return win;
}

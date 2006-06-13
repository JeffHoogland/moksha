/*
* vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
*/

#include "e.h"

#ifdef EFM_DEBUG
# define D(x)  do {printf(__FILE__ ":%d:  ", __LINE__); printf x; fflush(stdout);} while (0)
#else
# define D(x)  ((void) 0)
#endif

typedef struct _E_Thumb_Item E_Thumb_Item;

static char       *_e_thumb_file_id(char *file);
static void        _e_thumb_generate(void);
static int         _e_thumb_cb_exe_exit(void *data, int type, void *event);

static char       *thumb_path = NULL;
static Evas_List  *thumb_files = NULL;
static Evas_List  *event_handlers = NULL;
static pid_t       pid = -1;

struct _E_Thumb_Item
{
   char           path[PATH_MAX];
   Evas_Object   *obj;
   Evas          *evas;
   Evas_Coord     w, h;
   void (*cb)(Evas_Object *obj, void *data);
   void  *data;
};

EAPI int
e_thumb_init(void)
{
   char *homedir;
   char  path[PATH_MAX];
   
   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(path, sizeof(path), "%s/.e/e/fileman/thumbnails", homedir);
	if (!ecore_file_exists(path))
	  ecore_file_mkpath(path);
	thumb_path = strdup(path);
	free(homedir);
     }
   else return 0;
   
   event_handlers = 
     evas_list_append(event_handlers,
		      ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
					      _e_thumb_cb_exe_exit,
					      NULL));      
   return 1;
}

EAPI int
e_thumb_shutdown(void)
{
   E_FREE(thumb_path);
   while (event_handlers)     
     {   
	ecore_event_handler_del(event_handlers->data);
	event_handlers = evas_list_remove_list(event_handlers, event_handlers);
     }
   evas_list_free(thumb_files);
   
   return 1;
}

/* return dir where thumbs are saved */
EAPI const char *
e_thumb_dir_get(void)
{
   return thumb_path;
}

/* queue an image for thumbnailing or return the thumb if it exists */
EAPI Evas_Object *
e_thumb_generate_begin(char *path, Evas_Coord w, Evas_Coord h, Evas *evas, Evas_Object **tmp, void (*cb)(Evas_Object *obj, void *data), void *data)
{
   E_Thumb_Item *t;
   
   if(!ecore_file_exists(path)) return *tmp;   
   if (e_thumb_exists(path))
     {
	evas_object_del(*tmp);
	*tmp = e_thumb_evas_object_get(path, evas, w, h, 1);
	return *tmp;
     }
   
   t = E_NEW(E_Thumb_Item, 1);
   t->w = w;
   t->h = h;
   t->evas = evas;
   t->cb = cb;
   t->data = data;
   *tmp = e_icon_add(evas);
   t->obj = *tmp;
   snprintf(t->path, sizeof(t->path), "%s", path);
   thumb_files = evas_list_append(thumb_files, t);
   if (pid == -1) _e_thumb_generate();
   
   return *tmp;   
}

/* delete an image from the thumb queue */
EAPI void
e_thumb_generate_end(char *path)
{
   Evas_List *l;
   E_Thumb_Item *t;
   
   for(l = thumb_files; l; l = l->next)
     {
	t = l->data;
	if(!strcmp(path, t->path))
	   {
	      thumb_files = evas_list_remove_list(thumb_files, l);
	      break;
	   }
     }
}

/* return hashed path of thumb */
EAPI char *
e_thumb_file_get(char *file)
{
   char *id;
   char thumb[PATH_MAX];
   
   id = _e_thumb_file_id(file);   
   if (!id) return NULL;
   snprintf(thumb, sizeof(thumb), "%s/%s", thumb_path, id);
   free(id);
   return strdup(thumb);   
}

/* return the width and height of a thumb, if from_eet is set, then we
 * assume that the file being passed is the thumb's eet
 */
EAPI void
e_thumb_geometry_get(char *file, int *w, int *h, int from_eet)
{
   Eet_File *ef;   
   
   if(!from_eet)
     {
	char *eet_file;
	
	eet_file = _e_thumb_file_id(file);
	if(!eet_file)
	  {
	     if(w) *w = -1;
	     if(h) *h = -1;
	     return;
	  }
	ef = eet_open(eet_file, EET_FILE_MODE_READ);
	if (!ef)
	  {
	     eet_close(ef);	     
	     if(w) *w = -1;
	     if(h) *h = -1;
	     return;
	  }
     }
   else
     {
	ef = eet_open(file, EET_FILE_MODE_READ);
	if (!ef)
	  {
	     eet_close(ef);	     
	     if(w) *w = -1;
	     if(h) *h = -1;
	     return;
	  }
     }
   if(!eet_data_image_header_read(ef, "/thumbnail/data", w, h, NULL, NULL,
				  NULL, NULL))
     {
	eet_close(ef);	
	if(w) *w = -1;
	if(h) *h = -1;
	return;
     }      
   eet_close(ef);
}

/* return true if the saved thumb exists OR if its an eap */
EAPI int
e_thumb_exists(char *file)
{
   char *thumb;
   char *ext;

   ext = strrchr(file, '.');
   if ((ext) && (!strcasecmp(ext, ".eap")))
     return 1;

   thumb = e_thumb_file_get(file);
   if ((thumb) && (ecore_file_exists(thumb)))
     {
	free(thumb);
	return 1;
     }
   return 0;
}

EAPI int *
_e_thumb_image_create(char *file, Evas_Coord w, Evas_Coord h, int *ww, int *hh, int *alpha, Evas_Object **im, Ecore_Evas **buf)
{
   Evas *evasbuf;
   int iw, ih;
   
   *buf = ecore_evas_buffer_new(1, 1);
   evasbuf = ecore_evas_get(*buf);
   *im = evas_object_image_add(evasbuf);
   evas_object_image_file_set(*im, file, NULL);
   iw = 0; ih = 0;
   evas_object_image_size_get(*im, &iw, &ih);
   *alpha = evas_object_image_alpha_get(*im);
   if ((iw > 0) && (ih > 0))
     {
	*ww = w;
	*hh = (w * ih) / iw;
	if (*hh > h)
	  {
	     *hh = h;
	     *ww = (h * iw) / ih;
	  }
	ecore_evas_resize(*buf, *ww, *hh);
	evas_object_image_fill_set(*im, 0, 0, *ww, *hh);
	evas_object_resize(*im, *ww, *hh);
	evas_object_move(*im, 0, 0);
	evas_object_show(*im);

	return (int *)ecore_evas_buffer_pixels_get(*buf);
     }
   return NULL;
}

/* thumbnail an e17 background and return pixel data */
const int *
_e_thumb_ebg_create(char *file, Evas_Coord w, Evas_Coord h, int *ww, int *hh, int *alpha, Evas_Object **im, Ecore_Evas **buf)
{
   Evas *evasbuf;   
   Evas_Object *wallpaper;
   const int *pixels;   

   *ww = 640;
   *hh = 480;
   *alpha = 0;   
   
   w = 640;
   h = 480;
   
   *buf = ecore_evas_buffer_new(w, h);
   evasbuf = ecore_evas_get(*buf);
   
   wallpaper = edje_object_add(evasbuf);

   edje_object_file_set(wallpaper, file, "desktop/background");
      
   /* wallpaper */
   evas_object_move(wallpaper, 0, 0);
   evas_object_resize(wallpaper, w, h);   
      
   evas_object_show(wallpaper);
   
   pixels = ecore_evas_buffer_pixels_get(*buf);
   
   evas_object_del(wallpaper);   
   return pixels;
}

/* thumbnail an e17 theme and return pixel data */
const int *
_e_thumb_etheme_create(char *file, Evas_Coord w, Evas_Coord h, int *ww, int *hh, int *alpha, Evas_Object **im, Ecore_Evas **buf)
{
   Evas *evasbuf;   
   Evas_Object *wallpaper, *window, *clock, *start, **pager, *init;
   const int *pixels;   
   int is_init;
   
   *ww = 640;
   *hh = 480;
   *alpha = 0;   
   
   w = 640;
   h = 480;
   
   *buf = ecore_evas_buffer_new(w, h);
   evasbuf = ecore_evas_get(*buf);

   is_init = e_util_edje_collection_exists(file, "init/splash");
   if (is_init) 
     {
	init = edje_object_add(evasbuf);
	edje_object_file_set(init, file, "init/splash");
	evas_object_move(init, 0, 0);
	evas_object_resize(init, w, h);   	
	evas_object_show(init);
	pixels = ecore_evas_buffer_pixels_get(*buf);	
	evas_object_del(init);	
     }
   else 
     {
	wallpaper = edje_object_add(evasbuf);
	window    = edje_object_add(evasbuf);
	clock     = edje_object_add(evasbuf);
	start     = edje_object_add(evasbuf);
	pager     = E_NEW(Evas_Object*, 3);
	pager[0]  = edje_object_add(evasbuf);
	pager[1]  = edje_object_add(evasbuf);
	pager[2]  = edje_object_add(evasbuf);

	edje_object_file_set(wallpaper, file, "desktop/background");   
	edje_object_file_set(window,	file, "widgets/border/default/border");
	edje_object_file_set(clock, file, "modules/clock/main");   
	edje_object_file_set(clock, file, "modules/clock/main");   
	edje_object_file_set(start, file, "modules/start/main");   
	edje_object_file_set(pager[0], file, "modules/pager/main");
	edje_object_file_set(pager[1], file, "modules/pager/desk");
	edje_object_file_set(pager[2], file, "modules/pager/window");   
	edje_object_part_text_set(window, "title_text", file);   
	edje_object_part_swallow(pager[0], "items", pager[1]);
	edje_object_part_swallow(pager[1], "items", pager[2]);
   
	/* wallpaper */
	evas_object_move(wallpaper, 0, 0);
	evas_object_resize(wallpaper, w, h);   
	/* main window */
	evas_object_move(window, (w * 0.1), (h * 0.05));
	evas_object_resize(window, w * 0.8, h * 0.75);   
	/* clock */
	evas_object_move(clock, (w * 0.9), (h * 0.9));
	evas_object_resize(clock, w * 0.1, h * 0.1);
	/* start */
	evas_object_move(start, 0.1, (h * 0.9));
	evas_object_resize(start, w * 0.1, h * 0.1);   
	/* pager */
	evas_object_move(pager[0], (w * 0.3), (h * 0.9));
	evas_object_resize(pager[0], w * 0.1, h * 0.1);
      
	evas_object_show(wallpaper);
	evas_object_show(window);
	evas_object_show(clock);
	evas_object_show(start);
	evas_object_show(pager[0]);
	evas_object_show(pager[1]);
	evas_object_show(pager[2]);

	pixels = ecore_evas_buffer_pixels_get(*buf);
	
	evas_object_del(wallpaper);
	evas_object_del(window);
	evas_object_del(clock);
	evas_object_del(start);
	evas_object_del(pager[0]);
	evas_object_del(pager[1]);
	evas_object_del(pager[2]);   
	free(pager);
     }

   return pixels;
}

/* create and save a thumb to disk */
EAPI int
e_thumb_create(char *file, Evas_Coord w, Evas_Coord h)
{
   Eet_File *ef;
   char *thumbpath, *ext;
   Evas_Object *im = NULL;
   const int *data;
   int size, ww, hh;
   Ecore_Evas *buf;
   int alpha;
   
   ext = strrchr(file, '.');
   if ((ext) && (!strcasecmp(ext, ".eap")))
     return 1;
   
   thumbpath = e_thumb_file_get(file);
   if (!thumbpath)
     return -1;
   
   if (ext)
     {
	if (!strcasecmp(ext, ".edj"))
	  {
	     /* for now, this function does both the bg and theme previews */
	     data = _e_thumb_etheme_create(file, w, h, &ww, &hh, &alpha, &im, &buf);
	  }
	else
	  data = _e_thumb_image_create(file, w, h, &ww, &hh, &alpha, &im, &buf);	 
     }
   else
     data = _e_thumb_image_create(file, w, h, &ww, &hh, &alpha, &im, &buf);
   
   if (data)
     {
	ef = eet_open(thumbpath, EET_FILE_MODE_WRITE);
	if (!ef)
	  {
	     free(thumbpath);
	     if (im) evas_object_del(im);
	     ecore_evas_free(buf);
	     return -1;
	  }
	free(thumbpath);

	eet_write(ef, "/thumbnail/orig_path", file, strlen(file), 1);
	if ((size = eet_data_image_write(ef, "/thumbnail/data",
					 (void *)data, ww, hh, alpha,
					 0, 91, 1)) <= 0)
	  {
	     if (im) evas_object_del(im);
	     ecore_evas_free(buf);
	     eet_close(ef);
	     return -1;
	  }
	eet_close(ef);
     }
   if (im) evas_object_del(im);
   ecore_evas_free(buf);
   return 1;
}

/* get evas object containing image of the thumb */
EAPI Evas_Object *
e_thumb_evas_object_get(char *file, Evas *evas, Evas_Coord width, Evas_Coord height, int shrink)
{
   Eet_File *ef;
   char *thumb, *ext;
   Evas_Object *im = NULL;

#define DEF_THUMB_RETURN im = evas_object_rectangle_add(evas); \
	       evas_object_color_set(im, 255, 255, 255, 255); \
	       evas_object_resize(im, width, height); \
	       return im 
   
   D(("e_thumb_evas_object_get: (%s)\n", file));
     
   /* eap thumbnailer */
   ext = strrchr(file, '.');
   if (ext)
     {
	if (!strcasecmp(ext, ".eap"))
	  {
	     E_App *app;

	     D(("e_thumb_evas_object_get: eap found\n"));
	     app = e_app_new(file, 0);
	     D(("e_thumb_evas_object_get: eap loaded\n"));
	     if (!app)	     
	       { 
		  D(("e_thumb_evas_object_get: invalid eap\n"));
		  DEF_THUMB_RETURN;
	       }
	     else
	       {
		  D(("e_thumb_evas_object_get: creating eap thumb\n"));
		  im = e_icon_add(evas);
		  e_icon_file_edje_set(im, file, "icon");
		  e_object_unref(E_OBJECT(app));	       
		  D(("e_thumb_evas_object_get: returning eap thumb\n"));
		  return im;
	       }
	  }
     }
     
   /* saved thumb */
   /* TODO: add ability to fetch thumbs from freedesktop dirs */
   if (!e_thumb_exists(file))
     {
	if (!e_thumb_create(file, width, height))
	  {
	     DEF_THUMB_RETURN;
	  }
     }
   
   thumb = e_thumb_file_get(file);
   if (!thumb)
     {
	DEF_THUMB_RETURN;
     }
   
   ef = eet_open(thumb, EET_FILE_MODE_READ);
   if (!ef)
     {
	eet_close(ef);
	free(thumb);
	DEF_THUMB_RETURN;
     }
   
   im = e_icon_add(evas);
   e_icon_file_key_set(im, thumb, "/thumbnail/data");
   if (shrink)
     {
	Evas_Coord sw, sh;
	
	e_icon_size_get(im, &sw, &sh);
	evas_object_resize(im, sw, sh);
     }
   e_icon_fill_inside_set(im, 1);
   free(thumb);
   eet_close(ef);
   return im;
}

/* return hash for a file */
static char *
_e_thumb_file_id(char *file)
{
   char                s[256], *sp;
   const char         *chmap =
     "0123456789abcdef"
     "ghijklmnopqrstuv"
     "wxyz`~!@#$%^&*()"
     "[];',.{}<>?-=_+|";
   unsigned int        id[4], i;
   struct stat         st;

   if (stat(file, &st) < 0)
     return NULL;
   
   id[0] = st.st_ino;
   id[1] = st.st_dev;
   id[2] = (st.st_size & 0xffffffff);
   id[3] = (st.st_mtime & 0xffffffff);

   sp = s;
   for (i = 0; i < 4; i++)
     {
	unsigned int t, tt;
	int j;

	t = id[i];
	j = 32;
	while (j > 0)
	  {
	     tt = t & ((1 << 6) - 1);
	     *sp = chmap[tt];
	     t >>= 6;
	     j -= 6;
	     sp++;
	  }
     }
   *sp = 0;
   return strdup(s);
}

/* generate a thumb from the list of queued thumbs */
static void
_e_thumb_generate(void)
{
   E_Thumb_Item *t;
   
   if ((!thumb_files) || (pid != -1)) return;
   pid = fork();
   if (pid == 0)
     {
	/* reset signal handlers for the child */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	
	t = thumb_files->data;
	if (!e_thumb_exists(t->path))
	  e_thumb_create(t->path, t->w, t->h);
	eet_clearcache();
	exit(0);
     }   
}

/* called when a thumb is generated */
static int
_e_thumb_cb_exe_exit(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   E_Thumb_Item         *t;
   char                 *ext;
   
   ev = event;
   if (ev->pid != pid) return 1;
   if (!thumb_files) return 1;
   
   t = thumb_files->data;
   thumb_files = evas_list_remove_list(thumb_files, thumb_files);
   
   ext = strrchr(t->path, '.');
   if ((ext) && (strcasecmp(ext, ".eap")))
     ext = NULL;
   
   if ((ext) || (ecore_file_exists(t->path)))
     {
	Evas_Coord w, h;
	Evas_Object *tmp;
	void *data;
	
	tmp = e_thumb_evas_object_get(t->path,
				      t->evas,
				      t->w,
				      t->h,
				      1);
	if (tmp && t)
	  {
	     data = e_icon_data_get(tmp, &w, &h);
	     e_icon_data_set(t->obj, data, w, h);
	     evas_object_del(tmp);
	     if(t->cb)
	       t->cb(t->obj, t->data);
	     free(t);
	  }
     }
   
   pid = -1;
   _e_thumb_generate();
   return 1;
}

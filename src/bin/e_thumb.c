/*
* vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
*/

#include "e.h"

static char       *_e_thumb_file_id(char *file);

static char       *thumb_path = NULL;

int
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
   
   return 1;
}

int
e_thumb_shutdown(void)
{
   E_FREE(thumb_path);
   return 1;
}

/* return dir where thumbs are saved */
char *
e_thumb_dir_get(void)
{
   return strdup(thumb_path);
}

/* return hashed path of thumb */
char *
e_thumb_file_get(char *file)
{
   char *id;
   char thumb[PATH_MAX];
   
   id = _e_thumb_file_id(file);   
   if(!id) { return NULL; }
   snprintf(thumb, sizeof(thumb), "%s/%s", thumb_path, id);
   free(id);
   return strdup(thumb);   
}

/* check wether a file has a saved thumb */
int
e_thumb_exists(char *file)
{
   char *thumb;
   int ret;
   
   thumb = e_thumb_file_get(file);
   ret = ecore_file_exists(thumb);
   free(thumb);
   
   return ret;     
}

/* create and save a thumb to disk */
int
e_thumb_create(char *file, Evas_Coord w, Evas_Coord h)
{
   Eet_File *ef;
   char *thumbpath;
   Evas_Object *im;
   const int *data;
   int size, iw, ih, ww, hh;
   Ecore_Evas *buf;
   Evas *evasbuf;
   int alpha;
   
   thumbpath = e_thumb_file_get(file);
   if (!thumbpath)
     {
	free(thumbpath);
	return -1;
     }
   
   ef = eet_open(thumbpath, EET_FILE_MODE_WRITE);
   if (!ef)
     {
	free(thumbpath);
	return -1;
     }
   
   free(thumbpath);
   
   buf = ecore_evas_buffer_new(w, h);
   evasbuf = ecore_evas_get(buf);
   im = evas_object_image_add(evasbuf);
   evas_object_image_file_set(im, file, NULL);
   iw = 0; ih = 0;
   evas_object_image_size_get(im, &iw, &ih);
   alpha = evas_object_image_alpha_get(im);
   ww = w;
   hh = (w * ih) / iw;
   if (hh > h)
     {
	hh = h;
	ww = (h * iw) / ih;
     }
   ecore_evas_resize(buf, ww, hh);
   evas_object_image_fill_set(im, 0, 0, ww, hh);
   evas_object_resize(im, ww, hh);
   evas_object_show(im);
   data = ecore_evas_buffer_pixels_get(buf);
   
   eet_write(ef, "/thumbnail/orig_path", file, strlen(file), 1);
   if ((size = eet_data_image_write(ef, "/thumbnail/data",
				    (void *)data, ww, hh, alpha,
				    0, 91, 1)) <= 0)
    {
       eet_close(ef);
       return -1;
    }
   evas_object_del(im);
   eet_close(ef);
   ecore_evas_free(buf);
   return 1;
}

/* get evas object containing image of the thumb */
Evas_Object *
e_thumb_evas_object_get(char *file, Evas *evas, Evas_Coord width, Evas_Coord height)
{
   Eet_File *ef;
   char *thumb, *ext;
   Evas_Object *im = NULL;
   void *data;
   unsigned int w, h;
   int a, c, q, l;
   
#define DEF_THUMB_RETURN im = evas_object_rectangle_add(evas); \
	       evas_object_color_set(im, 255, 255, 255, 255); \
	       evas_object_resize(im, width, height); \
	       return im 
   
   /* eap thumbnailer */
   ext = strrchr(file, '.');
   if(ext)
    {
       if(!strcasecmp(ext, ".eap"))
	{
	   E_App *app;
	   
	   app = e_app_new(file, 0);
	   
	   if(!app)	     
	    { 
	       DEF_THUMB_RETURN;
	    }
	   else
	    {
	       im = edje_object_add(evas);
	       edje_object_file_set(im, file, "icon");
	       e_object_unref(E_OBJECT(app));	       
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
   e_icon_fill_inside_set(im, 1);
   free(thumb);
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
   unsigned int        id[6], i;
   struct stat         st;

   if (stat(file, &st) < 0)
     return NULL;
   
   id[0] = (int)st.st_ino;
   id[1] = (int)st.st_dev;
   id[2] = (int)(st.st_size & 0xffffffff);
   id[3] = (int)((st.st_size >> 32) & 0xffffffff);
   id[4] = (int)(st.st_mtime & 0xffffffff);
   id[5] = (int)((st.st_mtime >> 32) & 0xffffffff);

   sp = s;
   for (i = 0; i < 6; i++)
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

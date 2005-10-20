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
   int size;
   Ecore_Evas *buf;
   Evas *evasbuf;
   
   thumbpath = e_thumb_file_get(file);
   if(!thumbpath) { free(thumbpath); return -1; }
   
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
   evas_object_image_fill_set(im, 0, 0, w, h);
   evas_object_resize(im, w, h);
   evas_object_show(im);
   data = ecore_evas_buffer_pixels_get(buf);
   
   if ((size = eet_data_image_write(ef, "/thumbnail/data", (void *)data, w, h, 1, 0, 70, 1)) < 0)
    {
       printf("e_thumb: BUG: Couldn't write thumb db\n");
       eet_close(ef);
       return -1;
    }
   
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
       if(!e_thumb_create(file, width, height))
	{
	   DEF_THUMB_RETURN;
	}
    }
      
   thumb = e_thumb_file_get(file);
   if(!thumb)
    {
       DEF_THUMB_RETURN;
    }
   
   ef = eet_open(thumb, EET_FILE_MODE_READ);
   if (!ef)
    {
       free(thumb);
       DEF_THUMB_RETURN;
    }
   
   free(thumb);
   
   data = eet_data_image_read(ef, "/thumbnail/data", &w, &h, &a, &c, &q, &l);
   if (data)
    {
       im = evas_object_image_add(evas);
       evas_object_image_alpha_set(im, 1);
       evas_object_image_size_set(im, w, h);
       evas_object_image_smooth_scale_set(im, 0);
       evas_object_image_data_copy_set(im, data);
       evas_object_image_data_update_add(im, 0, 0, w, h);
       evas_object_image_fill_set(im, 0, 0, w, h);
       evas_object_resize(im, w, h);
       free(data);
    }
   else
    {
       DEF_THUMB_RETURN;
    }
   
   eet_close(ef);
   return im;
}

/* return hash for a file */
static char *
_e_thumb_file_id(char *file)
{
   char                s[256];
   const char         *chmap =
     "0123456789abcdefghijklmnopqrstuvwxyz€‚ƒ„…†‡ˆŠ‹ŒŽ‘’“-_";
   int                 id[2];
   struct stat         st;

   if (stat(file, &st) < 0)
     return NULL;
   
   id[0] = (int)st.st_ino;
   id[1] = (int)st.st_dev;

   sprintf(s,
	   "%c%c%c%c%c%c"
	   "%c%c%c%c%c%c",
	   chmap[(id[0] >> 0) & 0x3f],
	   chmap[(id[0] >> 6) & 0x3f],
	   chmap[(id[0] >> 12) & 0x3f],
	   chmap[(id[0] >> 18) & 0x3f],
	   chmap[(id[0] >> 24) & 0x3f],
	   chmap[(id[0] >> 28) & 0x3f],
	   chmap[(id[1] >> 0) & 0x3f],
	   chmap[(id[1] >> 6) & 0x3f],
	   chmap[(id[1] >> 12) & 0x3f],
	   chmap[(id[1] >> 18) & 0x3f],
	   chmap[(id[1] >> 24) & 0x3f], chmap[(id[1] >> 28) & 0x3f]);

   return strdup(s);
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* IGNORE this code for now! */

typedef struct _E_Fm2_Smart_Data E_Fm2_Smart_Data;
typedef struct _E_Fm2_Region     E_Fm2_Region;
typedef struct _E_Fm2_Icon       E_Fm2_Icon;

struct _E_Fm2_Smart_Data
{
   Evas_Coord        x, y, w, h;
   Evas_Object      *obj;
   Evas_Object      *clip;
   char             *dev;
   char             *path;
   E_Fm2_View_Mode   view_mode;
   struct {
      Evas_Coord     w, h;
   } max;
   struct {
      Evas_Coord     x, y;
   } pos;
   struct {
      Evas_List     *list;
      int            member_max;
   } regions;
   Evas_List        *icons;
   Evas_List        *unsorted;
//   unsigned char     no_dnd : 1;
//   unsigned char     single_select : 1;
//   unsigned char     single_click : 1;
//   unsigned char     show_ext : 1;
};

struct _E_Fm2_Region
{
   E_Fm2_Smart_Data *smart;
   Evas_Coord        x, y, w, h;
   Evas_List        *list;
   unsigned char     realized : 1;
};

struct _E_Fm2_Icon
{
   E_Fm2_Smart_Data *smart;
   E_Fm2_Region     *region;
   Evas_Coord        x, y, w, h;
   Evas_Object      *obj;
   char             *file;
   char             *mime;
   unsigned char     realized : 1;
   unsigned char     selected : 1;
   unsigned char     thumb : 1;
//   unsigned char     single_click : 1;
};

static void                _e_fm2_smart_add(Evas_Object *object);
static void                _e_fm2_smart_del(Evas_Object *object);
static void                _e_fm2_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void                _e_fm2_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void                _e_fm2_smart_show(Evas_Object *object);
static void                _e_fm2_smart_hide(Evas_Object *object);
static void                _e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void                _e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void                _e_fm2_smart_clip_unset(Evas_Object *obj);

static char *_meta_path = NULL;
static Evas_Smart *_e_fm2_smart = NULL;

/* externally accessible functions */
EAPI int
e_fm2_init(void)
{
   char *homedir;
   char  path[PATH_MAX];

   homedir = e_user_homedir_get();
   if (homedir)
     {
	snprintf(path, sizeof(path), "%s/.e/e/fileman/metadata", homedir);
	ecore_file_mkpath(path);
	_meta_path = strdup(path);
	free(homedir);
     }
   else return 0;

   _e_fm2_smart = evas_smart_new("e_fm",
				 _e_fm2_smart_add, /* add */
				 _e_fm2_smart_del, /* del */
				 NULL, NULL, NULL, NULL, NULL,
				 _e_fm2_smart_move, /* move */
				 _e_fm2_smart_resize, /* resize */
				 _e_fm2_smart_show,/* show */
				 _e_fm2_smart_hide,/* hide */
				 _e_fm2_smart_color_set, /* color_set */
				 _e_fm2_smart_clip_set, /* clip_set */
				 _e_fm2_smart_clip_unset, /* clip_unset */
				 NULL); /* data*/
   return 1;
}

EAPI int
e_fm2_shutdown(void)
{
   evas_smart_free(_e_fm2_smart);
   _e_fm2_smart = NULL;
   E_FREE(_meta_path);
   return 1;
}

EAPI Evas_Object *
e_fm2_add(Evas *evas)
{
   return evas_object_smart_add(evas, _e_fm2_smart);
}

EAPI void
e_fm2_path_set(Evas_Object *obj, char *dev, char *path)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (sd->dev) free(sd->dev);
   if (sd->path) free(sd->path);
   // FIXME: nuke regions, icons etc. etc.
   // dev == some system device or virtual filesystem eg: /dev/sda1
   // or /dev/hdc or /dev/dvd, /dev/cdrom etc.
   // if it is NULL root fs is assumed. vfs's could be specified with
   // music: for example or photos: etc. and soem vfs definition specifies
   // how to map that vfs id to a list of files and subdirs
   if (dev) sd->dev = strdup(dev);
   sd->path = strdup(path);
   // FIXME: begin dir scan/build

}

EAPI void
e_fm2_path_get(Evas_Object *obj, char **dev, char **path)
{
   E_Fm2_Smart_Data *sd;

   if (dev) *dev = NULL;
   if (path) *path = NULL;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return; // safety
   if (!evas_object_type_get(obj)) return; // safety
   if (strcmp(evas_object_type_get(obj), "e_fm")) return; // safety
   if (dev) *dev = sd->dev;
   if (path) *path = sd->path;
}





/* local subsystem functions */
static void
_e_fm2_file_add(Evas_Object *obj, char *file)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* create icon obj and append to unsorted list */
}

static void
_e_fm2_file_del(Evas_Object *obj, char *file)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* find icon of file and remove from unsorted or main list */
}

static void
_e_fm2_queue_process(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take unsorted and insert into the regions */
}

static void
_e_fm2_regions_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* free up all regions */
}

static void
_e_fm2_regions_populate(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* take the icon list and split into regions */
}

static void
_e_fm2_icons_free(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   /* free all icons */
}


static void
_e_fm2_smart_add(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = E_NEW(E_Fm2_Smart_Data, 1);
   if (!sd) return;
   sd->obj = obj;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, sd->x, sd->y);
   evas_object_resize(sd->clip, sd->w, sd->h);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, 0, 0);
}

static void
_e_fm2_smart_del(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_fm2_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->x == x) || (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->clip, x, y);
}

static void
_e_fm2_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->w == w) || (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->clip, w, h);
}

static void
_e_fm2_smart_show(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_show(sd->clip);
}

static void
_e_fm2_smart_hide(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_fm2_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_fm2_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_fm2_smart_clip_unset(Evas_Object *obj)
{
   E_Fm2_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}

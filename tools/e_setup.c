#include <Evas.h>
#include <Ebits.h>
#include <Ecore.h>
#include <Edb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include "../config.h"

#ifndef PATH_MAX
#define PATH_MAX     4096
#endif

/* stuff we want from e */
#define OBJ_PROPERTIES \
int references; \
void (*e_obj_free) (void *e_obj);
#define OBJ_INIT(_e_obj, _e_obj_free_func) \
{ \
  _e_obj->references = 1; \
  _e_obj->e_obj_free = (void *) _e_obj_free_func; \
}
#define OBJ_REF(_e_obj) _e_obj->references++
#define OBJ_UNREF(_e_obj) _e_obj->references--
#define OBJ_IF_FREE(_e_obj) if (_e_obj->references == 0)
#define OBJ_FREE(_e_obj) _e_obj->e_obj_free(_e_obj);
#define OBJ_DO_FREE(_e_obj) \
{ \
  OBJ_UNREF(_e_obj); \
  OBJ_IF_FREE(_e_obj) \
    { \
      OBJ_FREE(_e_obj); \
    } \
}
#define e_strdup(__dest, __var) \
{ \
if (!__var) __dest = NULL; \
else { \
__dest = malloc(strlen(__var) + 1); \
if (__dest) strcpy(__dest, __var); \
} }

typedef struct _E_Background          E_Background;
typedef struct _E_Background_Layer    E_Background_Layer;

struct _E_Background
{
   OBJ_PROPERTIES;
   
   Evas  evas;
   char *file;
   
   struct {
      int sx, sy;
      int w, h;
   } geom;
   
   Evas_List layers;
   
   Evas_Object base_obj;
};


struct _E_Background_Layer
{
   int mode;
   int type;
   int inlined;
   struct {
      float x, y;
   } scroll;
   struct {
      float x, y;
   } pos;
   struct {
      float w, h;
      struct {
	 int w, h;
      } orig;
   } size, fill;
   char *color_class;
   char *file;
   double angle;
   struct {
      int r, g, b, a;
   } fg, bg;
   
   double x, y, w, h, fw, fh;
   
   Evas_Object obj;
};


void           e_background_free(E_Background *bg);
E_Background  *e_background_new(void);
E_Background  *e_background_load(char *file);
void           e_background_realize(E_Background *bg, Evas evas);
void           e_background_set_scroll(E_Background *bg, int sx, int sy);
void           e_background_set_size(E_Background *bg, int w, int h);
void           e_background_set_color_class(E_Background *bg, char *cc, int r, int g, int b, int a);

void
e_background_free(E_Background *bg)
{
   Evas_List l;
   
   if (bg->layers)
     {
	for (l = bg->layers; l; l = l->next)
	  {
	     E_Background_Layer *bl;
	     
	     bl = l->data;
	     if (bl->color_class) FREE(bl->color_class);
	     if (bl->file) FREE(bl->file);
	     if (bl->obj) evas_del_object(bg->evas, bl->obj);
	     FREE(bl);
	  }
	evas_list_free(bg->layers);
     }
   if (bg->file) FREE (bg->file);
   if (bg->base_obj) evas_del_object(bg->evas, bg->base_obj);
   FREE(bg);   
}

E_Background *
e_background_new(void)
{
   E_Background *bg;
   
   bg = NEW(E_Background, 1);
   ZERO(bg, E_Background, 1);
   OBJ_INIT(bg, e_background_free);
   
   return bg;
}

E_Background *
e_background_load(char *file)
{
   E_Background *bg;
   E_DB_File *db;
   int i, num;
   
   db = e_db_open_read(file);
   if (!db) return NULL;
   num = 0;
   e_db_int_get(db, "/type/bg", &num);
   if (num != 1)
     {
	e_db_close(db);
	e_db_flush();
	return NULL;
     }
   e_db_int_get(db, "/layers/count", &num);
   
   bg = e_background_new();
   e_strdup(bg->file, file);
   for (i = 0; i < num; i++)
     {
	E_Background_Layer *bl;
	char buf[4096];
	
	bl = NEW(E_Background_Layer, 1);
	ZERO(bl, E_Background_Layer, 1);
	bg->layers = evas_list_append(bg->layers, bl);
	
	sprintf(buf, "/layers/%i/type", i); e_db_int_get(db, buf, &(bl->type));
	sprintf(buf, "/layers/%i/inlined", i); e_db_int_get(db, buf, &(bl->inlined));
	sprintf(buf, "/layers/%i/color_class", i); bl->color_class = e_db_str_get(db, buf);
	if (bl->inlined)
	  {
	     sprintf(buf, "%s:/layers/%i/image", file, i); e_strdup(bl->file, buf);
	  }
	else
	  {
	     sprintf(buf, "/layers/%i/file", i); bl->file = e_db_str_get(db, buf);
	  }
	sprintf(buf, "/layers/%i/scroll.x", i); e_db_float_get(db, buf, &(bl->scroll.x));
	sprintf(buf, "/layers/%i/scroll.y", i); e_db_float_get(db, buf, &(bl->scroll.y));
	sprintf(buf, "/layers/%i/pos.x", i); e_db_float_get(db, buf, &(bl->pos.x));
	sprintf(buf, "/layers/%i/pos.y", i); e_db_float_get(db, buf, &(bl->pos.y));
	sprintf(buf, "/layers/%i/size.w", i); e_db_float_get(db, buf, &(bl->size.w));
	sprintf(buf, "/layers/%i/size.h", i); e_db_float_get(db, buf, &(bl->size.h));
	sprintf(buf, "/layers/%i/size.orig.w", i); e_db_int_get(db, buf, &(bl->size.orig.w));
	sprintf(buf, "/layers/%i/size.orig.h", i); e_db_int_get(db, buf, &(bl->size.orig.h));
	sprintf(buf, "/layers/%i/fill.w", i);  e_db_float_get(db, buf, &(bl->fill.w));
	sprintf(buf, "/layers/%i/fill.h", i);  e_db_float_get(db, buf, &(bl->fill.h));
	sprintf(buf, "/layers/%i/fill.orig.w", i); e_db_int_get(db, buf, &(bl->fill.orig.w));
	sprintf(buf, "/layers/%i/fill.orig.h", i); e_db_int_get(db, buf, &(bl->fill.orig.h));
	sprintf(buf, "/layers/%i/angle", i);  e_db_float_get(db, buf, (float*)&(bl->angle));
	sprintf(buf, "/layers/%i/fg.r", i); e_db_int_get(db, buf, &(bl->fg.r));
	sprintf(buf, "/layers/%i/fg.g", i); e_db_int_get(db, buf, &(bl->fg.g));
	sprintf(buf, "/layers/%i/fg.b", i); e_db_int_get(db, buf, &(bl->fg.b));
	sprintf(buf, "/layers/%i/fg.a", i); e_db_int_get(db, buf, &(bl->fg.a));
	sprintf(buf, "/layers/%i/bg.r", i); e_db_int_get(db, buf, &(bl->bg.r));
	sprintf(buf, "/layers/%i/bg.g", i); e_db_int_get(db, buf, &(bl->bg.g));
	sprintf(buf, "/layers/%i/bg.b", i); e_db_int_get(db, buf, &(bl->bg.b));
	sprintf(buf, "/layers/%i/bg.a", i); e_db_int_get(db, buf, &(bl->bg.a));
     }
   return bg;
}

void
e_background_realize(E_Background *bg, Evas evas)
{
   Evas_List l;
   int ww, hh, count;

   if (bg->evas) return;
   bg->evas = evas;
   if (!bg->evas) return;
   for (count = 0, l = bg->layers; l; l = l->next, count++)
     {
	E_Background_Layer *bl;
	
	bl = l->data;
	if (bl->type == 0) /* 0 == image */
	  {
	     bl->obj = evas_add_image_from_file(bg->evas, bl->file);
	     evas_set_layer(bg->evas, bl->obj, 0);
	     evas_show(bg->evas, bl->obj);
#if 0 /* dont need this... do we? */
	     if (evas_get_image_alpha(bg->evas, bl->obj))
	       {
		  printf("Adding rectangle to bg!\n");
		  bg->base_obj = evas_add_rectangle(bg->evas);
		  evas_lower(bg->evas, bg->base_obj);
		  evas_move(bg->evas, bg->base_obj, 0, 0);
		  evas_resize(bg->evas, bg->base_obj, 999999999, 999999999);
		  evas_set_color(bg->evas, bg->base_obj, 255, 255, 255, 255);
		  evas_show(bg->evas, bg->base_obj);
	       }
#endif
	  }
	else if (bl->type == 1) /* 1 == gradient */
	  {
	  }
	else if (bl->type == 2) /* 2 == solid */
	  {
	  }
     }
   ww = bg->geom.w;
   hh = bg->geom.h;
   bg->geom.w = 0;
   bg->geom.h = 0;
   e_background_set_size(bg, ww, hh);

}

void
e_background_set_scroll(E_Background *bg, int sx, int sy)
{
   Evas_List l;
   
   if ((bg->geom.sx == sx) && (bg->geom.sy == sy)) return;
   bg->geom.sx = sx;
   bg->geom.sy = sy;
   if (!bg->evas) return;
   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;
	
	bl = l->data;
	if (bl->type == 0) /* 0 == image */
	  {
	     evas_set_image_fill(bg->evas, bl->obj, 
				 (double)bg->geom.sx * bl->scroll.x, 
				 (double)bg->geom.sy * bl->scroll.y,
				 bl->fw, bl->fh);
	  }
     }
}

void
e_background_set_size(E_Background *bg, int w, int h)
{
   Evas_List l;
   
   if ((bg->geom.w == w) && (bg->geom.h == h)) return;
   bg->geom.w = w;
   bg->geom.h = h;
   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;
	double x, y, w, h, fw, fh;
	int iw, ih;
	
	bl = l->data;
	iw = 0;
	ih = 0;
	if (bg->evas) evas_get_image_size(bg->evas, bl->obj, &iw, &ih);
	w = bl->size.w * (double)bg->geom.w;
	h = bl->size.h * (double)bg->geom.h;
	if (bl->size.orig.w) w = (double)iw * bl->size.w;
	if (bl->size.orig.h) h = (double)ih * bl->size.h;
	fw = bl->fill.w * w;
	fh = bl->fill.h * h;
	if (bl->fill.orig.w) fw = (double)iw * bl->fill.w;
	if (bl->fill.orig.h) fh = (double)ih * bl->fill.h;
	x = ((double)bg->geom.w - w + 1) * bl->pos.x;
	y = ((double)bg->geom.h - h + 1) * bl->pos.y;
	bl->x = x;
	bl->y = y;
	bl->w = w;
	bl->h = h;
	bl->fw = fw;
	bl->fh = fh;
	if (bg->evas)
	  {
	     evas_move(bg->evas, bl->obj, bl->x, bl->y);
	     evas_resize(bg->evas, bl->obj, bl->w, bl->h);
	     if (bl->type == 0) /* 0 == image */
	       {
		  evas_set_image_fill(bg->evas, bl->obj,
				      (double)bg->geom.sx * bl->scroll.x,
				      (double)bg->geom.sy * bl->scroll.y,
				      bl->fw, bl->fh);
	       }
	     else if (bl->type == 1) /* 1 == gradient */
	       {
		  evas_set_angle(bg->evas, bl->obj, bl->angle);
	       }
	     else if (bl->type == 2) /* 2 == solid */
	       {
	       }
	  }
     }
}

void
e_background_set_color_class(E_Background *bg, char *cc, int r, int g, int b, int a)
{
   Evas_List l;
   
   for (l = bg->layers; l; l = l->next)
     {
	E_Background_Layer *bl;
	
	bl = l->data;
	if ((bl->color_class) && (cc) && (!strcmp(bl->color_class, cc)))
	  {
	     if (bg->evas)
	       {
		  if ((l == bg->layers) && (bg->base_obj))
		    evas_set_color(bg->evas, bl->obj, r, g, b, 255);
		  else
		    evas_set_color(bg->evas, bl->obj, r, g, b, a);
	       }
	  }
     }
}

/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */

int
e_file_exists(char *file)
{
   struct stat         st;
   
   if (stat(file, &st) < 0) return 0;
   return 1;
}

int
e_file_is_dir(char *file)
{
   struct stat         st;
   
   if (stat(file, &st) < 0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

char *
e_file_home(void)
{
   static char *home = NULL;
   
   if (home) return home;
   home = getenv("HOME");
   if (!home) home = getenv("TMPDIR");
   if (!home) home = "/tmp";
   return home;
}

static mode_t       default_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

int
e_file_mkdir(char *dir)
{
   if (mkdir(dir, default_mode) < 0) return 0;
   return 1;
}

void
e_file_mkdirs(char *s)
{
   char ss[PATH_MAX];
   int  i, ii;
   
   i = 0;
   ii = 0;
   while (s[i])
     {
	ss[ii++] = s[i];
	ss[ii] = 0;
	if (s[i] == '/')
	  {
	     if (!e_file_exists(ss))
	       e_file_mkdir(ss);
	     else if (!e_file_is_dir(ss))
	       return;
	  }
	i++;
     }
}

int
e_file_cp(char *src, char *dst)
{
   FILE *f1, *f2;
   char buf[16384];
   size_t num;
   
   f1 = fopen(src, "rb");
   if (!f1) return 0;
   f2 = fopen(dst, "wb");
   if (!f2)
     {
	fclose(f1);
	return 0;
     }
   while ((num = fread(buf, 1, 16384, f1)) > 0) fwrite(buf, 1, num, f2);
   fclose(f1);
   fclose(f2);
   return 1;
}

void
e_file_delete(char *s)
{
   unlink(s);
}

void
e_file_rename(char *s, char *ss)
{
   rename(s, ss);
}

int
e_glob_matches(char *str, char *glob)
{
   if (!fnmatch(glob, str, 0)) return 1;
   return 0;
}

char *
e_file_get_file(char *file)
{
   char *p;
   char *f;
   
   p = strrchr(file, '/');
   if (!p)
     {
	e_strdup(f, file);
	return f;
     }
   e_strdup(f, &(p[1]));
   return f;
}

char *
e_file_get_dir(char *file)
{
   char *p;
   char *f;
   char buf[PATH_MAX];
   
   strcpy(buf, file);
   p = strrchr(buf, '/');
   if (!p)
     {
	e_strdup(f, file);
	return f;
     }
   *p = 0;
   e_strdup(f, buf);
   return f;
}

int
e_file_can_exec(struct stat *st)
{
   static int have_uid = 0;
   static uid_t uid = -1;
   static gid_t gid = -1;
   int ok;
   
   if (!st) return 0;
   ok = 0;
   if (!have_uid) uid = getuid();
   if (!have_uid) gid = getgid();
   have_uid = 1;
   if (st->st_uid == uid)
     {
	if (st->st_mode & S_IXUSR) ok = 1;
     }
   else if (st->st_gid == gid)
     {
	if (st->st_mode & S_IXGRP) ok = 1;
     }
   else
     {
	if (st->st_mode & S_IXOTH) ok = 1;
     }
   return ok;
}

char *
e_file_link(char *link)
{
   char buf[PATH_MAX];
   char *f;
   int count;
   
   if ((count = readlink(link, buf, sizeof(buf))) < 0) return NULL;
   buf[count] = 0;
   e_strdup(f, buf);
   return f;
}

Evas_List
e_file_list_dir(char *dir)
{
   DIR                *dirp;
   struct dirent      *dp;
   Evas_List           list;
   
   dirp = opendir(dir);
   if (!dirp) return NULL;
   list = NULL;
   while ((dp = readdir(dirp)))
     {
	if ((strcmp(dp->d_name, ".")) &&
	    (strcmp(dp->d_name, "..")))
	  {
	     Evas_List l;
	     char *f;
	     
	     /* insertion sort */
	     for (l = list; l; l = l->next)
	       {
		  if (strcmp(l->data, dp->d_name) > 0)
		    {
		       e_strdup(f, dp->d_name);
		       list = evas_list_prepend_relative(list, f, l->data);
		       break;
		    }
	       }
	     /* nowhwre to go? just append it */
	     e_strdup(f, dp->d_name);
	     if (!l) list = evas_list_append(list, f);
	  }
     }
   closedir(dirp);
   return list;
}


/*                                                                           */
/*                                                                           */
/*                                                                           */

typedef struct _text_zone Text_Zone;

struct _text_zone
{
   double x, y;

   Evas_Object clip;
   Ebits_Object *bg;
   
   struct {
      double x, y, w, h;
   } l;
   
   struct {
      double dx, dy;
      int go;
   } move;
   Evas_List lines;
};

Window win_main;
Window win_evas;
Evas   evas;
double scr_w, scr_h;
Evas_Object pointer;

/* our stuff */
void idle(void *data);
void window_expose(Ecore_Event * ev);
void mouse_move(Ecore_Event * ev);
void mouse_down(Ecore_Event * ev);
void mouse_up(Ecore_Event * ev);
void key_down(Ecore_Event * ev);
void setup(void);
Text_Zone *txz_new(double x, double y, char *text);
void txz_free(Text_Zone *txz);
void txz_show(Text_Zone *txz);
void txz_hide(Text_Zone *txz);
void txz_move(Text_Zone *txz, double x, double y);
void txz_text(Text_Zone *txz, char *text);
void animate_logo(int v, void *data);

void
idle(void *data)
{
   evas_render(evas);
}

void
window_expose(Ecore_Event * ev)
{
   Ecore_Event_Window_Expose      *e;
   
   e = (Ecore_Event_Window_Expose *)ev->event;
   evas_update_rect(evas, e->x, e->y, e->w, e->h);
}

void
mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move      *e;
   
   e = (Ecore_Event_Mouse_Move *)ev->event;
   evas_move(evas, pointer,
	     evas_screen_x_to_world(evas, e->x),
	     evas_screen_y_to_world(evas, e->y));
   evas_event_move(evas, e->x, e->y);
}

void
mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Down      *e;
   
   e = (Ecore_Event_Mouse_Down *)ev->event;
   evas_event_button_down(evas, e->x, e->y, e->button);
}

void
mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up      *e;
   
   e = (Ecore_Event_Mouse_Up *)ev->event;
   evas_event_button_up(evas, e->x, e->y, e->button);
}

void
key_down(Ecore_Event * ev)
{
   Ecore_Event_Key_Down          *e;
   
   e = ev->event;
   if (!strcmp(e->key, "Escape"))
     {
	exit(0);
     }
}

void
setup(void)
{
   int root_w, root_h;
   E_Background *bg;

   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, window_expose);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE,    mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN,    mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP,      mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN,      key_down);
   
   ecore_event_filter_idle_handler_add(idle, NULL);
   
   ecore_window_get_geometry(0, NULL, NULL, &root_w, &root_h);
   win_main = ecore_window_override_new(0, 0, 0, root_w, root_h);
   evas = evas_new_all(ecore_display_get(), 
		       win_main, 
		       0, 0, root_w, root_w, 
		       RENDER_METHOD_ALPHA_SOFTWARE,
		       216,   1024 * 1024,   16 * 1024 * 1024,
		       PACKAGE_DATA_DIR"/data/fonts/");
   
   bg = e_background_load(PACKAGE_DATA_DIR"/data/setup/setup.bg.db");
   if (!bg)
     {
	/* FIXME: must detect this error better and tell user */
	printf("ERROR: Enlightenment not installed properly\n");
	exit(-1);
     }
   e_background_realize(bg, evas);
   e_background_set_size(bg, root_w, root_h);
   
   pointer = evas_add_image_from_file(evas, PACKAGE_DATA_DIR"/data/setup/pointer.png");
   evas_set_pass_events(evas, pointer, 1);
   evas_set_layer(evas, pointer, 1000000);
   evas_show(evas, pointer);
      
   win_evas = evas_get_window(evas);   
   ecore_window_set_events(win_evas, XEV_EXPOSE | XEV_BUTTON | XEV_MOUSE_MOVE | XEV_KEY);
   ecore_set_blank_pointer(win_evas);
   
   ecore_window_show(win_evas);
   ecore_window_show(win_main);
   ecore_keyboard_grab(win_evas);
   
     {
	Evas_Object o;
	int w, h;
	
	o = evas_add_image_from_file(evas, PACKAGE_DATA_DIR"/data/setup/logo.png");
	evas_get_image_size(evas, o, &w, &h);
	evas_move(evas, o, (root_w - w) / 2, -32);
	evas_set_layer(evas, o, 20);
	evas_show(evas, o);
	evas_set_pass_events(evas, o, 1);
     }
     {
	Text_Zone *txz;
	
	txz = txz_new
	  ((root_w - 512) / 2, 130,
	   "9c Enlightenment\n"
	   "5c \n"
	   "5c Welcome to Enlightenment 0.17 (pre-release). This is the setup\n"
	   "5c program. It will help you get a base configuration initialised\n"
	   "5c for your user and do some initial tweaks and system queries.\n"
	   "5c \n"
	   "5c Please be patient and read the dialogs carefully, as your answers\n"
	   "5c to questions posed will affect your initial setup of Enlightenment,\n"
	   "5c and so your initial impressions.\n"
	   "5c \n"
	   "5c N.B. - during pre-release stages, this setup program may come up\n"
	   "5c more than just once, as new setups need to be installed\n"
	   );
     }
     {
	Evas_Object o;
	
	o = evas_add_image_from_file(evas, PACKAGE_DATA_DIR"/data/setup/anim/e001.png");
	evas_move(evas, o, root_w - 120, -15);
	evas_set_layer(evas, o, 30);
	evas_show(evas, o);
	animate_logo(0, o);
	evas_set_pass_events(evas, o, 1);
     }
   scr_w = root_w;
   scr_h = root_h;
}

static void
_txz_cb_show(void *data)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     evas_show(evas, l->data);
	  }
	evas_show(evas, txz->clip);
     }
}

static void
_txz_cb_hide(void *data)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     evas_hide(evas, l->data);
	  }
	evas_hide(evas, txz->clip);
     }
}

static void
_txz_cb_move(void *data, double x, double y)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   txz->l.x = x;
   txz->l.y = y;
   txz_adjust_txt(txz);
}

static void
_txz_cb_resize(void *data, double w, double h)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   txz->l.w = w;
   txz->l.h = h;
   txz_adjust_txt(txz);
}

static void
_txz_cb_raise(void *data)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     evas_raise(evas, l->data);
	  }
     }
}

static void
_txz_cb_lower(void *data)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     evas_lower(evas, l->data);
	  }
     }
}

static void
_txz_cb_set_layer(void *data, int lay)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     evas_set_layer(evas, l->data, lay);
	  }
     }
}

static void
_txz_cb_get_min_size(void *data, double *minw, double *minh)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   *minw = 0;
   *minh = 0;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     double w, h;
	     
	     evas_get_geometry(evas, l->data, NULL, NULL, &w, &h);
	     if (w > *minw) *minw = w;
	     *minh += h;
	  }
     }
   *minw += 8;
   *minh += 8;
}

static void
_txz_cb_get_max_size(void *data, double *maxw, double *maxh)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   *maxw = 0;
   *maxh = 0;
     {
	Evas_List l;
	
	for (l = txz->lines; l; l = l->next)
	  {
	     double w, h;
	     
	     evas_get_geometry(evas, l->data, NULL, NULL, &w, &h);
	     if (w > *maxw) *maxw = w;
	     *maxh += h;
	  }
     }
   *maxw += 8;
   *maxh += 8;
}

static void
_txz_cb_title_down(void *_data, Ebits_Object _o,
		   char *_c, int _b, int _x, int _y,
		   int _ox, int _oy, int _ow, int _oh)
{
   Text_Zone *txz;
   
   txz = _data;
   txz->move.go = 1;
   txz->move.dx = _x - txz->x;
   txz->move.dy = _y - txz->y;
}

static void
_txz_cb_title_up(void *_data, Ebits_Object _o,
		   char *_c, int _b, int _x, int _y,
		   int _ox, int _oy, int _ow, int _oh)
{
   Text_Zone *txz;
   
   txz = _data;
   txz->move.go = 0;
}

static void
_txz_cb_title_move(void *_data, Ebits_Object _o,
		   char *_c, int _b, int _x, int _y,
		   int _ox, int _oy, int _ow, int _oh)
{
   Text_Zone *txz;
   
   txz = _data;
   if (txz->move.go)
     {
	txz_move(txz, _x - txz->move.dx, _y - txz->move.dy);
     }
}

void
txz_adjust_txt(Text_Zone *txz)
{
   Evas_List l;
   double ypos;
   
   ypos = txz->l.y + 4;
   evas_move(evas, txz->clip, txz->l.x, txz->l.y);
   evas_resize(evas, txz->clip, txz->l.w, txz->l.h);
   for (l = txz->lines; l; l = l->next)
     {  
	Evas_Object o;
        double tw, th;
	double x;
	char align;
	
	o = l->data;
	align = (char)evas_get_data(evas, o, "align");
	x = txz->l.x + 4;
        tw = evas_get_text_width(evas, o);
	th = evas_get_text_height(evas, o);
	if (align == 'c') x = txz->l.x + 4 + ((txz->l.w - 8 - tw) / 2);
	else if (align == 'r') x = txz->l.x + 4 + (txz->l.w - 8 - tw);
	evas_move(evas, o, x, ypos);
	ypos += th;
     }
}

Text_Zone *
txz_new(double x, double y, char *text)
{
   Text_Zone *txz;
   
   txz = NEW(Text_Zone, 1);
   ZERO(txz, Text_Zone, 1);
   
   txz->x = 0;
   txz->y = 0;

   txz->clip = evas_add_rectangle(evas);
   evas_set_color(evas, txz->clip, 255, 255, 255, 255);
   txz->bg = ebits_load(PACKAGE_DATA_DIR"/data/setup/textzone.bits.db");
   if (txz->bg)
     {
	ebits_add_to_evas(txz->bg, evas);
	ebits_set_layer(txz->bg, 9);
	ebits_set_named_bit_replace(txz->bg, "Contents",
				    _txz_cb_show,
				    _txz_cb_hide,
				    _txz_cb_move,
				    _txz_cb_resize,
				    _txz_cb_raise,
				    _txz_cb_lower,
				    _txz_cb_set_layer,
				    NULL,
				    NULL,
				    _txz_cb_get_min_size,
				    _txz_cb_get_max_size,
				    txz);
	ebits_set_classed_bit_callback(txz->bg, "Title_Bar",
				       CALLBACK_MOUSE_DOWN, 
				       _txz_cb_title_down,
				       txz);
	ebits_set_classed_bit_callback(txz->bg, "Title_Bar",
				       CALLBACK_MOUSE_UP, 
				       _txz_cb_title_up,
				       txz);
	ebits_set_classed_bit_callback(txz->bg, "Title_Bar",
				       CALLBACK_MOUSE_MOVE, 
				       _txz_cb_title_move,
				       txz);
     }
   
   txz_text(txz, text);
   txz_move(txz, x, y);
   txz_show(txz);
   return txz;
}

void
txz_free(Text_Zone *txz)
{
   Evas_List l;
   
   if (txz->bg)
     ebits_free(txz->bg);
   evas_del_object(evas, txz->clip);
   for (l = txz->lines; l; l = l->next)
     evas_del_object(evas, (Evas_Object)l->data);
   if (txz->lines) evas_list_free(txz->lines);
   FREE(txz);
}

void
txz_show(Text_Zone *txz)
{
   Evas_List l;
   
   if (txz->bg) 
     ebits_show(txz->bg);
   for (l = txz->lines; l; l = l->next)
     evas_show(evas, (Evas_Object)l->data);
}

void
txz_hide(Text_Zone *txz)
{
   Evas_List l;
   
   if (txz->bg)
     ebits_hide(txz->bg);
   for (l = txz->lines; l; l = l->next)
     evas_hide(evas, (Evas_Object)l->data);
}

void
txz_move(Text_Zone *txz, double x, double y)
{
   Evas_List l;
   
   txz->x = x;
   txz->y = y;
   if (txz->bg)
     {	
	ebits_move(txz->bg, txz->x, txz->y);
     }
}

void
txz_text(Text_Zone *txz, char *text)
{
   char *p, *tok;
   double ypos;
   Evas_List l;
   
   for (l = txz->lines; l; l = l->next)
     evas_del_object(evas, (Evas_Object)l->data);
   if (txz->lines) evas_list_free(txz->lines);
   txz->lines = NULL;
   
   p = text;
   ypos = txz->y;
   while ((p[0] != 0) && (tok = strchr(p, '\n')))
     {
	char line[4096], size[2], align[2], *str;
	int sz;
	double tw, th, hadv, vadv;
	Evas_Object o;
	
	strncpy(line, p, (tok - p));
	line[tok - p] = 0;
	size[0] = line[0];
	size[1] = 0;
	align[0] = line[1];
	align[1] = 0;
	str = &(line[3]);
	
	sz = atoi(size);
	sz = 4 + (sz * 2);
	o = evas_add_text(evas, "nationff", sz, str);
	evas_set_layer(evas, o, 9);
	evas_set_color(evas, o, 0, 0, 0, 255);
	txz->lines = evas_list_append(txz->lines, o);
	tw = evas_get_text_width(evas, o);
	th = evas_get_text_height(evas, o);
	evas_text_get_advance(evas, o, &hadv, &vadv);
	if      (align[0] == 'l')
	  evas_move(evas, o, txz->x, ypos);
	else if (align[0] == 'r')
	  evas_move(evas, o, txz->x + 512 - tw, ypos);
	else
	  evas_move(evas, o, txz->x + ((512 - tw) / 2), ypos);
	evas_set_clip(evas, o, txz->clip);
	evas_put_data(evas, o, "align", (void *)align[0]);
	ypos += th;
	
	p = tok + 1;
     }
   if (txz->bg)
     {
	int minw, minh;
	
	ebits_get_real_min_size(txz->bg, &minw, &minh);	
	ebits_resize(txz->bg, minw + 8, minh + 8);
	ebits_hide(txz->bg);
	ebits_show(txz->bg);
	ebits_set_layer(txz->bg, 0);
	ebits_set_layer(txz->bg, 9);
     }
}

void
animate_logo(int v, void *data)
{
   Evas_Object o;
   double t;
   static double start_t;
   char buf[4096];
   int frame;
   
   o = (Evas_Object)data;
   if (v == 0) start_t = ecore_get_time();
   t = ecore_get_time() - start_t;
   frame = (int)(t * 25);
   frame = frame % 120;
   frame++;
   if      (frame < 10)   sprintf(buf, PACKAGE_DATA_DIR"/data/setup/anim/e00%i.png", frame);
   else if (frame < 100)  sprintf(buf, PACKAGE_DATA_DIR"/data/setup/anim/e0%i.png", frame);
   else if (frame < 1000) sprintf(buf, PACKAGE_DATA_DIR"/data/setup/anim/e%i.png", frame);
   evas_set_image_file(evas, o, buf);   
   ecore_add_event_timer("animate_logo", 0.01, animate_logo, 1, data);
}

int
main(int argc, char **argv)
{
   ecore_display_init(NULL);
   ecore_event_signal_init();
   ecore_event_filter_init();
   ecore_event_x_init();
   
   setup();
   
   ecore_event_loop();   
}

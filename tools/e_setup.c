#include <Evas.h>
#include <Ebits.h>
#include <Ecore.h>
#include <Edb.h>
#include <Ebg.h>
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

void
e_file_list_dir_free(Evas_List list)
{
   while (list)
     {
	FREE(list->data);
	list = evas_list_remove(list, list->data);
     }
}

/*                                                                           */
/*                                                                           */
/*                                                                           */

typedef struct _text_zone Text_Zone;
typedef struct _text_zone_button Text_Zone_Button;

struct _text_zone
{
   double x, y;

   Evas_Object clip;
   Ebits_Object *bg;
   
   struct {
      double x, y, w, h;
   } l;
   struct {
      double x, y, w, h;
   } b;
   
   struct {
      double dx, dy;
      int go;
   } move;
   Evas_List lines;
   Evas_List buttons;
};

struct _text_zone_button
{
   Evas_Object   label;
   Ebits_Object  *bg;
   
   double        x, y, w, h;
   void         (*func) (void *data);
   void          *func_data;
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
void child_exit(Ecore_Event *ev);
void setup(void);
Text_Zone *txz_new(double x, double y, char *text);
void txz_free(Text_Zone *txz);
void txz_show(Text_Zone *txz);
void txz_hide(Text_Zone *txz);
void txz_move(Text_Zone *txz, double x, double y);
void txz_text(Text_Zone *txz, char *text);
void txz_button(Text_Zone *txz, char *text, void (*func) (void *data), void *data);
void txz_adjust_txt(Text_Zone *txz);
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
child_exit(Ecore_Event *ev)
{
   Ecore_Event_Child *e;
   
   e = ev->event;
/*   
   e->pid;
   e->exit_code;
 */
}

void
setup(void)
{
   int root_w, root_h;
   E_Background bg;

   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, window_expose);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE,    mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN,    mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP,      mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN,      key_down);
   ecore_event_filter_handler_add(ECORE_EVENT_CHILD,         child_exit);
   
   ecore_event_filter_idle_handler_add(idle, NULL);
   
   ecore_window_get_geometry(0, NULL, NULL, &root_w, &root_h);
   win_main = ecore_window_override_new(0, 0, 0, root_w, root_h);
   evas = evas_new_all(ecore_display_get(), 
		       win_main, 
		       0, 0, root_w, root_w, 
		       RENDER_METHOD_ALPHA_SOFTWARE,
		       216,   1024 * 1024,   16 * 1024 * 1024,
		       PACKAGE_DATA_DIR"/data/fonts/");
   
   bg = e_bg_load(PACKAGE_DATA_DIR"/data/setup/setup.bg.db");
   if (!bg)
     {
	/* FIXME: must detect this error better and tell user */
	printf("ERROR: Enlightenment not installed properly\n");
	exit(-1);
     }
   e_bg_add_to_evas(bg, evas);
   e_bg_resize(bg, root_w, root_h);
   e_bg_show(bg);	
   
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
	   "7c Enlightenment\n"
	   "4c \n"
	   "4c Welcome to Enlightenment 0.17 (pre-release). This is the setup\n"
	   "4c program. It will help you get a base configuration initialised\n"
	   "4c for your user and do some initial tweaks and system queries.\n"
	   "4c \n"
	   "4c Please be patient and read the dialogs carefully, as your answers\n"
	   "4c to questions posed will affect your initial setup of Enlightenment,\n"
	   "4c and so your initial impressions.\n"
	   "4c \n"
	   "4c N.B. - during pre-release stages, this setup program may come up\n"
	   "4c more than just once, as new setups need to be installed\n"
	   );
	txz_button(txz, "OK", NULL, NULL);
	txz_button(txz, "Cancel", NULL, NULL);
	
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
}

static void
_txz_cb_resize(void *data, double w, double h)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   txz->l.w = w;
   txz->l.h = h;
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
_txz_cb_tb_move(void *data, double x, double y)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   txz->b.x = x;
   txz->b.y = y;
}

static void
_txz_cb_tb_resize(void *data, double w, double h)
{
   Text_Zone *txz;
   
   txz = (Text_Zone *)data;
   txz->b.w = w;
   txz->b.h = h;
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
	ebits_set_named_bit_replace(txz->bg, "Button_Area",
				    NULL,
				    NULL,
				    _txz_cb_tb_move,
				    _txz_cb_tb_resize,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
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
   for (l = txz->buttons; l; l = l->next)
     {
	Text_Zone_Button *tb;
	
	tb = l->data;
	if (tb->bg) ebits_show(tb->bg);
	evas_show(evas, tb->label);
     }
   txz_adjust_txt(txz);
}

void
txz_hide(Text_Zone *txz)
{
   Evas_List l;
   
   if (txz->bg)
     ebits_hide(txz->bg);
   for (l = txz->lines; l; l = l->next)
     evas_hide(evas, (Evas_Object)l->data);
   for (l = txz->buttons; l; l = l->next)
     {
	Text_Zone_Button *tb;
	
	tb = l->data;
	if (tb->bg) ebits_hide(tb->bg);
	evas_hide(evas, tb->label);
     }
   txz_adjust_txt(txz);
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
   txz_adjust_txt(txz);
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
	evas_put_data(evas, o, "align", (void *)((int)align[0]));
	ypos += th;
	
	p = tok + 1;
     }
   if (txz->bg)
     {
	int minw, minh;
	
	ebits_get_real_min_size(txz->bg, &minw, &minh);	
	ebits_resize(txz->bg, minw + 8, minh + 8);
	ebits_show(txz->bg);
	ebits_set_layer(txz->bg, 9);
     }
   txz_adjust_txt(txz);
}

void
txz_button(Text_Zone *txz, char *text, void (*func) (void *data), void *data)
{
   Text_Zone_Button *tb;
   
   tb = NEW(Text_Zone_Button, 1);
   ZERO(tb, Text_Zone_Button, 1);
   
   txz->buttons = evas_list_append(txz->buttons, tb);
   tb->label = evas_add_text(evas, "nationff", 12, text);
   evas_set_pass_events(evas, tb->label, 1);
   evas_set_color(evas, tb->label, 0, 0, 0, 255);
   evas_set_layer(evas, tb->label, 12);
   tb->bg = ebits_load(PACKAGE_DATA_DIR"/data/setup/textzone_button.bits.db");
   if (tb->bg)
     ebits_add_to_evas(tb->bg, evas);
   txz_adjust_txt(txz);
}

void
txz_adjust_txt(Text_Zone *txz)
{
   Evas_List l;
   double ypos;
   double xpos;
   
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
	align = (char)((int)evas_get_data(evas, o, "align"));
	x = txz->l.x + 4;
        tw = evas_get_text_width(evas, o);
	th = evas_get_text_height(evas, o);
	if (align == 'c') x = txz->l.x + 4 + ((txz->l.w - 8 - tw) / 2);
	else if (align == 'r') x = txz->l.x + 4 + (txz->l.w - 8 - tw);
	evas_move(evas, o, x, ypos);
	ypos += th;
     }
   xpos = 0;
   for (l = txz->buttons; l; l = l->next)
     {
	Text_Zone_Button *tb;
	double tw, th;
	
	tb = l->data;
	tw = evas_get_text_width(evas, tb->label);
	th = evas_get_text_height(evas, tb->label);
	if (tb->bg)
	  {
	     int pl, pr, pt, pb;
	     
	     pl = pr = pt = pb = 0;
	     ebits_get_insets(tb->bg, &pl, &pr, &pt, &pb);
	     ebits_set_layer(tb->bg, 11);	
	     ebits_show(tb->bg);
	     ebits_resize(tb->bg, tw + pl + pr, txz->b.h);
	     ebits_move(tb->bg, txz->b.x + xpos, txz->b.y);
	     evas_move(evas, tb->label, txz->b.x + pl + xpos, txz->b.y + pt + ((txz->b.h - pt - pb - th) / 2));
	     evas_show(evas, tb->label);
	     xpos += tw + pl + pr;
	  }
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

#include "../config.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <fnmatch.h>
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif
#include <Imlib2.h>
#include <Evas.h>
#include <Ebits.h>
#include <Ecore.h>
#include <Edb.h>
#include <libefsd.h>

#define E_PROF 1
#ifdef E_PROF
extern Evas_List __e_profiles;

typedef struct _e_prof
{
   char  *func;
   double total;
   double t1, t2;
} E_Prof;
#define E_PROF_START(_prof_func) \
{ \
E_Prof __p, *__pp; \
Evas_List __pl; \
__p.func = _prof_func; \
__p.total = 0.0; \
__p.t1 = e_get_time(); \
__p.t2 = 0.0;

#define E_PROF_STOP \
__p.t2 = e_get_time(); \
for (__pl = __e_profiles; __pl; __pl = __pl->next) \
{ \
__pp = __pl->data; \
if (!strcmp(__p.func, __pp->func)) \
{ \
__pp->total += (__p.t2 - __p.t1); \
break; \
} \
} \
if (!__pl) \
{ \
__pp = NEW(E_Prof, 1); \
__pp->func = __p.func; \
__pp->total = __p.t2 - __p.t1; \
__pp->t1 = 0.0; \
__pp->t2 = 0.0; \
__e_profiles = evas_list_append(__e_profiles, __pp); \
} \
}
#define E_PROF_DUMP \
{ \
Evas_List __pl; \
for (__pl = __e_profiles; __pl; __pl = __pl->next) \
{ \
E_Prof *__p; \
__p = __pl->data; \
printf("%3.3f : %s()\n", __p->total, __p->func); \
} \
}
#else
#define E_PROF_START(_prof_func)
#define E_PROF_STOP
#define E_PROF_DUMP
#endif


#define OBJ_REF(_e_obj) _e_obj->references++
#define OBJ_UNREF(_e_obj) _e_obj->references--
#define OBJ_IF_FREE(_e_obj) if (_e_obj->references == 0)
#define OBJ_FREE(_e_obj) _e_obj->e_obj_free(_e_obj)
#define OBJ_DO_FREE(_e_obj) \
OBJ_UNREF(_e_obj); \
OBJ_IF_FREE(_e_obj) \
{ \
OBJ_FREE(_e_obj); \
}

#define OBJ_PROPERTIES \
int references; \
void (*e_obj_free) (void *e_obj);
#define OBJ_INIT(_e_obj, _e_obj_free_func) \
{ \
_e_obj->references = 1; \
_e_obj->e_obj_free = (void *) _e_obj_free_func; \
}

#define SPANS_COMMON(x1, w1, x2, w2) \
  (!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))
#define UN(_blah) _blah = 0

#define ACT_MOUSE_IN      0
#define ACT_MOUSE_OUT     1
#define ACT_MOUSE_CLICK   2
#define ACT_MOUSE_DOUBLE  3
#define ACT_MOUSE_TRIPLE  4
#define ACT_MOUSE_UP      5
#define ACT_MOUSE_CLICKED 6
#define ACT_MOUSE_MOVE    7
#define ACT_KEY_DOWN      8
#define ACT_KEY_UP        9

#define SET_BORDER_GRAVITY(_b, _grav) \
e_window_gravity_set(_b->win.container, _grav); \
e_window_gravity_set(_b->win.input, _grav); \
e_window_gravity_set(_b->win.l, _grav); \
e_window_gravity_set(_b->win.r, _grav); \
e_window_gravity_set(_b->win.t, _grav); \
e_window_gravity_set(_b->win.b, _grav);

typedef struct _E_Object              E_Object;
typedef struct _E_Border              E_Border;
typedef struct _E_Grab                E_Grab;
typedef struct _E_Action              E_Action;
typedef struct _E_Action_Proto        E_Action_Proto;
typedef struct _E_Desktop             E_Desktop;
typedef struct _E_Rect                E_Rect;
typedef struct _E_Active_Action_Timer E_Active_Action_Timer;
typedef struct _E_View                E_View;
typedef struct _E_Icon                E_Icon;
typedef struct _E_Shelf               E_Shelf;
typedef struct _E_Background          E_Background;
typedef struct _E_Menu                E_Menu;
typedef struct _E_Menu_Item           E_Menu_Item;
typedef struct _E_Build_Menu          E_Build_Menu;
typedef struct _E_Entry               E_Entry;
typedef struct _E_Pack_Object_Class   E_Pack_Object_Class;
typedef struct _E_Pack_Object         E_Pack_Object;

struct _E_Object
{
   OBJ_PROPERTIES;
};

struct _E_Border
{
   OBJ_PROPERTIES;

   struct {
      Window main;
      Window l, r, t, b;
      Window input;
      Window container;
      Window client;
   } win;
   struct {
      Evas l, r, t, b;
   } evas;
   struct {
      struct {
	 Evas_Object l, r, t, b;
      } title;
      struct {
	 Evas_Object l, r, t, b;
      } title_clip;
   } obj;   
   struct {
      Pixmap l, r, t, b;
   } pixmap;
   struct {
      int new;
      Ebits_Object l, r, t, b;
   } bits;
   
   struct {
      struct {
	 int x, y, w, h;
	 int visible;
	 int dx, dy;
      } requested;
      int   x, y, w, h;
      int   visible;
      int   selected;
      int   shaded;
      int   has_shape;
      int   shape_changes;
      int   shaped_client;
   } current, previous;
   
   struct {
      struct {
	 int w, h;
	 double aspect;
      } base, min, max, step;
      int layer;
      char *title;
      char *name;
      char *class;
      char *command;
      Window group;
      int takes_focus;
      int sticky;
      Colormap colormap;
      int fixed;
      int arrange_ignore;
      int hidden;
      int iconified;
      int titlebar;
      int border;
      int handles;
      int w, h;
   } client;
   
   struct {
      int move, resize;
   } mode;
   
   struct {
      int x, y, w, h;
      int is;
   } max;
   
   int ignore_unmap;
   int shape_changed;
   int placed;
   
   Evas_List grabs;
   E_Desktop *desk;
   
   char *border_file;

   int first_expose;
   
   int changed;
};

struct _E_Grab
{
   int              button;
   Ev_Key_Modifiers mods;
   int              any_mod;
   int              remove_after;
   int              allow;
};

struct _E_Action
{
   OBJ_PROPERTIES;
   
   char           *name;
   char           *action;
   char           *params;
   int             event;
   int             button;
   char           *key;
   int             modifiers;
   E_Action_Proto *action_proto;
   void           *object;
   int             started;
};

struct _E_Action_Proto
{
   OBJ_PROPERTIES;
   
   char  *action;
   void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
   void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
   void (*func_go)    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);
};

struct _E_Desktop
{
   OBJ_PROPERTIES;
   
   char *name;
   char *dir;
   struct {
      Window main;
      Window container;
      Window desk;
   } win;
   E_View *view;
   int x, y;
   struct {
      int w, h;
   } real, virt;
   Evas_List windows;
   int changed;
};

struct _E_Rect
{
   int x, y, w, h;
   int v1, v2, v3, v4;
};

struct _E_Active_Action_Timer
{
   void *object;
   char *name;
};

struct _E_View
{
   OBJ_PROPERTIES;
   
   char                  *dir;
   
   struct {
      Evas_Render_Method  render_method;
      int                 back_pixmap;
   } options;
   
   Evas                   evas;
   struct {
      Window              base;
      Window              main;
   } win;
   Pixmap                 pmap;
   struct {
      int                 w, h;
   } size;
   struct {
      int                 x, y;
   } viewport;
   struct {
      int                 x, y;
   } location;
   struct {
      Evas_Object         obj_rect;
      Evas_Object         obj_l1;
      Evas_Object         obj_l2;
      Evas_Object         obj_l3;
      Evas_Object         obj_l4;
      int                 on;
      int                 start_x, start_y;
      int                 x, y, w, h;
   } selection;
   
   Evas_Object            obj_bg;
   
   E_Background          *bg;
   
   int                    is_listing;
   int                    monitor_id;
   
   Evas_List              icons;
   Evas_List              shelves;
   
   int                    changed;
};

struct _E_Icon
{
   OBJ_PROPERTIES;
   
   char   *file;
   
   E_View *view;
   
   char    *shelf_name;
   E_Shelf *shelf;
   
   struct {
      struct {
	 char *base, *type;
      } mime;
      EfsdCmdId   link_get_id;
      char *link;
      int   is_exe;
      int   is_dir;
      struct {
	 char *normal;
	 char *selected;
	 char *clicked;
      } icon;
      int ready;
   } info;
   
   struct {
      int     x, y, w, h;
      struct {
	 int text_location;
	 int show_text;
	 int show_icon;
      } options;
      struct {
	 int  clicked;
	 int  selected;
	 int  hilited;
      } state;
      char   *icon;
      int     visible;
   } current, previous;
   
   struct {
      Evas_Object   icon;
      Evas_Object   filename;
      Evas_Object   sel1, sel2;
      Ebits_Object  sel_icon;
      Ebits_Object  sel_text;
      Ebits_Object  base_icon;
      Ebits_Object  base_text;
   } obj;
   int     changed;   
};

struct _E_Shelf
{
   OBJ_PROPERTIES;
   
   char *name;
   E_View *view;
   
   int x, y, w, h;
   struct {
      Ebits_Object border;
   } bit;
   struct {
      Evas_Object clipper;
   } obj;
   int visible;
   int icon_count;
   struct {
      int moving;
      int resizing;
   } state;
};

struct _E_Background
{
   OBJ_PROPERTIES;
   
   Evas  evas;
   struct {
      int sx, sy;
      int w, h;
   } geom;
   
   /* FIXME: REALLY boring for now - just a scaled image  - temporoary */
   char *image;
   Evas_Object obj;
};

struct _E_Menu
{
   OBJ_PROPERTIES;
   
   struct {
      int       x, y, w, h;
      int       visible;
   } current, previous;
   struct {
      int l, r, t, b;
   } border, sel_border;
   struct {
      Window       main, evas;
   } win;
   Evas         evas;
   Ebits_Object bg;
   Evas_List    entries;
   char        *bg_file;
   
   int       first_expose;

   int       recalc_entries;
   int       redo_sel;
   int       changed;
   
   struct {
      int state, icon, text;
   } size;
   struct {
      int icon, state;
   } pad;
   
   E_Menu_Item *selected;
   
   Time      time;
};

struct _E_Menu_Item
{
   OBJ_PROPERTIES;
   
   int x, y;
   struct {
      struct {
	 int w, h;
      } min;
      int w, h;
   } size;
   
   Ebits_Object  bg;
   char         *bg_file;
   int           selected;

   Evas_Object   obj_entry;
   
   char         *str;
   Evas_Object   obj_text;
   
   char         *icon;
   Evas_Object   obj_icon;
   int           scale_icon;
   
   Ebits_Object  state;
   char         *state_file;
   
   Ebits_Object  sep;
   char         *sep_file;
   
   int           separator;
   int           radio_group;
   int           radio;
   int           check;
   int           on;
   
   E_Menu       *menu;
   E_Menu       *submenu;
   
   void        (*func_select) (E_Menu *m, E_Menu_Item *mi, void *data);
   void         *func_select_data;
};

struct _E_Build_Menu
{
   OBJ_PROPERTIES;

   char      *file;
   time_t     mod_time;
   
   E_Menu    *menu;
   
   Evas_List  menus;
   Evas_List  commands;
};

struct _E_Entry
{
   Evas  evas;
   char *buffer;
   int   cursor_pos;
   struct {
      int start, length, down;
   } select;
   int   mouse_down;
   int   visible;
   int   focused;
   int   scroll_pos;
   int   x, y, w, h;
   int   min_size;
   Ebits_Object obj_base;
   Ebits_Object obj_cursor;
   Ebits_Object obj_selection;
   Evas_Object event_box;
   Evas_Object clip_box;
   Evas_Object text;
   Window paste_win;
   Window selection_win;
   int end_width;
   void (*func_changed) (E_Entry *entry, void *data);
   void *data_changed;
   void (*func_enter) (E_Entry *entry, void *data);
   void *data_enter;
   void (*func_focus_in) (E_Entry *entry, void *data);
   void *data_focus_in;
   void (*func_focus_out) (E_Entry *entry, void *data);
   void *data_focus_out;
};

struct _E_Pack_Object_Class
{
   void * (*new)         (void);
   void   (*free)        (void *object);
   void   (*show)        (void *object);
   void   (*hide)        (void *object);
   void   (*raise)       (void *object);
   void   (*lower)       (void *object);
   void   (*layer)       (void *object, int l);
   void   (*evas)        (void *object, Evas e);
   void   (*clip)        (void *object, Evas_Object clip);
   void   (*unclip)      (void *object);
   void   (*move)        (void *object, int x, int y);
   void   (*resize)      (void *object, int w, int h);
   void   (*min)         (void *object, int *w, int *h);
   void   (*max)         (void *object, int *w, int *h);
   void   (*size)        (void *object, int x, int y);
};

#define PACK_HBOX   0
#define PACK_VBOX   1
#define PACK_TABLE  2
#define PACK_ENTRY  3
#define PACK_LABEL  4
#define PACK_BUTTON 5
#define PACK_RADIO  6
#define PACK_CHECK  7

#define PACK_MAX    8

struct _E_Pack_Object
{
   int                  type;
   E_Pack_Object_Class  class;
   struct {
      Evas              evas;
      Evas_Object       clip;
      int               layer;
      int               visible;
      int               x, y, w, h;
      void             *object;
   } data;
   int                  references;
   E_Pack_Object       *parent;
   Evas_List            children;
};

#define DO(_object, _method, _args...) \
{ if (_object->class._method) _object->class._method(_object->data.object, ## _args); }

void e_entry_init(void);
void e_entry_free(E_Entry *entry);
E_Entry *e_entry_new(void);
void e_entry_handle_keypress(E_Entry *entry, Ev_Key_Down *e);
void e_entry_set_evas(E_Entry *entry, Evas evas);
void e_entry_show(E_Entry *entry);
void e_entry_hide(E_Entry *entry);
void e_entry_raise(E_Entry *entry);
void e_entry_lower(E_Entry *entry);
void e_entry_set_layer(E_Entry *entry, int l);
void e_entry_set_clip(E_Entry *entry, Evas_Object clip);
void e_entry_unset_clip(E_Entry *entry);
void e_entry_move(E_Entry *entry, int x, int y);
void e_entry_resize(E_Entry *entry, int w, int h);
void e_entry_query_max_size(E_Entry *entry, int *w, int *h);
void e_entry_max_size(E_Entry *entry, int *w, int *h);
void e_entry_min_size(E_Entry *entry, int *w, int *h);
void e_entry_set_size(E_Entry *entry, int w, int h);
void e_entry_set_focus(E_Entry *entry, int focused);
void e_entry_set_text(E_Entry *entry, const char *text);
const char *e_entry_get_text(E_Entry *entry);
void e_entry_set_cursor(E_Entry *entry, int cursor_pos);
int e_entry_get_cursor(E_Entry *entry);
void e_entry_set_changed_callback(E_Entry *entry, void (*func) (E_Entry *_entry, void *_data), void *data);
void e_entry_set_enter_callback(E_Entry *entry, void (*func) (E_Entry *_entry, void *_data), void *data);
void e_entry_set_focus_in_callback(E_Entry *entry, void (*func) (E_Entry *_entry, void *_data), void *data);
void e_entry_set_focus_out_callback(E_Entry *entry, void (*func) (E_Entry *_entry, void *_data), void *data);
void e_entry_insert_text(E_Entry *entry, char *text);
void e_entry_clear_selection(E_Entry *entry);
void e_entry_delete_to_left(E_Entry *entry);
void e_entry_delete_to_right(E_Entry *entry);
char *e_entry_get_selection(E_Entry *entry);


void e_action_add_proto(char *action,
			void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
			void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
			void (*func_go)    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy));
void e_actions_init(void);
void e_action_start(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry);
void e_action_stop(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry);
void e_action_go(char *action, int act, int button, char *key, Ev_Key_Modifiers mods, void *o, void *data, int x, int y, int rx, int ry, int dx, int dy);
void e_action_stop_by_object(void *o, void *data, int x, int y, int rx, int ry);
void e_actions_del_timer(void *o, char *name);
void e_actions_add_timer(void *o, char *name);
void e_actions_del_timer_object(void *o);
  
void e_border_apply_border(E_Border *b);
E_Border * e_border_new(void);
E_Border * e_border_adopt(Window win, int use_client_pos);
void e_border_reshape(E_Border *b);  
void e_border_free(E_Border *b);
void e_border_remove_mouse_grabs(E_Border *b);
void e_border_attach_mouse_grabs(E_Border *b);
void e_border_remove_all_mouse_grabs(void);
void e_border_attach_all_mouse_grabs(void);
void e_border_redo_grabs(void);
E_Border * e_border_find_by_window(Window win);
void e_border_set_bits(E_Border *b, char *file);
void e_border_set_color_class(E_Border *b, char *class, int rr, int gg, int bb, int aa);
void e_border_adjust_limits(E_Border *b);
void e_border_update(E_Border *b);
void e_border_set_layer(E_Border *b, int layer);
void e_border_raise(E_Border *b);
void e_border_lower(E_Border *b);
void e_border_raise_above(E_Border *b, E_Border *above);
void e_border_lower_below(E_Border *b, E_Border *below);
void e_border_init(void);
void e_border_adopt_children(Window win);
    
void e_icccm_move_resize(Window win, int x, int y, int w, int h);
void e_icccm_delete(Window win);
void e_icccm_state_mapped(Window win);
void e_icccm_state_iconified(Window win);
void e_icccm_state_withdrawn(Window win);
void e_icccm_adopt(Window win);
void e_icccm_release(Window win);
void e_icccm_get_size_info(Window win, E_Border *b);
void e_icccm_get_mwm_hints(Window win, E_Border *b);
void e_icccm_get_layer(Window win, E_Border *b);
void e_icccm_get_title(Window win, E_Border *b);
void e_icccm_set_frame_size(Window win, int l, int r, int t, int b);
void e_icccm_set_desk_area(Window win, int ax, int ay);
void e_icccm_set_desk_area_size(Window win, int ax, int ay);
void e_icccm_set_desk(Window win, int d);
int  e_icccm_is_shaped(Window win);
void e_icccm_handle_property_change(Atom a, E_Border *b);
void e_icccm_handle_client_message(Ev_Message *e);
void e_icccm_advertise_e_compat(void);
void e_icccm_advertise_mwm_compat(void);
void e_icccm_advertise_gnome_compat(void);
void e_icccm_advertise_kde_compat(void);
void e_icccm_advertise_net_compat(void);

void e_desktops_init(void);    
void e_desktops_scroll(E_Desktop *desk, int dx, int dy);
void e_desktops_free(E_Desktop *desk);
void e_desktops_init_file_display(E_Desktop *desk);
E_Desktop * e_desktops_new(void);
void e_desktops_add_border(E_Desktop *d, E_Border *b);
void e_desktops_del_border(E_Desktop *d, E_Border *b);
void e_desktops_delete(E_Desktop *d);
void e_desktops_show(E_Desktop *d);
void e_desktops_hide(E_Desktop *d);
int e_desktops_get_num(void);
E_Desktop * e_desktops_get(int d);
int e_desktops_get_current(void);
void e_desktops_update(E_Desktop *desk);

void e_resist_border(E_Border *b);
    
time_t e_file_modified_time(char *file);
void e_set_env(char *variable, char *content);
int e_file_exists(char *file);
int e_file_is_dir(char *file);
char *e_file_home(void);
int e_file_mkdir(char *dir);
int e_file_cp(char *src, char *dst);
char *e_file_real(char *file);
char *e_file_get_file(char *file);
char *e_file_get_dir(char *file);
void *e_memdup(void *data, int size);
int e_glob_matches(char *str, char *glob);
int e_file_can_exec(struct stat *st);
char *e_file_link(char *link);

void e_exec_set_args(int argc, char **argv);
void e_exec_restart(void);
pid_t e_exec_run(char *exe);
pid_t e_exec_run_in_dir(char *exe, char *dir);
pid_t e_run_in_dir_with_env(char *exe, char *dir, int *launch_id_ret, char **env, char *launch_path);

/* something to check validity of config files where we get data from */
/* for now its just a 5 second timout so it will only invalidate */
/* if we havent looked for 5 seconds... BUT later when efsd is more solid */
/* we should use that to tell us when its invalid */
typedef struct _e_config_file E_Config_File;
struct _e_config_file
{
   char   *src;
   double  last_fetch;
};
#define E_CFG_FILE(_var, _src) \
static E_Config_File _var = {_src, 0.0}
#define E_CONFIG_CHECK_VALIDITY(_var, _src) \
{ \
double __time; \
__time = e_get_time(); \
if (_var.last_fetch < (__time - 5.0)) { \
_var.last_fetch = __time;
#define E_CONFIG_CHECK_VALIDITY_END \
} \
}

typedef struct _e_config_element E_Config_Element;
struct _e_config_element 
{
   char   *src;
   char   *key;
   double  last_fetch;
   int     type;
   int     def_int_val;
   float   def_float_val;
   char   *def_str_val;
   void   *def_data_val;
   int     def_data_val_size;
   int     cur_int_val;
   float   cur_float_val;
   char   *cur_str_val;
   void   *cur_data_val;
   int     cur_data_val_size;
};
#define E_CFG_INT_T   123
#define E_CFG_FLOAT_T 1234
#define E_CFG_STR_T   12345
#define E_CFG_DATA_T  123456
#define E_CFG_INT(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_INT_T, \
_default, 0.0, NULL, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}
#define E_CFG_FLOAT(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_FLOAT_T, \
0, _default, NULL, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}
#define E_CFG_STR(_var, _src, _key, _default) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_STR_T, \
0, 0.0, _default, NULL, 0, \
0, 0.0, NULL, NULL, 0, \
}
#define E_CFG_DATA(_var, _src, _key, _default, _default_size) \
static E_Config_Element _var = { _src, _key, 0.0, E_CFG_DATAT_T, \
0, 0.0, NULL, _default, _default_size, \
0, 0.0, NULL, NULL, 0, \
}
/* yes for now it only fetches them every 5 seconds - in the end i need a */
/* validity flag for the database file to know if it changed and only then */
/* get the value again. this is waiting for efsd to become more solid */
#define E_CFG_VALIDITY_CHECK(_var) \
{ \
double __time; \
__time = e_get_time(); \
if (_var.last_fetch < (__time - 5.0)) { \
int __cfg_ok = 0; \
_var.last_fetch = __time;
#define E_CFG_END_VALIDITY_CHECK \
} \
}

#define E_CONFIG_INT_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
E_DB_INT_GET(e_config_get(_var.src), _var.key, _var.cur_int_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_int_val = _var.def_int_val; \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_int_val;}
#define E_CONFIG_FLOAT_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
E_DB_FLOAT_GET(e_config_get(_var.src), _var.key, _var.cur_float_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_float_val = _var.def_float_val; \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_float_val;}
#define E_CONFIG_STR_GET(_var, _val) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
if (_var.cur_str_val) free(_var.cur_str_val); \
_var.cur_str_val = NULL; \
E_DB_STR_GET(e_config_get(_var.src), _var.key, _var.cur_str_val, __cfg_ok); \
if (!__cfg_ok) _var.cur_str_val = _var.def_str_val \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_str_val;}
#define E_CONFIG_DATA_GET(_var, _val, _size) \
{{ \
E_CFG_VALIDITY_CHECK(_var) \
if (_var.cur_data_val) free(_var.cur_data_val); \
_var.cur_data_val = NULL; \
_var.cur_data_size = 0; \
{ E_DB_File *__db; \
__db = e_db_open_read(e_config_get(_var.src)); \
if (__db) { \
_var.cur_data_val = e_db_data_get(__db, _var.key, &(_var.cur_data_size)); \
if (_var.cur_data_val) __cfg_ok = 1; \
e_db_close(__db); \
} \
} \
if (!__cfg_ok) { \
_var.cur_data_val = e_memdup(_var.def_data_val, _var.def_data_size); \
_var.cur_data_size = _var.def_data_size; \
} \
E_CFG_END_VALIDITY_CHECK \
} \
_val = _var.cur_data_val; \
_size = _var.cur_data_size;}

char *e_config_get(char *type);
void  e_config_init(void);
void  e_config_set_user_dir(char *dir);
char *e_config_user_dir(void);

void e_view_free(E_View *v);
E_View *e_view_new(void);
void e_view_init(void);

void e_menu_callback_item(E_Menu *m, E_Menu_Item *mi);
void e_menu_item_set_callback(E_Menu_Item *mi, void (*func) (E_Menu *m, E_Menu_Item *mi, void *data), void *data);
void e_menu_hide_submenus(E_Menu *menus_after);
void e_menu_select(int dx, int dy);
void e_menu_init(void);
void e_menu_event_win_show(void);
void e_menu_event_win_hide(void);
void e_menu_set_background(E_Menu *m);
void e_menu_set_sel(E_Menu *m, E_Menu_Item *mi);
void e_menu_set_sep(E_Menu *m, E_Menu_Item *mi);
void e_menu_set_state(E_Menu *m, E_Menu_Item *mi);
void e_menu_free(E_Menu *m);
E_Menu *e_menu_new(void);
void e_menu_hide(E_Menu *m);
void e_menu_show(E_Menu *m);
void e_menu_move_to(E_Menu *m, int x, int y);
void e_menu_show_at_mouse(E_Menu *m, int x, int y, Time t);
void e_menu_add_item(E_Menu *m, E_Menu_Item *mi);
void e_menu_del_item(E_Menu *m, E_Menu_Item *mi);
void e_menu_item_update(E_Menu *m, E_Menu_Item *mi);
void e_menu_item_unrealize(E_Menu *m, E_Menu_Item *mi);
void e_menu_item_realize(E_Menu *m, E_Menu_Item *mi);
E_Menu_Item *e_menu_item_new(char *str);
void e_menu_obscure_outside_screen(E_Menu *m);
void e_menu_scroll_all_by(int dx, int dy);
void e_menu_update_visibility(E_Menu *m);
void e_menu_update_base(E_Menu *m);
void e_menu_update_finish(E_Menu *m);
void e_menu_update_shows(E_Menu *m);
void e_menu_update_hides(E_Menu *m);
void e_menu_update(E_Menu *m);
void e_menu_item_set_icon(E_Menu_Item *mi, char *icon);
void e_menu_item_set_text(E_Menu_Item *mi, char *text);
void e_menu_item_set_separator(E_Menu_Item *mi, int sep);
void e_menu_item_set_radio(E_Menu_Item *mi, int radio);
void e_menu_item_set_check(E_Menu_Item *mi, int check);
void e_menu_item_set_state(E_Menu_Item *mi, int state);
void e_menu_item_set_submenu(E_Menu_Item *mi, E_Menu *submenu);
void e_menu_item_set_scale_icon(E_Menu_Item *mi, int scale);
void e_menu_set_padding_icon(E_Menu *m, int pad);
void e_menu_set_padding_state(E_Menu *m, int pad);

void          e_build_menu_unbuild(E_Build_Menu *bm);
E_Menu       *e_build_menu_build_number(E_Build_Menu *bm, E_DB_File *db, int num);
void          e_build_menu_build(E_Build_Menu *bm);
void          e_build_menu_free(E_Build_Menu *bm);
E_Build_Menu *e_build_menu_new_from_db(char *file);

void e_fs_add_event_handler(void (*func) (EfsdEvent *ev));
void e_fs_init(void);
EfsdConnection *e_fs_get_connection(void);
    

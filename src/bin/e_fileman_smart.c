/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

/* TODO:
 *
 * - add ability to have icons on desktop (this works, but we need some fixes)
 *   files should go on ~/.e/e/desktop for example.
 *
 * - when we select multiple items, the right click menu on the icons needs
 *   to display some group related things and its actions need to work
 *   on the group.
 *
 * - is the offset code working properly? i have a feeling we're displayin
 *   more icons that the visible space can take and they are being hidden.
 *
 * - emit all sorts of signals on double click, right click, single click...
 *
 * - aspect ratio on thumbnails.
 *
 * - add typebuffer like in evidence.
 *
 * - keyboard shortcuts for directory and file navigation.
 *
 * - allow for icon movement inside the canvas
 *
 * - add metadata system which allows us to save icon positions and will
 *   eventually allow us to have custom icon sizes, custom bgs per dir...
 *
 * - double check dir monitoring. note: when we are in a dir that is constantly
 *   changing, we cant keep calling redraw_new as it will kill us.
 *
 * - we need to fix the icon edc to allow us to have icon labels what will
 *   wrap on wrap=char
 */

/* BUGS:
 * 
 * - Closing Efm window while its thumbnailing causes a segv
 * 
 * - Deleting a dir causes a segv
 */

int E_EVENT_FM_RECONFIGURE;
int E_EVENT_FM_DIRECTORY_CHANGE;

#ifdef EFM_DEBUG
# define D(x)  do {printf(__FILE__ ":%d:  ", __LINE__); printf x; fflush(stdout);} while (0)
#else
# define D(x)  ((void) 0)
#endif

typedef struct _E_Fm_Smart_Data            E_Fm_Smart_Data;
typedef struct _E_Fm_Icon                  E_Fm_Icon;
typedef struct _E_Fm_Icon_CFData           E_Fm_Icon_CFData;
typedef struct _E_Fm_Config                E_Fm_Config;
typedef struct _E_Fm_Fake_Mouse_Up_Info    E_Fm_Fake_Mouse_Up_Info;
typedef enum   _E_Fm_Arrange               E_Fm_Arrange;

struct _E_Fm_Config
{
   int width;
   int height;
   Evas_List *apps;
};

struct _E_Fm_Icon
{
   E_Fm_File       *file;
   Evas_Object     *icon_object;
   E_Fm_Smart_Data *sd;
   
   struct {
      unsigned char selected : 1;
   } state;
   
   E_Menu *menu;
};

enum _E_Fm_Arrange
{
   E_FILEMAN_CANVAS_ARRANGE_NAME = 0,
   E_FILEMAN_CANVAS_ARRANGE_MODTIME = 1,
   E_FILEMAN_CANVAS_ARRANGE_SIZE = 2,
};

struct _E_Fm_Fake_Mouse_Up_Info
{
   Evas *canvas;
   int button;
};

struct _E_Fm_Smart_Data
{
   E_Menu *menu;
   E_Win *win;
   Evas *evas;
   
   Evas_Object *bg;
   Evas_Object *clip;
   Evas_Object *layout;
   Evas_Object *object;
   Evas_Object *entry_object;   
   
   char *dir;

   Evas_List *event_handlers;
   
   Evas_List *files;
   Ecore_File_Monitor *monitor;
   E_Fm_Arrange arrange;

   int frozen;
   double position;

   Evas_Coord x, y, w, h;
   
   struct {
      unsigned char start : 1;
      int x, y;
      Ecore_Evas *ecore_evas;
      Evas *evas;
      Ecore_X_Window win;
      Evas_Object *icon_object;
      Evas_Object *image_object;
   } drag;
   
   struct {
      Evas_Coord x_space, y_space, w, h;
   } icon_info;   

   struct {
      Evas_Coord w, h;      
   } max;

   struct {
	Evas_List *files;
	E_Fm_Icon *current_file;

	struct {
	     unsigned char enabled : 1;
	     Evas_Coord x, y;
	     Evas_Object *obj;
	     Evas_List *files;
	} band;

   } selection;
   
   struct {
      E_Config_DD *main_edd;
      E_Config_DD *assoc_app_edd;
      E_Fm_Config *main;      
   } conf;
};

static void                _e_fm_smart_add(Evas_Object *object);
static void                _e_fm_smart_del(Evas_Object *object);
static void                _e_fm_smart_raise(Evas_Object *object);
static void                _e_fm_smart_lower(Evas_Object *object);
static void                _e_fm_smart_stack_above(Evas_Object *object, Evas_Object *above);
static void                _e_fm_smart_stack_below(Evas_Object *object, Evas_Object *below);
static void                _e_fm_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void                _e_fm_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void                _e_fm_smart_show(Evas_Object *object);
static void                _e_fm_smart_hide(Evas_Object *object);

static void                _e_fm_redraw(E_Fm_Smart_Data *sd);

static void                _e_fm_file_menu_open(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_copy(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_cut(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_paste(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_rename(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi);
#if 0
static void                _e_fm_file_menu_properties(void *data, E_Menu *m, E_Menu_Item *mi);
#endif
static void                _e_fm_file_delete_yes_cb(void *data, E_Dialog *dia);

static void                _e_fm_menu_arrange_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_menu_refresh_cb(void *data, E_Menu *m, E_Menu_Item *mi);

static void                _e_fm_file_rename         (E_Fm_Icon *icon, const char *name);
static void                _e_fm_file_delete         (E_Fm_Icon *icon);

static void                _e_fm_dir_set             (E_Fm_Smart_Data *sd, const char *dir);
static Evas_List          *_e_fm_dir_files_get       (E_Fm_Smart_Data *sd, int type);
static char               *_e_fm_dir_pop             (const char *path);
static void                _e_fm_file_free           (E_Fm_Icon *icon);
static void                _e_fm_dir_monitor_cb      (void *data, Ecore_File_Monitor *ecore_file_monitor,  Ecore_File_Event event, const char *path);
static void                _e_fm_selections_clear    (E_Fm_Smart_Data *sd);
static void                _e_fm_selections_add      (E_Fm_Icon *icon);
static void                _e_fm_selections_rect_add (E_Fm_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
static void                _e_fm_selections_del      (E_Fm_Icon *icon);

static void                _e_fm_fake_mouse_up_later     (Evas *evas, int button);
static void                _e_fm_fake_mouse_up_all_later (Evas *evas);
static void                _e_fm_fake_mouse_up_cb        (void *data);

static void                _e_fm_mouse_down_cb      (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_mouse_move_cb      (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_mouse_up_cb        (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_icon_mouse_down_cb (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_icon_mouse_up_cb   (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_icon_mouse_in_cb   (void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_icon_mouse_out_cb  (void *data, Evas *e, Evas_Object *obj, void *event_info);
static int                 _e_fm_win_mouse_move_cb  (void *data, int type, void *event);
static int                 _e_fm_win_mouse_up_cb    (void *data, int type, void *event);

static int                 _e_fm_drop_enter_cb     (void *data, int type, void *event);
static int                 _e_fm_drop_leave_cb     (void *data, int type, void *event);
static int                 _e_fm_drop_position_cb  (void *data, int type, void *event);
static int                 _e_fm_drop_drop_cb      (void *data, int type, void *event);
static int                 _e_fm_drop_selection_cb (void *data, int type, void *event);
static void                _e_fm_drop_done_cb      (E_Drag *drag, int dropped);

static int                 _e_fm_files_sort_name_cb    (void *d1, void *d2);
static int                 _e_fm_files_sort_modtime_cb (void *d1, void *d2);

static Ecore_Event_Handler *e_fm_mouse_up_handler = NULL;
static double               e_fm_grab_time = 0;
static Evas_Smart          *e_fm_smart = NULL;


/* dummy for now */
static int dummy_val;

/* externally accessible functions */
int
e_fm_init(void)
{
   e_fm_smart = evas_smart_new("e_fm",
			       _e_fm_smart_add, /* add */
			       _e_fm_smart_del, /* del */
			       NULL, /* layer_set */
			       _e_fm_smart_raise, /* raise */
			       _e_fm_smart_lower, /* lower */
			       _e_fm_smart_stack_above, /* stack_above */
			       _e_fm_smart_stack_below, /* stack_below */
			       _e_fm_smart_move, /* move */
			       _e_fm_smart_resize, /* resize */
			       _e_fm_smart_show, /* show */
			       _e_fm_smart_hide, /* hide */
			       NULL, /* color_set */
			       NULL, /* clip_set */
			       NULL, /* clip_unset */
			       NULL); /* data*/
   
   E_EVENT_FM_RECONFIGURE = ecore_event_type_new();
   E_EVENT_FM_DIRECTORY_CHANGE = ecore_event_type_new();   
   return 1;   
}

int
e_fm_shutdown(void)
{
   evas_smart_free(e_fm_smart);
   return 1;
}

Evas_Object *
e_fm_add(Evas *evas)
{
   return evas_object_smart_add(evas, e_fm_smart);
}

void
e_fm_dir_set(Evas_Object *object, const char *dir)
{
   E_Fm_Smart_Data *sd;

   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   _e_fm_dir_set(sd, dir);
}

char *
e_fm_dir_get(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return NULL;
   
   return strdup(sd->dir);
}

void
e_fm_e_win_set(Evas_Object *object, E_Win *win)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   sd->win = win;   
}

E_Win *
e_fm_e_win_get(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;

   sd = evas_object_smart_data_get(object);
   if (!sd) return NULL;
   
   return sd->win;   
}

void
e_fm_scroll_horizontal(Evas_Object *object, double percent)
{

}

void
e_fm_scroll_vertical(Evas_Object *object, double percent)
{
   E_Fm_Smart_Data *sd;
   int offsetpx;
   Evas_Coord x, y, w, h;   

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   sd->position = percent;
   
   offsetpx = (percent) * (sd->max.h - sd->h);   
   
   evas_object_geometry_get(sd->layout, &x, &y, &w, &h);
   e_icon_layout_clip_freeze(sd->layout);
   evas_object_move(sd->layout, x, sd->y - offsetpx);       
   e_icon_layout_clip_thaw(sd->layout);           
}

void
e_fm_geometry_virtual_get(Evas_Object *object, Evas_Coord *w, Evas_Coord *h)
{  
   E_Fm_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   if(w)
     *w = sd->max.w;
   
   if(h)
     *h = sd->max.h;   
}

void
e_fm_menu_set(Evas_Object *object, E_Menu *menu)
{
   E_Fm_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   if (menu)
     sd->menu = menu;
}

E_Menu *
e_fm_menu_get(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return NULL;

   return sd->menu;
}

int
e_fm_freeze(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   sd->frozen++;
   evas_event_freeze(sd->evas);
   D(("e_fm_freeze: %d\n", sd->frozen));
   return sd->frozen;
}

int
e_fm_thaw(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   if(!sd->frozen) return 0;   
   
   sd->frozen--;
   evas_event_thaw(sd->evas);
   D(("e_fm_thaw: %d\n", sd->frozen));
   return sd->frozen;
}

/* local subsystem functions */

static void
_e_fm_smart_add(Evas_Object *object)
{
   char dir[PATH_MAX];
   E_Fm_Smart_Data *sd;

   sd = E_NEW(E_Fm_Smart_Data, 1);
   if (!sd) return;
   sd->object = object;   

   sd->icon_info.w = 48;
   sd->icon_info.h = 48;
   sd->icon_info.x_space = 15;
   sd->icon_info.y_space = 15;   
      
   sd->evas = evas_object_evas_get(object);   
   sd->frozen = 0;
   
   sd->bg = evas_object_rectangle_add(sd->evas); // this should become an edje
   evas_object_color_set(sd->bg, 0, 0, 0, 0);
   evas_object_show(sd->bg);
   
   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_fm_mouse_down_cb, sd);
   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_UP,
				  _e_fm_mouse_up_cb, sd);
   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_MOVE,
				  _e_fm_mouse_move_cb, sd);
   evas_object_smart_member_add(sd->bg, object);
   
   sd->layout = e_icon_layout_add(sd->evas);
   e_icon_layout_spacing_set(sd->layout, sd->icon_info.x_space, sd->icon_info.y_space);
   evas_object_stack_above(sd->layout, sd->bg);
   evas_object_show(sd->layout);   
   
   sd->clip = evas_object_rectangle_add(sd->evas);
   evas_object_smart_member_add(sd->clip, object);
   evas_object_move(sd->clip, -100000, -100000);
   evas_object_resize(sd->clip, 200000, 200000);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   evas_object_clip_set(sd->bg, sd->clip);
   evas_object_clip_set(sd->layout, sd->clip);   
   
   sd->selection.band.obj = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->selection.band.obj,
			   "base/theme/fileman/rubberband",
			   "fileman/rubberband");

   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_XDND_ENTER,
								 _e_fm_drop_enter_cb,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_XDND_LEAVE,
								 _e_fm_drop_leave_cb,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_XDND_POSITION,
								 _e_fm_drop_position_cb,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_XDND_DROP,
								 _e_fm_drop_drop_cb,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY,
								 _e_fm_drop_selection_cb,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
								 _e_fm_win_mouse_move_cb,
								 sd));
   
   sd->monitor = NULL;
   sd->position = 0.0;
   sd->event_handlers = NULL;

   sd->conf.main_edd = E_CONFIG_DD_NEW("E_Fm_Config", E_Fm_Config);
   sd->conf.assoc_app_edd = E_CONFIG_DD_NEW("E_Fm_Assoc_App",E_Fm_Assoc_App);
   
#undef T
#undef DD
#define T E_Fm_Assoc_App
#define DD sd->conf.assoc_app_edd
   E_CONFIG_VAL(DD, T, mime, STR);
   E_CONFIG_VAL(DD, T, app, STR);
#undef T
#undef DD
#define T E_Fm_Config
#define DD sd->conf.main_edd
   E_CONFIG_VAL(DD, T, width, INT);
   E_CONFIG_VAL(DD, T, height, INT);
   E_CONFIG_LIST(DD, T, apps, sd->conf.assoc_app_edd);
   
   sd->conf.main = e_config_domain_load("efm", sd->conf.main_edd);
   if(!sd->conf.main)
    {
       /* no saved config */
       sd->conf.main = E_NEW(E_Fm_Config, 1);
       sd->conf.main->width = 640;
       sd->conf.main->height = 480;
       sd->conf.main->apps = NULL;
       
       
       /* some test values not meant for everyone */
       
	{
	   E_Fm_Assoc_App *assoc;
	   
	   assoc = E_NEW(E_Fm_Assoc_App, 1);
	   assoc->mime = (char*)E_NEW(char *, 5);
	   snprintf(assoc->mime, 5, "%s", ".jpg");
	   assoc->app = (char*)E_NEW(char *, 7);
	   snprintf(assoc->app, 7, "gqview");
	   sd->conf.main->apps = evas_list_append(sd->conf.main->apps, assoc);
	 
	   assoc = E_NEW(E_Fm_Assoc_App, 1);
	   assoc->mime = (char*)E_NEW(char *, 5);
	   snprintf(assoc->mime, 5, "%s", ".png");
	   assoc->app = (char*)E_NEW(char *, 7);
	   snprintf(assoc->app, 7, "gqview");
	   sd->conf.main->apps = evas_list_append(sd->conf.main->apps, assoc);
	}
       
    }
   
   evas_object_smart_data_set(object, sd);

   if (getcwd(dir, sizeof(dir)))
     _e_fm_dir_set(sd, dir);
}

static void
_e_fm_smart_del(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;
   
   e_config_domain_save("efm", sd->conf.main_edd, sd->conf.main);
   
//  if (sd->monitor) ecore_file_monitor_del(sd->monitor);
   sd->monitor = NULL;
   
   while (sd->files)
    {
       _e_fm_file_free(sd->files->data);
       sd->files = evas_list_remove_list(sd->files, sd->files);
    }
   
   while (sd->event_handlers)
    {
       ecore_event_handler_del(sd->event_handlers->data);
       sd->event_handlers = evas_list_remove_list(sd->event_handlers, sd->event_handlers);
    }   

   evas_object_del(sd->selection.band.obj);
   evas_object_del(sd->clip);
   evas_object_del(sd->bg);
   evas_object_del(sd->layout);
   if (sd->entry_object) evas_object_del(sd->entry_object);
   if (sd->menu) e_object_del(E_OBJECT(sd->menu));
   
   free(sd->dir);
   free(sd);   
}

static void
_e_fm_smart_raise(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_raise(sd->clip);
   evas_object_stack_below(sd->bg, sd->clip);
   evas_object_stack_below(sd->layout, sd->clip);   
}

static void
_e_fm_smart_lower(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_lower(sd->clip);
   evas_object_stack_below(sd->bg, sd->clip);
   evas_object_stack_below(sd->layout, sd->clip);   
}

static void
_e_fm_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_stack_above(sd->clip, above);
   evas_object_stack_below(sd->bg, sd->clip);
   evas_object_stack_below(sd->layout, sd->clip);
}

static void
_e_fm_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_stack_below(sd->clip, below);
   evas_object_stack_below(sd->bg, sd->clip);
   evas_object_stack_below(sd->layout, sd->clip);
}

static void
_e_fm_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_move(sd->bg, x, y);
   evas_object_move(sd->clip, x, y);
   evas_object_move(sd->layout, x, y);
   
   sd->x = x;
   sd->y = y;
}

static void
_e_fm_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Fm_Smart_Data *sd;
   E_Event_Fm_Reconfigure *ev;
   
   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_resize(sd->bg, w, h);
   evas_object_resize(sd->clip, w, h);   
   evas_object_resize(sd->layout, w, h);
   e_icon_layout_width_fix(sd->layout, w);
   e_icon_layout_virtual_size_get(sd->layout, &sd->max.w, &sd->max.h);
   sd->conf.main->width = w;
   sd->conf.main->height = h;

   sd->w = w;
   sd->h = h;
   
   if(sd->position > 0.0 && sd->max.h > sd->h)
     e_fm_scroll_vertical(object, sd->position);
   else
     sd->position = 0.0;    
   
   if(sd->frozen)
     return;   
   
   ev = E_NEW(E_Event_Fm_Reconfigure, 1);
   if (ev)
    {
       Evas_Coord w, h;
       
       evas_object_geometry_get(sd->layout, NULL, NULL, &w, &h);
       ev->object = sd->object;
       ev->w = sd->max.w;
       ev->h = sd->max.h;
       ecore_event_add(E_EVENT_FM_RECONFIGURE, ev, NULL, NULL);
    }
}

static void
_e_fm_smart_show(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_show(sd->clip);
}

static void
_e_fm_smart_hide(Evas_Object *object)
{
   E_Fm_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(object);
   if (!sd) return;
   
   evas_object_hide(sd->clip);
}

static void
_e_fm_redraw(E_Fm_Smart_Data *sd)
{
   E_Event_Fm_Reconfigure *ev;
   
   e_icon_layout_redraw_force(sd->layout);

   if(sd->frozen)
     return;
      
   ev = E_NEW(E_Event_Fm_Reconfigure, 1);
   if (ev)
    {
       Evas_Coord w, h;
       evas_object_geometry_get(sd->layout, NULL, NULL, &w, &h);
       
       ev->object = sd->object;
       ev->w = sd->max.w;
       ev->h = sd->max.h;
       ecore_event_add(E_EVENT_FM_RECONFIGURE, ev, NULL, NULL);
    }   
}

static void
_e_fm_file_rename(E_Fm_Icon *icon, const char* name)
{
   if (!name || !name[0])
     return;
   
   if (e_fm_file_rename(icon->file, name))
    {
       e_fm_icon_title_set(icon->icon_object, name);
    }
}

static void
_e_fm_file_delete(E_Fm_Icon *icon)
{
   /* TODO */
#if 0
   if (!e_fm_file_delete(icon->file))
    {
       E_Dialog *dia;
       E_Fm *fileman;
       char *text;
       
       fileman = icon->sd->fileman;
       dia = e_dialog_new(fileman->con);
       e_dialog_button_add(dia, "Ok", NULL, NULL, NULL);
       e_dialog_button_focus_num(dia, 1);
       e_dialog_title_set(dia, "Error");
       text = E_NEW(char, PATH_MAX + 256);
       snprintf(text, PATH_MAX + 256, "Could not delete  <br><b>%s</b> ?", icon->file->path);
       e_dialog_text_set(dia, text);
       
               e_dialog_show(dia);
    }
   
   icon->sd->files = evas_list_remove(icon->sd->files, icon);
   _e_fm_file_free(icon);
#endif
}

static void
_e_fm_file_menu_open(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   
   icon = data;
   
   switch (icon->file->type)
    {
     case E_FM_FILE_TYPE_DIRECTORY:
       _e_fm_dir_set(icon->sd, icon->file->path);
       break;
     case E_FM_FILE_TYPE_FILE:
       if ((!e_fm_assoc_exec(icon->sd, icon->file->path) && (e_fm_file_can_exec(icon->file))))
	 e_fm_file_exec(icon->file);
       break;
     default:
       break;
    }
}

static void
_e_fm_file_menu_copy(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   
   icon = data;
}

static void
_e_fm_file_menu_cut(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   
   icon = data;
}

static void
_e_fm_file_menu_paste(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   
   icon = data;
}

static void
_e_fm_file_menu_rename(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   
   icon = data;
   
   icon->sd->entry_object = e_entry_add(icon->sd->evas);
   evas_object_focus_set(icon->sd->entry_object, 1);
   evas_object_show(icon->sd->entry_object);
   e_entry_cursor_show(icon->sd->entry_object);
   e_entry_text_set(icon->sd->entry_object, icon->file->name);
   e_entry_cursor_move_at_end(icon->sd->entry_object);
   e_entry_cursor_move_at_start(icon->sd->entry_object);
   
   e_fm_icon_edit_entry_set(icon->icon_object, icon->sd->entry_object);
   e_fm_icon_title_set(icon->icon_object, "");
   
   e_fm_mouse_up_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
						   _e_fm_win_mouse_up_cb, icon);
   e_grabinput_get(icon->sd->win->evas_win, 1, icon->sd->win->evas_win);
   e_fm_grab_time = ecore_time_get();
}

static void
_e_fm_file_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Icon *icon;
   E_Dialog *dia;
   char *text;
   
   icon = data;
   
   dia = e_dialog_new(icon->sd->win->container);
   e_dialog_button_add(dia, "Yes", NULL, _e_fm_file_delete_yes_cb, icon);
   e_dialog_button_add(dia, "No", NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 1);
   e_dialog_title_set(dia, "Confirm");
   text = E_NEW(char, PATH_MAX + 256);
      snprintf(text, PATH_MAX + 256, " Are you sure you want to delete <br><b>%s</b> ?", icon->file->name);
   e_dialog_text_set(dia, text);
   free(text);
   e_dialog_show(dia);
}

static void
_e_fm_file_delete_yes_cb(void *data, E_Dialog *dia)
{
   E_Fm_Icon *icon;
   E_Fm_Smart_Data *sd;
   
   icon = data;
   sd = icon->sd;
   /*
    * TODO
    _e_fm_file_delete(file);
    e_object_del(E_OBJECT(dia));
    _e_fm_redraw(sd); // no_new
    */
}
   
#if 0
static void
_e_fm_icon_prop_fill_data( E_Fm_Icon_CFData *cfdata)
{
   cfdata->protect = 1
   cfdata->readwrite = 0;
}

static void *
_e_fm_icon_prop_create_data(E_Config_Dialog *cfd)
{
   E_Fm_Icon_CFData *cfdata;
   
   cfdata = E_NEW(CFData, 1);
   _fill_data(cfdata);
   return cfdata;   
}

static void
_e_fm_icon_prop_free_data(E_Config_Dialog *cfd, E_Fm_Icon_CFData *cfdata)
{   
   free(cfdata);
}

static int
_e_fm_icon_prop_basic_apply_data(E_Config_Dialog *cfd, E_Fm_Icon_CFData *cfdata)
{
   return 1;
}
   
static int
_e_fm_icon_prop_advanced_apply_data(E_Config_Dialog *cfd, E_Fm_Icon_CFData *cfdata)
{
   return 1;
}

static Evas_Object *
_e_fm_icon_prop_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Fm_Icon_CFData *cfdata)
{
   E_Fm_File *file;
   E_Win *win;
   Evas_Object *table;
   Evas_Object *name;
   Evas_Coord w, h;
   struct group *grp;
   struct passwd *usr;
   struct tm *t;
   char *fullname;
   char *size, *username, *groupname, *lastaccess, *lastmod, *permissions;
   char text[512];
   E_Dialog *dia;
   Evas_Object *o, *ol, *hb;
   Evas_Coord mw, mh;
   E_Radio_Group *rg;	   
   Evas *e;
   
   file = cfdata->file;

   _e_fm_icon_prop_fill_data(cfdata);
   
   
   size = E_NEW(char, 64);
   snprintf(size, 64, "%ld KB", file->attr->size / 1024);

   username = E_NEW(char, 128); // max length of username?
   usr = getpwuid(file->attr->owner);
   snprintf(username, 128, "%s", usr->pw_name);
   //free(usr);

   groupname = E_NEW(char, 128); // max length of group?
   grp = getgrgid(file->attr->group);
   snprintf(groupname, 128, "%s", grp->gr_name);
   //free(grp);

   t = gmtime(&file->attr->atime);
   lastaccess = E_NEW(char, 128);
   strftime(lastaccess, 128, "%a %b %d %T %Y", t);

   t = gmtime(&file->attr->mtime);
   lastmod = E_NEW(char, 128);
   strftime(lastmod, 128, "%a %b %d %T %Y", t);

   permissions = E_NEW(char, 128); // todo
   snprintf(permissions, 128, "%s", "");
    
   dia = e_dialog_new(file->sd->win->container);
   e_dialog_title_set(dia, _("Properties"));   		   
   e = e_win_evas_get(dia->win);   
   
   ol = e_widget_list_add(e, 0, 0);
   
   hb = e_widget_list_add(e, 1, 1);
   
   o = e_widget_frametable_add(e, _("General"), 0);
   
   snprintf(text, 512, _("File:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s", file->attr->name);
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 0, 1, 1,
				    1, 1, 1, 1);
   
   snprintf(text, 512, _("Size:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s Kb", size);
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 1, 1, 1,
				    1, 1, 1, 1);
   
   snprintf(text, 512, _("Type:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s", "An Image");
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 2, 1, 1,
				    1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_check_add(e, _("Protect this file"), &dummy_val),
			    0, 3, 2, 1,
			    1, 1, 1, 1);
   
   rg = e_widget_radio_group_new(&dummy_val);
   
   e_widget_frametable_object_append(o, e_widget_radio_add(e, _("Let others see this file"), 0, rg),
				     0, 4, 2, 1,
				     1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_radio_add(e, _("Let others modify this file"), 0, rg),
				     0, 5, 2, 1,
				     1, 1, 1, 1);
   
   e_widget_list_object_append(ol, hb, 1, 0, 0.0);
   
   e_widget_min_size_get(ol, &w, &h);
   
   return ol;
   
}

static Evas_Object *
_e_fm_icon_prop_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Fm_Icon_CFData *cfdata)
{
   
}

static void
_e_fm_file_menu_properties(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_File *file;   
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   file = data;
   
   
   /* methods */
   v.create_cfdata           = _e_fm_icon_prop_create_data;
   v.free_cfdata             = _e_fm_icon_prop_free_data;
   v.basic.apply_cfdata      = _e_fm_icon_prop_basic_apply_data;
   v.basic.create_widgets    = _e_fm_icon_prop_basic_create_widgets;
   v.advanced.apply_cfdata   = _e_fm_icon_prop_advanced_apply_data;
   v.advanced.create_widgets = _e_fm_icon_prop_advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(file->sd->win->container, _("Properties"), NULL, 0, &v, NULL);
   
}

static void
_e_fm_file_menu_properties(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_File *file;
   E_Win *win;
   Evas_Object *table;
   Evas_Object *name;
   Evas_Object *bg;
   Evas_Coord w, h;
   struct group *grp;
   struct passwd *usr;
   struct tm *t;
   char *fullname;
   char *size, *username, *groupname, *lastaccess, *lastmod, *permissions;
   char text[512];
   E_Dialog *dia;
   Evas_Object *o, *ol, *hb;
   Evas_Coord mw, mh;
   E_Radio_Group *rg;	   
   Evas *e;
   
   file = data;
         	               
   size = E_NEW(char, 64);
   snprintf(size, 64, "%ld KB", file->attr->size / 1024);

   username = E_NEW(char, 128); // max length of username?
   usr = getpwuid(file->attr->owner);
   snprintf(username, 128, "%s", usr->pw_name);
   //free(usr);

   groupname = E_NEW(char, 128); // max length of group?
   grp = getgrgid(file->attr->group);
   snprintf(groupname, 128, "%s", grp->gr_name);
   //free(grp);

   t = gmtime(&file->attr->atime);
   lastaccess = E_NEW(char, 128);
   strftime(lastaccess, 128, "%a %b %d %T %Y", t);

   t = gmtime(&file->attr->mtime);
   lastmod = E_NEW(char, 128);
   strftime(lastmod, 128, "%a %b %d %T %Y", t);

   permissions = E_NEW(char, 128); // todo
   snprintf(permissions, 128, "%s", "");
    
   dia = e_dialog_new(file->sd->win->container);
   e_dialog_title_set(dia, _("Properties"));   		   
   e = e_win_evas_get(dia->win);   
   
   ol = e_widget_list_add(e, 0, 0);
   
   hb = e_widget_list_add(e, 1, 1);
   
   o = e_widget_frametable_add(e, _("General"), 0);
   
   snprintf(text, 512, _("File:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 0, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s", file->attr->name);
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 0, 1, 1,
				    1, 1, 1, 1);
   
   snprintf(text, 512, _("Size:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 1, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s Kb", size);
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 1, 1, 1,
				    1, 1, 1, 1);
   
   snprintf(text, 512, _("Type:"));
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				     0, 2, 1, 1,
				     1, 1, 1, 1);
   snprintf(text, 512, "%s", "An Image");
   e_widget_frametable_object_append(o, e_widget_label_add(e, text),
				    1, 2, 1, 1,
				    1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_check_add(e, _("Protect this file"), &dummy_val),
			    0, 3, 2, 1,
			    1, 1, 1, 1);
   
   rg = e_widget_radio_group_new(&dummy_val);
   
   e_widget_frametable_object_append(o, e_widget_radio_add(e, _("Let others see this file"), 0, rg),
				     0, 4, 2, 1,
				     1, 1, 1, 1);
   
   e_widget_frametable_object_append(o, e_widget_radio_add(e, _("Let others modify this file"), 0, rg),
				     0, 5, 2, 1,
				     1, 1, 1, 1);
   
/* Use those in advanced dialog.
   
   snprintf(text, 512, _("Owner:"));
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   snprintf(text, 512, "%s", username);
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   
   snprintf(text, 512, _("Group:"));
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   snprintf(text, 512, "%s", groupname);
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   
   snprintf(text, 512, _("Type:"));
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   switch(_e_fm_file_type(file))
    {
     case E_FILEMAN_FILETYPE_DIRECTORY:
       snprintf(text, 512, _("Directory"));
       break;
     case E_FILEMAN_FILETYPE_FILE:
       snprintf(text, 512, _("File"));
       break;
     case E_FILEMAN_FILETYPE_SYMLINK:
       snprintf(text, 512, _("Symlink"));
       break;
     default:
       snprintf(text, 512, _("Unknown"));
       break;
    }       
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   
   snprintf(text, 512, _("Last Access:"));
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   snprintf(text, 512, "%s", lastaccess);
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   
   snprintf(text, 512, _("Last Modification"));
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
   snprintf(text, 512, "%s", lastmod);
   e_widget_framelist_object_append(o, e_widget_label_add(e, text));
  
 */ 
 
   e_widget_list_object_append(hb, o, 0, 1, 0.0);
   
/*   use those in the advanced dialog
   
   o = e_widget_framelist_add(e, _("Permissions"), 0);
   
   e_widget_framelist_object_append(o, e_widget_label_add(e, _("User:")));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Read"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Write"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Execute"), &dummy_val));
								     
   e_widget_framelist_object_append(o, e_widget_label_add(e, _("Group:")));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Read"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Write"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Execute"), &dummy_val));
   
   e_widget_framelist_object_append(o, e_widget_label_add(e, _("World:")));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Read"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Write"), &dummy_val));
   e_widget_framelist_object_append(o, e_widget_check_add(e, _("Execute"), &dummy_val));   

   e_widget_list_object_append(hb, o, 0, 0, 0.0);
  
 */
 
   e_widget_list_object_append(ol, hb, 1, 0, 0.0);
   
   o = e_widget_button_add(e, _("Advanced"), "enlightenment/configuration", NULL, NULL, NULL);
   e_widget_list_object_append(ol, o, 0, 0, 1.0);
   
   
   e_widget_min_size_get(ol, &w, &h);
   
   e_dialog_content_set(dia, ol, w, h);
        
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   //e_dialog_button_add(dia, _("Apply"), "enlightenment/reset", NULL, NULL);
   e_dialog_button_add(dia, _("Cancel"), "enlightenment/exit", NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);

   file->prop.dia = dia;
   
   return;   
}
   
#endif

static void
_e_fm_menu_arrange_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Smart_Data *sd;
   
   sd = data;
   
   switch (e_menu_item_num_get(mi))
    {
     case E_FILEMAN_CANVAS_ARRANGE_NAME:
       sd->files = evas_list_sort(sd->files, evas_list_count(sd->files), _e_fm_files_sort_name_cb);
       sd->arrange = E_FILEMAN_CANVAS_ARRANGE_NAME;
       _e_fm_redraw(sd); // no_new
       break;
       
     case E_FILEMAN_CANVAS_ARRANGE_MODTIME:
       sd->files = evas_list_sort(sd->files, evas_list_count(sd->files), _e_fm_files_sort_modtime_cb);
       sd->arrange = E_FILEMAN_CANVAS_ARRANGE_MODTIME;
       _e_fm_redraw(sd); // no new
       break;
    }
}

static void
_e_fm_menu_refresh_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fm_Smart_Data *sd;
   
   sd = data;
   /* TODO: Update files */
   _e_fm_redraw(sd);
}

static void
_e_fm_dir_set(E_Fm_Smart_Data *sd, const char *dir)
{
   Evas_List *l;
   E_Event_Fm_Reconfigure *ev;
   
   if (!dir) return;
   if ((sd->dir) && (!strcmp(sd->dir, dir))) return;
   
   if (sd->dir) free (sd->dir);
   sd->dir = strdup(dir);

   /* Reset position */
   sd->position = 0.0;
   
   /* Clear old selection */
   _e_fm_selections_clear(sd);
   
   /* Remove old files */
   while (sd->files)
    {
       _e_fm_file_free(sd->files->data);
       sd->files = evas_list_remove_list(sd->files, sd->files);
    }
   
   /* Get new files */
   sd->files = _e_fm_dir_files_get(sd, E_FM_FILE_TYPE_NORMAL);
//   if (sd->monitor) ecore_file_monitor_del(sd->monitor);
//   sd->monitor = ecore_file_monitor_add(sd->dir, _e_fm_dir_monitor_cb, sd);
   /* Get special prev dir */
   if (strcmp(sd->dir, "/"))
    {
       E_Fm_Icon *icon;
       char path[PATH_MAX];
       
       icon = E_NEW(E_Fm_Icon, 1);
       if (icon)
	{
	   snprintf(path, sizeof(path), "%s/..", sd->dir);
	   icon->file = e_fm_file_new(path);
	   icon->file->mode = 0040000;
	   icon->file->type = E_FM_FILE_TYPE_DIRECTORY;
	   icon->icon_object = e_fm_icon_add(sd->evas);
	   icon->sd = sd;
	   e_fm_icon_file_set(icon->icon_object, icon->file);
	   sd->files = evas_list_prepend(sd->files, icon);
	}
    }
   
   /* Add icons to layout */
   e_icon_layout_freeze(sd->layout);
   for (l = sd->files; l; l = l->next)
    {
       E_Fm_Icon *icon;
       
       icon = l->data;
       
	{ 
	   Evas_Coord icon_w, icon_h;
	   	   
	   evas_object_resize(icon->icon_object, sd->icon_info.w, sd->icon_info.h);
	   evas_object_show(icon->icon_object);	   
	   evas_object_geometry_get(icon->icon_object, NULL, NULL, &icon_w, &icon_h);
	   D(("_e_fm_dir_set: Icon, w=%d h=%d\n", icon_w, icon_h));
	}
       
       e_icon_layout_pack(sd->layout, icon->icon_object);
       
       evas_object_event_callback_add(icon->icon_object, EVAS_CALLBACK_MOUSE_DOWN, _e_fm_icon_mouse_down_cb, icon);
       evas_object_event_callback_add(icon->icon_object, EVAS_CALLBACK_MOUSE_UP, _e_fm_icon_mouse_up_cb, icon);
       evas_object_event_callback_add(icon->icon_object, EVAS_CALLBACK_MOUSE_IN, _e_fm_icon_mouse_in_cb, icon);
       evas_object_event_callback_add(icon->icon_object, EVAS_CALLBACK_MOUSE_OUT, _e_fm_icon_mouse_out_cb, icon);
    }
   e_icon_layout_thaw(sd->layout);
   
   /* Draw layout */
   _e_fm_redraw(sd);
   
   /* raise dir change event */
   ev = E_NEW(E_Event_Fm_Directory_Change, 1);
   if (ev)
    {       
       ev->object = sd->object;
       ev->w = sd->max.w;
       ev->h = sd->max.h;
       ecore_event_add(E_EVENT_FM_DIRECTORY_CHANGE, ev, NULL, NULL);
    }      
   
   if(sd->frozen)
     return;
   
   /* raise reconfigure event */
   ev = E_NEW(E_Event_Fm_Reconfigure, 1);
   if (ev)
    {
       e_icon_layout_virtual_size_get(sd->layout, &sd->max.w, &sd->max.h);
       ev->object = sd->object;
       ev->w = sd->max.w;
       ev->h = sd->max.h;
       ecore_event_add(E_EVENT_FM_RECONFIGURE, ev, NULL, NULL);
    }   
}

static Evas_List *
_e_fm_dir_files_get(E_Fm_Smart_Data *sd, int type)
{
   DIR           *dir;
   char           path[PATH_MAX];
   struct dirent *dir_entry;
   Evas_List     *files = NULL;
   
   if (!(dir = opendir(sd->dir))) return NULL;
   
   while ((dir_entry = readdir(dir)) != NULL)
    {
       E_Fm_Icon *icon;
       if ((!strcmp(dir_entry->d_name, ".") || (!strcmp (dir_entry->d_name, "..")))) continue;
       if ((dir_entry->d_name[0] == '.') && (!(type & E_FM_FILE_TYPE_HIDDEN))) continue;
       
       icon = E_NEW(E_Fm_Icon, 1);
       if (!icon) continue;
       snprintf(path, sizeof(path), "%s/%s", sd->dir, dir_entry->d_name);
       icon->file = e_fm_file_new(path);
       icon->icon_object = e_fm_icon_add(sd->evas);
       icon->sd = sd;
       e_fm_icon_file_set(icon->icon_object, icon->file);
       evas_object_resize(icon->icon_object, sd->icon_info.w, sd->icon_info.h);       
       files = evas_list_append(files, icon);
    }
   closedir(dir);
   
   files = evas_list_sort(files, evas_list_count(files), _e_fm_files_sort_name_cb);
   
   return files;
}

static char *
_e_fm_dir_pop(const char *path)
{
   char *start, *end, *dir;
   int i;
   
   i = 0;
   start = strchr(path, '/');
   end = strrchr(path ,'/');
   
   if (start == end)
    {
       dir = strdup("/");;
    }
   else if ((!start) || (!end))
    {
       dir = strdup("");
    }
   else
    {
       dir = malloc((end - start + 1));
       if (dir)
	{
	   memcpy(dir, start, end - start);
	   dir[end - start] = 0;
	}
    }
   return dir;
}

static void
_e_fm_dir_monitor_cb(void *data, Ecore_File_Monitor *ecore_file_monitor,
		     Ecore_File_Event event, const char *path)
{
   E_Fm_Smart_Data *sd;
   
   sd = data;
   
   /* FIXME! */
   return;
   
   if (event == ECORE_FILE_EVENT_DELETED_SELF)
    {
       char *dir;
       
       dir = _e_fm_dir_pop(sd->dir);
       /* FIXME: we need to fix this, uber hack alert */
       if (sd->win)
	 e_win_title_set(sd->win, dir);
       _e_fm_dir_set(sd, dir);
       free(dir);
       return;
    }
   
   _e_fm_redraw(sd);
}

static void
_e_fm_file_free(E_Fm_Icon *icon)
{
   e_icon_layout_unpack(icon->icon_object);
   evas_object_del(icon->icon_object);
   e_object_del(E_OBJECT(icon->file));
   /*
    if (file->menu)
    e_object_del(E_OBJECT(file->menu));
    */
   free(icon);
}

static void
_e_fm_selections_clear(E_Fm_Smart_Data *sd)
{
   Evas_List *l;
   
   D(("_e_fm_selections_clear:\n"));
   for (l = sd->selection.files; l; l = l->next)
    {
       E_Fm_Icon *icon;
       
       icon = l->data;
       e_fm_icon_signal_emit(icon->icon_object, "unclicked", "");
       icon->state.selected = 0;
    }
   sd->selection.files = evas_list_free(sd->selection.files);
   sd->selection.band.files = evas_list_free(sd->selection.band.files);
   sd->selection.current_file = NULL;
}

static void
_e_fm_selections_add(E_Fm_Icon *icon)
{
   if (icon->state.selected) return;
   e_fm_icon_signal_emit(icon->icon_object, "clicked", "");
   icon->sd->selection.current_file = icon;
   icon->sd->selection.files = evas_list_append(icon->sd->selection.files, icon);
   icon->state.selected = 1;
}

static void
_e_fm_selections_rect_add(E_Fm_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_List *l;
   
   for (l = sd->files; l; l = l->next)
    {
       E_Fm_Icon *icon;
       Evas_Coord xx, yy, ww, hh;
       
       icon = l->data;
       
       evas_object_geometry_get(icon->icon_object, &xx, &yy, &ww, &hh);
       if (E_INTERSECTS(x, y, w, h, xx, yy, ww, hh))
	{
	   if (!evas_list_find(icon->sd->selection.band.files, icon))
	    {
	       if (icon->state.selected)
		 _e_fm_selections_del(icon);
	       else
		 _e_fm_selections_add(icon);
	       icon->sd->selection.band.files = evas_list_append(icon->sd->selection.band.files, icon);
	    }
	}
       else
	{
	   if (evas_list_find(icon->sd->selection.band.files, icon))
	    {
	       if (icon->state.selected)
		 _e_fm_selections_del(icon);
	       else
		 _e_fm_selections_add(icon);
	       icon->sd->selection.band.files = evas_list_remove(icon->sd->selection.band.files, icon);
	    }
	}
    }   
}

static void
_e_fm_selections_del(E_Fm_Icon *icon)
{
   if (!icon->state.selected) return;
   e_fm_icon_signal_emit(icon->icon_object, "unclicked", "");
   icon->sd->selection.files = evas_list_remove(icon->sd->selection.files, icon);
   if (icon->sd->selection.current_file == icon)
    {
       icon->sd->selection.current_file = NULL;
       if (icon->sd->selection.files)
	 icon->sd->selection.current_file = icon->sd->selection.files->data;
    }
   icon->state.selected = 0;
}

/* fake mouse up */
static void
_e_fm_fake_mouse_up_later(Evas *evas, int button)
{
   E_Fm_Fake_Mouse_Up_Info *info;
   
   info = E_NEW(E_Fm_Fake_Mouse_Up_Info, 1);
   if (!info) return;
   
   info->canvas = evas;
   info->button = button;
   ecore_job_add(_e_fm_fake_mouse_up_cb, info);
}

static void
_e_fm_fake_mouse_up_all_later(Evas *evas)
{
   _e_fm_fake_mouse_up_later(evas, 1);
   _e_fm_fake_mouse_up_later(evas, 2);
   _e_fm_fake_mouse_up_later(evas, 3);
}

static void
_e_fm_fake_mouse_up_cb(void *data)
{
   E_Fm_Fake_Mouse_Up_Info *info;
   
   info = data;
   if (!info) return;
   
   evas_event_feed_mouse_up(info->canvas, info->button, EVAS_BUTTON_NONE, ecore_x_current_time_get(), NULL);
   free(info);
}

/* mouse events on the bg and on the icons */
static void
_e_fm_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Smart_Data *sd;
   Evas_Event_Mouse_Down *ev;
   E_Menu      *mn;
   E_Menu_Item *mi;
   int x, y, w, h;
   
   ev = event_info;
   sd = data;
   
   switch (ev->button)
    {
     case 1:
       
       if (!evas_key_modifier_is_set(evas_key_modifier_get(sd->evas), "Control"))
	 _e_fm_selections_clear(sd);
       
       sd->selection.band.enabled = 1;
       evas_object_move(sd->selection.band.obj, ev->canvas.x, ev->canvas.y);
       evas_object_resize(sd->selection.band.obj, 1, 1);
       evas_object_show(sd->selection.band.obj);
       sd->selection.band.x = ev->canvas.x;
       sd->selection.band.y = ev->canvas.y;
       break;
       
     case 3:
       if (!sd->win) break;
       
       mn = e_menu_new();
       
       sd->menu = mn;
       
       mi = e_menu_item_new(mn);
       e_menu_item_label_set(mi, "Arrange Icons");
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/arrange"),
				 "fileman/button/arrange");
       
       mn = e_menu_new();
       e_menu_item_submenu_set(mi, mn);
       
       mi = e_menu_item_new(mn);
       e_menu_item_label_set(mi, "By Name");
       e_menu_item_radio_set(mi, 1);
       e_menu_item_radio_group_set(mi, 2);
       if (sd->arrange == E_FILEMAN_CANVAS_ARRANGE_NAME) e_menu_item_toggle_set(mi, 1);
       e_menu_item_callback_set(mi, _e_fm_menu_arrange_cb, sd);
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/arrange_name"),
				 "fileman/button/arrange_name");
       
       mi = e_menu_item_new(mn);
       e_menu_item_label_set(mi, "By Mod Time");
       e_menu_item_radio_set(mi, 1);
       e_menu_item_radio_group_set(mi, 2);
       if (sd->arrange == E_FILEMAN_CANVAS_ARRANGE_MODTIME) e_menu_item_toggle_set(mi, 1);
       e_menu_item_callback_set(mi, _e_fm_menu_arrange_cb, sd);
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/arrange_time"),
				 "fileman/button/arrange_time");
       
       mi = e_menu_item_new(sd->menu);
       e_menu_item_label_set(mi, "View");
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/view"),
				 "fileman/button/view");
       
       mn = e_menu_new();
       e_menu_item_submenu_set(mi, mn);
       
       mi = e_menu_item_new(mn);
       e_menu_item_label_set(mi, "Name Only");
       e_menu_item_radio_set(mi, 1);
       e_menu_item_radio_group_set(mi, 2);
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/view_name"),
				 "fileman/button/view_name");
       
       mi = e_menu_item_new(mn);
       e_menu_item_label_set(mi, "Details");
       e_menu_item_radio_set(mi, 1);
       e_menu_item_radio_group_set(mi, 2);
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/view_details"),
				 "fileman/button/view_details");
       
       mi = e_menu_item_new(sd->menu);
       e_menu_item_label_set(mi, "Refresh");
       e_menu_item_callback_set(mi, _e_fm_menu_refresh_cb, sd);
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/refresh"),
				 "fileman/button/refresh");
#if 0       
       mi = e_menu_item_new(sd->menu);
       e_menu_item_label_set(mi, "Properties");
       e_menu_item_icon_edje_set(mi,
				 (char *)e_theme_edje_file_get("base/theme/fileman",
							       "fileman/button/properties"),
				 "fileman/button/properties");
#endif
       ecore_evas_geometry_get(sd->win->ecore_evas, &x, &y, &w, &h);
       
       e_menu_activate_mouse(sd->menu, sd->win->border->zone,
			     ev->output.x + x, ev->output.y + y, 1, 1,
			     E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
       _e_fm_fake_mouse_up_all_later(sd->win->evas);
       break;
    }
}

static void
_e_fm_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Smart_Data *sd;
   Evas_Event_Mouse_Move *ev;

   ev = event_info;
   sd = data;

   if (!sd->selection.band.obj)
     return;

   if (sd->selection.band.enabled)
     {
	Evas_Coord x, y, w, h;

	evas_object_geometry_get(sd->selection.band.obj, &x, &y, &w, &h);

	if ((ev->cur.canvas.x > sd->selection.band.x) &&
	    (ev->cur.canvas.y < sd->selection.band.y))
	  {
	     /* growing towards top right */
	     evas_object_move(sd->selection.band.obj,
			      sd->selection.band.x,
			      ev->cur.canvas.y);
	     evas_object_resize(sd->selection.band.obj,
				ev->cur.canvas.x - sd->selection.band.x,
				sd->selection.band.y - ev->cur.canvas.y);
	  }
	else if ((ev->cur.canvas.x > sd->selection.band.x) &&
		 (ev->cur.canvas.y > sd->selection.band.y))
	  {
	     /* growing towards bottom right */
	     w = ev->cur.canvas.x - sd->selection.band.x;
	     h = ev->cur.canvas.y - sd->selection.band.y;

	     evas_object_resize(sd->selection.band.obj, w, h);
	  }
	else if ((ev->cur.canvas.x < sd->selection.band.x) &&
		 (ev->cur.canvas.y < sd->selection.band.y))
	  {
	     /* growing towards top left */
	     evas_object_move(sd->selection.band.obj,
			      ev->cur.canvas.x,
			      ev->cur.canvas.y);
	     evas_object_resize(sd->selection.band.obj,
				sd->selection.band.x - ev->cur.canvas.x,
				sd->selection.band.y - ev->cur.canvas.y);
	  }
	else if ((ev->cur.canvas.x < sd->selection.band.x) &&
		 (ev->cur.canvas.y > sd->selection.band.y))
	  {
	     /* growing towards button left */
	     evas_object_move(sd->selection.band.obj,
			      ev->cur.canvas.x,
			      sd->selection.band.y);
	     evas_object_resize(sd->selection.band.obj,
				sd->selection.band.x - ev->cur.canvas.x,
				ev->cur.canvas.y - sd->selection.band.y);
	  }
	
	evas_object_geometry_get(sd->selection.band.obj, &x, &y, &w, &h);
	_e_fm_selections_rect_add(sd, x, y, w, h);
     }
}

static void
_e_fm_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Smart_Data *sd;
   Evas_Event_Mouse_Up *ev;

   sd = data;
   ev = event_info;

   if (sd->selection.band.enabled)
     {
	sd->selection.band.enabled = 0;
	evas_object_resize(sd->selection.band.obj, 1, 1);
	evas_object_hide(sd->selection.band.obj);
	sd->selection.band.files = evas_list_free(sd->selection.band.files);
     }
}

// TODO:
// - send signals to edje for animations etc...
static void
_e_fm_icon_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Icon *icon;
   Evas_Event_Mouse_Down *ev;

   ev = event_info;
   icon = data;
   
   if (ev->button == 1)
    {
       if (icon->file->type == E_FM_FILE_TYPE_DIRECTORY && (ev->flags == EVAS_BUTTON_DOUBLE_CLICK))
	 {
	    char *fullname;
	    
	    icon->sd->drag.start = 0;
	    
	    if (!strcmp(icon->file->name, ".")) return;

	    if (!strcmp(icon->file->name, ".."))
	      {
		 fullname = _e_fm_dir_pop(icon->sd->dir);
	      }
	    else
	      {
		 fullname = strdup(icon->file->path);
	      }

	    /* FIXME: we need to fix this, uber hack alert */
	    if (fullname)
	      {
		 if (icon->sd->win)
		   e_win_title_set(icon->sd->win, fullname);
		 _e_fm_dir_set(icon->sd, fullname);
		 free(fullname);
	      }
	 }
       else if (icon->file->type == E_FM_FILE_TYPE_FILE && (ev->flags == EVAS_BUTTON_DOUBLE_CLICK))
	 {
	    icon->sd->drag.start = 0;
	    
	    if(!e_fm_assoc_exec(icon->sd, icon->file->path))
	      if (e_fm_file_can_exec(icon->file))
		e_fm_file_exec(icon->file);	    
	 }
       else
	 {
	    icon->sd->drag.start = 1;
	    icon->sd->drag.y = -1;
	    icon->sd->drag.x = -1;
	    icon->sd->drag.icon_object = icon;

	    if (!icon->state.selected)
	      {
		 if (evas_key_modifier_is_set(evas_key_modifier_get(icon->sd->evas), "Control"))
		   icon->sd->selection.files =
		      evas_list_append(icon->sd->selection.files, icon);
		 else
		   _e_fm_selections_clear(icon->sd);

		 _e_fm_selections_add(icon);
	      }
	    else
	      {
		 if (evas_key_modifier_is_set(evas_key_modifier_get(icon->sd->evas), "Control"))
		   _e_fm_selections_del(icon);
		 else
		   {
		      _e_fm_selections_clear(icon->sd);
		      _e_fm_selections_add(icon);
		   }
	      }

	 }
    }
   else if (ev->button == 3)
     {
	E_Menu      *mn;
	E_Menu_Item *mi;
	int x, y, w, h;

	_e_fm_selections_clear(icon->sd);
	_e_fm_selections_add(icon);	
	
	mn = e_menu_new();
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Open"));
	e_menu_item_callback_set(mi, _e_fm_file_menu_open, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/open"),
				  "fileman/button/open");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Copy"));
	e_menu_item_callback_set(mi, _e_fm_file_menu_copy, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/copy"),
				  "fileman/button/copy");

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Cut"));
	e_menu_item_callback_set(mi, _e_fm_file_menu_cut, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/cut"),
				  "fileman/button/cut");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Rename"));
	e_menu_item_callback_set(mi, _e_fm_file_menu_rename, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/rename"),
				  "fileman/button/rename");

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Delete"));
	e_menu_item_callback_set(mi, _e_fm_file_menu_delete, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/delete"),
				  "fileman/button/delete");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Properties"));
	//e_menu_item_callback_set(mi, _e_fm_file_menu_properties, icon);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/properties"),
				  "fileman/button/properties");

	icon->menu = mn;

	if (!icon->sd->win) return;

	ecore_evas_geometry_get(icon->sd->win->ecore_evas, &x, &y, &w, &h);

	e_menu_activate_mouse(icon->menu, icon->sd->win->border->zone,
			      ev->output.x + x, ev->output.y + y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	_e_fm_fake_mouse_up_all_later(icon->sd->win->evas);
     }
}

static void
_e_fm_icon_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Icon *icon;
   Evas_Event_Mouse_Move *ev;
   
   ev = event_info;
   icon = data;
   
   icon->sd->drag.start = 0;
}

static void
_e_fm_icon_mouse_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Icon *icon;
   Evas_Event_Mouse_In *ev;
   
   ev = event_info;
   icon = data;
   
   e_fm_icon_signal_emit(icon->icon_object, "hilight", "");
   
}

static void
_e_fm_icon_mouse_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fm_Icon *icon;
   Evas_Event_Mouse_Out *ev;
   
   ev = event_info;
   icon = data;
   
   e_fm_icon_signal_emit(icon->icon_object, "default", "");   
}

static int
_e_fm_win_mouse_move_cb(void *data, int type, void *event)
{
   E_Fm_Smart_Data *sd;
   E_Fm_Icon *icon;
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   sd = data;
   
/* TODO - rethink this code */

   icon = sd->drag.icon_object;

   if (!icon) return 1;

   if (sd->drag.start)
     {
	if ((sd->drag.x == -1) && (sd->drag.y == -1))
	  {
	     sd->drag.x = ev->root.x;
	     sd->drag.y = ev->root.y;
	  }
	else
	  {
	     int dx, dy;

	     dx = sd->drag.x - ev->root.x;
	     dy = sd->drag.y - ev->root.y;

	     if (((dx * dx) + (dy * dy)) > (100))
	       {
		  E_Drag *drag;
		  Evas_Object *o = NULL;
		  Evas_Coord x, y, w, h;
		  int cx, cy;
		  char data[PATH_MAX];
		  const char *path = NULL, *part = NULL;
		  const char *drop_types[] = { "text/uri-list" };

		  snprintf(data, sizeof(data), "file://%s", icon->file->path);

		  ecore_evas_geometry_get(sd->win->ecore_evas, &cx, &cy, NULL, NULL);
		  evas_object_geometry_get(icon->icon_object, &x, &y, &w, &h);
		  drag = e_drag_new(sd->win->container, cx + x, cy + y,
				    drop_types, 1, strdup(data), strlen(data),
				    _e_fm_drop_done_cb);
		  
		  o = e_fm_icon_add(drag->evas);
		  e_fm_icon_file_set(o, icon->file);
		  e_fm_icon_title_set(o, "");
		  evas_object_resize(o, w, h);		  
		  
/* This shouldnt be needed anymore
		  edje_object_file_get(icon->icon_object, &path, &part);
		  if ((path) && (part))
		    {
		       o = edje_object_add(drag->evas);
		       edje_object_file_set(o, path, part);
		    }
		  else
		    {
		       int iw, ih;

		       o = evas_object_image_add(drag->evas);
		       evas_object_image_size_get(icon->icon_object, &iw, &ih);
		       evas_object_image_size_set(o, iw, ih);
		       evas_object_image_data_copy_set(o, evas_object_image_data_get(icon->icon_object, 0));
		       evas_object_image_data_update_add(o, 0, 0, iw, ih);
		       evas_object_image_fill_set(o, 0, 0, iw, ih);
		       evas_object_resize(o, iw, ih);
		    }
 */ 
		  if (!o)
		    {
		       // FIXME: fallback icon for drag
		       o = evas_object_rectangle_add(drag->evas);
		       evas_object_color_set(o, 255, 255, 255, 255);
		    }
		  e_drag_object_set(drag, o);

		  e_drag_resize(drag, w, h);
		  e_drag_xdnd_start(drag, sd->drag.x, sd->drag.y);
		  evas_event_feed_mouse_up(sd->evas, 1, EVAS_BUTTON_NONE, ev->time, NULL);
		  sd->drag.start = 0;
	       }
	  }
     }

   return 1;
}

static int   
_e_fm_win_mouse_up_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   E_Fm_Icon *icon;
   double t;
   const char *name;
   
   ev = event;
   icon = data;
   
   t = ecore_time_get() - e_fm_grab_time;
   if (t < 1.0) return 1;
   
   name = e_entry_text_get(icon->sd->entry_object);
   e_fm_icon_edit_entry_set(icon->icon_object, NULL);
   evas_object_focus_set(icon->sd->entry_object, 0);
   evas_object_del(icon->sd->entry_object);
   icon->sd->entry_object = NULL;
   
   e_fm_file_rename(icon->file, name);
   
   ecore_event_handler_del(e_fm_mouse_up_handler);
   e_fm_mouse_up_handler = NULL;
   
          e_grabinput_release(icon->sd->win->evas_win, icon->sd->win->evas_win);
   return 0;
}

 static int
_e_fm_drop_enter_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Enter *ev;
   E_Fm_Smart_Data *sd;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   return 1;
}

static int
_e_fm_drop_leave_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Leave *ev;
   E_Fm_Smart_Data *sd;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   return 1;
}

static int
_e_fm_drop_position_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Position *ev;
   E_Fm_Smart_Data *sd;
   Ecore_X_Rectangle rect;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   rect.x = 0;
   rect.y = 0;
   rect.width = 0;
   rect.height = 0;

   ecore_x_dnd_send_status(1, 0, rect, ECORE_X_DND_ACTION_PRIVATE);

   return 1;
}

static int
_e_fm_drop_drop_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Drop *ev;
   E_Fm_Smart_Data *sd;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   ecore_x_selection_xdnd_request(sd->win->evas_win, "text/uri-list");

   return 1;
}

static int                     
_e_fm_drop_selection_cb(void *data, int type, void *event)
{                                                      
   Ecore_X_Event_Selection_Notify *ev;
   E_Fm_Smart_Data *sd;
   Ecore_X_Selection_Data_Files *files;
   int i;                                              
   
   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;
   
   files = ev->data;
   
   /* FIXME: Add this file to the current files */
   for (i = 0; i < files->num_files; i++)
    {
       char new_file[PATH_MAX];
       
       snprintf(new_file, PATH_MAX, "%s/%s", sd->dir,
		ecore_file_get_file(files->files[i]));
       ecore_file_cp(strstr(files->files[i], "/"), new_file);
    }
   
   ecore_x_dnd_send_finished();
   _e_fm_redraw(sd);
   
   return 1;
}


static void
_e_fm_drop_done_cb(E_Drag *drag, int dropped)
{
   /* FIXME: If someone takes this internal drop, we might want to not free it */
   free(drag->data);
}

/* sort functions */
static int
_e_fm_files_sort_name_cb(void *d1, void *d2)
{
   E_Fm_Icon *e1, *e2;
   
   e1 = d1;
   e2 = d2;
   
   return (strcmp(e1->file->name, e2->file->name));
}

static int
_e_fm_files_sort_modtime_cb(void *d1, void *d2)
{
   E_Fm_Icon *e1, *e2;
   
   e1 = d1;
   e2 = d2;
   
   return (e1->file->mtime > e2->file->mtime);
}

/* associations / mime type functions */
int
e_fm_assoc_exec( E_Fm_Smart_Data *sd, char *file)
{
   char app[PATH_MAX * 2];
   Evas_List *l;
   E_Fm_Assoc_App *assoc;
   Ecore_Exe *exe;
   
   for (l = sd->conf.main->apps; l; l = l->next)
    {
       char *ext;
       
       assoc = l->data;
       ext = strrchr(file, '.');
       if(ext)
	 if(!strcasecmp(ext, assoc->mime))
	   break;
       assoc = NULL;
    }
   
   if(!assoc) return 0;
   
   snprintf(app, PATH_MAX * 2, "%s %s", assoc->app, file);
   exe = ecore_exe_run(app, NULL);
   
   if (!exe)
    {
       e_error_dialog_show(_("Run Error"),
			   _("Enlightenment was unable fork a child process:\n"
			     "\n"
			     "%s\n"
			     "\n"),
			   app);
       D(("e_fm_assoc_exec: fail (%s)\n", app));
       return 0;
    }
   /*
    * ecore_exe_tag_set(exe, "E/app");
    */
   D(("e_fm_assoc_exec: ok (%s)\n", app));
   return 1;      
}

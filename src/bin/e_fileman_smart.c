/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

/* TODO:
 * - when we drop something onto an efm from an efm we might not be releasing
 *   the mouse grab.
 * 
 * - checking wether events belong to us (ecore events)
 *
 * - scrolling
 *
 * - we need a redraw function that will just re-arrange and not do
 *   the whole thing. for example, when we resize, we should just
 *   check the file offset and fill the empty space with icons
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
 * - multi select
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

typedef struct _E_Fileman_Smart_Data         E_Fileman_Smart_Data;
typedef struct _E_Fileman_File               E_Fileman_File;
typedef struct _E_Fileman_Thumb_Pending      E_Fileman_Thumb_Pending;
typedef struct _E_Fileman_Fake_Mouse_Up_Info E_Fileman_Fake_Mouse_Up_Info;

typedef enum   _E_Fileman_File_Type          E_Fileman_File_Type;
typedef enum   _E_Fileman_Arrange            E_Fileman_Arrange;

struct _E_Fileman_File
{
   struct dirent *dir_entry;

   Evas_Object *icon;
   Evas_Object *icon_img;
   Evas_Object *entry;
   Evas_Object *event;
   Evas_Object *title;

   E_Fileman_Smart_Data *sd;

   E_Menu *menu;
   struct {
	Evas_Object *table;
	E_Win *win;
	Evas_Object *bg;
	Evas_List *objects;
   } prop;

   struct {
	unsigned char selected : 1;
   } state;

   void *data;
};

struct _E_Fileman_Thumb_Pending
{
   E_Fileman_File *file;
   pid_t pid;
};

enum _E_Fileman_File_Type
{
   E_FILEMAN_FILETYPE_ALL = 0,
   E_FILEMAN_FILETYPE_NORMAL = 1,
   E_FILEMAN_FILETYPE_DIRECTORY = 2,
   E_FILEMAN_FILETYPE_FILE = 3,
   E_FILEMAN_FILETYPE_HIDDEN = 4,
   E_FILEMAN_FILETYPE_UNKNOWN = 5
};

enum _E_Fileman_Arrange
{
   E_FILEMAN_CANVAS_ARRANGE_NAME = 0,
   E_FILEMAN_CANVAS_ARRANGE_MODTIME = 1,
   E_FILEMAN_CANVAS_ARRANGE_SIZE = 2,
};

struct _E_Fileman_Fake_Mouse_Up_Info
{
   Evas *canvas;
   int button;
};

struct _E_Fileman_Smart_Data
{
   Evas_Object *bg;
   Evas_Object *clip;

   E_Menu *menu;
   E_Win *win;
   Evas *evas;

   Evas_List *event_handlers;
   Evas_List *pending_thumbs;

   char *dir;
   Evas_List *files;
   Evas_List *files_raw;
   Ecore_File_Monitor *monitor;
   int arrange;

   int file_offset;
   int visible_files;
   double position;

   char *thumb_path;

   struct {
	unsigned char start : 1;
	int x, y;
	double time;
	E_Fileman_File *file;
	Ecore_Evas *ecore_evas;
	Evas *evas;
	Ecore_X_Window win;
	Evas_Object *icon;
	Evas_Object *icon_img;
   } drag;

   struct {
	E_Drop_Handler *drop_file;
   } drop;

   struct {
	int w;
	int h;
	int x_space;
	int y_space;
   } icon_info;

   struct {
	int w;
	int h;
   } max;

   struct {
	Evas_List *files;
	E_Fileman_File *current_file;

	struct {
	     unsigned char enabled : 1;
	     Evas_Coord x, y;
	     Evas_Object *obj;
	} band;

   } selection;
};

static void _e_fm_smart_add(Evas_Object *object);
static void _e_fm_smart_del(Evas_Object *object);
static void _e_fm_smart_raise(Evas_Object *object);
static void _e_fm_smart_lower(Evas_Object *object);
static void _e_fm_smart_stack_above(Evas_Object *object, Evas_Object *above);
static void _e_fm_smart_stack_below(Evas_Object *object, Evas_Object *below);
static void _e_fm_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y);
static void _e_fm_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h);
static void _e_fm_smart_show(Evas_Object *object);
static void _e_fm_smart_hide(Evas_Object *object);

static void                _e_fm_redraw_new(E_Fileman_Smart_Data *sd);
static void                _e_fm_redraw_update(E_Fileman_Smart_Data *sd);
static void                _e_fm_size_calc(E_Fileman_Smart_Data *sd);
static void                _e_fm_selections_clear(E_Fileman_Smart_Data *sd);
static void                _e_fm_selections_add(E_Fileman_File *file);
static void                _e_fm_selections_del(E_Fileman_File *file);
static void                _e_fm_selections_add_rect(E_Fileman_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
static void                _e_fm_files_free(E_Fileman_Smart_Data *sd);
static Evas_Bool           _e_fm_file_can_preview(E_Fileman_File *file);
static void                _e_fm_file_delete(E_Fileman_File *file);
static void                _e_fm_file_exec(E_Fileman_File *file);
static E_Fileman_File_Type _e_fm_file_type(E_Fileman_File *file);
static char               *_e_fm_file_fullname(E_Fileman_File *file);
static Evas_Object        *_e_fm_file_icon_mime_get(E_Fileman_File *file);
static Evas_Object        *_e_fm_file_icon_get(E_Fileman_File *file);
static void                _e_fm_file_icon_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_file_icon_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_file_icon_mouse_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_file_icon_mouse_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_file_menu_open(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_copy(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_cut(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_paste(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_rename(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_menu_properties(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_file_delete_yes_cb(void *data, E_Dialog *dia);
static void                _e_fm_file_delete_no_cb(void *data, E_Dialog *dia);
static char               *_e_fm_file_thumb_path_get(char *file);
static Evas_Bool           _e_fm_file_thumb_exists(char *file);
static Evas_Bool           _e_fm_file_thumb_create(char *file);
static Evas_Object        *_e_fm_file_thumb_get(E_Fileman_File *file);
static char               *_e_fm_file_id(char *file);
static void                _e_fm_fake_mouse_up_cb(void *data);
static void                _e_fm_fake_mouse_up_later(Evas *evas, int button);
static void                _e_fm_fake_mouse_up_all_later(Evas *evas);
static int                 _e_fm_dir_files_sort_name_cb(void *d1, void *d2);
static int                 _e_fm_dir_files_sort_modtime_cb(void *d1, void *d2);
static void                _e_fm_dir_set(E_Fileman_Smart_Data *sd, const char *dir);
static void                _e_fm_dir_monitor_cb(void *data, Ecore_File_Monitor *ecore_file_monitor,  Ecore_File_Event event, const char *path);
static Evas_List          *_e_fm_dir_files_get(char *dirname, E_Fileman_File_Type type);
static char               *_e_fm_dir_pop(const char *path);
static void                _e_fm_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_fm_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int                 _e_fm_win_mouse_move_cb(void *data, int type, void *event);
static int                 _e_fm_grabbed_mouse_up_cb(void *data, int type, void *event);
static void                _e_fm_menu_arrange_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void                _e_fm_menu_refresh_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static int                 _e_fm_drop_enter_cb(void *data, int type, void *event);
static int                 _e_fm_drop_leave_cb(void *data, int type, void *event);
static int                 _e_fm_drop_position_cb(void *data, int type, void *event);
static int                 _e_fm_drop_selection_cb(void *data, int type, void *event);
static void                _e_fm_drop_cb(E_Drag *drag, int dropped);
static int                 _e_fm_drop_drop_cb(void *data, int type, void *event);
static int                 _e_fm_thumbnailer_exit(void *data, int type, void *event);

static Ecore_Event_Handler *_e_fm_mouse_up_handler = NULL;
static char *thumb_path;
static double _e_fm_grab_time = 0;
static Evas_Smart *e_fm_smart = NULL;

/* externally accessible functions */

Evas_Object *
e_fm_add(Evas *evas)
{
   if (!e_fm_smart)
     {
	e_fm_smart = evas_smart_new("e_fileman",
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
     }
   return evas_object_smart_add(evas, e_fm_smart);
}

void
e_fm_dir_set(Evas_Object *object, const char *dir)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   _e_fm_dir_set(sd, dir);
}

char *
e_fm_dir_get(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return NULL;

   return strdup(sd->dir);
}

void
e_fm_e_win_set(Evas_Object *object, E_Win *win)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   sd->win = win;
}

E_Win *
e_fm_e_win_get(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return NULL;

   return sd->win;
}

void
e_fm_menu_set(Evas_Object *object, E_Menu *menu)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   if (menu)
     sd->menu = menu;
}

E_Menu *
e_fm_menu_get(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return NULL;

   return sd->menu;
}

void
e_fm_scroll_horizontal(Evas_Object *object, double percent)
{

}

void
e_fm_scroll_vertical(Evas_Object *object, double percent)
{
   E_Fileman_Smart_Data *sd;
   int offsetpx;
   Evas_Coord bx, by, bw, bh;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   /* fix me */
   return;

   evas_object_geometry_get(sd->bg, &bx, &by, &bw, &bh);

   offsetpx = (sd->position - percent) * sd->max.h;
   sd->position = percent;

   printf("offsetpx = %d\n", offsetpx);

   if (offsetpx > 0) // moving up
     {
	int offset;
	int visible;
	Evas_List *dirs;

	offset = sd->file_offset - sd->visible_files;
	visible = sd->visible_files;
	dirs = evas_list_nth_list(sd->files, offset);
	offsetpx = abs(offsetpx);
	printf("downscroll!!\n");
	while (dirs)
	  {
	     Evas_Coord x, y, w, h;
	     E_Fileman_File *file;

	     printf("moving icons %d pixels!\n",offsetpx);

	     file = dirs->data;
	     evas_object_geometry_get(file->icon, &x, &y, &w, &h);

	     if ((y + offsetpx + w) > (by + bh))
	       sd->file_offset--;

	     evas_object_move(file->icon, x, y + offsetpx);

	     dirs = dirs->next;
	  }
     }
   else if (offsetpx < 0) // moving down
     {
	int offset;
	int visible;
	Evas_List *dirs;

	offset = sd->file_offset - sd->visible_files;
	visible = sd->visible_files;
	dirs = evas_list_nth_list(sd->files, offset);
	offsetpx = abs(offsetpx);
	printf("downscroll!!\n");
	while (dirs)
	  {
	     Evas_Coord x, y, w, h;
	     E_Fileman_File *file;

	     printf("moving icons %d pixels!\n",offsetpx);

	     file = dirs->data;
	     evas_object_geometry_get(file->icon, &x, &y, &w, &h);

	     if ((y - offsetpx + w) < by)
	       {
		  sd->file_offset++;
		  sd->visible_files--;
	       }

	     evas_object_move(file->icon, x, y - offsetpx);

	     dirs = dirs->next;
	  }
     }

   _e_fm_redraw_update(sd);
}

static void
_e_fm_redraw_update(E_Fileman_Smart_Data *sd)
{
   Evas_List *dirs = NULL;
   Evas_Coord x, y, w, h;
   Evas_Coord yo;

   E_Fileman_File *file;

   if (!sd->dir)
     return;

   dirs = evas_list_nth(sd->files, sd->file_offset - 1);
   printf("offset is %d\n",sd->file_offset);
   file = dirs->data;
   if (!file) printf("CANT GET FILE!\n");
   evas_object_geometry_get(file->icon, &x, &y, &w, &h);
   x += w + sd->icon_info.x_space;

   evas_object_geometry_get(sd->bg, NULL, &yo, &w, &h);

   dirs = sd->files_raw;

   sd->file_offset -= sd->visible_files;

   dirs = evas_list_nth_list(dirs, sd->file_offset);

   while (dirs)
     {
	struct dirent *dir_entry;
	int icon_w, icon_h;
	Evas_Object *icon;

	if (y > (yo + h))
	  return;

	dir_entry = evas_list_data (dirs);

	icon = edje_object_add(sd->evas);
	e_theme_edje_object_set(icon, "base/theme/fileman",
				"fileman/icon");

	file = E_NEW(E_Fileman_File, 1);
	file->icon = icon;
	file->dir_entry = dir_entry;
	file->sd = sd;
	file->icon_img = _e_fm_file_icon_get(file);
	edje_object_part_swallow(icon, "icon_swallow", file->icon_img);
	edje_object_part_text_set(icon, "icon_title", dir_entry->d_name);
	file->event = evas_object_rectangle_add(sd->evas);
	evas_object_color_set(file->event, 0, 0, 0, 0);

	edje_object_size_min_calc(icon, &icon_w, &icon_h);
	evas_object_resize(icon, icon_w, icon_h);
	evas_object_resize(file->event, icon_w, icon_h);

	if ((x > w) || ((x + icon_w) > w))
	  {
	     x = sd->icon_info.x_space;
	     y += icon_h + sd->icon_info.y_space;
	  }

	evas_object_move(icon, x, y);
	evas_object_move(file->event, x, y);
	evas_object_stack_above(icon, sd->bg);
	evas_object_stack_above(file->event, icon);
	evas_object_clip_set(icon, sd->clip);
	evas_object_clip_set(file->event, sd->clip);
	evas_object_show(icon);
	evas_object_show(file->event);

	x += icon_w + sd->icon_info.x_space;

	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_DOWN, _e_fm_file_icon_mouse_down_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_UP, _e_fm_file_icon_mouse_up_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_IN, _e_fm_file_icon_mouse_in_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_OUT, _e_fm_file_icon_mouse_out_cb, file);
	evas_object_repeat_events_set(file->event, TRUE);

	sd->files = evas_list_append(sd->files, file);

	sd->file_offset++;
	sd->visible_files++;
	dirs = dirs->next;
     }
}

/* local subsystem functions */

static void
_e_fm_smart_add(Evas_Object *object)
{
   Evas *evas;
   char *homedir;
   char dir[PATH_MAX];
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(evas = evas_object_evas_get(object)))
     return;

   sd = E_NEW(E_Fileman_Smart_Data, 1);
   if (!sd) return;

   sd->bg = evas_object_rectangle_add(evas); // this should become an edje
   evas_object_color_set(sd->bg, 0, 0, 0, 0);
   evas_object_show(sd->bg);

   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_fm_mouse_down_cb, sd);
   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_UP,
				  _e_fm_mouse_up_cb, sd);
   evas_object_event_callback_add(sd->bg, EVAS_CALLBACK_MOUSE_MOVE,
				  _e_fm_mouse_move_cb, sd);
   evas_object_smart_member_add(sd->bg, object);

   sd->clip = evas_object_rectangle_add(evas);
   evas_object_smart_member_add(sd->clip, object);
   evas_object_move(sd->clip, -100000, -100000);
   evas_object_resize(sd->clip, 200000, 200000);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);

   evas_object_clip_set(sd->bg, sd->clip);

   sd->icon_info.w = 48;
   sd->icon_info.h = 48;
   sd->icon_info.x_space = 15;
   sd->icon_info.y_space = 15;

   homedir = e_user_homedir_get();
   if (homedir)
     {
	thumb_path = E_NEW(char, PATH_MAX);
	snprintf(thumb_path, PATH_MAX, "%s/.e/e/fileman/thumbnails", homedir);
	if (!ecore_file_exists(thumb_path))
	  ecore_file_mkpath(thumb_path);
	free(homedir);
     }

   sd->monitor = NULL;
   sd->file_offset = 0;
   sd->position = 0;
   sd->evas = evas;
   sd->pending_thumbs = NULL;
   sd->event_handlers = NULL;

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
					 ecore_event_handler_add(ECORE_EVENT_EXE_EXIT,
								 _e_fm_thumbnailer_exit,
								 sd));
   sd->event_handlers = evas_list_append(sd->event_handlers,
					 ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
								 _e_fm_win_mouse_move_cb,
								 sd));
   evas_object_smart_data_set(object, sd);

   if (getcwd(dir, sizeof(dir)))
     _e_fm_dir_set(sd, dir);
}

static void
_e_fm_smart_del(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   _e_fm_files_free(sd);

   for (l = sd->event_handlers; l; l = l->next)
     {
	Ecore_Event_Handler *h;

	h = l->data;
	ecore_event_handler_del(h);
     }
   evas_list_free(sd->event_handlers);
   sd->event_handlers = NULL;

   evas_object_del(sd->selection.band.obj);
   evas_object_del(sd->clip);
   evas_object_del(sd->bg);

   free(sd->dir);
   free(sd);
}

static void
_e_fm_smart_raise(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_raise(sd->bg);
   for (l = sd->files; l; l = l->next)
     {
	E_Fileman_File *file;

	file = l->data;
	evas_object_stack_above(file->icon, sd->bg);
     }
}

static void
_e_fm_smart_lower(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_lower (sd->bg);
   for (l = sd->files; l; l = l->next)
     {
	E_Fileman_File *file;

	file = l->data;
	evas_object_stack_above(file->icon, sd->bg);
     }
}

static void
_e_fm_smart_stack_above(Evas_Object *object, Evas_Object *above)
{
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_above(sd->bg, above);
   for (l = sd->files; l; l = l->next)
     {
	E_Fileman_File *file;

	file = l->data;
	evas_object_stack_above(file->icon, sd->bg);
     }
}

static void
_e_fm_smart_stack_below(Evas_Object *object, Evas_Object *below)
{
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_stack_below(sd->bg, below);
   for (l = sd->files; l; l = l->next)
     {
	E_Fileman_File *file;

	file = l->data;
	evas_object_stack_above(file->icon, sd->bg);
     }
}

static void
_e_fm_smart_move(Evas_Object *object, Evas_Coord x, Evas_Coord y)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_move(sd->bg, x, y);
   evas_object_move(sd->clip, x, y);

   _e_fm_redraw_new(sd); // no new
}

static void
_e_fm_smart_resize(Evas_Object *object, Evas_Coord w, Evas_Coord h)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_resize(sd->bg, w, h);
   evas_object_resize(sd->clip, w, h);
   // optimize
   _e_fm_redraw_new(sd); // no new
}

static void
_e_fm_smart_show(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_show(sd->clip);
}

static void
_e_fm_smart_hide(Evas_Object *object)
{
   E_Fileman_Smart_Data *sd;

   if ((!object) || !(sd = evas_object_smart_data_get(object)))
     return;

   evas_object_hide(sd->clip);
}

static void
_e_fm_redraw_new(E_Fileman_Smart_Data *sd)
{
   Evas_List *dirs = NULL;
   Evas_Coord x, y, w, h;
   Evas_Coord xo, yo;

   E_Fileman_File *file;
   struct dirent *dir_entry;

   if (!sd->dir)
     return;

   if (sd->files)
     _e_fm_files_free(sd);

   evas_object_geometry_get(sd->bg, &x, &y, &w, &h);
   xo = x;
   yo = y;

   x = sd->icon_info.x_space;
   y = sd->icon_info.y_space;

   sd->files_raw = _e_fm_dir_files_get(sd->dir, E_FILEMAN_FILETYPE_NORMAL);
   dirs = sd->files_raw;

   if (sd->monitor)
     ecore_file_monitor_del(sd->monitor);
   sd->monitor = ecore_file_monitor_add(sd->dir, _e_fm_dir_monitor_cb, sd);

   dir_entry = E_NEW(struct dirent, 1);
   dir_entry->d_type = 4;
   snprintf(dir_entry->d_name, NAME_MAX + 1, "..");

   dirs = evas_list_prepend(dirs, dir_entry);

   //sd->file_offset = 0;
   //sd->visible_files = 0;

   sd->file_offset -= sd->visible_files;

   if (sd->file_offset <= 0)
     sd->file_offset = 0;
   else
     dirs = evas_list_nth_list(dirs, sd->file_offset);

   sd->visible_files = 0;

   //_e_fm_size_calc(sd);

   while (dirs)
     {
	struct dirent *dir_entry;
	int icon_w, icon_h;
	Evas_Object *icon;

	if (y > (yo + h)) break;

	dir_entry = (struct dirent*)evas_list_data(dirs);

	icon = edje_object_add(sd->evas);
	e_theme_edje_object_set(icon, "base/theme/fileman",
				"fileman/icon");

	file = E_NEW(E_Fileman_File, 1);
	file->icon = icon;
	file->dir_entry = dir_entry;
	file->sd = sd;
	file->icon_img = _e_fm_file_icon_get(file);
	edje_object_part_swallow(icon, "icon_swallow", file->icon_img);
	edje_object_part_text_set(icon, "icon_title", dir_entry->d_name);
	file->event = evas_object_rectangle_add(sd->evas);
	evas_object_color_set(file->event, 0, 0, 0, 0);

#if 0
	  {
	     Evas_Textblock_Style *e_editable_text_style;

	     e_editable_text_style = evas_textblock2_style_new();
	     evas_textblock2_style_set(e_editable_text_style, "DEFAULT='font=Vera font_size=10 align=center color=#000000 wrap=char'");

	     file->title = evas_object_textblock2_add(sd->evas);
	     evas_object_textblock2_style_set(file->title, e_editable_text_style);
	     evas_object_textblock2_text_markup_set(file->title, dir_entry->d_name);

	     evas_object_resize(file->title,  sd->icon_info.w*2, 1);
	     evas_object_textblock2_size_formatted_get(file->title, &fw, &fh);
	     evas_object_textblock2_style_insets_get(file->title, &il, &ir, &it, &ib);
	     evas_object_resize(file->title, sd->icon_info.w*2, fh + it + ib);
	     edje_extern_object_min_size_set(file->title, sd->icon_info.w*2, fh + it + ib);
	     edje_object_part_swallow(icon, "icon_title", file->title);
	  }
#endif

	edje_object_size_min_calc(icon, &icon_w, &icon_h);
	evas_object_resize(icon, icon_w, icon_h);
	evas_object_resize(file->event, icon_w, icon_h);

	if ((x > w) || ((x + icon_w) > w))
	  {
	     x = sd->icon_info.x_space;
	     y += icon_h + sd->icon_info.y_space;
	  }

	evas_object_move(icon, x, y);
	evas_object_move(file->event, x, y);
	evas_object_stack_above(icon, sd->bg);
	evas_object_stack_above(file->event, icon);
	evas_object_clip_set(icon, sd->clip);
	evas_object_clip_set(file->event, sd->clip);
	evas_object_show(icon);
	evas_object_show(file->event);

	x += icon_w + sd->icon_info.x_space;

	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_DOWN, _e_fm_file_icon_mouse_down_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_UP, _e_fm_file_icon_mouse_up_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_IN, _e_fm_file_icon_mouse_in_cb, file);
	evas_object_event_callback_add(file->event, EVAS_CALLBACK_MOUSE_OUT, _e_fm_file_icon_mouse_out_cb, file);
	evas_object_repeat_events_set(file->event, FALSE);

	sd->files = evas_list_append(sd->files, file);

	sd->file_offset++;
	sd->visible_files++;
	dirs = dirs->next;
     }
}

// when this is enabled, the thumbnailer is broken
static void
_e_fm_size_calc(E_Fileman_Smart_Data *sd)
{
   Evas_List *dirs = NULL;
   Evas_Coord x, y, w, h;
   E_Fileman_File *file;
   struct dirent *dir_entry;

   if (!sd->dir)
     return;

   evas_object_geometry_get(sd->bg, &x, &y, &w, &h);

   x = 0;
   y = 0;

   dirs = sd->files_raw;

   dir_entry = E_NEW(struct dirent, 1);
   dir_entry->d_type = 4;
   snprintf(dir_entry->d_name, NAME_MAX + 1, "..");

   dirs = evas_list_prepend(dirs, dir_entry);

   while (dirs)
     {
	struct dirent *dir_entry;
	int icon_w, icon_h;
	Evas_Object *icon;

	dir_entry = (struct dirent*)evas_list_data(dirs);

	icon = edje_object_add(sd->evas);
	e_theme_edje_object_set(icon, "base/theme/fileman", "fileman/icon");

	file = E_NEW(E_Fileman_File, 1);
	file->icon = icon;
	file->dir_entry = dir_entry;
	file->sd = sd;
	file->icon_img = _e_fm_file_icon_get(file); // this might be causing borkage
	edje_object_part_swallow(icon, "icon_swallow", file->icon_img);
	edje_object_part_text_set(icon, "icon_title", dir_entry->d_name);
	edje_object_size_min_calc(icon, &icon_w, &icon_h);

	if ((x > w) || ((x + icon_w) > w))
	  {
	     x = sd->icon_info.x_space;
	     y += icon_h + sd->icon_info.y_space;
	  }

	x += icon_w + sd->icon_info.x_space;

	evas_object_del(file->icon);
	evas_object_del(file->icon_img);
	free(file);

	dirs = dirs->next;
     }

   sd->max.w = x; // not really max w.
   sd->max.h = y;
}

static void
_e_fm_selections_clear(E_Fileman_Smart_Data *sd)
{
   Evas_List *l;

   for (l = sd->selection.files; l; l = l->next)
     {
	E_Fileman_File *file;

	file = l->data;
	if (!file) continue;
	edje_object_signal_emit(file->icon, "unclicked", "");
	edje_object_signal_emit(file->icon_img, "unclicked", "");
	file->state.selected = 0;
     }
   sd->selection.files = evas_list_free(sd->selection.files);
   sd->selection.current_file = NULL;
}

static void
_e_fm_selections_add(E_Fileman_File *file)
{
   if (!file)
     return;

   edje_object_signal_emit(file->icon, "clicked", "");
   edje_object_signal_emit(file->icon_img, "clicked", "");
   file->sd->selection.current_file = file;
   file->state.selected = 1;
   file->sd->selection.files = evas_list_append(file->sd->selection.files, file);
}

static void
_e_fm_selections_add_rect(E_Fileman_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_List *l;
      
   for (l = sd->files; l; l = l->next)
     {
	E_Fileman_File *file;
	Evas_Coord xx, yy, ww, hh;

	file = l->data;
	if (!file) continue;

	evas_object_geometry_get(file->icon, &xx, &yy, &ww, &hh);
	if(E_INTERSECTS(x, y, w, h, xx, yy, ww, hh))
	 {
	    if(!file->state.selected)
	     {
		_e_fm_selections_add(file);
	     }
	 } else {
	    if(file->state.selected) // todo: add control+rubberband
	      _e_fm_selections_del(file);
	 }
     }
}

static void                
_e_fm_selections_del(E_Fileman_File *file)
{
   if (!file)
     return;

   edje_object_signal_emit(file->icon, "unclicked", "");
   edje_object_signal_emit(file->icon_img, "unclicked", "");
   file->state.selected = 0;
   file->sd->selection.files = evas_list_remove(file->sd->selection.files, file);
   file->sd->selection.current_file = evas_list_nth(file->sd->selection.files, 0);
}

static void
_e_fm_dir_set(E_Fileman_Smart_Data *sd, const char *dir)
{
   if ((!sd) || (!dir)) return;
   if ((sd->dir) && (!strcmp(sd->dir, dir))) return;

   if (sd->dir) free (sd->dir);
   sd->dir = strdup(dir);

   _e_fm_selections_clear(sd);
   _e_fm_redraw_new(sd);
}

static void
_e_fm_files_free(E_Fileman_Smart_Data *sd)
{
   E_Fileman_File *file;
   Evas_List *l;

   if (!sd->files)
     return;

   for (l = sd->files; l; l = l->next)
     {
	file = l->data;
	evas_object_del(file->icon_img);
	evas_object_del(file->icon);
	evas_object_del(file->entry);
	evas_object_del(file->title);
	evas_object_del(file->event);
	file->sd = NULL;

	E_FREE(file->dir_entry);
	if (file->menu)
	  e_object_del(E_OBJECT(file->menu));
	free(file);
     }
   evas_list_free(sd->files);

   sd->files = NULL;
   sd->drag.file = NULL;

   _e_fm_selections_clear(sd);
}

static void
_e_fm_dir_monitor_cb(void *data, Ecore_File_Monitor *ecore_file_monitor,
		     Ecore_File_Event event, const char *path)
{
   E_Fileman_Smart_Data *sd;

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

   // OPTIMIZE !!
   _e_fm_redraw_new(sd);
}

static Evas_List *
_e_fm_dir_files_get(char *dirname, E_Fileman_File_Type type)
{
   DIR *dir;
   struct dirent *dir_entry;
   Evas_List *files;

   files = NULL;

   if ((dir = opendir(dirname)) == NULL)
     return NULL;

   while ((dir_entry = readdir(dir)) != NULL)
     {
	struct dirent *dir_entry2;

	if ((!strcmp(dir_entry->d_name, ".") || (!strcmp (dir_entry->d_name, "..")))) continue;
	if ((dir_entry->d_name[0] == '.') && (type != E_FILEMAN_FILETYPE_HIDDEN)) continue;

	dir_entry2 = E_NEW(struct dirent, 1);
	dir_entry2->d_ino = dir_entry->d_ino;
	// dir_entry2->d_off = dir_entry->d_off; // not portable
	// dir_entry2->d_reclen = dir_entry->d_reclen; // note portable
	dir_entry2->d_type = dir_entry->d_type;
	strncpy(dir_entry2->d_name, dir_entry->d_name, NAME_MAX);

	files = evas_list_append(files, dir_entry2);
     }

   closedir(dir);

   files = evas_list_sort(files, evas_list_count(files), _e_fm_dir_files_sort_name_cb);

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

static Evas_Bool
_e_fm_file_can_preview(E_Fileman_File *file)
{
   if (!file) return 0;

   if (file->dir_entry->d_type == 8)
     {
	char *ext;

	ext = strrchr(file->dir_entry->d_name, '.');
	if (!ext) return 0;
	return (!strcasecmp(ext, ".jpg")) || (!strcasecmp(ext, ".png"));
     }
   return 0;
}

static void
_e_fm_file_rename(E_Fileman_File *file, const char* name)
{
   char old_name[PATH_MAX], new_name[PATH_MAX];

   if (!name || !name[0])
     return;

   edje_object_part_text_set(file->icon, "icon_title", name);
   snprintf(old_name, PATH_MAX, "%s/%s", file->sd->dir, file->dir_entry->d_name);
   snprintf(new_name, PATH_MAX, "%s/%s", file->sd->dir, name);
   ecore_file_mv(old_name, new_name);
}

// todo: check if the deletion was ok, if not, pop up dialog
static void
_e_fm_file_delete(E_Fileman_File *file)
{
   char full_name[PATH_MAX];

   if (!file)
     return;

   snprintf(full_name, PATH_MAX, "%s/%s", file->sd->dir, file->dir_entry->d_name);
   if (!ecore_file_unlink(full_name))
     {
	E_Dialog *dia;
	E_Fileman *fileman;
	char *text;

	fileman = file->data;
	dia = e_dialog_new(fileman->con);
	e_dialog_button_add(dia, "Ok", NULL, _e_fm_file_delete_no_cb, file);
	e_dialog_button_focus_num(dia, 1);
	e_dialog_title_set(dia, "Error");
	text = E_NEW(char, PATH_MAX + 256);
	snprintf(text, PATH_MAX + 256, "Could not delete  <br><b>%s</b> ?", file->dir_entry->d_name);
	e_dialog_text_set(dia, text);

	e_dialog_show(dia);
     }
}

// TODO: overhaul
static Evas_Object *
_e_fm_file_icon_mime_get(E_Fileman_File *file)
{
   Evas_Object *icon_img;

   if (!file) return NULL;

   icon_img = edje_object_add(file->sd->evas);

   if (file->dir_entry->d_type == 4)
     {
	e_theme_edje_object_set(icon_img, "base/theme/fileman",
				"fileman/icons/folder");
     }
   else
     {
	char *ext;

	ext = strrchr(file->dir_entry->d_name, '.');
	if (ext)
	  {
	     if (!strcasecmp(ext, ".pdf"))
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/pdf");
	     else if (!strcasecmp(ext, ".c"))
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/c");
	     else if (!strcasecmp(ext, ".h"))
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/h");
	     else if (!strcasecmp(ext, ".jpg"))
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/jpg");
	     else if (!strcasecmp(ext, ".png"))
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/png");
	     else
	       e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/file");
	  }
	else
	  e_theme_edje_object_set(icon_img, "base/theme/fileman", "fileman/icons/file");
     }

   return icon_img;
}

static int
_e_fm_thumbnailer_exit(void *data, int type, void *event)
{
   Ecore_Event_Exe_Exit *ev;
   E_Fileman_Smart_Data *sd;
   Evas_List *l;

   ev = event;
   sd = data;

   for (l = sd->pending_thumbs; l; l = l->next)
     {
	E_Fileman_Thumb_Pending *pthumb;
	Evas_Object *thumb;

	pthumb = l->data;
	if (pthumb->pid != ev->pid) continue;

	if (pthumb->file->icon_img)
	  {
	     edje_object_part_unswallow(pthumb->file->icon, pthumb->file->icon_img);
	     evas_object_del(pthumb->file->icon_img);
	     pthumb->file->icon_img = NULL;
	  }

	thumb = _e_fm_file_thumb_get(pthumb->file);
	if (thumb)
	  {
	     pthumb->file->icon_img = thumb;
	     edje_object_part_swallow(pthumb->file->icon, "icon_swallow", pthumb->file->icon_img);
	  }

	sd->pending_thumbs = evas_list_remove(sd->pending_thumbs, pthumb);
	break;
     }

   return 1;
}

static void
_e_fm_file_thumb_generate_job(void *data)
{
   E_Fileman_Thumb_Pending *pthumb;
   pid_t pid;
   E_Fileman_File *file;

   file = data;
   if (!file)
     return;

   pthumb = E_NEW(E_Fileman_Thumb_Pending, 1);
   pthumb->file = file;

   pid = fork();

   if (pid == 0)
     {
	/* child */
	char *fullname;

	fullname = _e_fm_file_fullname(file);
	if (fullname)
	  {
	     if (!_e_fm_file_thumb_exists(fullname))
	       _e_fm_file_thumb_create(fullname);
	     free(fullname);
	  }
	exit(0);
     }
   else if (pid > 0)
     {
	/* parent */
	pthumb->pid = pid;
	file->sd->pending_thumbs = evas_list_append(file->sd->pending_thumbs, pthumb);
     }
}

static Evas_Object *
_e_fm_file_icon_get(E_Fileman_File *file)
{
   char *fullname;

   if (!file)
     return NULL;

   if (!_e_fm_file_can_preview(file))
     return _e_fm_file_icon_mime_get(file);

   fullname = _e_fm_file_fullname(file);
   if (fullname)
     {
	Evas_Object *o = NULL;
	if (_e_fm_file_thumb_exists(fullname))
	  o = _e_fm_file_thumb_get(file);
	free(fullname);
	if (o) return o;
     }

   ecore_job_add(_e_fm_file_thumb_generate_job, file);
   return _e_fm_file_icon_mime_get(file);
}

static void
_e_fm_file_menu_open(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;
   char *fullname;

   file = data;

   switch (_e_fm_file_type(file))
     {
      case E_FILEMAN_FILETYPE_DIRECTORY:
	 fullname = _e_fm_file_fullname(file);
	 _e_fm_dir_set(file->sd, fullname);
	 free(fullname);
	 break;
      case E_FILEMAN_FILETYPE_FILE:
      case E_FILEMAN_FILETYPE_ALL:
      case E_FILEMAN_FILETYPE_NORMAL:
      case E_FILEMAN_FILETYPE_HIDDEN:
      case E_FILEMAN_FILETYPE_UNKNOWN:
	 break;
     }
}

static void
_e_fm_file_menu_copy(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;

   file = data;
}

static void
_e_fm_file_menu_cut(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;

   file = data;
}

static void
_e_fm_file_menu_paste(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;

   file = data;
}

// TODO: This needs to capture the mouse and end
// the entry when we click anywhere.
static void
_e_fm_file_menu_rename(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;

   file = data;
   file->entry = e_entry_add(file->sd->evas);
   edje_object_part_swallow(file->icon, "icon_title_edit_swallow", file->entry);
   edje_object_part_text_set(file->icon, "icon_title", "");
   evas_object_focus_set(file->entry, 1);
   evas_object_show(file->entry);
   e_entry_cursor_show(file->entry);
   e_entry_text_set(file->entry, file->dir_entry->d_name);
   e_entry_cursor_move_at_end(file->entry);
   e_entry_cursor_move_at_start(file->entry);

   _e_fm_mouse_up_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_BUTTON_UP,
						    _e_fm_grabbed_mouse_up_cb, file);
   e_grabinput_get(file->sd->win->evas_win,
		   1, file->sd->win->evas_win);

   _e_fm_grab_time = ecore_time_get();
}

static void
_e_fm_file_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;
   E_Dialog *dia;
   char *text;

   file = data;
   dia = e_dialog_new(file->sd->win->container);
   e_dialog_button_add(dia, "Yes", NULL, _e_fm_file_delete_yes_cb, file);
   e_dialog_button_add(dia, "No", NULL, _e_fm_file_delete_no_cb, file);
   e_dialog_button_focus_num(dia, 1);
   e_dialog_title_set(dia, "Confirm");
   text = E_NEW(char, PATH_MAX + 256);
   snprintf(text, PATH_MAX + 256, " Are you sure you want to delete <br><b>%s</b> ?", file->dir_entry->d_name);
   e_dialog_text_set(dia, text);
   e_dialog_show(dia);
}

static void
_e_fm_file_delete_yes_cb(void *data, E_Dialog *dia)
{
   E_Fileman_File *file;

   file = data;

   _e_fm_file_delete(file);
   e_object_del(E_OBJECT(dia));
   _e_fm_redraw_new(file->sd); // no_new
}

static void
_e_fm_file_delete_no_cb(void *data, E_Dialog *dia)
{
   e_object_del(E_OBJECT(dia));
}

static void
_e_fm_file_menu_properties_del_cb(E_Win *win)
{
   E_Fileman_File *file;
   Evas_List *l;

   file = win->data;
   evas_object_del(file->prop.table);

   for (l = file->prop.objects; l; l = l->next)
     evas_object_del(l->data);

   evas_object_del(file->prop.bg);

   e_object_del(E_OBJECT(win));
}

static void
_e_fm_file_menu_properties_resize_cb(E_Win *win)
{
   E_Fileman_File *file;

   file = win->data;
   evas_object_resize(file->prop.bg, win->w, win->h);
}

static void
_e_fm_file_menu_properties(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_File *file;
   E_Win *win;
   Evas_Object *table;
   Evas_Object *name;
   Evas_Object *bg;
   Evas_Coord w, h;
   struct stat st;
   struct group *grp;
   struct passwd *usr;
   struct tm *t;
   char *fullname;
   char *size, *username, *groupname, *lastaccess, *lastmod, *permissions;

   file = data;

   fullname = _e_fm_file_fullname(file);
   stat(fullname, &st);
   free(fullname);

   size = E_NEW(char, 64);
   snprintf(size, 64, "%ld KB", st.st_size / 1024);

   username = E_NEW(char, 128); // max length of username?
   usr = getpwuid(st.st_uid);
   snprintf(username, 128, "%s", usr->pw_name);
   //free(usr);

   groupname = E_NEW(char, 128); // max length of group?
   grp = getgrgid(st.st_gid);
   snprintf(groupname, 128, "%s", grp->gr_name);
   //free(grp);

   t = gmtime(&st.st_atime);
   lastaccess = E_NEW(char, 128);
   strftime(lastaccess, 128, "%a %b %d %T %Y", t);

   t = gmtime(&st.st_mtime);
   lastmod = E_NEW(char, 128);
   strftime(lastmod, 128, "%a %b %d %T %Y", t);

   permissions = E_NEW(char, 128); // todo
   snprintf(permissions, 128, "%s", "");

   win = e_win_new(file->sd->win->container);
   e_win_delete_callback_set(win, _e_fm_file_menu_properties_del_cb);
   e_win_resize_callback_set(win, _e_fm_file_menu_properties_resize_cb);
   win->data = file;

   bg = edje_object_add(win->evas);
   e_theme_edje_object_set(bg, "base/theme/fileman/properties", "fileman/properties");
   edje_object_part_text_set(bg, "title", file->dir_entry->d_name);
   evas_object_move(bg, 0, 0);
   evas_object_show(bg);

   table = e_table_add(win->evas);
   e_table_homogenous_set(table, 1);

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Name");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 0, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Owner");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 1, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Group");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 2, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Size");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 3, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Last Accessed");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 4, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Last Modified");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 5, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, "Permissions");
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 0, 6, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, file->dir_entry->d_name);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 0, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, username);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 1, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, groupname);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 2, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, size);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 3, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, lastaccess);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 4, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, lastmod);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 5, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   name = evas_object_text_add(win->evas);
   evas_object_text_font_set(name, "Vera", 10);
   evas_object_text_text_set(name, permissions);
   evas_object_color_set(name, 0, 0, 0, 255);
   evas_object_geometry_get(name, NULL, NULL, &w, &h);
   evas_object_show(name);
   file->prop.objects = evas_list_append(file->prop.objects, name);
   e_table_pack(table, name, 1, 6, 1, 1);
   e_table_pack_options_set(name,
			    1, 1, 1, 1, /* fill */
			    0, 0.5,     /* align */
			    w, h,       /* min w, h */
			    w, h);      /* max w, h */

   file->prop.table = table;;
   file->prop.win = win;
   file->prop.bg = bg;

   edje_object_part_swallow(file->prop.bg, "content_swallow", file->prop.table);

   evas_object_show(table);

   e_box_min_size_get(table, &w, &h);
   evas_object_resize(table, w, h);

   // do proper calculation
   w += 20;
   h += 50;

   if (w < 223) w = 223;
   if (h < 209) h = 209;

   e_win_title_set(win, "Properties");
   e_win_resize(win, w, h);
   e_win_size_min_set(win, w, h);
   e_win_size_base_set(win, w, h);
   e_win_size_max_set(win, w, h);
   e_win_show(win);
}

static void
_e_fm_fake_mouse_up_cb(void *data)
{
   E_Fileman_Fake_Mouse_Up_Info *info;

   info = data;
   if (!info) return;

   evas_event_feed_mouse_up(info->canvas, info->button, EVAS_BUTTON_NONE, ecore_x_current_time_get(), NULL);
   free(info);
}

static void
_e_fm_fake_mouse_up_later(Evas *evas, int button)
{
   E_Fileman_Fake_Mouse_Up_Info *info;

   info = E_NEW(E_Fileman_Fake_Mouse_Up_Info, 1);
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

static int
_e_fm_dir_files_sort_name_cb(void *d1, void *d2)
{
   struct dirent *de1, *de2;

   de1 = (struct dirent*)d1;
   de2 = (struct dirent*)d2;

   return (strcmp(de1->d_name, de2->d_name));
}

static int
_e_fm_dir_files_sort_modtime_cb(void *d1, void *d2)
{
   struct dirent *de1, *de2;

   /* FIXME! */

   de1 = (struct dirent*)d1;
   de2 = (struct dirent*)d2;

   return (strcmp(de1->d_name, de2->d_name));
}

static void
_e_fm_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_Smart_Data *sd;
   Evas_Event_Mouse_Up *ev;

   sd = data;
   ev = event_info;

   if (!sd->selection.band.obj)
     return;

   if (sd->selection.band.enabled)
     {
	sd->selection.band.enabled = 0;
	evas_object_resize(sd->selection.band.obj, 1, 1);
	evas_object_hide(sd->selection.band.obj);
     }
}

static void
_e_fm_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_Smart_Data *sd;
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
	_e_fm_selections_add_rect(sd, x, y, w, h);
     }
}

static void
_e_fm_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_Smart_Data *sd;
   Evas_Event_Mouse_Down *ev;
   E_Menu      *mn;
   E_Menu_Item *mi;
   int x, y, w, h;

   ev = event_info;
   sd = data;

   printf("mouse down!!\n");

   switch (ev->button)
     {
      case 1:
	 if (evas_key_modifier_is_set(evas_key_modifier_get(sd->evas), "Control"))
	   {

	   }
	 else
	   {
	      _e_fm_selections_clear(sd);
	      sd->selection.band.enabled = 1;
	      evas_object_move(sd->selection.band.obj, ev->canvas.x, ev->canvas.y);
	      evas_object_resize(sd->selection.band.obj, 1, 1);
	      evas_object_show(sd->selection.band.obj);
	      sd->selection.band.x = ev->canvas.x;
	      sd->selection.band.y = ev->canvas.y;
	   }
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

	 mi = e_menu_item_new(sd->menu);
	 e_menu_item_label_set(mi, "Properties");
	 e_menu_item_icon_edje_set(mi,
				   (char *)e_theme_edje_file_get("base/theme/fileman",
								 "fileman/button/properties"),
				   "fileman/button/properties");

	 ecore_evas_geometry_get(sd->win->ecore_evas, &x, &y, &w, &h);

	 e_menu_activate_mouse(sd->menu, sd->win->border->zone,
			       ev->output.x + x, ev->output.y + y, 1, 1,
			       E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	 _e_fm_fake_mouse_up_all_later(sd->win->evas);
	 break;
     }
}

static void
_e_fm_file_icon_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_File *file;
   Evas_Event_Mouse_Move *ev;

   ev = event_info;
   file = data;

   file->sd->drag.start = 0;
}

// TODO:
// - send signals to edje for animations etc...
// - look at case where we have // and no /
static void
_e_fm_file_icon_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_File *file;
   Evas_Event_Mouse_Down *ev;

   ev = event_info;
   file = data;

   if (ev->button == 1)
    {
       if ((file->dir_entry->d_type == 4) && (ev->flags == EVAS_BUTTON_DOUBLE_CLICK))
	 {
	    char *fullname = NULL;

	    file->sd->drag.start = 0;

	    if (!strcmp(file->dir_entry->d_name, ".")) return;

	    if (!strcmp(file->dir_entry->d_name, ".."))
	      {
		 fullname = _e_fm_dir_pop(file->sd->dir);
	      }
	    else
	      {
		 fullname = _e_fm_file_fullname(file);
	      }

	    /* FIXME: we need to fix this, uber hack alert */
	    if (fullname)
	      {
		 if (file->sd->win)
		   e_win_title_set(file->sd->win, fullname);
		 _e_fm_dir_set(file->sd, fullname);
		 free(fullname);
	      }
	 }
       else if ((file->dir_entry->d_type == 8) && (ev->flags == EVAS_BUTTON_DOUBLE_CLICK))
	 {
	    char *fullname;

	    file->sd->drag.start = 0;

	    fullname = _e_fm_file_fullname(file);
	    if (ecore_file_can_exec(fullname))
	      _e_fm_file_exec(file);
	    free(fullname);
	 }
       else
	 {
	    file->sd->drag.start = 1;
	    file->sd->drag.y = -1;
	    file->sd->drag.x = -1;
	    file->sd->drag.file = file;
	    printf("drag file: %s\n", file->dir_entry->d_name);

	    if (!file->state.selected)
	      {
		 if (evas_key_modifier_is_set(evas_key_modifier_get(file->sd->evas), "Control"))
		   file->sd->selection.files =
		      evas_list_append(file->sd->selection.files, file);
		 else
		   _e_fm_selections_clear(file->sd);

		 _e_fm_selections_add(file);
	      }
	    else
	      {
		 if (evas_key_modifier_is_set(evas_key_modifier_get(file->sd->evas), "Control"))
		   _e_fm_selections_del(file);
		 else
		   {
		      _e_fm_selections_clear(file->sd);
		      _e_fm_selections_add(file);
		   }
	      }
	 }
    }
   else if (ev->button == 3)
     {
	E_Menu      *mn;
	E_Menu_Item *mi;
	int x, y, w, h;

	mn = e_menu_new();

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Open");
	e_menu_item_callback_set(mi, _e_fm_file_menu_open, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/open"),
				  "fileman/button/open");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Copy");
	e_menu_item_callback_set(mi, _e_fm_file_menu_copy, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/copy"),
				  "fileman/button/copy");

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Cut");
	e_menu_item_callback_set(mi, _e_fm_file_menu_cut, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/cut"),
				  "fileman/button/cut");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Rename");
	e_menu_item_callback_set(mi, _e_fm_file_menu_rename, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/rename"),
				  "fileman/button/rename");

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Delete");
	e_menu_item_callback_set(mi, _e_fm_file_menu_delete, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/delete"),
				  "fileman/button/delete");

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Properties");
	e_menu_item_callback_set(mi, _e_fm_file_menu_properties, file);
	e_menu_item_icon_edje_set(mi,
				  (char *)e_theme_edje_file_get("base/theme/fileman",
								"fileman/button/properties"),
				  "fileman/button/properties");

	file->menu = mn;

	if (!file->sd->win) return;

	ecore_evas_geometry_get(file->sd->win->ecore_evas, &x, &y, &w, &h);

	e_menu_activate_mouse(file->menu, file->sd->win->border->zone,
			      ev->output.x + x, ev->output.y + y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	_e_fm_fake_mouse_up_all_later(file->sd->win->evas);
     }
}

static void
_e_fm_file_icon_mouse_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_File *file;
   Evas_Event_Mouse_In *ev;

   ev = event_info;
   file = data;

   edje_object_signal_emit(file->icon, "hilight", "");
   edje_object_signal_emit(file->icon_img, "hilight", "");
}

static void
_e_fm_file_icon_mouse_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Fileman_File *file;
   Evas_Event_Mouse_Out *ev;

   ev = event_info;
   file = data;

   edje_object_signal_emit(file->icon, "default", "");
   edje_object_signal_emit(file->icon_img, "default", "");
}

static void
_e_fm_menu_arrange_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_Smart_Data *sd;

   sd = data;

   switch (e_menu_item_num_get(mi))
     {
      case E_FILEMAN_CANVAS_ARRANGE_NAME:
	 sd->files = evas_list_sort(sd->files, evas_list_count(sd->files), _e_fm_dir_files_sort_name_cb);
	 sd->arrange = E_FILEMAN_CANVAS_ARRANGE_NAME;
	 _e_fm_redraw_new(sd); // no_new
	 break;

      case E_FILEMAN_CANVAS_ARRANGE_MODTIME:
	 sd->files = evas_list_sort(sd->files, evas_list_count(sd->files), _e_fm_dir_files_sort_modtime_cb);
	 sd->arrange = E_FILEMAN_CANVAS_ARRANGE_MODTIME;
	 _e_fm_redraw_new(sd); // no new
	 break;
     }
}

static void
_e_fm_drop_cb(E_Drag *drag, int dropped)
{
   /* FIXME: If someone takes this internal drop, we might want to not free it */
   free(drag->data);
}

// TODO: add images for icons with image thumb and not edje part
static int
_e_fm_win_mouse_move_cb(void *data, int type, void *event)
{
   E_Fileman_Smart_Data *sd;
   E_Fileman_File *file;
   Ecore_X_Event_Mouse_Move *ev;

   ev = event;
   sd = data;
   file = sd->drag.file;

   if (!file) return 1;

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

		  snprintf(data, sizeof(data), "file://%s/%s", sd->dir, file->dir_entry->d_name);

		  ecore_evas_geometry_get(sd->win->ecore_evas, &cx, &cy, NULL, NULL);
		  evas_object_geometry_get(file->icon_img, &x, &y, &w, &h);
		  drag = e_drag_new(sd->win->container, cx + x, cy + y,
				    drop_types, 1, strdup(data), strlen(data),
				    _e_fm_drop_cb);

		  edje_object_file_get(file->icon_img, &path, &part);
		  if ((path) && (part))
		    {
		       o = edje_object_add(drag->evas);
		       edje_object_file_set(o, path, part);
		    }
		  else
		    {
		       int iw, ih;

		       o = evas_object_image_add(drag->evas);
		       evas_object_image_size_get(file->icon_img, &iw, &ih);
		       evas_object_image_size_set(o, iw, ih);
		       evas_object_image_data_copy_set(o, evas_object_image_data_get(file->icon_img, 0));
		       evas_object_image_data_update_add(o, 0, 0, iw, ih);
		       evas_object_image_fill_set(o, 0, 0, iw, ih);
		       evas_object_resize(o, iw, ih);
		    }
		  if (!o)
		    {
		       /* FIXME: fallback icon for drag */
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
_e_fm_drop_enter_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Enter *ev;
   E_Fileman_Smart_Data *sd;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   return 1;
}

static int
_e_fm_drop_leave_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Leave *ev;
   E_Fileman_Smart_Data *sd;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   return 1;
}

static int
_e_fm_drop_position_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Xdnd_Position *ev;
   E_Fileman_Smart_Data *sd;
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
   E_Fileman_Smart_Data *sd;

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
   E_Fileman_Smart_Data *sd;
   Ecore_X_Selection_Data_Files *files;
   int i;

   ev = event;
   sd = data;
   if (ev->win != sd->win->evas_win) return 1;

   files = ev->data;

   for (i = 0; i < files->num_files; i++)
    {
       char new_file[PATH_MAX];

       snprintf(new_file, PATH_MAX, "%s/%s", sd->dir,
		ecore_file_get_file(files->files[i]));
       ecore_file_cp(strstr(files->files[i], "/"), new_file);
    }

   ecore_x_dnd_send_finished();
   _e_fm_redraw_new(sd);

   return 1;
}

static void
_e_fm_menu_refresh_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Fileman_Smart_Data *sd;

   sd = data;
   // optimize!!
   _e_fm_redraw_new(sd);
}

static int
_e_fm_grabbed_mouse_up_cb(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Button_Up *ev;
   double t;
   E_Fileman_File *file;
   const char *name;

   ev = event;
   file = data;

   t = ecore_time_get() - _e_fm_grab_time;

   if (t < 1.0)
     return 1;

   name = e_entry_text_get(file->entry);
   edje_object_part_unswallow(file->icon, file->entry);
   evas_object_focus_set(file->entry, 0);
   evas_object_del(file->entry);
   file->entry = NULL;

   _e_fm_file_rename(file, name);

   ecore_event_handler_del(_e_fm_mouse_up_handler);

   e_grabinput_release(file->sd->win->evas_win, file->sd->win->evas_win);
   return 0;
}

static char *
_e_fm_file_thumb_path_get(char *file)
{
   char *id;
   char thumb[PATH_MAX];
   id = _e_fm_file_id(file);
   snprintf(thumb, sizeof(thumb), "%s/%s", thumb_path, id);
   free(id);
   return strdup(thumb);
}

static Evas_Bool
_e_fm_file_thumb_exists(char *file)
{
   char *thumb;
   Evas_Bool ret;

   thumb = _e_fm_file_thumb_path_get(file);
   ret = ecore_file_exists(thumb);
   free(thumb);

   return ret;
}

static Evas_Bool
_e_fm_file_thumb_create(char *file)
{
   Eet_File *ef;
   char *thumbpath;
   Evas_Object *im;
   const int *data;
   int size;
   Ecore_Evas *buf;
   Evas *evasbuf;

   thumbpath = _e_fm_file_thumb_path_get(file);

   ef = eet_open(thumbpath, EET_FILE_MODE_WRITE);
   if (!ef)
     {
	free(thumpath);
       	return -1;
     }
   free(thumbpath);

   // we need to remove the hardcode somehow.
   //buf = ecore_evas_buffer_new(file->sd->icon_info.w,file->sd->icon_info.h);
   buf = ecore_evas_buffer_new(48, 48);
   evasbuf = ecore_evas_get(buf);
   im = evas_object_image_add(evasbuf);
   evas_object_image_file_set(im, file, NULL);
   evas_object_image_fill_set(im, 0, 0, 48, 48);
   evas_object_resize(im, 48, 48);
   evas_object_show(im);
   data = ecore_evas_buffer_pixels_get(buf);

   if ((size = eet_data_image_write(ef, "/thumbnail/data", (void *)data, 48, 48, 1, 0, 70, 1)) < 0)
     {
	printf("BUG: Couldn't write thumb db\n");
     }

   eet_close(ef);

   ecore_evas_free(buf);
   return 1;
}

static Evas_Object *
_e_fm_file_thumb_get(E_Fileman_File *file)
{
   Eet_File *ef;
   char *thumb, *fullname;
   Evas_Object *im = NULL;
   void *data;
   unsigned int w, h;
   int a, c, q, l;


   fullname = _e_fm_file_fullname(file);
   if (!_e_fm_file_thumb_exists(fullname))
     _e_fm_file_thumb_create(fullname);

   thumb = _e_fm_file_thumb_path_get(fullname);

   ef = eet_open(thumb, EET_FILE_MODE_READ);
   if (!ef)
     {
	free(fullname);
	free(thumb);
       	return NULL;
     }
   free(fullname);
   free(thumb);

   data = eet_data_image_read(ef, "/thumbnail/data", &w, &h, &a, &c, &q, &l);
   if (data)
    {
       im = evas_object_image_add(file->sd->evas);
       evas_object_image_alpha_set(im, 0);
       evas_object_image_size_set(im, w, h);
       evas_object_image_smooth_scale_set(im, 0);
       evas_object_image_data_copy_set(im, data);
       evas_object_image_data_update_add(im, 0, 0, w, h);
       evas_object_image_fill_set(im, 0, 0, w, h);
       evas_object_resize(im, w, h);
       free(data);
    }
   eet_close(ef);
   return im;
}

static E_Fileman_File_Type
_e_fm_file_type(E_Fileman_File *file)
{
   switch (file->dir_entry->d_type)
     {
      case 8:
	 return E_FILEMAN_FILETYPE_DIRECTORY;
      case 4:
	 return E_FILEMAN_FILETYPE_FILE;
      default:
	 return E_FILEMAN_FILETYPE_UNKNOWN;
     }
}

static void
_e_fm_file_exec(E_Fileman_File *file)
{
   Ecore_Exe *exe;
   char *fullname;

   fullname = _e_fm_file_fullname(file);
   exe = ecore_exe_run(fullname, NULL);
   if (!exe)
     {
	e_error_dialog_show(_("Run Error"),
			    _("Enlightenment was unable fork a child process:\n"
			      "\n"
			      "%s\n"
			      "\n"),
			    fullname);
	free(fullname);
	return;
     }
   ecore_exe_tag_set(exe, "E/app");
   free(fullname);
}

static char *
_e_fm_file_fullname(E_Fileman_File *file)
{
   char fullname[PATH_MAX];

   if (!strcmp(file->sd->dir, "/"))
     snprintf(fullname, sizeof(fullname), "/%s", file->dir_entry->d_name);
   else
     snprintf(fullname, sizeof(fullname), "%s/%s", file->sd->dir, file->dir_entry->d_name);

   return strdup(fullname);
}

static char *
_e_fm_file_id(char *file)
{
   char                s[256];
   const char         *chmap =
     "0123456789abcdefghijklmnopqrstuvwxyzÄÅÇÉÑÖÜáàäãåçéèêëíìù-_";
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

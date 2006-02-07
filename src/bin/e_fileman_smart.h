/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fm_Smart_Data             E_Fm_Smart_Data;
typedef struct _E_Fm_Icon                   E_Fm_Icon;
typedef struct _E_Fm_Icon_CFData            E_Fm_Icon_CFData;
typedef struct _E_Fm_Config                 E_Fm_Config;
typedef struct _E_Fm_Dir_Metadata           E_Fm_Dir_Metadata;
typedef struct _E_Fm_Fake_Mouse_Up_Info     E_Fm_Fake_Mouse_Up_Info;
typedef enum   _E_Fm_Arrange                E_Fm_Arrange;
typedef enum   _E_Fm_State                  E_Fm_State;
typedef struct _E_Event_Fm_Reconfigure      E_Event_Fm_Reconfigure;
typedef struct _E_Event_Fm_Directory_Change E_Event_Fm_Directory_Change;
typedef struct _E_Fm_Assoc_App              E_Fm_Assoc_App;

#else
#ifndef E_FM_SMART_H
#define E_FM_SMART_H

struct _E_Fm_Config
{
   int width;
   int height;
   Evas_List *apps;
};

struct _E_Fm_Dir_Metadata
{
   char *name;             /* dir name */
   char *bg;               /* dir's custom bg */
   int   view;             /* dir's saved view type */
   Evas_List *files;       /* files in dir */

   /* these are generated post-load */
   Evas_Hash *files_hash;  /* quick lookup hash */
};

struct _E_Fm_Icon
{
   E_Fm_File       *file;
   Evas_Object     *icon_obj;
   E_Fm_Smart_Data *sd;

   struct {
      unsigned char selected : 1;
   }
   state;

   E_Menu *menu;
};

struct _E_Fm_Icon_CFData
{
   /*- BASIC -*/
   int protect;
   int readwrite;
   /*- ADVANCED -*/
   struct {
      int r;
      int w;
      int x;
   }
   user, group, world;
   /*- common -*/
   E_Fm_Icon *icon;
};

enum _E_Fm_Arrange
{
   E_FILEMAN_CANVAS_ARRANGE_NAME = 0,
     E_FILEMAN_CANVAS_ARRANGE_MODTIME = 1,
     E_FILEMAN_CANVAS_ARRANGE_SIZE = 2,
};

enum _E_Fm_State
{
   E_FILEMAN_STATE_IDLE = 0,
     E_FILEMAN_STATE_TYPEBUFFER = 1,
     E_FILEMAN_STATE_RENAME = 2,
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

   Evas_Object *edje_obj;
   Evas_Object *event_obj;
   Evas_Object *clip_obj;
   Evas_Object *layout;
   Evas_Object *object;
   Evas_Object *entry_obj;

   E_Fm_Dir_Metadata *meta;

   Evas_Hash *mime_menu_hash;

   char *dir;
   DIR  *dir2;

   double timer_int;
   Ecore_Timer *timer;

   Evas_List *event_handlers;

   Evas_List *files;
   Evas_List *files_raw;
   Ecore_File_Monitor *monitor;
   E_Fm_Arrange arrange;

   E_Fm_State state;
   //  E_Fm_Icon *active_file;
   int frozen;
   double position;

   int is_selector;
   void (*selector_func) (Evas_Object *object, char *file, void *data);
   void  *selector_data;
   void (*selector_hilite_func) (Evas_Object *object, char *file, void *data);

   Evas_Coord x, y, w, h;

   struct {
      unsigned char start : 1;
      int x, y;
      int dx, dy;
      Ecore_Evas *ecore_evas;
      Evas *evas;
      Ecore_X_Window win;
      E_Fm_Icon *icon_obj;
      Evas_Object *image_object;
   }
   drag;

   struct {
      Evas_Coord x_space, y_space, w, h;
   }
   icon_info;

   struct {
      Evas_Coord x, y, w, h;
   }
   child;

   struct {
      Evas_List *files;

      struct {
	 E_Fm_Icon *file;
	 Evas_List *ptr;
      }
      current;

      struct {
	 unsigned char enabled : 1;
	 Evas_Coord x, y;
	 Evas_Object *obj;
	 Evas_List *files;
      }
      band;

   }
   selection;

   struct {
      E_Config_DD *main_edd;
      E_Fm_Config *main;
   }
   conf;
};

struct _E_Event_Fm_Reconfigure  
{
   Evas_Object *object;
   Evas_Coord x, y, w, h;   
};

struct _E_Event_Fm_Directory_Change
{
   Evas_Object *object;
   Evas_Coord w, h;  
};

struct _E_Fm_Assoc_App
{
   char *mime;
   char *app;
};

EAPI int                   e_fm_init(void);
EAPI int                   e_fm_shutdown(void);
EAPI Evas_Object          *e_fm_add(Evas *evas);
EAPI void                  e_fm_dir_set(Evas_Object *object, const char *dir);
EAPI char                 *e_fm_dir_get(Evas_Object *object);
EAPI void                  e_fm_e_win_set(Evas_Object *object, E_Win *win);
EAPI E_Win                *e_fm_e_win_get(Evas_Object *object);
EAPI void                  e_fm_menu_set(Evas_Object *object, E_Menu *menu);
EAPI E_Menu               *e_fm_menu_get(Evas_Object *object);

EAPI void                  e_fm_scroll_set(Evas_Object *object, Evas_Coord x, Evas_Coord y);
EAPI void                  e_fm_scroll_get(Evas_Object *object, Evas_Coord *x, Evas_Coord *y);
EAPI void                  e_fm_scroll_max_get(Evas_Object *object, Evas_Coord *x, Evas_Coord *y);

EAPI void                  e_fm_scroll_horizontal(Evas_Object *object, double percent);
EAPI void                  e_fm_scroll_vertical(Evas_Object *object, double percent);

EAPI void                  e_fm_geometry_virtual_get(Evas_Object *object, Evas_Coord *w, Evas_Coord *h);
EAPI void                  e_fm_reconfigure_callback_add(Evas_Object *object, void (*func)(void *data, Evas_Object *obj, E_Event_Fm_Reconfigure *ev), void *data);
EAPI int                   e_fm_freeze(Evas_Object *freeze);
EAPI int                   e_fm_thaw(Evas_Object *freeze);
EAPI void                  e_fm_selector_enable(Evas_Object *object, void (*func)(Evas_Object *object, char *file, void *data), void (*hilite_func)(Evas_Object *object, char *file, void *data), void *data);
EAPI void                  e_fm_background_set(Evas_Object *object, Evas_Object *bg);

EAPI Evas_Object          *e_fm_icon_create(void *data);
EAPI void                  e_fm_icon_destroy(Evas_Object *obj, void *data);

extern int E_EVENT_FM_RECONFIGURE;
extern int E_EVENT_FM_DIRECTORY_CHANGE;
#endif
#endif

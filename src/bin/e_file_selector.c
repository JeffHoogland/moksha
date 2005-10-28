/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*- DESCRIPTION -*/
/* This is a list of files in a scrollbable container. When a file is selected
 * this object will return the file name.
 */

#define SMART_NAME "e_fileselector"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
#define THEME_SET(OBJ, GROUP) e_theme_edje_object_set(OBJ, "base/theme/widgets/fileselector", "widgets/fileselector/"GROUP)

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{ 
   Evas_Object   *parent;
   Evas_Object   *theme; /* theme object */
   Evas_Object   *files; /* file view object */

   int            view;  /* view type, icons, list */
   
   Evas_Coord     x, y, w, h; /* coords */

   void         (*func) (Evas_Object *obj, char *file, void *data); /* selection cb */
   void          *func_data; /* selection cb data */
}; 

/* local subsystem functions */
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_smart_clip_unset(Evas_Object *obj);
static void _e_smart_init(void);
static void _e_file_selector_selected_cb(Evas_Object *obj, char *file, void *data);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
e_file_selector_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

void
e_file_selector_view_set(Evas_Object *obj, int type)
{
   API_ENTRY return;
   
   switch(type)
     {
      case E_FILE_SELECTOR_ICONVIEW:
	break;
	
      case E_FILE_SELECTOR_LISTVIEW:
	break;
     }   
}

int
e_file_selector_view_get(Evas_Object *obj)
{
   API_ENTRY return;
   return sd->view;
}

void
e_file_selector_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void *data)
{
   API_ENTRY return;
   sd->func = func;
   sd->func_data = data;
}

/* local subsystem functions */
static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->parent = obj;
   sd->func = NULL;
   sd->func_data = NULL;
   sd->view = E_FILE_SELECTOR_ICONVIEW;
   
   evas = evas_object_evas_get(obj);   
   sd->theme = edje_object_add(evas);
   evas_object_smart_member_add(sd->theme, obj);   
   THEME_SET(sd->theme, "main");
      
   sd->files = e_fm_add(evas);
   e_fm_selector_enable(sd->files, _e_file_selector_selected_cb, sd);
   evas_object_smart_member_add(sd->files, obj);

   edje_object_part_swallow(sd->theme, "items", sd->files);
   
   evas_object_smart_data_set(obj, sd);
   
   evas_object_show(sd->theme);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
 
   evas_object_del(sd->theme);
   evas_object_del(sd->files);
   
   free(sd);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->theme, x, y);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->theme, w, h);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->theme);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->theme);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->theme, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->theme, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->theme);
}  

static void
_e_file_selector_selected_cb(Evas_Object *obj, char *file, void *data)
{
   E_Smart_Data *sd;
   
   sd = data;
   if(sd->func)
     sd->func(sd->parent, file, sd->func_data);
}

/* never need to touch this */

static void
_e_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new(SMART_NAME,
			     _e_smart_add,
			     _e_smart_del, 
			     NULL, NULL, NULL, NULL, NULL,
			     _e_smart_move,
			     _e_smart_resize,
			     _e_smart_show,
			     _e_smart_hide,
			     _e_smart_color_set,
			     _e_smart_clip_set,
			     _e_smart_clip_unset,
			     NULL);
}

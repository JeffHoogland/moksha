/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_deskpreview"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object   *smart_obj;
   Evas_Object   *table_obj;
   Evas_Object   *scrollframe_obj;
   struct {
      int x, y, w, h;
      int zone, container;
   } region;
   Evas_List     *items;
};

struct _E_Smart_Item
{
   Evas_Object *edje_obj;
   Evas_Object *desk_obj;
   int          x, y, container, zone;
};

/* local subsystem functions */
static void _e_smart_reconfigure(E_Smart_Data *sd);
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

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
e_deskpreview_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

void
e_deskpreview_mini_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   API_ENTRY return;
}

void
e_deskpreview_region_set(Evas_Object *obj, int x, int y, int w, int h, int zone, int container)
{
   API_ENTRY return;
}

/* local subsystem functions */

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->scrollframe_obj, sd->x, sd->y);
   evas_object_resize(sd->scrollframe_obj, sd->w, sd->h);
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->scrollframe_obj = e_scrollframe_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->scrollframe_obj, obj);
   sd->table_obj = e_table_add(evas_object_evas_get(obj));
   e_table_homogenous_set(sd->table_obj, 1);
   e_scrollframe_child_set(sd->scrollframe_obj, sd->table_obj);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_del(sd->table_obj);
   evas_object_del(sd->scrollframe_obj);
   free(sd);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->scrollframe_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->scrollframe_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->scrollframe_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->scrollframe_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->scrollframe_obj);
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


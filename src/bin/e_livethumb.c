/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_livethumb"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object   *smart_obj;
   Evas_Object   *evas_obj;
   Evas_Object   *thumb_obj;
   Evas          *evas;
   Evas_Coord     vw, vh;
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
EAPI Evas_Object *
e_livethumb_add(Evas *e)
{
   _e_smart_init();
   return evas_object_smart_add(e, _e_smart);
}

EAPI Evas *
e_livethumb_evas_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->evas;
}

EAPI void
e_livethumb_vsize_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   API_ENTRY return;
   if ((w == sd->vw) && (h == sd->vh)) return;
   sd->vw = w;
   sd->vh = h;
   evas_object_image_size_set(sd->evas_obj, sd->vw, sd->vh);
   if (sd->thumb_obj) evas_object_resize(sd->thumb_obj, sd->vw, sd->vh);
}

EAPI void
e_livethumb_vsize_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   if (w) *w = sd->vw;
   if (h) *h = sd->vh;
}

EAPI void
e_livethumb_thumb_set(Evas_Object *obj, Evas_Object *thumb)
{
   API_ENTRY return;
   if (!thumb)
     {
	sd->thumb_obj = NULL;
	return;
     }
   sd->thumb_obj = thumb;
   evas_object_show(sd->thumb_obj);
   evas_object_move(sd->thumb_obj, 0, 0);
   evas_object_resize(sd->thumb_obj, sd->vw, sd->vh);
}

EAPI Evas_Object *
e_livethumb_thumb_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->thumb_obj;
}
   
/* local subsystem functions */

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->evas_obj, sd->x, sd->y);
   evas_object_resize(sd->evas_obj, sd->w, sd->h);
   evas_object_image_fill_set(sd->evas_obj, 0, 0, sd->w, sd->h);
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
   sd->vw = 1;
   sd->vh = 1;

   sd->evas_obj = ecore_evas_object_image_new(ecore_evas_ecore_evas_get(evas_object_evas_get(obj)));
   evas_object_smart_member_add(sd->evas_obj, obj);
   evas_object_image_size_set(sd->evas_obj, sd->vw, sd->vh);
   sd->evas = ecore_evas_get(evas_object_data_get(sd->evas_obj, "Ecore_Evas"));
   e_canvas_add(evas_object_data_get(sd->evas_obj, "Ecore_Evas"));
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   e_canvas_del(evas_object_data_get(sd->evas_obj, "Ecore_Evas"));
   evas_object_del(sd->evas_obj);
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
   evas_object_show(sd->evas_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->evas_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->evas_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->evas_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->evas_obj);
}  

/* never need to touch this */

static void
_e_smart_init(void)
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_smart_add,
	       _e_smart_del, 
	       _e_smart_move,
	       _e_smart_resize,
	       _e_smart_show,
	       _e_smart_hide,
	       _e_smart_color_set,
	       _e_smart_clip_set,
	       _e_smart_clip_unset,
	       NULL,
	       NULL,
	       NULL,
	       NULL
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}


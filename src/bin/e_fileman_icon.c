/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Smart_Data       E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h, iw, ih;

   Evas            *evas;
   Evas_Object     *obj;

   char            *saved_title;

   Evas_Object     *event_object;
   Evas_Object     *icon_object;
   Evas_Object     *image_object;
   Evas_Object     *entry_object;
   Evas_Object     *thumb_object;

   E_Fm_File       *file;

   unsigned char    visible : 1;

   int              type;

   E_Fm_Icon_Metadata *meta;
};

/* local subsystem functions */
static void _e_fm_icon_smart_add         (Evas_Object *obj);
static void _e_fm_icon_smart_del         (Evas_Object *obj);
static void _e_fm_icon_smart_move        (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_fm_icon_smart_resize      (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_fm_icon_smart_show        (Evas_Object *obj);
static void _e_fm_icon_smart_hide        (Evas_Object *obj);
static void _e_fm_icon_smart_clip_set    (Evas_Object *obj, Evas_Object *clip);
static void _e_fm_icon_smart_clip_unset  (Evas_Object *obj);

/* Create icons */
static void  _e_fm_icon_icon_mime_get(E_Smart_Data *sd);
static void  _e_fm_icon_thumb_generate_cb(Evas_Object *obj, void *data);
static void  _e_fm_icon_type_set(E_Smart_Data *sd);
static void  _e_fm_icon_meta_fill(E_Fm_Icon_Metadata *m, E_Smart_Data *sd);

/* local subsystem globals */
static Evas_Smart *e_smart = NULL;

static Evas_List  *thumb_files = NULL;

static Evas_List  *event_handlers = NULL;

/*
static Eet_Data_Descriptor *_e_fm_icon_meta_edd = NULL;
*/

/* externally accessible functions */
EAPI int
e_fm_icon_init(void)
{
   return 1;
}

EAPI int
e_fm_icon_shutdown(void)
{
   while (event_handlers)
     {
	ecore_event_handler_del(event_handlers->data);
	event_handlers = evas_list_remove_list(event_handlers, event_handlers);
     }
   evas_list_free(thumb_files);
   evas_smart_free(e_smart);
   return 1;
}

EAPI Evas_Object *
e_fm_icon_add(Evas *evas)
{
   Evas_Object *e_fm_icon_smart;

   if (!e_smart)
     {
	e_smart = evas_smart_new("e_fm_icon_smart",
	                         _e_fm_icon_smart_add,
	                         _e_fm_icon_smart_del,
				 NULL, NULL, NULL, NULL, NULL,
	                         _e_fm_icon_smart_move,
	                         _e_fm_icon_smart_resize,
	                         _e_fm_icon_smart_show,
	                         _e_fm_icon_smart_hide,
	                         NULL,
	                         _e_fm_icon_smart_clip_set,
	                         _e_fm_icon_smart_clip_unset,
	                         NULL);
     }

   e_fm_icon_smart = evas_object_smart_add(evas, e_smart);

   return e_fm_icon_smart;
}

EAPI void
e_fm_icon_type_set(Evas_Object *obj, int type)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   if(sd->type == type)
     return;

   sd->type = type;
   _e_fm_icon_type_set(sd);
}

EAPI E_Fm_File *
e_fm_icon_file_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   return sd->file;
}

EAPI void
e_fm_icon_file_set(Evas_Object *obj, E_Fm_File *file)
{
   E_Smart_Data *sd;
   Evas_Coord icon_w, icon_h;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (!sd->file) e_object_ref(E_OBJECT(file));
   sd->file = file;
   file->icon_object = obj;

   if (sd->file->mime->thumbnail)
     {
	/* FIXME: should use smart callback to change icon when thumb done */
	sd->thumb_object = e_thumb_icon_add(sd->evas);
	e_thumb_icon_file_set(sd->thumb_object, sd->file->path, NULL);
	e_thumb_icon_size_set(sd->thumb_object, 64, 64);
	e_thumb_icon_begin(sd->thumb_object);
	evas_object_show(sd->thumb_object);
	sd->image_object = edje_object_add(sd->evas);
	e_theme_edje_object_set(sd->image_object, "base/theme/fileman",
				"fileman/icon_thumb");
	edje_extern_object_min_size_set(sd->thumb_object, sd->iw, sd->ih);
	edje_extern_object_max_size_set(sd->thumb_object, sd->iw, sd->ih);
	edje_object_part_swallow(sd->image_object, "icon_swallow",
				 sd->thumb_object);
	edje_object_size_min_calc(sd->image_object, &icon_w, &icon_h);
	sd->iw = icon_w;
	sd->ih = icon_h;
	edje_extern_object_min_size_set(sd->image_object, icon_w, icon_h);
	edje_extern_object_max_size_set(sd->image_object, icon_w, icon_h);
	edje_object_part_swallow(sd->icon_object, "icon_swallow",
				 sd->image_object);
     }
   else
     {
	_e_fm_icon_icon_mime_get(sd);
     }
   _e_fm_icon_type_set(sd);
   edje_object_size_min_calc(sd->icon_object, &icon_w, &icon_h);
   evas_object_resize(sd->obj, 0, 0); // because it still thinks its the same size
   // 96 is an aritary width - should be user configurable
   evas_object_resize(sd->obj, 80, icon_h);
}

EAPI void
e_fm_icon_appear_cb(Evas_Object *obj, void *data)
{
   E_Smart_Data *sd;

   evas_object_show(obj);
   return;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->visible) return;
   sd->visible = 1;

   sd->icon_object = edje_object_add(sd->evas);
   evas_object_smart_member_add(sd->icon_object, obj);

   sd->event_object = evas_object_rectangle_add(sd->evas);
   evas_object_color_set(sd->event_object, 0, 0, 0, 0);
   evas_object_smart_member_add(sd->event_object, obj);
   evas_object_smart_data_set(obj, sd);

   e_fm_icon_file_set(obj, sd->file);
   evas_object_show(obj);
}

EAPI void
e_fm_icon_disappear_cb(Evas_Object *obj, void *data)
{
   E_Smart_Data *sd;

   evas_object_hide(obj);
   return;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(!sd->visible) return;
   sd->visible = 0;

   if (sd->event_object) evas_object_del(sd->event_object);
   if (sd->icon_object) evas_object_del(sd->icon_object);
   if (sd->image_object) evas_object_del(sd->image_object);
   if (sd->thumb_object) evas_object_del(sd->thumb_object);
   E_FREE(sd->saved_title);
}

EAPI char *
e_fm_icon_title_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   return edje_object_part_text_get(sd->icon_object, "icon_title");
}

EAPI void
e_fm_icon_title_set(Evas_Object *obj, const char *title)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   E_FREE(sd->saved_title);
   sd->saved_title = E_NEW(char, strlen(title) + 1);
   snprintf(sd->saved_title, strlen(title) + 1, "%s", title);
   //if (sd->icon_object) edje_object_part_text_set(sd->icon_object, "icon_title", title);
   if(sd->icon_object)
     {
#if 0
	Evas_Textblock_Style *e_editable_text_style;
	Evas_Coord fw, fh, il, ir, it, ib;

	e_editable_text_style = evas_textblock_style_new();
	evas_textblock_style_set(e_editable_text_style, "DEFAULT='font=Vera font_size=10 style=shadow shadow_color=#ffffff80 align=center color=#000000 wrap=char'");

	evas_object_textblock_style_set(sd->title_object, e_editable_text_style);
	evas_object_textblock_text_markup_set(sd->title_object, title);

	evas_object_resize(sd->title_object,  sd->w, 1);
	evas_object_textblock_size_formatted_get(sd->title_object, &fw, &fh);
	evas_object_textblock_style_insets_get(sd->title_object, &il, &ir, &it, &ib);

	sd->h = sd->ih + fh + it + ib;

	evas_object_resize(sd->title_object, sd->w, fh + it + ib);
	edje_extern_object_min_size_set(sd->title_object, sd->w, fh + it + ib);
	evas_object_resize(sd->icon_object, sd->w, sd->h);
        evas_object_resize(sd->obj, sd->w, sd->h);
	edje_object_part_swallow(sd->icon_object, "icon_title", sd->title_object);
#endif
	edje_object_part_text_set(sd->icon_object, "icon_title", title);
     }
}

EAPI void
e_fm_icon_edit_entry_set(Evas_Object *obj, Evas_Object *entry)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (entry)
     {
	sd->entry_object = entry;
	edje_object_part_swallow(sd->icon_object, "icon_title_edit_swallow", sd->entry_object);
     }
   else
     {
	edje_object_part_unswallow(sd->icon_object, sd->entry_object);
	sd->entry_object = NULL;
     }
}

EAPI void
e_fm_icon_signal_emit(Evas_Object *obj, const char *source, const char *emission)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->icon_object) edje_object_signal_emit(sd->icon_object, source, emission);
   if (sd->image_object) edje_object_signal_emit(sd->image_object, source, emission);
}

EAPI E_Fm_Icon_Metadata *
e_fm_icon_meta_generate(Evas_Object *obj)
{
   E_Smart_Data *sd;
   E_Fm_Icon_Metadata *m;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return NULL;

   m = calloc(1, sizeof(E_Fm_Icon_Metadata));
   if (!m) return NULL;
   _e_fm_icon_meta_fill(m, sd);

   sd->meta = m;

   return m;
}

EAPI void
e_fm_icon_meta_free(E_Fm_Icon_Metadata *m)
{
   if (!m) return;
   E_FREE(m->name);

   free(m);
}

EAPI int
e_fm_icon_assoc_set(Evas_Object *obj, const char *assoc)
{
   /* TODO
    * Store the associated exe in a cfg
    */
   return 0;
}

/* local subsystem functions */
static void
_e_fm_icon_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = E_NEW(E_Smart_Data, 1);
   if (!sd) return;

   sd->evas = evas_object_evas_get(obj);
   sd->obj = obj;
   sd->saved_title = NULL;
   sd->type = E_FM_ICON_NORMAL;
   sd->w = 64;
   sd->h = 64;
   sd->iw = 48;
   sd->ih = 48;
   sd->file = NULL;
   sd->meta = NULL;

   sd->icon_object = edje_object_add(sd->evas);
   evas_object_smart_member_add(sd->icon_object, obj);

   sd->event_object = evas_object_rectangle_add(sd->evas);
   evas_object_color_set(sd->event_object, 0, 0, 0, 0);
   evas_object_smart_member_add(sd->event_object, obj);
   evas_object_smart_data_set(obj, sd);

   sd->visible = 1;

}

static void
_e_fm_icon_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->event_object) evas_object_del(sd->event_object);
   if (sd->icon_object) evas_object_del(sd->icon_object);
   if (sd->image_object) evas_object_del(sd->image_object);
   if (sd->thumb_object) evas_object_del(sd->thumb_object);
   E_FREE(sd->saved_title);
   if (sd->file) e_object_unref(E_OBJECT(sd->file));
   free(sd);
}

static void
_e_fm_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   if(sd->event_object)
     evas_object_move(sd->event_object, x, y);
   if(sd->icon_object)
     evas_object_move(sd->icon_object, x, y);
   if(sd->meta)
     {
        //printf("update meta for %s: x=%d y=%d\n", sd->file->name, x, y);
	sd->meta->x = x;
	sd->meta->y = y;
     }
}

static void
_e_fm_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   if(sd->event_object)
     evas_object_resize(sd->event_object, sd->w, sd->h);
   if(sd->icon_object)
     evas_object_resize(sd->icon_object, sd->w, sd->h);
   if(sd->meta)
     {
	printf("update meta: w=%d h=%d\n", w, h);
	sd->meta->w = w;
	sd->meta->h = h;
     }
}

static void
_e_fm_icon_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->event_object)
     evas_object_show(sd->icon_object);
   if(sd->icon_object)
     evas_object_show(sd->event_object);
}

static void
_e_fm_icon_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if(sd->event_object)
     evas_object_hide(sd->icon_object);
   if(sd->icon_object)
     evas_object_hide(sd->event_object);
}

static void
_e_fm_icon_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->event_object, clip);
   evas_object_clip_set(sd->icon_object, clip);
}

static void
_e_fm_icon_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->event_object);
   evas_object_clip_unset(sd->icon_object);
}

static void
_e_fm_icon_icon_mime_get(E_Smart_Data *sd)
{
  char part[PATH_MAX];
	
  sd->image_object = edje_object_add(sd->evas);
  /* since a mime is set with every creation of an E_Fm_File its existence isnt checked here */
   	  
  snprintf(part, PATH_MAX, "icons/fileman/%s", (sd->file->mime->name)); 
  if (!e_theme_edje_object_set(sd->image_object, "base/theme/fileman", part))
    e_theme_edje_object_set(sd->image_object, "base/theme/fileman", "icons/fileman/file");

   evas_object_show(sd->image_object);
   edje_extern_object_min_size_set(sd->image_object, sd->iw, sd->ih);
   edje_extern_object_max_size_set(sd->image_object, sd->iw, sd->ih);
   edje_object_part_swallow(sd->icon_object, "icon_swallow",
			    sd->image_object);
}

static void
_e_fm_icon_type_set(E_Smart_Data *sd)
{
   switch (sd->type)
     {
      case E_FM_ICON_NORMAL:
	e_theme_edje_object_set(sd->icon_object, "base/theme/fileman",
				"fileman/icon_normal");
	break;

      case E_FM_ICON_LIST:
	e_theme_edje_object_set(sd->icon_object, "base/theme/fileman",
				"fileman/icon_list");
	break;

      default:
	e_theme_edje_object_set(sd->icon_object, "base/theme/fileman",
				"fileman/icon_normal");
	break;
     }

   if (sd->image_object)
     {
	edje_extern_object_min_size_set(sd->image_object, sd->iw, sd->ih);
	edje_extern_object_max_size_set(sd->image_object, sd->iw, sd->ih);
	edje_object_part_swallow(sd->icon_object, "icon_swallow",
				 sd->image_object);
	evas_object_show(sd->image_object);
     }

   if (sd->saved_title)
     {
#if 0
	Evas_Textblock_Style *e_editable_text_style;
	Evas_Coord fw, fh, il, ir, it, ib;

	e_editable_text_style = evas_textblock_style_new();
	evas_textblock_style_set(e_editable_text_style, "DEFAULT='font=Vera font_size=10 style=shadow shadow_color=#ffffff80  align=center color=#000000 wrap=char'");

	evas_object_textblock_style_set(sd->title_object, e_editable_text_style);
	evas_object_textblock_text_markup_set(sd->title_object, sd->saved_title);

	evas_object_resize(sd->title_object,  sd->w, 1);
	evas_object_textblock_size_formatted_get(sd->title_object, &fw, &fh);
	evas_object_textblock_style_insets_get(sd->title_object, &il, &ir, &it, &ib);

	sd->h = sd->ih + fh + it + ib;

	evas_object_resize(sd->title_object, sd->w, fh + it + ib);
	edje_extern_object_min_size_set(sd->title_object, sd->w, fh + it + ib);
	evas_object_resize(sd->icon_object, sd->w, sd->h);
        evas_object_resize(sd->obj, sd->w, sd->h);
	edje_object_part_swallow(sd->icon_object, "icon_title", sd->title_object);
#endif
	edje_object_part_text_set(sd->icon_object, "icon_title", sd->saved_title);
     }
   else
     {
#if 0
	Evas_Textblock_Style *e_editable_text_style;
	Evas_Coord fw, fh, il, ir, it, ib;

	e_editable_text_style = evas_textblock_style_new();
	evas_textblock_style_set(e_editable_text_style, "DEFAULT='font=Vera font_size=10 style=shadow shadow_color=#ffffff80 align=center color=#000000 wrap=char'");

	evas_object_textblock_style_set(sd->title_object, e_editable_text_style);
	evas_object_textblock_text_markup_set(sd->title_object, sd->file->name);

	evas_object_resize(sd->title_object,  sd->w, 1);
	evas_object_textblock_size_formatted_get(sd->title_object, &fw, &fh);
	evas_object_textblock_style_insets_get(sd->title_object, &il, &ir, &it, &ib);

	sd->h = sd->ih + fh + it + ib;

	evas_object_resize(sd->title_object, sd->w, fh + it + ib);
	edje_extern_object_min_size_set(sd->title_object, sd->w, fh + it + ib);
	evas_object_resize(sd->icon_object, sd->w, sd->h);
        evas_object_resize(sd->obj, sd->w, sd->h);
	edje_object_part_swallow(sd->icon_object, "icon_title", sd->title_object);
#endif
	edje_object_part_text_set(sd->icon_object, "icon_title", sd->file->name);
     }
}

static void
_e_fm_icon_meta_fill(E_Fm_Icon_Metadata *m, E_Smart_Data *sd)
{
   m->x = sd->x;
   m->y = sd->y;
   m->w = sd->w;
   m->h = sd->h;
   m->name = strdup(sd->file->name);
}

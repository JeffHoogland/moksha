#include <e.h>
#include "e_busycover.h"
#include "e_cfg.h"

// FIXME: make code work - place busycover in safe area
// FIXME: make theme for it

/* internal calls */

E_Busycover *_e_busycover_new(E_Zone *zone, const char *themedir);
static void _e_busycover_free(E_Busycover *esw);
static int _e_busycover_cb_zone_move_resize(void *data, int type, void *event);
static void _e_busycover_cb_item_sel(void *data, void *data2);

static Evas_Object *_theme_obj_new(Evas *e, const char *custom_dir, const char *group);

/* state */
static Eina_List *busycovers = NULL;

static void
_e_busycover_add_object(E_Busycover *esw)
{
   int x, y, w, h;
   Evas_Object *o;
   
   esw->base_obj = _theme_obj_new(esw->zone->container->bg_evas,
				  esw->themedir,
				  "e/modules/busycover/default");
   edje_object_part_text_set(esw->base_obj, "e.text.title", "LOADING");
   e_slipshelf_safe_app_region_get(esw->zone, &x, &y, &w, &h);
   evas_object_move(esw->base_obj, x, y);
   evas_object_resize(esw->base_obj, w, h);
   evas_object_layer_set(esw->base_obj, 100);
}

/* called from the module core */
EAPI int
e_busycover_init(void)
{
   return 1;
}

EAPI int
e_busycover_shutdown(void)
{
   return 1;
}

EAPI E_Busycover *
e_busycover_new(E_Zone *zone, const char *themedir)
{
   E_Busycover *esw;
     
   esw = E_OBJECT_ALLOC(E_Busycover, E_BUSYCOVER_TYPE, _e_busycover_free);
   if (!esw) return NULL;
   
   esw->zone = zone;
   if (themedir) esw->themedir = evas_stringshare_add(themedir);

   busycovers = eina_list_append(busycovers, esw);

   esw->handlers = eina_list_append
     (esw->handlers,
      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
			      _e_busycover_cb_zone_move_resize, esw));
   
   return esw;
}

EAPI E_Busycover_Handle *
e_busycover_push(E_Busycover *esw, const char *message, const char *icon)
{
   E_Busycover_Handle *h;
   
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK_RETURN(esw, E_BUSYCOVER_TYPE, NULL);
   if (!esw->base_obj) _e_busycover_add_object(esw);
   h = calloc(1, sizeof(E_Busycover_Handle));
   h->busycover = esw;
   if (message) h->message = evas_stringshare_add(message);
   if (icon) h->icon = evas_stringshare_add(icon);
   esw->handles = eina_list_prepend(esw->handles, h);
   edje_object_part_text_set(esw->base_obj, "e.text.label", h->message);
   /* FIXME: handle icon... */
   evas_object_show(esw->base_obj);
   return h;
}

EAPI void
e_busycover_pop(E_Busycover *esw, E_Busycover_Handle *handle)
{
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_BUSYCOVER_TYPE);
   if (!eina_list_data_find(esw->handles, handle)) return;
   esw->handles = eina_list_remove(esw->handles, handle);
   if (handle->message) evas_stringshare_del(handle->message);
   if (handle->icon) evas_stringshare_del(handle->icon);
   free(handle);
   if (esw->handles)
     {
	handle = esw->handles->data;
	edje_object_part_text_set(esw->base_obj, "e.text.label", 
				  handle->message);
     }
   else
     {
	evas_object_del(esw->base_obj);
	esw->base_obj = NULL;
     }
}


/* internal calls */
static void
_e_busycover_free(E_Busycover *esw)
{
   if (esw->base_obj) evas_object_del(esw->base_obj);
   busycovers = eina_list_remove(busycovers, esw);
   while (esw->handlers)
     {
	if (esw->handlers->data)
	  ecore_event_handler_del(esw->handlers->data);
	esw->handlers = eina_list_remove_list(esw->handlers, esw->handlers);
     }
   if (esw->themedir) evas_stringshare_del(esw->themedir);
   free(esw);
}

static int
_e_busycover_cb_zone_move_resize(void *data, int type, void *event)
{
   E_Event_Zone_Move_Resize *ev;
   E_Busycover *esw;
   
   ev = event;
   esw = data;
   if (esw->zone == ev->zone)
     {
	int x, y, w, h;
	
	e_slipshelf_safe_app_region_get(esw->zone, &x, &y, &w, &h);
	evas_object_move(esw->base_obj, x, y);
	evas_object_resize(esw->base_obj, w, h);
     }
   return 1;
}






static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;
   
   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];
	     
	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}


#include "e.h"
#include "e_busycover.h"

/* local function prototypes */
static void _e_busycover_add_object(E_Busycover *esw);
static void _e_busycover_cb_free(E_Busycover *esw);
static int _e_busycover_zone_cb_move_resize(void *data, int type, void *event);
static Evas_Object *_theme_obj_new(Evas *evas, const char *custom_dir, const char *group);

/* local variables */
static Eina_List *busycovers = NULL;

/* public functions */
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

   esw = E_OBJECT_ALLOC(E_Busycover, E_BUSYCOVER_TYPE, _e_busycover_cb_free);
   if (!esw) return NULL;

   esw->zone = zone;
   if (themedir) esw->themedir = eina_stringshare_add(themedir);

   busycovers = eina_list_append(busycovers, esw);

   esw->handlers = 
     eina_list_append(esw->handlers, 
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                                              _e_busycover_zone_cb_move_resize, 
                                              esw));
   return esw;
}

EAPI E_Busycover_Handle *
e_busycover_push(E_Busycover *esw, const char *msg, const char *icon) 
{
   E_Busycover_Handle *h;

   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK_RETURN(esw, E_BUSYCOVER_TYPE, NULL);
   if (!esw->o_base) _e_busycover_add_object(esw);
   h = E_NEW(E_Busycover_Handle, 1);
   h->busycover = esw;
   if (msg) h->msg = eina_stringshare_add(msg);
   if (icon) h->icon = eina_stringshare_add(icon);
   esw->handles = eina_list_prepend(esw->handles, h);
   edje_object_part_text_set(esw->o_base, "e.text.label", h->msg);
   /* FIXME: handle icon */
   evas_object_show(esw->o_base);
   return h;
}

EAPI void 
e_busycover_pop(E_Busycover *esw, E_Busycover_Handle *handle) 
{
   E_OBJECT_CHECK(esw);
   E_OBJECT_TYPE_CHECK(esw, E_BUSYCOVER_TYPE);
   if (!eina_list_data_find(esw->handles, handle)) return;
   esw->handles = eina_list_remove(esw->handles, handle);
   if (handle->msg) eina_stringshare_del(handle->msg);
   if (handle->icon) eina_stringshare_del(handle->icon);
   free(handle);
   if (esw->handles) 
     {
        handle = esw->handles->data;
        edje_object_part_text_set(esw->o_base, "e.text.label", handle->msg);
     }
   else 
     {
        evas_object_del(esw->o_base);
        esw->o_base = NULL;
     }
}

/* local functions */
static void 
_e_busycover_add_object(E_Busycover *esw) 
{
   Evas_Object *o;
   int x, y, w, h;

   esw->o_base = _theme_obj_new(esw->zone->container->bg_evas, esw->themedir, 
                                "modules/illume-home/busycover/default");
   edje_object_part_text_set(esw->o_base, "e.text.title", "LOADING");
   e_zone_useful_geometry_get(esw->zone, &x, &y, &w, &h);
   evas_object_move(esw->o_base, x, y);
   evas_object_resize(esw->o_base, w, h);
   evas_object_layer_set(esw->o_base, 100);
}

static void 
_e_busycover_cb_free(E_Busycover *esw) 
{
   Ecore_Event_Handler *handle;

   if (esw->o_base) evas_object_del(esw->o_base);
   busycovers = eina_list_remove(busycovers, esw);
   EINA_LIST_FREE(esw->handlers, handle)
     ecore_event_handler_del(handle);
   if (esw->themedir) eina_stringshare_del(esw->themedir);
   E_FREE(esw);
}

static int 
_e_busycover_zone_cb_move_resize(void *data, int type, void *event) 
{
   E_Event_Zone_Move_Resize *ev;
   E_Busycover *esw;

   ev = event;
   esw = data;
   if (esw->zone == ev->zone) 
     {
        int x, y, w, h;

        e_zone_useful_geometry_get(esw->zone, &x, &y, &w, &h);
        evas_object_move(esw->o_base, x, y);
        evas_object_resize(esw->o_base, w, h);
     }
   return 1;
}

static Evas_Object *
_theme_obj_new(Evas *evas, const char *custom_dir, const char *group) 
{
   Evas_Object *o;

   o = edje_object_add(evas);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume-home", group)) 
     {
        if (custom_dir) 
          {
             char buff[PATH_MAX];

             snprintf(buff, sizeof(buff), "%s/e-module-illume-home.edj", custom_dir);
             if (edje_object_file_set(o, buff, group)) 
               printf("OK FALLBACK %s\n", buff);
          }
     }
   return o;
}


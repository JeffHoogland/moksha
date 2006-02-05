/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_shelf_free(E_Shelf *es);

static Evas_List *shelves = NULL;
static int shelf_id = 0;

/* externally accessible functions */
EAPI int
e_shelf_init(void)
{
   return 1;
}

EAPI int
e_shelf_shutdown(void)
{
   return 1;
}

EAPI E_Shelf *
e_shelf_zone_new(E_Zone *zone, char *name)
{
   E_Shelf *es;
   char buf[1024];
   
   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;

   /* FIXME: geometry, layer and style shoudl be loaded from config for this
    named shelf */
   es->popup = e_popup_new(zone, 0, 0, zone->w, 32);
   e_popup_layer_set(es->popup, 255);
   es->style = strdup("default");
   
   es->ee = es->popup->ecore_evas;
   es->evas = es->popup->evas;
   es->o_base = edje_object_add(es->evas);
   es->name = strdup(name);
   snprintf(buf, sizeof(buf), "shelf/%s/base", es->style);
   evas_object_resize(es->o_base, es->popup->w, es->popup->h);
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf", "shelf/default/base");
   evas_object_show(es->o_base);
   e_popup_edje_bg_object_set(es->popup, es->o_base);
   e_popup_show(es->popup);
   
   snprintf(buf, sizeof(buf), "%i", shelf_id);
   shelf_id++;
   es->gadcon = e_gadcon_swallowed_new(es->name, buf, es->o_base, "items");
   e_gadcon_orient(es->gadcon, E_GADCON_ORIENT_HORIZ);
   
   shelves = evas_list_append(shelves, es);
   
   return es;
}

EAPI void
e_shelf_populate(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   e_gadcon_populate(es->gadcon);
}

/* local subsystem functions */
static void
_e_shelf_free(E_Shelf *es)
{
   shelves = evas_list_remove(shelves, es);
   e_object_del(E_OBJECT(es->gadcon));
   E_FREE(es->name);
   E_FREE(es->style);
   evas_object_del(es->o_base);
   e_object_del(E_OBJECT(es->popup));
   free(es);
}

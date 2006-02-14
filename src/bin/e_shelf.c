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
e_shelf_zone_new(E_Zone *zone, char *name, int popup, int layer)
{
   E_Shelf *es;
   char buf[1024];
   
   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;

   /* FIXME: geometry, layer and style shoudl be loaded from config for this
    named shelf */
   es->x = 0;
   es->y = 0;
   es->w = zone->w;
   es->h = 32;
   if (popup)
     {
	es->popup = e_popup_new(zone, es->x, es->y, es->w, es->h);
	e_popup_layer_set(es->popup, layer);
	es->ee = es->popup->ecore_evas;
	es->evas = es->popup->evas;
     }
   else
     {
	es->ee = zone->container->bg_ecore_evas;
	es->evas = zone->container->bg_evas;
     }
   es->layer = layer;
   es->zone = zone;
   es->style = strdup("default");
   
   es->o_base = edje_object_add(es->evas);
   es->name = strdup(name);
   snprintf(buf, sizeof(buf), "shelf/%s/base", es->style);
   evas_object_resize(es->o_base, es->w, es->h);
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf", "shelf/default/base");
   if (es->popup)
     {
	e_popup_edje_bg_object_set(es->popup, es->o_base);
	e_popup_show(es->popup);
     }
   else
     {
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_layer_set(es->o_base, layer);
     }
   evas_object_show(es->o_base);
   
   snprintf(buf, sizeof(buf), "%i", shelf_id);
   shelf_id++;
   es->gadcon = e_gadcon_swallowed_new(es->name, buf, es->o_base, "items");
   e_gadcon_orient(es->gadcon, E_GADCON_ORIENT_TOP);
   e_gadcon_zone_set(es->gadcon, zone);
   e_gadcon_ecore_evas_set(es->gadcon, es->ee);
   
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

EAPI void
e_shelf_move(E_Shelf *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->x = x;
   es->y = y;
   if (es->popup)
     e_popup_move(es->popup, es->x, es->y);
   else
     evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
}

EAPI void
e_shelf_resize(E_Shelf *es, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->w = w;
   es->h = h;
   if (es->popup)
     e_popup_resize(es->popup, es->w, es->h);
   else
     evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->x = x;
   es->y = y;
   es->w = w;
   es->h = h;
   if (es->popup)
     e_popup_move_resize(es->popup, es->x, es->y, es->w, es->h);
   else
     {
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_resize(es->o_base, es->w, es->h);
     }
}

EAPI void
e_shelf_layer_set(E_Shelf *es, int layer)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->layer = layer;
   if (es->popup)
     e_popup_layer_set(es->popup, es->layer);
   else
     evas_object_layer_set(es->o_base, es->layer);
}

EAPI void
e_shelf_save(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   /* FIXME: find or create saved shelf node and then modify and queue a
    * save
    */
}

EAPI void
e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   e_gadcon_orient(es->gadcon, orient);
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
   if (es->popup) e_object_del(E_OBJECT(es->popup));
   free(es);
}

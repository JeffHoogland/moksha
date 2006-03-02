/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_shelf_free(E_Shelf *es);
static void _e_shelf_config_port(E_Config_Shelf_Config *cf1, E_Config_Shelf_Config *cf2);

static Evas_List *shelves = NULL;
static int shelf_id = 0;

/* FIXME: shelves need to do this:
 * 1. allow them to be moved, resized etc. etc.
 * 2. have a configuration panel per shelf to select if its inline, layer etc.
 * 3. catch all right clicks not on modules for right click context menu
 * 4. a global config dialog that lists shelves u have to configure them
 */

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

EAPI void
e_shelf_config_init(void)
{
   Evas_List *l, *l2;
   
   for (l = e_config->shelves; l; l = l->next)
     {
	E_Config_Shelf *cf_es;
	E_Config_Shelf_Config *cf_escf, *cf_escf2;
	E_Zone *zone;
	int closeness;
	
	cf_es = l->data;
	zone = e_util_container_zone_number_get(cf_es->container, cf_es->zone);
	if (zone)
	  {
	     cf_escf2 = NULL;
	     closeness = 0x7fffffff;
	     for (l2 = cf_es->configs; l2; l2 = l2->next)
	       {
		  cf_escf = l2->data;
		  if ((cf_escf->res.w == zone->w) &&
		      (cf_escf->res.h == zone->h))
		    {
		       cf_escf2 = cf_escf;
		       closeness = 0;
		       break;
		    }
		  else
		    {
		       int difx, dify;
		       
		       difx = cf_escf->res.w - zone->w;
		       if (difx < 0) difx = -difx;
		       dify = cf_escf->res.h - zone->h;
		       if (dify < 0) dify = -dify;
		       difx = difx * dify;
		       if (difx < closeness)
			 {
			    closeness = difx;
			    cf_escf2 = cf_escf;
			 }
		    }
	       }
	     if ((closeness != 0) && (cf_escf2))
	       {
		  cf_escf = E_NEW(E_Config_Shelf_Config, 1);
		  cf_escf->res.w = zone->w;
		  cf_escf->res.h = zone->h;
		  cf_escf->orient = cf_escf2->orient;
		  cf_escf->style = evas_stringshare_add(cf_escf2->style);
		  _e_shelf_config_port(cf_escf2, cf_escf);
		  cf_escf2 = cf_escf;
		  e_config_save_queue();
	       }
	     if (cf_escf2)
	       {
		  E_Shelf *es;
		  
		  es = e_shelf_zone_new(zone, cf_es->name, cf_escf2->style,
					cf_es->popup, cf_es->layer);
		  if (es)
		    {
		       es->cfg = cf_es;
		       e_shelf_move_resize(es, cf_escf2->x, cf_escf2->y,
					   cf_escf2->w, cf_escf2->h);
		       e_shelf_orient(es, cf_escf2->orient);
		       e_shelf_populate(es);
		       e_shelf_show(es);
		    }
	       }
	  }
     }
}

EAPI E_Shelf *
e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, int layer)
{
   E_Shelf *es;
   char buf[1024];
   
   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;

   es->x = 0;
   es->y = 0;
   es->w = 32;
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
   es->style = evas_stringshare_add(style);
   
   es->o_base = edje_object_add(es->evas);
   es->name = evas_stringshare_add(name);
   snprintf(buf, sizeof(buf), "shelf/%s/base", es->style);
   evas_object_resize(es->o_base, es->w, es->h);
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf",
			     "shelf/default/base");
   if (es->popup)
     {
	evas_object_show(es->o_base);
	e_popup_edje_bg_object_set(es->popup, es->o_base);
     }
   else
     {
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_layer_set(es->o_base, layer);
     }
   
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
e_shelf_show(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->popup)
     e_popup_show(es->popup);
   else
     evas_object_show(es->o_base);
}

EAPI void
e_shelf_hide(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->popup)
     e_popup_hide(es->popup);
   else
     evas_object_hide(es->o_base);
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
     {
	e_popup_resize(es->popup, es->w, es->h);
	evas_object_resize(es->o_base, es->w, es->h);
     }
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
     {
	e_popup_move_resize(es->popup, es->x, es->y, es->w, es->h);
	evas_object_resize(es->o_base, es->w, es->h);
     }
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
   if (es->cfg)
     {
	Evas_List *l;
	E_Config_Shelf_Config *cf_escf = NULL;
	
	for (l = es->cfg->configs; l; l = l->next)
	  {
	     cf_escf = l->data;
	     if ((cf_escf->res.w == es->zone->w) &&
		 (cf_escf->res.h == es->zone->h))
	       break;
	     cf_escf = NULL;
	  }
	if (!cf_escf)
	  {
             cf_escf = E_NEW(E_Config_Shelf_Config, 1);
	     if (cf_escf)
	       {
		  cf_escf->res.w = es->zone->w;
		  cf_escf->res.h = es->zone->h;
		  es->cfg->configs = evas_list_append(es->cfg->configs, cf_escf);
	       }
	  }
	if (cf_escf)
	  {
	     cf_escf->x = es->x;
	     cf_escf->y = es->y;
	     cf_escf->w = es->w;
	     cf_escf->h = es->h;
	     cf_escf->orient = es->gadcon->orient;
	     if (cf_escf->style) evas_stringshare_del(cf_escf->style);
	     cf_escf->style = evas_stringshare_add(es->style);
	  }
     }
   else
     {
	E_Config_Shelf *cf_es;
	E_Config_Shelf_Config *cf_escf = NULL;
	
	cf_es = E_NEW(E_Config_Shelf, 1);
	cf_es->name = evas_stringshare_add(es->name);
	cf_es->container = es->zone->container->num;
	cf_es->zone = es->zone->num;
	if (es->popup) cf_es->popup = 1;
	cf_es->layer = es->layer;
	e_config->shelves = evas_list_append(e_config->shelves, cf_es);
	es->cfg = cf_es;
	
	cf_escf = E_NEW(E_Config_Shelf_Config, 1);
	cf_escf->res.w = es->zone->w;
	cf_escf->res.h = es->zone->h;
	cf_escf->x = es->x;
	cf_escf->y = es->y;
	cf_escf->w = es->w;
	cf_escf->h = es->h;
	cf_escf->orient = es->gadcon->orient;
	cf_escf->style = evas_stringshare_add(es->style);
	cf_es->configs = evas_list_append(cf_es->configs, cf_escf);
     }
   e_config_save_queue();
}

EAPI void
e_shelf_unsave(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->cfg)
     {
	e_config->shelves = evas_list_remove(e_config->shelves, es->cfg);
	evas_stringshare_del(es->cfg->name);
	while (es->cfg->configs)
	  {
	     E_Config_Shelf_Config *cf_escf;
	     
	     cf_escf = es->cfg->configs->data;
	     if (cf_escf->style) evas_stringshare_del(cf_escf->style);
	     free(cf_escf);
	     es->cfg->configs = evas_list_remove_list(es->cfg->configs, es->cfg->configs);
	  }
	free(es->cfg);
     }
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
   evas_stringshare_del(es->name);
   evas_stringshare_del(es->style);
   evas_object_del(es->o_base);
   if (es->popup) e_object_del(E_OBJECT(es->popup));
   free(es);
}

static void
_e_shelf_config_port(E_Config_Shelf_Config *cf1, E_Config_Shelf_Config *cf2)
{
   int px[4], py[4];
   int i;
   
   /* 
    * We have 4 corners. figure out what gravity zone (3x3 grid) they are in
    * then lock them in relative to the edge or middle of that zone and re
    * calculate it all then.
    */
   px[0] = cf1->x;
   px[1] = cf1->x + cf1->w - 1;
   px[2] = cf1->x;
   px[3] = cf1->x + cf1->w - 1;
   py[0] = cf1->y;
   py[1] = cf1->y;
   py[2] = cf1->y + cf1->h - 1;
   py[3] = cf1->y + cf1->h - 1;
   for (i = 0; i < 4; i++)
     {
	if (px[i] < (cf1->res.w / 3))
	  px[i] = px[i];
	else if (px[i] > ((2 * cf1->res.w) / 3))
	  px[i] = (cf2->res.w) + (px[i] - cf1->res.w);
	else
	  px[i] = (cf2->res.w / 2) + (px[i] - (cf1->res.w / 2));
	if (py[i] < (cf1->res.h / 3))
	  py[i] = py[i];
	else if (py[i] > ((2 * cf1->res.h) / 3))
	  py[i] = (cf2->res.h) + (py[i] - cf1->res.h);
	else
	  py[i] = (cf2->res.h / 2) + (py[i] - (cf1->res.h / 2));
     }
   cf2->x = px[0];
   cf2->y = py[0];
   cf2->w = px[3] - px[0] + 1;
   cf2->h = py[3] = py[0] + 1;
}

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Shelf E_Shelf;

#else
#ifndef E_SHELF_H
#define E_SHELF_H

#define E_SHELF_TYPE 0xE0b01024

struct _E_Shelf
{
   E_Object             e_obj_inherit;
   int                  x, y, w, h;
   int                  layer;
   E_Popup             *popup; /* NULL if its within an existing canvas */
   E_Zone              *zone; 
   Evas_Object         *o_base;
   Evas_Object         *o_event;
   Ecore_Evas          *ee;
   Evas                *evas;
   E_Gadcon            *gadcon;
   const char          *name;
   const char          *style;
   E_Config_Shelf      *cfg;
   unsigned char        fit_along : 1;
   unsigned char        fit_size : 1;
   int                  size;
};

EAPI int              e_shelf_init(void);
EAPI int              e_shelf_shutdown(void);
EAPI void             e_shelf_config_init(void);
EAPI Evas_List       *e_shelf_list(void);
EAPI E_Shelf         *e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, int layer);
EAPI void             e_shelf_populate(E_Shelf *es);
EAPI void             e_shelf_show(E_Shelf *es);
EAPI void             e_shelf_hide(E_Shelf *es);
EAPI void             e_shelf_move(E_Shelf *es, int x, int y);
EAPI void             e_shelf_resize(E_Shelf *es, int w, int h);
EAPI void             e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h);
EAPI void             e_shelf_layer_set(E_Shelf *es, int layer);
EAPI void             e_shelf_save(E_Shelf *es);
EAPI void             e_shelf_unsave(E_Shelf *es);
EAPI void             e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient);
    
#endif
#endif

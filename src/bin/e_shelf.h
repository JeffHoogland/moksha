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
   E_Popup             *popup; /* NULL if its within an existing canvas */
   Evas_Object         *o_base;
   Ecore_Evas          *ee;
   Evas                *evas;
   E_Gadcon            *gadcon;
   char                *name;
   char                *style;
   /* FIXME: we need a more powerful sizing/placement policy rather than just
    * x,y, width & height
    */
};

EAPI int              e_shelf_init(void);
EAPI int              e_shelf_shutdown(void);
EAPI E_Shelf         *e_shelf_zone_new(E_Zone *zone, char *name);
EAPI E_Shelf         *e_shelf_inline_new(Ecore_Evas *ee, char *name);
EAPI void             e_shelf_populate(E_Shelf *es);
    
#endif
#endif

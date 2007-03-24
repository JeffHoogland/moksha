/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Order        E_Order;

#else
#ifndef E_ORDER_H
#define E_ORDER_H

#define E_ORDER_TYPE 0xE0b01020

struct _E_Order
{
   E_Object            e_obj_inherit;
   
   const char         *path;
   Evas_List          *desktops; /* A list of Efreet_Desktop files this .order contains */
   Ecore_File_Monitor *monitor; /* Check for changes int the .order file */
};

EAPI E_Order *e_order_new(const char *path);

#endif
#endif

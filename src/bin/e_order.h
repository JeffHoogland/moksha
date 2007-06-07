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

   struct {
	void (*update)(void *data, E_Order *eo);
	void *data;
   } cb;
};

EAPI int e_order_init(void);
EAPI int e_order_shutdown(void);

EAPI E_Order *e_order_new(const char *path);
EAPI void     e_order_update_callback_set(E_Order *eo, void (*cb)(void *data, E_Order *eo), void *data);

EAPI void e_order_remove(E_Order *eo, Efreet_Desktop *desktop);
EAPI void e_order_append(E_Order *eo, Efreet_Desktop *desktop);
EAPI void e_order_prepend_relative(E_Order *eo, Efreet_Desktop *desktop, Efreet_Desktop *before);
EAPI void e_order_files_append(E_Order *eo, Evas_List *files);
EAPI void e_order_files_prepend_relative(E_Order *eo, Evas_List *files, Efreet_Desktop *before);
EAPI void e_order_clear(E_Order *eo);

#endif
#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_INT_BORDER_MENU_H
#define E_INT_BORDER_MENU_H

EAPI void e_int_border_menu_create(E_Border *bd);
EAPI void e_int_border_menu_show(E_Border *bd, Evas_Coord x, Evas_Coord y, int key, Ecore_X_Time timestamp);
EAPI void e_int_border_menu_del(E_Border *bd);

#endif
#endif

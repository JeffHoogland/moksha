/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_MAXIMIZE_H
#define E_MAXIMIZE_H

EAPI void e_maximize_border_shelf_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir);
EAPI void e_maximize_border_dock_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2);
EAPI void e_maximize_border_shelf_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir);
EAPI void e_maximize_border_border_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir);

#endif
#endif

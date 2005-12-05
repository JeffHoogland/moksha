/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_ICONSEL_H
#define E_WIDGET_ICONSEL_H

EAPI Evas_Object *e_widget_iconsel_add(Evas *evas, Evas_Object *icon, Evas_Coord minw, Evas_Coord minh, char **file);
EAPI Evas_Object *e_widget_iconsel_add_from_file(Evas *evas, char *icon, Evas_Coord minw, Evas_Coord minh, char **file);
EAPI void         e_widget_iconsel_select_callback_add(Evas_Object *obj, void (*func)(Evas_Object *obj, char *file, void *data), void *data);
EAPI void         e_widget_iconsel_hilite_callback_add(Evas_Object *obj, void (*func)(Evas_Object *obj, char *file, void *data), void *data);

#endif
#endif

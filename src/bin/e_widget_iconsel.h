/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_BUTTON_H
#define E_WIDGET_BUTTON_H

EAPI Evas_Object *e_widget_iconsel_add(Evas *evas, Evas_Object *icon, Evas_Coord minw, Evas_Coord minh, void (*func) (void *data, void *data2), void *data, void *data2);
EAPI Evas_Object *e_widget_iconsel_add_from_file(Evas *evas, char *icon, Evas_Coord minw, Evas_Coord minh, void (*func) (void *data, void *data2), void *data, void *data2);
EAPI void         e_widget_iconsel_select_callback_add(Evas_Object *obj, void (*func)(Evas_Object *obj, char *file, void *data), void *data);

#endif
#endif

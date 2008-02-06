/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_SLIDER_H
#define E_WIDGET_SLIDER_H

EAPI Evas_Object *e_widget_slider_add(Evas *evas, int horiz, int rev, const char *fmt, double min, double max, double step, int count, double *dval, int *ival, Evas_Coord size);
EAPI int e_widget_slider_value_double_set(Evas_Object *slider, double dval);
EAPI int e_widget_slider_value_int_set(Evas_Object *slider, int ival);
EAPI int e_widget_slider_value_double_get(Evas_Object *slider, double *dval);
EAPI int e_widget_slider_value_int_get(Evas_Object *slider, int *ival);

#endif
#endif

#ifndef E_WIDGET_SPECTRUM_H
#define E_WIDGET_SPECTRUM_H
Evas_Object *e_widget_spectrum_add(Evas *evas, E_Color_Component mode, E_Color *cv);
void e_widget_spectrum_mode_set(Evas_Object *obj, E_Color_Component mode);
void e_widget_spectrum_update(Evas_Object *obj, int redraw);
#endif

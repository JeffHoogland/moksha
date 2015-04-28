#ifndef E_SPECTRUM_H
#define E_SPECTRUM_H

Evas_Object *e_spectrum_add(Evas *e);
void e_spectrum_color_value_set(Evas_Object *o, E_Color *cv);
void e_spectrum_mode_set(Evas_Object *o, E_Color_Component mode);
void e_spectrum_update(Evas_Object *o);

#endif

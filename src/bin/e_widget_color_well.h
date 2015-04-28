#ifndef E_WIDGET_COLOR_WELL_H
#define E_WIDGET_COLOR_WELL_H

EAPI Evas_Object *e_widget_color_well_add              (Evas *evas, E_Color *color, Eina_Bool show_color_dialog);
EAPI Evas_Object *e_widget_color_well_add_full         (Evas *evas, E_Color *color, Eina_Bool show_color_dialog, Eina_Bool alpha_enabled);
EAPI void         e_widget_color_well_update           (Evas_Object *obj);

#endif

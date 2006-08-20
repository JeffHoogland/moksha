/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Color_Dialog E_Color_Dialog;

#else
#ifndef E_COLOR_DIALOG_H
#define E_COLOR_DIALOG_H

#define E_COLOR_DIALOG_TYPE 0xE0b01026

struct _E_Color_Dialog
{
   E_Object             e_obj_inherit;
   
   E_Container         *con;
   E_Dialog            *dia;
   
   E_Color             *color;
   
   void (*select_func)(E_Color_Dialog *dia, E_Color *color, void *data);
   void  *select_data;
   void (*cancel_func)(E_Color_Dialog *dia, E_Color *color, void *data);
   void  *cancel_data;
};

EAPI E_Color_Dialog  *e_color_dialog_new                (E_Container *con, const E_Color *initial_color);
EAPI void             e_color_dialog_show               (E_Color_Dialog *dia);
EAPI void             e_color_dialog_title_set          (E_Color_Dialog *dia, const char *title);
EAPI void             e_color_dialog_select_callback_add(E_Color_Dialog *dia, void (*func)(E_Color_Dialog *dia, E_Color *color, void *data), void *data);
EAPI void             e_color_dialog_cancel_callback_add(E_Color_Dialog *dia, void (*func)(E_Color_Dialog *dia, E_Color *color, void *data), void *data);

#endif
#endif

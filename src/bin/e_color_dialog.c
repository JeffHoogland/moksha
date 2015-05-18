#include "e.h"

static void _e_color_dialog_button1_click(void *data, E_Dialog *edia);
static void _e_color_dialog_button2_click(void *data, E_Dialog *edia);
static void _e_color_dialog_free(E_Color_Dialog *dia);
static void _e_color_dialog_dia_del(void *obj);
static void _e_color_dialog_cb_csel_change(void *data, Evas_Object *obj, void *ev);

/**
 * Create a color selector dialog.
 *
 * @param con container to display on
 * @param color color to initialize to (or NULL for black).
 * @param alpha_enabled if set, uses alpha and let user edit it.
 */
E_Color_Dialog  *
e_color_dialog_new(E_Container *con, const E_Color *color, Eina_Bool alpha_enabled)
{
   E_Color_Dialog *dia;
   Evas_Object *o, *bx, *re, *fr;

   dia = E_OBJECT_ALLOC(E_Color_Dialog, E_COLOR_DIALOG_TYPE, _e_color_dialog_free);
   if (!dia) return NULL;
   dia->dia = e_dialog_new(con, "E", "_color_dialog");
   e_dialog_title_set(dia->dia, _("Color Selector"));

   dia->color = calloc(1, sizeof(E_Color));
   dia->initial = calloc(1, sizeof(E_Color));

   if (color)
     e_color_copy(color, dia->color);

   if ((!color) || (!alpha_enabled))
     dia->color->a = 255;

   e_color_copy(dia->color, dia->initial);

   bx = elm_box_add(dia->dia->win);
   E_EXPAND(bx);
   E_FILL(bx);

   o = elm_colorselector_add(bx);
   elm_colorselector_mode_set(o, ELM_COLORSELECTOR_COMPONENTS);
   elm_colorselector_color_set(o, dia->color->r, dia->color->g, dia->color->b, dia->color->a);
   E_EXPAND(o);
   E_FILL(o); 
   evas_object_show(o);
   elm_box_pack_end(bx, o);

   fr = elm_frame_add(bx);
   E_WEIGHT(fr, EVAS_HINT_EXPAND, 0);
   E_FILL(fr);
   elm_object_text_set(fr, _("Color Preview"));
   evas_object_show(fr);
   elm_box_pack_end(bx, fr);

   re = evas_object_rectangle_add(evas_object_evas_get(dia->dia->win));
   evas_object_data_set(o, "rect", re);
   evas_object_size_hint_min_set(re, 1, ELM_SCALE_SIZE(100));
   evas_object_show(re);
   elm_object_content_set(fr, re);

   evas_object_smart_callback_add(o, "changed", _e_color_dialog_cb_csel_change, dia);
   e_dialog_content_set(dia->dia, bx, 250, -1);

   /* buttons at the bottom */
   e_dialog_button_add(dia->dia, _("Select"), NULL, _e_color_dialog_button1_click, dia);
   e_dialog_button_add(dia->dia, _("Cancel"), NULL, _e_color_dialog_button2_click, dia);
   e_win_centered_set(dia->dia->win, 1);

   dia->dia->data = dia;
   e_object_del_attach_func_set(E_OBJECT(dia->dia), _e_color_dialog_dia_del);

   return dia;
}

void
e_color_dialog_show(E_Color_Dialog *dia)
{
   e_dialog_show(dia->dia);
   e_dialog_border_icon_set(dia->dia, "enlightenment/colors");
   evas_object_size_hint_min_set(dia->dia->win, 250, -1);
}

void
e_color_dialog_title_set(E_Color_Dialog *dia, const char *title)
{
   e_dialog_title_set(dia->dia, title);
}

void
e_color_dialog_select_callback_set(E_Color_Dialog *dia, void (*func)(E_Color_Dialog *dia, E_Color *color, void *data), void *data)
{
   dia->select_func = func;
   dia->select_data = data;
}

void
e_color_dialog_cancel_callback_set(E_Color_Dialog *dia, void (*func)(E_Color_Dialog *dia, E_Color *color, void *data), void *data)
{
   dia->cancel_func = func;
   dia->cancel_data = data;
}

EAPI void
e_color_dialog_change_callback_set(E_Color_Dialog *dia, void (*func)(E_Color_Dialog *dia, E_Color *color, void *data), void *data)
{
   dia->change_func = func;
   dia->change_data = data;
}

static void
_e_color_dialog_cb_csel_change(void *data, Evas_Object *obj, void *ev EINA_UNUSED)
{
   E_Color_Dialog *dia = data;
   Evas_Object *re;
   int r, g, b;

   elm_colorselector_color_get(obj, &dia->color->r, &dia->color->g, &dia->color->b, &dia->color->a);
   r = dia->color->r;
   g = dia->color->g;
   b = dia->color->b;
   re = evas_object_data_get(obj, "rect");
   evas_color_argb_premul(dia->color->a, &r, &g, &b);
   evas_object_color_set(re, r, g, b, dia->color->a);
   if (dia->change_func && dia->color)
     dia->change_func(dia, dia->color, dia->change_data);
}

static void
_e_color_dialog_button1_click(void *data, E_Dialog *edia __UNUSED__)
{
   E_Color_Dialog *dia;

   dia = data;
   if (dia->select_func)
     dia->select_func(dia, dia->color, dia->select_data);
   _e_color_dialog_free(dia);
}

static void
_e_color_dialog_button2_click(void *data, E_Dialog *edia __UNUSED__)
{
   E_Color_Dialog *dia;

   dia = data;
   if (dia->cancel_func)
     dia->cancel_func(dia, dia->initial, dia->cancel_data);
   _e_color_dialog_free(data);
}

static void
_e_color_dialog_free(E_Color_Dialog *dia)
{
   if (dia->dia)
     {
	e_object_del_attach_func_set(E_OBJECT(dia->dia), NULL);
	e_object_del(E_OBJECT(dia->dia));
	dia->dia = NULL;
     }
   E_FREE(dia->color);
   E_FREE(dia);
}

static void
_e_color_dialog_dia_del(void *obj)
{
   E_Dialog *dia = obj;
   E_Color_Dialog *cdia = dia->data;
   if (cdia->cancel_func && cdia->initial)
     cdia->cancel_func(cdia, cdia->initial, cdia->cancel_data);

   cdia->dia = NULL;
   e_object_del(E_OBJECT(cdia));
}

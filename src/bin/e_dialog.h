/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Dialog E_Dialog;
typedef struct _E_Dialog_Button E_Dialog_Button;

#else
#ifndef E_DIALOG_H
#define E_DIALOG_H

#define E_DIALOG_TYPE 0xE0b01012

struct _E_Dialog
{
   E_Object             e_obj_inherit;
   
   E_Win               *win;
   Evas_Object         *bg_object;
   Evas_Object         *box_object;
   Evas_Object         *text_object;
   Evas_Object         *icon_object;
   Evas_Object         *event_object;
   Evas_List           *buttons;
   Evas_List           *focused;
   void                *data;
};

EAPI E_Dialog *e_dialog_new        (E_Container *con);
EAPI void      e_dialog_button_add (E_Dialog *dia, char *label, char *icon, void (*func) (void *data, E_Dialog *dia), void *data);
EAPI int       e_dialog_button_focus_num(E_Dialog *dia, int button);
EAPI int       e_dialog_button_focus_button(E_Dialog *dia, E_Dialog_Button *button);
EAPI void      e_dialog_title_set  (E_Dialog *dia, char *title);
EAPI void      e_dialog_text_set   (E_Dialog *dia, char *text);
EAPI void      e_dialog_icon_set   (E_Dialog *dia, char *icon, Evas_Coord size);
EAPI void      e_dialog_show       (E_Dialog *dia);
    
#endif
#endif

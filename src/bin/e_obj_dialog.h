/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Obj_Dialog E_Obj_Dialog;

#else
#ifndef E_OBJ_DIALOG_H
#define E_OBJ_DIALOG_H

#define E_OBJ_DIALOG_TYPE 0xE0b0101b

struct _E_Obj_Dialog
{
   E_Object             e_obj_inherit;
   
   E_Win               *win;
   Evas_Object         *bg_object;
   void                *data;
   void		       (*cb_delete)(E_Obj_Dialog *od);
};

EAPI E_Obj_Dialog   *e_obj_dialog_new(E_Container *con, char *title, char *class_name, char *class_class);
EAPI void            e_obj_dialog_icon_set(E_Obj_Dialog *od, char *icon);
EAPI void            e_obj_dialog_show(E_Obj_Dialog *od);
EAPI void            e_obj_dialog_obj_part_text_set(E_Obj_Dialog *od, char *part, char *text);
EAPI void            e_obj_dialog_obj_theme_set(E_Obj_Dialog *od, char *theme_cat, char *theme_obj);
EAPI void	     e_obj_dialog_cb_delete_set(E_Obj_Dialog *od, void (*func)(E_Obj_Dialog *od));
    
#endif
#endif

#ifdef E_TYPEDEFS

typedef struct _E_Grab_Dialog E_Grab_Dialog;

#else
#ifndef E_GRAB_DIALOG_H
#define E_GRAB_DIALOG_H

#define E_GRAB_DIALOG_TYPE 0xE0b0104A

struct _E_Grab_Dialog
{
   E_Object e_obj_inherit;

   E_Dialog *dia;
   Ecore_X_Window grab_win;
   Ecore_Event_Handler_Cb key;
   Ecore_Event_Handler_Cb mouse;
   Ecore_Event_Handler_Cb wheel;
   Eina_List *handlers;
   void *data;
};

EAPI E_Grab_Dialog *e_grab_dialog_show(E_Win *parent, Eina_Bool is_mouse, Ecore_Event_Handler_Cb key, Ecore_Event_Handler_Cb mouse, Ecore_Event_Handler_Cb wheel, const void *data);

#endif
#endif

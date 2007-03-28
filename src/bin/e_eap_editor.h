/*
  * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
  */
#if 1
#ifdef E_TYPEDEFS

typedef struct _E_Desktop_Edit E_Desktop_Edit;

#else
#ifndef E_DESKTOP_EDIT_H
#define E_DESKTOP_EDIT_H

#define E_DESKTOP_EDIT_TYPE 0xE0b01019

struct _E_Desktop_Edit
{
   E_Object                     e_obj_inherit;
   
   Efreet_Desktop       *desktop;
   Evas        *evas;
   
   Evas_Object *img;
   Evas_Object *img_widget;
   Evas_Object *fsel;
   E_Dialog    *fsel_dia;
   int          img_set;
   
   E_Config_Dialog *cfd;
};

EAPI E_Desktop_Edit *e_desktop_edit_show(E_Container *con, Efreet_Desktop *desktop);

#endif
#endif
#endif

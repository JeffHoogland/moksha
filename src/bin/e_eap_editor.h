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
   Evas_Object *icon_fsel;
   E_Dialog    *icon_fsel_dia;
   Evas_Object *entry_widget;
   Evas_Object *exec_fsel;
   E_Dialog    *exec_fsel_dia;
   //int          img_set;

   char *tmp_image_path;
   int new_desktop;
   int saved; /* whether desktop has been saved or not */
   
   E_Config_Dialog *cfd;
};

EAPI Efreet_Desktop *e_desktop_border_create(E_Border *bd);
EAPI E_Desktop_Edit *e_desktop_border_edit(E_Container *con, E_Border *bd);
EAPI E_Desktop_Edit *e_desktop_edit(E_Container *con, Efreet_Desktop *desktop);

#endif
#endif
#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fileman E_Fileman;

#else
#ifndef E_FILEMAN_H
#define E_FILEMAN_H

#define E_FILEMAN_TYPE 0xE0b01016

struct _E_Fileman
{
   E_Object e_obj_inherit;

   E_Container *con;
   E_Win       *win;

   Evas *evas;
   Evas_Object *main;
   Evas_Object *vscrollbar;   

   Evas_Object *smart;   

   double xpos;
   double ypos;
};

EAPI E_Fileman *e_fileman_new(E_Container *con);
EAPI void       e_fileman_show(E_Fileman *fileman);
EAPI void       e_fileman_hide(E_Fileman *fileman);

#endif
#endif

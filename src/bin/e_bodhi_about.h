#ifdef E_TYPEDEFS

typedef struct _E_Obj_Dialog E_Bodhi_About;

#else
#ifndef E_BODHI_ABOUT_H
#define E_BODHI_ABOUT_H

EAPI E_Bodhi_About  *e_bodhi_about_new  (E_Container *con);
EAPI void            e_bodhi_about_show (E_Bodhi_About *about);

#endif
#endif

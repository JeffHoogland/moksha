/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Obj_Dialog E_About;

#else
#ifndef E_ABOUT_H
#define E_ABOUT_H

EAPI E_About  *e_about_new         (E_Container *con);
EAPI void      e_about_show        (E_About *about);
    
#endif
#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_About E_About;

#else
#ifndef E_ABOUT_H
#define E_ABOUT_H

#define E_ABOUT_TYPE 0xE0b0101a

struct _E_About
{
   E_Object             e_obj_inherit;
   
   E_Win               *win;
   Evas_Object         *bg_object;
   void                *data;
};

EAPI E_About  *e_about_new         (E_Container *con);
EAPI void      e_about_show        (E_About *about);
    
#endif
#endif

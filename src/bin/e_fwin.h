/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fwin E_Fwin;

#else
#ifndef E_FWIN_H
#define E_FWIN_H

#define E_FWIN_TYPE 0xE0b0101f

struct _E_Fwin
{
   E_Object             e_obj_inherit;
   
   E_Win               *win;
   Evas_Object         *scrollframe_obj;
   Evas_Object         *fm_obj;
   Evas_Object         *bg_obj;
};

EAPI int     e_fwin_init             (void);
EAPI int     e_fwin_shutdown         (void);
EAPI E_Fwin *e_fwin_new              (E_Container *con, const char *dev, const char *path);
    
#endif
#endif

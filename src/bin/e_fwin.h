/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fwin E_Fwin;
typedef struct _E_Fwin_Apps_Dialog E_Fwin_Apps_Dialog;

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
   E_Fwin_Apps_Dialog  *fad;

   Evas_Object         *under_obj;
   Evas_Object         *over_obj;
   struct {
      Evas_Coord        x, y, max_x, max_y, w, h;
   } fm_pan, fm_pan_last;
   unsigned char        under_tiled : 1;
   unsigned char        over_tiled : 1;
};

struct _E_Fwin_Apps_Dialog
{
   E_Dialog           *dia;
   E_Fwin             *fwin;
   char               *app1, *app2;
   Evas_Object        *o_ilist, *o_fm;
};

EAPI int     e_fwin_init             (void);
EAPI int     e_fwin_shutdown         (void);
EAPI E_Fwin *e_fwin_new              (E_Container *con, const char *dev, const char *path);
    
#endif
#endif

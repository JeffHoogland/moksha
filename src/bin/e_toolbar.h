#ifdef E_TYPEDEFS

typedef struct _E_Toolbar E_Toolbar;

#else
#ifndef E_TOOLBAR_H
#define E_TOOLBAR_H

#define E_TOOLBAR_TYPE 0xE0b0101f

struct _E_Toolbar 
{
   E_Object e_obj_inherit;

   int x, y, w, h, id;
   const char *name;

   Evas *evas;
   E_Gadcon *gadcon;
   E_Menu *menu;

   E_Win *fwin;
   Evas_Object *fm2;

   E_Config_Dialog *cfg_dlg;
   Evas_Object *o_base, *o_event;
};

EAPI int          e_toolbar_init          (void);
EAPI int          e_toolbar_shutdown      (void);
EAPI E_Toolbar   *e_toolbar_new           (Evas *evas, const char *name, E_Win *fwin, Evas_Object *fm2);
EAPI void         e_toolbar_fwin_set      (E_Toolbar *tbar, E_Win *fwin);
EAPI E_Win       *e_toolbar_fwin_get      (E_Toolbar *tbar);
EAPI void         e_toolbar_fm2_set       (E_Toolbar *tbar, Evas_Object *fm2);
EAPI Evas_Object *e_toolbar_fm2_get       (E_Toolbar *tbar);
EAPI void         e_toolbar_show          (E_Toolbar *tbar);
EAPI void         e_toolbar_move          (E_Toolbar *tbar, int x, int y);
EAPI void         e_toolbar_resize        (E_Toolbar *tbar, int w, int h);
EAPI void         e_toolbar_move_resize   (E_Toolbar *tbar, int x, int y, int w, int h);
EAPI void         e_toolbar_orient        (E_Toolbar *tbar, E_Gadcon_Orient orient);
EAPI void         e_toolbar_position_calc (E_Toolbar *tbar);
EAPI void         e_toolbar_populate      (E_Toolbar *tbar);

#endif
#endif

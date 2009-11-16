#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Il_Home_Win Il_Home_Win;

#define IL_HOME_WIN_TYPE 0xE0b0102f

struct _Il_Home_Win 
{
   E_Object e_obj_inherit;

   E_Win *win;
   Evas_Object *o_bg;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif

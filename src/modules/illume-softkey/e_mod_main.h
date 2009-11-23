#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define IL_SK_WIN_TYPE 0xE1b0784

typedef struct _Il_Sk_Win Il_Sk_Win;

struct _Il_Sk_Win 
{
   E_Object e_obj_inherit;

   E_Win *win;
   Evas_Object *o_base, *o_box;
   Evas_Object *b_close, *b_back;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif

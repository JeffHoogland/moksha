#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# define IL_IND_WIN_TYPE 0xE1b0886

typedef struct _Il_Ind_Win Il_Ind_Win;

struct _Il_Ind_Win 
{
   E_Object e_obj_inherit;
   E_Win *win;
   E_Menu *menu;
   E_Gadcon *gadcon;
   Evas_Object *o_base, *o_event;
   struct 
     {
        int y, start, dnd;
     } drag;
   int mouse_down;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

extern const char *mod_dir;

#endif

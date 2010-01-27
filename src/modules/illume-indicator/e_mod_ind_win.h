#ifndef E_MOD_IND_WIN_H
# define E_MOD_IND_WIN_H

# define IL_IND_WIN_TYPE 0xE1b0886

typedef struct _Il_Ind_Win Il_Ind_Win;
struct _Il_Ind_Win 
{
   E_Object e_obj_inherit;
   E_Win *win;
   E_Zone *zone;
   E_Menu *menu;
   E_Gadcon *gadcon;
   E_Border_Hook *hook;
   Evas_Object *o_base, *o_event;
   struct 
     {
        int y, start, dnd;
     } drag;
   int mouse_down;
};

Il_Ind_Win *e_mod_ind_win_new(E_Zone *zone);

#endif

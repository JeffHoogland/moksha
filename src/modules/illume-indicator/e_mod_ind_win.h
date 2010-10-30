#ifndef E_MOD_IND_WIN_H
# define E_MOD_IND_WIN_H

/* define indicator window object type */
# define IND_WIN_TYPE 0xE1b0886

/* define structure for indicator window */
typedef struct _Ind_Win Ind_Win;
struct _Ind_Win 
{
   E_Object e_obj_inherit;

   E_Zone *zone;
   Eina_List *hdls;

   E_Win *win;
   E_Popup *popup;
   Evas_Object *o_base, *o_event;
   E_Gadcon *gadcon;
   E_Menu *menu;

   struct 
     {
        int y, start, dnd, by;
     } drag;

   int mouse_down;
};

Ind_Win *e_mod_ind_win_new(E_Zone *zone);

#endif

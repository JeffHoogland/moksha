#ifndef E_MOD_SFT_WIN_H
# define E_MOD_SFT_WIN_H

# define IL_SFT_WIN_TYPE 0xE1b0784

typedef struct _Il_Sft_Win Il_Sft_Win;
struct _Il_Sft_Win 
{
   E_Object e_obj_inherit;
   E_Win *win;
   E_Zone *zone;
   E_Border_Hook *hook;
   Evas_Object *o_base, *o_box;
   Evas_Object *b_close, *b_back;
};

Il_Sft_Win *e_mod_sft_win_new(E_Zone *zone);

#endif

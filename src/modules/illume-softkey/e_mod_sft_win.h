#ifndef E_MOD_SFT_WIN_H
# define E_MOD_SFT_WIN_H

/* define softkey window object type */
# define SFT_WIN_TYPE 0xE1b0784

/* define structure for softkey window */
typedef struct _Sft_Win Sft_Win;
struct _Sft_Win 
{
   E_Object e_obj_inherit;

   E_Zone *zone;
   Eina_List *hdls;

   E_Win *win;
   Evas_Object *o_base;
   Eina_List *btns, *extra_btns;
};

Sft_Win *e_mod_sft_win_new(E_Zone *zone);

#endif

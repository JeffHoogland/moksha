#ifndef E_MOD_NOTIFY_H
# define E_MOD_NOTIFY_H

# define IND_NOTIFY_WIN_TYPE 0xE1b0887

typedef struct _Ind_Notify_Win Ind_Notify_Win;
struct _Ind_Notify_Win 
{
   E_Object e_obj_inherit;

   E_Notification *notify;

   E_Zone *zone;
   E_Win *win;
   Evas_Object *o_base, *o_icon;
   Ecore_Timer *timer;
};

int e_mod_notify_init(void);
int e_mod_notify_shutdown(void);

#endif

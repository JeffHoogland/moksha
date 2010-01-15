#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_H
#define E_MOD_COMP_H

typedef struct _E_Comp     E_Comp;
typedef struct _E_Comp_Win E_Comp_Win;

Eina_Bool    e_mod_comp_init                (void);
void         e_mod_comp_shutdown            (void);

void         e_mod_comp_shadow_set          (void);

E_Comp      *e_mod_comp_manager_get         (E_Manager *man);
E_Comp_Win  *e_mod_comp_win_find_by_window  (E_Comp *c, Ecore_X_Window win);
E_Comp_Win  *e_mod_comp_win_find_by_border  (E_Comp *c, E_Border *bd);
E_Comp_Win  *e_mod_comp_win_find_by_popup   (E_Comp *c, E_Popup *pop);
E_Comp_Win  *e_mod_comp_win_find_by_menu    (E_Comp *c, E_Menu *menu);
Evas_Object *e_mod_comp_win_evas_object_get (E_Comp_Win *cw);
    
#endif
#endif

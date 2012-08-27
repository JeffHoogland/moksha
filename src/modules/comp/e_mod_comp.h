#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_H
#define E_MOD_COMP_H

typedef struct _E_Comp      E_Comp;
typedef struct _E_Comp_Win  E_Comp_Win;
typedef struct _E_Comp_Zone E_Comp_Zone;

Eina_Bool e_mod_comp_init(void);
void      e_mod_comp_shutdown(void);
void      e_mod_comp_fps_toggle(void);
void      e_mod_comp_shadow_set(void);

#endif
#endif

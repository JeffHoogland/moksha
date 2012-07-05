#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_PHYSICS_H
#define E_MOD_PHYSICS_H

typedef struct _E_Physics     E_Physics;
typedef struct _E_Physics_Win E_Physics_Win;

void      e_mod_physics_mass_update(void);
Eina_Bool e_mod_physics_init(void);
void      e_mod_physics_shutdown(void);

#endif
#endif

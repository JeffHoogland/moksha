#ifndef E_MOD_PHYSICS_CFDATA_H
#define E_MOD_PHYSICS_CFDATA_H

typedef struct _Config Config;

struct _Config
{
   unsigned int delay; /* delay before applying physics */
   double max_mass; /* maximum mass for a window */
   double gravity; /* maximum mass for a window */
   Eina_Bool ignore_fullscreen;
   Eina_Bool ignore_maximized;
};

EAPI E_Config_DD *e_mod_physics_cfdata_edd_init(void);
EAPI Config *e_mod_physics_cfdata_config_new(void);
EAPI void    e_mod_cfdata_config_free(Config *cfg);

#endif

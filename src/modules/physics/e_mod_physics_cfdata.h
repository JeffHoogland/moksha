#ifndef E_MOD_PHYSICS_CFDATA_H
#define E_MOD_PHYSICS_CFDATA_H

typedef struct _Config Config;

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      0x0001
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0x0001
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

struct _Config
{
   int config_version;
   unsigned int delay; /* delay before applying physics */
   double max_mass; /* maximum mass for a window */
   double gravity; /* maximum mass for a window */
   Eina_Bool ignore_fullscreen;
   Eina_Bool ignore_maximized;
   Eina_Bool ignore_shelves;
   struct
   {
      Eina_Bool disable_rotate;
      Eina_Bool disable_move;
   } shelf;
};

EAPI E_Config_DD *e_mod_physics_cfdata_edd_init(void);
EAPI Config *e_mod_physics_cfdata_config_new(void);
EAPI void    e_mod_cfdata_config_free(Config *cfg);

#endif

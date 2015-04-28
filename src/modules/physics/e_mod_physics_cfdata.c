#include "e.h"
#include "e_mod_main.h"
#include "e_mod_physics_cfdata.h"

EAPI E_Config_DD *
e_mod_physics_cfdata_edd_init(void)
{
   E_Config_DD *conf_edd;

   conf_edd = E_CONFIG_DD_NEW("Physics_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, config_version, INT);
   E_CONFIG_VAL(D, T, delay, UINT);
   E_CONFIG_VAL(D, T, max_mass, DOUBLE);
   E_CONFIG_VAL(D, T, gravity, DOUBLE);
   E_CONFIG_VAL(D, T, ignore_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, ignore_maximized, UCHAR);
   E_CONFIG_VAL(D, T, ignore_shelves, UCHAR);
   E_CONFIG_VAL(D, T, shelf.disable_move, UCHAR);
   E_CONFIG_VAL(D, T, shelf.disable_rotate, UCHAR);
   return conf_edd;
}

EAPI Config *
e_mod_physics_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);
   cfg->config_version = (MOD_CONFIG_FILE_EPOCH << 16);
   cfg->delay = 10;
   cfg->max_mass = 3.0;
   cfg->gravity = 0.0;
   cfg->ignore_fullscreen = EINA_TRUE;
   cfg->ignore_maximized = EINA_TRUE;
   cfg->ignore_shelves = EINA_TRUE;

   return cfg;
}

EAPI void
e_mod_cfdata_config_free(Config *cfg)
{
   free(cfg);
}


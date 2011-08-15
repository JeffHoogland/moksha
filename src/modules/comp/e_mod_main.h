#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e_mod_comp_cfdata.h"

typedef struct _Mod    Mod;

struct _Mod
{
   E_Module        *module;

   E_Config_DD     *conf_edd;
   E_Config_DD     *conf_match_edd;
   Config          *conf;

   E_Config_Dialog *config_dialog;
};

extern Mod *_comp_mod;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);
EAPI int   e_modapi_info(E_Module *m);

void       _e_mod_config_new(E_Module *m);
void       _e_mod_config_free(E_Module *m);

#endif

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_remembers.h"
#undef E_TYPEDEFS
#include "e_int_config_remembers.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif

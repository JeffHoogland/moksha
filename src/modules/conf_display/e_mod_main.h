#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_display.h"
#include "e_int_config_screensaver.h"
#include "e_int_config_dpms.h"
#include "e_int_config_desklock.h"
#include "e_int_config_desklock_fsel.h"
#include "e_int_config_desks.h"
#include "e_int_config_desk.h"

#undef E_TYPEDEFS
#include "e_int_config_display.h"
#include "e_int_config_screensaver.h"
#include "e_int_config_dpms.h"
#include "e_int_config_desklock.h"
#include "e_int_config_desklock_fsel.h"
#include "e_int_config_desks.h"
#include "e_int_config_desk.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

#endif

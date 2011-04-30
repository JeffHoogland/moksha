#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_window_manip.h"
#include "e_int_config_window_display.h"
#include "e_int_config_focus.h"
#include "e_int_config_clientlist.h"

#undef E_TYPEDEFS
#include "e_int_config_window_manip.h"
#include "e_int_config_window_display.h"
#include "e_int_config_focus.h"
#include "e_int_config_clientlist.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

#endif

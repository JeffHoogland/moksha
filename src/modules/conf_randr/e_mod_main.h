/*
 * vim:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_randr.h"

#undef E_TYPEDEFS
#include "e_int_config_randr.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_RandR RandR (Screen Resize, Rotate and Mirror)
 *
 * Configures size, rotation and mirroring of screen. Uses the X11
 * RandR protocol (does not work with NVidia proprietary drivers).
 *
 * @}
 */
#endif

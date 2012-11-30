#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

#ifndef ECORE_X_RANDR_1_2
# define ECORE_X_RANDR_1_2 ((1 << 16) | 2)
#endif

#ifndef ECORE_X_RANDR_1_3
# define ECORE_X_RANDR_1_3 ((1 << 16) | 3)
#endif

#ifndef E_RANDR_12
# define E_RANDR_12 (e_randr_screen_info.rrvd_info.randr_info_12)
#endif

E_Config_Dialog *e_int_config_randr(E_Container *con, const char *params __UNUSED__);
extern const char *mod_dir;

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

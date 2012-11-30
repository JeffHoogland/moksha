#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_desk(E_Container *con, const char *params);
E_Config_Dialog *e_int_config_screensaver(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_dpms(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_display(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_desks(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_desklock(E_Container *con, const char *params __UNUSED__);
void e_int_config_desklock_fsel_done(E_Config_Dialog *cfd, Evas_Object *bg, const char *bg_file);
E_Config_Dialog *e_int_config_desklock_fsel(E_Config_Dialog *parent, Evas_Object *bg);
void e_int_config_desklock_fsel_del(E_Config_Dialog *cfd);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Display Display Configuration
 *
 * Configures the physical and virtual display, including screen
 * saver, screen lock and power saving settings (DPMS).
 *
 * @}
 */
#endif

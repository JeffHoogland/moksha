#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_THEME_H
#define E_INT_CONFIG_THEME_H

E_Config_Dialog *e_int_config_theme(E_Container *con, const char *params __UNUSED__);

void             e_int_config_theme_import_done(E_Config_Dialog *dia);
void             e_int_config_theme_update(E_Config_Dialog *dia, char *file);
void             e_int_config_theme_web_done(E_Config_Dialog *dia);

#endif
#endif

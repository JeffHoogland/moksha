#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_IMC_H
#define E_INT_CONFIG_IMC_H

E_Config_Dialog *e_int_config_imc(E_Container *con, const char *params __UNUSED__);
void e_int_config_imc_import_done(E_Config_Dialog *dia);
void e_int_config_imc_update(E_Config_Dialog *dia, const char *file);

#endif
#endif

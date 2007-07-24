/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_IMC_H
#define E_INT_CONFIG_IMC_H

EAPI E_Config_Dialog *e_int_config_imc(E_Container *con, const char *params __UNUSED__);
EAPI void e_int_config_imc_import_done(E_Config_Dialog *dia);
EAPI void e_int_config_imc_update(E_Config_Dialog *dia, const char *file);

#endif
#endif

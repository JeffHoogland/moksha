/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* This file is the counterpart for data storage of e_intl */
/* This only needs to be separate because the e_imc binary and other third parties
   many waht to include the functionality to read IMC data from EET files
 */
static Eet_Data_Descriptor *_e_intl_input_method_config_edd = NULL;

EAPI int
e_intl_data_init(void)
{
   _e_intl_input_method_config_edd = E_CONFIG_DD_NEW("input_method_config", E_Input_Method_Config);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, version, INT);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, e_im_name, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, gtk_im_module, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, qt_im_module, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, xmodifiers, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, e_im_exec, STR);
   E_CONFIG_VAL(_e_intl_input_method_config_edd, E_Input_Method_Config, e_im_setup_exec, STR);

   return 1;
}

EAPI int
e_intl_data_shutdown(void)
{
   E_CONFIG_DD_FREE(_e_intl_input_method_config_edd);

   return 1;
}


/* Get the input method configuration from the file */
EAPI E_Input_Method_Config *
e_intl_input_method_config_read(Eet_File *imc_file)
{
   E_Input_Method_Config *imc;

   imc = NULL;
   if (imc_file)
     {
	imc = (E_Input_Method_Config *) eet_data_read(imc_file, _e_intl_input_method_config_edd, "imc");
     }
   return imc;
}

/* Write the input method configuration to the file */
EAPI int
e_intl_input_method_config_write(Eet_File *imc_file, E_Input_Method_Config *imc)
{
   int ok = 0;

   if (imc_file)
     {
	ok = eet_data_write(imc_file, _e_intl_input_method_config_edd, "imc", imc, 0);
     }
   return ok;
}

EAPI void
e_intl_input_method_config_free(E_Input_Method_Config *imc)
{
   if (imc != NULL)
     {
	if (imc->e_im_name) eina_stringshare_del(imc->e_im_name);
	if (imc->gtk_im_module) eina_stringshare_del(imc->gtk_im_module);
	if (imc->qt_im_module) eina_stringshare_del(imc->qt_im_module);
	if (imc->xmodifiers) eina_stringshare_del(imc->xmodifiers);
	if (imc->e_im_exec) eina_stringshare_del(imc->e_im_exec);
	if (imc->e_im_setup_exec) eina_stringshare_del(imc->e_im_setup_exec);
	E_FREE(imc);
     }
}


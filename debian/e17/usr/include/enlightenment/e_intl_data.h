#ifdef E_TYPEDEFS

typedef struct _E_Input_Method_Config E_Input_Method_Config;

#else
#ifndef E_INTL_DATA_H
#define E_INTL_DATA_H

#define E_INTL_INPUT_METHOD_CONFIG_VERSION 3

struct _E_Input_Method_Config
{
   int version;
   const char *e_im_name;
   const char *gtk_im_module;
   const char *qt_im_module;
   const char *xmodifiers;
   const char *ecore_imf_module;
   const char *e_im_exec;
   const char *e_im_setup_exec;
};

EINTERN int		 e_intl_data_init(void);
EINTERN int		 e_intl_data_shutdown(void);
EAPI E_Input_Method_Config *e_intl_input_method_config_read(Eet_File *imc_file);
EAPI int		 e_intl_input_method_config_write(Eet_File *imc_file, E_Input_Method_Config *imc);
EAPI void		 e_intl_input_method_config_free(E_Input_Method_Config *imc);
#endif
#endif

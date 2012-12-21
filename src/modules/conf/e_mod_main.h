#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      1
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH * 1000000) + MOD_CONFIG_FILE_GENERATION)

typedef struct _Config Config;
struct _Config 
{
   E_Module *module;
   E_Config_Dialog *cfd;
   E_Int_Menu_Augmentation *aug;
   int version;
   int menu_augmentation;
};

void e_configure_show(E_Container *con, const char *params);
void e_configure_del(void);

E_Config_Dialog *e_int_config_conf_module(E_Container *con, const char *params);
void e_mod_config_menu_add(void *data, E_Menu *m);

extern Config *conf;

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf Main Configuration Dialog
 *
 * Show the main configuration dialog used to access other
 * configuration.
 *
 * @}
 */

#endif

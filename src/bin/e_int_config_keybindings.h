#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_KEYBINDINGS_H
#define E_INT_CONFIG_KEYBINDINGS_H

#define e_register_action_predef_name(ag, an, ac, ap, r, f) \
   e_int_config_keybindings_register_action_predef_name(ag, an, ac, ap, r, f)

#define e_unregister_action_predef_name(ag, an) \
   e_int_config_keybindings_unregister_action_predef_name(ag, an)

#define e_unregister_all_action_predef_names \
   e_int_config_keybindings_unregister_all_action_predef_names

typedef enum{EDIT_RESTRICT_NONE = (0 << 0), // allows to edit action and params in config dialog
	     EDIT_RESTRICT_ACTION = (1 << 0), // denies to edit action in config dialog
	     EDIT_RESTRICT_PARAMS = (1 << 1) // denies to edit params in config dialog
	    }act_restrict_t;

EAPI E_Config_Dialog *e_int_config_keybindings(E_Container *con);

int e_int_config_keybindings_register_action_predef_name(const char *action_group,
							 const char *action_name,
							 const char *action_cmd,
							 const char *action_params,
							 act_restrict_t restrictions,
							 int flag);

int e_int_config_keybindings_unregister_action_predef_name(const char *action_group,
							   const char *action_name);

void e_int_config_keybindings_unregister_all_action_predef_names();

#endif
#endif

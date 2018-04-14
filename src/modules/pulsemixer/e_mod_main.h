#ifndef _E_MOD_MAIN_H_
#define _E_MOD_MAIN_H_

#define CONFIG_VERSION 1

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);

#endif /* _E_MOD_MAIN_H_ */

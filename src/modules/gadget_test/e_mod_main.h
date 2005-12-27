#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Test Test;
struct _Test 
{
  int val;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
/*EAPI int   e_modapi_config   (E_Module *module);*/

#endif

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config       Config;

struct _Config
{
   /* saved * loaded config values */
   Eina_Bool            window;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif

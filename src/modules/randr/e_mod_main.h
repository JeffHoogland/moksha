/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Randr       Randr;

struct _Config
{
   int store;
   int width;
   int height;
};

struct _Randr
{
   E_Menu      *config_menu;
   E_Menu      *resolution_menu;

   E_Int_Menu_Augmentation *augmentation;

   Config      *conf;
};

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);

#endif

/*
 * Main module header.
 * Contains some i18n stuff, module versioning,
 * config and public prototypes from main.
 */

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Xkb
{
   E_Module *module;
   E_Config_Dialog *cfd;
   Ecore_Event_Handler *evh;
} Xkb;

/* Prototypes */

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init    (E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save    (E_Module *m);

void                 _xkb_update_icon(int);
E_Config_Dialog     *_xkb_cfg_dialog(E_Container *con, const char *params);

extern Xkb           _xkb;

#endif

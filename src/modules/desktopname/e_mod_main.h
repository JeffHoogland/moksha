#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _DesktopName DesktopName;

struct _Config
{
   double speed;
};

struct _DesktopName
{
   E_Config_DD *conf_edd;
   Config      *conf;
   E_Popup     *popup;
   Evas_Object *obj;
   
   Ecore_Event_Handler *ev_handler_desk_show;
};

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);

#endif

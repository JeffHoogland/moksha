#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config     Config;
typedef struct _Clock      Clock;
typedef struct _Clock_Face Clock_Face;

struct _Config
{
   int dummy; /* just here to hold space */
};

struct _Clock
{
   E_Menu      *config_menu;
   Clock_Face *face;
   
/*   E_Config_DD *conf_edd;*/
   Config      *conf;
};

struct _Clock_Face
{
   Clock       *clock;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *clock_object;
   Evas_Object *event_object;
   
   E_Gadman_Client *gmc;
/*   Ecore_Event_Handler *ev_handler_container_resize;*/
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

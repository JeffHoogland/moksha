#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config           Config;
typedef struct _Temperature      Temperature;
typedef struct _Temperature_Face Temperature_Face;

struct _Config
{
   double poll_time;
   int low, high;
};

struct _Temperature
{
   E_Menu           *config_menu;
   E_Menu           *config_menu1;
   E_Menu           *config_menu2;
   E_Menu           *config_menu3;
   Temperature_Face *face;
   
   E_Config_DD *conf_edd;
   Config      *conf;
};

struct _Temperature_Face
{
   Temperature *temp;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *temp_object;
   Evas_Object *event_object;
   
   int             have_temp;
   
   Ecore_Timer         *temperature_check_timer;
   
   E_Gadman_Client *gmc;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

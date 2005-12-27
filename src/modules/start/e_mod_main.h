#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Face Config_Face;
typedef struct _Start       Start;
typedef struct _Start_Face  Start_Face;

struct _Config
{
   Evas_List *faces;
};

struct _Config_Face
{
   unsigned char enabled;
};

struct _Start
{
   Evas_List   *faces;
   E_Menu      *config_menu;
   
   Config      *conf;
};

struct _Start_Face
{
   E_Container *con;
   E_Menu      *menu;
   E_Menu      *main_menu;
   
   Config_Face *conf;
   
   Evas_Object *button_object;
   Evas_Object *event_object;
   
   E_Gadman_Client *gmc;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
/* EAPI int   e_modapi_config   (E_Module *module); */

#endif

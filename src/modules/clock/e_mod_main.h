#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config     Config;
typedef struct _Clock      Clock;
typedef struct _Clock_Face Clock_Face;

struct _Config
{
   int width;
   double x, y;
};

struct _Clock
{
   E_Menu      *config_menu;
   Clock_Face *face;
   
   E_Config_DD *conf_edd;
   Config      *conf;
};

struct _Clock_Face
{
   Clock       *clock;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *clock_object;
   Evas_Object *event_object;
   
   Evas_Coord   minsize, maxsize;
   
   unsigned char   move : 1;
   unsigned char   resize : 1;
   Evas_Coord      xx, yy;
   Evas_Coord      fx, fy, fw;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

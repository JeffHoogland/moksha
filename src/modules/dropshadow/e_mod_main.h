#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Shpix      Shpix;
typedef struct _Config     Config;
typedef struct _Dropshadow Dropshadow;
typedef struct _Shadow     Shadow;

struct _Config
{
   int shadow_x, shadow_y;
   int blur_size;
   double shadow_darkness;
};

struct _Dropshadow
{
   E_Module       *module;
   Evas_List      *shadows;
   Evas_List      *cons;
   E_Before_Idler *idler_before;

   E_Config_DD    *conf_edd;
   Config         *conf;
   
   struct {
      unsigned char *gauss;
      int            gauss_size;
   } table;
};

struct _Shadow
{
   Dropshadow *ds;
   int x, y, w, h;
   E_Container_Shape *shape;
   
   Evas_Object *object[4];
   
   unsigned char reshape : 1;
   unsigned char square : 1;
   unsigned char toosmall : 1;
};

struct _Shpix
{
   int            w, h;
   unsigned char *pix;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config     Config;
typedef struct _Snow       Snow;
typedef struct _Snow_Flake Snow_Flake;

struct _Config
{
   int tree_count;
   int flake_count;
};

struct _Snow
{
   E_Module       *module;
   Evas_List      *cons;
   Evas           *canvas;
   Ecore_Animator *animator;
   Evas_List      *trees;
   Evas_List      *flakes;

   E_Config_DD    *conf_edd;
   Config         *conf;
};

struct _Snow_Flake
{
   Evas_Object    *flake;
   double          start_time;
   int             speed;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

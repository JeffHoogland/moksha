#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef enum _Flame_Palette_Type Flame_Palette_Type;

typedef struct _Config     Config;
typedef struct _Flame      Flame;
typedef struct _Flame_Face Flame_Face;

enum _Flame_Palette_Type
{
   DEFAULT_NONE,
   DEFAULT_PALETTE,
     PLASMA_PALETTE
};

struct _Config
{
   Evas_Coord         height;
   int                hspread, vspread;
   int                variance;
   int                vartrend;
   int                residual;
   Flame_Palette_Type palette_type;
};

struct _Flame
{
   E_Menu     *config_menu;
   Flame_Face *face;
   
   E_Config_DD *conf_edd;
   Config      *conf;
};

struct _Flame_Face
{
   Flame       *flame;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *flame_object;
   Ecore_Animator *anim;
   
   Evas_Coord   xx, yy, ww;
   
   /* palette */
   unsigned int *palette;
   unsigned int *im;
   int           ims;
   
   /* the flame arrays */
   int           ws;
   unsigned int *f_array1, *f_array2;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif /* E_MOD_MAIN_H */

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Shpix         Shpix;
typedef struct _Shstore       Shstore;
typedef struct _Config        Config;
typedef struct _Dropshadow    Dropshadow;
typedef struct _Shadow        Shadow;
typedef struct _Shadow_Object Shadow_Object;
typedef struct _Tilebuf       Tilebuf;
typedef struct _Tilebuf_Tile  Tilebuf_Tile;

struct _Shpix
{
   int            w, h;
   unsigned char *pix;
};

struct _Shstore
{
   int            w, h;
   unsigned int  *pix;
};

struct _Config
{
   int shadow_x, shadow_y;
   int blur_size;
   int quality;
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
   E_Config_Dialog *config_dialog;
   
   struct {
      unsigned char *gauss;
      int            gauss_size;
      unsigned char *gauss2;
      int            gauss2_size;
   } table;
   
   struct {
      Shstore *shadow[4];
      int      ref;
   } shared;
};

struct _Shadow
{
   Dropshadow *ds;
   int x, y, w, h;
   E_Container_Shape *shape;
   
   Evas_Object *object[4];
   
   Evas_List *object_list;
   
   unsigned char initted : 1;
   unsigned char reshape : 1;
   unsigned char square : 1;
   unsigned char toosmall : 1;
   unsigned char use_shared : 1;
   unsigned char visible : 1;
};

struct _Shadow_Object
{
   int x, y, w, h;
   Evas_Object *obj;
};

struct _Tilebuf
{
   int outbuf_w;
   int outbuf_h;
   
   struct {
      int           w, h;
   } tile_size;
   
   struct {
      int           w, h;
      Tilebuf_Tile *tiles;
   } tiles;
};

struct _Tilebuf_Tile
{
   int redraw : 1;
};

extern E_Module *dropshadow_mod;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);

void  _dropshadow_cb_config_updated(void *data);

#endif

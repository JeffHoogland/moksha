#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _IBar        IBar;
typedef struct _IBar_Bar    IBar_Bar;
typedef struct _IBar_Icon   IBar_Icon;

struct _Config
{
   char         *appdir;
   double        follow_speed;
   double        autoscroll_speed;
   int           width;
   int           iconsize;
   int           edge;
   double        anchor;
   double        handle;
   char          autohide;
};

struct _IBar
{
   E_App       *apps;
   Evas_List   *bars;
   E_Menu      *config_menu;
   
   E_Config_DD *conf_edd;
   Config      *conf;
};

struct _IBar_Bar
{
   IBar        *ibar;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *bar_object;
   Evas_Object *overlay_object;
   Evas_Object *box_object;
   Evas_Object *event_object;
   
   Evas_List   *icons;
   
   Evas_Coord   minsize, maxsize;
   
   double          align, align_req;
   double          follow, follow_req;
   Ecore_Timer    *timer;
   Ecore_Animator *animator;
   
   Evas_Coord      x, y, w, h;
   
   unsigned char   move : 1;
   unsigned char   resize1 : 1;
   unsigned char   resize2 : 1;
   Evas_Coord      start_x, start_y;
   Evas_Coord      start_bx, start_by, start_bw, start_bh;
   
   Ecore_Event_Handler *ev_handler_container_resize;
};

struct _IBar_Icon
{
   IBar_Bar      *ibb;
   E_App         *app;
   Evas_Object   *bg_object;
   Evas_Object   *overlay_object;
   Evas_Object   *icon_object;
   Evas_Object   *event_object;
   Evas_List     *extra_icons;
   
   unsigned char  raise_on_hilight : 1;
};

#define EDGE_BOTTOM 0
#define EDGE_TOP    1
#define EDGE_LEFT   2
#define EDGE_RIGHT  3

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _IBar        IBar;
typedef struct _IBar_Bar    IBar_Bar;
typedef struct _IBar_Icon   IBar_Icon;

#define IBAR_WIDTH_AUTO -1

struct _Config
{
   char         *appdir;
   int           edge;
   double        follow_speed;
   double        autoscroll_speed;
   int           iconsize;
   int           width;
   /*
   double        anchor;
   double        handle;
   char          autohide;
   */
};

struct _IBar
{
   E_App       *apps;
   Evas_List   *bars;
   E_Menu      *config_menu;
   
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
   
   double          align, align_req;
   double          follow, follow_req;
   Ecore_Timer    *timer;
   Ecore_Animator *animator;
   
   Evas_Coord      x, y, w, h;
   
   E_Gadman_Client *gmc;
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

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

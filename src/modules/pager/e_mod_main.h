#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config     Config;
typedef struct _Pager      Pager;

struct _Config
{
   int    width, height;
   double x, y;
};

struct _Pager
{
   E_Menu       *config_menu;
   Evas         *evas;
   Evas_Object  *base, *screen;
   Evas_List    *desks, *wins;
   
   E_Container  *con;
   E_Config_DD  *conf_edd;
   Config       *conf;
   unsigned char move : 1;
   unsigned char resize : 1;
   Ecore_Event_Handler *ev_handler_container_resize;
   
   Ecore_Event_Handler *ev_handler_border_resize;
   Ecore_Event_Handler *ev_handler_border_move;
   Ecore_Event_Handler *ev_handler_border_add;
   Ecore_Event_Handler *ev_handler_border_remove;
   Ecore_Event_Handler *ev_handler_border_hide;
   Ecore_Event_Handler *ev_handler_border_show;
   Ecore_Event_Handler *ev_handler_border_desk_set;

   Evas_Coord    fx, fy, fw, fh, tw, th;
   Evas_Coord    xx, yy;

   /* FIXME: want to fix click detection once leftdrag is not used */
   Evas_Coord    clickhackx, clickhacky;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif

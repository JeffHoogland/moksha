/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Face Config_Face;
typedef struct _Pager       Pager;
typedef struct _Pager_Face  Pager_Face;
typedef struct _Pager_Desk  Pager_Desk;
typedef struct _Pager_Win   Pager_Win;
typedef struct _Pager_Popup Pager_Popup;

#define PAGER_RESIZE_NONE 0
#define PAGER_RESIZE_HORZ 1
#define PAGER_RESIZE_VERT 2
#define PAGER_RESIZE_BOTH 3

#define PAGER_DESKNAME_NONE   0
#define PAGER_DESKNAME_TOP    1
#define PAGER_DESKNAME_BOTTOM 2
#define PAGER_DESKNAME_LEFT   3
#define PAGER_DESKNAME_RIGHT  4

struct _Config
{
   Evas_List *faces;
   /* Position of desktop name */
   unsigned int deskname_pos;
   /* How the popup is shown on desk change */
   double popup_speed;
   /* Show popup? */
   unsigned int popup;

};

struct _Config_Face
{
   /* Show face */
   unsigned char enabled;
   /* Keep scale of desktops */
   unsigned char scale;
   /* Resize pager when adding/removing desktops */
   unsigned char resize;
};

struct _Pager
{
   Evas_List   *faces;
   E_Menu      *config_menu;
   E_Menu      *config_menu_deskname;
   E_Menu      *config_menu_speed;
   Evas_List   *menus;

   Config      *conf;

   Ecore_Event_Handler *ev_handler_border_resize;
   Ecore_Event_Handler *ev_handler_border_move;
   Ecore_Event_Handler *ev_handler_border_add;
   Ecore_Event_Handler *ev_handler_border_remove;
   Ecore_Event_Handler *ev_handler_border_iconify;
   Ecore_Event_Handler *ev_handler_border_uniconify;
   Ecore_Event_Handler *ev_handler_border_stick;
   Ecore_Event_Handler *ev_handler_border_unstick;
   Ecore_Event_Handler *ev_handler_border_desk_set;
   Ecore_Event_Handler *ev_handler_border_stack;
   Ecore_Event_Handler *ev_handler_border_icon_change;
   Ecore_Event_Handler *ev_handler_zone_desk_count_set;
   Ecore_Event_Handler *ev_handler_desk_show;
   Ecore_Event_Handler *ev_handler_desk_name_change;
   
   E_Config_Dialog *config_dialog;
};

struct _Pager_Face
{
   Pager           *pager;
   E_Gadman_Client *gmc;
   E_Menu          *menu;
   Evas            *evas;

   E_Zone          *zone;
   Evas_List       *desks;

   Evas_Object  *pager_object;
   Evas_Object  *table_object;

   Evas_Coord    fx, fy, fw, fh;
   struct {
	Evas_Coord l, r, t, b;
   } inset;

   /* Current nr. of desktops */
   int           xnum, ynum;

   Config_Face  *conf;

   E_Drop_Handler *drop_handler;

   Pager_Popup *current_popup;

   unsigned char dragging:1;
};

struct _Pager_Desk
{
   E_Desk      *desk;
   Pager_Face  *face;
   Evas_List   *wins;

   Evas_Object *desk_object;
   Evas_Object *layout_object;
   Evas_Object *event_object;

   int          xpos, ypos;

   int          current : 1;
};

struct _Pager_Win
{
   E_Border    *border;
   Pager_Desk  *desk;

   Evas_Object *window_object;
   Evas_Object *icon_object;
   Evas_Object *event_object;

   struct {
	Pager_Face *from_face;
	unsigned char start : 1;
	int x, y;
   } drag;
};

struct _Pager_Popup
{
   E_Popup     *popup;
   Pager_Face  *src_face, *face;
   Evas_Object *bg_object;
   Ecore_Timer *timer;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *module);
EAPI int   e_modapi_shutdown (E_Module *module);
EAPI int   e_modapi_save     (E_Module *module);
EAPI int   e_modapi_info     (E_Module *module);
EAPI int   e_modapi_about    (E_Module *module);
EAPI int   e_modapi_config   (E_Module *module);

void  _pager_cb_config_updated(void *data);

#endif

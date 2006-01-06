/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Box  Config_Box;
typedef struct _IBox        IBox;
typedef struct _IBox_Box    IBox_Box;
typedef struct _IBox_Icon   IBox_Icon;

#define IBOX_WIDTH_AUTO -1
#define IBOX_WIDTH_FIXED -2

struct _Config
{
   double        follow_speed;
   int           follower;
   double        autoscroll_speed;
   int           iconsize;
   int           width;
   Evas_List    *boxes;
};

struct _Config_Box
{
   unsigned char enabled;
};

struct _IBox
{
   Evas_List   *boxes;
   E_Menu      *config_menu;
   
   Config      *conf;
   E_Config_Dialog *config_dialog;
};

struct _IBox_Box
{
   IBox        *ibox;
   E_Container *con;
   Evas        *evas;
   E_Menu      *menu;
   
   Evas_Object *box_object;
   Evas_Object *overlay_object;
   Evas_Object *item_object;
   Evas_Object *event_object;
   
   Evas_List   *icons;

   Ecore_Event_Handler *ev_handler_border_iconify;
   Ecore_Event_Handler *ev_handler_border_uniconify;
   Ecore_Event_Handler *ev_handler_border_remove;
   
   double          align, align_req;
   double          follow, follow_req;
   Ecore_Timer    *timer;
   Ecore_Animator *animator;
   
   Evas_Coord      x, y, w, h;
   struct {
	Evas_Coord l, r, t, b;
   } box_inset;
   struct {
	Evas_Coord l, r, t, b;
   } icon_inset;

   E_Gadman_Client *gmc;

   Config_Box     *conf;
};

struct _IBox_Icon
{
   IBox_Box      *ibb;
   E_Border      *border;
   Evas_Object   *bg_object;
   Evas_Object   *overlay_object;
   Evas_Object   *icon_object;
   Evas_Object   *event_object;

   unsigned char  raise_on_hilight : 1;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

void _ibox_box_cb_config_updated(void *data);

#endif

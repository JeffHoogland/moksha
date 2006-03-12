/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Box  Config_Box;
typedef struct _ITray        ITray;
typedef struct _ITray_Box    ITray_Box;
typedef struct _ITray_Tray   ITray_Tray;

#define ITRAY_WIDTH_AUTO -1
#define ITRAY_WIDTH_FIXED -2

struct _Config
{
   int           rowsize;
   int           width;
   Evas_List    *boxes;
};

struct _Config_Box
{
   unsigned char enabled;
};

struct _ITray
{
   Evas_List   *boxes;
   E_Menu      *config_menu;
   
   Config      *conf;
   E_Config_Dialog *config_dialog;
};

struct _ITray_Box
{
   ITray       *itray;
   E_Container *con;
   Evas        *evas;
   E_Menu      *menu;
   
   Evas_Object *box_object;
   Evas_Object *item_object;
   Evas_Object *event_object;
   
   Evas_Coord      x, y, w, h;
   struct {
      Evas_Coord l, r, t, b;
   } box_inset;

   E_Gadman_Client *gmc;

   ITray_Tray    *tray;
   Config_Box     *conf;
};

struct _ITray_Tray
{
   int			w, h;
   int			icons, rows;
   Evas_List		*wins;
   Ecore_X_Window	win;

   Ecore_Event_Handler	*msg_handler;
   Ecore_Event_Handler	*dst_handler;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

void _itray_box_cb_config_updated(void *data);

#endif

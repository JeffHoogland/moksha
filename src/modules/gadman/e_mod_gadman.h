#ifndef E_MOD_GADMAN_H
#define E_MOD_GADMAN_H

#define DEFAULT_POS_X  0.1
#define DEFAULT_POS_Y  0.1
#define DEFAULT_SIZE_W 0.07
#define DEFAULT_SIZE_H 0.07

#define DRAG_START 0
#define DRAG_STOP  1
#define DRAG_MOVE  2

#define BG_STD    0
#define BG_COLOR  1
#define BG_CUSTOM 2
#define BG_TRANS  3

#define MIN_VISIBLE_MARIGIN 20

typedef struct _Manager Manager;
typedef struct _Config Config;

struct _Config
{
   int bg_type;
   int color_r;
   int color_g;
   int color_b;
   int color_a;
   const char* custom_bg;
   int anim_bg;
   int anim_gad;
};

typedef enum
{
  GADMAN_LAYER_BG = 0, /* layer is considered unsigned int */
  GADMAN_LAYER_TOP,
  GADMAN_LAYER_COUNT
} Gadman_Layer_Type;

#define ID_GADMAN_LAYER_BASE 114
#define ID_GADMAN_LAYER_BG (ID_GADMAN_LAYER_BASE + GADMAN_LAYER_BG)
#define ID_GADMAN_LAYER_TOP (ID_GADMAN_LAYER_BASE + GADMAN_LAYER_TOP)

typedef struct Gadman_Popup
{
   EINA_INLIST;
   E_Gadcon_Client *gcc;
   E_Gadcon_Popup *pop;
   Ecore_Timer *timer;
   Ecore_Event_Handler *eh;
} Gadman_Popup;

struct _Manager
{
   Eina_List   *gadcons[GADMAN_LAYER_COUNT];
   E_Gadcon    *gc_top;
   E_Gadcon_Location *location[GADMAN_LAYER_COUNT];
   Eina_List   *gadgets[GADMAN_LAYER_COUNT];
   Ecore_Timer *gadman_reset_timer;
   Evas_Object *movers[GADMAN_LAYER_COUNT];
   Evas_Object *full_bg;
   const char  *icon_name;
   E_Gadcon_Client *drag_gcc[GADMAN_LAYER_COUNT];

   Eina_List *drag_handlers;

   Eina_Inlist *gadman_popups;

   Eina_List *waiting;
   Ecore_Event_Handler *add;
   
   int             visible;
   int             use_composite;
   Ecore_X_Window  top_win;
   Ecore_Evas     *top_ee;
   E_Container    *container;

   Evas_Coord  width, height;
   
   E_Module                *module;
   E_Config_Dialog         *config_dialog;
   E_Int_Menu_Augmentation *maug;
   E_Menu_Category_Callback *mcat;
   E_Action                *action;

   E_Config_DD    *conf_edd;
   Config         *conf;
};

extern Manager *Man;

void             gadman_init(E_Module *m);
void             gadman_shutdown(void);
E_Gadcon_Client *gadman_gadget_add(const E_Gadcon_Client_Class *cc, E_Gadcon_Client *, Gadman_Layer_Type layer);
void             gadman_gadget_edit_start(E_Gadcon_Client *gcc);
void             gadman_gadget_edit_end(void *data, Evas_Object *obj, const char *emission, const char *source);
void             gadman_gadgets_toggle(void);
void             gadman_update_bg(void);
Eina_Bool gadman_gadget_add_handler(void *d, int type, E_Event_Gadcon_Client_Add *ev);
#endif

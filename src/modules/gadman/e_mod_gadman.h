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

struct _Manager
{
   E_Gadcon    *gc;
   E_Gadcon    *gc_top;
   Eina_List   *gadgets;
   Evas_Object *mover;
   Evas_Object *mover_top;
   Evas_Object *full_bg;
   const char  *icon_name;
   
   int             visible;
   int             use_composite;
   Ecore_X_Window  top_win;
   Ecore_Evas     *top_ee;
   E_Container    *container;

   Evas_Coord  width, height;
   
   E_Module                *module;
   E_Config_Dialog         *config_dialog;
   E_Int_Menu_Augmentation *maug;
   E_Action                *action;

   E_Config_DD    *conf_edd;
   Config         *conf;
};

Manager *Man;

void             gadman_init(E_Module *m);
void             gadman_shutdown(void);
E_Gadcon_Client *gadman_gadget_add(E_Gadcon_Client_Class *cc, int ontop);
void             gadman_gadget_del(E_Gadcon_Client *gcc);
E_Gadcon_Client *gadman_gadget_place(E_Config_Gadcon_Client *cf, int ontop);
void             gadman_gadget_edit_start(E_Gadcon_Client *gcc);
void             gadman_gadget_edit_end(void);
void             gadman_gadgets_toggle(void);
void             gadman_update_bg(void);

#endif

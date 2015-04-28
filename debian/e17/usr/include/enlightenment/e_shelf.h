#ifdef E_TYPEDEFS

typedef struct _E_Shelf E_Shelf;

#else
#ifndef E_SHELF_H
#define E_SHELF_H

#define E_SHELF_TYPE 0xE0b0101e
#define E_SHELF_DUMMY_TYPE 0xE0b0102e

struct _E_Shelf
{
   E_Object             e_obj_inherit;
   int                  id;
   int                  x, y, w, h;
   E_Layer              layer;
   E_Popup             *popup; /* NULL if its within an existing canvas */
   E_Zone              *zone;
   Evas_Object         *o_base;
   Evas_Object         *o_event;
   Ecore_Evas          *ee;
   Evas                *evas;
   E_Gadcon            *gadcon;
   const char          *name;
   const char          *style;
   E_Config_Shelf      *cfg;
   int                  size;
   E_Config_Dialog     *config_dialog;
   E_Entry_Dialog      *rename_dialog;
   E_Menu              *menu;
   Ecore_Timer         *hide_timer;
   Ecore_Animator      *hide_animator;
   int                  hide_step;
   int                  hidden_state_size;
   int                  hide_origin;
   int                  interrupted;
   float                instant_delay;
   Ecore_Timer         *instant_timer;
   Ecore_Timer         *autohide_timer;
   Ecore_Timer         *module_init_end_timer;
   Eina_List           *handlers;
   Ecore_Event_Handler *autohide;
   unsigned char        fit_along   : 1;
   unsigned char        fit_size    : 1;
   unsigned char        hidden      : 1;
   unsigned char        toggle      : 1;
   unsigned char        edge        : 1;
   unsigned char        urgent_show : 1;
   unsigned char        dummy : 1;
   Eina_Bool            cfg_delete : 1;
   unsigned int         locked;
};

typedef struct E_Event_Shelf
{
   E_Shelf *shelf;
} E_Event_Shelf;

typedef struct E_Event_Shelf E_Event_Shelf_Rename;
typedef struct E_Event_Shelf E_Event_Shelf_Add;
typedef struct E_Event_Shelf E_Event_Shelf_Del;

EAPI extern int E_EVENT_SHELF_RENAME;
EAPI extern int E_EVENT_SHELF_ADD;
EAPI extern int E_EVENT_SHELF_DEL;

EINTERN int              e_shelf_init(void);
EINTERN int              e_shelf_shutdown(void);
EAPI void             e_shelf_config_update(void);
EAPI E_Entry_Dialog *e_shelf_new_dialog(E_Zone *zone);
EAPI Eina_List       *e_shelf_list(void);
EAPI Eina_List       *e_shelf_list_all(void); // includes dummy shelves
EAPI E_Shelf         *e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, E_Layer layer, int id);
EAPI E_Shelf         *e_shelf_zone_dummy_new(E_Zone *zone, Evas_Object *obj, int id);
EAPI void             e_shelf_zone_move_resize_handle(E_Zone *zone);
EAPI void             e_shelf_populate(E_Shelf *es);
EAPI void             e_shelf_show(E_Shelf *es);
EAPI void             e_shelf_hide(E_Shelf *es);
EAPI void             e_shelf_locked_set(E_Shelf *es, int lock);
EAPI void             e_shelf_toggle(E_Shelf *es, int show);
EAPI void             e_shelf_urgent_show(E_Shelf *es);
EAPI void             e_shelf_move(E_Shelf *es, int x, int y);
EAPI void             e_shelf_resize(E_Shelf *es, int w, int h);
EAPI void             e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h);
EAPI void             e_shelf_layer_set(E_Shelf *es, E_Layer layer);
EAPI void             e_shelf_save(E_Shelf *es);
EAPI void             e_shelf_unsave(E_Shelf *es);
EAPI void             e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient);
EAPI const char      *e_shelf_orient_string_get(E_Shelf *es);
EAPI void             e_shelf_position_calc(E_Shelf *es);
EAPI void             e_shelf_style_set(E_Shelf *es, const char *style);
EAPI void             e_shelf_popup_set(E_Shelf *es, int popup);
EAPI E_Shelf         *e_shelf_config_new(E_Zone *zone, E_Config_Shelf *cf_es);
EAPI void             e_shelf_name_set(E_Shelf *es, const char *name);
EAPI void             e_shelf_rename_dialog(E_Shelf *es);
EAPI void             e_shelf_autohide_set(E_Shelf *es, int autohide_type);
EAPI Eina_Bool       e_shelf_desk_visible(E_Shelf *es, E_Desk *desk);
#endif
#endif

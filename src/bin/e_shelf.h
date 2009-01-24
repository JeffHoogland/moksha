/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Shelf E_Shelf;

#else
#ifndef E_SHELF_H
#define E_SHELF_H

#define E_SHELF_TYPE 0xE0b0101e

struct _E_Shelf
{
   E_Object             e_obj_inherit;
   int                  id;
   int                  x, y, w, h;
   int                  layer;
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
   E_Menu              *menu;
   Ecore_Timer         *hide_timer;
   Ecore_Animator      *hide_animator;
   int                  hide_step;
   int                  hidden_state_size;
   int                  hide_origin;
   int                  interrupted;
   float                instant_delay;
   Ecore_Timer         *instant_timer;
   Eina_List           *handlers;
   unsigned char        fit_along : 1;
   unsigned char        fit_size  : 1;
   unsigned char        hidden    : 1;
   unsigned char        toggle    : 1;
   unsigned char        edge      : 1;
   unsigned int         locked; 
};

EAPI int              e_shelf_init(void);
EAPI int              e_shelf_shutdown(void);
EAPI void             e_shelf_config_init(void);
EAPI Eina_List       *e_shelf_list(void);
EAPI E_Shelf         *e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, int layer, int id);
EAPI void             e_shelf_zone_move_resize_handle(E_Zone *zone);
EAPI void             e_shelf_populate(E_Shelf *es);
EAPI void             e_shelf_show(E_Shelf *es);
EAPI void             e_shelf_hide(E_Shelf *es);
EAPI void             e_shelf_locked_set(E_Shelf *es, int lock);
EAPI void             e_shelf_toggle(E_Shelf *es, int show);
EAPI void             e_shelf_move(E_Shelf *es, int x, int y);
EAPI void             e_shelf_resize(E_Shelf *es, int w, int h);
EAPI void             e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h);
EAPI void             e_shelf_layer_set(E_Shelf *es, int layer);
EAPI void             e_shelf_save(E_Shelf *es);
EAPI void             e_shelf_unsave(E_Shelf *es);
EAPI void             e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient);
EAPI void             e_shelf_position_calc(E_Shelf *es);
EAPI void             e_shelf_style_set(E_Shelf *es, const char *style);
EAPI void             e_shelf_popup_set(E_Shelf *es, int popup);
EAPI E_Shelf         *e_shelf_config_new(E_Zone *zone, E_Config_Shelf *cf_es);

#endif
#endif

#ifdef E_TYPEDEFS

typedef struct _E_Popup E_Popup;

#else
#ifndef E_POPUP_H
#define E_POPUP_H

#define E_POPUP_TYPE 0xE0b0100e

struct _E_Popup
{
   E_Object             e_obj_inherit;

   int                  x, y, w, h, zx, zy;
   E_Layer              layer;
   unsigned char        visible : 1;
   unsigned char        shaped : 1;
   unsigned char        need_shape_export : 1;

   Ecore_Evas          *ecore_evas;
   Evas                *evas;
   Ecore_X_Window       evas_win;
   E_Container_Shape   *shape;
   E_Zone              *zone;
   Eina_List           *objects;
   const char          *name;
   int                  shape_rects_num;
   Ecore_X_Rectangle   *shape_rects;
   Ecore_Idle_Enterer  *idle_enterer;
};

EINTERN int         e_popup_init(void);
EINTERN int         e_popup_shutdown(void);

EAPI E_Popup    *e_popup_new(E_Zone *zone, int x, int y, int w, int h);
EAPI void        e_popup_name_set(E_Popup *pop, const char *name);
EAPI void        e_popup_show(E_Popup *pop);
EAPI void        e_popup_hide(E_Popup *pop);
EAPI void        e_popup_move(E_Popup *pop, int x, int y);
EAPI void        e_popup_resize(E_Popup *pop, int w, int h);
EAPI void        e_popup_move_resize(E_Popup *pop, int x, int y, int w, int h);
EAPI void        e_popup_ignore_events_set(E_Popup *pop, int ignore);
EAPI void        e_popup_edje_bg_object_set(E_Popup *pop, Evas_Object *o);
EAPI void        e_popup_layer_set(E_Popup *pop, E_Layer layer);
EAPI void        e_popup_idler_before(void);
EAPI void        e_popup_object_add(E_Popup *pop, Evas_Object *obj);
EAPI void        e_popup_object_remove(E_Popup *pop, Evas_Object *obj);
EAPI E_Popup    *e_popup_find_by_window(Ecore_X_Window win);

#endif
#endif

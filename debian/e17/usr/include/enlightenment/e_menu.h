#ifdef E_TYPEDEFS

#define E_MENU_POP_DIRECTION_NONE  0
#define E_MENU_POP_DIRECTION_LEFT  1
#define E_MENU_POP_DIRECTION_RIGHT 2
#define E_MENU_POP_DIRECTION_UP    3
#define E_MENU_POP_DIRECTION_DOWN  4
#define E_MENU_POP_DIRECTION_AUTO  5
#define E_MENU_POP_DIRECTION_LAST  6

typedef struct _E_Menu         E_Menu;
typedef struct _E_Menu_Item    E_Menu_Item;
typedef struct _E_Menu_Category_Callback E_Menu_Category_Callback;


#else
#ifndef E_MENU_H
#define E_MENU_H

#define E_MENU_TYPE 0xE0b01009

#define E_MENU_ITEM_TYPE 0xE0b0100a

typedef void (*E_Menu_Cb) (void *data, E_Menu *m, E_Menu_Item *mi);


struct _E_Menu
{
   E_Object             e_obj_inherit;

   const char	       *category;

   struct {
      char              visible : 1;
      int               x, y, w, h;
   } cur, prev;

   int                  frozen;

   struct {
      const char       *title;
      const char       *icon_file;
      Evas_Object      *icon;
   } header;

   Eina_List           *items;

   /* the zone it belongs to */
   E_Zone              *zone;

   /* if a menu item spawned this menu, what item is it? */
   E_Menu_Item         *parent_item;

   /* only useful if realized != 0 (ie menu is ACTUALLY realized) */
   Ecore_Evas          *ecore_evas;
   Evas                *evas;
   Ecore_X_Window       evas_win;
   Evas_Object         *bg_object;
   Evas_Object         *container_object;
   Evas_Coord           container_x, container_y, container_w, container_h;
   E_Container_Shape   *shape;
   int                  shape_rects_num;
   Ecore_X_Rectangle   *shape_rects;

   struct {
      void *data;
      void (*func) (void *data, E_Menu *m);
   } pre_activate_cb, post_deactivate_cb;

   Eina_Bool        realized : 1; /* 1 if it is realized */
   Eina_Bool        active : 1; /* 1 if it is in active list */
   Eina_Bool        changed : 1;
   Eina_Bool        fast_mouse : 1;
   Eina_Bool        pending_new_submenu : 1;
   Eina_Bool        have_submenu : 1;
   Eina_Bool        in_active_list : 1;
   Eina_Bool        shaped : 1;
   Eina_Bool        need_shape_export : 1;
};

struct _E_Menu_Item
{
   E_Object       e_obj_inherit;
   E_Menu        *menu;
   const char    *icon;
   const char    *icon_key;
   const char    *label;
   E_Menu        *submenu;

   Evas_Object   *separator_object;

   Evas_Object   *bg_object;

   Evas_Object   *container_object;

   Evas_Object   *toggle_object;
   Evas_Object   *icon_bg_object;
   Evas_Object   *icon_object;
   Evas_Object   *label_object;
   Evas_Object   *submenu_object;

   Evas_Object   *event_object;

   Eina_List	 *list_position;

   int            label_w, label_h;
   int            icon_w, icon_h;
   int            separator_w, separator_h;
   int            submenu_w, submenu_h;
   int            toggle_w, toggle_h;
   int            radio_group;
   int            x, y, w, h;

   struct {
      void *data;
      E_Menu_Cb func;
   } cb; /* Callback for menu item activation */

   struct {
      void *data;
      E_Menu_Cb func;
   } realize_cb; /* Callback for menu item icon realization */

    struct {
      void *data;
      E_Menu_Cb func;
   } submenu_pre_cb;

    struct {
      void *data;
      E_Menu_Cb func;
   } submenu_post_cb;

   struct {
      void *data;
      E_Menu_Cb func;
   } drag_cb; /* Callback for menu item dragging */

   struct {
      int x, y;
   } drag;

   Eina_Bool  separator : 1;
   Eina_Bool  radio : 1;
   Eina_Bool  check : 1;
   Eina_Bool  toggle : 1;
   Eina_Bool  changed : 1;
   Eina_Bool  active : 1;
   Eina_Bool  disable : 1;
};

struct _E_Menu_Category_Callback
{
   const char *category;
   void *data;
   void (*create) (E_Menu *m, void *category_data, void *data);
   void (*free) (void *data);
};


EINTERN int          e_menu_init(void);
EINTERN int          e_menu_shutdown(void);

EAPI E_Menu      *e_menu_new(void);
EAPI void         e_menu_activate_key(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir);
EAPI void         e_menu_activate_mouse(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir, Ecore_X_Time activate_time);
EAPI void         e_menu_activate(E_Menu *m, E_Zone *zone, int x, int y, int w, int h, int dir);
EAPI void         e_menu_deactivate(E_Menu *m);
EAPI int          e_menu_freeze(E_Menu *m);
EAPI int          e_menu_thaw(E_Menu *m);
EAPI void         e_menu_title_set(E_Menu *m, const char *title);
EAPI void         e_menu_icon_file_set(E_Menu *m, const char *icon);

/* menu categories functions */
EAPI void         e_menu_category_set(E_Menu *m, const char *category);
EAPI void         e_menu_category_data_set(char *category, void *data);
EAPI E_Menu_Category_Callback  *e_menu_category_callback_add(char *category, void (*create) (E_Menu *m, void *category_data, void *data), void (free) (void *data), void *data);
EAPI void         e_menu_category_callback_del(E_Menu_Category_Callback *cb);


EAPI void         e_menu_pre_activate_callback_set(E_Menu *m,  void (*func) (void *data, E_Menu *m), void *data);
EAPI void         e_menu_post_deactivate_callback_set(E_Menu *m,  void (*func) (void *data, E_Menu *m), void *data);

EAPI E_Menu      *e_menu_root_get(E_Menu *m);

EAPI E_Menu_Item *e_menu_item_new(E_Menu *m);
EAPI E_Menu_Item *e_menu_item_new_relative(E_Menu *m, E_Menu_Item *rel);
EAPI E_Menu_Item *e_menu_item_nth(E_Menu *m, int n);
EAPI int          e_menu_item_num_get(const E_Menu_Item *mi);
EAPI void         e_menu_item_icon_file_set(E_Menu_Item *mi, const char *icon);
EAPI void         e_menu_item_icon_edje_set(E_Menu_Item *mi, const char *icon, const char *key);
EAPI void         e_menu_item_label_set(E_Menu_Item *mi, const char *label);
EAPI void         e_menu_item_submenu_set(E_Menu_Item *mi, E_Menu *sub);
EAPI void         e_menu_item_separator_set(E_Menu_Item *mi, int sep);
EAPI void         e_menu_item_check_set(E_Menu_Item *mi, int chk);
EAPI void         e_menu_item_radio_set(E_Menu_Item *mi, int rad);
EAPI void         e_menu_item_radio_group_set(E_Menu_Item *mi, int radg);
EAPI void         e_menu_item_toggle_set(E_Menu_Item *mi, int tog);
EAPI int          e_menu_item_toggle_get(E_Menu_Item *mi);
EAPI void         e_menu_item_callback_set(E_Menu_Item *mi,  E_Menu_Cb func, const void *data);
EAPI void         e_menu_item_realize_callback_set(E_Menu_Item *mi,  E_Menu_Cb func, void *data);
EAPI void         e_menu_item_submenu_pre_callback_set(E_Menu_Item *mi,  E_Menu_Cb func, const void *data);
EAPI void         e_menu_item_submenu_post_callback_set(E_Menu_Item *mi,  E_Menu_Cb func, const void *data);
EAPI void         e_menu_item_drag_callback_set(E_Menu_Item *mi,  E_Menu_Cb func, void *data);
EAPI E_Menu_Item *e_menu_item_active_get(void);
EAPI void         e_menu_active_item_activate(void);
EAPI void         e_menu_item_active_set(E_Menu_Item *mi, int active);
EAPI void         e_menu_item_disabled_set(E_Menu_Item *mi, int disable);

EAPI void         e_menu_idler_before(void);

EAPI Ecore_X_Window e_menu_grab_window_get(void);

EAPI E_Menu      *e_menu_find_by_window(Ecore_X_Window win);

#endif
#endif

#ifndef E_MENU_H
#define E_MENU_H

#include "e.h"
#include "object.h"

typedef struct _E_Menu                E_Menu;
typedef struct _E_Menu_Item           E_Menu_Item;

struct _E_Menu
{
   E_Object     o;
   
   struct {
      int       x, y, w, h;
      int       visible;
   } current, previous;
   struct {
      int l, r, t, b;
   } border, sel_border;
   struct {
      Window       main, evas;
   } win;
   Evas         evas;
   Ebits_Object bg;
   Evas_List    entries;
   char        *bg_file;
   
   int       first_expose;

   int       recalc_entries;
   int       redo_sel;
   int       changed;
   
   int       delete_me;
   
   struct {
      int state, icon, text;
   } size;
   struct {
      int icon, state;
   } pad;
   
   E_Menu_Item *selected;
   
   Time      time;

   void  (*func_hide) (E_Menu *m, void  *data);
   void  *func_hide_data;
};

struct _E_Menu_Item
{
   int x, y;
   struct {
      struct {
	 int w, h;
      } min;
      int w, h;
   } size;
   
   Ebits_Object  bg;
   char         *bg_file;
   int           selected;

   Evas_Object   obj_entry;
   
   char         *str;
   Evas_Object   obj_text;
   
   char         *icon;
   Evas_Object   obj_icon;
   int           scale_icon;
   
   Ebits_Object  state;
   char         *state_file;
   
   Ebits_Object  sep;
   char         *sep_file;
   
   int           separator;
   int           radio_group;
   int           radio;
   int           check;
   int           on;
   
   E_Menu       *menu;
   E_Menu       *submenu;
   
   void        (*func_select) (E_Menu *m, E_Menu_Item *mi, void *data);
   void         *func_select_data;
};


/**
 * e_menu_init - Menu event handling initalization.
 *
 * This function hooks in the necessary event handlers for
 * menu handling.
 */
void         e_menu_init(void );

void         e_menu_callback_item(E_Menu *m, E_Menu_Item *mi);
void         e_menu_item_set_callback(E_Menu_Item *mi, void  (*func) (E_Menu *m, E_Menu_Item *mi, void  *data), void  *data);
void         e_menu_hide_callback(E_Menu *m, void  (*func) (E_Menu *m, void  *data), void  *data);
void         e_menu_hide_submenus(E_Menu *menus_after);
void         e_menu_select(int dx, int dy);
void         e_menu_event_win_show(void );
void         e_menu_event_win_hide(void );
void         e_menu_set_background(E_Menu *m);
void         e_menu_set_sel(E_Menu *m, E_Menu_Item *mi);
void         e_menu_set_sep(E_Menu *m, E_Menu_Item *mi);
void         e_menu_set_state(E_Menu *m, E_Menu_Item *mi);
E_Menu      *e_menu_new(void );
void         e_menu_hide(E_Menu *m);
void         e_menu_show(E_Menu *m);
void         e_menu_move_to(E_Menu *m, int x, int y);
void         e_menu_show_at_mouse(E_Menu *m, int x, int y, Time t);
void         e_menu_add_item(E_Menu *m, E_Menu_Item *mi);
void         e_menu_del_item(E_Menu *m, E_Menu_Item *mi);
void         e_menu_item_update(E_Menu *m, E_Menu_Item *mi);
void         e_menu_item_unrealize(E_Menu *m, E_Menu_Item *mi);
void         e_menu_item_realize(E_Menu *m, E_Menu_Item *mi);
E_Menu_Item *e_menu_item_new(char *str);
void         e_menu_obscure_outside_screen(E_Menu *m);
void         e_menu_scroll_all_by(int dx, int dy);
void         e_menu_update_visibility(E_Menu *m);
void         e_menu_update_base(E_Menu *m);
void         e_menu_update_finish(E_Menu *m);
void         e_menu_update_shows(E_Menu *m);
void         e_menu_update_hides(E_Menu *m);
void         e_menu_update(E_Menu *m);
void         e_menu_item_set_icon(E_Menu_Item *mi, char *icon);
void         e_menu_item_set_text(E_Menu_Item *mi, char *text);
void         e_menu_item_set_separator(E_Menu_Item *mi, int sep);
void         e_menu_item_set_radio(E_Menu_Item *mi, int radio);
void         e_menu_item_set_check(E_Menu_Item *mi, int check);
void         e_menu_item_set_state(E_Menu_Item *mi, int state);
void         e_menu_item_set_submenu(E_Menu_Item *mi, E_Menu *submenu);
void         e_menu_item_set_scale_icon(E_Menu_Item *mi, int scale);
void         e_menu_set_padding_icon(E_Menu *m, int pad);
void         e_menu_set_padding_state(E_Menu *m, int pad);

#endif

#ifndef E_MOD_BORDER_H
#define E_MOD_BORDER_H

typedef enum _Illume_Anim_Class
{
   ILLUME_ANIM_APP,
     ILLUME_ANIM_KEYBOARD,
     ILLUME_ANIM_SHELF,
     ILLUME_ANIM_DIALOG,
     ILLUME_ANIM_OTHER
} Illume_Anim_Class;

void e_mod_border_activate(E_Border *bd);
void e_mod_border_show(E_Border *bd);
void e_mod_border_deactivate(E_Border *bd);

Eina_Bool e_mod_border_is_dialog(E_Border *bd);
Eina_Bool e_mod_border_is_keyboard(E_Border *bd);
Eina_Bool e_mod_border_is_bottom_panel(E_Border *bd);
Eina_Bool e_mod_border_is_top_shelf(E_Border *bd);
Eina_Bool e_mod_border_is_mini_app(E_Border *bd);
Eina_Bool e_mod_border_is_notification(E_Border *bd);
Eina_Bool e_mod_border_is_home(E_Border *bd);
Eina_Bool e_mod_border_is_side_pane_left(E_Border *bd);
Eina_Bool e_mod_border_is_side_pane_right(E_Border *bd);
Eina_Bool e_mod_border_is_overlay(E_Border *bd);
Eina_Bool e_mod_border_is_conformant(E_Border *bd);
Eina_Bool e_mod_border_is_quickpanel(E_Border *bd);

Eina_List *e_mod_border_valid_borders_get(E_Zone *zone);
E_Border *e_mod_border_valid_border_get(E_Zone *zone);
int e_mod_border_valid_count_get(E_Zone *zone);
E_Border *e_mod_border_at_xy_get(E_Zone *zone, int x, int y);
E_Border *e_mod_border_in_region_get(E_Zone *zone, int x, int y, int w, int h);
E_Border *e_mod_border_keyboard_get(E_Zone *zone);
E_Border *e_mod_border_top_shelf_get(E_Zone *zone);
E_Border *e_mod_border_bottom_panel_get(E_Zone *zone);
void e_mod_border_top_shelf_pos_get(E_Zone *zone, int *x, int *y);
void e_mod_border_top_shelf_size_get(E_Zone *zone, int *w, int *h);
void e_mod_border_bottom_panel_pos_get(E_Zone *zone, int *x, int *y);
void e_mod_border_bottom_panel_size_get(E_Zone *zone, int *w, int *h);

void e_mod_border_slide_to(E_Border *bd, int x, int y, Illume_Anim_Class aclass);
void e_mod_border_min_get(E_Border *bd, int *mw, int *mh);
void e_mod_border_max_get(E_Border *bd, int *mw, int *mh);

void e_mod_border_app1_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);
void e_mod_border_app2_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);

#endif

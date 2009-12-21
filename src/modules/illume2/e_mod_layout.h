#ifndef E_MOD_LAYOUT_H
#define E_MOD_LAYOUT_H

void e_mod_layout_init(E_Module *m);
void e_mod_layout_shutdown(void);

typedef enum _Illume_Anim_Class
{
   ILLUME_ANIM_APP,
     ILLUME_ANIM_KEYBOARD,
     ILLUME_ANIM_SHELF,
     ILLUME_ANIM_DIALOG,
     ILLUME_ANIM_OTHER
} Illume_Anim_Class;

typedef struct _Illume_Layout_Mode Illume_Layout_Mode;

struct _Illume_Layout_Mode
{
   const char *name;
   const char *label;
   // may need more things later, but name + label will do for now
   struct 
     {
        void (*border_add) (E_Border *bd);
        void (*border_del) (E_Border *bd);
        void (*border_focus_in) (E_Border *bd);
        void (*border_focus_out) (E_Border *bd);
        void (*zone_layout) (E_Zone *zone);
        void (*zone_move_resize) (E_Zone *zone);
        // --- add more below (activate callback, and more)
        void (*border_activate) (E_Border *bd);
        void (*drag_start) (E_Border *bd);
        void (*drag_end) (E_Border *bd);
     } funcs;
};

void illume_layout_mode_register(const Illume_Layout_Mode *laymode);
void illume_layout_mode_unregister(const Illume_Layout_Mode *laymode);
Eina_List *illume_layout_modes_get(void);

void illume_border_activate(E_Border *bd);
void illume_border_show(E_Border *bd);
void illume_border_deactivate(E_Border *bd);

Eina_Bool illume_border_is_dialog(E_Border *bd);
Eina_Bool illume_border_is_keyboard(E_Border *bd);
Eina_Bool illume_border_is_bottom_panel(E_Border *bd);
Eina_Bool illume_border_is_top_shelf(E_Border *bd);
Eina_Bool illume_border_is_mini_app(E_Border *bd);
Eina_Bool illume_border_is_notification(E_Border *bd);
Eina_Bool illume_border_is_home(E_Border *bd);
Eina_Bool illume_border_is_side_pane_left(E_Border *bd);
Eina_Bool illume_border_is_side_pane_right(E_Border *bd);
Eina_Bool illume_border_is_overlay(E_Border *bd);
Eina_Bool illume_border_is_conformant(E_Border *bd);

Eina_List *illume_border_valid_borders_get(E_Zone *zone);
E_Border *illume_border_valid_border_get(E_Zone *zone);
int illume_border_valid_count_get(E_Zone *zone);
E_Border *illume_border_at_xy_get(E_Zone *zone, int x, int y);
E_Border *illume_border_in_region_get(E_Zone *zone, int x, int y, int w, int h);
E_Border *illume_border_keyboard_get(E_Zone *zone);
E_Border *illume_border_top_shelf_get(E_Zone *zone);
E_Border *illume_border_bottom_panel_get(E_Zone *zone);
void illume_border_top_shelf_pos_get(E_Zone *zone, int *x, int *y);
void illume_border_top_shelf_size_get(E_Zone *zone, int *w, int *h);
void illume_border_bottom_panel_pos_get(E_Zone *zone, int *x, int *y);
void illume_border_bottom_panel_size_get(E_Zone *zone, int *w, int *h);

void illume_border_slide_to(E_Border *bd, int x, int y, Illume_Anim_Class aclass);
void illume_border_min_get(E_Border *bd, int *mw, int *mh);
void illume_border_max_get(E_Border *bd, int *mw, int *mh);

void illume_border_app1_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);
void illume_border_app2_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);

#endif

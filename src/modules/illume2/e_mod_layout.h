#ifndef E_MOD_LAYOUT_H
#define E_MOD_LAYOUT_H

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

void e_mod_layout_init(E_Module *m);
void e_mod_layout_shutdown(void);

void illume_layout_mode_register(const Illume_Layout_Mode *laymode);
void illume_layout_mode_unregister(const Illume_Layout_Mode *laymode);
Eina_List *illume_layout_modes_get(void);

#endif

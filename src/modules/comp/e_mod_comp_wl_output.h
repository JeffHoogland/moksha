#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_OUTPUT_H
#  define E_MOD_COMP_WL_OUTPUT_H

Eina_Bool e_mod_comp_wl_output_init(void);
Eina_Bool e_mod_comp_wl_output_shutdown(void);
void e_mod_comp_wl_output_damage(Wayland_Output *output);
Wayland_Output *e_mod_comp_wl_output_get(void);
void e_mod_comp_wl_output_idle_repaint(void *data);

# endif
#endif

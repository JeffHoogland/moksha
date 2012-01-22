#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_COMP_H
#  define E_MOD_COMP_WL_COMP_H

Eina_Bool e_mod_comp_wl_comp_init(void);
Eina_Bool e_mod_comp_wl_comp_shutdown(void);
Wayland_Compositor *e_mod_comp_wl_comp_get(void);
void e_mod_comp_wl_comp_repick(void);
Wayland_Surface *e_mod_comp_wl_comp_surface_pick(int32_t x, int32_t y, int32_t *sx, int32_t *sy);
void e_mod_comp_wl_comp_schedule_repaint(void);
void e_mod_comp_wl_comp_wake(void);

# endif
#endif

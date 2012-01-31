#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_COMP_H
#  define E_MOD_COMP_WL_COMP_H

Eina_Bool e_mod_comp_wl_comp_init(void);
void e_mod_comp_wl_comp_shutdown(void);
Wayland_Compositor *e_mod_comp_wl_comp_get(void);
void e_mod_comp_wl_comp_repick(struct wl_input_device *device, uint32_t timestamp);

# endif
#endif

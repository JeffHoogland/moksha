#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_INPUT_H
#  define E_MOD_COMP_WL_INPUT_H

Eina_Bool e_mod_comp_wl_input_init(void);
Eina_Bool e_mod_comp_wl_input_shutdown(void);
Wayland_Input *e_mod_comp_wl_input_get(void);
void e_mod_comp_wl_input_repick(struct wl_input_device *device, uint32_t timestamp);

# endif
#endif

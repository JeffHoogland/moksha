#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_COMP_H
#  define E_MOD_COMP_WL_COMP_H

Eina_Bool e_mod_comp_wl_comp_init(void);
void e_mod_comp_wl_comp_shutdown(void);
Wayland_Compositor *e_mod_comp_wl_comp_get(void);

# endif
#endif

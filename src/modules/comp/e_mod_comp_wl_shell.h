#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_SHELL_H
#  define E_MOD_COMP_WL_SHELL_H

Eina_Bool e_mod_comp_wl_shell_init(void);
void e_mod_comp_wl_shell_shutdown(void);
struct wl_shell *e_mod_comp_wl_shell_get(void);

# endif
#endif

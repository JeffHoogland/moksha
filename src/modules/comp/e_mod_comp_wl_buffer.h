#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_BUFFER_H
#  define E_MOD_COMP_WL_BUFFER_H

void e_mod_comp_wl_buffer_post_release(struct wl_buffer *buffer);
void e_mod_comp_wl_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface);

# endif
#endif

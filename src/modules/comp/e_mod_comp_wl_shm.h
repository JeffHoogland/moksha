#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_SHM_H
#  define E_MOD_COMP_WL_SHM_H

void e_mod_comp_wl_shm_buffer_created(struct wl_buffer *buffer);
void e_mod_comp_wl_shm_buffer_damaged(struct wl_buffer *buffer, int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t width __UNUSED__, int32_t height __UNUSED__);
void e_mod_comp_wl_shm_buffer_destroyed(struct wl_buffer *buffer);

# endif
#endif

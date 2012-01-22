#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_SURFACE_H
#  define E_MOD_COMP_WL_SURFACE_H

Wayland_Surface *e_mod_comp_wl_surface_create(int32_t x, int32_t y, int32_t width, int32_t height);
void e_mod_comp_wl_surface_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource);
void e_mod_comp_wl_surface_attach(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y);
void e_mod_comp_wl_surface_damage(struct wl_client *client __UNUSED__, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
void e_mod_comp_wl_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
void e_mod_comp_wl_surface_destroy_surface(struct wl_resource *resource);
void e_mod_comp_wl_surface_draw(Wayland_Surface *ws, Wayland_Output *output __UNUSED__, pixman_region32_t *clip);
void e_mod_comp_wl_surface_damage_surface(Wayland_Surface *ws);
void e_mod_comp_wl_surface_transform(Wayland_Surface *ws, int32_t x, int32_t y, int32_t *sx, int32_t *sy);
void e_mod_comp_wl_surface_configure(Wayland_Surface *ws, int32_t x, int32_t y, int32_t width, int32_t height);
void e_mod_comp_wl_surface_assign_output(Wayland_Surface *ws);
void e_mod_comp_wl_surface_damage_below(Wayland_Surface *ws);
void e_mod_comp_wl_surface_activate(Wayland_Surface *ws, Wayland_Input *device, uint32_t timestamp);
Eina_Bool e_mod_comp_wl_surface_move(Wayland_Surface *ws, Wayland_Input *wi, uint32_t timestamp);
Eina_Bool e_mod_comp_wl_surface_resize(Wayland_Shell_Surface *wss, Wayland_Input *wi, uint32_t timestamp, uint32_t edges);

# endif
#endif

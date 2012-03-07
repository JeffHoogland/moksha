#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_SURFACE_H
#  define E_MOD_COMP_WL_SURFACE_H

Wayland_Surface *e_mod_comp_wl_surface_create(int32_t x, int32_t y, int32_t w, int32_t h);
void e_mod_comp_wl_surface_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource);
void e_mod_comp_wl_surface_attach(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y);
void e_mod_comp_wl_surface_damage(struct wl_client *client __UNUSED__, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
void e_mod_comp_wl_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
void e_mod_comp_wl_surface_set_opaque_region(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *region_resource);
void e_mod_comp_wl_surface_set_input_region(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *region_resource);
void e_mod_comp_wl_surface_destroy_surface(struct wl_resource *resource);
void e_mod_comp_wl_surface_configure(Wayland_Surface *ws, int32_t x, int32_t y, int32_t width, int32_t height);
void e_mod_comp_wl_surface_activate(Wayland_Surface *ws, Wayland_Input *wi, uint32_t timestamp);
void e_mod_comp_wl_surface_damage_surface(Wayland_Surface *ws);

# endif
#endif

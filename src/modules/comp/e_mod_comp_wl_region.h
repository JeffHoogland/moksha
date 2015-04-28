#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_REGION_H
#  define E_MOD_COMP_WL_REGION_H

void e_mod_comp_wl_region_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource);
void e_mod_comp_wl_region_add(struct wl_client *client __UNUSED__, struct wl_resource *resource, int x, int y, int w, int h);
void e_mod_comp_wl_region_subtract(struct wl_client *client __UNUSED__, struct wl_resource *resource, int x, int y, int w, int h);

# endif
#endif

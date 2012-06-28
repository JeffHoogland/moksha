#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_region.h"
#endif

void
e_mod_comp_wl_region_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   wl_resource_destroy(resource);
}

void
e_mod_comp_wl_region_add(struct wl_client *client __UNUSED__, struct wl_resource *resource, int x, int y, int w, int h)
{
   Wayland_Region *region;

   region = resource->data;
   pixman_region32_union_rect(&region->region, &region->region, x, y, w, h);
}

void
e_mod_comp_wl_region_subtract(struct wl_client *client __UNUSED__, struct wl_resource *resource, int x, int y, int w, int h)
{
   Wayland_Region *region;
   pixman_region32_t rect;

   region = resource->data;
   pixman_region32_init_rect(&rect, x, y, w, h);
   pixman_region32_subtract(&region->region, &region->region, &rect);
   pixman_region32_fini(&rect);
}


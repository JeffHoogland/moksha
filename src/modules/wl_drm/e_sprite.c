#include "e.h"
#include "e_mod_main.h"

/* local function prototypes */
static void _e_sprite_cb_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__);
static void _e_sprite_cb_pending_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__);

/* local variables */
/* wayland interfaces */
/* external variables */

EINTERN E_Sprite *
e_sprite_create(E_Drm_Compositor *dcomp, drmModePlane *plane)
{
   E_Sprite *es;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!plane) return NULL;

   es = malloc(sizeof(E_Sprite) + ((sizeof(unsigned int)) * plane->count_formats));
   if (!es) return NULL;

   memset(es, 0, sizeof(E_Sprite));

   es->compositor = dcomp;
   es->possible_crtcs = plane->possible_crtcs;
   es->plane_id = plane->plane_id;
   es->surface = NULL;
   es->pending_surface = NULL;
   es->fb_id = 0;
   es->pending_fb_id = 0;
   es->destroy_listener.notify = _e_sprite_cb_buffer_destroy;
   es->pending_destroy_listener.notify = _e_sprite_cb_pending_buffer_destroy;
   es->format_count = plane->count_formats;
   memcpy(es->formats, plane->formats, 
          plane->count_formats * sizeof(plane->formats[0]));

   return es;
}

EINTERN Eina_Bool 
e_sprite_crtc_supported(E_Output *output, unsigned int supported)
{
   E_Compositor *comp;
   E_Drm_Compositor *dcomp;
   E_Drm_Output *doutput;
   int crtc = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = output->compositor;
   dcomp = (E_Drm_Compositor *)comp;
   doutput = (E_Drm_Output *)output;

   for (crtc = 0; crtc < dcomp->num_crtcs; crtc++)
     {
        if (dcomp->crtcs[crtc] != doutput->crtc_id)
          continue;
        if (supported & (1 << crtc)) 
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/* local functions */
static void 
_e_sprite_cb_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   E_Sprite *es;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   es = container_of(listener, E_Sprite, destroy_listener);
   es->surface = NULL;
}

static void 
_e_sprite_cb_pending_buffer_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   E_Sprite *es;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   es = container_of(listener, E_Sprite, pending_destroy_listener);
   es->pending_surface = NULL;
}

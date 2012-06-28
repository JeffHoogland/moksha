#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_output.h"
#endif

# define WL_OUTPUT_FLIPPED 0x01

/* local function prototypes */
static void _e_mod_comp_wl_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);

/* private variables */
static Wayland_Output *_wl_output;

Eina_Bool
e_mod_comp_wl_output_init(void)
{
   Ecore_X_Window *roots;
   int num = 0, rw, rh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
        EINA_LOG_ERR("Could not get root window list\n");
        return EINA_FALSE;
     }
   ecore_x_window_size_get(roots[0], &rw, &rh);
   free(roots);

   if (!(_wl_output = malloc(sizeof(Wayland_Output))))
     {
        EINA_LOG_ERR("Could not allocate space for output\n");
        return EINA_FALSE;
     }

   memset(_wl_output, 0, sizeof(*_wl_output));

   _wl_output->mode.flags =
     (WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED);
   _wl_output->mode.w = rw;
   _wl_output->mode.h = rh;
   _wl_output->mode.refresh = 60;

   _wl_output->x = 0;
   _wl_output->y = 0;
   _wl_output->w = rw;
   _wl_output->h = rh;
   _wl_output->flags = WL_OUTPUT_FLIPPED;

   wl_list_init(&_wl_output->link);
   wl_list_init(&_wl_output->frame_callbacks);

   if (!wl_display_add_global(_wl_disp, &wl_output_interface, _wl_output,
                              _e_mod_comp_wl_output_bind))
     {
        EINA_LOG_ERR("Failed to add output to wayland\n");
        free(_wl_output);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void
e_mod_comp_wl_output_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_wl_output) return;
   wl_list_remove(&_wl_output->frame_callbacks);
   wl_list_remove(&_wl_output->link);
   free(_wl_output);
}

Wayland_Output *
e_mod_comp_wl_output_get(void)
{
   return _wl_output;
}

/* local functions */
static void
_e_mod_comp_wl_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   Wayland_Output *output;
   struct wl_resource *resource;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(output = data)) return;
   resource =
     wl_client_add_object(client, &wl_output_interface, NULL, id, data);
   wl_resource_post_event(resource, WL_OUTPUT_GEOMETRY, output->x, output->y,
                          output->w, output->h, output->subpixel,
                          output->make, output->model);
   wl_resource_post_event(resource, WL_OUTPUT_MODE, output->mode.flags,
                          output->mode.w, output->mode.h, output->mode.refresh);
}


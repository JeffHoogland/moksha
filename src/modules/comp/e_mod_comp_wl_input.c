#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_input.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_input_unbind(struct wl_resource *resource);
static void _e_mod_comp_wl_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *buffer_resource __UNUSED__, int32_t x, int32_t y);

/* wayland interfaces */
static const struct wl_input_device_interface _wl_input_interface =
{
   _e_mod_comp_wl_input_attach,
};

/* private variables */
static Wayland_Input *_wl_input;

Eina_Bool
e_mod_comp_wl_input_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_input = malloc(sizeof(Wayland_Input))))
     {
        EINA_LOG_ERR("Could not allocate space for input\n");
        return EINA_FALSE;
     }

   memset(_wl_input, 0, sizeof(*_wl_input));

   wl_input_device_init(&_wl_input->input_device);
   if (!wl_display_add_global(_wl_disp, &wl_input_device_interface, _wl_input,
                              _e_mod_comp_wl_input_bind))
     {
        EINA_LOG_ERR("Failed to add input to wayland\n");
        free(_wl_input);
        return EINA_FALSE;
     }

   _wl_input->hotspot_x = 16;
   _wl_input->hotspot_y = 16;

   return EINA_TRUE;
}

void
e_mod_comp_wl_input_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_wl_input) return;
   wl_input_device_release(&_wl_input->input_device);
   free(_wl_input);
}

Wayland_Input *
e_mod_comp_wl_input_get(void)
{
   return _wl_input;
}

/* local functions */
static void
_e_mod_comp_wl_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   struct wl_input_device *device;
   struct wl_resource *resource;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   device = data;
   resource =
     wl_client_add_object(client, &wl_input_device_interface,
                          &_wl_input_interface, id, data);
   wl_list_insert(&device->resource_list, &resource->link);
   resource->destroy = _e_mod_comp_wl_input_unbind;
}

static void
_e_mod_comp_wl_input_unbind(struct wl_resource *resource)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_list_remove(&resource->link);
   free(resource);
}

static void
_e_mod_comp_wl_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *buffer_resource __UNUSED__, int32_t x, int32_t y)
{
   Wayland_Input *wi;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wi = resource->data;
   if (serial < wi->input_device.pointer_focus_serial) return;
   if (!wi->input_device.pointer_focus) return;
   if (wi->input_device.pointer_focus->resource.client != client) return;
   wi->hotspot_x = x;
   wi->hotspot_y = y;
}


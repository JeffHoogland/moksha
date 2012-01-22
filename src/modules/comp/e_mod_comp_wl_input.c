#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_surface.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_input_unbind(struct wl_resource *resource);
static void _e_mod_comp_wl_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t timestamp, struct wl_resource *buffer_resource, int32_t x, int32_t y);

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
   wl_display_add_global(_wl_disp, &wl_input_device_interface, _wl_input, 
                         _e_mod_comp_wl_input_bind);

   _wl_input->sprite = NULL;
   _wl_input->hotspot_x = 16;
   _wl_input->hotspot_y = 16;

   return EINA_TRUE;
}

Eina_Bool 
e_mod_comp_wl_input_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_wl_input) return EINA_TRUE;

   if (_wl_input->sprite)
     e_mod_comp_wl_surface_destroy_surface(&_wl_input->sprite->surface.resource);

   wl_input_device_release(&_wl_input->input_device);

   free(_wl_input);
   return EINA_TRUE;
}

Wayland_Input *
e_mod_comp_wl_input_get(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return _wl_input;
}

void 
e_mod_comp_wl_input_repick(struct wl_input_device *device, uint32_t timestamp)
{
   const struct wl_grab_interface *interface;
   Wayland_Surface *surface, *focus;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   surface = 
     e_mod_comp_wl_comp_surface_pick(device->x, device->y, 
                                     &device->current_x, &device->current_y);

   if (&surface->surface != device->current)
     {
        interface = device->grab->interface;
        interface->focus(device->grab, timestamp, &surface->surface, 
                         device->current_x, device->current_y);
        device->current = &surface->surface;
     }

   if ((focus = (Wayland_Surface *)device->grab->focus))
     {
        e_mod_comp_wl_surface_transform(focus, device->x, device->y, 
                                        &device->grab->x, &device->grab->y);
     }
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
_e_mod_comp_wl_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t timestamp, struct wl_resource *buffer_resource, int32_t x, int32_t y)
{
   Wayland_Input *device;
   struct wl_buffer *buffer;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   device = resource->data;
   if (timestamp < device->input_device.pointer_focus_time) return;
   if (!device->input_device.pointer_focus) return;
   if (device->input_device.pointer_focus->resource.client != client)
     return;

   if (device->sprite) 
     e_mod_comp_wl_surface_damage_below(device->sprite);

   if (!buffer_resource)
     {
        if (device->sprite)
          {
             e_mod_comp_wl_surface_destroy_surface(&device->sprite->surface.resource);
             device->sprite = NULL;
          }
        return;
     }

   if (!device->sprite)
     {
        device->sprite = 
          e_mod_comp_wl_surface_create(device->input_device.x, 
                                       device->input_device.y, 32, 32);
        wl_list_init(&device->sprite->link);
     }

   buffer = buffer_resource->data;

   e_mod_comp_wl_buffer_attach(buffer, &device->sprite->surface);

   device->hotspot_x = x;
   device->hotspot_y = y;
   device->sprite->width = buffer->width;
   device->sprite->height = buffer->height;
   device->sprite->x = device->input_device.x - device->hotspot_x;
   device->sprite->y = device->input_device.y - device->hotspot_y;

   e_mod_comp_wl_surface_damage_surface(device->sprite);
}

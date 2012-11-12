#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_input.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_input_unbind(struct wl_resource *resource);
static void _e_mod_comp_wl_input_pointer_get(struct wl_client *client, struct wl_resource *resource, unsigned int id);
static void _e_mod_comp_wl_input_keyboard_get(struct wl_client *client, struct wl_resource *resource, unsigned int id);
static void _e_mod_comp_wl_input_touch_get(struct wl_client *client, struct wl_resource *resource, unsigned int id);
static void _e_mod_comp_wl_input_pointer_cursor_set(struct wl_client *client, struct wl_resource *resource, unsigned int serial, struct wl_resource *surface_resource, int x, int y);

/* wayland interfaces */
static const struct wl_seat_interface _wl_seat_interface =
{
   _e_mod_comp_wl_input_pointer_get,
   _e_mod_comp_wl_input_keyboard_get,
   _e_mod_comp_wl_input_touch_get
};

static const struct wl_pointer_interface _wl_pointer_interface = 
{
   _e_mod_comp_wl_input_pointer_cursor_set
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

   wl_seat_init(&_wl_input->seat);
   if (!wl_display_add_global(_wl_disp, &wl_seat_interface, _wl_input,
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
   wl_seat_release(&_wl_input->seat);
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
   Wayland_Input *input;
   struct wl_seat *device;
   struct wl_resource *resource;
   enum wl_seat_capability caps = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   input = data;
   device = &input->seat;

   resource =
     wl_client_add_object(client, &wl_seat_interface,
                          &_wl_seat_interface, id, data);
   wl_list_insert(&device->base_resource_list, &resource->link);
   resource->destroy = _e_mod_comp_wl_input_unbind;

   if (device->pointer) caps |= WL_SEAT_CAPABILITY_POINTER;
   if (device->keyboard) caps |= WL_SEAT_CAPABILITY_KEYBOARD;
   if (device->touch) caps |= WL_SEAT_CAPABILITY_TOUCH;

   wl_seat_send_capabilities(resource, caps);
}

static void
_e_mod_comp_wl_input_unbind(struct wl_resource *resource)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_list_remove(&resource->link);
   free(resource);
}

static void 
_e_mod_comp_wl_input_pointer_get(struct wl_client *client, struct wl_resource *resource, unsigned int id)
{
   Wayland_Input *wi;
   struct wl_resource *res;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(wi = resource->data)) return;
   if (!wi->seat.pointer) return;

   res = wl_client_add_object(client, &wl_pointer_interface, 
                              &_wl_pointer_interface, id, wi);
   wl_list_insert(&wi->seat.pointer->resource_list, &res->link);
   res->destroy = _e_mod_comp_wl_input_unbind;

   if ((wi->seat.pointer->focus) && 
       (wi->seat.pointer->focus->resource.client == client))
     {
        /* TODO: surface_from_global_fixed ?? */

        wl_pointer_set_focus(wi->seat.pointer, wi->seat.pointer->focus, 
                             wi->seat.pointer->x, 
                             wi->seat.pointer->y);
     }
}

static void 
_e_mod_comp_wl_input_keyboard_get(struct wl_client *client, struct wl_resource *resource, unsigned int id)
{
   Wayland_Input *wi;
   struct wl_resource *res;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(wi = resource->data)) return;
   if (!wi->seat.keyboard) return;

   res = wl_client_add_object(client, &wl_keyboard_interface, NULL, id, wi);
   wl_list_insert(&wi->seat.keyboard->resource_list, &res->link);
   res->destroy = _e_mod_comp_wl_input_unbind;

   /* TODO: wl_keyboard_send_keymap ?? */

   if ((wi->seat.keyboard->focus) && 
       (wi->seat.keyboard->focus->resource.client == client))
     {
        wl_keyboard_set_focus(wi->seat.keyboard, wi->seat.keyboard->focus);
     }
}

static void 
_e_mod_comp_wl_input_touch_get(struct wl_client *client, struct wl_resource *resource, unsigned int id)
{
   Wayland_Input *wi;
   struct wl_resource *res;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(wi = resource->data)) return;
   if (!wi->seat.touch) return;

   res = wl_client_add_object(client, &wl_touch_interface, NULL, id, wi);
   wl_list_insert(&wi->seat.touch->resource_list, &res->link);
   res->destroy = _e_mod_comp_wl_input_unbind;
}

static void 
_e_mod_comp_wl_input_pointer_cursor_set(struct wl_client *client, struct wl_resource *resource, unsigned int serial, struct wl_resource *surface_resource, int x, int y)
{
   /* Wayland_Input *wi; */
   /* Wayland_Surface *ws; */

   /* if (!(wi = resource->data)) return; */
   /* if (!wi->seat.pointer->focus) return; */
   /* if (wi->seat.pointer->focus->resource.client != client) return; */

   /* if (surface_resource) ws = surface_resource->data; */

   /* if ((ws) && (ws != wi->sprite)) */
   /*   { */
   /*      if (ws->configure) */
   /*        { */
   /*           wl_resource_post_error(&ws->surface.resource,  */
   /*                                  WL_DISPLAY_ERROR_INVALID_OBJECT,  */
   /*                                  "Surface configure already set"); */
   /*           return; */
   /*        } */
   /*   } */

   /* TODO: Unmap sprite ? */

   /* if (!ws) return; */

   /* wl_signal_add(&ws->surface.resource.destroy_signal,  */
   /*               &wi->sprite_destroy_listener); */

   /* ws->configure = ; */
   /* ws->private = seat; */
   /* wi->sprite = ws; */
   /* wi->hotspot_x = x; */
   /* wi->hotspot_y = y; */

   /* if (ws->buffer) */
   /*   { */
        /* TODO: cursor surface configure */
     /* } */
}

/* static void */
/* _e_mod_comp_wl_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *buffer_resource __UNUSED__, int32_t x, int32_t y) */
/* { */
/*    Wayland_Input *wi; */

/*    LOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    wi = resource->data; */
/*    if (serial < wi->input_device.pointer_focus_serial) return; */
/*    if (!wi->input_device.pointer_focus) return; */
/*    if (wi->input_device.pointer_focus->resource.client != client) return; */
/*    wi->hotspot_x = x; */
/*    wi->hotspot_y = y; */
/* } */


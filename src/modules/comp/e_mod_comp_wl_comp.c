#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_surface.h"
# include "e_mod_comp_wl_region.h"
#endif

#ifdef __linux__
# include <linux/input.h>
#else
# define BTN_LEFT      0x110
# define BTN_RIGHT     0x111
# define BTN_MIDDLE    0x112
# define BTN_SIDE      0x113
# define BTN_EXTRA     0x114
# define BTN_FORWARD   0x115
# define BTN_BACK      0x116
#endif

#define MODIFIER_CTRL  (1 << 8)
#define MODIFIER_ALT   (1 << 9)
#define MODIFIER_SUPER (1 << 10)

/* local function prototypes */
static Eina_Bool        _e_mod_comp_wl_comp_egl_init(void);
static void             _e_mod_comp_wl_comp_egl_shutdown(void);
static void             _e_mod_comp_wl_comp_destroy(void);
static void             _e_mod_comp_wl_comp_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void             _e_mod_comp_wl_comp_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void             _e_mod_comp_wl_comp_region_create(struct wl_client *client, struct wl_resource *resource, unsigned int id);
static void             _e_mod_comp_wl_comp_region_destroy(struct wl_resource *resource);
static Eina_Bool        _e_mod_comp_wl_cb_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool        _e_mod_comp_wl_cb_mouse_in(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_mouse_out(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_mouse_down(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_mouse_up(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool        _e_mod_comp_wl_cb_key_up(void *data __UNUSED__, int type __UNUSED__, void *event);

static Wayland_Surface *_e_mod_comp_wl_comp_pick_surface(int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t *sx, int32_t *sy);
static void             _e_mod_comp_wl_comp_update_modifier(Wayland_Input *input, uint32_t key, uint32_t state);

/* wayland interfaces */
static const struct wl_compositor_interface _wl_comp_interface =
{
   _e_mod_comp_wl_comp_surface_create,
   _e_mod_comp_wl_comp_region_create
};
static const struct wl_surface_interface _wl_surface_interface =
{
   e_mod_comp_wl_surface_destroy,
   e_mod_comp_wl_surface_attach,
   e_mod_comp_wl_surface_damage,
   e_mod_comp_wl_surface_frame,
   e_mod_comp_wl_surface_set_opaque_region,
   e_mod_comp_wl_surface_set_input_region,
   e_mod_comp_wl_surface_commit
};
static const struct wl_region_interface _wl_region_interface =
{
   e_mod_comp_wl_region_destroy,
   e_mod_comp_wl_region_add,
   e_mod_comp_wl_region_subtract
};

/* private variables */
static Wayland_Compositor *_wl_comp;
static Eina_List *_wl_event_handlers = NULL;

Eina_Bool
e_mod_comp_wl_comp_init(void)
{
   const char *extensions;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_comp = malloc(sizeof(Wayland_Compositor))))
     {
        EINA_LOG_ERR("Could not allocate space for compositor\n");
        return EINA_FALSE;
     }

   memset(_wl_comp, 0, sizeof(*_wl_comp));

   if (!_e_mod_comp_wl_comp_egl_init())
     {
        EINA_LOG_ERR("Could not initialize egl\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->destroy = _e_mod_comp_wl_comp_destroy;

   if (!wl_display_add_global(_wl_disp, &wl_compositor_interface, _wl_comp,
                              _e_mod_comp_wl_comp_bind))
     {
        EINA_LOG_ERR("Failed to add compositor to wayland\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->image_target_texture_2d =
     (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");
   _wl_comp->image_target_renderbuffer_storage = (void *)
     eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
   _wl_comp->create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
   _wl_comp->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");

   _wl_comp->bind_display =
     (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
   _wl_comp->unbind_display =
     (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");

   extensions = (const char *)glGetString(GL_EXTENSIONS);
   if (!strstr(extensions, "GL_EXT_texture_format_BGRA8888"))
     {
        EINA_LOG_ERR("GL_EXT_texture_format_BGRA8888 not available\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   extensions =
     (const char *)eglQueryString(_wl_comp->egl.display, EGL_EXTENSIONS);
   if (strstr(extensions, "EGL_WL_bind_wayland_display"))
     _wl_comp->has_bind = EINA_TRUE;
   if (_wl_comp->has_bind)
     _wl_comp->bind_display(_wl_comp->egl.display, _wl_disp);

   wl_data_device_manager_init(_wl_disp);

   wl_list_init(&_wl_comp->surfaces);

   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN,
                                              _e_mod_comp_wl_cb_focus_in, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT,
                                              _e_mod_comp_wl_cb_focus_out, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN,
                                              _e_mod_comp_wl_cb_mouse_in, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT,
                                              _e_mod_comp_wl_cb_mouse_out, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                              _e_mod_comp_wl_cb_mouse_move, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                              _e_mod_comp_wl_cb_mouse_down, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                              _e_mod_comp_wl_cb_mouse_up, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                              _e_mod_comp_wl_cb_key_down, NULL));
   _wl_event_handlers =
     eina_list_append(_wl_event_handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_UP,
                                              _e_mod_comp_wl_cb_key_up, NULL));

   return EINA_TRUE;
}

void
e_mod_comp_wl_comp_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   E_FREE_LIST(_wl_event_handlers, ecore_event_handler_del);

   if (_wl_comp) _wl_comp->destroy();
}

Wayland_Compositor *
e_mod_comp_wl_comp_get(void)
{
   return _wl_comp;
}

void
e_mod_comp_wl_comp_repick(struct wl_seat *seat, uint32_t timestamp __UNUSED__)
{
   Wayland_Surface *ws, *focus;
   struct wl_pointer *pointer;

   if (!(pointer = seat->pointer)) return;

   ws =
     _e_mod_comp_wl_comp_pick_surface(pointer->x, pointer->y,
                                      &pointer->current_x, &pointer->current_y);
   if (!ws) return;
   if (&ws->surface != pointer->current)
     {
        const struct wl_pointer_grab_interface *interface;

        interface = pointer->grab->interface;
        pointer->current = &ws->surface;
        interface->focus(pointer->grab, &ws->surface,
                         pointer->current_x, pointer->current_y);
     }

   if ((focus = (Wayland_Surface *)pointer->grab->focus))
     {
        pointer->grab->x = pointer->x - focus->x;
        pointer->grab->y = pointer->y - focus->y;
     }
}

/* local functions */
static Eina_Bool
_e_mod_comp_wl_comp_egl_init(void)
{
   EGLint major, minor, n;
   EGLint config_attribs[] =
   {
      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 1, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE,
      EGL_WINDOW_BIT, EGL_NONE
   };
   EGLint context_attribs[] =
   {
      EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
   };

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_comp->egl.display = eglGetDisplay(NULL)))
     {
        EINA_LOG_ERR("Failed to create EGL display\n");
        return EINA_FALSE;
     }

   if (!eglInitialize(_wl_comp->egl.display, &major, &minor))
     {
        EINA_LOG_ERR("Failed to initialize EGL\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!eglBindAPI(EGL_OPENGL_ES_API))
     {
        EINA_LOG_ERR("Failed to bind EGL API\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if ((!eglChooseConfig(_wl_comp->egl.display, config_attribs,
                         &_wl_comp->egl.config, 1, &n) || (n == 0)))
     {
        EINA_LOG_ERR("Failed to choose EGL config\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!(_wl_comp->egl.context =
           eglCreateContext(_wl_comp->egl.display, _wl_comp->egl.config,
                            EGL_NO_CONTEXT, context_attribs)))
     {
        EINA_LOG_ERR("Failed to create EGL context\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!eglMakeCurrent(_wl_comp->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       _wl_comp->egl.context))
     {
        EINA_LOG_ERR("Failed to make EGL context current\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_mod_comp_wl_comp_egl_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   eglMakeCurrent(_wl_comp->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                  EGL_NO_CONTEXT);
   eglTerminate(_wl_comp->egl.display);
   eglReleaseThread();
}

static void
_e_mod_comp_wl_comp_destroy(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_comp->has_bind)
     _wl_comp->unbind_display(_wl_comp->egl.display, _wl_disp);

   _e_mod_comp_wl_comp_egl_shutdown();

   if (&_wl_comp->surfaces) wl_list_remove(&_wl_comp->surfaces);
   free(_wl_comp);
}

static void
_e_mod_comp_wl_comp_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_client_add_object(client, &wl_compositor_interface,
                        &_wl_comp_interface, id, data);
}

static void
_e_mod_comp_wl_comp_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ws = e_mod_comp_wl_surface_create(0, 0, 0, 0)))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   ws->surface.resource.destroy = e_mod_comp_wl_surface_destroy_surface;
   ws->surface.resource.object.id = id;
   ws->surface.resource.object.interface = &wl_surface_interface;
   ws->surface.resource.object.implementation =
     (void (* *)(void)) & _wl_surface_interface;
   ws->surface.resource.data = ws;

   wl_client_add_resource(client, &ws->surface.resource);
}

static void
_e_mod_comp_wl_comp_region_create(struct wl_client *client, struct wl_resource *resource, unsigned int id)
{
   Wayland_Region *region;

   region = malloc(sizeof(*region));
   if (!region)
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   region->resource.destroy = _e_mod_comp_wl_comp_region_destroy;
   region->resource.object.id = id;
   region->resource.object.interface = &wl_region_interface;
   region->resource.object.implementation =
     (void (* *)(void)) & _wl_region_interface;
   region->resource.data = region;

   pixman_region32_init(&region->region);
   wl_client_add_resource(client, &region->resource);
}

static void
_e_mod_comp_wl_comp_region_destroy(struct wl_resource *resource)
{
   Wayland_Region *region;

   region = container_of(resource, Wayland_Region, resource);
   pixman_region32_fini(&region->region);
   free(region);
}

static Eina_Bool
_e_mod_comp_wl_cb_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Wayland_Surface *ws;
   Ecore_X_Event_Window_Focus_In *ev;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;

   input = e_mod_comp_wl_input_get();
   wl_list_for_each(ws, &_wl_comp->surfaces, link)
   {
      if (((ws->win) && (ws->win->border))
          && (ws->win->border->win == ev->win))
        {
           wl_keyboard_set_focus(input->seat.keyboard, &ws->surface);
           wl_data_device_set_keyboard_focus(&input->seat);
           break;
        }
   }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Wayland_Input *input;
   /* Ecore_X_Event_Window_Focus_Out *ev; */

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* ev = event; */
   input = e_mod_comp_wl_input_get();
   if ((!input) || (!input->seat.keyboard)) 
     return ECORE_CALLBACK_PASS_ON;
   wl_keyboard_set_focus(input->seat.keyboard, NULL);
   wl_data_device_set_keyboard_focus(&input->seat);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_mouse_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Ecore_X_Event_Mouse_In *ev;
   uint32_t timestamp;
   struct wl_pointer *ptr;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;
   input = e_mod_comp_wl_input_get();

   ptr = input->seat.pointer;
   ptr->x = ev->x;
   ptr->y = ev->y;

   timestamp = e_mod_comp_wl_time_get();
   e_mod_comp_wl_comp_repick(&input->seat, timestamp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_mouse_out(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Ecore_X_Event_Mouse_Out *ev;
   uint32_t timestamp;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;
   input = e_mod_comp_wl_input_get();

   timestamp = e_mod_comp_wl_time_get();
   e_mod_comp_wl_comp_repick(&input->seat, timestamp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Ecore_Event_Mouse_Move *ev;
   const struct wl_pointer_grab_interface *interface;
   uint32_t timestamp;
   struct wl_pointer *ptr;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;

   input = e_mod_comp_wl_input_get();

   ptr = input->seat.pointer;
   ptr->x = ev->x;
   ptr->y = ev->y;

   timestamp = e_mod_comp_wl_time_get();
   e_mod_comp_wl_comp_repick(&input->seat, timestamp);

   interface = ptr->grab->interface;
   interface->motion(ptr->grab, timestamp, ptr->grab->x, ptr->grab->y);

   /* TODO: handle sprite */

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_mouse_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Ecore_Event_Mouse_Button *ev;
   struct wl_pointer *ptr;
   int btn = 0;
   uint32_t timestamp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;

   switch (ev->buttons)
     {
      case 1:
        btn = ev->buttons + BTN_LEFT - 1;
        break;

      case 2:
        btn = BTN_MIDDLE;
        break;

      case 3:
        btn = BTN_RIGHT;
        break;
     }

   input = e_mod_comp_wl_input_get();
   ptr = input->seat.pointer;
   timestamp = e_mod_comp_wl_time_get();
   if (ptr->button_count == 0)
     {
        ptr->grab_button = btn;
        ptr->grab_time = timestamp;
        ptr->grab_x = ptr->x;
        ptr->grab_y = ptr->y;
     }

   ptr->button_count++;

   /* TODO: Run binding ?? */

   ptr->grab->interface->button(ptr->grab, timestamp, btn, 1);

   if (ptr->button_count == 1)
     ptr->grab_serial = wl_display_get_serial(_wl_disp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_mouse_up(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Wayland_Input *input;
   Ecore_Event_Mouse_Button *ev;
   struct wl_pointer *ptr;
   int btn = 0;
   uint32_t timestamp;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON;

   switch (ev->buttons)
     {
      case 1:
        btn = ev->buttons + BTN_LEFT - 1;
        break;

      case 2:
        btn = BTN_MIDDLE;
        break;

      case 3:
        btn = BTN_RIGHT;
        break;
     }

   input = e_mod_comp_wl_input_get();
   ptr = input->seat.pointer;
   ptr->button_count--;

   /* TODO: Run binding ?? */
   timestamp = e_mod_comp_wl_time_get();
   ptr->grab->interface->button(ptr->grab, timestamp, btn, 0);

   if (ptr->button_count == 1)
     ptr->grab_serial = wl_display_get_serial(_wl_disp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   /* Wayland_Input *input; */
   /* Ecore_Event_Key *ev; */
   /* struct wl_keyboard *kbd; */
   /* uint32_t *k, *end, key = 0; */
   /* uint32_t timestamp; */

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* ev = event; */
   /* if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON; */

   /* input = e_mod_comp_wl_input_get(); */
   /* kbd = input->seat.keyboard; */
   /* timestamp = e_mod_comp_wl_time_get(); */

   /* key = ecore_x_keysym_keycode_get(ev->key); */

   /* input->modifier_state = 0; */
   /* _e_mod_comp_wl_comp_update_modifier(input, key, 1); */

   /* end = kbd->keys.data + kbd->keys.size; */
   /* for (k = kbd->keys.data; k < end; k++) */
   /*   if (*k == key) *k = *--end; */
   /* kbd->keys.size = (void *)end - kbd->keys.data; */

   /* k = wl_array_add(&kbd->keys, sizeof(*k)); */
   /* *k = key; */

   /* if (kbd->keyboard_focus_resource) */
   /*   wl_resource_post_event(device->keyboard_focus_resource, */
   /*                          WL_INPUT_DEVICE_KEY, timestamp, key, 1); */

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_wl_cb_key_up(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   /* Wayland_Input *input; */
   /* Ecore_Event_Key *ev; */
   /* struct wl_input_device *device; */
   /* uint32_t *k, *end, key = 0; */
   /* uint32_t timestamp; */

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* ev = event; */
   /* if (wl_list_empty(&_wl_comp->surfaces)) return ECORE_CALLBACK_PASS_ON; */

   /* input = e_mod_comp_wl_input_get(); */
   /* device = &input->input_device; */
   /* timestamp = e_mod_comp_wl_time_get(); */

   /* key = ecore_x_keysym_keycode_get(ev->key); */

   /* _e_mod_comp_wl_comp_update_modifier(input, key, 0); */

   /* end = device->keys.data + device->keys.size; */
   /* for (k = device->keys.data; k < end; k++) */
   /*   if (*k == key) *k = *--end; */
   /* device->keys.size = (void *)end - device->keys.data; */

   /* k = wl_array_add(&device->keys, sizeof(*k)); */
   /* *k = ev->key; */

   /* if (device->keyboard_focus_resource) */
   /*   wl_resource_post_event(device->keyboard_focus_resource, */
   /*                          WL_INPUT_DEVICE_KEY, timestamp, key, 0); */

   return ECORE_CALLBACK_PASS_ON;
}

static Wayland_Surface *
_e_mod_comp_wl_comp_pick_surface(int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t *sx, int32_t *sy)
{
   Wayland_Surface *ws;

   if (wl_list_empty(&_wl_comp->surfaces)) return NULL;
   wl_list_for_each(ws, &_wl_comp->surfaces, link)
   {
      if (ws->surface.resource.client == NULL) continue;
      if ((0 <= *sx) && (*sx < ws->w) &&
          (0 <= *sy) && (*sy < ws->h))
        return ws;
   }

   return NULL;
}

static void
_e_mod_comp_wl_comp_update_modifier(Wayland_Input *input, uint32_t key, uint32_t state)
{
   uint32_t mod;

   switch (key)
     {
      case KEY_LEFTCTRL:
      case KEY_RIGHTCTRL:
        mod = MODIFIER_CTRL;
        break;

      case KEY_LEFTALT:
      case KEY_RIGHTALT:
        mod = MODIFIER_ALT;
        break;

      case KEY_LEFTMETA:
      case KEY_RIGHTMETA:
        mod = MODIFIER_SUPER;
        break;

      default:
        mod = 0;
        break;
     }

   if (state)
     input->modifier_state |= mod;
   else
     input->modifier_state &= ~mod;
}


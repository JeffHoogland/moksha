#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_surface.h"
# include "e_mod_comp_wl_shell.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_shell_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_shell_lock(Wayland_Shell *base);
static void _e_mod_comp_wl_shell_unlock(Wayland_Shell *base);
static void _e_mod_comp_wl_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height);
static void _e_mod_comp_wl_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static void _e_mod_comp_wl_shell_destroy(Wayland_Shell *base);

static void _e_mod_comp_wl_shell_surface_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp);

static Wayland_Shell_Surface *_e_mod_comp_wl_shell_get_shell_surface(Wayland_Surface *surface);
static Wayland_Shell_Surface_Type _e_mod_comp_wl_shell_get_shell_surface_type(Wayland_Surface *surface);
static void _e_mod_comp_wl_shell_center_on_output(Wayland_Surface *ws);
static void _e_mod_comp_wl_shell_activate(Wayland_Shell *base, Wayland_Surface *ws, Wayland_Input *device, uint32_t timestamp);

static void _e_mod_comp_wl_shell_shell_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource);
static void _e_mod_comp_wl_shell_surface_destroy(struct wl_resource *resource);
static void _e_mod_comp_wl_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp);
static void _e_mod_comp_wl_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges);
static void _e_mod_comp_wl_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _e_mod_comp_wl_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags __UNUSED__);
static void _e_mod_comp_wl_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _e_mod_comp_wl_shell_surface_set_popup(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource  __UNUSED__, uint32_t timestamp  __UNUSED__, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags  __UNUSED__);
static Eina_Bool _e_mod_comp_wl_shell_surface_type_reset(Wayland_Shell_Surface *wss);

/* wayland interfaces */
static const struct wl_shell_interface _wl_shell_interface = 
{
   _e_mod_comp_wl_shell_shell_surface_get
};
static const struct wl_shell_surface_interface _wl_shell_surface_interface = 
{
   _e_mod_comp_wl_shell_surface_move,
   _e_mod_comp_wl_shell_surface_resize,
   _e_mod_comp_wl_shell_surface_set_toplevel,
   _e_mod_comp_wl_shell_surface_set_transient,
   _e_mod_comp_wl_shell_surface_set_fullscreen,
   _e_mod_comp_wl_shell_surface_set_popup
};

/* private variables */
struct wl_shell *_wl_shell;

Eina_Bool 
e_mod_comp_wl_shell_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_shell = malloc(sizeof(*_wl_shell))))
     {
        EINA_LOG_ERR("Cannot allocate space for shell\n");
        return EINA_FALSE;
     }

   memset(_wl_shell, 0, sizeof(*_wl_shell));

   _wl_shell->shell.lock = _e_mod_comp_wl_shell_lock;
   _wl_shell->shell.unlock = _e_mod_comp_wl_shell_unlock;
   _wl_shell->shell.map = _e_mod_comp_wl_shell_map;
   _wl_shell->shell.configure = _e_mod_comp_wl_shell_configure;
   _wl_shell->shell.destroy = _e_mod_comp_wl_shell_destroy;

   wl_list_init(&_wl_shell->hidden_surfaces);

   /* FIXME: Shell configuration for screensaver ? */

   if (!wl_display_add_global(_wl_disp, &wl_shell_interface, _wl_shell, 
                             _e_mod_comp_wl_shell_bind))
     {
        EINA_LOG_ERR("Could not create shell\n");
        free(_wl_shell);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool 
e_mod_comp_wl_shell_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _wl_shell->shell.destroy(&_wl_shell->shell);
   return EINA_TRUE;
}

Wayland_Shell *
e_mod_comp_wl_shell_get(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return &_wl_shell->shell;
}

/* local functions */
static void 
_e_mod_comp_wl_shell_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = data;
   wl_client_add_object(client, &wl_shell_interface, &_wl_shell_interface, 
                        id, shell);
}

static void 
_e_mod_comp_wl_shell_lock(Wayland_Shell *base)
{
   Wayland_Compositor *comp;
   Wayland_Surface *cur, *tmp;
   Wayland_Input *input;
   struct wl_shell *shell;
   uint32_t timestamp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   shell = container_of(base, struct wl_shell, shell);

   if (shell->locked) return;
   shell->locked = EINA_TRUE;

   if (!wl_list_empty(&shell->hidden_surfaces))
     EINA_LOG_ERR("Shell Hidden Surface list is not empty\n");

   wl_list_for_each_safe(cur, tmp, &comp->surfaces, link)
     {
        if (!cur->surface.resource.client) continue;

        if (_e_mod_comp_wl_shell_get_shell_surface_type(cur) == 
            SHELL_SURFACE_BACKGROUND) 
          continue;

        wl_list_remove(&cur->link);
        wl_list_insert(shell->hidden_surfaces.prev, &cur->link);
     }

   e_mod_comp_wl_comp_repick();

   /* reset keyboard focus */
   input = e_mod_comp_wl_input_get();
   timestamp = e_mod_comp_wl_time_get();
   wl_input_device_set_keyboard_focus(&input->input_device, NULL, timestamp);
}

static void 
_e_mod_comp_wl_shell_unlock(Wayland_Shell *base)
{
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
   if ((!shell->locked) || (shell->lock_surface)) 
     {
        e_mod_comp_wl_comp_wake();
        return;
     }

   if (!shell->child.desktop_shell)
     {
        /* TODO: Resume desktop */
        return;
     }

   if (shell->prepare_event_sent) return;
   wl_resource_post_event(shell->child.desktop_shell, 1);
   shell->prepare_event_sent = EINA_TRUE;
}

static void 
_e_mod_comp_wl_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height)
{
   struct wl_shell *shell;
   struct wl_list *list;
   Wayland_Compositor *comp;
   Wayland_Shell_Surface *shsurf;
   Wayland_Shell_Surface_Type type;
   Eina_Bool do_configure = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   type = SHELL_SURFACE_NONE;
   comp = e_mod_comp_wl_comp_get();
   shell = container_of(base, struct wl_shell, shell);

   if ((shsurf = _e_mod_comp_wl_shell_get_shell_surface(surface)))
     type = shsurf->type;
   if (shell->locked)
     {
        list = &shell->hidden_surfaces;
        do_configure = EINA_FALSE;
     }
   else 
     {
        list = &comp->surfaces;
        do_configure = EINA_TRUE;
     }

   surface->width = width;
   surface->height = height;

   /* initial position */
   switch (type)
     {
      case SHELL_SURFACE_TOPLEVEL:
        surface->x = 10 + random() % 400;
        surface->y = 10 + random() % 400;
        break;
      case SHELL_SURFACE_SCREENSAVER:
      case SHELL_SURFACE_FULLSCREEN:
        _e_mod_comp_wl_shell_center_on_output(surface);
        break;
      case SHELL_SURFACE_LOCK:
        _e_mod_comp_wl_shell_center_on_output(surface);
        break;
      default:
        break;
     }

   /* surface stacking */
   switch (type)
     {
      case SHELL_SURFACE_BACKGROUND:
        wl_list_insert(comp->surfaces.prev, &surface->link);
        do_configure = EINA_TRUE;
        break;
      case SHELL_SURFACE_PANEL:
        wl_list_insert(list, &surface->link);
        break;
      case SHELL_SURFACE_LOCK:
        wl_list_insert(&comp->surfaces, &surface->link);
        e_mod_comp_wl_comp_repick();
        e_mod_comp_wl_comp_wake();
        do_configure = EINA_TRUE;
        break;
      case SHELL_SURFACE_SCREENSAVER:
        /* TODO: Handle show of screensaver */
        do_configure = EINA_FALSE;
        break;
      default:
        /* skip panels */
        wl_list_insert(list, &surface->link);
        break;
     }

   switch (type)
     {
      case SHELL_SURFACE_TOPLEVEL:
        surface->x = 10 + random() % 400;
        surface->y = 10 + random() % 400;
        break;
      case SHELL_SURFACE_POPUP:
        /* TODO: handle popup map */
        break;
      default:
        break;
     }

   surface->width = width;
   surface->height = height;
   if (do_configure)
     {
        e_mod_comp_wl_surface_configure(surface, surface->x, surface->y, 
                                       width, height);
        e_mod_comp_wl_comp_repick();
     }

   switch (type) 
     {
      case SHELL_SURFACE_TOPLEVEL:
      case SHELL_SURFACE_TRANSIENT:
      case SHELL_SURFACE_FULLSCREEN:
        if (!shell->locked) 
          _e_mod_comp_wl_shell_activate(base, surface, 
                                        e_mod_comp_wl_input_get(), 
                                        e_mod_comp_wl_time_get());
        break;
      default:
        break;
     }

   /* NB: Maybe handle zoom ?? */
}

static void 
_e_mod_comp_wl_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
   struct wl_shell *shell;
   Wayland_Shell_Surface *shsurf;
   Wayland_Shell_Surface_Type type;
   Eina_Bool do_configure = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   type = SHELL_SURFACE_NONE;
   shell = container_of(base, struct wl_shell, shell);
   do_configure = !shell->locked;
   if ((shsurf = _e_mod_comp_wl_shell_get_shell_surface(surface)))
     type = shsurf->type;

   surface->width = width;
   surface->height = height;

   switch (type) 
     {
      case SHELL_SURFACE_SCREENSAVER:
        do_configure = !do_configure;
      case SHELL_SURFACE_FULLSCREEN:
        _e_mod_comp_wl_shell_center_on_output(surface);
        break;
      default:
        break;
     }

   if (do_configure) 
     e_mod_comp_wl_surface_configure(surface, x, y, width, height);
}

static void 
_e_mod_comp_wl_shell_destroy(Wayland_Shell *base)
{
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
   if (shell->child.client) wl_client_destroy(shell->child.client);
   free(shell);
}

static void 
_e_mod_comp_wl_shell_surface_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp)
{
   Wayland_Shell_Surface *shsurf;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shsurf = container_of(listener, Wayland_Shell_Surface, surface_destroy_listener);
   shsurf->surface = NULL;
   wl_resource_destroy(&shsurf->resource, timestamp);
}

static Wayland_Shell_Surface *
_e_mod_comp_wl_shell_get_shell_surface(Wayland_Surface *surface)
{
   struct wl_list *lst;
   struct wl_listener *listener;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   lst = &surface->surface.resource.destroy_listener_list;
   wl_list_for_each(listener, lst, link)
     {
        if (listener->func == _e_mod_comp_wl_shell_surface_destroy_handle)
          return container_of(listener, Wayland_Shell_Surface, 
                              surface_destroy_listener);
     }

   return NULL;
}

static Wayland_Shell_Surface_Type 
_e_mod_comp_wl_shell_get_shell_surface_type(Wayland_Surface *surface)
{
   Wayland_Shell_Surface *shsurf;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(shsurf = _e_mod_comp_wl_shell_get_shell_surface(surface)))
     return SHELL_SURFACE_NONE;

   return shsurf->type;
}

static void 
_e_mod_comp_wl_shell_center_on_output(Wayland_Surface *ws)
{
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = e_mod_comp_wl_output_get();
   ws->x = output->x + (output->mode.width - ws->width) / 2;
   ws->y = output->y + (output->mode.height - ws->height) / 2;
}

static void 
_e_mod_comp_wl_shell_activate(Wayland_Shell *base, Wayland_Surface *ws, Wayland_Input *device, uint32_t timestamp)
{
   Wayland_Compositor *comp;
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   shell = container_of(base, struct wl_shell, shell);

   e_mod_comp_wl_surface_activate(ws, device, timestamp);

   switch (_e_mod_comp_wl_shell_get_shell_surface_type(ws))
     {
      case SHELL_SURFACE_BACKGROUND:
        wl_list_remove(&ws->link);
        wl_list_insert(comp->surfaces.prev, &ws->link);
        break;
      case SHELL_SURFACE_PANEL:
        break;
      case SHELL_SURFACE_SCREENSAVER:
        break;
      default:
        if (!shell->locked) 
          {
             /* TODO: bring panel on top */
          }
        break;
     }
}

static void 
_e_mod_comp_wl_shell_shell_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
   Wayland_Surface *ws;
   Wayland_Shell_Surface *shsurf;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = surface_resource->data;
   if (_e_mod_comp_wl_shell_get_shell_surface(ws))
     {
        wl_resource_post_error(surface_resource, 
                               WL_DISPLAY_ERROR_INVALID_OBJECT, 
                               "get_shell_surface already requested");
        return;
     }

   if (!(shsurf = calloc(1, sizeof(*shsurf))))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   shsurf->resource.destroy = _e_mod_comp_wl_shell_surface_destroy;
   shsurf->resource.object.id = id;
   shsurf->resource.object.interface = &wl_shell_surface_interface;
   shsurf->resource.object.implementation = 
     (void (**)(void)) &_wl_shell_surface_interface;
   shsurf->resource.data = shsurf;

   shsurf->surface = ws;
   shsurf->surface_destroy_listener.func = 
     _e_mod_comp_wl_shell_surface_destroy_handle;
   wl_list_insert(ws->surface.resource.destroy_listener_list.prev, 
                  &shsurf->surface_destroy_listener.link);

   wl_list_init(&shsurf->link);

   shsurf->type = SHELL_SURFACE_NONE;
   wl_client_add_resource(client, &shsurf->resource);
}

static void 
_e_mod_comp_wl_shell_surface_destroy(struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   /* TODO: popup grab input */
   if (wss->surface)
     wl_list_remove(&wss->surface_destroy_listener.link);
   wl_list_remove(&wss->link);
   free(wss);
}

static void 
_e_mod_comp_wl_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp)
{
   Wayland_Input *wi;
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wi = input_resource->data;
   wss = resource->data;

   if ((!wi->input_device.button_count) || 
       (wi->input_device.grab_time != timestamp) || 
       (wi->input_device.pointer_focus != &wss->surface->surface))
     return;

   if (!e_mod_comp_wl_surface_move(wss->surface, wi, timestamp))
     wl_resource_post_no_memory(resource);
}

static void 
_e_mod_comp_wl_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges)
{
   Wayland_Input *wi;
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wi = input_resource->data;
   wss = resource->data;

   if ((!wi->input_device.button_count) || 
       (wi->input_device.grab_time != timestamp) || 
       (wi->input_device.pointer_focus != &wss->surface->surface))
     return;

   if (!e_mod_comp_wl_surface_resize(wss, wi, timestamp, edges))
     wl_resource_post_no_memory(resource);
}

static void 
_e_mod_comp_wl_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   if (!_e_mod_comp_wl_shell_surface_type_reset(wss)) return;
   e_mod_comp_wl_surface_damage_surface(wss->surface);
   wss->type = SHELL_SURFACE_TOPLEVEL;
}

static void 
_e_mod_comp_wl_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags __UNUSED__)
{
   Wayland_Shell_Surface *wss, *pss;
   Wayland_Surface *ws, *pws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   pss = parent_resource->data;
   ws = wss->surface;
   pws = pss->surface;

   if (!_e_mod_comp_wl_shell_surface_type_reset(wss)) return;
   ws->x = pws->x + x;
   ws->y = pws->y + y;
   e_mod_comp_wl_surface_damage_surface(ws);
   wss->type = SHELL_SURFACE_TRANSIENT;
}

static void 
_e_mod_comp_wl_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;
   Wayland_Surface *ws;
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   ws = wss->surface;
   if (!_e_mod_comp_wl_shell_surface_type_reset(wss)) return;

   output = e_mod_comp_wl_output_get();
   wss->saved_x = ws->x;
   wss->saved_y = ws->y;
   ws->x = (output->width - ws->width) / 2;
   ws->y = (output->height - ws->height) / 2;
   e_mod_comp_wl_surface_damage_surface(ws);
   wss->type = SHELL_SURFACE_FULLSCREEN;
}

static void 
_e_mod_comp_wl_shell_surface_set_popup(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource  __UNUSED__, uint32_t timestamp  __UNUSED__, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags  __UNUSED__)
{
   Wayland_Shell_Surface *wss;
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   ws = wss->surface;
   e_mod_comp_wl_surface_damage_surface(ws);
   wss->type = SHELL_SURFACE_POPUP;
   wss->parent = parent_resource->data;
   wss->popup.x = x;
   wss->popup.y = y;
}

static Eina_Bool 
_e_mod_comp_wl_shell_surface_type_reset(Wayland_Shell_Surface *wss)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (wss->type)
     {
      case SHELL_SURFACE_FULLSCREEN:
        wss->surface->x = wss->saved_x;
        wss->surface->y = wss->saved_y;
        break;
      case SHELL_SURFACE_PANEL:
      case SHELL_SURFACE_BACKGROUND:
        wl_list_remove(&wss->link);
        wl_list_init(&wss->link);
        break;
      case SHELL_SURFACE_SCREENSAVER:
      case SHELL_SURFACE_LOCK:
        wl_resource_post_error(&wss->resource, 
                               WL_DISPLAY_ERROR_INVALID_METHOD,
                               "Cannot reassign surface type");
        return EINA_FALSE;
        break;
      default:
        break;
     }

   wss->type = SHELL_SURFACE_NONE;
   return EINA_TRUE;
}

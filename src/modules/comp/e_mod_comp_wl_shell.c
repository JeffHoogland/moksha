#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_shell.h"
# include "e_mod_comp_wl_surface.h"
#endif

/* local function prototypes */
static void                   _e_mod_comp_wl_shell_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void                   _e_mod_comp_wl_shell_lock(Wayland_Shell *base);
static void                   _e_mod_comp_wl_shell_unlock(Wayland_Shell *base);
static void                   _e_mod_comp_wl_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height);
static void                   _e_mod_comp_wl_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static void                   _e_mod_comp_wl_shell_destroy(Wayland_Shell *base);
static void                   _e_mod_comp_wl_shell_activate(Wayland_Shell *base, Wayland_Surface *surface, uint32_t timestamp);

static void                   _e_mod_comp_wl_shell_shell_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource);
static void                   _e_mod_comp_wl_shell_surface_pong(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int serial);
static void                   _e_mod_comp_wl_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp);
static void                   _e_mod_comp_wl_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges);
static void                   _e_mod_comp_wl_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void                   _e_mod_comp_wl_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags __UNUSED__);
static void                   _e_mod_comp_wl_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource, uint32_t method __UNUSED__, uint32_t framerate __UNUSED__, struct wl_resource *output_resource __UNUSED__);
static void                   _e_mod_comp_wl_shell_surface_set_popup(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource  __UNUSED__, uint32_t timestamp  __UNUSED__, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags  __UNUSED__);
static void                   _e_mod_comp_wl_shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource __UNUSED__);
static void                   _e_mod_comp_wl_shell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title);
static void                   _e_mod_comp_wl_shell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *clas);

static void                   _e_mod_comp_wl_shell_surface_destroy_handle(struct wl_listener *listener, void *data __UNUSED__);
static Wayland_Shell_Surface *_e_mod_comp_wl_shell_get_shell_surface(Wayland_Surface *ws);
static void                   _e_mod_comp_wl_shell_surface_destroy(struct wl_resource *resource);

/* wayland interfaces */
static const struct wl_shell_interface _wl_shell_interface =
{
   _e_mod_comp_wl_shell_shell_surface_get
};
static const struct wl_shell_surface_interface _wl_shell_surface_interface =
{
   _e_mod_comp_wl_shell_surface_pong,
   _e_mod_comp_wl_shell_surface_move,
   _e_mod_comp_wl_shell_surface_resize,
   _e_mod_comp_wl_shell_surface_set_toplevel,
   _e_mod_comp_wl_shell_surface_set_transient,
   _e_mod_comp_wl_shell_surface_set_fullscreen,
   _e_mod_comp_wl_shell_surface_set_popup,
   _e_mod_comp_wl_shell_surface_set_maximized,
   _e_mod_comp_wl_shell_surface_set_title,
   _e_mod_comp_wl_shell_surface_set_class
};

/* private variables */
struct wl_shell *_wl_shell;

Eina_Bool
e_mod_comp_wl_shell_init(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_shell = malloc(sizeof(*_wl_shell))))
     {
        EINA_LOG_ERR("Could not allocate space for shell\n");
        return EINA_FALSE;
     }

   memset(_wl_shell, 0, sizeof(*_wl_shell));

   _wl_shell->shell.lock = _e_mod_comp_wl_shell_lock;
   _wl_shell->shell.unlock = _e_mod_comp_wl_shell_unlock;
   _wl_shell->shell.map = _e_mod_comp_wl_shell_map;
   _wl_shell->shell.configure = _e_mod_comp_wl_shell_configure;
   _wl_shell->shell.destroy = _e_mod_comp_wl_shell_destroy;

   if (!wl_display_add_global(_wl_disp, &wl_shell_interface, _wl_shell,
                              _e_mod_comp_wl_shell_bind))
     {
        EINA_LOG_ERR("Could not create shell\n");
        free(_wl_shell);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void
e_mod_comp_wl_shell_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_shell)
     _wl_shell->shell.destroy(&_wl_shell->shell);
}

struct wl_shell *
e_mod_comp_wl_shell_get(void)
{
   return _wl_shell;
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
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
}

static void
_e_mod_comp_wl_shell_unlock(Wayland_Shell *base)
{
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
}

static void
_e_mod_comp_wl_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height)
{
   Wayland_Compositor *comp;
   Wayland_Shell_Surface *wss;
   struct wl_shell *shell;
   struct wl_list *list;
   uint32_t type;
   Eina_Bool do_configure = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   type = SHELL_SURFACE_NONE;
   shell = container_of(base, struct wl_shell, shell);

   if ((wss = _e_mod_comp_wl_shell_get_shell_surface(surface)))
     type = wss->type;

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

   surface->w = width;
   surface->h = height;

   switch (type)
     {
      case SHELL_SURFACE_TOPLEVEL:
        surface->x = 10 + random() % 400;
        surface->y = 10 + random() % 400;
        break;

      default:
        wl_list_insert(list, &surface->link);
        break;
     }

   if (do_configure)
     e_mod_comp_wl_surface_configure(surface, surface->x, surface->y,
                                     surface->w, surface->h);

   switch (type)
     {
      case SHELL_SURFACE_TOPLEVEL:
      case SHELL_SURFACE_TRANSIENT:
      case SHELL_SURFACE_FULLSCREEN:
        if (!shell->locked)
          _e_mod_comp_wl_shell_activate(base, surface, e_mod_comp_wl_time_get());
        break;
     }
}

static void
_e_mod_comp_wl_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
   struct wl_shell *shell;
   Wayland_Shell_Surface *wss;
   Wayland_Shell_Surface_Type type;
   Eina_Bool do_configure = EINA_FALSE;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   type = SHELL_SURFACE_NONE;
   shell = container_of(base, struct wl_shell, shell);
   do_configure = !shell->locked;
   if ((wss = _e_mod_comp_wl_shell_get_shell_surface(surface)))
     type = wss->type;

   surface->w = width;
   surface->h = height;

   switch (type)
     {
      case SHELL_SURFACE_SCREENSAVER:
        do_configure = !do_configure;

      case SHELL_SURFACE_FULLSCREEN:
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
_e_mod_comp_wl_shell_activate(Wayland_Shell *base, Wayland_Surface *surface, uint32_t timestamp)
{
   struct wl_shell *shell;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
   e_mod_comp_wl_surface_activate(surface, e_mod_comp_wl_input_get(), timestamp);
}

static void
_e_mod_comp_wl_shell_shell_surface_get(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
   Wayland_Surface *ws;
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = surface_resource->data;
   if (_e_mod_comp_wl_shell_get_shell_surface(ws))
     {
        wl_resource_post_error(surface_resource,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "get_shell_surface already requested");
        return;
     }

   if (!(wss = calloc(1, sizeof(*wss))))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   wss->resource.destroy = _e_mod_comp_wl_shell_surface_destroy;
   wss->resource.object.id = id;
   wss->resource.object.interface = &wl_shell_surface_interface;
   wss->resource.object.implementation =
     (void (* *)(void)) & _wl_shell_surface_interface;
   wss->resource.data = wss;

   wss->surface = ws;
   wss->surface_destroy_listener.notify =
     _e_mod_comp_wl_shell_surface_destroy_handle;
   wl_signal_add(&ws->surface.resource.destroy_signal,
                 &wss->surface_destroy_listener);

   wl_list_init(&wss->link);

   wss->type = SHELL_SURFACE_NONE;
   wl_client_add_resource(client, &wss->resource);
}

static void
_e_mod_comp_wl_shell_surface_pong(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int serial)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   /* TODO: handle ping timer */
}

static void
_e_mod_comp_wl_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_e_mod_comp_wl_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);
}

static void
_e_mod_comp_wl_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;

   /* TODO: Surface type reset */

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

   /* TODO: Surface type reset */

   ws->x = pws->x + x;
   ws->y = pws->y + y;

   e_mod_comp_wl_surface_damage_surface(ws);

   wss->type = SHELL_SURFACE_TRANSIENT;
}

static void
_e_mod_comp_wl_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource, uint32_t method __UNUSED__, uint32_t framerate __UNUSED__, struct wl_resource *output_resource __UNUSED__)
{
   Wayland_Shell_Surface *wss;
   Wayland_Surface *ws;
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   ws = wss->surface;
   output = e_mod_comp_wl_output_get();

   wss->saved_x = ws->x;
   wss->saved_y = ws->y;
   ws->x = (output->w - ws->w) / 2;
   ws->y = (output->h - ws->h) / 2;
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

static void
_e_mod_comp_wl_shell_surface_set_maximized(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource __UNUSED__)
{
   Wayland_Shell_Surface *wss;
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   ws = wss->surface;
   /* FIXME: Implement */
}

static void
_e_mod_comp_wl_shell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   if (wss->title) free(wss->title);
   wss->title = strdup(title);
}

static void
_e_mod_comp_wl_shell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *clas)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;
   if (wss->clas) free(wss->clas);
   wss->clas = strdup(clas);
}

static void
_e_mod_comp_wl_shell_surface_destroy_handle(struct wl_listener *listener, void *data __UNUSED__)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = container_of(listener, Wayland_Shell_Surface, surface_destroy_listener);
   wss->surface = NULL;
   wl_resource_destroy(&wss->resource);
}

static Wayland_Shell_Surface *
_e_mod_comp_wl_shell_get_shell_surface(Wayland_Surface *ws)
{
   struct wl_listener *listener;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   listener =
     wl_signal_get(&ws->surface.resource.destroy_signal,
                   _e_mod_comp_wl_shell_surface_destroy_handle);
   if (listener)
     return container_of(listener, Wayland_Shell_Surface,
                         surface_destroy_listener);
   return NULL;
}

static void
_e_mod_comp_wl_shell_surface_destroy(struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wss = resource->data;

   /* TODO: popup grab input */

   if (wss->surface) wl_list_remove(&wss->surface_destroy_listener.link);
   wl_list_remove(&wss->link);
   free(wss);
}


#include "e.h"
#include "e_mod_main.h"

/* local function prototypes */
static void _shell_cb_lock(E_Shell *base);
static void _shell_cb_unlock(E_Shell *base);
/* static void _shell_cb_map(E_Shell *base, E_Surface *surface, int w, int h, int sx, int sy); */
/* static void _shell_cb_configure(E_Shell *base, E_Surface *surface, GLfloat x, GLfloat y, int w, int h); */
static void _shell_cb_destroy(E_Shell *base);
static void _shell_cb_destroy_shell_surface(struct wl_resource *resource);
static void _shell_cb_bind(struct wl_client *client, void *data, unsigned int version __UNUSED__, unsigned int id);
static void _shell_cb_bind_desktop(struct wl_client *client, void *data, unsigned int version __UNUSED__, unsigned int id);
static void _shell_cb_unbind_desktop(struct wl_resource *resource);
static void _shell_cb_get_shell_surface(struct wl_client *client, struct wl_resource *resource, unsigned int id, struct wl_resource *surface_resource);
static void _shell_cb_handle_surface_destroy(struct wl_listener *listener, void *data __UNUSED__);
static void _shell_cb_handle_lock_surface_destroy(struct wl_listener *listener, void *data __UNUSED__);
/* static void _shell_cb_activate(E_Shell *base, E_Surface *es, E_Input_Device *eid, unsigned int timestamp); */

static void _shell_cb_desktop_set_background(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource);
static void _shell_cb_desktop_set_panel(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource);
static void _shell_cb_desktop_set_lock_surface(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *surface_resource);
static void _shell_cb_desktop_unlock(struct wl_client *client __UNUSED__, struct wl_resource *resource);

static void _shell_cb_shell_surface_pong(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int serial);
static void _shell_cb_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, unsigned int timestamp);
static void _shell_cb_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, unsigned int timestamp, unsigned int edges);
static void _shell_cb_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _shell_cb_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int x, int y, unsigned int flags __UNUSED__);
static void _shell_cb_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int method, unsigned int framerate, struct wl_resource *output_resource);
static void _shell_cb_shell_surface_set_popup(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_device_resource __UNUSED__, unsigned int timestamp __UNUSED__, struct wl_resource *parent_resource, int x, int y, unsigned int flags __UNUSED__);
static void _shell_cb_shell_surface_set_maximized(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource);

static E_Shell_Surface *_shell_get_shell_surface(E_Surface *es);
static int _shell_reset_shell_surface_type(E_Shell_Surface *ess);
/* static void _shell_center_on_output(E_Surface *es, E_Output *output); */
/* static void _shell_map_fullscreen(E_Shell_Surface *ess); */
/* static void _shell_configure_fullscreen(E_Shell_Surface *ess); */
/* static void _shell_stack_fullscreen(E_Shell_Surface *ess); */
static void _shell_unset_fullscreen(E_Shell_Surface *ess);
/* static struct wl_shell *_shell_surface_get_shell(E_Shell_Surface *ess); */
/* static E_Surface *_shell_create_black_surface(E_Compositor *comp, GLfloat x, GLfloat y, int w, int h); */
/* static E_Shell_Surface_Type _shell_get_shell_surface_type(E_Surface *es); */

/* static void _shell_map_popup(E_Shell_Surface *ess, unsigned int timestamp __UNUSED__); */
static void _shell_cb_popup_grab_focus(struct wl_pointer_grab *grab, struct wl_surface *surface, int x, int y);
static void _shell_cb_popup_grab_motion(struct wl_pointer_grab *grab, unsigned int timestamp, int sx, int sy);
static void _shell_cb_popup_grab_button(struct wl_pointer_grab *grab, unsigned int timestamp, unsigned int button, int state);

/* local variables */

/* wayland interfaces */
static const struct wl_shell_interface _e_shell_interface = 
{
   _shell_cb_get_shell_surface
};
static const struct desktop_shell_interface _e_desktop_shell_interface = 
{
   _shell_cb_desktop_set_background,
   _shell_cb_desktop_set_panel,
   _shell_cb_desktop_set_lock_surface,
   _shell_cb_desktop_unlock
};
static const struct wl_shell_surface_interface _e_shell_surface_interface = 
{
   _shell_cb_shell_surface_pong,
   _shell_cb_shell_surface_move,
   _shell_cb_shell_surface_resize,
   _shell_cb_shell_surface_set_toplevel,
   _shell_cb_shell_surface_set_transient,
   _shell_cb_shell_surface_set_fullscreen,
   _shell_cb_shell_surface_set_popup,
   _shell_cb_shell_surface_set_maximized
};
static const struct wl_pointer_grab_interface _e_popup_grab_interface = 
{
   _shell_cb_popup_grab_focus,
   _shell_cb_popup_grab_motion,
   _shell_cb_popup_grab_button,
};

/* external variables */

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Shell" };

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Compositor *comp;
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(comp = m->data)) return NULL;

   if (!(shell = malloc(sizeof(*shell)))) return NULL;

   memset(shell, 0, sizeof(*shell));

   shell->compositor = comp;
   shell->shell.lock = _shell_cb_lock;
   shell->shell.unlock = _shell_cb_unlock;
   /* shell->shell.map = _shell_cb_map; */
   /* shell->shell.configure = _shell_cb_configure; */
   shell->shell.destroy = _shell_cb_destroy;

   wl_list_init(&shell->backgrounds);
   wl_list_init(&shell->panels);

   e_layer_init(&shell->fullscreen_layer, &comp->cursor_layer.link);
   e_layer_init(&shell->panel_layer, &shell->fullscreen_layer.link);
   e_layer_init(&shell->toplevel_layer, &shell->panel_layer.link);
   e_layer_init(&shell->background_layer, &shell->toplevel_layer.link);

   wl_list_init(&shell->lock_layer.surfaces);

   if (!wl_display_add_global(comp->display, &wl_shell_interface, 
                              shell, _shell_cb_bind))
     return NULL;

   if (!wl_display_add_global(comp->display, &desktop_shell_interface, 
                              shell, _shell_cb_bind_desktop))
     return NULL;

   comp->shell = &shell->shell;

   /* m->data = &shell->shell; */

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__)
{
   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   return 1;
}

/* local functions */
static void 
_shell_cb_lock(E_Shell *base)
{
   struct wl_shell *shell;
   unsigned int timestamp;
   E_Input_Device *device;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);

   if (shell->locked)
     {
        E_Output *output;

        wl_list_for_each(output, &shell->compositor->outputs, link)
          if (output->set_dpms) output->set_dpms(output, E_DPMS_STANDBY);

        return;
     }

   shell->locked = EINA_TRUE;

   wl_list_remove(&shell->panel_layer.link);
   wl_list_remove(&shell->toplevel_layer.link);
   wl_list_remove(&shell->fullscreen_layer.link);
   wl_list_insert(&shell->compositor->cursor_layer.link, 
                  &shell->lock_layer.link);

   /* TODO: launch screensaver */
   /* TODO: shell screensaver */
   /* TODO: loop screensaver surfaces */

   e_compositor_schedule_repaint(shell->compositor);

   timestamp = e_compositor_get_time();
   wl_list_for_each(device, &shell->compositor->inputs, link)
     wl_input_device_set_keyboard_focus(&device->input_device, NULL);
}

static void 
_shell_cb_unlock(E_Shell *base)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
   if ((!shell->locked) || (shell->lock_surface))
     {
        e_compositor_wake(shell->compositor);
        return;
     }

   /* TODO: handle desktop shell going away ? */

   if (shell->prepare_event_sent) return;

   /* TODO: handle desktop_shell_send_prepare_lock_surface */
   /* desktop_shell_send_prepare_lock_surface(shell->); */

   shell->prepare_event_sent = EINA_TRUE;
}

/* static void  */
/* _shell_cb_map(E_Shell *base, E_Surface *surface, int w, int h, int sx, int sy) */
/* { */
/*    struct wl_shell *shell; */
/*    E_Compositor *comp; */
/*    E_Shell_Surface *ess; */
/*    E_Shell_Surface_Type type = E_SHELL_SURFACE_NONE; */
/*    E_Surface *parent; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    shell = container_of(base, struct wl_shell, shell); */
/*    comp = shell->compositor; */

/*    if ((ess = _shell_get_shell_surface(surface))) */
/*      type = ess->type; */

/*    surface->geometry.w = w; */
/*    surface->geometry.h = h; */
/*    surface->geometry.dirty = EINA_TRUE; */

/*    switch (type) */
/*      { */
/*       case E_SHELL_SURFACE_TOPLEVEL: */
/*         e_surface_set_position(surface, surface->geometry.x, surface->geometry.y); */
/*         break; */
/*       case E_SHELL_SURFACE_SCREENSAVER: */
/*         _shell_center_on_output(surface, ess->fullscreen_output); */
/*         break; */
/*       case E_SHELL_SURFACE_FULLSCREEN: */
/*         _shell_map_fullscreen(ess); */
/*         break; */
/*       case E_SHELL_SURFACE_MAXIMIZED: */
/*         e_surface_set_position(surface, surface->output->x, surface->output->y); */
/*         break; */
/*       case E_SHELL_SURFACE_LOCK: */
/*         _shell_center_on_output(surface, e_output_get_default(comp)); */
/*         break; */
/*       case E_SHELL_SURFACE_POPUP: */
/*         _shell_map_popup(ess, ess->popup.timestamp); */
/*         break; */
/*       case E_SHELL_SURFACE_NONE: */
/*         e_surface_set_position(surface, surface->geometry.x + sx,  */
/*                                surface->geometry.y + sy); */
/*         break; */
/*       default: */
/*         break; */
/*      } */

/*    switch (type) */
/*      { */
/*       case E_SHELL_SURFACE_BACKGROUND: */
/*         wl_list_insert(&shell->background_layer.surfaces, &surface->layers); */
/*         break; */
/*       case E_SHELL_SURFACE_PANEL: */
/*         wl_list_insert(&shell->panel_layer.surfaces, &surface->layers); */
/*         break; */
/*       case E_SHELL_SURFACE_LOCK: */
/*         wl_list_insert(&shell->lock_layer.surfaces, &surface->layers); */
/*         e_compositor_wake(comp); */
/*         break; */
/*       case E_SHELL_SURFACE_SCREENSAVER: */
/*         if (shell->locked) */
/*           { */
/*              e_compositor_wake(comp); */
/*              if (!shell->lock_surface) */
/*                comp->state = E_COMPOSITOR_STATE_IDLE; */
/*           } */
/*         break; */
/*       case E_SHELL_SURFACE_POPUP: */
/*       case E_SHELL_SURFACE_TRANSIENT: */
/*         parent = ess->parent->surface; */
/*         wl_list_insert(parent->layers.prev, &surface->layers); */
/*         break; */
/*       case E_SHELL_SURFACE_FULLSCREEN: */
/*       case E_SHELL_SURFACE_NONE: */
/*         break; */
/*       default: */
/*         wl_list_insert(&shell->toplevel_layer.surfaces, &surface->layers); */
/*         break; */
/*      } */

/*    if (type != E_SHELL_SURFACE_NONE) */
/*      { */
/*         e_surface_assign_output(surface); */
/*         if (type == E_SHELL_SURFACE_MAXIMIZED) */
/*           surface->output = ess->output; */
/*      } */

/*    switch (type) */
/*      { */
/*       case E_SHELL_SURFACE_TOPLEVEL: */
/*       case E_SHELL_SURFACE_TRANSIENT: */
/*       case E_SHELL_SURFACE_FULLSCREEN: */
/*       case E_SHELL_SURFACE_MAXIMIZED: */
/*         if (!shell->locked) */
/*           _shell_cb_activate(base, surface,  */
/*                              (E_Input_Device *)comp->input_device,  */
/*                              e_compositor_get_time()); */
/*         break; */
/*       default: */
/*         break; */
/*      } */

/*    if (type == E_SHELL_SURFACE_TOPLEVEL) */
/*      { */
/*      } */
/* } */

/* static void  */
/* _shell_cb_configure(E_Shell *base, E_Surface *surface, GLfloat x, GLfloat y, int w, int h) */
/* { */
/*    struct wl_shell *shell; */
/*    E_Shell_Surface_Type type = E_SHELL_SURFACE_NONE; */
/*    E_Shell_Surface_Type ptype = E_SHELL_SURFACE_NONE; */
/*    E_Shell_Surface *ess; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    shell = container_of(base, struct wl_shell, shell); */

/*    if ((ess = _shell_get_shell_surface(surface))) */
/*      type = ess->type; */

/*    surface->geometry.x = x; */
/*    surface->geometry.y = y; */
/*    surface->geometry.w = w; */
/*    surface->geometry.h = h; */
/*    surface->geometry.dirty = EINA_TRUE; */

/*    switch (type) */
/*      { */
/*       case E_SHELL_SURFACE_SCREENSAVER: */
/*         _shell_center_on_output(surface, ess->fullscreen_output); */
/*         break; */
/*       case E_SHELL_SURFACE_FULLSCREEN: */
/*         _shell_configure_fullscreen(ess); */
/*         if (ptype != E_SHELL_SURFACE_FULLSCREEN) */
/*           _shell_stack_fullscreen(ess); */
/*         break; */
/*       case E_SHELL_SURFACE_MAXIMIZED: */
/*         surface->geometry.x = surface->output->x; */
/*         surface->geometry.y = surface->output->y; */
/*         break; */
/*       case E_SHELL_SURFACE_TOPLEVEL: */
/*         break; */
/*       default: */
/*         break; */
/*      } */

/*    if (surface->output) */
/*      { */
/*         e_surface_assign_output(surface); */
/*         if (type == E_SHELL_SURFACE_SCREENSAVER) */
/*           surface->output = ess->output; */
/*         else if (type == E_SHELL_SURFACE_MAXIMIZED) */
/*           surface->output = ess->output; */
/*      } */
/* } */

static void 
_shell_cb_destroy(E_Shell *base)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(base, struct wl_shell, shell);
   free(shell);
}

static void 
_shell_cb_destroy_shell_surface(struct wl_resource *resource)
{
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   if (ess->popup.grab.input_device)
     wl_input_device_end_pointer_grab(ess->popup.grab.input_device);
   if (ess->surface)
     wl_list_remove(&ess->surface_destroy_listener.link);
   if (ess->fullscreen.black_surface)
     e_surface_destroy(ess->fullscreen.black_surface);
   wl_list_remove(&ess->link);
   free(ess);
}

static void 
_shell_cb_bind(struct wl_client *client, void *data, unsigned int version __UNUSED__, unsigned int id)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(shell = data)) return;
   wl_client_add_object(client, &wl_shell_interface, 
                        &_e_shell_interface, id, shell);
}

static void 
_shell_cb_bind_desktop(struct wl_client *client, void *data, unsigned int version __UNUSED__, unsigned int id)
{
   struct wl_shell *shell;
   struct wl_resource *resource;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(shell = data)) return;
   resource = wl_client_add_object(client, &desktop_shell_interface, 
                                   &_e_desktop_shell_interface, id, shell);
   resource->destroy = _shell_cb_unbind_desktop;
}

static void 
_shell_cb_unbind_desktop(struct wl_resource *resource)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = resource->data;
   if (shell->locked) 
     {
        /* TODO: resume desktop */
     }
   shell->prepare_event_sent = EINA_FALSE;
   free(resource);
}

static void 
_shell_cb_get_shell_surface(struct wl_client *client, struct wl_resource *resource, unsigned int id, struct wl_resource *surface_resource)
{
   E_Surface *es;
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   es = surface_resource->data;

   if (_shell_get_shell_surface(es))
     {
        wl_resource_post_error(surface_resource, 
                               WL_DISPLAY_ERROR_INVALID_OBJECT, 
                               "wl_shell::shell_surface already requested");
        return;
     }

   if (!(ess = calloc(1, sizeof(E_Shell_Surface)))) 
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   ess->resource.destroy = _shell_cb_destroy_shell_surface;
   ess->resource.object.id = id;
   ess->resource.object.interface = &wl_shell_surface_interface;
   ess->resource.object.implementation = 
     (void (**)(void)) &_e_shell_surface_interface;
   ess->resource.data = ess;

   ess->saved_pos_valid = EINA_FALSE;
   ess->surface = es;
   ess->fullscreen.type = WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT;
   ess->fullscreen.framerate = 0;
   ess->fullscreen.black_surface = NULL;
   wl_list_init(&ess->fullscreen.transform.link);

   ess->surface_destroy_listener.notify = _shell_cb_handle_surface_destroy;
   wl_signal_add(&es->surface.resource.destroy_signal,
                 &ess->surface_destroy_listener);

   wl_list_init(&ess->link);
   wl_list_init(&ess->rotation.transform.link);
   e_matrix_init(&ess->rotation.rotation);

   ess->type = E_SHELL_SURFACE_NONE;

   wl_client_add_resource(client, &ess->resource);
}

static void 
_shell_cb_handle_surface_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = container_of(listener, E_Shell_Surface, surface_destroy_listener);
   ess->surface = NULL;
   wl_resource_destroy(&ess->resource);
}

static void 
_shell_cb_handle_lock_surface_destroy(struct wl_listener *listener, void *data __UNUSED__)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = container_of(listener, struct wl_shell, lock_surface_listener);
   shell->lock_surface = NULL;
}

/* static void  */
/* _shell_cb_activate(E_Shell *base, E_Surface *es, E_Input_Device *eid, unsigned int timestamp) */
/* { */
/*    struct wl_shell *shell; */
/*    E_Compositor *comp; */
/*    E_Shell_Surface_Type type = E_SHELL_SURFACE_NONE; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    shell = container_of(base, struct wl_shell, shell); */
/*    comp = shell->compositor; */

/*    e_surface_activate(es, eid, timestamp); */

/*    type = _shell_get_shell_surface_type(es); */
/*    switch (type) */
/*      { */
/*       case E_SHELL_SURFACE_BACKGROUND: */
/*       case E_SHELL_SURFACE_PANEL: */
/*       case E_SHELL_SURFACE_LOCK: */
/*         break; */
/*       case E_SHELL_SURFACE_SCREENSAVER: */
/*         if (shell->lock_surface) */
/*           e_surface_restack(es, &shell->lock_surface->surface->layers); */
/*         break; */
/*       case E_SHELL_SURFACE_FULLSCREEN: */
/*         break; */
/*       default: */
/*         e_surface_restack(es, &shell->toplevel_layer.surfaces); */
/*         break; */
/*      } */
/* } */

static void 
_shell_cb_desktop_set_background(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource)
{
   struct wl_shell *shell;
   E_Shell_Surface *ess, *priv;
   E_Surface *es;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = resource->data;
   ess = surface_resource->data;
   es = ess->surface;

   if (_shell_reset_shell_surface_type(ess)) return;

   wl_list_for_each(priv, &shell->backgrounds, link)
     {
        if (priv->output == output_resource->data)
          {
             priv->surface->output = NULL;
             wl_list_remove(&priv->surface->layers);
             wl_list_remove(&priv->link);
             break;
          }
     }

   ess->type = E_SHELL_SURFACE_BACKGROUND;
   ess->output = output_resource->data;
   wl_list_insert(&shell->backgrounds, &ess->link);

   e_surface_set_position(es, ess->output->x, ess->output->y);

   desktop_shell_send_configure(resource, e_compositor_get_time(), 0, 
                                surface_resource, ess->output->current->w, 
                                ess->output->current->h);
}

static void 
_shell_cb_desktop_set_panel(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource, struct wl_resource *surface_resource)
{
   struct wl_shell *shell;
   E_Shell_Surface *ess, *priv;
   E_Surface *es;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = resource->data;
   ess = surface_resource->data;
   es = ess->surface;

   if (_shell_reset_shell_surface_type(ess)) return;

   wl_list_for_each(priv, &shell->panels, link)
     {
        if (priv->output == output_resource->data)
          {
             priv->surface->output = NULL;
             wl_list_remove(&priv->surface->layers);
             wl_list_remove(&priv->link);
             break;
          }
     }

   ess->type = E_SHELL_SURFACE_PANEL;
   ess->output = output_resource->data;

   wl_list_insert(&shell->panels, &ess->link);

   e_surface_set_position(es, ess->output->x, ess->output->y);

   desktop_shell_send_configure(resource, e_compositor_get_time(), 0, 
                                surface_resource, ess->output->current->w, 
                                ess->output->current->h);
}

static void 
_shell_cb_desktop_set_lock_surface(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *surface_resource)
{
   struct wl_shell *shell;
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = resource->data;
   ess = surface_resource->data;

   if (_shell_reset_shell_surface_type(ess)) return;

   shell->prepare_event_sent = EINA_FALSE;
   if (!shell->locked) return;

   shell->lock_surface = ess;
   shell->lock_surface_listener.notify = _shell_cb_handle_lock_surface_destroy;
   wl_signal_add(&surface_resource->destroy_signal,
                 &shell->lock_surface_listener);

   shell->lock_surface->type = E_SHELL_SURFACE_LOCK;
}

static void 
_shell_cb_desktop_unlock(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   struct wl_shell *shell;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   shell = resource->data;
   shell->prepare_event_sent = EINA_FALSE;
   if (shell->locked)
     {
        /* TODO: resume desktop */
     }
}

static void 
_shell_cb_shell_surface_pong(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int serial)
{
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   /* TODO: handle ping timer */
}

static void 
_shell_cb_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, unsigned int timestamp)
{
   E_Input_Device *eid;
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   eid = input_resource->data;
   ess = resource->data;

   if ((eid->input_device.button_count == 0) || 
       (eid->input_device.grab_time != timestamp) || 
       (eid->input_device.pointer_focus != &ess->surface->surface))
     return;

   if (!e_surface_move(ess->surface, eid, timestamp))
     wl_resource_post_no_memory(resource);
}

static void 
_shell_cb_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, unsigned int timestamp, unsigned int edges)
{
   E_Input_Device *eid;
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   eid = input_resource->data;
   ess = resource->data;

   if (ess->type == E_SHELL_SURFACE_FULLSCREEN) return;

   if ((eid->input_device.button_count == 0) || 
       (eid->input_device.grab_time != timestamp) || 
       (eid->input_device.pointer_focus != &ess->surface->surface))
     return;

   if (!e_surface_resize(ess->surface, eid, timestamp, edges))
     wl_resource_post_no_memory(resource);
}

static void 
_shell_cb_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;

   if (_shell_reset_shell_surface_type(ess)) return;
   ess->type = E_SHELL_SURFACE_TOPLEVEL;
}

static void 
_shell_cb_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int x, int y, unsigned int flags __UNUSED__)
{
   E_Shell_Surface *ess, *pess;
   E_Surface *es, *pes;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   es = ess->surface;
   pess = parent_resource->data;
   pes = pess->surface;

   if (_shell_reset_shell_surface_type(ess)) return;

   ess->output = pes->output;
   e_surface_set_position(es, pes->geometry.x + x, pes->geometry.y + y);
   ess->type = E_SHELL_SURFACE_TRANSIENT;
}

static void 
_shell_cb_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource, unsigned int method, unsigned int framerate, struct wl_resource *output_resource)
{
   E_Shell_Surface *ess;
   E_Surface *es;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   es = ess->surface;

   if (output_resource)
     ess->output = output_resource->data;
   else
     ess->output = e_output_get_default(es->compositor);

   if (_shell_reset_shell_surface_type(ess)) return;

   ess->fullscreen_output = ess->output;
   ess->fullscreen.type = method;
   ess->fullscreen.framerate = framerate;
   ess->type = E_SHELL_SURFACE_FULLSCREEN;
   ess->sx = es->geometry.x;
   ess->sy = es->geometry.y;
   ess->saved_pos_valid = EINA_TRUE;

   if (es->output) ess->surface->force_configure = EINA_TRUE;

   wl_shell_surface_send_configure(&ess->resource, 0, 
                                   ess->output->current->w, 
                                   ess->output->current->h);
}

static void 
_shell_cb_shell_surface_set_popup(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_device_resource __UNUSED__, unsigned int timestamp __UNUSED__, struct wl_resource *parent_resource, int x, int y, unsigned int flags __UNUSED__)
{
   E_Shell_Surface *ess;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   ess->type = E_SHELL_SURFACE_POPUP;
   ess->parent = parent_resource->data;
   ess->popup.x = x;
   ess->popup.y = y;
}

static void 
_shell_cb_shell_surface_set_maximized(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *output_resource)
{
   E_Shell_Surface *ess;
   E_Surface *es;
   /* struct wl_shell *shell = NULL; */
   unsigned int edges = 0;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = resource->data;
   es = ess->surface;

   if (output_resource)
     ess->output = output_resource->data;
   else 
     ess->output = e_output_get_default(es->compositor);

   if (_shell_reset_shell_surface_type(ess)) return;

   ess->sx = es->geometry.x;
   ess->sy = es->geometry.y;
   ess->saved_pos_valid = EINA_TRUE;

   /* TODO: handle getting panel size ?? */

   edges = WL_SHELL_SURFACE_RESIZE_TOP | WL_SHELL_SURFACE_RESIZE_LEFT;
   wl_shell_surface_send_configure(&ess->resource, edges, 
                                   es->output->current->w, 
                                   es->output->current->h);
   ess->type = E_SHELL_SURFACE_MAXIMIZED;
}

static E_Shell_Surface *
_shell_get_shell_surface(E_Surface *es)
{
   struct wl_listener *listener;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   listener = wl_signal_get(&es->surface.resource.destroy_signal, 
                            _shell_cb_handle_surface_destroy);
   if (listener)
     return container_of(listener, E_Shell_Surface, surface_destroy_listener);

   return NULL;
}

static int 
_shell_reset_shell_surface_type(E_Shell_Surface *ess)
{
   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (ess->type)
     {
      case E_SHELL_SURFACE_FULLSCREEN:
        _shell_unset_fullscreen(ess);
        break;
      case E_SHELL_SURFACE_MAXIMIZED:
        ess->output = e_output_get_default(ess->surface->compositor);
        e_surface_set_position(ess->surface, ess->sx, ess->sy);
        break;
      case E_SHELL_SURFACE_PANEL:
      case E_SHELL_SURFACE_BACKGROUND:
        wl_list_remove(&ess->link);
        wl_list_init(&ess->link);
        break;
      case E_SHELL_SURFACE_SCREENSAVER:
      case E_SHELL_SURFACE_LOCK:
        wl_resource_post_error(&ess->resource, 
                               WL_DISPLAY_ERROR_INVALID_METHOD, 
                               "cannot reassign surface type");
        return -1;
        break;
      case E_SHELL_SURFACE_NONE:
      case E_SHELL_SURFACE_TOPLEVEL:
      case E_SHELL_SURFACE_TRANSIENT:
      case E_SHELL_SURFACE_POPUP:
        break;
     }

   ess->type = E_SHELL_SURFACE_NONE;
   return 0;
}

/* static void  */
/* _shell_center_on_output(E_Surface *es, E_Output *output) */
/* { */
/*    E_Output_Mode *mode; */
/*    GLfloat x, y; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    mode = output->current; */
/*    x = (mode->w - es->geometry.w) / 2; */
/*    y = (mode->h - es->geometry.h) / 2; */
/*    e_surface_set_position(es, output->x + x, output->y + y); */
/* } */

/* static void  */
/* _shell_map_fullscreen(E_Shell_Surface *ess) */
/* { */
/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    _shell_configure_fullscreen(ess); */
/*    _shell_stack_fullscreen(ess); */
/* } */

/* static void  */
/* _shell_configure_fullscreen(E_Shell_Surface *ess) */
/* { */
/*    E_Output *output; */
/*    E_Surface *es; */
/*    E_Matrix *matrix; */
/*    float scale; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    output = ess->fullscreen_output; */
/*    es = ess->surface; */

/*    _shell_center_on_output(es, output); */

/*    if (!ess->fullscreen.black_surface) */
/*      { */
/*         ess->fullscreen.black_surface =  */
/*           _shell_create_black_surface(es->compositor, output->x, output->y,  */
/*                                      output->current->w, output->current->h); */
/*      } */

/*    wl_list_remove(&ess->fullscreen.black_surface->layers); */
/*    wl_list_insert(&es->layers, &ess->fullscreen.black_surface->layers); */
/*    ess->fullscreen.black_surface->output = output; */

/*    switch (ess->fullscreen.type) */
/*      { */
/*       case WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT: */
/*         break; */
/*       case WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE: */
/*         matrix = &ess->fullscreen.transform.matrix; */
/*         e_matrix_init(matrix); */
/*         scale = (float)output->current->w / (float)es->geometry.w; */
/*         e_matrix_scale(matrix, scale, scale, 1); */
/*         wl_list_remove(&ess->fullscreen.transform.link); */
/*         wl_list_insert(es->geometry.transforms.prev,  */
/*                        &ess->fullscreen.transform.link); */
/*         e_surface_set_position(es, output->x, output->y); */
/*         break; */
/*       case WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER: */
/*         break; */
/*       case WL_SHELL_SURFACE_FULLSCREEN_METHOD_FILL: */
/*         break; */
/*       default: */
/*         break; */
/*      } */
/* } */

/* static void  */
/* _shell_stack_fullscreen(E_Shell_Surface *ess) */
/* { */
/*    E_Surface *es; */
/*    struct wl_shell *shell; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    es = ess->surface; */
/*    shell = _shell_surface_get_shell(ess); */

/*    wl_list_remove(&es->layers); */
/*    wl_list_remove(&ess->fullscreen.black_surface->layers); */

/*    wl_list_insert(&shell->fullscreen_layer.surfaces, &es->layers); */
/*    wl_list_insert(&es->layers, &ess->fullscreen.black_surface->layers); */

/*    e_surface_damage(es); */
/*    e_surface_damage(ess->fullscreen.black_surface); */
/* } */

static void 
_shell_unset_fullscreen(E_Shell_Surface *ess)
{
   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess->fullscreen.type = WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT;
   ess->fullscreen.framerate = 0;
   wl_list_remove(&ess->fullscreen.transform.link);
   wl_list_init(&ess->fullscreen.transform.link);
   e_surface_destroy(ess->fullscreen.black_surface);
   ess->fullscreen.black_surface = NULL;
   ess->fullscreen_output = NULL;
   ess->surface->force_configure = EINA_TRUE;
   e_surface_set_position(ess->surface, ess->sx, ess->sy);
}

/* static struct wl_shell * */
/* _shell_surface_get_shell(E_Shell_Surface *ess) */
/* { */
/*    E_Surface *es; */
/*    E_Shell *shell; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    es = ess->surface; */
/*    shell = es->compositor->shell; */
/*    return (struct wl_shell *)container_of(shell, struct wl_shell, shell); */
/* } */

/* static E_Surface * */
/* _shell_create_black_surface(E_Compositor *comp, GLfloat x, GLfloat y, int w, int h) */
/* { */
/*    E_Surface *es = NULL; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    if (!(es = e_surface_create(comp))) */
/*      return NULL; */

/*    e_surface_configure(es, x, y, w, h); */
/*    e_surface_set_color(es, 0.0, 0.0, 0.0, 1); */

/*    return es; */
/* } */

/* static E_Shell_Surface_Type  */
/* _shell_get_shell_surface_type(E_Surface *es) */
/* { */
/*    E_Shell_Surface *ess; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    if (!(ess = _shell_get_shell_surface(es))) */
/*      return E_SHELL_SURFACE_NONE; */

/*    return ess->type; */
/* } */

/* static void  */
/* _shell_map_popup(E_Shell_Surface *ess, unsigned int timestamp __UNUSED__) */
/* { */
/*    struct wl_input_device *device; */
/*    E_Surface *es, *parent; */

/*    SLOGFN(__FILE__, __LINE__, __FUNCTION__); */

/*    es = ess->surface; */
/*    parent = ess->parent->surface; */
/*    es->output = parent->output; */
/*    device = es->compositor->input_device; */

/*    ess->popup.grab.interface = &_e_popup_grab_interface; */
/*    e_surface_update_transform(parent); */
/*    if (parent->transform.enabled) */
/*      ess->popup.parent_transform.matrix = parent->transform.matrix; */
/*    else */
/*      { */
/*         e_matrix_init(&ess->popup.parent_transform.matrix); */
/*         ess->popup.parent_transform.matrix.d[12] = parent->geometry.x; */
/*         ess->popup.parent_transform.matrix.d[13] = parent->geometry.y; */
/*      } */

/*    wl_list_insert(es->geometry.transforms.prev,  */
/*                   &ess->popup.parent_transform.link); */
/*    e_surface_set_position(es, ess->popup.x, ess->popup.y); */

/*    ess->popup.grab.input_device = device; */
/*    ess->popup.timestamp = device->grab_time; */
/*    ess->popup.initial_up = EINA_FALSE; */

/*    wl_input_device_start_pointer_grab(ess->popup.grab.input_device,  */
/*                                       &ess->popup.grab); */
/* } */

static void 
_shell_cb_popup_grab_focus(struct wl_pointer_grab *grab, struct wl_surface *surface, int x, int y)
{
   struct wl_input_device *device;
   E_Shell_Surface *priv;
   struct wl_client *client;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   device = grab->input_device;
   priv = container_of(grab, E_Shell_Surface, popup.grab);
   client = priv->surface->surface.resource.client;

   if ((surface) && (surface->resource.client == client))
     {
        wl_input_device_set_pointer_focus(device, surface, x, y);
        grab->focus = surface;
     }
   else
     {
        wl_input_device_set_pointer_focus(device, NULL, 0, 0);
        grab->focus = NULL;
     }
}

static void 
_shell_cb_popup_grab_motion(struct wl_pointer_grab *grab, unsigned int timestamp, int sx, int sy)
{
   /* struct wl_input_device *device; */
   /* E_Surface_Move_Grab *move; */
   /* E_Surface *es; */

   /* device = grab->input_device; */
   /* move = (E_Surface_Move_Grab *)grab; */

   /* es = move->surface; */
   /* e_surface_configure(es, device->x + move->dx, device->y + move->dy,  */
   /*                     es->geometry.w, es->geometry.h); */

   struct wl_resource *resource;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((resource = grab->input_device->pointer_focus_resource))
     wl_input_device_send_motion(resource, timestamp, sx, sy);
}

static void 
_shell_cb_popup_grab_button(struct wl_pointer_grab *grab, unsigned int timestamp, unsigned int button, int state)
{
   /* struct wl_input_device *device; */

   /* device = grab->input_device; */
   /* if ((device->button_count == 0) && (state == 0)) */
   /*   { */
   /*      wl_input_device_end_pointer_grab(device, timestamp); */
   /*      free(grab); */
   /*   } */

   struct wl_resource *resource;
   E_Shell_Surface *ess = NULL;

   SLOGFN(__FILE__, __LINE__, __FUNCTION__);

   ess = container_of(grab, E_Shell_Surface, popup.grab);

   if ((resource = grab->input_device->pointer_focus_resource))
     {
        struct wl_display *disp;
        unsigned int serial = 0;

        disp = wl_client_get_display(resource->client);
        serial = wl_display_get_serial(disp);
        wl_input_device_send_button(resource, serial, timestamp, button, state);
     }
   else if ((state == 0) && 
            ((ess->popup.initial_up) || 
                (timestamp - ess->popup.timestamp > 500)))
     {
        wl_shell_surface_send_popup_done(&ess->resource);
        wl_input_device_end_pointer_grab(grab->input_device);
        ess->popup.grab.input_device = NULL;
     }

   if (state == 0) ess->popup.initial_up = EINA_TRUE;
}
